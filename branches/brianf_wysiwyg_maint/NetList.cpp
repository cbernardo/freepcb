// NetList.cpp: implementation of the CNetList class.
//

#include "stdafx.h"
#include <math.h>
#include <stdlib.h>
#include "DlgMyMessageBox.h"
#include "gerber.h"
#include "utility.h"
#include "php_polygon.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//#define PROFILE		// profiles calls to OptimizeConnections() for "GND"

BOOL bDontShowSelfIntersectionWarning = FALSE;
BOOL bDontShowSelfIntersectionArcsWarning = FALSE;
BOOL bDontShowIntersectionWarning = FALSE;
BOOL bDontShowIntersectionArcsWarning = FALSE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// carea constructor
carea::carea()
{
	m_dlist = 0;
	npins = 0;
	nvias = 0;
	poly = 0;
	utility = 0;
}

void carea::Initialize( CDisplayList * dlist )
{
	m_dlist = dlist;
	poly = new CPolyLine( m_dlist );
}

// carea destructor
carea::~carea()
{
	delete poly;
}

// carea copy constructor
// doesn't actually copy anything but required for CArray<carea,carea>.InsertAt()
carea::carea( const carea& s )
{
	npins = 0;
	nvias = 0;
	poly = new CPolyLine( m_dlist );
}

// carea assignment operator
// doesn't actually assign but required for CArray<carea,carea>.InsertAt to work
carea &carea::operator=( carea &a )
{
	return *this;
}


cnet::~cnet()
{
	CIterator_cnet::OnRemove(this);
}


CNetList::CNetList( CDisplayList * dlist, CPartList * plist )
{
	m_dlist = dlist;			// attach display list
	m_plist = plist;			// attach part list
	m_bSMT_connect = FALSE;
}

CNetList::~CNetList()
{
	RemoveAllNets();
}


// Add new net to netlist
//
cnet * CNetList::AddNet( CString name, int max_pins, CNetWidthInfo const &def_width_attrib )
{
	// create new net
	cnet * new_net = new cnet( m_dlist );

	// set array sizes
	new_net->pin.SetSize( 0 );
	new_net->connect.SetSize( 0 );
	new_net->nconnects = 0;
	new_net->npins = 0;

	// zero areas
	new_net->nareas = 0;

	// set default trace width
	new_net->def_width_attrib = def_width_attrib;
	new_net->def_width_attrib.SetParent(m_def_width_attrib);
	new_net->def_width_attrib.Update();

	// create id and set name
	id id( ID_NET, 0 );
	new_net->id = id;
	new_net->name = name;

	// visible by default
	new_net->visible = 1;

	// add name and pointer to map
	m_map.SetAt( name, (void*)new_net );

	return new_net;
}


// Remove net from list
//
void CNetList::RemoveNet( cnet * net )
{
	if( m_plist )
	{
		// remove pointers to net from pins on part
		for( int ip=0; ip<net->npins; ip++ )
		{
			cpart * pin_part = net->pin[ip].part;
			if( pin_part )
			{
				CShape * s = pin_part->shape;
				if( s )
				{
					int pin_index = s->GetPinIndexByName( net->pin[ip].pin_name );
					if( pin_index >= 0 )
                    {
                        // Remove association & clearance parent
						pin_part->pin[pin_index].set_net();
                    }
				}
			}
		}
	}
	// destroy arrays
	net->connect.RemoveAll();
	net->pin.RemoveAll();
	net->area.RemoveAll();
	m_map.RemoveKey( net->name );
	delete( net );
}

// remove all nets from netlist
//
void CNetList::RemoveAllNets()
{
	// remove all nets
   POSITION pos;
   CString name;
   void * ptr;
   for( pos = m_map.GetStartPosition(); pos != NULL; )
   {
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		RemoveNet( net );
   }
   m_map.RemoveAll();
}


// set utility parameter of all nets
//
void CNetList::MarkAllNets( int utility )
{
	CIterator_cnet net_iter(this);
	for( cnet * net = net_iter.GetFirst(); net != NULL; net = net_iter.GetNext() )
	{
		net->utility = utility;
		for( int ip=0; ip<net->npins; ip++ )
			net->pin[ip].utility = utility;
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			cconnect * c = &net->connect[ic];
			c->utility = utility;
			for( int is=0; is<c->nsegs+1; is++ )
			{
				if( is < c->nsegs )
					c->seg[is].utility = utility;
				c->vtx[is].utility = utility;
			}
		}
		for( int ia=0; ia<net->nareas; ia++ )
		{
			carea * a = &net->area[ia];
			a->utility = utility;
			for( int is=0; is<a->poly->GetNumSides(); is++ )
			{
				a->poly->SetUtility( is, utility );
			}
		}
	}
}

// move origin of coordinate system
//
void CNetList::MoveOrigin( int x_off, int y_off )
{
	// remove all nets
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			cconnect * c =  &net->connect[ic];
			UndrawConnection( net, ic );
			for( int iv=0; iv<=c->nsegs; iv++ )
			{
				cvertex * v = &c->vtx[iv];
				v->x += x_off;
				v->y += y_off;
			}
			DrawConnection( net, ic );
		}
		for( int ia=0; ia<net->area.GetSize(); ia++ )
		{
			carea * a = &net->area[ia];
			a->poly->MoveOrigin( x_off, y_off );
			SetAreaConnections( net, ia );
		}
	}
}

// Undraw all of the grelements of a connection
// If m_dlist == 0, do nothing
//
void CNetList::UndrawConnection( cnet * net, int ic )
{
	if( m_dlist )
	{
		cconnect * c = &net->connect[ic];
		int nsegs = c->nsegs;
		int nvtx = nsegs + 1;
		for( int is=0; is<nsegs; is++ )
		{
			cseg * s = &c->seg[is];

			s->Undraw();
		}
		for( int iv=0; iv<nvtx; iv++ )
		{
			UndrawVia( net, ic, iv );
		}
	}
}

// Draw all of the grelements of a connection
// If m_dlist == 0, do nothing
//
void CNetList::DrawConnection( cnet * net, int ic )
{
	if( m_dlist )
	{
		UndrawConnection( net, ic );
		cconnect * c = &net->connect[ic];
		int nsegs = c->nsegs;
		for( int is=0; is<nsegs; is++ )
		{
			cseg * s = &c->seg[is];
			cvertex *pre_v = &c->vtx[is];
			cvertex *post_v = &c->vtx[is+1];
			id s_id = net->id;
			s_id.st = ID_CONNECT;
			s_id.i = ic;
			s_id.sst = ID_SEG;
			s_id.ii = is;

			int v = 1;
			if( s->layer == LAY_RAT_LINE )
				v = net->visible;

			s->dl_el = m_dlist->Add( s_id, net, s->layer, DL_LINE, v,
				s->width(), 0, s->clearance(), pre_v->x, pre_v->y, post_v->x, post_v->y,
				0, 0
			);

			s_id.sst = ID_SEL_SEG;
			s->dl_sel = m_dlist->AddSelector( s_id, net, s->layer, DL_LINE, v,
				s->width(), 0, pre_v->x, pre_v->y, post_v->x, post_v->y,
				0, 0
			);
		}

		int nvtx;
		if( c->end_pin == cconnect::NO_END )
			nvtx = nsegs + 1;
		else
			nvtx = nsegs;

		for( int iv=1; iv<nvtx; iv++ )
			ReconcileVia( net, ic, iv );

		// if tee stub, reconcile via of tee vertex
		if( c->end_pin == cconnect::NO_END )
		{
			int id = c->vtx[c->nsegs].tee_ID;
			if( id )
			{
				int tee_ic;
				int tee_iv;
				BOOL bFound = FindTeeVertexInNet( net, id, &tee_ic, &tee_iv );
				if( bFound )
					ReconcileVia( net, tee_ic, tee_iv );
			}
		}
	}
}


// Add new pin to net
//
part_pin * CNetList::AddNetPin( cnet * net, CString const &ref_des, CString const &pin_name, BOOL set_areas )
{
	// set size of pin array
	net->pin.SetSize( net->npins + 1 );

	// add pin to array
	cpin *pin = &net->pin[net->npins];

	pin->set_ref_des( ref_des );
	pin->pin_name = pin_name;

	// now lookup part and hook to net if successful
	cpart *part = m_plist->GetPart( ref_des );
	pin->part = part;

	part_pin *part_pin = NULL;

	if( part )
	{
		// hook part to net
		if( part->shape )
		{
			int pin_index = part->shape->GetPinIndexByName( pin_name );
			if( pin_index >= 0 )
			{
				part_pin = &part->pin[pin_index];

				// hook net to part
				part_pin->set_net( net );
			}
		}
	}

	net->npins++;

	// adjust connections to areas
	if( net->nareas && set_areas )
	{
		SetAreaConnections( net );
	}

	return part_pin;
}

// Remove pin from net (by reference designator and pin number)
// Use this if the part may not actually exist in the partlist,
// or the pin may not exist in the part
//
void CNetList::RemoveNetPin( cnet * net, CString const &ref_des, CString const &pin_name )
{
	// find pin in pin list for net
	int net_pin = GetNetPinIndex( net, ref_des, pin_name );
	if( net_pin == -1 )
	{
		// pin not found
		ASSERT(0);
	}
	RemoveNetPin( net, net_pin );
}

// Remove pin from net (by pin index)
// Use this if the part may not actually exist in the partlist,
// or the pin may not exist in the part
//
void CNetList::RemoveNetPin( cnet * net, int net_pin_index )
{
	// now remove all connections to/from this pin
	int ic = 0;
	while( ic<net->nconnects )
	{
		cconnect * c = &net->connect[ic];
		if( c->start_pin == net_pin_index || c->end_pin == net_pin_index )
			RemoveNetConnect( net, ic, FALSE );
		else
			ic++;
	}
	// now remove link to net from part pin (if it exists)
	cpin *pin = &net->pin[net_pin_index];
	cpart * part = pin->part;
	if( part )
	{
		if( part->shape )
		{
			int part_pin_index = part->shape->GetPinIndexByName( pin->pin_name );
			if( part_pin_index >= 0 )
				part->pin[part_pin_index].set_net();
		}
	}
	// now remove pin from net
	net->pin.RemoveAt(net_pin_index);
	net->npins--;
	// now adjust pin numbers in remaining connections
	for( ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		if( c->start_pin > net_pin_index )
			c->start_pin--;
		if( c->end_pin > net_pin_index )
			c->end_pin--;
	}
	// adjust connections to areas
	if( net->nareas )
		SetAreaConnections( net );
}

// Remove pin from net (by part and pin_name), including all connections to pin
//
void CNetList::RemoveNetPin( cpart * part, CString * pin_name )
{
	if( !part )
		ASSERT(0);
	if( !part->shape )
		ASSERT(0);
	int pin_index = part->shape->GetPinIndexByName( *pin_name );
	if( pin_index == -1 )
		ASSERT(0);
	cnet * net = (cnet*)part->pin[pin_index].net;
	if( net == 0 )
	{
		// no net attached to pin
		ASSERT(0);
	}
	// find pin in pin list for net
	int net_pin = -1;
	for( int ip=0; ip<net->npins; ip++ )
	{
		if( net->pin[ip].part == part )
		{
			if( net->pin[ip].pin_name == *pin_name )
			{
				net_pin = ip;
				break;
			}
		}
	}
	if( net_pin == -1 )
	{
		// pin not found
		ASSERT(0);
	}
	RemoveNetPin( net, net_pin );
}

// Remove connections to part->pin from part->pin->net
// set part->pin->net pointer and net->pin->part pointer to NULL
//
void CNetList::DisconnectNetPin( cpart * part, CString const &pin_name )
{
	if( !part )
		ASSERT(0);
	if( !part->shape )
		ASSERT(0);
	int pin_index = part->shape->GetPinIndexByName( pin_name );
	if( pin_index == -1 )
		ASSERT(0);
	cnet * net = (cnet*)part->pin[pin_index].net;
	if( net == 0 )
	{
		return;
	}
	// find pin in pin list for net
	int net_pin = -1;
	for( int ip=0; ip<net->npins; ip++ )
	{
		if( net->pin[ip].part == part )
		{
			if( net->pin[ip].pin_name == pin_name )
			{
				net_pin = ip;
				break;
			}
		}
	}
	if( net_pin == -1 )
	{
		// pin not found
		ASSERT(0);
	}
	// now remove all connections to/from this pin
	int ic = 0;
	while( ic<net->nconnects )
	{
		cconnect * c = &net->connect[ic];
		if( c->start_pin == net_pin || c->end_pin == net_pin )
			RemoveNetConnect( net, ic, FALSE );
		else
			ic++;
	}
	// now remove link to net from part
	part->pin[pin_index].set_net();
	// now remove link to part from net
	net->pin[net_pin].part = NULL;
	// adjust connections to areas
	if( net->nareas )
		SetAreaConnections( net );
}


// Disconnect pin from net (by reference designator and pin number)
// Use this if the part may not actually exist in the partlist,
// or the pin may not exist in the part
//
void CNetList::DisconnectNetPin( cnet * net, CString const &ref_des, CString const &pin_name )
{
	// find pin in pin list for net
	int net_pin = GetNetPinIndex( net, ref_des, pin_name );
	if( net_pin == -1 )
	{
		// pin not found
		ASSERT(0);
	}
	// now remove all connections to/from this pin
	int ic = 0;
	while( ic<net->nconnects )
	{
		cconnect * c = &net->connect[ic];
		if( c->start_pin == net_pin || c->end_pin == net_pin )
			RemoveNetConnect( net, ic, FALSE );
		else
			ic++;
	}
	// now remove link to net from part pin (if it exists)
	cpart * part = net->pin[net_pin].part;
	if( part )
	{
		if( part->shape )
		{
			int pin_index = part->shape->GetPinIndexByName( pin_name );
			if( pin_index != -1 )
				part->pin[pin_index].set_net();
		}
	}
	net->pin[net_pin].part = NULL;
	// adjust connections to areas
	if( net->nareas )
		SetAreaConnections( net );
}

// return pin index or -1 if not found
//
int CNetList::GetNetPinIndex( cnet * net, CString const &ref_des, CString const &pin_name )
{
	// find pin in pin list for net
	int net_pin = -1;
	for( int ip=0; ip<net->npins; ip++ )
	{
		if( net->pin[ip].ref_des() == ref_des && net->pin[ip].pin_name == pin_name )
		{
			net_pin = ip;
			break;
		}
	}
	return net_pin;
}

// Add new connection to net, consisting of one unrouted segment
// p1 and p2 are indexes into pin array for this net
// returns index to connection, or -1 if fails
//
int CNetList::AddNetConnect( cnet * net, int p1, int p2 )
{
	if( net->nconnects != net->connect.GetSize() )
		ASSERT(0);

	net->connect.SetSize( net->nconnects + 1 );
	net->connect[net->nconnects].seg.SetSize( 1 );
	net->connect[net->nconnects].seg[0].Initialize( m_dlist );
	net->connect[net->nconnects].vtx.SetSize( 2 );
	net->connect[net->nconnects].vtx[0].Initialize( m_dlist );
	net->connect[net->nconnects].vtx[1].Initialize( m_dlist );
	net->connect[net->nconnects].nsegs = 1;
	net->connect[net->nconnects].locked = 0;
	net->connect[net->nconnects].start_pin = p1;
	net->connect[net->nconnects].end_pin = p2;

	// check for valid pins
	cpart * part1 = net->pin[p1].part;
	cpart * part2 = net->pin[p2].part;
	if( part1 == 0 || part2 == 0 )
		return -1;
	CShape * shape1 = part1->shape;
	CShape * shape2 = part2->shape;
	if( shape1 == 0 || shape2 == 0 )
		return -1;
	int pin_index1 = shape1->GetPinIndexByName( net->pin[p1].pin_name );
	int pin_index2 = shape2->GetPinIndexByName( net->pin[p2].pin_name );
	if( pin_index1 == -1 || pin_index2 == -1 )
		return -1;

	// add a single unrouted segment
	CPoint pi, pf;
	pi = m_plist->GetPinPoint( net->pin[p1].part, net->pin[p1].pin_name );
	pf = m_plist->GetPinPoint( net->pin[p2].part, net->pin[p2].pin_name );
	int xi = pi.x;
	int yi = pi.y;
	int xf = pf.x;
	int yf = pf.y;

	net->connect[net->nconnects].seg[0].layer = LAY_RAT_LINE;
	net->connect[net->nconnects].seg[0].width_attrib.m_seg_width = CInheritableInfo::E_USE_PARENT;
	net->connect[net->nconnects].seg[0].width_attrib.SetParent( net->def_width_attrib );
	net->connect[net->nconnects].seg[0].selected = 0;

	net->connect[net->nconnects].vtx[0].x = xi;
	net->connect[net->nconnects].vtx[0].y = yi;
	net->connect[net->nconnects].vtx[0].pad_layer = m_plist->GetPinLayer( net->pin[p1].part, net->pin[p1].pin_name );
	net->connect[net->nconnects].vtx[0].force_via_flag = 0;
	net->connect[net->nconnects].vtx[0].tee_ID = 0;
	net->connect[net->nconnects].vtx[0].SetNoVia();

	net->connect[net->nconnects].vtx[1].x = xf;
	net->connect[net->nconnects].vtx[1].y = yf;
	net->connect[net->nconnects].vtx[1].pad_layer = m_plist->GetPinLayer( net->pin[p2].part, net->pin[p2].pin_name );
	net->connect[net->nconnects].vtx[1].force_via_flag = 0;
	net->connect[net->nconnects].vtx[1].tee_ID = 0;
	net->connect[net->nconnects].vtx[1].SetNoVia();

	// create id for this segment
	id id( ID_NET, ID_CONNECT, net->nconnects );

	net->connect[net->nconnects].seg[0].dl_el  = NULL;
	net->connect[net->nconnects].seg[0].dl_sel = NULL;
	net->connect[net->nconnects].vtx[0].dl_sel = NULL;
	net->connect[net->nconnects].vtx[1].dl_sel = NULL;

	if( m_dlist )
	{
		// draw graphic elements for segment
		id.sst = ID_SEG;
		net->connect[net->nconnects].seg[0].dl_el  = m_dlist->Add        ( id, net, LAY_RAT_LINE, DL_LINE, net->visible, 0, 0, 0, xi, yi, xf, yf, 0, 0 );

		id.sst = ID_SEL_SEG;
		net->connect[net->nconnects].seg[0].dl_sel = m_dlist->AddSelector( id, net, LAY_RAT_LINE, DL_LINE, net->visible, 0, 0,    xi, yi, xf, yf, 0, 0 );
	}

	return net->nconnects++;
}

// add connection to net consisting of starting vertex only
// i.e. this will be a stub trace with no end pin
// returns index to connection or -1 if fails
//
int CNetList::AddNetStub( cnet * net, int p1 )
{
	if( net->nconnects != net->connect.GetSize() )
		ASSERT(0);

	if( net->pin[p1].part == 0 )
		return -1;

	net->connect.SetSize( net->nconnects + 1 );
	net->connect[net->nconnects].seg.SetSize( 0 );
	net->connect[net->nconnects].vtx.SetSize( 1 );
	net->connect[net->nconnects].vtx[0].Initialize( m_dlist );
	net->connect[net->nconnects].nsegs = 0;
	net->connect[net->nconnects].locked = 0;
	net->connect[net->nconnects].start_pin = p1;
	net->connect[net->nconnects].end_pin = cconnect::NO_END;

	// add a single vertex
	CPoint pi = m_plist->GetPinPoint( net->pin[p1].part, net->pin[p1].pin_name );
	net->connect[net->nconnects].vtx[0].x = pi.x;
	net->connect[net->nconnects].vtx[0].y = pi.y;
	net->connect[net->nconnects].vtx[0].pad_layer = m_plist->GetPinLayer( net->pin[p1].part, net->pin[p1].pin_name );
	net->connect[net->nconnects].vtx[0].force_via_flag = 0;
	net->connect[net->nconnects].vtx[0].tee_ID = 0;
	net->connect[net->nconnects].vtx[0].SetNoVia();
	net->connect[net->nconnects].vtx[0].dl_sel = 0;

	return net->nconnects++;
}

// test for hit on end-pad of connection
// if dir == 0, check end pad
// if dir == 1, check start pad
//
BOOL CNetList::TestHitOnConnectionEndPad( int x, int y, cnet * net, int ic,
										 int layer, int dir )
{
	int ip;
	cconnect * c =&net->connect[ic];
	if( dir == 1 )
	{
		// get first pad in connection
		ip = c->start_pin;
	}
	else
	{
		// get last pad in connection
		ip = c->end_pin;
	}
	if( ip != cconnect::NO_END )
	{
		cpart * part = net->pin[ip].part;
		CString pin_name = net->pin[ip].pin_name;
		if( !part )
			ASSERT(0);
		if( !part->shape )
			ASSERT(0);
		return( m_plist->TestHitOnPad( part, &pin_name, x, y, layer ) );
	}
	else
		return FALSE;
}

// test for hit on any pad on this net
// returns -1 if not found, otherwise index into net->pin[]
//
int CNetList::TestHitOnAnyPadInNet( int x, int y, int layer, cnet * net )
{
	int ix = -1;
	for( int ip=0; ip<net->pin.GetSize(); ip++ )
	{
		cpart * part = net->pin[ip].part;
		if( part )
		{
			CString pin_name = net->pin[ip].pin_name;
//			if( !part )
//				ASSERT(0);
//			if( !part->shape )
//				ASSERT(0);
			if( m_plist->TestHitOnPad( part, &pin_name, x, y, layer ) )
			{
				ix = ip;
				break;
			}
		}
	}
	return ix;
}

// Clean up connections by removing connections with no segments,
// removing zero-length segments and combining segments
//
void CNetList::CleanUpConnections( cnet * net, CString * logstr )
{
	for( int ic=net->nconnects-1; ic>=0; ic-- )
	{
		UndrawConnection( net, ic );
		cconnect * c = &net->connect[ic];
		for( int is=c->nsegs-1; is>=0; is-- )
		{
			// check for zero-length segment
			if( c->vtx[is].x == c->vtx[is+1].x && c->vtx[is].y == c->vtx[is+1].y )
			{
				// yes, analyze segment
				enum { UNDEF=0, THRU_PIN, SMT_PIN, VIA, TEE, SEGMENT, END_STUB };
				int pre_type = UNDEF;	// type of preceding item
				int pre_layer = UNDEF;	// layer if SEGMENT or SMT_PIN
				int post_type = UNDEF;	// type of following item
				int post_layer = UNDEF;	// layer if SEGMENT or SMT_PIN
				int layer = c->seg[is].layer;
				// analyze start of segment
				if( is == 0 )
				{
					// first segment
					pre_layer = c->vtx[0].pad_layer;
					if( pre_layer == LAY_PAD_THRU )
						pre_type = THRU_PIN;	// starts on a thru pin
					else
						pre_type = SMT_PIN;		// starts on a SMT pin
				}
				else
				{
					// not first segment
					pre_layer = c->seg[is-1].layer;	// preceding layer
					if( c->vtx[is].tee_ID )
						pre_type = TEE;				// starts on a tee-vertex
					else if( c->vtx[is].viaExists() )
						pre_type = VIA;				// starts on a via
					else
						pre_type = SEGMENT;			// starts on a segment
				}
				// analyze end of segment
				if( is == c->nsegs-1 && c->end_pin == cconnect::NO_END )
				{
					// last segment of stub trace
					if( c->vtx[is+1].tee_ID )
						post_type = TEE;			// ends on a tee-vertex
					else if( c->vtx[is+1].viaExists() )
						post_type = VIA;			// ends on a via
					else
						post_type = END_STUB;		// ends a stub (no via or tee)
				}
				else if( is == c->nsegs-1 )
				{
					// last segment of regular trace
					post_layer = c->vtx[is+1].pad_layer;
					if( post_layer == LAY_PAD_THRU )
						post_type = THRU_PIN;		// ends on a thru pin
					else
						post_type = SMT_PIN;		// ends on a SMT pin
				}
				else
				{
					// not last segment
					post_layer = c->seg[is+1].layer;
					if( c->vtx[is+1].tee_ID )
						post_type = TEE;				// ends on a tee-vertex
					else if( c->vtx[is+1].viaExists() )
						post_type = VIA;				// ends on a via
					else
						post_type = SEGMENT;			// ends on a segment
				}
				// OK, now see if we can remove the zero-length segment by removing
				// the starting vertex
				BOOL bRemove = FALSE;
				if( pre_type == SEGMENT && pre_layer == layer
					|| pre_type == SEGMENT && layer == LAY_RAT_LINE
					|| pre_type == VIA && post_type == VIA
					|| pre_type == VIA && post_type == THRU_PIN
					|| post_type == END_STUB )
				{
					// remove starting vertex
					c->vtx.RemoveAt(is);
					bRemove = TRUE;
				}
				else if( post_type == SEGMENT && post_layer == layer
					|| post_type == SEGMENT && layer == LAY_RAT_LINE
					|| post_type == VIA && pre_type == THRU_PIN )
				{
					// remove following vertex
					c->vtx.RemoveAt(is+1);
					bRemove = TRUE;
				}
				if( bRemove )
				{
					c->seg.RemoveAt(is);
					c->nsegs--;
					if( logstr )
					{
						CString str;
						if( c->end_pin == cconnect::NO_END )
						{
							str.Format( "net %s: stub trace from %s.%s: removing zero-length segment\r\n",
								net->name,
								net->pin[c->start_pin].ref_des(), net->pin[c->start_pin].pin_name );
						}
						else
						{
							str.Format( "net %s: trace %s.%s to %s.%s: removing zero-length segment\r\n",
								net->name,
								net->pin[c->start_pin].ref_des(), net->pin[c->start_pin].pin_name,
								net->pin[c->end_pin].ref_des(), net->pin[c->end_pin].pin_name );
						}
						*logstr += str;
					}
				}
			}
		}
		// see if there are any segments left
		if( c->nsegs == 0 )
		{
			// no, remove connection
			net->connect.RemoveAt(ic);
			net->nconnects--;
//			return;
		}
		else
		{
			// look for segments on same layer, with same width,
			// not separated by a tee or via
			for( int is=c->nsegs-2; is>=0; is-- )
			{
				if( c->seg[is].layer == c->seg[is+1].layer
					&& c->seg[is].width() == c->seg[is+1].width()
					&& !c->vtx[is+1].viaExists()
					&& c->vtx[is+1].tee_ID == 0 )
				{
					// see if colinear
					double dx1 = c->vtx[is+1].x - c->vtx[is].x;
					double dy1 = c->vtx[is+1].y - c->vtx[is].y;
					double dx2 = c->vtx[is+2].x - c->vtx[is+1].x;
					double dy2 = c->vtx[is+2].y - c->vtx[is+1].y;
					if( dy1*dx2 == dy2*dx1 && (dx1*dx2>0.0 || dy1*dy2>0.0) )
					{
						// yes, combine these segments
						if( logstr )
						{
							CString str;
							if( c->end_pin == cconnect::NO_END )
							{
								str.Format( "net %s: stub trace from %s.%s: combining colinear segments\r\n",
									net->name,
									net->pin[c->start_pin].ref_des(), net->pin[c->start_pin].pin_name );
							}
							else
							{
								str.Format( "net %s: trace %s.%s to %s.%s: combining colinear segments\r\n",
									net->name,
									net->pin[c->start_pin].ref_des(), net->pin[c->start_pin].pin_name,
									net->pin[c->end_pin].ref_des(), net->pin[c->end_pin].pin_name );
							}
							*logstr += str;
						}
						c->vtx.RemoveAt(is+1);
						c->seg.RemoveAt(is+1);
						c->nsegs--;
					}
				}
			}
			if( m_dlist )
				DrawConnection( net, ic );
		}
	}
	// now look for malformed traces due to autorouter
	// check for stubs ending at same point on same layer
	for( int ic=net->nconnects-2; ic>=0; ic-- )
	{
		cconnect * c = &net->connect[ic];
		cvertex * end_v = &c->vtx[c->nsegs];
		if( c->end_pin == cconnect::NO_END && end_v->tee_ID == 0 )
		{
			// stub
			int x = end_v->x;
			int y = end_v->y;

			int layer = LAY_PAD_THRU;
			if( !end_v->viaExists() )
				layer = c->seg[c->nsegs-1].layer;

			// now loop through other stubs to see if a match
			for( int icc=net->nconnects-1; icc>ic; icc-- )
			{
				cconnect * cc = &net->connect[icc];
				cvertex * end_vv = &cc->vtx[cc->nsegs];
				if( cc->end_pin == cconnect::NO_END && end_vv->tee_ID == 0 )
				{
					int xx = end_vv->x;
					int yy = end_vv->y;

					int llayer = LAY_PAD_THRU;
					if( !end_vv->viaExists() )
						llayer = cc->seg[cc->nsegs-1].layer;

					if( x==xx && y==yy && (layer==llayer || layer==LAY_PAD_THRU || llayer==LAY_PAD_THRU ) )
					{
						if( logstr )
						{
							CString str;
							str.Format( "net %s: stub traces from %s.%s and %s.%s: same end-point\r\n",
									net->name,
									net->pin[c->start_pin].ref_des(), net->pin[c->start_pin].pin_name,
									net->pin[cc->start_pin].ref_des(), net->pin[cc->start_pin].pin_name 
							);
							*logstr += str;
						}
					}
				}
			}
		}
	}
	//
	RenumberConnections( net );
}


void CNetList::CleanUpAllConnections( CString * logstr )
{
	CString str;

	cnet * net;
	CIterator_cnet net_iter(this);
	for( net = net_iter.GetFirst(); net != NULL; net = net_iter.GetNext() )
	{
		CleanUpConnections( net, logstr );
	}

	// check tee_IDs in array
	if( logstr )
		*logstr += "\r\nChecking tees and branches:\r\n";
	if( logstr )
	{
		str.Format( "  %d tee_IDs in array:\r\n", m_tee.GetSize() );
		*logstr += str;
	}
	for( int it=0; it<m_tee.GetSize(); it++ )
	{
		int tee_id = m_tee[it];
		cnet * net = NULL;
		int ic;
		int iv;
		BOOL bFound = FindTeeVertex( tee_id, &net, &ic, &iv );
		if( !bFound )
		{
			if( logstr )
			{
				str.Format( "    tee_id %d not found in project, removed\r\n", tee_id );
				*logstr += str;
			}
			RemoveTeeID( tee_id );
		}
	}
	// now check tee_IDs in project
	for( net = net_iter.GetFirst(); net != NULL; net = net_iter.GetNext() )
	{
		for( int ic=net->nconnects-1; ic>=0; ic-- )
		{
			cconnect * c = &net->connect[ic];
			if( c->end_pin == cconnect::NO_END )
			{
				// branch, check for tee
				int end_id = c->vtx[c->nsegs].tee_ID;
				if( end_id )
				{
					BOOL bError = FALSE;
					CString no_tee_str = "";
					CString no_ID_str = "";
					if( !FindTeeVertexInNet( net, end_id, 0, 0 ) )
					{
						no_tee_str = ", not in trace";
						bError = TRUE;
					}
					if( FindTeeID( end_id ) == -1 )
					{
						no_ID_str = ", not in ID array";
						bError = TRUE;
					}
					if( bError && logstr )
					{
						str.Format( "  tee_id %d found in branch%s%s, branch removed\r\n",
							end_id, no_tee_str, no_ID_str );
						*logstr += str;
					}
					if( bError )
						RemoveNetConnect( net, ic, FALSE );
				}
			}
			else
			{
				for( int iv=1; iv<c->nsegs; iv++ )
				{
					int id = c->vtx[iv].tee_ID;
					if( id )
					{
						// tee-vertex, check array
						if( FindTeeID(id) == -1 && logstr )
						{
							str.Format( "  tee_id %d found in trace, not in ID array\r\n", id );
							*logstr += str;
						}
					}
				}
			}
		}
	}
}


// Remove connection from net
// Does not remove any orphaned branches that result
// Leave pins in pin list for net
//
int CNetList::RemoveNetConnect( cnet * net, int ic, BOOL set_areas )
{
	cconnect * c = &net->connect[ic];
	if( c->end_pin == cconnect::NO_END )
	{
		// stub
		if( c->vtx[c->nsegs].tee_ID )
		{
			// stub trace ending on tee, remove tee
			DisconnectBranch( net, ic );
		}
	}

	// see if contains tee-vertices
	for( int iv=1; iv<c->nsegs; iv++ )
	{
		int id = c->vtx[iv].tee_ID;
		if( id )
			RemoveTee( net, id );	// yes, remove it
	}

	// remove connection
	net->connect.RemoveAt( ic );
	net->nconnects = net->connect.GetSize();
	RenumberConnections( net );
	// adjust connections to areas
	if( net->nareas && set_areas )
		SetAreaConnections( net );
	return 0;
}

// Unroute all segments of a connection and merge if possible
// Preserves tees
//
int CNetList::UnrouteNetConnect( cnet * net, int ic )
{
	cconnect * c = &net->connect[ic];
	for( int is=0; is<c->nsegs; is++ )
		UnrouteSegmentWithoutMerge( net, ic, is );
	MergeUnroutedSegments( net, ic );
	return 0;
}

// Change the start or end pin of a connection and redraw it
//
void CNetList::ChangeConnectionPin( cnet * net, int ic, int end_flag,
								   cpart * part, CString * pin_name )
{
	// find pin in pin list for net
	int pin_index = -1;
	for( int ip=0; ip<net->npins; ip++ )
	{
		if( net->pin[ip].part == part && net->pin[ip].pin_name == *pin_name )
		{
			pin_index = ip;
			break;
		}
	}
	if( pin_index == -1 )
	{
		// pin not found
		ASSERT(0);
	}
	cconnect * c = &net->connect[ic];
	cpin * pin = &net->pin[pin_index];
	CPoint p = m_plist->GetPinPoint( part, *pin_name );
	int layer = m_plist->GetPinLayer( part, *pin_name );
	if( end_flag )
	{
		// change end pin
		int is = c->nsegs-1;
		c->end_pin = pin_index;
		c->vtx[is+1].x = p.x;
		c->vtx[is+1].y = p.y;
		c->vtx[is+1].pad_layer = layer;
		m_dlist->Set_xf( c->seg[is].dl_el, p.x );
		m_dlist->Set_yf( c->seg[is].dl_el, p.y );
		m_dlist->Set_xf( c->seg[is].dl_sel, p.x );
		m_dlist->Set_yf( c->seg[is].dl_sel, p.y );
	}
	else
	{
		// change start pin
		c->start_pin = pin_index;
		c->vtx[0].x = p.x;
		c->vtx[0].y = p.y;
		c->vtx[0].pad_layer = layer;
		m_dlist->Set_x( c->seg[0].dl_el, p.x );
		m_dlist->Set_y( c->seg[0].dl_el, p.y );
		m_dlist->Set_x( c->seg[0].dl_sel, p.x );
		m_dlist->Set_y( c->seg[0].dl_sel, p.y );
	}
}



// Unroute segment
// return id of new segment (since seg[] array may change as a result)
//
id CNetList::UnrouteSegment( cnet * net, int ic, int is )
{
	cconnect * c = &net->connect[ic];
	id seg_id = id(ID_NET, ID_CONNECT, ic, ID_SEG, is );
	UnrouteSegmentWithoutMerge( net, ic, is );
	id mid = MergeUnroutedSegments( net, ic );
	if( mid.type == 0 )
		mid = seg_id;
	return mid;
}

// Merge any adjacent unrouted segment of this connection
// unless separated by a tee-connection
// Returns id of first merged segment in connection
// Reconciles vias for any tee-connections by calling DrawConnection()
//
id CNetList::MergeUnroutedSegments( cnet * net, int ic )
{
	id mid( ID_NET, ID_CONNECT, ic, ID_SEL_SEG, -1 );

	cconnect * c = &net->connect[ic];
	if( c->nsegs == 1 )
		mid.Clear();

	if( m_dlist )
		UndrawConnection( net, ic );
	for( int is=c->nsegs-2; is>=0; is-- )
	{
		cseg * post_s = &c->seg[is+1];
		cseg * s = &c->seg[is];
		if( post_s->layer == LAY_RAT_LINE && s->layer == LAY_RAT_LINE
			&& c->vtx[is+1].tee_ID == 0 && c->vtx[is+1].force_via_flag == 0 )
		{
			// this segment and next are unrouted,
			// remove next segment and interposed vertex
			c->seg.RemoveAt(is+1);
			c->vtx.RemoveAt(is+1);
			c->nsegs = c->seg.GetSize();
			mid.ii = is;
		}
	}
	if( mid.ii == -1 )
		mid.Clear();
	if( m_dlist )
		DrawConnection( net, ic );
	return mid;
}

// Unroute segment, but don't merge with adjacent unrouted segments
// Assume that there will be an eventual call to MergeUnroutedSegments() to set vias
//
void CNetList::UnrouteSegmentWithoutMerge( cnet * net, int ic, int is )
{
	cconnect * c = &net->connect[ic];
	cseg *seg = &c->seg[is];

	// unroute segment
	seg->Unroute();

	// redraw segment
	if( m_dlist )
	{
		if( seg->dl_el )
		{
			int xi, yi, xf, yf;
			xi = c->vtx[is].x;
			yi = c->vtx[is].y;
			xf = c->vtx[is+1].x;
			yf = c->vtx[is+1].y;
			id seg_id = seg->dl_el->id;
			id sel_id = seg->dl_sel->id;

			seg->Undraw();

			seg->dl_el  = m_dlist->Add        ( seg_id, net, LAY_RAT_LINE, DL_LINE, net->visible, 1, 0, 0, xi, yi, xf, yf, 0, 0 );
			seg->dl_sel = m_dlist->AddSelector( sel_id, net, LAY_RAT_LINE, DL_LINE, net->visible, 1, 0,    xi, yi, xf, yf, 0, 0 );
		}
	}

	ReconcileVia( net, ic, is );
	ReconcileVia( net, ic, is+1 );
}

// Remove segment ... currently only used for last segment of stub trace
// or segment of normal pin-pin trace without tees
// If adjacent segments are unrouted, removes them too
// NB: May change connect[] array
// If stub trace and bHandleTee == FALSE, will only alter connections >= ic
//
void CNetList::RemoveSegment( cnet * net, int ic, int is, BOOL bHandleTee )
{
	int id = 0;
	cconnect * c = &net->connect[ic];
	if( c->end_pin == cconnect::NO_END )
	{
		// stub trace, must be last segment
		if( is != (c->nsegs-1) )
		{
			ASSERT(0);
			return;
		}
		// if this is a branch, disconnect it
		if( c->vtx[c->nsegs].tee_ID )
		{
			DisconnectBranch( net, ic );
		}
		if( c->vtx[c->nsegs-1].tee_ID )
		{
			// special case...the vertex preceding this segment is a tee-vertex
			id = c->vtx[c->nsegs-1].tee_ID;
		}
		c->seg.RemoveAt(is);
		c->vtx.RemoveAt(is+1);
		c->nsegs--;
		if( c->nsegs == 0 )
		{
			net->connect.RemoveAt(ic);
			net->nconnects--;
			RenumberConnections( net );
		}
		else
		{
			if( c->seg[is-1].layer == LAY_RAT_LINE
				&& !c->vtx[is-1].viaExists()
				&& c->vtx[is-1].tee_ID == 0 )
			{
				c->seg.RemoveAt(is-1);
				c->vtx.RemoveAt(is);
				c->nsegs--;
				if( c->nsegs == 0 )
				{
					net->connect.RemoveAt(ic);
					net->nconnects--;
					RenumberConnections( net );
				}
			}
		}
		// if a tee became a branch, resolve it
		if( id && bHandleTee )
			RemoveOrphanBranches( net, id );
	}
	else
	{
		// pin-pin trace
		// check for tee-vertex
		for( int iv=1; iv<=c->nsegs; iv++ )
		{
			cvertex * v = &c->vtx[iv];
			if( v->tee_ID )
				ASSERT(0);
		}
		// now convert connection to stub traces
		// first, make new stub trace from the end
		if( is < c->nsegs-1 )
		{
			int new_ic = AddNetStub( net, c->end_pin );
			for( int iss=c->nsegs-1; iss>is; iss-- )
			{
//				AppendSegment( net, new_ic,
			}
		}
	}
	// adjust connections to areas
	if( net->nareas )
		SetAreaConnections( net );
}

// renumber all ids and dl_elements for net connections
// should be used after deleting a connection
//
void CNetList::RenumberConnections( cnet * net )
{
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		RenumberConnection( net, ic );
	}
}

// renumber all ids and dl_elements for net connection
//
void CNetList::RenumberConnection( cnet * net, int ic )
{
	cconnect * c = &net->connect[ic];
	for( int is=0; is<c->nsegs; is++ )
	{
		if( c->seg[is].dl_el )
		{
			c->seg[is].dl_el->id.i = ic;
			c->seg[is].dl_el->id.ii = is;
		}
		if( c->seg[is].dl_sel )
		{
			c->seg[is].dl_sel->id.i = ic;
			c->seg[is].dl_sel->id.ii = is;
		}
	}
	for( int iv=0; iv<=c->nsegs; iv++ )
	{
		for( int il=0; il<c->vtx[iv].dl_el.GetSize(); il++ )
		{
			if( c->vtx[iv].dl_el[il] )
			{
				c->vtx[iv].dl_el[il]->id.i = ic;
				c->vtx[iv].dl_el[il]->id.ii = iv;
			}
		}
		if( c->vtx[iv].dl_sel )
		{
			c->vtx[iv].dl_sel->id.i = ic;
			c->vtx[iv].dl_sel->id.ii = iv;
		}
		if( c->vtx[iv].dl_hole )
		{
			c->vtx[iv].dl_hole->id.i = ic;
			c->vtx[iv].dl_hole->id.ii = iv;
		}
	}
}

// renumber the ids for graphical elements in areas
// should be called after deleting an area
//
void CNetList::RenumberAreas( cnet * net )
{
	id a_id;
	for( int ia=0; ia<net->nareas; ia++ )
	{
		a_id = net->area[ia].poly->GetId();
		a_id.i = ia;
		net->area[ia].poly->SetId( &a_id );
	}
}

// Set segment layer (must be a copper layer, not the ratline layer)
// returns 1 if unable to comply due to SMT pad
//
int CNetList::ChangeSegmentLayer( cnet * net, int ic, int iseg, int layer )
{
	cconnect * c = &net->connect[ic];
	// check layer settings of adjacent vertices to make sure this is legal
	if( iseg == 0 )
	{
		// first segment, check starting pad layer
		int pad_layer = c->vtx[0].pad_layer;
		if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
			return 1;
	}
	if( iseg == (c->nsegs - 1) && c->end_pin != cconnect::NO_END )
	{
		// last segment, check destination pad layer
		int pad_layer = c->vtx[iseg+1].pad_layer;
		if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
			return 1;
	}
	// change segment layer
	cseg * s = &c->seg[iseg];
	cvertex * pre_v = &c->vtx[iseg];
	cvertex * post_v = &c->vtx[iseg+1];
	s->layer = layer;

	// get old graphic elements
	dl_element * old_el  = c->seg[iseg].dl_el;
	dl_element * old_sel = c->seg[iseg].dl_sel;

	// create new graphic elements
	dl_element * new_el = m_dlist->Add( old_el->id, old_el->ptr, layer, old_el->gtype,
		old_el->visible, s->width(), 0, s->clearance(), pre_v->x, pre_v->y,
		post_v->x, post_v->y, 0, 0, 0, layer );

	dl_element * new_sel = m_dlist->AddSelector( old_sel->id, old_sel->ptr, layer,
		old_sel->gtype, old_sel->visible, s->width(), 0, pre_v->x, pre_v->y,
		post_v->x, post_v->y, 0, 0, 0 );

	// remove old graphic elements
	c->seg[iseg].Undraw();

	// add new graphics
	c->seg[iseg].dl_el  = new_el;
	c->seg[iseg].dl_sel = new_sel;

	// now adjust vias
	ReconcileVia( net, ic, iseg );
	ReconcileVia( net, ic, iseg+1 );
	if( iseg == (c->nsegs - 1) && c->end_pin == cconnect::NO_END
		&& post_v->tee_ID )
	{
		// changed last segment of a stub that connects to a tee
		// reconcile tee via
		int icc, ivv;
		BOOL bTest = FindTeeVertexInNet( net, post_v->tee_ID, &icc, &ivv );
		if( bTest )
			ReconcileVia( net, icc, ivv );
		else
			ASSERT(0);
	}
	return 0;
}

// Convert segment from unrouted to routed
// returns 1 if segment can't be routed on given layer due to connection to SMT pad
// Adds/removes vias as necessary
//
int CNetList::RouteSegment( cnet * net, int ic, int iseg, int layer, CSegWidthInfo const &width )
{
	// check layer settings of adjacent vertices to make sure this is legal
	cconnect * c = &net->connect[ic];
	if( iseg == 0 )
	{
		// first segment, check starting pad layer
		int pad_layer = c->vtx[0].pad_layer;
		if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
			return 1;
	}
	if( iseg == (c->nsegs - 1) )
	{
		// last segment, check destination pad layer
		if( c->end_pin == cconnect::NO_END )
		{
		}
		else
		{
			int pad_layer = c->vtx[iseg+1].pad_layer;
			if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
				return 1;
		}
	}

	// remove old graphic elements
	c->seg[iseg].Undraw();

	// modify segment parameters
	c->seg[iseg].layer = layer;

	// Set width attrib
	c->seg[iseg].width_attrib = width;
	c->seg[iseg].width_attrib.SetParent( net->def_width_attrib );
	c->seg[iseg].width_attrib.Update();

	c->seg[iseg].selected  = 0;

	// draw elements
	if( m_dlist )
	{
		int xi = c->vtx[iseg].x;
		int yi = c->vtx[iseg].y;
		int xf = c->vtx[iseg+1].x;
		int yf = c->vtx[iseg+1].y;
		id id( ID_NET, ID_CONNECT, ic, ID_SEG, iseg );

		c->seg[iseg].dl_el  = m_dlist->Add        ( id, net, layer, DL_LINE, 1, c->seg[iseg].width(), 0, c->seg[iseg].clearance(), xi, yi, xf, yf, 0, 0 );
		id.sst = ID_SEL_SEG;
		c->seg[iseg].dl_sel = m_dlist->AddSelector( id, net, layer, DL_LINE, 1, c->seg[iseg].width(), 0, xi, yi, xf, yf, 0, 0 );
	}

	// now adjust vias
	ReconcileVia( net, ic, iseg );
	ReconcileVia( net, ic, iseg+1 );

	return 0;
}


// Append new segment to connection
// this is mainly used for stub traces
// returns index to new segment
//
int CNetList::AppendSegment( cnet * net, int ic, int x, int y, int layer, CSegWidthInfo const &width )
{
	// add new vertex and segment
	cconnect * c =&net->connect[ic];
	c->seg.SetSize( c->nsegs + 1 );
	c->seg[c->nsegs].Initialize( m_dlist );
	c->vtx.SetSize( c->nsegs + 2 );
	c->vtx[c->nsegs].Initialize( m_dlist );
	c->vtx[c->nsegs+1].Initialize( m_dlist );
	int iseg = c->nsegs;

	// set position for new vertex, zero dl_element pointers
	c->vtx[iseg+1].x = x;
	c->vtx[iseg+1].y = y;
	if( m_dlist )
		UndrawVia( net, ic, iseg+1 );

	// create new segment
	c->seg[iseg].layer = layer;

	// Set width attrib
	c->seg[iseg].width_attrib = width;
	c->seg[iseg].width_attrib.SetParent( net->def_width_attrib );
	c->seg[iseg].width_attrib.Update();

	c->seg[iseg].selected  = 0;

	int xi = c->vtx[iseg].x;
	int yi = c->vtx[iseg].y;
	if( m_dlist )
	{
		id id( ID_NET, ID_CONNECT, ic, ID_SEG, iseg );
		c->seg[iseg].dl_el =  m_dlist->Add        ( id, net, layer, DL_LINE, 1, c->seg[iseg].width(), 0, c->seg[iseg].clearance(), xi, yi, x, y, 0, 0 );
		id.sst = ID_SEL_SEG;
		c->seg[iseg].dl_sel = m_dlist->AddSelector( id, net, layer, DL_LINE, 1, c->seg[iseg].width(), 0, xi, yi, x, y, 0, 0 );
		id.sst = ID_SEL_VERTEX;
		id.ii = iseg+1;
		c->vtx[iseg+1].dl_sel = m_dlist->AddSelector( id, net, layer, DL_HOLLOW_RECT,
			1, 0, 0, x-10*PCBU_PER_MIL, y-10*PCBU_PER_MIL,
			x+10*PCBU_PER_MIL, y+10*PCBU_PER_MIL, 0, 0 );
	}

	// done
	c->nsegs++;

	// take care of preceding via
	ReconcileVia( net, ic, iseg );

	return iseg;
}

// Insert new segment into connection, unless the new segment ends at the
// endpoint of the old segment, then replace old segment
// if dir=0 add forward in array, if dir=1 add backwards
// return 1 if segment inserted, 0 if replaced
// tests position within +/- 10 nm.
//
int CNetList::InsertSegment( cnet * net, int ic, int iseg, int x, int y, int layer,
								CSegWidthInfo const &width,
								int dir )
{
	const int TOL = 10;

	// see whether we need to insert new segment or just modify old segment
	cconnect * c = &net->connect[ic];
	int insert_flag = 1;
	if( dir == 0 )
	{
		// routing forward
		if( (abs(x-c->vtx[iseg+1].x) + abs(y-c->vtx[iseg+1].y )) < TOL )
		{
			// new vertex is the same as end of old segment
			if( iseg < (c->nsegs-1) )
			{
				// not the last segment
				if( layer == c->seg[iseg+1].layer )
				{
					// next segment routed on same layer, don't insert new seg
					insert_flag = 0;
				}
			}
			else if( iseg == (c->nsegs-1) )
			{
				// last segment, should connect to pad
				int pad_layer = c->vtx[iseg+1].pad_layer;
				if( pad_layer == LAY_PAD_THRU || layer == LAY_RAT_LINE
					|| (pad_layer == LAY_TOP_COPPER && layer == LAY_TOP_COPPER)
					|| (pad_layer == LAY_BOTTOM_COPPER && layer == LAY_BOTTOM_COPPER) )
				{
					// layer OK to connect to pad, don't insert new seg
					insert_flag = 0;
				}
			}
		}
	}
	else
	{
		// routing backward
		if( x == c->vtx[iseg].x
			&& y == c->vtx[iseg].y )
		{
			// new vertex is the same as start of old segment
			if( iseg >0 )
			{
				// not the first segment
				if( layer == c->seg[iseg-1].layer )
				{
					// prev segment routed on same layer, don't insert new seg
					insert_flag = 0;
				}
			}
			else if( iseg == 0 )
			{
				// first segment, should connect to pad
				int pad_layer = c->vtx[iseg].pad_layer;
				if( pad_layer == LAY_PAD_THRU || layer == LAY_RAT_LINE
					|| (pad_layer == LAY_TOP_COPPER && layer == LAY_TOP_COPPER)
					|| (pad_layer == LAY_BOTTOM_COPPER && layer == LAY_BOTTOM_COPPER) )
				{
					// layer OK to connect to pad, don't insert new seg
					insert_flag = 0;
				}
			}
		}
	}

	cseg *seg;
	if( insert_flag )
	{
		// insert new vertex and segment
		c->seg.SetSize( c->nsegs + 1 );
		c->seg[c->nsegs].Initialize( m_dlist );
		c->vtx.SetSize( c->nsegs + 2 );
		c->vtx[c->nsegs].Initialize( m_dlist );
		c->vtx[c->nsegs+1].Initialize( m_dlist );

		// shift higher segments and vertices up to make room
		for( int i=c->nsegs; i>iseg; i-- )
		{
			c->seg[i] = c->seg[i-1];
			if( c->seg[i].dl_el )
				c->seg[i].dl_el->id.ii = i;
			if( c->seg[i].dl_sel )
				c->seg[i].dl_sel->id.ii = i;
			c->vtx[i+1] = c->vtx[i];
			c->vtx[i].tee_ID = 0;
			c->vtx[i].force_via_flag = FALSE;
			if( c->vtx[i+1].dl_sel )
				c->vtx[i+1].dl_sel->id.ii = i+1;
			if( c->vtx[i+1].dl_hole )
				c->vtx[i+1].dl_hole->id.ii = i+1;
			for( int il=0; il<c->vtx[i+1].dl_el.GetSize(); il++ )
			{
				if( c->vtx[i+1].dl_el[il] )
					c->vtx[i+1].dl_el[il]->id.ii = i+1;
			}
			if( c->vtx[i+1].dl_hole )
				c->vtx[i+1].dl_hole->id.ii = i+1;
		}
		// note that seg[iseg+1] now duplicates seg[iseg], vtx[iseg+2] duplicates vtx[iseg+1]
		// we must replace or zero the dl_element pointers for seg[iseg+1]
		//
		// set position for new vertex, zero dl_element pointers
		c->vtx[iseg+1].x = x;
		c->vtx[iseg+1].y = y;
		if( m_dlist )
			UndrawVia( net, ic, iseg+1 );

		// fill in data for new seg[iseg] or seg[is+1] (depending on dir)
		seg = &c->seg[ (dir == 0) ? iseg : iseg+1 ];

		seg->layer     = layer;
		seg->selected  = 0;

		// Set width attrib
		seg->width_attrib = width;
		seg->width_attrib.SetParent( net->def_width_attrib );
		seg->width_attrib.Update();

		if( dir == 0 )
		{
			// route forward
			int xi = c->vtx[iseg].x;
			int yi = c->vtx[iseg].y;

			if( m_dlist )
			{
				id id( ID_NET, ID_CONNECT, ic, ID_SEG, iseg );
				seg->dl_el  = m_dlist->Add        ( id, net, layer, DL_LINE, 1, seg->width(), 0, seg->clearance(), xi, yi, x, y, 0, 0 );

				id.sst = ID_SEL_SEG;
				seg->dl_sel = m_dlist->AddSelector( id, net, layer, DL_LINE, 1, seg->width(), 0, xi, yi, x, y, 0, 0 );

				id.sst = ID_SEL_VERTEX;
				id.ii = iseg+1;
				c->vtx[iseg+1].dl_sel = m_dlist->AddSelector( id, net, layer, DL_HOLLOW_RECT,
					1, 1, 0,
					x-10*PCBU_PER_MIL, y-10*PCBU_PER_MIL,
					x+10*PCBU_PER_MIL, y+10*PCBU_PER_MIL,
					0, 0
				);
			}
		}
		else
		{
			// route backward
			int xf = c->vtx[iseg+2].x;
			int yf = c->vtx[iseg+2].y;

			if( m_dlist )
			{
				id id( ID_NET, ID_CONNECT, ic, ID_SEG, iseg+1 );
				seg->dl_el  = m_dlist->Add        ( id, net, layer, DL_LINE, 1, seg->width(), 0, seg->clearance(), x, y, xf, yf, 0, 0 );

				id.sst = ID_SEL_SEG;
				seg->dl_sel = m_dlist->AddSelector( id, net, layer, DL_LINE, 1, seg->width(), 0, x, y, xf, yf, 0, 0 );

				id.sst = ID_SEL_VERTEX;
				id.ii = iseg+1;
				c->vtx[iseg+1].dl_sel = m_dlist->AddSelector( id, net, layer, DL_HOLLOW_RECT,
					1, 0, 0,
					x-10*PCBU_PER_MIL, y-10*PCBU_PER_MIL,
					x+10*PCBU_PER_MIL, y+10*PCBU_PER_MIL,
					0, 0
				);
			}
		}

		// modify adjacent old segment for new endpoint
		if( m_dlist )
		{
			seg = &c->seg[ (dir == 0) ? iseg+1 : iseg ];

			if( dir == 0 )
			{
				// adjust next segment for new starting position, and make visible
				m_dlist->Set_x(seg->dl_el, x);
				m_dlist->Set_y(seg->dl_el, y);
				if( seg->dl_el )
					seg->dl_el->id.ii = iseg+1;
				m_dlist->Set_visible(seg->dl_el, 1);

				m_dlist->Set_x(seg->dl_sel, x);
				m_dlist->Set_y(seg->dl_sel, y);
				if( seg->dl_sel )
					m_dlist->Set_visible(seg->dl_sel, 1);
			}
			if( dir == 1 )
			{
				// adjust previous segment for new ending position, and make visible
				m_dlist->Set_xf(seg->dl_el, x);
				m_dlist->Set_yf(seg->dl_el, y);
				if( seg->dl_el )
					seg->dl_el->id.ii = iseg;
				m_dlist->Set_visible(seg->dl_el, 1);

				m_dlist->Set_xf(seg->dl_sel, x);
				m_dlist->Set_yf(seg->dl_sel, y);
				if( seg->dl_sel )
					seg->dl_sel->id.ii = iseg;
				m_dlist->Set_visible(seg->dl_sel, 1);
			}
		}

		// done
		c->nsegs++;
	}
	else
	{
		// don't insert, just modify old segment
		seg = &c->seg[ iseg ];

		seg->selected  = 0;
		seg->layer     = layer;

		// Set width attrib
		seg->width_attrib = width;
		seg->width_attrib.SetParent( net->def_width_attrib );
		seg->width_attrib.Update();

		if( m_dlist )
		{
			int x = m_dlist->Get_x(seg->dl_el);
			int y = m_dlist->Get_y(seg->dl_el);
			int xf = m_dlist->Get_xf(seg->dl_el);
			int yf = m_dlist->Get_yf(seg->dl_el);
			id id  = seg->dl_el->id;

			seg->dl_el->Remove();
			seg->dl_el = m_dlist->Add( id, net, layer, DL_LINE, 1, seg->width(), 0, seg->clearance(), x, y, xf, yf, 0, 0 );
		}
	}

	// clean up vias
	ReconcileVia( net, ic, iseg );
	ReconcileVia( net, ic, iseg+1 );
	if( (iseg+1) < c->nsegs )
		ReconcileVia( net, ic, iseg+2 );

	return insert_flag;
}

// Set trace size attributes for routed segment (ignores unrouted segs)
// Does not set via clearances
int CNetList::SetSegmentWidth( cnet * net, int ic, int is, CInheritableInfo const &attrib )
{
	{
		cconnect * c = &net->connect[ic];
		cseg *pSeg = &c->seg[is];

		// Ensure parent is correct
		pSeg->width_attrib.SetParent( net->def_width_attrib );

		if( pSeg->layer != LAY_RAT_LINE )
		{
			// Set new attributes.
			// Existing attributes are first assigned to attrib, then updated.
			// The resulting behavior is such that if an item relies on its
			// parent, that item is always updated at this point, regardless
			// of whether the item was defined in 'attrib'.  This is consistent
			// with how items are stored in the .fpc file.
			CSegWidthInfo seg_attrib( pSeg->width_attrib );
			seg_attrib = attrib;
			seg_attrib.Update();

			// Update from 'width_attrib', no need to update()
			pSeg->width_attrib = seg_attrib;

			// Update display
			m_dlist->Set_w( pSeg->dl_el,  pSeg->width() );
			m_dlist->Set_w( pSeg->dl_sel, pSeg->width() );

			m_dlist->Set_clearance( pSeg->dl_el, pSeg->clearance() );
	  	}
	}

	{
		CViaWidthInfo ViaInfo( attrib );

		SetViaSizeAttrib( net, ic, is,   ViaInfo );
		SetViaSizeAttrib( net, ic, is+1, ViaInfo );
	}

	return 0;
}



void CNetList::InsertVia( cnet * net, int ic, int ivtx, CViaWidthInfo const &width )
{
	cconnect *c = &net->connect[ic];
	cvertex *pVtx = &c->vtx[ivtx];

	pVtx->via_width_attrib.SetParent( net->def_width_attrib );

	// Get orig attrib
	CViaWidthInfo via_width(pVtx->via_width_attrib);

	// Update based on 'width'
	via_width = width;
	via_width.Update();

	if( via_width.m_via_width.m_val != 0 )
	{
		pVtx->via_width_attrib = via_width;

		DrawVia( net, ic, ivtx );
	}
}


void CNetList::SetViaSizeAttrib( cnet * net, int ic, int ivtx, CInheritableInfo const &width )
{
	cconnect *c = &net->connect[ic];
	cvertex *pVtx = &c->vtx[ivtx];

	if( ( ivtx == 0 ) || ( ( ivtx == c->nsegs ) && ( c->end_pin != cconnect::NO_END ) ) )
	{
		// PIN
		int ip = ( ivtx == 0 ) ? c->start_pin : c->end_pin;

		cpin *pin = &net->pin[ip];
		cpart *part = pin->part;
		CShape *shape = part->shape;

		// Shape may not assigned (==NULL) if the part
		// hasn't been assigned to a shape yet.
		if( shape != NULL )
		{
			int pin_index = shape->GetPinIndexByName( pin->pin_name );
			if( pin_index >= 0 )
			{
				part->pin[pin_index].set_clearance( CClearanceInfo( width ) );
			}
		}
	}
	else if( ViaExists( net, ic, ivtx ) )
	{
		// VIA
		CVertexIterator vi( net, ic, ivtx );
		for( cvertex * pVtx = vi.GetFirst(); pVtx != NULL; pVtx = vi.GetNext() )
		{
			ic   = vi.getcur_ic();
			ivtx = vi.getcur_ivtx();

			// Ensure parent is correct
			pVtx->via_width_attrib.SetParent( net->def_width_attrib );

			// Set new attributes.
			// Existing attributes are first assigned to attrib, then updated.
			// The resulting behavior is such that if an item relies on its
			// parent, that item is always updated at this point, regardless
			// of whether the item was defined in 'attrib'.  This is consistent
			// with how items are stored in the .fpc file.
			CViaWidthInfo via_attrib( pVtx->via_width_attrib );
			via_attrib = width;
			via_attrib.Update();

			// Update from 'width_attrib', no need to update()
			pVtx->via_width_attrib = via_attrib;

			DrawVia( net, ic, ivtx );
		}
	}
}


int CNetList::SetConnectionWidth( cnet * net, int ic, CInheritableInfo const &width )
{
	cconnect * c = &net->connect[ic];
	for( int is=0; is<c->nsegs; is++ )
	{
		SetSegmentWidth( net, ic, is, width );
	}
	return 0;
}


int CNetList::SetNetWidth( cnet * net, CInheritableInfo const &width )
{
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		for( int is=0; is<c->nsegs; is++ )
		{
			SetSegmentWidth( net, ic, is, width );
		}
	}
	return 0;
}



// Applies attributes to unrouted segments
int CNetList::UpdateNetAttributes( cnet * net )
{
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		for( int is=0; is<c->nsegs; is++ )
		{
			cseg *seg = &c->seg[is];

			if( seg->layer == LAY_RAT_LINE )
			{
				seg->width_attrib = net->def_width_attrib;
				seg->width_attrib.SetParent( net->def_width_attrib );
				seg->width_attrib.Update();
			}
		}
	}
	return 0;
}

// part added, hook up to nets
//
void CNetList::PartAdded( cpart * part )
{
	CString ref_des = part->ref_des;

	// iterate through all nets, hooking up to part
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		for( int ip=0; ip<net->npins; ip++ )
		{
			if( net->pin[ip].ref_des() == ref_des )
			{
				// found net->pin which attaches to part
				cpart *old_part = net->pin[ip].part;
				net->pin[ip].part = part;	// set net->pin->part

				if( part->shape )
				{
					int pin_index = part->shape->GetPinIndexByName( net->pin[ip].pin_name );
					if( pin_index != -1 )
					{
						part_pin *pp = &part->pin[pin_index];

						// hook it up
						pp->set_net( net );
						pp->set_clearance( CClearanceInfo() );
					}
				}
			}
		}
	}
}

// Swap 2 pins
//
void CNetList::SwapPins( cpart * part1, CString * pin_name1,
						cpart * part2, CString * pin_name2 )
{
	// get pin1 info
	int pin_index1 = part1->shape->GetPinIndexByName( *pin_name1 );
	CPoint pin_pt1 = m_plist->GetPinPoint( part1, *pin_name1 );
	int pin_lay1 = m_plist->GetPinLayer( part1, *pin_name1 );
	cnet * net1 = m_plist->GetPinNet( part1, pin_name1 );
	int net1_pin_index = -1;
	if( net1 )
	{
		for( int ip=0; ip<net1->npins; ip++ )
		{
			if( net1->pin[ip].part == part1 && net1->pin[ip].pin_name == *pin_name1 )
			{
				net1_pin_index = ip;
				break;
			}
		}
		if( net1_pin_index ==  -1 )
			ASSERT(0);
	}

	// get pin2 info
	int pin_index2 = part2->shape->GetPinIndexByName( *pin_name2 );
	CPoint pin_pt2 = m_plist->GetPinPoint( part2, *pin_name2 );
	int pin_lay2 = m_plist->GetPinLayer( part2, *pin_name2 );
	cnet * net2 = m_plist->GetPinNet( part2, pin_name2 );
	int net2_pin_index = -1;
	if( net2 )
	{
		// find net2_pin_index for part2->pin2
		for( int ip=0; ip<net2->npins; ip++ )
		{
			if( net2->pin[ip].part == part2 && net2->pin[ip].pin_name == *pin_name2 )
			{
				net2_pin_index = ip;
				break;
			}
		}
		if( net2_pin_index ==  -1 )
			ASSERT(0);
	}

	if( net1 == NULL && net2 == NULL )
	{
		// both pins unconnected, there is nothing to do
		return;
	}
	else if( net1 == net2 )
	{
		// both pins on same net
		for( int ic=0; ic<net1->nconnects; ic++ )
		{
			cconnect * c = &net1->connect[ic];
			int p1 = c->start_pin;
			int p2 = c->end_pin;
			int nsegs = c->nsegs;
			if( nsegs )
			{
				if( p1 == net1_pin_index && p2 == net2_pin_index )
					continue;
				else if( p1 == net2_pin_index && p2 == net1_pin_index )
					continue;
				if( p1 == net1_pin_index )
				{
					// starting pin is on part, unroute first segment
					if( net1->connect[ic].seg[0].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net1, ic, 0 );
						nsegs = net1->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net1->connect[ic].vtx[0].x = pin_pt2.x;
					net1->connect[ic].vtx[0].y = pin_pt2.y;
					net1->connect[ic].vtx[0].pad_layer = pin_lay2;

					// now draw
					m_dlist->Set_x( net1->connect[ic].seg[0].dl_el, pin_pt2.x );
					m_dlist->Set_y( net1->connect[ic].seg[0].dl_el, pin_pt2.y );
					m_dlist->Set_visible( net1->connect[ic].seg[0].dl_el, net1->visible );

					m_dlist->Set_x( net1->connect[ic].seg[0].dl_sel, pin_pt2.x );
					m_dlist->Set_y( net1->connect[ic].seg[0].dl_sel, pin_pt2.y );
					m_dlist->Set_visible( net1->connect[ic].seg[0].dl_sel, net1->visible );
				}
				if( p2 == net1_pin_index )
				{
					// ending pin is on part, unroute last segment
					if( net1->connect[ic].seg[nsegs-1].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net1, ic, nsegs-1 );
						nsegs = net1->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net1->connect[ic].vtx[nsegs].x = pin_pt2.x;
					net1->connect[ic].vtx[nsegs].y = pin_pt2.y;
					net1->connect[ic].vtx[nsegs].pad_layer = pin_lay2;

					m_dlist->Set_xf( net1->connect[ic].seg[nsegs-1].dl_el, pin_pt2.x );
					m_dlist->Set_yf( net1->connect[ic].seg[nsegs-1].dl_el, pin_pt2.y );
					m_dlist->Set_visible( net1->connect[ic].seg[nsegs-1].dl_el, net1->visible );

					m_dlist->Set_xf( net1->connect[ic].seg[nsegs-1].dl_sel, pin_pt2.x );
					m_dlist->Set_yf( net1->connect[ic].seg[nsegs-1].dl_sel, pin_pt2.y );
					m_dlist->Set_visible( net1->connect[ic].seg[nsegs-1].dl_sel, net1->visible );
				}
				if( p1 == net2_pin_index )
				{
					// starting pin is on part, unroute first segment
					if( net2->connect[ic].seg[0].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net2, ic, 0 );
						nsegs = net2->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net2->connect[ic].vtx[0].x = pin_pt1.x;
					net2->connect[ic].vtx[0].y = pin_pt1.y;
					net2->connect[ic].vtx[0].pad_layer = pin_lay1;
					// now draw
					m_dlist->Set_x( net2->connect[ic].seg[0].dl_el, pin_pt1.x );
					m_dlist->Set_y( net2->connect[ic].seg[0].dl_el, pin_pt1.y );
					m_dlist->Set_visible( net2->connect[ic].seg[0].dl_el, net2->visible );

					m_dlist->Set_x( net2->connect[ic].seg[0].dl_sel, pin_pt1.x );
					m_dlist->Set_y( net2->connect[ic].seg[0].dl_sel, pin_pt1.y );
					m_dlist->Set_visible( net2->connect[ic].seg[0].dl_sel, net2->visible );
				}
				if( p2 == net2_pin_index )
				{
					// ending pin is on part, unroute last segment
					if( net2->connect[ic].seg[nsegs-1].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net2, ic, nsegs-1 );
						nsegs = net2->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net2->connect[ic].vtx[nsegs].x = pin_pt1.x;
					net2->connect[ic].vtx[nsegs].y = pin_pt1.y;
					net2->connect[ic].vtx[nsegs].pad_layer = pin_lay1;

					m_dlist->Set_xf( net2->connect[ic].seg[nsegs-1].dl_el, pin_pt1.x );
					m_dlist->Set_yf( net2->connect[ic].seg[nsegs-1].dl_el, pin_pt1.y );
					m_dlist->Set_visible( net2->connect[ic].seg[nsegs-1].dl_el, net2->visible );

					m_dlist->Set_xf( net2->connect[ic].seg[nsegs-1].dl_sel, pin_pt1.x );
					m_dlist->Set_yf( net2->connect[ic].seg[nsegs-1].dl_sel, pin_pt1.y );
					m_dlist->Set_visible( net2->connect[ic].seg[nsegs-1].dl_sel, net2->visible );
				}
			}
		}
		// reassign pin1
		net2->pin[net2_pin_index].pin_name = *pin_name1;
		// reassign pin2
		net1->pin[net1_pin_index].pin_name = *pin_name2;
		SetAreaConnections( net1 );
		return;
	}

	// now move all part1->pin1 connections to part2->pin2
	// change part2->pin2->net to net1
	// change net1->pin->part to part2
	// change net1->pin->ref_des to part2->ref_des
	// change net1->pin->pin_name to pin_name2
	if( net1 )
	{
		// remove any stub traces with one segment
		for( int ic=0; ic<net1->nconnects; ic++ )
		{
			int nsegs = net1->connect[ic].nsegs;
			if( nsegs == 1)
			{
				int p1 = net1->connect[ic].start_pin;
				int p2 = net1->connect[ic].end_pin;
				if( p1 == net1_pin_index && p2 == cconnect::NO_END )
				{
					// stub trace with 1 segment, remove it
					RemoveNetConnect( net1, ic, FALSE );
					ic--;
					continue;	// next connection
				}
			}
		}
		// now check all connections
		for( int ic=0; ic<net1->nconnects; ic++ )
		{
			int nsegs = net1->connect[ic].nsegs;
			if( nsegs )
			{
				int p1 = net1->connect[ic].start_pin;
				int p2 = net1->connect[ic].end_pin;
				if( p1 == net1_pin_index )
				{
					// starting pin is on part, unroute first segment
					if( net1->connect[ic].seg[0].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net1, ic, 0 );
						nsegs = net1->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net1->connect[ic].vtx[0].x = pin_pt2.x;
					net1->connect[ic].vtx[0].y = pin_pt2.y;
					net1->connect[ic].vtx[0].pad_layer = pin_lay2;
					// now draw
					m_dlist->Set_x( net1->connect[ic].seg[0].dl_el, pin_pt2.x );
					m_dlist->Set_y( net1->connect[ic].seg[0].dl_el, pin_pt2.y );
					m_dlist->Set_visible( net1->connect[ic].seg[0].dl_el, net1->visible );

					m_dlist->Set_x( net1->connect[ic].seg[0].dl_sel, pin_pt2.x );
					m_dlist->Set_y( net1->connect[ic].seg[0].dl_sel, pin_pt2.y );
					m_dlist->Set_visible( net1->connect[ic].seg[0].dl_sel, net1->visible );
				}
				if( p2 == net1_pin_index )
				{
					// ending pin is on part, unroute last segment
					if( net1->connect[ic].seg[nsegs-1].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net1, ic, nsegs-1 );
						nsegs = net1->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net1->connect[ic].vtx[nsegs].x = pin_pt2.x;
					net1->connect[ic].vtx[nsegs].y = pin_pt2.y;
					net1->connect[ic].vtx[nsegs].pad_layer = pin_lay2;

					m_dlist->Set_xf( net1->connect[ic].seg[nsegs-1].dl_el, pin_pt2.x );
					m_dlist->Set_yf( net1->connect[ic].seg[nsegs-1].dl_el, pin_pt2.y );
					m_dlist->Set_visible( net1->connect[ic].seg[nsegs-1].dl_el, net1->visible );

					m_dlist->Set_xf( net1->connect[ic].seg[nsegs-1].dl_sel, pin_pt2.x );
					m_dlist->Set_yf( net1->connect[ic].seg[nsegs-1].dl_sel, pin_pt2.y );
					m_dlist->Set_visible( net1->connect[ic].seg[nsegs-1].dl_sel, net1->visible );
				}
			}
		}
		// reassign pin2 to net1
		net1->pin[net1_pin_index].part = part2;
		net1->pin[net1_pin_index].set_ref_des( part2->ref_des );
		net1->pin[net1_pin_index].pin_name = *pin_name2;

		part2->pin[pin_index2].set_net( net1 );
	}
	else
	{
		// pin2 is unconnected
		part2->pin[pin_index2].set_net();
	}
	// now move all part2->pin2 connections to part1->pin1
	// change part1->pin1->net to net2
	// change net2->pin->part to part1
	// change net2->pin->ref_des to part1->ref_des
	// change net2->pin->pin_name to pin_name1
	if( net2 )
	{
		// second pin is connected
		// remove any stub traces with one segment
		for( int ic=0; ic<net2->nconnects; ic++ )
		{
			int nsegs = net2->connect[ic].nsegs;
			if( nsegs == 1 )
			{
				int p1 = net2->connect[ic].start_pin;
				int p2 = net2->connect[ic].end_pin;
				if( p1 == net2_pin_index && p2 == cconnect::NO_END )
				{
					// stub trace with 1 segment, remove it
					RemoveNetConnect( net2, ic, FALSE );
					ic--;
					continue;
				}
			}
		}
		// now check all connections
		for( int ic=0; ic<net2->nconnects; ic++ )
		{
			int nsegs = net2->connect[ic].nsegs;
			if( nsegs )
			{
				int p1 = net2->connect[ic].start_pin;
				int p2 = net2->connect[ic].end_pin;
				if( p1 == net2_pin_index )
				{
					// starting pin is on part, unroute first segment
					if( net2->connect[ic].seg[0].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net2, ic, 0 );
						nsegs = net2->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net2->connect[ic].vtx[0].x = pin_pt1.x;
					net2->connect[ic].vtx[0].y = pin_pt1.y;
					net2->connect[ic].vtx[0].pad_layer = pin_lay1;
					// now draw
					m_dlist->Set_x( net2->connect[ic].seg[0].dl_el, pin_pt1.x );
					m_dlist->Set_y( net2->connect[ic].seg[0].dl_el, pin_pt1.y );
					m_dlist->Set_visible( net2->connect[ic].seg[0].dl_el, net2->visible );

					m_dlist->Set_x( net2->connect[ic].seg[0].dl_sel, pin_pt1.x );
					m_dlist->Set_y( net2->connect[ic].seg[0].dl_sel, pin_pt1.y );
					m_dlist->Set_visible( net2->connect[ic].seg[0].dl_sel, net2->visible );
				}
				if( p2 == net2_pin_index )
				{
					// ending pin is on part, unroute last segment
					if( net2->connect[ic].seg[nsegs-1].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net2, ic, nsegs-1 );
						nsegs = net2->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net2->connect[ic].vtx[nsegs].x = pin_pt1.x;
					net2->connect[ic].vtx[nsegs].y = pin_pt1.y;
					net2->connect[ic].vtx[nsegs].pad_layer = pin_lay1;

					m_dlist->Set_xf( net2->connect[ic].seg[nsegs-1].dl_el, pin_pt1.x );
					m_dlist->Set_yf( net2->connect[ic].seg[nsegs-1].dl_el, pin_pt1.y );
					m_dlist->Set_visible( net2->connect[ic].seg[nsegs-1].dl_el, net2->visible );

					m_dlist->Set_xf( net2->connect[ic].seg[nsegs-1].dl_sel, pin_pt1.x );
					m_dlist->Set_yf( net2->connect[ic].seg[nsegs-1].dl_sel, pin_pt1.y );
					m_dlist->Set_visible( net2->connect[ic].seg[nsegs-1].dl_sel, net2->visible );
				}
			}
		}
		// reassign pin1 to net2
		net2->pin[net2_pin_index].part = part1;
		net2->pin[net2_pin_index].set_ref_des( part1->ref_des );
		net2->pin[net2_pin_index].pin_name = *pin_name1;
		part1->pin[pin_index1].set_net( net2 );
	}
	else
	{
		// pin2 is unconnected
		part1->pin[pin_index1].set_net();
	}
	SetAreaConnections( net1 );
	SetAreaConnections( net2 );
}


// Part moved, so unroute starting and ending segments of connections
// to this part, and update positions of endpoints
// Undraw and Redraw any changed connections
//
int CNetList::PartMoved( cpart * part )
{
	// first, mark all nets and connections unmodified
	cnet * net;
	CIterator_cnet net_iter(this);
	for( net = net_iter.GetFirst(); net != NULL; net = net_iter.GetNext() )
	{
		net->utility = 0;
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			net->connect[ic].utility = 0;
		}
	}

	// disable drawing/undrawing
	CDisplayList * old_dlist = m_dlist;
	m_dlist = 0;

	// find nets that connect to this part
	for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
	{
		net = (cnet*)part->pin[ip].net;
		if( net )
		{
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				int nsegs = c->nsegs;
				if( nsegs )
				{
					// check this connection
					int p1 = c->start_pin;
					CString pin_name1 = net->pin[p1].pin_name;
					int pin_index1 = part->shape->GetPinIndexByName( pin_name1 );
					int p2 = c->end_pin;
					cseg * s0 = &c->seg[0];
					cvertex * v0 = &c->vtx[0];
					if( net->pin[p1].part == part )
					{
						// start pin is on part, unroute first segment
						net->utility = 1;	// mark net modified
						c->utility = 1;		// mark connection modified
//BAF						UnrouteSegment( net, ic, 0 );
						nsegs = c->nsegs;
						// modify vertex[0] position and layer
						v0->x = part->pin[pin_index1].x;
						v0->y = part->pin[pin_index1].y;
						if( part->shape->m_padstack[pin_index1].hole_size )
						{
							// through-hole pad
							v0->pad_layer = LAY_PAD_THRU;
						}
						else if( part->side == 0 && part->shape->m_padstack[pin_index1].top.shape != PAD_NONE
							|| part->side == 1 && part->shape->m_padstack[pin_index1].bottom.shape != PAD_NONE )
						{
							// SMT pad on top
							v0->pad_layer = LAY_TOP_COPPER;
						}
						else
						{
							// SMT pad on bottom
							v0->pad_layer = LAY_BOTTOM_COPPER;
						}
						if( part->pin[pin_index1].net != net )
							part->pin[pin_index1].set_net( net );
					}
					if( p2 != cconnect::NO_END )
					{
						if( net->pin[p2].part == part )
						{
							// end pin is on part, unroute last segment
							net->utility = 1;	// mark net modified
							c->utility = 1;		// mark connection modified
//BAF							UnrouteSegment( net, ic, nsegs-1 );
							nsegs = c->nsegs;
							// modify vertex position and layer
							CString pin_name2 = net->pin[p2].pin_name;
							int pin_index2 = part->shape->GetPinIndexByName( pin_name2 );
							c->vtx[nsegs].x = part->pin[pin_index2].x;
							c->vtx[nsegs].y = part->pin[pin_index2].y;
							if( part->shape->m_padstack[pin_index2].hole_size )
							{
								// through-hole pad
								c->vtx[nsegs].pad_layer = LAY_PAD_THRU;
							}
							else if( part->side == 0 && part->shape->m_padstack[pin_index2].top.shape != PAD_NONE
								|| part->side == 0 && part->shape->m_padstack[pin_index2].top.shape != PAD_NONE )
							{
								// SMT pad, part on top
								c->vtx[nsegs].pad_layer = LAY_TOP_COPPER;
							}
							else
							{
								// SMT pad, part on bottom
								c->vtx[nsegs].pad_layer = LAY_BOTTOM_COPPER;
							}
							if( part->pin[pin_index2].net != net )
								part->pin[pin_index2].set_net( net );
						}
					}
				}
			}
		}
	}

	// now redraw connections
	m_dlist = old_dlist;
	if( m_dlist )
	{
		CIterator_cnet net_iter(this);
		for( cnet * net = net_iter.GetFirst(); net != NULL; net = net_iter.GetNext() )
		{
			if( net->utility )
			{
				for( int ic=0; ic<net->nconnects; ic++ )
				{
					if( net->connect[ic].utility )
						DrawConnection( net, ic );
				}
			}
		}
	}
	return 0;
}

// Part footprint changed, check new pins and positions
// If changed, unroute starting and ending segments of connections
// to this part, and update positions of endpoints
//
int CNetList::PartFootprintChanged( cpart * part )
{
	POSITION pos;
	CString name;
	void * ptr;

	// first, clear existing net assignments to part pins
	for( int ip=0; ip<part->pin.GetSize(); ip++ )
		part->pin[ip].set_net();

	// find nets which connect to this part
	CIterator_cnet net_iter(this);
	for( cnet * net = net_iter.GetFirst(); net != NULL; net = net_iter.GetNext() )
	{
		// check each connection in net
		for( int ic=net->nconnects-1; ic>=0; ic-- )
		{
			cconnect * c = &net->connect[ic];
			int nsegs = c->nsegs;
			if( nsegs )
			{
				int p1 = c->start_pin;
				int p2 = c->end_pin;
				if( net->pin[p1].part != part )
				{
					// connection doesn't start on this part
					if( p2 == cconnect::NO_END )
						continue; // stub trace, ignore it
					if( net->pin[p2].part != part )
						continue;	// doesn't end on part, ignore it
				}
				CString pin_name1 = net->pin[p1].pin_name;
				if( net->pin[p1].part == part )
				{
					// starting pin is on part, see if this pin still exists
					int pin_index1 = part->shape->GetPinIndexByName( pin_name1 );
					if( pin_index1 == -1 )
					{
						// no, remove connection
						RemoveNetConnect( net, ic, FALSE );
						continue;
					}
					// yes, rehook pin to net
					part->pin[pin_index1].set_net( net );
					// see if position or pad type has changed
					int old_x = c->vtx[0].x;
					int old_y = c->vtx[0].y;
					int old_layer = c->seg[0].layer;
					int new_x = part->pin[pin_index1].x;
					int new_y = part->pin[pin_index1].y;
					int new_layer;
					if( part->side == 0 )
						new_layer = LAY_TOP_COPPER;
					else
						new_layer = LAY_BOTTOM_COPPER;
					BOOL layer_ok = new_layer == old_layer || part->shape->m_padstack[pin_index1].hole_size > 0;
					// see if pin position has changed
					if( old_x != new_x || old_y != new_y || !layer_ok )
					{
						// yes, unroute if necessary and update connection
						if( old_layer != LAY_RAT_LINE )
						{
							UnrouteSegment( net, ic, 0 );
							nsegs = c->nsegs;
						}
						// modify vertex position
						c->vtx[0].x = new_x;
						c->vtx[0].y = new_y;
						m_dlist->Set_x( c->seg[0].dl_el, c->vtx[0].x );
						m_dlist->Set_y( c->seg[0].dl_el, c->vtx[0].y );
						m_dlist->Set_visible( c->seg[0].dl_el, net->visible );

						m_dlist->Set_x( c->seg[0].dl_sel, c->vtx[0].x );
						m_dlist->Set_y( c->seg[0].dl_sel, c->vtx[0].y );
						m_dlist->Set_visible( c->seg[0].dl_sel, net->visible );
					}
				}
				if( p2 == cconnect::NO_END )
					continue;
				CString pin_name2 = net->pin[p2].pin_name;
				if( net->pin[p2].part == part )
				{
					// ending pin is on part, see if this pin still exists
					int pin_index2 = part->shape->GetPinIndexByName( pin_name2 );
					if( pin_index2 == -1 )
					{
						// no, remove connection
						RemoveNetConnect( net, ic, FALSE );
						continue;
					}
					// rehook pin to net
					part->pin[pin_index2].set_net( net );

					// see if position has changed
					int old_x = c->vtx[nsegs].x;
					int old_y = c->vtx[nsegs].y;
					int old_layer = c->seg[nsegs-1].layer;
					int new_x = part->pin[pin_index2].x;
					int new_y = part->pin[pin_index2].y;
					int new_layer;
					if( part->side == 0 )
						new_layer = LAY_TOP_COPPER;
					else
						new_layer = LAY_BOTTOM_COPPER;
					BOOL layer_ok = (new_layer == old_layer) || (part->shape->m_padstack[pin_index2].hole_size > 0);
					if( old_x != new_x || old_y != new_y || !layer_ok )
					{
						// yes, unroute if necessary and update connection
						if( c->seg[nsegs-1].layer != LAY_RAT_LINE )
						{
							UnrouteSegment( net, ic, nsegs-1 );
							nsegs = c->nsegs;
						}
						// modify vertex position
						c->vtx[nsegs].x = new_x;
						c->vtx[nsegs].y = new_y;
						m_dlist->Set_xf( c->seg[nsegs-1].dl_el, c->vtx[nsegs].x );
						m_dlist->Set_yf( c->seg[nsegs-1].dl_el, c->vtx[nsegs].y );
						m_dlist->Set_visible( c->seg[nsegs-1].dl_el, net->visible );

						m_dlist->Set_xf( c->seg[nsegs-1].dl_sel, c->vtx[nsegs].x );
						m_dlist->Set_yf( c->seg[nsegs-1].dl_sel, c->vtx[nsegs].y );
						m_dlist->Set_visible( c->seg[nsegs-1].dl_sel, net->visible );
					}
				}
			}
		}
		// now see if new connections need to be added
		for( int ip=0; ip<net->pin.GetSize(); ip++ )
		{
			if( net->pin[ip].ref_des() == part->ref_des )
			{
				int pin_index = part->shape->GetPinIndexByName( net->pin[ip].pin_name );
				if( pin_index == -1 )
				{
					// pin doesn't exist in part
				}
				else
				{
					// pin exists, see if connected to net
					if( part->pin[pin_index].net != net )
					{
						// no, make connection
						part->pin[pin_index].set_net( net );
						net->pin[ip].part = part;
					}
				}
			}
		}
		RemoveOrphanBranches( net, 0, TRUE );
	}
	return 0;
}

// Part deleted, so unroute and remove all connections to this part
// and remove all references from netlist
//
int CNetList::PartDeleted( cpart * part )
{
	// find nets which connect to this part, remove pins and adjust areas
	POSITION pos;
	CString name;
	void * ptr;

	// find nets which connect to this part
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		for( int ip=0; ip<net->pin.GetSize(); )
		{
			if( net->pin[ip].ref_des() == part->ref_des )
			{
				RemoveNetPin( net, net->pin[ip].ref_des(), net->pin[ip].pin_name );
			}
			else
			{
				ip++;
			}
		}
		RemoveOrphanBranches( net, 0, TRUE );
	}
	return 0;
}

// Part reference designator changed
// replace all references from netlist
//
void CNetList::PartRefChanged( CString * old_ref_des, CString * new_ref_des )
{
	// find nets which connect to this part, adjust pin names
	POSITION pos;
	CString name;
	void * ptr;

	// find nets which connect to this part
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		for( int ip=0; ip<net->pin.GetSize(); ip++ )
		{
			if( net->pin[ip].ref_des() == *old_ref_des )
				net->pin[ip].set_ref_des( *new_ref_des );
		}
	}
}

// Part disconnected, so unroute and remove all connections to this part
// Also remove net pointers from part->pins
// and part pointer from net->pins
// Do not remove pins from netlist, however
//
int CNetList::PartDisconnected( cpart * part )
{
	// find nets which connect to this part, remove pins and adjust areas
	POSITION pos;
	CString name;
	void * ptr;

	// find nets which connect to this part
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		for( int ip=0; ip<net->pin.GetSize(); ip++ )
		{
			if( net->pin[ip].ref_des() == part->ref_des )
			{
				DisconnectNetPin( net, net->pin[ip].ref_des(), net->pin[ip].pin_name );
			}
		}
		RemoveOrphanBranches( net, 0, TRUE );
	}
	return 0;
}

// utility function used by OptimizeConnections()
//
void AddPinsToGrid( char * grid, int p1, int p2, int npins )
{
#define GRID(a,b) grid[a*npins+b]
#define COPY_ROW(a,b) for(int k=0;k<npins;k++){ if( GRID(a,k) ) GRID(b,k) = 1; }

	// add p2 to row p1
	GRID(p1,p2) = 1;
	// now copy row p2 into p1
	COPY_ROW(p2,p1);
	// now copy row p1 into each row connected to p1
	for( int ip=0; ip<npins; ip++ )
		if( GRID(p1,ip) )
			COPY_ROW(p1, ip);
}

// optimize all unrouted connections
//
void CNetList::OptimizeConnections()
{
	// traverse map
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		OptimizeConnections( net );
	}
}

// optimize all unrouted connections for a part
//
void CNetList::OptimizeConnections( cpart * part )
{
	// find nets which connect to this part
	cnet * net;
	if( part->shape )
	{
		// mark all nets unoptimized
		for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
		{
			net = (cnet*)part->pin[ip].net;
			if( net )
				net->utility = 0;
		}
		// optimize each net and mark it optimized so it won't be repeated
		for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
		{
			net = (cnet*)part->pin[ip].net;
			if( net )
			{
				if( net->utility == 0 )
				{
					OptimizeConnections( net );
					net->utility = 1;
				}
			}
		}
	}
}

// optimize the unrouted connections for a net
// if ic_track >= 0, returns new ic corresponding to old ic or -1 if unable
//
int CNetList::OptimizeConnections( cnet * net, int ic_track )
{
#ifdef PROFILE
	StartTimer();	//****
#endif

	// get number of pins N and make grid[NxN] array and pair[N*2] array
	int npins = net->npins;
	char * grid = (char*)calloc( npins*npins, sizeof(char) );
	for( int ip=0; ip<npins; ip++ )
		grid[ip*npins+ip] = 1;
	CArray<int> pair;			// use collection class because size is unknown,
	pair.SetSize( 2*npins );	// although this should be plenty

	// first, flag ic_track if requested
	int ic_new = -1;
	if( ic_track >= 0 && ic_track < net->nconnects )
	{
		for( int ic=0; ic<net->nconnects; ic++ )
			net->connect[ic].utility = 0;
		net->connect[ic_track].utility = 1;		// flag connection
	}

	// go through net, deleting unrouted and unlocked connections
	// and recording pins of routed or locked connections
	for( int ic=0; ic<net->nconnects; /* ic++ is deferred */ )
	{
		cconnect * c = &net->connect[ic];
		int routed = 0;
		if( c->nsegs > 1
			|| c->seg[0].layer != LAY_RAT_LINE
			|| c->end_pin == cconnect::NO_END )
			routed = 1;
		int p1, p2;
		if( routed || c->locked )
		{
			// routed or locked...don't delete connection
			// record pins in pair[] and grid[]
			p1 = c->start_pin;
			p2 = c->end_pin;
			if( p2 != cconnect::NO_END )
			{
				AddPinsToGrid( grid, p1, p2, npins );
			}
			// if a stub, record connection to tee
			else
			{
				if( int id = c->vtx[c->nsegs].tee_ID )
				{
					int ic;
					int iv;
					BOOL bFound = FindTeeVertexInNet( net, id, &ic, &iv );
					if( !bFound )
						ASSERT(0);
					// get start of tee trace
					cconnect * tee_c = &net->connect[ic];
					int tee_p1 = tee_c->start_pin;
					AddPinsToGrid( grid, p1, tee_p1, npins );
				}
			}
			// increment counter
			ic++;
		}
		else
		{
			// unrouted and unlocked, so delete connection
			// don't advance ic or n_routed
			RemoveNetConnect( net, ic, FALSE );
		}
	}

	// find ic_track if still present
	if( ic_track >= 0 )
	{
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			if( net->connect[ic].utility == 1 )
			{
				ic_new = ic;
				break;
			}
		}
	}

	//** TEMP
	if( net->visible == 0 )
	{
		free( grid );
		return ic_new;
	}

	// now add pins connected to copper areas
	for( int ia=0; ia<net->nareas; ia++ )
	{
		SetAreaConnections( net, ia );
		if( (net->area[ia].npins + net->area[ia].nvias) > 1 )
		{
			int p1, p2, ic;
			if( net->area[ia].npins > 0 )
				p1 = net->area[ia].pin[0];
			else
			{
				ic = net->area[ia].vcon[0];
				cconnect * c = &net->connect[ic];
				p1 = c->start_pin;
			}
			for( int ip=1; ip<net->area[ia].npins; ip++ )
			{
				p2 = net->area[ia].pin[ip];
				if( p2 != p1 )
				{
					AddPinsToGrid( grid, p1, p2, npins );
				}
			}
			for( int iv=0; iv<net->area[ia].nvias; iv++ )
			{
				ic = net->area[ia].vcon[iv];
				cconnect * c = &net->connect[ic];
				p2 = c->start_pin;
				if( p2 != p1 )
				{
					AddPinsToGrid( grid, p1, p2, npins );
				}
				if( c->end_pin != cconnect::NO_END )
				{
					p2 = c->end_pin;
					if( p2 != p1 )
					{
						AddPinsToGrid( grid, p1, p2, npins );
					}
				}
			}
		}
	}
#ifdef PROFILE
	double time1 = GetElapsedTime();
	StartTimer();
#endif

	// now optimize the unrouted and unlocked connections
	long num_loops = 0;
	int n_optimized = 0;
	int min_p1, min_p2, flag;
	double min_dist;

	// create arrays of pin params for efficiency
	CArray<BOOL>legal;
	CArray<double>x, y;
	CArray<double>d;
	x.SetSize(npins);
	y.SetSize(npins);
	d.SetSize(npins*npins);
	legal.SetSize(npins);
	CPoint p;
	for( int ip=0; ip<npins; ip++ )
	{
		legal[ip] = FALSE;
		cpart * part = net->pin[ip].part;
		if( part )
			if( part->shape )
			{
				{
					CString pin_name = net->pin[ip].pin_name;
					int pin_index = part->shape->GetPinIndexByName( pin_name );
					if( pin_index != -1 )
					{
						p = m_plist->GetPinPoint( net->pin[ip].part, pin_name );
						x[ip] = p.x;
						y[ip] = p.y;
						legal[ip] = TRUE;
					}
				}
			}
	}
	for( int p1=0; p1<npins; p1++ )
	{
		for( int p2=0; p2<p1; p2++ )
		{
			if( legal[p1] && legal[p2] )
			{
				double dist = sqrt((x[p1]-x[p2])*(x[p1]-x[p2])+(y[p1]-y[p2])*(y[p1]-y[p2]));
				d[p1*npins+p2] = dist;
				d[p2*npins+p1] = dist;
			}
		}
	}

	// make array of distances for all pin pairs p1 and p2
	// where p2<p1 and index = (p1)*(p1-1)/2
	// first, get number of legal pins
	int n_legal = 0;
	for( int p1=0; p1<npins; p1++ )
		if( legal[p1] )
			n_legal++;

	int n_elements = (n_legal*(n_legal-1))/2;
	int * numbers = (int*)calloc( sizeof(int), n_elements );
	int * index = (int*)calloc( sizeof(int), n_elements );
	int i = 0;
	for( int p1=1; p1<npins; p1++ )
	{
		for( int p2=0; p2<p1; p2++ )
		{
			if( legal[p1] && legal[p2] )
			{
				index[i] = p1*npins + p2;
				double number = d[p1*npins+p2];
				if( number > INT_MAX )
					ASSERT(0);
				numbers[i] = number;
				i++;
				if( i > n_elements )
					ASSERT(0);
			}
		}
	}
	// sort
	::q_sort(numbers, index, 0, n_elements - 1);
	for( int i=0; i<n_elements; i++ )
	{
		int dd = numbers[i];
		int p1 = index[i]/npins;
		int p2 = index[i]%npins;
		if( i>0 )
		{
			if( dd < numbers[i-1] )
				ASSERT(0);
		}
	}

	// now make connections, shortest first
	for( int i=0; i<n_elements; i++ )
	{
		int p1 = index[i]/npins;
		int p2 = index[i]%npins;
		// find shortest connection between unconnected pins
		if( legal[p1] && legal[p2] && !grid[p1*npins+p2] )
		{
			// connect p1 to p2
			AddPinsToGrid( grid, p1, p2, npins );
			pair.SetAtGrow(n_optimized*2, p1);
			pair.SetAtGrow(n_optimized*2+1, p2);
			n_optimized++;
		}
	}
	free( numbers );
	free( index );

	// add new optimized connections
	for( int ic=0; ic<n_optimized; ic++ )
	{
		// make new connection with a single unrouted segment
		int p1 = pair[ic*2];
		int p2 = pair[ic*2+1];
		AddNetConnect( net, p1, p2 );
	}

	free( grid );

#ifdef PROFILE
	double time2 = GetElapsedTime();
	if( net->name == "GND" )
	{
		CString mess;
		mess.Format( "net \"%s\", %d pins\nloops = %ld\ntime1 = %f\ntime2 = %f",
			net->name, net->npins, num_loops, time1, time2 );
		AfxMessageBox( mess );
	}
#endif

	return ic_new;
}

// reset pointers on part pins for net
//
int CNetList::RehookPartsToNet( cnet * net )
{
	for( int ip=0; ip<net->npins; ip++ )
	{
		CString ref_des = net->pin[ip].ref_des();
		CString pin_name = net->pin[ip].pin_name;
		cpart * part = m_plist->GetPart( ref_des );
		if( part )
		{
			if( part->shape )
			{
				int pin_index = part->shape->GetPinIndexByName( pin_name );
				if( pin_index != -1 )
				{
					part_pin *pp = &part->pin[pin_index];

					pp->set_net( net );
				}
			}
		}
	}
	return 0;
}

void CNetList::SetViaVisible( cnet * net, int ic, int iv, BOOL visible )
{
	cconnect * c = &net->connect[ic];
	cvertex * v = &c->vtx[iv];
	for( int il=0; il < v->dl_el.GetSize(); il++ )
		if( v->dl_el[il] )
			v->dl_el[il]->visible = visible;
	if( v->dl_hole )
		v->dl_hole->visible = visible;
}

// start dragging end vertex of a stub trace to move it
//
void CNetList::StartDraggingEndVertex( CDC * pDC, cnet * net, int ic, int ivtx, int crosshair )
{
	cconnect * c = &net->connect[ic];
	m_dlist->CancelHighLight();
	c->seg[ivtx-1].dl_el->visible = 0;
	SetViaVisible( net, ic, ivtx, FALSE );

	m_dlist->StartDraggingLine( pDC,
		c->vtx[ivtx-1].x,
		c->vtx[ivtx-1].y,
		c->vtx[ivtx-1].x,
		c->vtx[ivtx-1].y,
		c->seg[ivtx-1].layer,
		c->seg[ivtx-1].width(),
		c->seg[ivtx-1].layer,
		0, 0, crosshair, DSS_STRAIGHT, IM_NONE );
}

// cancel dragging end vertex of a stub trace
//
void CNetList::CancelDraggingEndVertex( cnet * net, int ic, int ivtx )
{
	cconnect * c = &net->connect[ic];
	m_dlist->StopDragging();
	c->seg[ivtx-1].dl_el->visible = 1;
	SetViaVisible( net, ic, ivtx, TRUE );
}

// move end vertex of a stub trace
//
void CNetList::MoveEndVertex( cnet * net, int ic, int ivtx, int x, int y )
{
	cconnect * c = &net->connect[ic];
	m_dlist->StopDragging();
	c->vtx[ivtx].x = x;
	c->vtx[ivtx].y = y;

	m_dlist->Set_xf( c->seg[ivtx-1].dl_el, x );
	m_dlist->Set_yf( c->seg[ivtx-1].dl_el, y );
	m_dlist->Set_visible( c->seg[ivtx-1].dl_el, 1 );

	m_dlist->Set_xf( c->seg[ivtx-1].dl_sel, x );
	m_dlist->Set_yf( c->seg[ivtx-1].dl_sel, y );

	c->vtx[ivtx].x = x;
	c->vtx[ivtx].y = y;
	ReconcileVia( net, ic, ivtx );
	for( int ia=0; ia<net->nareas; ia++ )
		SetAreaConnections( net, ia );
}

// move vertex
//
void CNetList::MoveVertex( cnet * net, int ic, int ivtx, int x, int y )
{
	cvertex * v;
	cconnect * c;

	m_dlist->StopDragging();

	CVertexIterator vi(net, ic, ivtx);
	for( v = vi.GetFirst(); v != NULL; v = vi.GetNext() )
	{
		v->x = x;
		v->y = y;

		// Remove selector from stub/branch traces
		if( vi.get_index() > 0 )
		{
			if( v->dl_sel )
			{
				m_dlist->Remove( v->dl_sel );
				v->dl_sel = NULL;
			}
		}

		c = vi.getcur_connect();
		ivtx = vi.getcur_ivtx();
		ic   = vi.getcur_ic();

		if( ivtx > 0 )
		{
			if( c->seg[ivtx-1].dl_el )
			{
				m_dlist->Set_xf( c->seg[ivtx-1].dl_el, x );
				m_dlist->Set_yf( c->seg[ivtx-1].dl_el, y );
				m_dlist->Set_visible( c->seg[ivtx-1].dl_el, 1 );
			}
			if( c->seg[ivtx-1].dl_sel )
			{
				m_dlist->Set_xf( c->seg[ivtx-1].dl_sel, x );
				m_dlist->Set_yf( c->seg[ivtx-1].dl_sel, y );
			}
		}
		if( ivtx < c->nsegs )
		{
			if( c->seg[ivtx].dl_el )
			{
				m_dlist->Set_x( c->seg[ivtx].dl_el, x );
				m_dlist->Set_y( c->seg[ivtx].dl_el, y );
				m_dlist->Set_visible( c->seg[ivtx].dl_el, 1 );
			}
			if( c->seg[ivtx].dl_sel )
			{
				m_dlist->Set_x( c->seg[ivtx].dl_sel, x );
				m_dlist->Set_y( c->seg[ivtx].dl_sel, y );
			}
		}
		ReconcileVia( net, ic, ivtx );
	}
}


// Start dragging trace vertex
//
int CNetList::StartDraggingVertex( CDC * pDC, cnet * net, int ic, int ivtx,
								   int x, int y, int crosshair )
{
	// cancel previous selection and make segments and via invisible
	cconnect * c =&net->connect[ic];
	cvertex * v = &c->vtx[ivtx];
	m_dlist->CancelHighLight();

	m_dlist->Set_visible(c->seg[ivtx-1].dl_el, 0);
	m_dlist->Set_visible(c->seg[ivtx].dl_el, 0);

	SetViaVisible( net, ic, ivtx, FALSE );

	// if tee connection, also drag tee segment(s)
	if( v->tee_ID && ivtx < c->nsegs )
	{
		int ntsegs = 0;
		// find all tee segments
		for( int icc=0; icc<net->nconnects; icc++ )
		{
			cconnect * cc = &net->connect[icc];
			if( cc != c && cc->end_pin == cconnect::NO_END )
			{
				cvertex * vv = &cc->vtx[cc->nsegs];
				if( vv->tee_ID == v->tee_ID )
				{
					ntsegs++;
				}
			}
		}
		m_dlist->MakeDragRatlineArray( ntsegs, 0 );
		// now add them one-by-one
		for( int icc=0; icc<net->nconnects; icc++ )
		{
			cconnect * cc = &net->connect[icc];
			if( cc != c && cc->end_pin == cconnect::NO_END )
			{
				cvertex * vv = &cc->vtx[cc->nsegs];
				if( vv->tee_ID == v->tee_ID )
				{
					CPoint pi, pf;
					pi.x = cc->vtx[cc->nsegs-1].x;
					pi.y = cc->vtx[cc->nsegs-1].y;
					pf.x = 0;
					pf.y = 0;
					m_dlist->AddDragRatline( pi, pf );
					m_dlist->Set_visible( cc->seg[cc->nsegs-1].dl_el, 0 );
				}
			}
		}
		m_dlist->StartDraggingArray( pDC, 0, 0, 0, LAY_RAT_LINE );
	}

	// start dragging
	int xi = c->vtx[ivtx-1].x;
	int yi = c->vtx[ivtx-1].y;
	int xf = c->vtx[ivtx+1].x;
	int yf = c->vtx[ivtx+1].y;
	int layer1 = c->seg[ivtx-1].layer;
	int layer2 = c->seg[ivtx].layer;
	int w1 = c->seg[ivtx-1].width();
	int w2 = c->seg[ivtx].width();
	m_dlist->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, layer1,
								layer2, w1, w2, DSS_STRAIGHT, DSS_STRAIGHT,
								0, 0, 0, 0, crosshair );
	return 0;
}

// Start moving trace segment
//
int CNetList::StartMovingSegment( CDC * pDC, cnet * net, int ic, int ivtx,
								   int x, int y, int crosshair, int use_third_segment )
{
	// cancel previous selection and make segments and via invisible
	cconnect * c =&net->connect[ic];
	cvertex * v = &c->vtx[ivtx];
	m_dlist->CancelHighLight();
	m_dlist->Set_visible(c->seg[ivtx-1].dl_el, 0);
	m_dlist->Set_visible(c->seg[ivtx].dl_el, 0);
	SetViaVisible( net, ic, ivtx,   FALSE );
	SetViaVisible( net, ic, ivtx+1, FALSE );
	if(use_third_segment)
	{
		m_dlist->Set_visible(c->seg[ivtx+1].dl_el, 0);
	}

	// start dragging
	ASSERT(ivtx > 0);

	int	xb = c->vtx[ivtx-1].x;
	int	yb = c->vtx[ivtx-1].y;
	int xi = c->vtx[ivtx  ].x;
	int yi = c->vtx[ivtx  ].y;
	int xf = c->vtx[ivtx+1].x;
	int yf = c->vtx[ivtx+1].y;


	int layer0 = c->seg[ivtx-1].layer;
	int layer1 = c->seg[ivtx].layer;

	int w0 = c->seg[ivtx-1].width();
	int w1 = c->seg[ivtx].width();

	int xe = 0, ye = 0;
	int layer2 = 0;
	int w2 = 0;
	if(use_third_segment)
	{
		xe = c->vtx[ivtx+2].x;
		ye = c->vtx[ivtx+2].y;
		layer2 = c->seg[ivtx+1].layer;
		w2 = c->seg[ivtx+1].width();
	}
	m_dlist->StartDraggingLineSegment( pDC, x, y, xb, yb, xi, yi, xf, yf, xe, ye,
									layer0, layer1, layer2,
									w0,		w1,		w2,
									DSS_STRAIGHT, DSS_STRAIGHT, use_third_segment?DSS_STRAIGHT:DSS_NONE,
									0, 0, 0,
									crosshair );
	return 0;
}

// Start dragging trace segment
//
int CNetList::StartDraggingSegment( CDC * pDC, cnet * net, int ic, int iseg,
								   int x, int y, int layer1, int layer2,
								   int layer_no_via, CConnectionWidthInfo const &width,
								   int dir, int crosshair )
{
	// cancel previous selection and make segment invisible
	cconnect * c =&net->connect[ic];
	m_dlist->CancelHighLight();
	m_dlist->Set_visible(c->seg[iseg].dl_el, 0);

	// start dragging
	int xi = c->vtx[iseg].x;
	int yi = c->vtx[iseg].y;
	int xf = c->vtx[iseg+1].x;
	int yf = c->vtx[iseg+1].y;

	m_dlist->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, layer1,
		layer2, width.m_seg_width.m_val, 1, DSS_STRAIGHT, DSS_STRAIGHT,
		layer_no_via,
		width.m_via_width.m_val,
		width.m_via_hole.m_val, dir, crosshair );

	return 0;
}

// Start dragging new vertex in existing trace segment
//
int CNetList::StartDraggingSegmentNewVertex( CDC * pDC, cnet * net, int ic, int iseg,
								   int x, int y, int layer, int w, int crosshair )
{
	// cancel previous selection and make segment invisible
	cconnect * c =&net->connect[ic];
	m_dlist->CancelHighLight();
	m_dlist->Set_visible(c->seg[iseg].dl_el, 0);
	// start dragging
	int xi = c->vtx[iseg].x;
	int yi = c->vtx[iseg].y;
	int xf = c->vtx[iseg+1].x;
	int yf = c->vtx[iseg+1].y;
	m_dlist->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, layer,
								layer, w, w, DSS_STRAIGHT, DSS_STRAIGHT,
								layer, 0, 0, 0, crosshair );
	return 0;
}

// Start dragging stub trace segment, iseg is index of new segment
//
void CNetList::StartDraggingStub( CDC * pDC, cnet * net, int ic, int iseg,
								   int x, int y, int layer1, int w,
								   int layer_no_via, int via_w, int via_hole_w,
								   int crosshair, int inflection_mode )
{
	cconnect * c = &net->connect[ic];
	m_dlist->CancelHighLight();
	SetViaVisible( net, ic, iseg, FALSE );

	// start dragging, start point is preceding vertex
	int xi = c->vtx[iseg].x;
	int yi = c->vtx[iseg].y;
	m_dlist->StartDraggingLine( pDC, x, y, xi, yi, layer1,
		w, layer_no_via, via_w, via_hole_w,
		crosshair, DSS_STRAIGHT, inflection_mode );
}

// Cancel dragging stub trace segment
//
void CNetList::CancelDraggingStub( cnet * net, int ic, int iseg )
{
	cconnect * c = &net->connect[ic];
	SetViaVisible( net, ic, iseg, TRUE );

	m_dlist->StopDragging();
	SetAreaConnections( net );
}

// Start dragging copper area corner to move it
//
int CNetList::StartDraggingAreaCorner( CDC *pDC, cnet * net, int iarea, int icorner, int x, int y, int crosshair )
{
	net->area[iarea].poly->StartDraggingToMoveCorner( pDC, icorner, x, y, crosshair );
	return 0;
}

// Start dragging inserted copper area corner
//
int CNetList::StartDraggingInsertedAreaCorner( CDC *pDC, cnet * net, int iarea, int icorner, int x, int y, int crosshair )
{
	net->area[iarea].poly->StartDraggingToInsertCorner( pDC, icorner, x, y, crosshair );
	return 0;
}

// Cancel dragging inserted area corner
//
int CNetList::CancelDraggingInsertedAreaCorner( cnet * net, int iarea, int icorner )
{
	net->area[iarea].poly->CancelDraggingToInsertCorner( icorner );
	return 0;
}

// Cancel dragging area corner
//
int CNetList::CancelDraggingAreaCorner( cnet * net, int iarea, int icorner )
{
	net->area[iarea].poly->CancelDraggingToMoveCorner( icorner );
	return 0;
}

// Cancel dragging segment
//
int CNetList::CancelDraggingSegment( cnet * net, int ic, int iseg )
{
	// make segment visible
	cconnect * c =&net->connect[ic];
	m_dlist->Set_visible(c->seg[iseg].dl_el, 1);
	m_dlist->StopDragging();
	return 0;
}

// Cancel dragging vertex
//
int CNetList::CancelDraggingVertex( cnet * net, int ic, int ivtx )
{
	// make segments and via visible
	cconnect * c =&net->connect[ic];
	cvertex * v = &c->vtx[ivtx];
	m_dlist->Set_visible(c->seg[ivtx-1].dl_el, 1);
	m_dlist->Set_visible(c->seg[ivtx].dl_el, 1);
	SetViaVisible( net, ic, ivtx, TRUE );

	// if tee, make connecting stubs visible
	if( v->tee_ID )
	{
		for( int icc=0; icc<net->nconnects; icc++ )
		{
			cconnect * cc = &net->connect[icc];
			if( cc != c && cc->end_pin == cconnect::NO_END )
			{
				cvertex * vv = &cc->vtx[cc->nsegs];
				if( vv->tee_ID == v->tee_ID )
				{
					m_dlist->Set_visible( cc->seg[cc->nsegs-1].dl_el, 1 );
				}
			}
		}
	}

	m_dlist->StopDragging();
	return 0;
}

// Cancel moving segment:
//
int CNetList::CancelMovingSegment( cnet * net, int ic, int ivtx )
{
	// cancel previous selection and make segments and via invisible
	cconnect * c =&net->connect[ic];
	cvertex * v = &c->vtx[ivtx];

	// make segments and via visible
	m_dlist->Set_visible(c->seg[ivtx-1].dl_el, 1);
	m_dlist->Set_visible(c->seg[ivtx].dl_el, 1);

	if( m_dlist->Dragging_third_segment() )
		m_dlist->Set_visible(c->seg[ivtx+1].dl_el, 1);

	SetViaVisible( net, ic, ivtx,   TRUE );
	SetViaVisible( net, ic, ivtx+1, TRUE );

	m_dlist->StopDragging();
	return 0;
}


// Cancel dragging vertex inserted into segment
//
int CNetList::CancelDraggingSegmentNewVertex( cnet * net, int ic, int iseg )
{
	// make segment visible
	cconnect * c =&net->connect[ic];
	m_dlist->Set_visible(c->seg[iseg].dl_el, 1);
	m_dlist->StopDragging();
	return 0;
}

// returns: VIA_NO_CONNECT if no via
//			VIA_TRACE if via connects to a trace segment on this layer
//			VIA_AREA if via connects to copper area
//
int CNetList::GetViaConnectionStatus( cnet * net, int ic, int iv, int layer )
{
	if( iv == 0 )
		ASSERT(0);

	int status = VIA_NO_CONNECT;
	cconnect * c = &net->connect[ic];
	cvertex * v = &c->vtx[iv];

	// check for end vertices of traces to pads
	if( iv == 0 )
		return status;
	if( c->end_pin != cconnect::NO_END  && iv == (c->nsegs + 1) )
		return status;

	// check for normal via pad
	if( !v->viaExists() && v->tee_ID == 0 )
		return status;

	// check for via pad at end of branch
	if( v->tee_ID != 0 && iv == c->nsegs && c->seg[iv-1].layer == layer )
		if( !TeeViaNeeded( net, v->tee_ID ) )
			return status;

	// check for trace connection to via pad
	c = &net->connect[ic];
	if( c->seg[iv-1].layer == layer )
		status |= VIA_TRACE;
	if( iv < c->nsegs )
		if( c->seg[iv].layer == layer )
			status |= VIA_TRACE;

	// see if it connects to any area in this net on this layer
	for( int ia=0; ia<net->nareas; ia++ )
	{
		// next area
		carea * a = &net->area[ia];
		if( a->poly->GetLayer() == layer )
		{
			// area is on this layer, loop through via connections to area
			for( int ivia=0; ivia<a->nvias; ivia++ )
			{
				if( a->vcon[ivia] == ic && a->vtx[ivia] == iv )
				{
					// via connects to area
					status |= VIA_AREA;
				}
			}
		}
	}
	return status;
}

// get via parameters for vertex
// note: if the vertex is the end-vertex of a branch, the via parameters
// will be taken from the tee-vertex that the branch connects to
//
void CNetList::GetViaPadInfo( cnet * net, int ic, int iv, int layer,
		int * pad_w, int * pad_hole_w, int * connect_status )
{
	int con_status = GetViaConnectionStatus( net, ic, iv, layer );
	cconnect * c = &net->connect[ic];
	cvertex * v = &c->vtx[iv];
	int w = v->via_w();
	int hole_w = v->via_hole_w();
	if( layer > LAY_BOTTOM_COPPER )
	{
		// inner layer
		if( con_status == VIA_NO_CONNECT )
		{
			w = 0;	// no connection, no pad
		}
		else if( v->tee_ID && iv == c->nsegs )
		{
			// end-vertex of branch, find tee-vertex that it connects to
			int tee_ic, tee_iv;
			if( FindTeeVertexInNet( net, v->tee_ID, &tee_ic, &tee_iv ) )
			{
				w = net->connect[tee_ic].vtx[tee_iv].via_w();
				hole_w = net->connect[tee_ic].vtx[tee_iv].via_hole_w();
			}
			else
				ASSERT(0);
		}
		else
			w = hole_w + 2*m_annular_ring;	// pad = annular ring
	}
	if( pad_w )
		*pad_w = w;
	if( pad_hole_w )
		*pad_hole_w = hole_w;
	if( connect_status )
		*connect_status = con_status;
}

// Test for a hit on a vertex in a routed or partially-routed trace
// If layer == 0, ignore layer
//
BOOL CNetList::TestForHitOnVertex( cnet * net, int layer, int x, int y,
		cnet ** hit_net, int * hit_ic, int * hit_iv )
{
	// loop through all connections
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		for( int iv=1; iv<c->nsegs; iv++ )
		{
			cvertex * v = &c->vtx[iv];
			cseg * pre_s = &c->seg[iv-1];
			cseg * post_s = &c->seg[iv];
			if( v->viaExists() || layer == 0 || layer == pre_s->layer || layer == post_s->layer
				|| (pre_s->layer == LAY_RAT_LINE && post_s->layer == LAY_RAT_LINE) )
			{
				int test_w = max( v->via_w(), pre_s->width() );
				test_w = max( test_w, post_s->width() );
				test_w = max( test_w, 10*NM_PER_MIL );		// in case widths are zero
				double dx = x - v->x;
				double dy = y - v->y;
				double d = sqrt( dx*dx + dy*dy );
				if( d < test_w/2 )
				{
					*hit_net = net;
					*hit_ic = ic;
					*hit_iv = iv;
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

// add empty copper area to net
// return index to area (zero-based)
//
int CNetList::AddArea( cnet * net, int layer, int x, int y, int hatch )
{
	net->area.SetSize( net->nareas+1 );
	net->area[net->nareas].Initialize( m_dlist );
	id area_id( ID_NET, ID_AREA, net->nareas );
	net->area[net->nareas].poly->Start( layer, 1, 10*NM_PER_MIL, x, y, hatch, &area_id, net );
	net->nareas++;
	return net->nareas-1;
}

// add empty copper area to net, inserting at net->area[iarea]
//
void CNetList::InsertArea( cnet * net, int iarea, int layer, int x, int y, int hatch )
{
	// make new area and insert it into area array
	carea test;
//	test.Initialize( m_dlist );
	net->area.InsertAt( iarea, test ) ;
	net->area[iarea].Initialize( m_dlist );
	id area_id( ID_NET, ID_AREA, iarea );
	net->area[iarea].poly->Start( layer, 1, 10*NM_PER_MIL, x, y, hatch, &area_id, net );
	net->nareas++;
}

// add corner to copper area, apply style to preceding side
//
int CNetList::AppendAreaCorner( cnet * net, int iarea, int x, int y, int style, BOOL bDraw )
{
	net->area[iarea].poly->AppendCorner( x, y, style, bDraw );
	return 0;
}

// insert corner into copper area, apply style to preceding side
//
int CNetList::InsertAreaCorner( cnet * net, int iarea, int icorner,
							int x, int y, int style )
{
	if( icorner == net->area[iarea].poly->GetNumCorners() && !net->area[iarea].poly->GetClosed() )
	{
		net->area[iarea].poly->AppendCorner( x, y, style );
		ASSERT(0);	// this is now an error, should be using AppendAreaCorner
	}
	else
	{
		net->area[iarea].poly->InsertCorner( icorner, x, y );
		net->area[iarea].poly->SetSideStyle( icorner-1, style );
	}
	return 0;
}

// move copper area corner
//
void CNetList::MoveAreaCorner( cnet * net, int iarea, int icorner, int x, int y )
{
	net->area[iarea].poly->MoveCorner( icorner, x, y );
}

// highlight
//
void CNetList::HighlightAreaCorner( cnet * net, int iarea, int icorner )
{
	net->area[iarea].poly->HighlightCorner( icorner );
}

// get copper area corner coords
//
CPoint CNetList::GetAreaCorner( cnet * net, int iarea, int icorner )
{
	CPoint pt;
	pt.x = net->area[iarea].poly->GetX( icorner );
	pt.y = net->area[iarea].poly->GetY( icorner );
	return pt;
}

// complete copper area contour by adding line to first corner
//
int CNetList::CompleteArea( cnet * net, int iarea, int style )
{
	if( net->area[iarea].poly->GetNumCorners() > 2 )
	{
		net->area[iarea].poly->Close( style );
		SetAreaConnections( net, iarea );
	}
	else
	{
		RemoveArea( net, iarea );
	}
	return 0;
}

// set connections for all areas
//
void CNetList::SetAreaConnections()
{
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		SetAreaConnections( net );
	}
}

// set connections for all areas
//
void CNetList::SetAreaConnections( cnet * net)
{
	if( net )
	{
		for( int ia=0; ia<net->nareas; ia++ )
			SetAreaConnections( net, ia );
	}
}

// set area connections for all nets on a part
// should be used when a part is added or moved
//
void CNetList::SetAreaConnections( cpart * part )
{
	// find nets which connect to this part and adjust areas
	for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
	{
		cnet * net = (cnet*)part->pin[ip].net;
		if( net )
		{
			int set_area_flag = 1;
			// see if this net already encountered
			for( int ipp=0; ipp<ip; ipp++ )
				if( part->pin[ipp].net == net )
					set_area_flag = 0;
			// set area connections for net
			if( set_area_flag )
				SetAreaConnections( net );
		}
	}
}

// set arrays of pins and stub traces connected to area
// does not modify connect[] array
//
void CNetList::SetAreaConnections( cnet * net, int iarea )
{
	carea * area = &net->area[iarea];

	// zero out previous arrays
	area->npins = 0;
	area->nvias = 0;
	area->pin.SetSize(0);
	area->vcon.SetSize(0);
	area->vtx.SetSize(0);

	// test all pins in net for being inside copper area
	id id( ID_NET, ID_AREA, iarea, ID_PIN_X );
	int area_layer = area->poly->GetLayer();	// layer of copper area
	for( int ip=0; ip<net->npins; ip++ )
	{
		cpart * part = net->pin[ip].part;
		if( part )
		{
			if( part->shape )
			{
				CString part_pin_name = net->pin[ip].pin_name;
				int pin_index = part->shape->GetPinIndexByName( part_pin_name );
				if( pin_index != -1 )
				{
					// see if pin allowed to connect to area
					int pin_layer = m_plist->GetPinLayer( part, part_pin_name );
					if( pin_layer != LAY_PAD_THRU )
					{
						// SMT pad
						if( pin_layer != area_layer )
							continue;	// not on area layer
					}
					// see if pad allowed to connect
					padstack * ps = &part->shape->m_padstack[pin_index];
					pad * ppad = &ps->inner;

					if( part->side == 0 && area_layer == LAY_TOP_COPPER
						|| part->side == 1 && area_layer == LAY_BOTTOM_COPPER )
						ppad = &ps->top;
					else if( part->side == 1 && area_layer == LAY_TOP_COPPER
						|| part->side == 0 && area_layer == LAY_BOTTOM_COPPER )
						ppad = &ps->bottom;

					if( ppad->connect_flag == PAD_CONNECT_NEVER )
						continue;	// pad never allowed to connect
					if( ppad->connect_flag == PAD_CONNECT_DEFAULT && !ps->hole_size && !m_bSMT_connect )
						continue;	// pad uses project option not to connect SMT pads
					if( pin_layer != LAY_PAD_THRU && ppad->shape == PAD_NONE )
						continue;	// no SMT pad defined (this should not happen)

					// see if pad is inside copper area
					CPoint p = m_plist->GetPinPoint( part, part_pin_name );
					if( area->poly->TestPointInside( p.x, p.y ) )
					{
						// pin is inside copper area
						cnet * part_pin_net = part->pin[pin_index].net;
						ASSERT(part_pin_net == net);	// inconsistency between part->pin->net and net->pin->part

						area->pin.SetSize( area->npins+1 );
						area->pin[area->npins] = ip;
						id.ii = ip;

						area->npins++;
					}
				}
			}
		}
	}
	// test all vias in traces for being inside copper area,
	// also test all end-vertices of non-branch stubs for being on same layer
	id.sst = ID_STUB_X;
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		int nsegs = c->nsegs;
		int nvtx = nsegs;
		if( c->end_pin == cconnect::NO_END )
			nvtx++;
		for( int iv=1; iv<nvtx; iv++ )
		{
			cvertex * v = &c->vtx[iv];
			if( v->viaExists() || c->seg[nsegs-1].layer == area->poly->GetLayer() )
			{
				// via or on same layer as copper area
				int x = v->x;
				int y = v->y;
				if( area->poly->TestPointInside( x, y ) )
				{
					// end point of trace is inside copper area
					area->vcon.SetSize( area->nvias+1 );
					area->vcon[area->nvias] = ic;
					area->vtx.SetSize( area->nvias+1 );
					area->vtx[area->nvias] = iv;
					id.ii = ic;

					area->nvias++;
				}
			}
		}
	}
}

// see if a point on a layer is inside a copper area in a net
// if layer == LAY_PAD_THRU, matches any layer
// if so, return iarea
//
BOOL CNetList::TestPointInArea( cnet * net, int x, int y, int layer, int * iarea )
{
	for( int ia=0; ia<net->nareas; ia++ )
	{
		carea * a = &net->area[ia];
		if( (a->poly->GetLayer() == layer || layer == LAY_PAD_THRU)&& a->poly->TestPointInside( x, y ) )
		{
			if( iarea )
				*iarea = ia;
			return TRUE;
		}
	}
	return FALSE;
}

// remove copper area from net
//
int CNetList::RemoveArea( cnet * net, int iarea )
{
	net->area.RemoveAt( iarea );
	net->nareas--;
	RenumberAreas( net );
	return 0;
}

// Get pointer to net with given name
//
cnet * CNetList::GetNetPtrByName( CString * name )
{
	// find element with name
	void * ptr;
	cnet * net;
	if( m_map.Lookup( *name, ptr ) )
	{
		net = (cnet*)ptr;
		return net;
	}
	return 0;
}

// Select copper area side
//
void CNetList::SelectAreaSide( cnet * net, int iarea, int iside )
{
	m_dlist->CancelHighLight();
	net->area[iarea].poly->HighlightSide( iside );
}

// Select copper area corner
//
void CNetList::SelectAreaCorner( cnet * net, int iarea, int icorner )
{
	m_dlist->CancelHighLight();
	net->area[iarea].poly->HighlightCorner( icorner );
}

// Set style for area side
//
void CNetList::SetAreaSideStyle( cnet * net, int iarea, int iside, int style )
{
	m_dlist->CancelHighLight();
	net->area[iarea].poly->SetSideStyle( iside, style );
	net->area[iarea].poly->HighlightSide( iside );
}


// Select all connections in net
//
void CNetList::HighlightNetConnections( cnet * net )
{
	for( int ic=0; ic<net->nconnects; ic++ )
		HighlightConnection( net, ic );
}

// Select connection
//
void CNetList::HighlightConnection( cnet * net, int ic )
{
	cconnect * c = &net->connect[ic];
	for( int is=0; is<c->seg.GetSize(); is++ )
		HighlightSegment( net, ic, is );
}

// Select segment
//
void CNetList::HighlightSegment( cnet * net, int ic, int iseg )
{
	cconnect * c =&net->connect[ic];
	m_dlist->HighLight( DL_LINE, m_dlist->Get_x(c->seg[iseg].dl_el),
								m_dlist->Get_y(c->seg[iseg].dl_el),
								m_dlist->Get_xf(c->seg[iseg].dl_el),
								m_dlist->Get_yf(c->seg[iseg].dl_el),
								m_dlist->Get_w(c->seg[iseg].dl_el),
								m_dlist->Get_layer(c->seg[iseg].dl_el) );

}

// Select vertex
//
void CNetList::HighlightVertex( cnet * net, int ic, int ivtx )
{
	// highlite square width is trace_width*2, via_width or 20 mils
	// whichever is greatest
	int w;
	cconnect * c =&net->connect[ic];
	if( ivtx > 0 && c->nsegs > ivtx )
		w = 2 * c->seg[ivtx-1].width(); // w = width of following segment
	else
		w = 0;
	if( c->nsegs > ivtx )
	{
		if ( (2*c->seg[ivtx].width()) > w )
			w = 2 * c->seg[ivtx].width();		// w = width of preceding segment
	}
	if( c->vtx[ivtx].via_w() > w )
		w = c->vtx[ivtx].via_w();

	if( w<(20*PCBU_PER_MIL) )
		w = 20*PCBU_PER_MIL;

	m_dlist->HighLight( DL_HOLLOW_RECT,
		c->vtx[ivtx].x - w/2,
		c->vtx[ivtx].y - w/2,
		c->vtx[ivtx].x + w/2,
		c->vtx[ivtx].y + w/2,
		0 );
}

// Highlight all sides of area
//
void CNetList::HighlightAreaSides( cnet * net, int ia )
{
	carea * a = &net->area[ia];
	int nsides = a->poly->GetNumSides();
	for( int is=0; is<nsides; is++ )
		a->poly->HighlightSide( is );
}

// Highlight entire net
//
void CNetList::HighlightNet( cnet * net)
{
	HighlightNetConnections( net );
	for( int ia=0;ia<net->nareas; ia++ )
		HighlightAreaSides( net, ia );
}

// force a via on a vertex
//
int CNetList::ForceVia( cnet * net, int ic, int ivtx, BOOL set_areas )
{
	cconnect * c = &net->connect[ic];
	c->vtx[ivtx].force_via_flag = 1;
	ReconcileVia( net, ic, ivtx );
	if( set_areas )
		SetAreaConnections( net );
	return 0;
}

// remove forced via on a vertex
// doesn't modify adjacent segments
//
int CNetList::UnforceVia( cnet * net, int ic, int ivtx, BOOL set_areas )
{
	cconnect * c = &net->connect[ic];
	c->vtx[ivtx].force_via_flag = 0;
	ReconcileVia( net, ic, ivtx);
	if( set_areas )
		SetAreaConnections( net );
	return 0;
}

// Reconcile via with preceding and following segments
// if a via needs to be created, use the attrib passed in 'new_via_attrib'
//
// Returns: Boolean: "is a via needed for this vertex?"
int CNetList::ReconcileVia( cnet * net, int ic, int ivtx, CViaWidthInfo const &new_via_attrib )
{
	cconnect * c = &net->connect[ic];
	cvertex * v = &c->vtx[ivtx];
	BOOL via_needed = FALSE;
	// see if via needed
	if( v->force_via_flag )
	{
		via_needed = 1;
	}
	else
	{
		if( c->end_pin == cconnect::NO_END && ivtx == c->nsegs )
		{
			// end vertex of a stub trace
			if( v->tee_ID )
			{
				// this is a branch, reconcile the main tee
				int tee_ic;
				int tee_iv;
				BOOL bFound = FindTeeVertexInNet( net, v->tee_ID, &tee_ic, &tee_iv );
				if( bFound )
				{
					via_needed = ReconcileVia( net, tee_ic, tee_iv, new_via_attrib );
				}
			}
		}
		else if( ivtx == 0 || ivtx == c->nsegs )
		{
			// first and last vertex are part pads
			return 0;
		}
		else if( v->tee_ID )
		{
			via_needed = TeeViaNeeded( net, v->tee_ID );
		}
		else
		{
			c->vtx[ivtx].pad_layer = 0;
			cseg * s1 = &c->seg[ivtx-1];
			cseg * s2 = &c->seg[ivtx];
			if( s1->layer != s2->layer && s1->layer != LAY_RAT_LINE && s2->layer != LAY_RAT_LINE )
			{
				via_needed = TRUE;
			}
		}
	}

	if( via_needed )
	{
		// via needed, make sure it exists or create it
		if( !v->viaExists() )
		{
			// This via (on this connection) doesn't exist.  Check if any via exists
			// on any connection.  If so, insert the via using the other via's attributes,
			// otherwise, create a new via using the net's default attributes.
			CVertexIterator vi( net, ic, ivtx );
			cvertex * vfound = NULL;

			for( cvertex * v = vi.GetFirst(); v != NULL; v = vi.GetNext() )
			{
				if( v->viaExists() )
				{
					vfound = v;
					break;
				}
			}

			// Start with the passed-in via attrib.
			CViaWidthInfo via_attrib(new_via_attrib);

			// If another via was found, use its attrib instead
			if( vfound != NULL )
			{
				via_attrib = vfound->via_width_attrib;
			}

			if( via_attrib.m_via_width.m_val == 0 )
			{
				// Via attrib. in have bad width value.  Reset to 'use_parent"
				via_attrib.m_via_width = CII_FreePcb::E_USE_PARENT;
				via_attrib.m_via_hole  = CII_FreePcb::E_USE_PARENT;
			}

			// Insert the via
			InsertVia( net, ic, ivtx, via_attrib );

			// Make sure all shared vias have the same attributes
			SetViaSizeAttrib( net, ic, ivtx, via_attrib );
		}
	}
	else
	{
		// via not needed
		v->SetNoVia();
	}

	if( m_dlist )
		DrawVia( net, ic, ivtx );

	return via_needed;
}


void CNetList::MakeTeeConnection( cnet *net, cvertex *from_vtx, int to_ic, int to_ivtx)
{
	int id = from_vtx->tee_ID;
	if( id == 0 )
	{
		// No tee-ID assigned yet, get a new one now
		id = GetNewTeeID();
		from_vtx->tee_ID = id;
	}

	net->connect[to_ic].vtx[to_ivtx].tee_ID = id;

	// Reconcile the new tee via
	ReconcileVia( net, to_ic, to_ivtx );
}


int CNetList::ViaExists( cnet * net, int ic, int ivtx )
{
	CVertexIterator vi( net, ic, ivtx );
	cvertex * v;

	for( v = vi.GetFirst(); v != NULL; v = vi.GetNext() )
	{
		if( v->viaExists() )
		{
			return 1;
		}
	}

	return 0;
}


// write nets to file
//
int CNetList::WriteNets( CStdioFile * file )
{
	CString line;
	cvertex * v;
	cseg * s;
	cnet * net;

	try
	{
		line.Format( "[nets]\n\n" );
		file->WriteString( line );

		// traverse map
		POSITION pos;
		CString name;
		void * ptr;
		for( pos = m_map.GetStartPosition(); pos != NULL; )
		{
			m_map.GetNextAssoc( pos, name, ptr );
			net = (cnet*)ptr;


			line.Format( "net: \"%s\" %d %d %d %d %d %d %d %d %d\n", net->name,
							net->npins,
							net->nconnects,
							net->nareas,
							CSegWidthInfo::ItemToFile( net->def_width_attrib.m_seg_width ),
							CSegWidthInfo::ItemToFile( net->def_width_attrib.m_via_width ),
							CSegWidthInfo::ItemToFile( net->def_width_attrib.m_via_hole  ),
							net->visible,
							net->def_width_attrib.m_ca_clearance.m_val,
							net->def_width_attrib.m_ca_clearance.m_status
			);

			file->WriteString( line );

			for( int ip=0; ip<net->npins; ip++ )
			{
				cpin *pin = &net->pin[ip];

				line.Format( "  pin: %d %s.%s\n", ip+1,
							pin->ref_des(),
							pin->pin_name
							);

				file->WriteString( line );
			}
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				line.Format( "  connect: %d %d %d %d %d\n", ic+1,
					c->start_pin,
					c->end_pin, c->nsegs, c->locked
				);
				file->WriteString( line );
				int nsegs = c->nsegs;
				for( int is=0; is<=nsegs; is++ )
				{
					v = &(c->vtx[is]);

					line.Format( "    vtx: %d %d %d %d %d %d %d %d %d %d %d\n",
						is+1,
						v->x, v->y,
						v->pad_layer,
						v->force_via_flag,
						v->via_width_attrib.m_via_width.m_val,
						v->via_width_attrib.m_via_hole.m_val,
						v->tee_ID,
						v->via_width_attrib.m_via_width.m_status,
						v->via_width_attrib.m_ca_clearance.m_val,
						v->via_width_attrib.m_ca_clearance.m_status
					);
					file->WriteString( line );

					if( is<nsegs )
					{

						s = &(c->seg[is]);
						line.Format( "    seg: %d %d %d 0 0 %d %d %d\n",
							is+1,
							s->layer,
							(s->layer == LAY_RAT_LINE) ? 0 : s->width_attrib.m_seg_width.m_val,
							s->width_attrib.m_seg_width.m_status,
							s->width_attrib.m_ca_clearance.m_val,
							s->width_attrib.m_ca_clearance.m_status
						);
						file->WriteString( line );
					}
				}
			}
			for( int ia=0; ia<net->nareas; ia++ )
			{
				line.Format( "  area: %d %d %d %d\n", ia+1,
					net->area[ia].poly->GetNumCorners(),
					net->area[ia].poly->GetLayer(),
					net->area[ia].poly->GetHatch()
					);
				file->WriteString( line );
				for( int icor=0; icor<net->area[ia].poly->GetNumCorners(); icor++ )
				{
					line.Format( "    corner: %d %d %d %d %d\n", icor+1,
						net->area[ia].poly->GetX( icor ),
						net->area[ia].poly->GetY( icor ),
						net->area[ia].poly->GetSideStyle( icor ),
						net->area[ia].poly->GetEndContour( icor )
						);
					file->WriteString( line );
				}
			}
			file->WriteString( "\n" );
		}
	}
	catch( CFileException * e )
	{
		CString str;
		if( e->m_lOsError == -1 )
			str.Format( "File error: %d\n", e->m_cause );
		else
			str.Format( "File error: %d %ld (%s)\n", e->m_cause, e->m_lOsError,
			_sys_errlist[e->m_lOsError] );
		return 1;
	}
	return 0;
}

// read netlist from file
// throws err_str on error
//
void CNetList::ReadNets( CStdioFile * pcb_file, double read_version, int * layer )
{
	int err, pos, np;
	CArray<CString> p;
	CString in_str, key_str;
	int * in_layer = layer;
	if( in_layer == NULL )
		in_layer = m_layer_by_file_layer;	// this is a translation table

	// find beginning of [nets] section
	do
	{
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			// error reading pcb file
			CString mess;
			mess.Format( "Unable to find [nets] section in file" );
			AfxMessageBox( mess );
			return;
		}
		in_str.Trim();
	}
	while( in_str != "[nets]" );

	// get each net in [nets] section
	ClearTeeIDs();
	while( 1 )
	{
		pos = (long)pcb_file->GetPosition();
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			CString * err_str = new CString( "unexpected EOF in project file" );
			throw err_str;
		}
		in_str.Trim();
		if( in_str[0] == '[' && in_str != "[nets]" )
		{
			pcb_file->Seek( pos, CFile::begin );
			return;		// start of next section, reset position and exit
		}
		else if( in_str.Left(4) == "net:" )
		{
			np = ParseKeyString( &in_str, &key_str, &p );
			CString net_name = p[0].Left(MAX_NET_NAME_SIZE);
			net_name.Trim();
			int npins = my_atoi( &p[1] );
			int nconnects = my_atoi( &p[2] );
			int nareas = my_atoi( &p[3] );

			CNetWidthInfo def_width_attrib;
			int sz;

			sz = my_atoi( &p[4] );
			CSegWidthInfo::FileToItem( (sz < 0) ? 0 : sz, def_width_attrib.m_seg_width );

			sz = my_atoi( &p[5] );
			CSegWidthInfo::FileToItem( (sz < 0) ? 0 : sz, def_width_attrib.m_via_width );

			sz = my_atoi( &p[6] );
			CSegWidthInfo::FileToItem( (sz < 0) ? 0 : sz, def_width_attrib.m_via_hole  );

			int visible = 1;
			if( np > 8 )
			{
				visible = my_atoi( &p[7] );
			}

			if( np > 9 )
			{
				def_width_attrib.m_ca_clearance = my_atoi( &p[8] );
				if( np > 10 )
				{
					// VAL & STATUS format
					def_width_attrib.m_ca_clearance.m_status = my_atoi( &p[9] );
				}
				else
				{
					// VAL_STATUS format (only present in intermediate development versions)
				}
			}
			else
			{
				def_width_attrib.m_ca_clearance = CInheritableInfo::E_USE_PARENT;
			}

			cnet * net = AddNet( net_name, npins, def_width_attrib );

			// Set visibility flag AFTER adding net
			net->visible = visible;

			for( int ip=0; ip<npins; ip++ )
			{
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString( "unexpected EOF in project file" );
					throw err_str;
				}
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "pin" || np < 3 )
				{
					CString * err_str = new CString( "error parsing [nets] section of project file" );
					throw err_str;
				}

				CString pin_str = p[1].Left(CShape::MAX_PIN_NAME_SIZE);
				int dot_pos = pin_str.FindOneOf( "." );
				CString ref_str = pin_str.Left( dot_pos );
				CString pin_num_str = pin_str.Right( pin_str.GetLength()-dot_pos-1 );

				AddNetPin( net, ref_str, pin_num_str );
			}
			for( int ic=0; ic<nconnects; ic++ )
			{
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString( "unexpected EOF in project file" );
					throw err_str;
				}
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "connect" || np < 6 )
				{
					CString * err_str = new CString( "error parsing [nets] section of project file" );
					throw err_str;
				}
//				int nc = my_atoi( &p[0] );
//				if( (nc-1) != ic )
//				{
//					CString * err_str = new CString( "error parsing [nets] section of project file" );
//					throw err_str;
//				}
				int start_pin = my_atoi( &p[1] );
				int end_pin = my_atoi( &p[2] );
				int nsegs = my_atoi( &p[3] );
				int locked = my_atoi( &p[4] );

				int nc;
				if( end_pin != cconnect::NO_END )
					nc = AddNetConnect( net, start_pin, end_pin );
				else
					nc = AddNetStub( net, start_pin );

				if( nc == -1 )
				{
					// invalid connection, remove it with this ugly code
					ic--;
					nconnects--;
					net->connect.SetSize( ic+1 );
					for( int i=0; i<nsegs*2+1; i++ )
					{
						err = pcb_file->ReadString( in_str );
						if( !err )
						{
							CString * err_str = new CString( "unexpected EOF in project file" );
							throw err_str;
						}
					}
				}
				else
				{
					net->connect[ic].locked = locked;

					// skip first vertex
					err = pcb_file->ReadString( in_str );
					if( !err )
					{
						CString * err_str = new CString( "unexpected EOF in project file" );
						throw err_str;
					}
					// now add all segments
					int test_not_done = 1;
					CViaWidthInfo pre_width;
					int is;
					for( is=0; is<nsegs; is++ )
					{
						// read segment data
						err = pcb_file->ReadString( in_str );
						if( !err )
						{
							CString * err_str = new CString( "unexpected EOF in project file" );
							throw err_str;
						}
						np = ParseKeyString( &in_str, &key_str, &p );
						if( key_str != "seg" || np < 6 )
						{
							CString * err_str = new CString( "error parsing [nets] section of project file" );
							throw err_str;
						}
						int file_layer = my_atoi( &p[1] );
						int layer = in_layer[file_layer];

						CSegWidthInfo width;
						width.m_seg_width = my_atoi( &p[2] );
						if (np > 6)
                        {
							// Segment clearance given
							if( np > 8 )
							{
								// VAL & STATUS format for clearance
								// p5 = seg width status
								// p6 = clearance val
								// p7 = clearance status
								width.m_seg_width.m_status = my_atoi( &p[5] );

								width.m_ca_clearance          = my_atoi( &p[6] );
								width.m_ca_clearance.m_status = my_atoi( &p[7] );
							}
							else
							{
								// VAL_STATUS format for clearance (only present in intermediate development versions)
								// p5 = clearance
								// p6 = seg width status (if present)
								width.m_ca_clearance = my_atoi( &p[5] );

								if( np > 7 )
								{
									// Segment width status given
									width.m_seg_width.m_status = my_atoi( &p[6] );
								}
							}
                        }

						// Convert any zero-width segs to "use parent"
						if( ( width.m_seg_width.m_val == 0 ) && ( layer != LAY_RAT_LINE ) )
						{
							width.m_seg_width = CInheritableInfo::E_USE_PARENT;
						}

						// read following vertex data
						err = pcb_file->ReadString( in_str );
						if( !err )
						{
							CString * err_str = new CString( "unexpected EOF in project file" );
							throw err_str;
						}
						np = ParseKeyString( &in_str, &key_str, &p );
						if( key_str != "vtx" || np < 8 )
						{
							CString * err_str = new CString( "error parsing [nets] section of project file" );
							throw err_str;
						}
						if( test_not_done )
						{
							// only add segments if we are not done
							int x = my_atoi( &p[1] );
							int y = my_atoi( &p[2] );
							int file_layer = my_atoi( &p[3] );
							int pad_layer = in_layer[file_layer];
							int force_via_flag = my_atoi( &p[4] );

							CViaWidthInfo via_width_attrib( width );

							via_width_attrib.m_via_width = my_atoi( &p[5] );
							via_width_attrib.m_via_hole  = my_atoi( &p[6] );

							int tee_ID = 0;
							if( np > 8 )
							{
								tee_ID = my_atoi( &p[7] );
								if( tee_ID )
									AddTeeID( tee_ID );
							}

							if( np > 9 )
							{
								// Clearance info provided
								if( np > 11 )
								{
									// VAL & STATUS format for clearance
									// p8  = via width status
									// p9  = clearance val
									// p10 = clearance status

									// Clearance
									via_width_attrib.m_ca_clearance          = my_atoi( &p[9] );
									via_width_attrib.m_ca_clearance.m_status = my_atoi( &p[10] );

									// Via width status info
									int via_status = my_atoi( &p[8] );

									via_width_attrib.m_via_width.m_status = via_status;
									via_width_attrib.m_via_hole .m_status = via_status;
								}
								else
								{
									// VAL_STATUS format for clearance (only present in intermediate development versions)
									// p8 = clearance
									// p9 = via status (if present)

									// Clearance
									via_width_attrib.m_ca_clearance = my_atoi( &p[8] );

									if( np > 10 )
									{
										// Via width status info provided
										int via_status = my_atoi( &p[9] );

										via_width_attrib.m_via_width.m_status = via_status;
										via_width_attrib.m_via_hole .m_status = via_status;
									}
								}
							}
							else
							{
								via_width_attrib.m_ca_clearance = CII_FreePcb::E_USE_PARENT;
							}

							if( end_pin != cconnect::NO_END )
							{
								CPoint end_pt;
								if( is == (nsegs-1) )
								{
									// last segment of pin-pin connection
									// force segment to end on pin
									cpart * end_part = net->pin[end_pin].part;
									end_pt = m_plist->GetPinPoint( end_part, net->pin[end_pin].pin_name );
									x = end_pt.x;
									y = end_pt.y;
								}
								test_not_done = InsertSegment( net, ic, is, x, y, layer, width, 0 );
							}
							else
							{
								AppendSegment( net, ic, x, y, layer, width );

								// set widths of following vertex
								// Always insert via - reconcile later.  Otherwise size info
								// can get lost if the via isn't initially created since a branch
								// or stub may not exist.
								InsertVia( net, ic, is+1, via_width_attrib );
							}
							//** this code is for bug in versions before 1.313
							if( force_via_flag )
							{
								if( end_pin == cconnect::NO_END && is == nsegs-1 )
									ForceVia( net, ic, is+1 );
								else if( read_version > 1.312001 )	// i.e. 1.313 or greater
									ForceVia( net, ic, is+1 );
							}
							net->connect[ic].vtx[is+1].tee_ID = tee_ID;
							if( is != 0 )
							{
								// set widths of preceding vertex
								// Always insert via - reconcile later.  Otherwise size info
								// can get lost if the via isn't initially created since a branch
								// or stub may not exist.
								InsertVia( net, ic, is, pre_width );
							}
							pre_width = via_width_attrib;
						}
					}

					// set widths of last vertex if not a pin
					// Always insert via - reconcile later.  Otherwise size info
					// can get lost if the via isn't initially created since a branch
					// or stub may not exist.
					if( end_pin == cconnect::NO_END )
					{
						InsertVia( net, ic, is, pre_width );
					}
				}
			}
			for( int ia=0; ia<nareas; ia++ )
			{
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString( "unexpected EOF in project file" );
					throw err_str;
				}
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "area" || np < 4 )
				{
					CString * err_str = new CString( "error parsing [nets] section of project file" );
					throw err_str;
				}
				int na = my_atoi( &p[0] );
				if( (na-1) != ia )
				{
					CString * err_str = new CString( "error parsing [nets] section of project file" );
					throw err_str;
				}
				int ncorners = my_atoi( &p[1] );
				int file_layer = my_atoi( &p[2] );
				int layer = in_layer[file_layer];
				int hatch = 1;
				if( np == 5 )
					hatch = my_atoi( &p[3] );
				int last_side_style = CPolyLine::STRAIGHT;
				for( int icor=0; icor<ncorners; icor++ )
				{
					err = pcb_file->ReadString( in_str );
					if( !err )
					{
						CString * err_str = new CString( "unexpected EOF in project file" );
						throw err_str;
					}
					np = ParseKeyString( &in_str, &key_str, &p );
					if( key_str != "corner" || np < 4 )
					{
						CString * err_str = new CString( "error parsing [nets] section of project file" );
						throw err_str;
					}
					int ncor = my_atoi( &p[0] );
					if( (ncor-1) != icor )
					{
						CString * err_str = new CString( "error parsing [nets] section of project file" );
						throw err_str;
					}
					int x = my_atoi( &p[1] );
					int y = my_atoi( &p[2] );
					if( icor == 0 )
						AddArea( net, layer, x, y, hatch );
					else
						AppendAreaCorner( net, ia, x, y, last_side_style, FALSE );
					if( np >= 5 )
						last_side_style = my_atoi( &p[3] );
					else
						last_side_style = CPolyLine::STRAIGHT;
					int end_cont = 0;
					if( np >= 6 )
						end_cont = my_atoi( &p[4] );
					if( icor == (ncorners-1) || end_cont )
					{
						CompleteArea( net, ia, last_side_style );
					}
				}
			}
			for(  int ia=nareas-1; ia>=0; ia-- )
			{
				if( net->area[ia].poly->GetNumCorners() < 3 )
					RemoveArea( net, ia );
			}
			CleanUpConnections( net );
			if( RemoveOrphanBranches( net, 0 ) )
			{
				//** we will hit this if FpcROUTE fails, so disabled
//				ASSERT(0);
			}
			UpdateNetAttributes( net );
		}
	}
}

// undraw via
//
void CNetList::UndrawVia( cnet * net, int ic, int iv )
{
	cconnect * c = &net->connect[ic];
	cvertex * v = &c->vtx[iv];
	if( v->dl_el.GetSize() )
	{
		for( int i=0; i<v->dl_el.GetSize(); i++ )
		{
			v->dl_el[i]->Remove();
//			v->dl_el[i] = NULL;
		}
		v->dl_el.RemoveAll();
	}

	v->dl_sel->Remove();
	v->dl_hole->Remove();

	v->dl_sel = NULL;
	v->dl_hole = NULL;
}

// draw vertex
//	i.e. draw selection box, draw via if needed
//
int CNetList::DrawVia( cnet * net, int ic, int iv )
{
	cconnect * c = &net->connect[ic];
	cvertex * v = &c->vtx[iv];
	if( v->m_dlist == NULL )
	{
		ASSERT(0);
		v->m_dlist = m_dlist;
	}

	// undraw previous via and selection box
	UndrawVia( net, ic, iv );

	// draw via if (v->via_w) > 0
	id vid( ID_NET, ID_CONNECT, ic, ID_VERTEX, iv );
	if( v->viaExists() )
	{
		// draw via
		v->dl_el.SetSize( m_layers );
		for( int il=0; il<m_layers; il++ )
		{
			int layer = LAY_TOP_COPPER + il;
			v->dl_el[il] = m_dlist->Add( vid, net, layer, DL_CIRC, 1,
				v->via_w(), 0, v->via_clearance(),
				v->x, v->y, 0, 0, 0, 0 );
		}
		v->dl_hole = m_dlist->Add( vid, net, LAY_PAD_THRU, DL_HOLE, 1,
				v->via_hole_w(), 0, 0,
				v->x, v->y, 0, 0, 0, 0 );
	}

	// test for tee-connection at end of stub trace
	if( v->tee_ID && c->end_pin == cconnect::NO_END && iv == c->nsegs )
	{
		// yes, no selector box
		v->dl_sel = NULL;
	}
	else
	{
		// draw selection box for vertex, using LAY_THRU_PAD if via or layer of adjacent
		// segments if no via
		CRect sel_rect;
		int sel_layer;
		if( v->viaExists() )
		{
			sel_layer = LAY_SELECTION;

			sel_rect.left   = v->x - v->via_w()/2;
			sel_rect.bottom = v->y - v->via_w()/2;
			sel_rect.right  = v->x + v->via_w()/2;
			sel_rect.top    = v->y + v->via_w()/2;
		}
		else
		{
			sel_layer = c->seg[iv-1].layer;

			sel_rect.left   = v->x - 10*PCBU_PER_MIL;
			sel_rect.bottom = v->y - 10*PCBU_PER_MIL;
			sel_rect.right  = v->x + 10*PCBU_PER_MIL;
			sel_rect.top    = v->y + 10*PCBU_PER_MIL;
		}

		vid.sst = ID_SEL_VERTEX;
		v->dl_sel = m_dlist->AddSelector( vid, net, sel_layer, DL_HOLLOW_RECT,
			1, 0, 0, sel_rect.left, sel_rect.bottom, sel_rect.right, sel_rect.top, 0, 0 );
	}
	return 0;
}

void CNetList::SetNetVisibility( cnet * net, BOOL visible )
{
	if( net->visible == visible )
	{
		return;
	}
	else if( visible )
	{
		// make segments visible and enable selection items
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			cconnect * c = &net->connect[ic];
			for( int is=0; is<c->nsegs; is++ )
			{
				c->seg[is].dl_el->visible = TRUE;
				c->seg[is].dl_sel->visible = TRUE;
			}
		}
	}
	else
	{
		// make ratlines invisible and disable selection items
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			cconnect * c = &net->connect[ic];
			for( int is=0; is<c->nsegs; is++ )
			{
				if( c->seg[is].layer == LAY_RAT_LINE )
				{
					c->seg[is].dl_el->visible = FALSE;
					c->seg[is].dl_sel->visible = FALSE;
				}
			}
		}
	}
	net->visible = visible;
}

BOOL CNetList::GetNetVisibility( cnet * net )
{
	return net->visible;
}

// export netlist data into a netlist_info structure so that it can
// be edited in a dialog
//
void CNetList::ExportNetListInfo( netlist_info * nl )
{
	// make copy of netlist data so that it can be edited
	POSITION pos;
	CString name;
	void * ptr;
	CString str;
	int i = 0;
	nl->SetSize( m_map.GetSize() );
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		(*nl)[i].name = net->name;
		(*nl)[i].net = net;
		(*nl)[i].visible = GetNetVisibility( net );
		(*nl)[i].width_attrib = net->def_width_attrib;
		(*nl)[i].width_attrib.Update();
		(*nl)[i].apply_trace_width = FALSE;
		(*nl)[i].apply_via_width = FALSE;
		(*nl)[i].apply_clearance = FALSE;
		(*nl)[i].modified = FALSE;
		(*nl)[i].deleted = FALSE;
		(*nl)[i].ref_des.SetSize(0);
		(*nl)[i].pin_name.SetSize(0);
		// now make copy of pin arrays
		(*nl)[i].ref_des.SetSize( net->npins );
		(*nl)[i].pin_name.SetSize( net->npins );
		for( int ip=0; ip<net->npins; ip++ )
		{
			(*nl)[i].ref_des[ip] = net->pin[ip].ref_des();
			(*nl)[i].pin_name[ip] = net->pin[ip].pin_name;
		}
		i++;
	}
}

// import netlist_info data back into netlist
//
void CNetList::ImportNetListInfo( netlist_info * nl, int flags, CDlgLog * log )
{
	CString mess;

	// loop through netlist_info and remove any nets that flagged for deletion
	int n_info_nets = nl->GetSize();
	for( int i=0; i<n_info_nets; i++ )
	{
		cnet * net = (*nl)[i].net;
		if( (*nl)[i].deleted && net )
		{
			// net was deleted, remove it
			if( log )
			{
				mess.Format( "  Removing net \"%s\"\r\n", net->name );
				log->AddLine( mess );
			}
			RemoveNet( net );
			(*nl)[i].net = NULL;
		}
	}

	// now handle any nets that were renamed
	// assumes that the new name is not a duplicate
	for( int i=0; i<n_info_nets; i++ )
	{
		cnet * net = (*nl)[i].net;
		if( net )
		{
			CString new_name = (*nl)[i].name;
			CString old_name = net->name;
			if( old_name != new_name )
			{
				m_map.RemoveKey( old_name );
				m_map.SetAt( new_name, net );
				net->name = new_name;	// rename net
			}
		}
	}

	// now check for existing nets that are not in netlist_info
	CArray<cnet*> delete_these;
	CIterator_cnet net_iter(this);
	for( cnet * net = net_iter.GetFirst(); net != NULL; net = net_iter.GetNext() )
	{
		// check if in netlist_info
		BOOL bFound = FALSE;
		for( int i=0; i<nl->GetSize(); i++ )
		{
			if( net->name == (*nl)[i].name )
			{
				bFound = TRUE;
				break;
			}
		}
		if( !bFound )
		{
			// net is not in netlist_info
			if( flags & KEEP_NETS )
			{
				if( log )
				{
					mess.Format( "  Keeping net \"%s\", not in imported netlist\r\n", net->name );
					log->AddLine( mess );
				}
			}
			else
			{
				if( log )
				{
					mess.Format( "  Removing net \"%s\"\r\n", net->name );
					log->AddLine( mess );
				}
				delete_these.Add( net );	// flag for deletion
			}
		}
	}
	// delete them
	for( int i=0; i<delete_these.GetSize(); i++ )
	{
		RemoveNet( delete_these[i] );
	}

	// now reloop, adding and modifying nets and deleting pins as needed
	for( int i=0; i<n_info_nets; i++ )
	{
		// ignore info nets marked for deletion
		if( (*nl)[i].deleted )
			continue;

		// try to find existing net with this name
		cnet * net = (*nl)[i].net;	// net from netlist_info (may be NULL)
		cnet * old_net = NULL;
		old_net = GetNetPtrByName( &(*nl)[i].name );
		if( net == NULL && old_net == NULL )
		{
			// no existing net, add to netlist
			if( !(*nl)[i].width_attrib.m_seg_width.isDefined() )
			{
				(*nl)[i].width_attrib.m_seg_width = CInheritableInfo::E_USE_PARENT;
			}
			if( !(*nl)[i].width_attrib.m_via_width.isDefined() )
			{
				(*nl)[i].width_attrib.m_via_width = CInheritableInfo::E_USE_PARENT;
			}
			if( !(*nl)[i].width_attrib.m_via_hole.isDefined() )
			{
				(*nl)[i].width_attrib.m_via_hole = CInheritableInfo::E_USE_PARENT;
			}

			net = AddNet( (*nl)[i].name, (*nl)[i].ref_des.GetSize(), (*nl)[i].width_attrib );

			(*nl)[i].net = net;
		}
		else if( net == NULL && old_net != NULL )
		{
			// no net from netlist_info but existing net with same name
			// use existing net and modify it
			(*nl)[i].modified = TRUE;
			net = old_net;
			(*nl)[i].net = net;

			net->def_width_attrib = (*nl)[i].width_attrib;
			net->def_width_attrib.SetParent( m_def_width_attrib );
			net->def_width_attrib.Update();

			UpdateNetAttributes( net );
		}
		else
		{
			// net from netlist_info and existing net have the same name
			if( net != old_net )
				ASSERT(0);	// make sure that they are actually the same net
			// modify existing net parameters, unless undefined
			net->def_width_attrib = (*nl)[i].width_attrib;
			net->def_width_attrib.SetParent( m_def_width_attrib );
			net->def_width_attrib.Update();

			UpdateNetAttributes( net );
		}

		// now set pin lists
		net->name = (*nl)[i].name;
		// now loop through net pins, deleting any which were removed
		for( int ipn=0; ipn<net->npins; )
		{
			CString ref_des = net->pin[ipn].ref_des();
			CString pin_name = net->pin[ipn].pin_name;
			BOOL pin_present = FALSE;
			for( int ip=0; ip<(*nl)[i].ref_des.GetSize(); ip++ )
			{
				if( ref_des == (*nl)[i].ref_des[ip]
				&& pin_name == (*nl)[i].pin_name[ip] )
				{
					// pin in net found in netlist_info
					pin_present = TRUE;
					break;
				}
			}
			if( !pin_present )
			{
				// pin in net but not in netlist_info
				if( flags & KEEP_PARTS_AND_CON )
				{
					// we may want to preserve this pin
					cpart * part = m_plist->GetPart( ref_des );
					if( !part )
						RemoveNetPin( net, ref_des, pin_name );
					else if( !part->bPreserve )
						RemoveNetPin( net, ref_des, pin_name );
					else
					{
						// preserve the pin
						ipn++;
					}
				}
				else
				{
					// delete it from net
					if( log )
					{
						mess.Format( "    Removing pin %s.%s from net \"%s\"\r\n",
							ref_des, pin_name, net->name  );
						log->AddLine( mess );
					}
					RemoveNetPin( net, ref_des, pin_name );
				}
			}
			else
			{
				ipn++;
			}
		}
	}

	// now reloop and add any pins that were added to netlist_info,
	// and delete any duplicates
	// separate loop to ensure that pins were deleted from all nets
	for( int i=0; i<n_info_nets; i++ )
	{
		cnet * net = (*nl)[i].net;
		if( net && !(*nl)[i].deleted && (*nl)[i].modified )
		{
			// loop through local pins, adding any new ones to net
			int n_local_pins = (*nl)[i].ref_des.GetSize();
			for( int ipl=0; ipl<n_local_pins; ipl++ )
			{
				// delete this pin from any other nets
				CIterator_cnet net_iter(this);
				for( cnet * test_net = net_iter.GetFirst(); test_net != NULL; test_net = net_iter.GetNext() )
				{
					if( test_net != net )
					{
						// test for duplicate pins
						for( int test_ip=test_net->npins-1; test_ip>=0; test_ip-- )
						{
							if( test_net->pin[test_ip].ref_des() == (*nl)[i].ref_des[ipl]
							&& test_net->pin[test_ip].pin_name == (*nl)[i].pin_name[ipl] )
							{
								if( log )
								{
									mess.Format( "    Removing pin %s.%s from net \"%s\"\r\n",
										test_net->pin[test_ip].ref_des(),
										test_net->pin[test_ip].pin_name,
										test_net->name  );
									log->AddLine( mess );
								}
								RemoveNetPin( test_net, test_ip );
							}
						}
					}
				}
				// now test for pin already present in net
				BOOL pin_present = FALSE;
				for( int ipp=0; ipp<net->npins; ipp++ )
				{
					if( net->pin[ipp].ref_des() == (*nl)[i].ref_des[ipl]
					&& net->pin[ipp].pin_name == (*nl)[i].pin_name[ipl] )
					{
						// pin in local array found in net
						pin_present = TRUE;
						break;
					}
				}
				if( !pin_present )
				{
					// pin not in net, add it
					AddNetPin( net, (*nl)[i].ref_des[ipl], (*nl)[i].pin_name[ipl] );
					if( log )
					{
						mess.Format( "    Adding pin %s.%s to net \"%s\"\r\n",
							(*nl)[i].ref_des[ipl],
							(*nl)[i].pin_name[ipl],
							net->name  );
						log->AddLine( mess );
					}
				}
			}
		}
	}

	// now set visibility and apply new widths, if requested
	for( int i=0; i<n_info_nets; i++ )
	{
		if( !(*nl)[i].modified )
			continue;

		cnet * net = (*nl)[i].net;
		if( net )
		{
			SetNetVisibility( net, (*nl)[i].visible );

			{
				CConnectionWidthInfo width;

				if( (*nl)[i].apply_trace_width )
				{
					width.m_via_width.Undef();
					width.m_via_hole.Undef();
					width.m_ca_clearance.Undef();

					width.SetParent( (*nl)[i].width_attrib );
					width.Update();
				}
				else
				{
					// Only modify net items marked as 'use parent'
					width.Undef();
				}
				SetNetWidth( net, width );
			}

			{
				CConnectionWidthInfo width;

				if( (*nl)[i].apply_via_width )
				{
					width.m_seg_width.Undef();
					width.m_ca_clearance.Undef();

					width.SetParent( (*nl)[i].width_attrib );
					width.Update();

					SetNetWidth( net, width );
				}
				else
				{
					// Only modify net items marked as 'use parent'
					width.Undef();
				}
				SetNetWidth( net, width );
			}

			{
				CClearanceInfo clearance;

				if( (*nl)[i].apply_clearance )
				{
					clearance.SetParent( (*nl)[i].width_attrib );
					clearance.Update();
				}
				else
				{
					// Only modify net items marked as 'use parent'
					clearance.Undef();
				}
				SetNetWidth( net, clearance );
			}
		}
	}
}

// Copy all data from another netlist (except display elements)
//
void CNetList::Copy( CNetList * src_nl )
{
	RemoveAllNets();

	CIterator_cnet net_iter(src_nl);
	for( cnet * src_net = net_iter.GetFirst(); src_net != NULL; src_net = net_iter.GetNext() )
	{
		cnet * net = AddNet( src_net->name, src_net->npins, src_net->def_width_attrib );
		net->pin.SetSize( src_net->npins );
		for( int ip=0; ip<src_net->npins; ip++ )
		{
			// add pin but don't modify part->pin->net
			cpin * src_pin = &src_net->pin[ip];
			cpin * pin = &net->pin[ip];
			*pin = *src_pin;
		}
		net->npins = src_net->npins;
		for( int ia=0; ia<src_net->nareas; ia++ )
		{
			carea * src_a = &src_net->area[ia];
			CPolyLine * src_poly = src_a->poly;
			AddArea( net, src_poly->GetLayer(), src_poly->GetX(0),
				src_poly->GetY(0), src_poly->GetHatch() );
			carea * a = &net->area[ia];
			CPolyLine * poly = a->poly;
			a->poly->Copy( src_poly );
			a->poly->SetDisplayList( NULL );
			a->npins = src_a->npins;
			a->pin.SetSize( a->npins );
			for( int ip=0; ip<a->npins; ip++ )
				a->pin[ip] = src_a->pin[ip];
			a->nvias = src_a->nvias;
			a->vcon.SetSize( a->nvias );
			a->vtx.SetSize( a->nvias );
			for( int ic=0; ic<a->nvias; ic++ )
			{
				a->vcon[ic] = src_a->vcon[ic];
				a->vtx[ic] = src_a->vtx[ic];
			}
		}
		for( int ic=0; ic<src_net->nconnects; ic++ )
		{
			cconnect * src_c = &src_net->connect[ic];
			if( src_c->end_pin == cconnect::NO_END )
				AddNetStub( net, src_c->start_pin );
			else
				AddNetConnect( net, src_c->start_pin, src_c->end_pin );
			cconnect * c = &net->connect[ic];
			c->seg.SetSize( src_c->nsegs );
			for( int is=0; is<src_c->nsegs; is++ )
			{
				c->seg[is] = src_c->seg[is];
				c->seg[is].dl_el = NULL;
				c->seg[is].dl_sel = NULL;
			}
			c->nsegs = src_c->nsegs;
			c->vtx.SetSize( c->nsegs+1 );
			for( int iv=0; iv<=c->nsegs; iv++ )
			{
				cvertex * v = &c->vtx[iv];
				cvertex * src_v = &src_c->vtx[iv];
				v->x = src_v->x;
				v->y = src_v->y;
				v->pad_layer = src_v->pad_layer;
				v->force_via_flag = src_v->force_via_flag;
				v->via_width_attrib = src_v->via_width_attrib;
				v->tee_ID = src_v->tee_ID;
			}
		}
		net->utility = src_net->utility;
	}
}

// reassign copper elements to new layers
// enter with layer[] = table of new copper layers for each old copper layer
//
void CNetList::ReassignCopperLayers( int n_new_layers, int * layer )
{
	if( m_layers < 1 || m_layers > 16 )
		ASSERT(0);

	CIterator_cnet net_iter(this);
	for( cnet * net = net_iter.GetFirst(); net != NULL; net = net_iter.GetNext() )
	{
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			cconnect * c = &net->connect[ic];
			for( int is=0; is<c->nsegs; is++ )
			{
				cseg * s = &c->seg[is];
				int old_layer = s->layer;
				if( old_layer >= LAY_TOP_COPPER )
				{
					int index = old_layer - LAY_TOP_COPPER;
					int new_layer = layer[index];
					if( new_layer == -1 )
					{
						// delete this layer
						UnrouteSegmentWithoutMerge( net, ic, is );
					}
					else
						s->layer = new_layer + LAY_TOP_COPPER;
				}
			}
			// check for first or last segments connected to SMT pins
			cvertex * v = &c->vtx[0];
			cseg * s = &c->seg[0];
			if( v->pad_layer && v->pad_layer != LAY_PAD_THRU && s->layer != v->pad_layer )
				UnrouteSegmentWithoutMerge( net, ic, 0 );
			if( c->end_pin != cconnect::NO_END )
			{
				v = &c->vtx[c->nsegs];
				s = &c->seg[c->nsegs-1];
				if( v->pad_layer && v->pad_layer != LAY_PAD_THRU && s->layer != v->pad_layer )
					UnrouteSegmentWithoutMerge( net, ic, c->nsegs-1 );
			}
			MergeUnroutedSegments( net, ic );
		}
		CleanUpConnections( net );
		for( int ia=net->nareas-1; ia>=0; ia-- )
		{
			CPolyLine * poly = net->area[ia].poly;
			int old_layer = poly->GetLayer();
			int index = old_layer - LAY_TOP_COPPER;
			int new_layer = layer[index];
			if( new_layer == -1 )
			{
				// delete this area
				RemoveArea( net, ia );
			}
			else
			{
				poly->SetLayer( new_layer + LAY_TOP_COPPER );
				poly->Draw();
			}
		}
		CombineAllAreasInNet( net, TRUE, FALSE );
	}
	m_layers = n_new_layers;
}

// When net names change, try to restore traces and areas from previous netlist
//
void CNetList::RestoreConnectionsAndAreas( CNetList * old_nl, int flags, CDlgLog * log )
{
	// loop through old nets
	CIterator_cnet net_iter(old_nl);
	for( cnet * old_net = net_iter.GetFirst(); old_net != NULL; old_net = net_iter.GetNext() )
	{
		if( flags & (KEEP_TRACES | KEEP_STUBS) )
		{
			BOOL bRestored = TRUE;	// flag to indicate at least one connection restored
			while( bRestored )
			{
				bRestored = FALSE;
				// loop through old connections
				for( int old_ic=0; old_ic<old_net->nconnects; old_ic++ )
				{
					cconnect * old_c = &old_net->connect[old_ic];
					if( old_c->utility )
						continue;	// ignore if already flagged
					if( old_c->nsegs == 1 && old_c->seg[0].layer == LAY_RAT_LINE )
					{
						old_c->utility = 1;
						continue;	// ignore pure ratline connections
					}
					// check net of starting pin
					cpin * old_start_pin = &old_net->pin[old_c->start_pin];
					cpart * new_start_part = m_plist->GetPart( old_start_pin->ref_des() );
					cnet * new_start_net = NULL;
					if( new_start_part )
						new_start_net = m_plist->GetPinNet( new_start_part, &old_start_pin->pin_name );
					if( !new_start_net )
					{
						old_c->utility = 1;
						continue;	// ignore if start pin not on any net
					}
					if( new_start_net->name == old_net->name )
					{
						old_c->utility = 1;
						continue;	// ignore if start pin has not changed net
					}
					// check position of starting pin
					CPoint st_p = m_plist->GetPinPoint( new_start_part, old_start_pin->pin_name );
					int st_l = m_plist->GetPinLayer( new_start_part, old_start_pin->pin_name );
					if( st_p.x != old_c->vtx[0].x || st_p.y != old_c->vtx[0].y )
					{
						old_c->utility = 1;
						continue;	// ignore if start pin position has changed
					}
					if( st_l != LAY_PAD_THRU && old_c->seg[0].layer != LAY_RAT_LINE && st_l != old_c->seg[0].layer )
					{
						old_c->utility = 1;
						continue;	// ignore if start pin layer doesn't match first segment
					}
					// see if we should move trace to new net
					cpin * old_end_pin = NULL;
					if( old_c->end_pin == cconnect::NO_END )
					{
						// stub trace
						int tee_ID = old_c->vtx[old_c->nsegs].tee_ID;
						if( !(flags & KEEP_STUBS) && tee_ID == 0 )
						{
							old_c->utility = 1;
							continue;	// ignore if we are not moving stubs
						}
						if( tee_ID && !FindTeeVertexInNet( new_start_net, tee_ID ) )
							continue;	// branch, but tee_ID not found in net
					}
					else
					{
						// normal trace
						if( flags & KEEP_TRACES )
						{
							old_end_pin = &old_net->pin[old_c->end_pin];
							// see if end pin still exists and is on the same new net
							cpart * new_end_part = m_plist->GetPart( old_end_pin->ref_des() );
							cnet * new_end_net = NULL;
							if( new_end_part )
								new_end_net = m_plist->GetPinNet( new_end_part, &old_end_pin->pin_name );
							if( new_start_net != new_end_net )
							{
								old_c->utility = 1;
								continue;
							}
							// check position of end pin
							CPoint e_p = m_plist->GetPinPoint( new_end_part, old_end_pin->pin_name );
							int e_l = m_plist->GetPinLayer( new_end_part, old_end_pin->pin_name );
							if( e_p.x != old_c->vtx[old_c->nsegs].x || e_p.y != old_c->vtx[old_c->nsegs].y )
							{
								old_c->utility = 1;
								continue;	// ignore if end pin position has changed
							}
							if( e_l != LAY_PAD_THRU && old_c->seg[old_c->nsegs-1].layer != LAY_RAT_LINE && e_l != old_c->seg[old_c->nsegs-1].layer )
							{
								old_c->utility = 1;
								continue;	// ignore if end pin layer doesn't match last segment
							}
						}
					}
					// Restore it in new net
					if( log )
					{
						CString line;
						if( old_c->end_pin == cconnect::NO_END )
						{
							// branch or stub
							int tee_id = old_c->vtx[old_c->nsegs].tee_ID;
							if( !tee_id )
								line.Format( "  Moving stub trace from %s.%s to new net \"%s\"\r\n",
								old_start_pin->ref_des(), old_start_pin->pin_name, new_start_net->name );
							else
								line.Format( "  Moving branch from %s.%s to new net \"%s\"\r\n",
								old_start_pin->ref_des(), old_start_pin->pin_name, new_start_net->name );
						}
						else
						{
							// pin-pin trace
							line.Format( "  Moving trace from %s.%s to %s.%s to new net \"%s\"\r\n",
								old_start_pin->ref_des(), old_start_pin->pin_name,
								old_end_pin->ref_des(),   old_end_pin->pin_name,
								new_start_net->name );
						}
						log->AddLine( line );
					}
					cnet * net = new_start_net;
					int ic = -1;
					int new_start_pin_index = GetNetPinIndex( net, old_start_pin->ref_des(), old_start_pin->pin_name );
					if( old_c->end_pin == cconnect::NO_END )
					{
						ic = AddNetStub( net, new_start_pin_index );
					}
					else
					{
						int new_end_pin_index = GetNetPinIndex( net, old_end_pin->ref_des(), old_end_pin->pin_name );
						ic = AddNetConnect( net, new_start_pin_index, new_end_pin_index );
					}
					if( ic > -1 )
					{
						old_c->utility = 1;		// flag as already drawn
						bRestored = TRUE;		// and a trace was restored
						UndrawConnection( net, ic );
						cconnect * c = &net->connect[ic];
						c->seg.SetSize( old_c->nsegs );
						for( int is=0; is<old_c->nsegs; is++ )
						{
							c->seg[is] = old_c->seg[is];
							c->seg[is].dl_el = NULL;
							c->seg[is].dl_sel = NULL;
						}
						c->nsegs = old_c->nsegs;
						c->vtx.SetSize( old_c->nsegs+1 );
						for( int iv=0; iv<=old_c->nsegs; iv++ )
						{
							cvertex * v = &c->vtx[iv];
							cvertex * src_v = &old_c->vtx[iv];
							v->x = src_v->x;
							v->y = src_v->y;
							v->pad_layer = src_v->pad_layer;
							v->force_via_flag = src_v->force_via_flag;
							v->via_width_attrib = src_v->via_width_attrib;
							v->tee_ID = src_v->tee_ID;
							if( v->tee_ID && iv < old_c->nsegs )
								AddTeeID( v->tee_ID );
							v->dl_el.RemoveAll();
							v->dl_sel = NULL;
							v->dl_hole = NULL;
							v->m_dlist = m_dlist;
						}
						DrawConnection( net, ic );
					}
				}
			}
		}
		if( flags & KEEP_AREAS )
		{
			// see if copper areas can be moved
			for( int ia=0; ia<old_net->nareas; ia++ )
			{
				BOOL bMoveIt = TRUE;
				cnet * new_area_net = NULL;
				carea * old_a = &old_net->area[ia];
				for( int ip=0; ip<old_a->npins; ip++ )
				{
					int old_pin_index = old_a->pin[ip];
					cpin * old_pin = &old_net->pin[old_pin_index];
					cpart * new_pin_part = m_plist->GetPart( old_pin->ref_des() );
					cnet * new_pin_net = NULL;
					if( new_pin_part )
						new_pin_net = m_plist->GetPinNet( new_pin_part, &old_pin->pin_name );
					if( new_area_net == NULL )
						new_area_net = new_pin_net;
					else if( new_pin_net != new_area_net )
					{
						// can't move area because pins are now on multiple nets
						bMoveIt = FALSE;
						break;
					}
				}
				if( bMoveIt )
				{
					for( int ic=0; ic<old_a->nvias; ic++ )
					{
						cconnect * old_con = &old_net->connect[old_a->vcon[ic]];
						int old_pin_index = old_con->start_pin;
						cpin * old_pin = &old_net->pin[old_pin_index];
						cpart * new_pin_part = m_plist->GetPart( old_pin->ref_des() );
						cnet * new_pin_net = NULL;
						if( new_pin_part )
							new_pin_net = m_plist->GetPinNet( new_pin_part, &old_pin->pin_name );
						if( new_area_net == NULL )
							new_area_net = new_pin_net;
						else if( new_pin_net != new_area_net )
						{
							// can't move area because pins and stubs are now on multiple nets
							bMoveIt = FALSE;
							break;
						}
					}
				}
				if( new_area_net == NULL )
					break;	// no pins or stubs connected, go to next area
				if( old_net->name == new_area_net->name )
					break;	// area doesn't need to be moved, go to next area
				if( log )
				{
					CString line;
					if( !bMoveIt )
						line.Format( "  Removing copper area on old net \"%s\"\r\n",
						old_net->name );
					else
						line.Format( "  Moving copper area from old net \"%s\" to new net \"%s\"\r\n",
						old_net->name, new_area_net->name );
					log->AddLine( line );
				}
				if( bMoveIt )
				{
					// move the area onto the new net
					CPolyLine * old_poly = old_a->poly;
					cnet * net = new_area_net;
					int ia = AddArea( net, old_poly->GetLayer(), old_poly->GetX(0),
						old_poly->GetY(0), old_poly->GetHatch() );
					carea * a = &net->area[ia];
					CPolyLine * poly = a->poly;
					poly->Copy( old_poly );
					id p_id( ID_NET, ID_AREA, ia, 0, 0 );
					poly->SetId( &p_id );
					poly->SetPtr( net );
					poly->Draw( m_dlist );
				}
			}
		}
	}
}

undo_con * CNetList::CreateConnectUndoRecord( cnet * net, int icon, BOOL set_areas )
{
	// calculate size needed, get memory
	cconnect * c = &net->connect[icon];

	int seg_offset = sizeof(undo_con);
	int vtx_offset = seg_offset + sizeof(undo_seg)*(c->nsegs);
	int size = vtx_offset + sizeof(undo_vtx)*(c->nsegs+1);
	void * ptr = malloc( size );

	undo_con * con = (undo_con*)ptr;
	undo_seg * seg = (undo_seg*)(seg_offset+(UINT)ptr);
	undo_vtx * vtx = (undo_vtx*)(vtx_offset+(UINT)ptr);

	con->size = size;
	strcpy( con->net_name, net->name );
	con->start_pin = c->start_pin;
	con->end_pin = c->end_pin;
	con->nsegs = c->nsegs;
	con->locked = c->locked;
	con->set_areas_flag = set_areas;
	con->seg_offset = seg_offset;
	con->vtx_offset = vtx_offset;

	for( int is=0; is<c->nsegs; is++ )
	{
		// Use placement new to call ctor for each segment
		undo_seg *pSeg = new(&seg[is]) undo_seg;

		pSeg->layer = c->seg[is].layer;

		// Save the width attrib
		pSeg->width_attrib = c->seg[is].width_attrib;
	}

	for( int iv=0; iv<=con->nsegs; iv++ )
	{
		// Use placement new to call ctor for each vertex
		undo_vtx *pVtx = new(&vtx[iv]) undo_vtx;

		pVtx->x              = c->vtx[iv].x;
		pVtx->y              = c->vtx[iv].y;
		pVtx->pad_layer      = c->vtx[iv].pad_layer;
		pVtx->force_via_flag = c->vtx[iv].force_via_flag;
		pVtx->tee_ID         = c->vtx[iv].tee_ID;
		pVtx->width_attrib   = c->vtx[iv].via_width_attrib;
	}
	con->nlist = this;
	return con;
}

// callback function for undoing connections
// note that this is declared static, since it is a callback
//
void CNetList::ConnectUndoCallback( int type, void * ptr, BOOL undo )
{
	if( undo )
	{
		undo_con * con = (undo_con*)ptr;
		CNetList * nl = con->nlist;
		if( type == UNDO_CONNECT_MODIFY )
		{
			// now recreate connection
			CString temp = con->net_name;
			cnet * net = nl->GetNetPtrByName( &temp );
			// get segment and vertex pointers
			undo_seg * seg = (undo_seg*)((UINT)ptr+con->seg_offset);
			undo_vtx * vtx = (undo_vtx*)((UINT)ptr+con->vtx_offset);
			// now add connection
			int nc;
			if( con->nsegs )
			{
				if( con->end_pin != cconnect::NO_END )
					nc = nl->AddNetConnect( net, con->start_pin, con->end_pin );
				else
					nc = nl->AddNetStub( net, con->start_pin );
				cconnect * c = &net->connect[nc];
				for( int is=0; is<con->nsegs; is++ )
				{
					if( con->end_pin != cconnect::NO_END )
					{
						// pin-pin trace
						nl->InsertSegment( net, nc, is, vtx[is+1].x, vtx[is+1].y,
							seg[is].layer, seg[is].width_attrib, 0 );
					}
					else
					{
						// stub trace
						nl->AppendSegment( net, nc, vtx[is+1].x, vtx[is+1].y,
							seg[is].layer, seg[is].width_attrib );
					}
				}
				for( int is=0; is < con->nsegs; is++ )
				{
					c->vtx[is+1].via_width_attrib = vtx[is+1].width_attrib;

					if( vtx[is+1].force_via_flag )
						nl->ForceVia( net, nc, is+1, FALSE );

					c->vtx[is+1].tee_ID = vtx[is+1].tee_ID;
					if( vtx[is+1].tee_ID )
						nl->AddTeeID( vtx[is+1].tee_ID );

					nl->ReconcileVia( net, nc, is+1, c->vtx[is+1].via_width_attrib );
				}
				// other parameters
				net->connect[nc].locked = con->locked;
			}
			nl->DrawConnection( net, nc );
		}
		else
			ASSERT(0);
	}
	free( ptr );
}

// Create undo record for a net
// Just saves the name and pin list
// Assumes that connection undo records will be created separately
// for all connections
//
undo_net * CNetList::CreateNetUndoRecord( cnet * net )
{
	int size = sizeof(undo_net) + net->npins*sizeof(undo_pin);
	undo_net * undo = new( malloc( size ) ) undo_net;

	strcpy( undo->name, net->name );

	undo->width_attrib = net->def_width_attrib;

	undo->npins = net->npins;
	undo_pin * pin_mem = (undo_pin*)((UINT)undo + sizeof(undo_net));
	for( int ip=0; ip<net->npins; ip++, pin_mem++ )
	{
		// Construct un_pin in place
		undo_pin * un_pin = new(pin_mem) undo_pin;

		strcpy( un_pin->ref_des,  net->pin[ip].ref_des() );
		strcpy( un_pin->pin_name, net->pin[ip].pin_name );

		un_pin->clearance.Undef();

		cpin *pin = &net->pin[ip];
		cpart *part = pin->part;

		if( part != NULL )
		{
			CShape *shape = part->shape;

			// Shape may not assigned (==NULL) if the part
			// hasn't been assigned to a shape yet.
			if( shape != NULL )
			{
				int pin_index = shape->GetPinIndexByName( pin->pin_name );
				if( pin_index >= 0 )
				{
					un_pin->clearance = part->pin[pin_index].clearance;
				}
			}
		}
	}
	undo->nlist = this;
	undo->size = size;
	return undo;
}

// callback function for undoing modifications to nets
// removes all connections, and regenerates pin array
// assumes that subsequent callbacks will regenerate connections
// and copper areas
//
void CNetList::NetUndoCallback( int type, void * ptr, BOOL undo )
{
	if( undo )
	{
		// remove all connections from net
		// assuming that they will be replaced by subsequent undo items
		// do not remove copper areas
		undo_net * undo = (undo_net*)ptr;
		undo_pin * un_pin = (undo_pin*)((UINT)ptr + sizeof(undo_net));
		CNetList * nl = undo->nlist;
		CString temp = undo->name;
		cnet * net = nl->GetNetPtrByName( &temp );
		if( type == UNDO_NET_OPTIMIZE )
		{
			// re-optimize the net
			nl->OptimizeConnections( net );
		}
		else if( type == UNDO_NET_ADD )
		{
			// just delete the net
			nl->RemoveNet( net );
		}
		else if( type == UNDO_NET_MODIFY )
		{
			// restore the net
			if( !net )
				ASSERT(0);
			for( int ic=(net->nconnects-1); ic>=0; ic-- )
				nl->RemoveNetConnect( net, ic, FALSE );

			// Reset size attributes
			net->def_width_attrib = undo->width_attrib;
			net->def_width_attrib.SetParent( nl->m_def_width_attrib );
			net->def_width_attrib.Update();

			// replace pin data
			net->pin.SetSize(0);
			net->npins = 0;
			for( int ip=0; ip<undo->npins; ip++ )
			{
				CString ref_str( un_pin[ip].ref_des );
				CString pin_name( un_pin[ip].pin_name );
				part_pin *pin = nl->AddNetPin( net, ref_str, pin_name, FALSE );
				if( pin != NULL )
				{
					pin->set_clearance( un_pin[ip].clearance );
				}
			}
			nl->RehookPartsToNet( net );
		}
		else
			ASSERT(0);
		// adjust connections to areas
//**		if( net->nareas )
//**			nl->SetAreaConnections( net );
	}
	free( ptr );
}

// create undo record for area
// only includes closed contours
//
undo_area * CNetList::CreateAreaUndoRecord( cnet * net, int iarea, int type )
{
	undo_area * un_a;
	if( type == CNetList::UNDO_AREA_ADD || type == CNetList::UNDO_AREA_CLEAR_ALL )
	{
		un_a = (undo_area*)malloc(sizeof(undo_area));
		strcpy( un_a->net_name, net->name );
		un_a->nlist = this;
		un_a->iarea = iarea;
		return un_a;
	}
	CPolyLine * p = net->area[iarea].poly;
	int n_cont = p->GetNumContours();
	if( !p->GetClosed() )
		n_cont--;
	int nc = p->GetContourEnd( n_cont-1 ) + 1;
	if( type == CNetList::UNDO_AREA_ADD )
		un_a = (undo_area*)malloc(sizeof(undo_area));
	else if( type == CNetList::UNDO_AREA_DELETE
		|| type == CNetList::UNDO_AREA_MODIFY )
		un_a = (undo_area*)malloc(sizeof(undo_area)+nc*sizeof(undo_corner));
	else
		ASSERT(0);
	un_a->size = sizeof(undo_area)+nc*sizeof(undo_corner);
	strcpy( un_a->net_name, net->name );
	un_a->iarea = iarea;
	un_a->ncorners = nc;
	un_a->layer = p->GetLayer();
	un_a->hatch = p->GetHatch();
	un_a->w = p->GetW();
	un_a->sel_box_w = p->GetSelBoxSize();
	undo_corner * un_c = (undo_corner*)((UINT)un_a + sizeof(undo_area));
	for( int ic=0; ic<nc; ic++ )
	{
		un_c[ic].x = p->GetX( ic );
		un_c[ic].y = p->GetY( ic );
		un_c[ic].end_contour = p->GetEndContour( ic );
		un_c[ic].style = p->GetSideStyle( ic );
	}
	un_a->nlist = this;
	return un_a;
}

// callback function for undoing areas
// note that this is declared static, since it is a callback
//
void CNetList::AreaUndoCallback( int type, void * ptr, BOOL undo )
{
	if( undo )
	{
		undo_area * a = (undo_area*)ptr;
		CNetList * nl = a->nlist;
		CString temp = a->net_name;
		cnet * net = nl->GetNetPtrByName( &temp );
		if( !net )
			ASSERT(0);
		if( type == UNDO_AREA_CLEAR_ALL )
		{
			// delete all areas in this net
			for( int ia=net->area.GetSize()-1; ia>=0; ia-- )
				nl->RemoveArea( net, ia );
		}
		else if( type == UNDO_AREA_ADD )
		{
			// delete selected area
			nl->RemoveArea( net, a->iarea );
		}
		else if( type == UNDO_AREA_MODIFY
				|| type == UNDO_AREA_DELETE )
		{
			undo_corner * c = (undo_corner*)((UINT)ptr+sizeof(undo_area));
			if( type == UNDO_AREA_MODIFY )
			{
				// remove area
				nl->RemoveArea( net, a->iarea );
			}
			// now recreate area at its original iarea in net->area[iarea]
			nl->InsertArea( net, a->iarea, a->layer, c[0].x, c[0].y, a->hatch );
			for( int ic=1; ic<a->ncorners; ic++ )
			{
				nl->AppendAreaCorner( net, a->iarea,
					c[ic].x, c[ic].y, c[ic-1].style, FALSE );
				if( c[ic].end_contour )
					nl->CompleteArea( net, a->iarea, c[ic].style );
			}
			nl->RenumberAreas( net );
		}
		else
			ASSERT(0);
	}
	free( ptr );
}

// cross-check netlist with partlist, report results in logstr
//
int CNetList::CheckNetlist( CString * logstr )
{
	CString str;
	int nwarnings = 0;
	int nerrors = 0;
	int nfixed = 0;
	CMapStringToPtr net_map;
	CMapStringToPtr pin_map;

	*logstr += "***** Checking Nets *****\r\n";

	// traverse map
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		// next net
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		CString net_name = net->name;
		if( net_map.Lookup( net_name, ptr ) )
		{
			str.Format( "ERROR: Net \"%s\" is duplicate\r\n", net_name );
			str += "    ###   To fix this, delete one instance of the net, then save and re-open project\r\n";
			*logstr += str;
			nerrors++;
		}
		else
			net_map.SetAt( net_name, NULL );
		int npins = net->pin.GetSize();
		if( npins == 0 )
		{
			str.Format( "Warning: Net \"%s\": has no pins\r\n", net->name );
			*logstr += str;
			nwarnings++;
		}
		else if( npins == 1 )
		{
			str.Format( "Warning: Net \"%s\": has single pin\r\n", net->name );
			*logstr += str;
			nwarnings++;
		}
		for( int ip=0; ip<net->pin.GetSize(); ip++ )
		{
			// next pin in net
			CString const * ref_des = &net->pin[ip].ref_des();
			CString const * pin_name = &net->pin[ip].pin_name;
			CString pin_id = *ref_des + "." + *pin_name;
			void * ptr;
			BOOL test = pin_map.Lookup( pin_id, ptr );
			cnet * dup_net = (cnet*)ptr;
			if( test )
			{
				if( dup_net->name == net_name )
				{
					str.Format( "ERROR: Net \"%s\": pin \"%s\" is duplicate\r\n",
						net->name, pin_id );
					*logstr += str;
					// reassign all connections
					// find index of first instance of pin
					int first_index = -1;
					for( int iip=0; iip<net->pin.GetSize(); iip++ )
					{
						if( net->pin[iip].ref_des() == *ref_des && net->pin[iip].pin_name == *pin_name )
						{
							first_index = iip;
							break;
						}
					}
					if( first_index == -1 )
						ASSERT(0);
					// reassign connections
					for( int ic=0; ic<net->connect.GetSize(); ic++ )
					{
						cconnect * c = &net->connect[ic];
						if( c->start_pin == ip )
							c->start_pin = first_index;
						if( c->end_pin == ip )
							c->end_pin = first_index;
					}
					// remove pin
					RemoveNetPin( net, ip );
					RehookPartsToNet( net );
					str.Format( "              Fixed: Connections repaired\r\n" );
					*logstr += str;
					nerrors++;
					nfixed++;
					continue;		// no further testing on this pin
				}
				else
				{
					str.Format( "ERROR: Net \"%s\": pin \"%s\" already assigned to net \"%s\"\r\n",
						net->name, pin_id, dup_net->name );
					str += "    ###   To fix this, delete pin from one of these nets, then save and re-open project\r\n";
					nerrors++;
					*logstr += str;
				}
			}
			else
				pin_map.SetAt( pin_id, net );

			cpart * part = net->pin[ip].part;
			if( !part )
			{
				// net->pin->part == NULL, find out why
				// see if part exists in partlist
				cpart * test_part = m_plist->GetPart( *ref_des );
				if( !test_part )
				{
					// no
					str.Format( "Warning: Net \"%s\": pin \"%s.%s\" not connected, part doesn't exist\r\n",
						net->name, *ref_des, *pin_name, net->name );
					*logstr += str;
					nwarnings++;
				}
				else
				{
					// yes, see if it has footprint
					if( !test_part->shape )
					{
						// no
						str.Format( "Warning: Net \"%s\": pin \"%s.%s\" connected, part doesn't have footprint\r\n",
							net->name, *ref_des, *pin_name, net->name );
						*logstr += str;
						nwarnings++;
					}
					else
					{
						// yes, see if pin exists
						int pin_index = test_part->shape->GetPinIndexByName( *pin_name );
						if( pin_index == -1 )
						{
							// no
							str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" not connected, but part exists although pin doesn't\r\n",
								net->name, *ref_des, *pin_name, net->name );
							str += "    ###   To fix this, fix any other errors then save and re-open project\r\n";
							*logstr += str;
							nerrors++;
						}
						else
						{
							// yes
							str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" not connected, but part and pin exist\r\n",
								net->name, *ref_des, *pin_name, net->name );
							str += "    ###   To fix this, fix any other errors then save and re-open project\r\n";
							*logstr += str;
							nerrors++;
						}
					}
				}
			}
			else
			{
				// net->pin->part exists, check parameters
				if( part->ref_des != *ref_des )
				{
					// net->pin->ref_des != net->pin->part->ref_des
					str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" connected to wrong part %s\r\n",
						net->name, *ref_des, *pin_name, part->ref_des );
					str += "    ###   To fix this, fix any other errors then save and re-open project\r\n";
					*logstr += str;
					nerrors++;
				}
				else
				{
					cpart * partlist_part = m_plist->GetPart( *ref_des );
					if( !partlist_part )
					{
						// net->pin->ref_des not found in partlist
						str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" connected but part not in partlist\r\n",
							net->name, *ref_des, *pin_name );
						*logstr += str;
						nerrors++;
					}
					else
					{
						if( part != partlist_part )
						{
							// net->pin->ref_des found in partlist, but doesn't match net->pin->part
							str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" connected but net->pin->part doesn't match partlist\r\n",
								net->name, *ref_des, *pin_name );
							str += "    ###   To fix this, fix any other errors then save and re-open project\r\n";
							*logstr += str;
							nerrors++;
						}
						else
						{
							if( !part->shape )
							{
								// part matches, but no footprint
								str.Format( "Warning: Net \"%s\": pin \"%s.%s\" connected but part doesn't have footprint\r\n",
									net->name, *ref_des, *pin_name );
								*logstr += str;
								nwarnings++;
							}
							else
							{
								int pin_index = part->shape->GetPinIndexByName( *pin_name );
								if( pin_index == -1 )
								{
									// net->pin->pin_name doesn't exist in part
									str.Format( "Warning: Net \"%s\": pin \"%s.%s\" connected but part doesn't have pin\r\n",
										net->name, *ref_des, *pin_name );
									*logstr += str;
									nwarnings++;
								}
								else
								{
									cnet * part_pin_net = part->pin[pin_index].net;
									if( part_pin_net != net )
									{
										// part->pin->net != net
										str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" connected but part->pin->net doesn't match\r\n",
											net->name, *ref_des, *pin_name );
										str += "    ###   To fix this, fix any other errors then save and re-open project\r\n";
										*logstr += str;
										nerrors++;
									}
									else
									{
										// OK, all is well, peace on earth
									}
								}
							}
						}
					}
				}
			}
		}
		// now check connections
		for( int ic=0; ic<net->connect.GetSize(); ic++ )
		{
			cconnect * c = &net->connect[ic];
			if( c->nsegs == 0 )
			{
				str.Format( "ERROR: Net \"%s\": connection with no segments\r\n",
					net->name );
				*logstr += str;
				RemoveNetConnect( net, ic, FALSE );
				str.Format( "              Fixed: Connection removed\r\n",
					net->name );
				*logstr += str;
				nerrors++;
				nfixed++;
			}
			else if( c->start_pin == c->end_pin )
			{
				str.Format( "ERROR: Net \"%s\": connection from pin to itself\r\n",
					net->name );
				*logstr += str;
				RemoveNetConnect( net, ic, FALSE );
				str.Format( "              Fixed: Connection removed\r\n",
					net->name );
				*logstr += str;
				nerrors++;
				nfixed++;
			}
		}
	}
	str.Format( "***** %d ERROR(S), %d FIXED, %d WARNING(S) *****\r\n",
		nerrors, nfixed, nwarnings );
	*logstr += str;
	return nerrors;
}

// cross-check netlist with partlist, report results in logstr
//
int CNetList::CheckConnectivity( CString * logstr )
{
	CString str;
	int nwarnings = 0;
	int nerrors = 0;
	int nfixed = 0;
	CMapStringToPtr net_map;
	CMapStringToPtr pin_map;

	// traverse map
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		// next net
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		CString net_name = net->name;
		// now check connections
		for( int ic=0; ic<net->connect.GetSize(); ic++ )
		{
			cconnect * c = &net->connect[ic];
			if( c->nsegs == 0 )
			{
				str.Format( "ERROR: Net \"%s\": connection with no segments\r\n",
					net->name );
				*logstr += str;
				RemoveNetConnect( net, ic, FALSE );
				str.Format( "              Fixed: Connection removed\r\n",
					net->name );
				*logstr += str;
				nerrors++;
				nfixed++;
			}
			else if( c->start_pin == c->end_pin )
			{
				str.Format( "ERROR: Net \"%s\": connection from pin to itself\r\n",
					net->name );
				*logstr += str;
				RemoveNetConnect( net, ic, FALSE );
				str.Format( "              Fixed: Connection removed\r\n",
					net->name );
				*logstr += str;
				nerrors++;
				nfixed++;
			}
			else
			{
				// check for unrouted or partially routed connection
				BOOL bUnrouted = FALSE;
				for( int is=0; is<c->nsegs; is++ )
				{
					if( c->seg[is].layer == LAY_RAT_LINE )
					{
						bUnrouted = TRUE;
						break;
					}
				}
				if( bUnrouted )
				{
					CString start_pin, end_pin;
					int istart = c->start_pin;
					start_pin = net->pin[istart].ref_des() + "." + net->pin[istart].pin_name;
					int iend = c->end_pin;
					if( iend == cconnect::NO_END )
					{
						str.Format( "Net \"%s\": partially routed stub trace from %s\r\n",
							net->name, start_pin );
						*logstr += str;
						nerrors++;
					}
					else
					{
						end_pin = net->pin[iend].ref_des() + "." + net->pin[iend].pin_name;
						if( c->nsegs == 1 )
						{
							str.Format( "Net \"%s\": unrouted connection from %s to %s\r\n",
								net->name, start_pin, end_pin );
							*logstr += str;
							nerrors++;
						}
						else
						{
							str.Format( "Net \"%s\": partially routed connection from %s to %s\r\n",
								net->name, start_pin, end_pin );
							*logstr += str;
							nerrors++;
						}
					}
				}
			}
		}
	}
	return nerrors;
}

// Test an area for self-intersection.
// Returns:
//	-1 if arcs intersect other sides
//	 0 if no intersecting sides
//	 1 if intersecting sides, but no intersecting arcs
// Also sets utility2 flag of area with return value
//
int CNetList::TestAreaPolygon( cnet * net, int iarea )
{
	CPolyLine * p = net->area[iarea].poly;
	// first, check for sides intersecting other sides, especially arcs
	BOOL bInt = FALSE;
	BOOL bArcInt = FALSE;
	int n_cont = p->GetNumContours();
	// make bounding rect for each contour
	CArray<CRect> cr;
	cr.SetSize( n_cont );
	for( int icont=0; icont<n_cont; icont++ )
		cr[icont] = p->GetCornerBounds( icont );
	for( int icont=0; icont<n_cont; icont++ )
	{
		int is_start = p->GetContourStart(icont);
		int is_end = p->GetContourEnd(icont);
		for( int is=is_start; is<=is_end; is++ )
		{
			int is_prev = is - 1;
			if( is_prev < is_start )
				is_prev = is_end;
			int is_next = is + 1;
			if( is_next > is_end )
				is_next = is_start;
			int style = p->GetSideStyle( is );
			int x1i = p->GetX( is );
			int y1i = p->GetY( is );
			int x1f = p->GetX( is_next );
			int y1f = p->GetY( is_next );
			// check for intersection with any other sides
			for( int icont2=icont; icont2<n_cont; icont2++ )
			{
				if( cr[icont].left > cr[icont2].right
					|| cr[icont].bottom > cr[icont2].top
					|| cr[icont2].left > cr[icont].right
					|| cr[icont2].bottom > cr[icont].top )
				{
					// rectangles don't overlap, do nothing
				}
				else
				{
					int is2_start = p->GetContourStart(icont2);
					int is2_end = p->GetContourEnd(icont2);
					for( int is2=is2_start; is2<=is2_end; is2++ )
					{
						int is2_prev = is2 - 1;
						if( is2_prev < is2_start )
							is2_prev = is2_end;
						int is2_next = is2 + 1;
						if( is2_next > is2_end )
							is2_next = is2_start;
						if( icont != icont2 || (is2 != is && is2 != is_prev && is2 != is_next && is != is2_prev && is != is2_next ) )
						{
							int style2 = p->GetSideStyle( is2 );
							int x2i = p->GetX( is2 );
							int y2i = p->GetY( is2 );
							int x2f = p->GetX( is2_next );
							int y2f = p->GetY( is2_next );
							int ret = FindSegmentIntersections( x1i, y1i, x1f, y1f, style, x2i, y2i, x2f, y2f, style2 );
							if( ret )
							{
								// intersection between non-adjacent sides
								bInt = TRUE;
								if( style != CPolyLine::STRAIGHT || style2 != CPolyLine::STRAIGHT )
								{
									bArcInt = TRUE;
									break;
								}
							}
						}
					}
				}
				if( bArcInt )
					break;
			}
			if( bArcInt )
				break;
		}
		if( bArcInt )
			break;
	}
	if( bArcInt )
		net->area[iarea].utility2 = -1;
	else if( bInt )
		net->area[iarea].utility2 = 1;
	else
		net->area[iarea].utility2 = 0;
	return net->area[iarea].utility2;
}

// Process an area that has been modified, by clipping its polygon against itself.
// This may change the number and order of copper areas in the net.
// If bMessageBoxInt == TRUE, shows message when clipping occurs.
// If bMessageBoxArc == TRUE, shows message when clipping can't be done due to arcs.
// Returns:
//	-1 if arcs intersect other sides, so polygon can't be clipped
//	 0 if no intersecting sides
//	 1 if intersecting sides
// Also sets net->area->utility1 flags if areas are modified
//
int CNetList::ClipAreaPolygon( cnet * net, int iarea,
							  BOOL bMessageBoxArc, BOOL bMessageBoxInt, BOOL bRetainArcs )
{
	CPolyLine * p = net->area[iarea].poly;
	int test = TestAreaPolygon( net, iarea );	// this sets utility2 flag
	if( test == -1 && !bRetainArcs )
		test = 1;
	if( test == -1 )
	{
		// arc intersections, don't clip unless bRetainArcs == FALSE
		if( bMessageBoxArc && bDontShowSelfIntersectionArcsWarning == FALSE )
		{
			CString str;
			str.Format( "Area %d of net \"%s\" has arcs intersecting other sides.\n",
				iarea+1, net->name );
			str += "This may cause problems with other editing operations,\n";
			str += "such as adding cutouts. It can't be fixed automatically.\n";
			str += "Manual correction is recommended.\n";
			CDlgMyMessageBox dlg;
			dlg.Initialize( str );
			dlg.DoModal();
			bDontShowSelfIntersectionArcsWarning = dlg.bDontShowBoxState;
		}
		return -1;	// arcs intersect with other sides, error
	}

	// mark all areas as unmodified except this one
	for( int ia=0; ia<net->nareas; ia++ )
		net->area[ia].utility = 0;
	net->area[iarea].utility = 1;

	if( test == 1 )
	{
		// non-arc intersections, clip the polygon
		if( bMessageBoxInt && bDontShowSelfIntersectionWarning == FALSE)
		{
			CString str;
			str.Format( "Area %d of net \"%s\" is self-intersecting and will be clipped.\n",
				iarea+1, net->name );
			str += "This may result in splitting the area.\n";
			str += "If the area is complex, this may take a few seconds.";
			CDlgMyMessageBox dlg;
			dlg.Initialize( str );
			dlg.DoModal();
			bDontShowSelfIntersectionWarning = dlg.bDontShowBoxState;
		}
	}
//** TODO test for cutouts outside of area
//**	if( test == 1 )
	{
		CArray<CPolyLine*> * pa = new CArray<CPolyLine*>;
		p->Undraw();
		int n_poly = net->area[iarea].poly->NormalizeWithGpc( pa, bRetainArcs );
		if( n_poly > 1 )
		{
			for( int ip=1; ip<n_poly; ip++ )
			{
				// create new copper area and copy poly into it
				CPolyLine * new_p = (*pa)[ip-1];
				int ia = AddArea( net, 0, 0, 0, 0 );
				// remove the poly that was automatically created for the new area
				// and replace it with a poly from NormalizeWithGpc
				delete net->area[ia].poly;
				net->area[ia].poly = new_p;
				net->area[ia].poly->SetDisplayList( net->m_dlist );
				net->area[ia].poly->SetHatch( p->GetHatch() );
				net->area[ia].poly->SetLayer( p->GetLayer() );
				id p_id( ID_NET, ID_AREA, ia );
				net->area[ia].poly->SetId( &p_id );
				net->area[ia].poly->Draw();
				net->area[ia].utility = 1;
			}
		}
		p->Draw();
		delete pa;
	}
	return test;
}

// Process an area that has been modified, by clipping its polygon against
// itself and the polygons for any other areas on the same net.
// This may change the number and order of copper areas in the net.
// If bMessageBox == TRUE, shows message boxes when clipping occurs.
// Returns:
//	-1 if arcs intersect other sides, so polygon can't be clipped
//	 0 if no intersecting sides
//	 1 if intersecting sides, polygon clipped
//
int CNetList::AreaPolygonModified( cnet * net, int iarea, BOOL bMessageBoxArc, BOOL bMessageBoxInt )
{
	// clip polygon against itself
	int test = ClipAreaPolygon( net, iarea, bMessageBoxArc, bMessageBoxInt );
	if( test == -1 )
		return test;
	// now see if we need to clip against other areas
	BOOL bCheckAllAreas = FALSE;
	if( test == 1 )
		bCheckAllAreas = TRUE;
	else
		bCheckAllAreas = TestAreaIntersections( net, iarea );
	if( bCheckAllAreas )
		CombineAllAreasInNet( net, bMessageBoxInt, TRUE );
	SetAreaConnections( net );
	return test;
}

// Checks all copper areas in net for intersections, combining them if found
// If bUseUtility == TRUE, don't check areas if both utility flags are 0
// Sets utility flag = 1 for any areas modified
// If an area has self-intersecting arcs, doesn't try to combine it
//
int CNetList::CombineAllAreasInNet( cnet * net, BOOL bMessageBox, BOOL bUseUtility )
{
	if( net->nareas > 1 )
	{
		// start by testing all area polygons to set utility2 flags
		for( int ia=0; ia<net->nareas; ia++ )
			TestAreaPolygon( net, ia );
		// now loop through all combinations
		BOOL message_shown = FALSE;
		for( int ia1=0; ia1<net->nareas-1; ia1++ )
		{
			// legal polygon
			CRect b1 = net->area[ia1].poly->GetCornerBounds();
			BOOL mod_ia1 = FALSE;
			for( int ia2=net->nareas-1; ia2 > ia1; ia2-- )
			{
				if( net->area[ia1].poly->GetLayer() == net->area[ia2].poly->GetLayer()
					&& net->area[ia1].utility2 != -1 && net->area[ia2].utility2 != -1 )
				{
					CRect b2 = net->area[ia2].poly->GetCornerBounds();
					if( !( b1.left > b2.right || b1.right < b2.left
						|| b1.bottom > b2.top || b1.top < b2.bottom ) )
					{
						// check ia2 against 1a1
						if( net->area[ia1].utility || net->area[ia2].utility || bUseUtility == FALSE )
						{
							int ret = TestAreaIntersection( net, ia1, ia2 );
							if( ret == 1 )
								ret = CombineAreas( net, ia1, ia2 );
							if( ret == 1 )
							{
								if( bMessageBox && bDontShowIntersectionWarning == FALSE )
								{
									CString str;
									str.Format( "Areas %d and %d of net \"%s\" intersect and will be combined.\n",
										ia1+1, ia2+1, net->name );
									str += "If they are complex, this may take a few seconds.";
									CDlgMyMessageBox dlg;
									dlg.Initialize( str );
									dlg.DoModal();
									bDontShowIntersectionWarning = dlg.bDontShowBoxState;
								}
								mod_ia1 = TRUE;
							}
							else if( ret == 2 )
							{
								if( bMessageBox && bDontShowIntersectionArcsWarning == FALSE )
								{
									CString str;
									str.Format( "Areas %d and %d of net \"%s\" intersect, but some of the intersecting sides are arcs.\n",
										ia1+1, ia2+1, net->name );
									str += "Therefore, these areas can't be combined.";
									CDlgMyMessageBox dlg;
									dlg.Initialize( str );
									dlg.DoModal();
									bDontShowIntersectionArcsWarning = dlg.bDontShowBoxState;
								}
							}
						}
					}
				}
			}
			if( mod_ia1 )
				ia1--;		// if modified, we need to check it again
		}
	}
	return 0;
}

// Check for intersection of copper area with other areas in same net
//
BOOL CNetList::TestAreaIntersections( cnet * net, int ia )
{
	CPolyLine * poly1 = net->area[ia].poly;
	for( int ia2=0; ia2<net->nareas; ia2++ )
	{
		if( ia != ia2 )
		{
			// see if polygons are on same layer
			CPolyLine * poly2 = net->area[ia2].poly;
			if( poly1->GetLayer() != poly2->GetLayer() )
				continue;

			// test bounding rects
			CRect b1 = poly1->GetCornerBounds();
			CRect b2 = poly2->GetCornerBounds();
			if(    b1.bottom > b2.top
				|| b1.top < b2.bottom
				|| b1.left > b2.right
				|| b1.right < b2.left )
				continue;

			// test for intersecting segments
			BOOL bInt = FALSE;
			BOOL bArcInt = FALSE;
			for( int icont1=0; icont1<poly1->GetNumContours(); icont1++ )
			{
				int is1 = poly1->GetContourStart( icont1 );
				int ie1 = poly1->GetContourEnd( icont1 );
				for( int ic1=is1; ic1<=ie1; ic1++ )
				{
					int xi1 = poly1->GetX(ic1);
					int yi1 = poly1->GetY(ic1);
					int xf1, yf1, style1;
					if( ic1 < ie1 )
					{
						xf1 = poly1->GetX(ic1+1);
						yf1 = poly1->GetY(ic1+1);
					}
					else
					{
						xf1 = poly1->GetX(is1);
						yf1 = poly1->GetY(is1);
					}
					style1 = poly1->GetSideStyle( ic1 );
					for( int icont2=0; icont2<poly2->GetNumContours(); icont2++ )
					{
						int is2 = poly2->GetContourStart( icont2 );
						int ie2 = poly2->GetContourEnd( icont2 );
						for( int ic2=is2; ic2<=ie2; ic2++ )
						{
							int xi2 = poly2->GetX(ic2);
							int yi2 = poly2->GetY(ic2);
							int xf2, yf2, style2;
							if( ic2 < ie2 )
							{
								xf2 = poly2->GetX(ic2+1);
								yf2 = poly2->GetY(ic2+1);
							}
							else
							{
								xf2 = poly2->GetX(is2);
								yf2 = poly2->GetY(is2);
							}
							style2 = poly2->GetSideStyle( ic2 );
							int n_int = FindSegmentIntersections( xi1, yi1, xf1, yf1, style1,
								xi2, yi2, xf2, yf2, style2 );
							if( n_int )
								return TRUE;
						}
					}
				}
			}
		}
	}
	return FALSE;
}

// Test for intersection of 2 copper areas
// ia2 must be > ia1
// returns: 0 if no intersection
//			1 if intersection
//			2 if arcs intersect
//
int CNetList::TestAreaIntersection( cnet * net, int ia1, int ia2 )
{
	// see if polygons are on same layer
	CPolyLine * poly1 = net->area[ia1].poly;
	CPolyLine * poly2 = net->area[ia2].poly;
	if( poly1->GetLayer() != poly2->GetLayer() )
		return 0;

	// test bounding rects
	CRect b1 = poly1->GetCornerBounds();
	CRect b2 = poly2->GetCornerBounds();
	if(    b1.bottom > b2.top
		|| b1.top < b2.bottom
		|| b1.left > b2.right
		|| b1.right < b2.left )
		return 0;

	// now test for intersecting segments
	BOOL bInt = FALSE;
	BOOL bArcInt = FALSE;
	for( int icont1=0; icont1<poly1->GetNumContours(); icont1++ )
	{
		int is1 = poly1->GetContourStart( icont1 );
		int ie1 = poly1->GetContourEnd( icont1 );
		for( int ic1=is1; ic1<=ie1; ic1++ )
		{
			int xi1 = poly1->GetX(ic1);
			int yi1 = poly1->GetY(ic1);
			int xf1, yf1, style1;
			if( ic1 < ie1 )
			{
				xf1 = poly1->GetX(ic1+1);
				yf1 = poly1->GetY(ic1+1);
			}
			else
			{
				xf1 = poly1->GetX(is1);
				yf1 = poly1->GetY(is1);
			}
			style1 = poly1->GetSideStyle( ic1 );
			for( int icont2=0; icont2<poly2->GetNumContours(); icont2++ )
			{
				int is2 = poly2->GetContourStart( icont2 );
				int ie2 = poly2->GetContourEnd( icont2 );
				for( int ic2=is2; ic2<=ie2; ic2++ )
				{
					int xi2 = poly2->GetX(ic2);
					int yi2 = poly2->GetY(ic2);
					int xf2, yf2, style2;
					if( ic2 < ie2 )
					{
						xf2 = poly2->GetX(ic2+1);
						yf2 = poly2->GetY(ic2+1);
					}
					else
					{
						xf2 = poly2->GetX(is2);
						yf2 = poly2->GetY(is2);
					}
					style2 = poly2->GetSideStyle( ic2 );
					int n_int = FindSegmentIntersections( xi1, yi1, xf1, yf1, style1,
									xi2, yi2, xf2, yf2, style2 );
					if( n_int )
					{
						bInt = TRUE;
						if( style1 != CPolyLine::STRAIGHT || style2 != CPolyLine::STRAIGHT )
							bArcInt = TRUE;
						break;
					}
				}
				if( bArcInt )
					break;
			}
			if( bArcInt )
				break;
		}
		if( bArcInt )
			break;
	}
	if( !bInt )
		return 0;
	if( bArcInt )
		return 2;
	return 1;
}

// If possible, combine 2 copper areas
// ia2 must be > ia1
// returns: 0 if no intersection
//			1 if intersection
//			2 if arcs intersect
//
int CNetList::CombineAreas( cnet * net, int ia1, int ia2 )
{
	if( ia2 <= ia1 )
		ASSERT(0);
#if 0
	// test for intersection
	int test = TestAreaIntersection( net, ia1, ia2 );
	if( test != 1 )
		return test;	// no intersection
#endif

	// polygons intersect, combine them
	CPolyLine * poly1 = net->area[ia1].poly;
	CPolyLine * poly2 = net->area[ia2].poly;
	CArray<CArc> arc_array1;
	CArray<CArc> arc_array2;
	poly1->MakeGpcPoly( -1, &arc_array1 );
	poly2->MakeGpcPoly( -1, &arc_array2 );
	int n_ext_cont1 = 0;
	for( int ic=0; ic<poly1->GetGpcPoly()->num_contours; ic++ )
		if( !((poly1->GetGpcPoly()->hole)[ic]) )
			n_ext_cont1++;
	int n_ext_cont2 = 0;
	for( int ic=0; ic<poly2->GetGpcPoly()->num_contours; ic++ )
		if( !((poly2->GetGpcPoly()->hole)[ic]) )
			n_ext_cont2++;

	gpc_polygon * union_gpc = new gpc_polygon;
	gpc_polygon_clip( GPC_UNION, poly1->GetGpcPoly(), poly2->GetGpcPoly(), union_gpc );

	// get number of outside contours
	int n_union_ext_cont = 0;
	for( int ic=0; ic<union_gpc->num_contours; ic++ )
		if( !((union_gpc->hole)[ic]) )
			n_union_ext_cont++;

	// if no intersection, free new gpc and return
	if( n_union_ext_cont == n_ext_cont1 + n_ext_cont2 )
	{
		gpc_free_polygon( union_gpc );
		delete union_gpc;
		return 0;
	}

	// intersection, replace ia1 with combined areas and remove ia2
	RemoveArea( net, ia2 );
	int hatch = net->area[ia1].poly->GetHatch();
	id a_id = net->area[ia1].poly->GetId();
	int layer = net->area[ia1].poly->GetLayer();
	int w = net->area[ia1].poly->GetW();
	int sel_box = net->area[ia1].poly->GetSelBoxSize();
	RemoveArea( net, ia1 );
	// create area with external contour
	for( int ic=0; ic<union_gpc->num_contours; ic++ )
	{
		if( !(union_gpc->hole)[ic] )
		{
			// external contour, replace this poly
			for( int i=0; i<union_gpc->contour[ic].num_vertices; i++ )
			{
				int x = ((union_gpc->contour)[ic].vertex)[i].x;
				int y = ((union_gpc->contour)[ic].vertex)[i].y;
				if( i==0 )
				{
					InsertArea( net, ia1, layer, x, y, hatch );
				}
				else
					AppendAreaCorner( net, ia1, x, y, CPolyLine::STRAIGHT, FALSE );
			}
			CompleteArea( net, ia1, CPolyLine::STRAIGHT );
			RenumberAreas( net );
		}
	}
	// add holes
	for( int ic=0; ic<union_gpc->num_contours; ic++ )
	{
		if( (union_gpc->hole)[ic] )
		{
			// hole
			for( int i=0; i<union_gpc->contour[ic].num_vertices; i++ )
			{
				int x = ((union_gpc->contour)[ic].vertex)[i].x;
				int y = ((union_gpc->contour)[ic].vertex)[i].y;
				AppendAreaCorner( net, ia1, x, y, CPolyLine::STRAIGHT, FALSE );
			}
			CompleteArea( net, ia1, CPolyLine::STRAIGHT );
		}
	}
	net->area[ia1].utility = 1;
	net->area[ia1].poly->RestoreArcs( &arc_array1 );
	net->area[ia1].poly->RestoreArcs( &arc_array2 );
	net->area[ia1].poly->Draw();
	gpc_free_polygon( union_gpc );
	delete union_gpc;
	return 1;
}

void CNetList::SetWidths( CNetWidthInfo const &width_attrib )
{
	m_def_width_attrib = width_attrib;

	// This is the top level - make sure there is no parent
	m_def_width_attrib.SetParent();

	//  Update all elements marked with "use default" (ie. "use parent")
	CNetWidthInfo update_use_parent_only;
	update_use_parent_only.Undef();

	CIterator_cnet net_iter(this);
	for( cnet * net = net_iter.GetFirst(); net != NULL; net = net_iter.GetNext() )
	{
		net->def_width_attrib.Update();
		SetNetWidth( net, update_use_parent_only );
	}
}


// get bounding rectangle for all net elements
//
BOOL CNetList::GetNetBoundaries( CRect * r )
{
	BOOL bValid = FALSE;
	CRect br;
	br.bottom = INT_MAX;
	br.left = INT_MAX;
	br.top = INT_MIN;
	br.right = INT_MIN;
	CIterator_cnet net_iter(this);
	for( cnet * net = net_iter.GetFirst(); net != NULL; net = net_iter.GetNext() )
	{
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			for( int iv=0; iv<net->connect[ic].vtx.GetSize(); iv++ )
			{
				cvertex * v = &net->connect[ic].vtx[iv];
				br.bottom = min( br.bottom, v->y - v->via_w() );
				br.top = max( br.top, v->y + v->via_w() );
				br.left = min( br.left, v->x - v->via_w() );
				br.right = max( br.right, v->x + v->via_w() );
				bValid = TRUE;
			}
		}
		for( int ia=0; ia<net->nareas; ia++ )
		{
			CRect r = net->area[ia].poly->GetBounds();
			br.bottom = min( br.bottom, r.bottom );
			br.top = max( br.top, r.top );
			br.left = min( br.left, r.left );
			br.right = max( br.right, r.right );
			bValid = TRUE;
		}
	}
	*r = br;
	return bValid;
}

// Remove all tee IDs from list
//
void CNetList::ClearTeeIDs()
{
	m_tee.RemoveAll();
}

// Find an ID and return array position or -1 if not found
//
int CNetList::FindTeeID( int id )
{
	for( int i=0; i<m_tee.GetSize(); i++ )
		if( m_tee[i] == id )
			return i;
	return -1;
}

// Assign a new ID and add to list
//
int CNetList::GetNewTeeID()
{
	int id;
	srand( (unsigned)time( NULL ) );
	do
	{
		id = rand();
	}while( id != 0 && FindTeeID(id) != -1 );
	m_tee.Add( id );
	return id;
}

// Remove an ID from the list
//
void CNetList::RemoveTeeID( int id )
{
	int i = FindTeeID( id );
	if( i >= 0 )
		m_tee.RemoveAt(i);
}

// Add tee_ID to list
//
void CNetList::AddTeeID( int id )
{
	if( id == 0 )
		return;
	if( FindTeeID( id ) == -1 )
		m_tee.Add( id );
}

// Find the main tee vertex for a tee_ID
//	return FALSE if not found
//	return TRUE if found, set ic and iv
//
BOOL CNetList::FindTeeVertexInNet( cnet * net, int id, int * ic, int * iv )
{
	for( int icc=0; icc<net->nconnects; icc++ )
	{
		cconnect * c = &net->connect[icc];
		for( int ivv=1; ivv<c->nsegs; ivv++ )
		{
			if( c->vtx[ivv].tee_ID == id )
			{
				if( ic )
					*ic = icc;
				if( iv )
					*iv = ivv;
				return TRUE;
			}
		}
	}
	return FALSE;
}


// find tee vertex in any net
//
BOOL CNetList::FindTeeVertex( int id, cnet ** net, int * ic, int * iv )
{
	CIterator_cnet net_iter(this);
	for( cnet * tnet = net_iter.GetFirst(); tnet != NULL; tnet = net_iter.GetNext() )
	{
		BOOL bFound = FindTeeVertexInNet( tnet, id, ic, iv );
		if( bFound )
		{
			*net = tnet;
			return TRUE;
		}
	}
	return FALSE;
}


// Check if stubs are still connected to tee, if not remove it
// Returns TRUE if tee still exists, FALSE if destroyed
//
BOOL CNetList::RemoveTeeIfNoBranches( cnet * net, int id )
{
	int n_stubs = 0;
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		if( c->vtx[c->nsegs].tee_ID == id )
			n_stubs++;
	}
	if( n_stubs == 0 )
	{
		// remove tee completely
		int tee_ic = RemoveTee( net, id );
		if( tee_ic >= 0 )
			MergeUnroutedSegments( net, tee_ic );
		return FALSE;
	}
	return TRUE;
}

// Disconnect branch from tee, remove tee if no more branches
// Returns TRUE if tee still exists, FALSE if destroyed
//
BOOL CNetList::DisconnectBranch( cnet * net, int ic )
{
	cconnect * c = &net->connect[ic];
	int id = c->vtx[c->nsegs].tee_ID;
	if( !id )
	{
		ASSERT(0);
		return FALSE;
	}
	else
	{
		c->vtx[c->nsegs].tee_ID = 0;
		ReconcileVia( net, ic, c->nsegs );
		int ic, iv;
		if( FindTeeVertexInNet( net, id, &ic, &iv ) )
		{
			ReconcileVia( net, ic, iv );
		}
		return RemoveTeeIfNoBranches( net, id );
	}
}

// Remove tee-vertex from net
// Don't change stubs connected to it
// return connection number of tee vertex or -1 if not found
//
int CNetList::RemoveTee( cnet * net, int id )
{
	int tee_ic = -1;
	for( int ic=net->nconnects-1; ic>=0; ic-- )
	{
		cconnect * c = &net->connect[ic];
		for( int iv=0; iv<=c->nsegs; iv++ )
		{
			cvertex * v = &c->vtx[iv];
			if( v->tee_ID == id )
			{
				if( iv < c->nsegs )
				{
					v->tee_ID = 0;
					ReconcileVia( net, ic, iv );
					tee_ic = ic;
				}
			}
		}
	}
	RemoveTeeID( id );
	return tee_ic;
}

// see if a tee vertex needs a via
//
BOOL CNetList::TeeViaNeeded( cnet * net, int id )
{
	int layer = 0;
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		for( int iv=1; iv<=c->nsegs; iv++ )
		{
			cvertex * v = &c->vtx[iv];
			if( v->tee_ID == id )
			{
				int seg_layer = c->seg[iv-1].layer;
				if( seg_layer >= LAY_TOP_COPPER )
				{
					if( layer == 0 )
						layer = seg_layer;
					else if( layer != seg_layer )
						return TRUE;
				}
				if( iv < c->nsegs )
				{
					seg_layer = c->seg[iv].layer;
					if( seg_layer >= LAY_TOP_COPPER )
					{
						if( layer == 0 )
							layer = seg_layer;
						else if( layer != seg_layer )
							return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}

// Finds branches without tees
// If possible, combines branches into a new trace
// If id == 0, check all ids for this net
// If bRemoveSegs, removes branches entirely instead of disconnecting them
// returns TRUE if corrections required, FALSE if not
// Note that the connect[] array may be changed by this function
//
BOOL CNetList::RemoveOrphanBranches( cnet * net, int id, BOOL bRemoveSegs )
{
	BOOL bFound = FALSE;
	BOOL bRemoved = TRUE;
	int test_id = id;

	if( test_id && FindTeeVertexInNet( net, test_id ) )
		return FALSE;	// not an orphan

	// check all connections
	while( bRemoved )
	{
		for( int ic=net->nconnects-1; ic>=0; ic-- )
		{
			cconnect * c = &net->connect[ic];
			c->utility = 0;
			BOOL bFixed = FALSE;
			int id = c->vtx[c->nsegs].tee_ID;
			if( c->end_pin == cconnect::NO_END && id != 0 )
			{
				// this is a branch
				if( test_id == 0 || id == test_id )
				{
					// find matching tee
					if( !FindTeeVertexInNet( net, id ) )
					{
						// no, this is an orphan
						bFound = TRUE;
						c->utility = 1;

						// now try to fix it by finding another branch
						for( int tic=ic+1; tic<net->nconnects; tic++ )
						{
							cconnect *tc = &net->connect[tic];
							if( tc->end_pin == cconnect::NO_END && tc->vtx[tc->nsegs].tee_ID == id )
							{
								// matching branch found, merge them
								// add ratline to start pin of branch
								AppendSegment( net, ic, tc->vtx[0].x, tc->vtx[0].y, LAY_RAT_LINE, CSegWidthInfo() );
								c->end_pin = tc->start_pin;
								c->vtx[c->nsegs].pad_layer = tc->vtx[0].pad_layer;
								for( int tis=tc->nsegs-1; tis>=0; tis-- )
								{
									if( tis > 0 )
									{
										int test = InsertSegment( net, ic, c->nsegs-1,
											tc->vtx[tis].x, tc->vtx[tis].y,
											tc->seg[tis].layer,
											tc->seg[tis].width_attrib,
											0
										);

										if( !test )
											ASSERT(0);

										c->vtx[c->nsegs-1] = tc->vtx[tis];
										tc->vtx[tis].tee_ID = 0;
									}
									else
									{
										RouteSegment( net, ic, c->nsegs-1,
											tc->seg[0].layer,
											tc->seg[tis].width_attrib
										);
									}
								}
								// add tee_ID back into tee array
								AddTeeID( id );
								// now delete the branch
								RemoveNetConnect( net, tic );
								bFixed = TRUE;
								break;
							}
						}
					}
				}
			}
			if( bFixed )
				c->utility = 0;
		}
		// now remove unfixed branches
		bRemoved = FALSE;
		for( int ic=net->nconnects-1; ic>=0; ic-- )
		{
			cconnect * c = &net->connect[ic];
			if( c->utility )
			{
				c->vtx[c->nsegs].tee_ID = 0;
				if( bRemoveSegs )
				{
					RemoveNetConnect( net, ic, FALSE );
					bRemoved = TRUE;
					test_id = 0;
				}
				else
					ReconcileVia( net, ic, c->nsegs );
			}
		}
	}
	// SetAreaConnections( net );
	return bFound;
}


#if 0 // [
void CNetList::ApplyClearancesToArea( cnet *net, int ia, int flags,
					int fill_clearance, int min_silkscreen_stroke_wid,
					int thermal_wid, int hole_clearance )
{
	//** testing only
	// find another area on a different net
	cnet * net2 = GetFirstNet();
	cnet * net3 = NULL;
	while( net2 )
	{
		if( net != net2 && net2->nareas > 0 )
		{
			net3 = net2;
		}
		net2 = GetNextNet();
	}
	if( net3 )
	{
		net->area[ia].poly->ClipPhpPolygon( A_AND_B, net3->area[0].poly );
	}
	return;

	//** end testing
	// get area layer
	int layer = net->area[ia].poly->GetLayer();
	net->area[ia].poly->Undraw();

	// iterate through all parts for pad clearances and thermals
	cpart * part = m_plist->m_start.next;
	while( part->next != 0 )
	{
		CShape * s = part->shape;
		if( s )
		{
			// iterate through all pins
			for( int ip=0; ip<s->GetNumPins(); ip++ )
			{
				// get pad info
				int pad_x;
				int pad_y;
				int pad_w;
				int pad_l;
				int pad_r;
				int pad_type;
				int pad_hole;
				int pad_connect;
				int pad_angle;
				cnet * pad_net;
				BOOL bPad = m_plist->GetPadDrawInfo( part, ip, layer, 0, 0, 0, 0,
					&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r, &pad_hole, &pad_angle,
					&pad_net, &pad_connect );

				if( bPad )
				{
					// pad or hole exists on this layer
					CPolyLine * pad_poly = NULL;
					if( pad_type == PAD_NONE && pad_hole > 0 )
					{
						net->area[ia].poly->AddContourForPadClearance( PAD_ROUND, pad_x, pad_y,
							pad_hole, pad_hole, pad_r, pad_angle, fill_clearance, pad_hole, hole_clearance );
					}
					else if( pad_type != PAD_NONE )
					{
						if( pad_connect & CPartList::AREA_CONNECT )
						{
							if( !(flags & GERBER_NO_PIN_THERMALS) )
							{
								// make thermal for pad
								net->area[ia].poly->AddContourForPadClearance( pad_type, pad_x, pad_y,
									pad_w, pad_l, pad_r, pad_angle, fill_clearance, pad_hole, hole_clearance,
									TRUE, thermal_wid );
							}
						}
						else
						{
							// make clearance for pad
							net->area[ia].poly->AddContourForPadClearance( pad_type, pad_x, pad_y,
								pad_w, pad_l, pad_r, pad_angle, fill_clearance, pad_hole, hole_clearance );
						}
					}

				}
			}
			part = part->next;
		}
	}

	// iterate through all nets and draw trace and via clearances
	CString name;
	cnet * t_net = GetFirstNet();
	while( t_net )
	{
		for( int ic=0; ic<t_net->nconnects; ic++ )
		{
			int nsegs = t_net->connect[ic].nsegs;
			cconnect * c = &t_net->connect[ic];
			for( int is=0; is<nsegs; is++ )
			{
				// get segment and vertices
				cseg * s = &c->seg[is];
				cvertex * pre_vtx = &c->vtx[is];
				cvertex * post_vtx = &c->vtx[is+1];
				double xi = pre_vtx->x;
				double yi = pre_vtx->y;
				double xf = post_vtx->x;
				double yf = post_vtx->y;
				double seg_angle = atan2( yf - yi, xf - xi );
				double w = (double)fill_clearance + (double)(s->width())/2.0;
				int test = GetViaConnectionStatus( t_net, ic, is+1, layer );
				// flash the via clearance if necessary
				if( post_vtx->viaExists() && layer >= LAY_TOP_COPPER )
				{
					// via exists and this is a copper layer
					if( layer > LAY_BOTTOM_COPPER && test == CNetList::VIA_NO_CONNECT )
					{
						// inner layer and no trace or thermal, just make hole clearance
						net->area[ia].poly->AddContourForPadClearance( PAD_ROUND, xf, yf,
							0, 0, 0, 0, 0, post_vtx->via_hole_w(), hole_clearance );
					}
					else if( !(test & VIA_AREA) )
					{
						// outer layer and no thermal, make pad clearance
						net->area[ia].poly->AddContourForPadClearance( PAD_ROUND, xf, yf,
							post_vtx->via_w, post_vtx->via_w, 0, 0, fill_clearance, post_vtx->via_hole_w, hole_clearance );
					}
					else if( layer > LAY_BOTTOM_COPPER && test & CNetList::VIA_AREA && !(test & CNetList::VIA_TRACE) )
					{
						// make small thermal
						if( flags & GERBER_NO_VIA_THERMALS )
						{
							// or not
						}
						else
						{
							// small thermal
							int w = post_vtx->via_hole_w + 2*m_annular_ring;
							net->area[ia].poly->AddContourForPadClearance( PAD_ROUND, post_vtx->x, post_vtx->y,
								w, w, 0, 0, fill_clearance, post_vtx->via_hole_w, hole_clearance,
								TRUE, thermal_wid );
						}
					}
					else if( test & CNetList::VIA_AREA )
					{
						// make normal thermal
						if( flags & GERBER_NO_VIA_THERMALS )
						{
							// no thermal
						}
						else
						{
							// thermal
							net->area[ia].poly->AddContourForPadClearance( PAD_ROUND, post_vtx->x, post_vtx->y,
								post_vtx->via_w, post_vtx->via_w, 0, 0, fill_clearance, post_vtx->via_hole_w, hole_clearance,
								TRUE, thermal_wid );
						}
					}
				}

				// make trace segment clearance
				if( t_net != net && s->layer == layer )
				{
					int npoints = 18;	// number of points in poly
					double x,y;
					// create points around beginning of segment
					double angle = seg_angle + PI/2.0;		// rotate 90 degrees ccw
					double angle_step = PI/(npoints/2-1);
					for( int i=0; i<npoints/2; i++ )
					{
						x = xi + w*cos(angle);
						y = yi + w*sin(angle);
						net->area[ia].poly->AppendCorner( x, y, CPolyLine::STRAIGHT, 0 );
						angle += angle_step;
					}
					// create points around end of segment
					angle = seg_angle - PI/2.0;
					for( int i=npoints/2; i<npoints; i++ )
					{
						x = xf + w*cos(angle);
						y = yf + w*sin(angle);
						net->area[ia].poly->AppendCorner( x, y, CPolyLine::STRAIGHT, 0 );
						angle += angle_step;
					}
					net->area[ia].poly->Close( CPolyLine::STRAIGHT );
				}
			}
		}
		t_net = GetNextNet();
	}

#if 0
			if( tl )
			{
				// draw clearances for text
				if( PASS1 )
				{
					f->WriteString( "\nG04 Draw clearances for text*\n" );
				}
				for( int it=0; it<tl->text_ptr.GetSize(); it++ )
				{
					CText * t = tl->text_ptr[it];
					if( t->m_layer == layer )
					{
						// draw text
						int w = t->m_stroke_width + 2*fill_clearance;
						CAperture text_ap( CAperture::AP_CIRCLE, w, 0 );
						ChangeAperture( &text_ap, &current_ap, &ap_array, PASS0, f );
						if( PASS1 )
						{
							for( int istroke=0; istroke<t->m_stroke.GetSize(); istroke++ )
							{
								::WriteMoveTo( f, t->m_stroke[istroke].xi, t->m_stroke[istroke].yi, LIGHT_OFF );
								::WriteMoveTo( f, t->m_stroke[istroke].xf, t->m_stroke[istroke].yf, LIGHT_ON );
							}
						}
					}
				}
			}
#endif
	// clip polygon, creating new areas if necessary
	ClipAreaPolygon( net, ia, FALSE, FALSE, FALSE );
}
#endif // ]

// recursive function for routing
//
// globals:
int g_score;					// score for current route
int g_num_steps;				// num paths in current route
int g_path_index[100];			// list of paths for current route
int g_path_end[100];			// list of path ends for current route
int g_best_score;				// best score found so far
int g_best_num_steps;			// num paths in best route
int g_best_path_index[100];		// list of paths for best route
int g_best_path_end[100];		// list of path ends for best route
//
int RouteToPin( int step,
				cnode * node,
				CArray<cnode> * nodes,
				CArray<cpath> * paths,
				CDlgLog * log )
{
	g_num_steps = step;			// number of steps so far
	int old_score = g_score;	// score so far
	if( step == 0 )
	{
		// if this is the first step, reset globals
		g_score = 0;
		g_best_score = -1;
		g_best_num_steps = 0;
	}
	else if( node->type == NPIN )
	{
		// SUCCESS, this node is a pin
		// if g_score > g_best_score, save this route
		if( g_score > g_best_score )
		{
			// first, restore used flags of previous best route
			if( g_best_score > -1 )
			{
				for( int i=0; i<g_best_num_steps; i++ )
				{
					cpath * path = &(*paths)[g_best_path_index[i]];
					path->n_used--;
				}
			}
			// now save new best route
			g_best_score = g_score;
			g_best_num_steps = g_num_steps;
			for( int i=0; i<step; i++ )
			{
				g_best_path_index[i] = g_path_index[i];
				g_best_path_end[i] = g_path_end[i];
				cpath * path = &(*paths)[g_path_index[i]];
				path->n_used++;
			}
		}
		return g_best_score;
	}

	// count available paths from this node
	int npaths = node->path_index.GetSize();
	int num_unused_paths = 0;
	for( int ip=0; ip<npaths; ip++ )
	{
		int path_index = node->path_index[ip];
		cpath * path = &(*paths)[path_index];
		if( !path->n_used )
			num_unused_paths++;
	}
	if( num_unused_paths == 0 )
	{
		// can't go on, but didn't reach pin
	}
	else
	{
		// increase score if this node has > 1 available path
		int new_score;
		if( num_unused_paths > 1 )
			new_score = old_score + 1;
		else
			new_score = old_score;
		// now try all available paths
		for( int ip=0; ip<npaths; ip++ )
		{
			int path_index = node->path_index[ip];
			int path_start = node->path_end[ip];	// start of path
			int path_end = 1 - node->path_end[ip];  // end of path
			cpath * path = &(*paths)[path_index];
			if( !path->n_used )
			{
				// try this path, get next node and continue routing
				int next_inode = path->GetInode( path_end );
				path->n_used++;
				cnode * next_node = &(*nodes)[next_inode];
				g_path_index[step] = path_index;
				g_path_end[step] = path_start;
				g_score = new_score;
				RouteToPin( step+1, next_node, nodes, paths, log );
				path->n_used--;
			}
		}
	}
	g_num_steps = step;		// revert num steps
	g_score = old_score;	// revert score
	return g_best_score;	// return best score found so far
}

// import routing data from autorouter for a net
//
void CNetList::ImportNetRouting( CString * name,
								CArray<cnode> * nodes,
								CArray<cpath> * paths,
								int tolerance,
								CDlgLog * log,
								BOOL bVerbose )
{
	CString mess;
	cnet * net = GetNetPtrByName( name );
	if( net == NULL )
	{
		ASSERT(0);
		return;
	}

	// unroute all traces
	for( int ic=net->nconnects-1; ic>=0; ic-- )
		RemoveNetConnect( net, ic, FALSE );
	SetAreaConnections( net );

	// add all pins in net to list of nodes
	for( int ip=0; ip<net->npins; ip++ )
	{
		cpin * net_pin = &net->pin[ip];
		CPoint p = m_plist->GetPinPoint( net_pin->part, net_pin->pin_name );
		int layer = m_plist->GetPinLayer( net_pin->part, net_pin->pin_name );
		int inode = nodes->GetSize();

		nodes->SetSize( inode+1 );
		cnode * node = &(*nodes)[inode];
		node->bUsed = FALSE;
		node->layer = layer;
		node->type = NPIN;
		node->x = p.x;
		node->y = p.y;
		node->pin_index = ip;
	}
	// now hook up paths and nodes
	for( int ipath=0; ipath<paths->GetSize(); ipath++ )
	{
		cpath * path = &(*paths)[ipath];
		for( int iend=0; iend<2; iend++ )
		{
			cpath_pt * pt = &path->pt[0];	// first point in path
			if( iend == 1 )
				pt = &path->pt[path->pt.GetSize()-1];	// last point
			// search all nodes for match
			BOOL bMatch = FALSE;
			int inode;
			for( inode=0; inode<nodes->GetSize(); inode++ )
			{
				cnode * node = &(*nodes)[inode];
				if( abs(pt->x-node->x)<tolerance && abs(pt->y-node->y)<tolerance
					&& ( path->layer == node->layer || node->layer == LAY_PAD_THRU ) )
				{
					// match, hook it up
					int ip = node->path_index.GetSize();
					node->path_index.SetSize(ip+1);
					node->path_end.SetSize(ip+1);
					node->path_index[ip] = ipath;
					node->path_end[ip] = iend;
					pt->inode = inode;
					pt->x = node->x;
					pt->y = node->y;
					bMatch = TRUE;
					break;
				}
			}
			if( !bMatch )
			{
				// node not found, make new junction node
				inode = nodes->GetSize();
				nodes->SetSize(inode+1);
				cnode * node = &(*nodes)[inode];
				node->bUsed = FALSE;
				node->layer = path->layer;
				node->type = NJUNCTION;
				node->x = pt->x;
				node->y = pt->y;
				int ip = node->path_index.GetSize();
				node->path_index.SetSize(ip+1);
				node->path_end.SetSize(ip+1);
				node->path_index[ip] = ipath;
				node->path_end[ip] = iend;
				pt->inode = inode;
			}
		}
	}
	// dump data
	if( log )
	{
		mess.Format( "\r\nrouting net \"%s\"\r\n", *name );
		log->AddLine( mess );
		if( bVerbose )
		{
			mess.Format( "num nodes = %d\r\n", nodes->GetSize() );
			log->AddLine( mess );
			for( int inode=0; inode<nodes->GetSize(); inode++ )
			{
				cnode * node = &(*nodes)[inode];
				CString type_str = "none";
				if( node->type == NPIN )
					type_str = "pin";
				else if( node->type == NVIA )
					type_str = "via";
				else if( node->type == NJUNCTION )
					type_str = "junction";
				mess.Format( "  node %d: %s x=%d y=%d layer=%d npaths=%d\r\n",
					inode, type_str, node->x/NM_PER_MIL, node->y/NM_PER_MIL, node->layer, node->path_index.GetSize() );
				log->AddLine( mess );
			}
		}
	}
	// start routing
	for( int ipass=0; ipass<3; ipass++ )
	{
		// if ipass == 0, route stubs
		// if ipass == 1, route pin-pin traces
		// if ipass == 2, route branches
		for( int inode=0; inode<nodes->GetSize(); inode++ )
		{
			cnode * node = &(*nodes)[inode];
			if( ( ipass == 0 && node->type != NPIN && node->path_index.GetSize() == 1 )
				|| ( ipass == 1 && node->type == NPIN )
				|| ( ipass == 2 && node->type != NPIN && node->path_index.GetSize() > 2 ) )
			{
				int num_unused_paths = 0;
				int npaths = node->path_index.GetSize();
				for( int ip=0; ip<npaths; ip++ )
				{
					int path_index = node->path_index[ip];
					cpath * path = &(*paths)[path_index];
					if( !path->n_used )
						num_unused_paths++;
				}
				BOOL bFailed = (num_unused_paths == 0);		// fails if no paths
				while( !bFailed )
				{
					// route trace
					int score = RouteToPin( 0, node, nodes, paths, NULL );
					if( score == -1 )
						bFailed = TRUE;
					if( !bFailed )
					{
						// add routed trace to project
						if( ipass == 0 )
							mess = "stub: ";
						else if( ipass == 1 )
							mess = "pin-pin: ";
						else if( ipass == 2 )
							mess = "branch: ";
						CString str;
						// create new connection
						int ic = -1;
						cconnect * c = NULL;
						int is = 0;
						for( int istep=g_best_num_steps-1; istep>=0; istep-- )
						{
							// iterate backward through steps, so we always start on pin
							int path_index = g_best_path_index[istep];
							int path_end = g_best_path_end[istep];
							cpath * path = &(*paths)[path_index];
							int next_inode = path->GetInode( path_end );
							int prev_inode = path->GetInode( 1-path_end );
							cnode * prev_node = &(*nodes)[prev_inode];
							cnode * next_node = &(*nodes)[next_inode];
							if( istep == g_best_num_steps-1 )
							{
								// first step, create connection
								if( ipass == 0 || ipass == 2 )
									ic = AddNetStub( net, prev_node->pin_index );
								else if( ipass == 1 )
								{
									int last_path_index = g_best_path_index[0];
									int last_path_end = g_best_path_end[0];
									cpath * last_path = &(*paths)[last_path_index];
									int last_inode = last_path->GetInode( last_path_end );
									cnode * last_node = &(*nodes)[last_inode];
									ic = AddNetConnect( net, prev_node->pin_index, last_node->pin_index );
								}
								str.Format( "n%d", prev_inode );
								mess += str;
							}
							if( ic > -1 )
								c = &net->connect[ic];
							else
								ASSERT(0);
							// iterate through points in path
							int n_pts = path->pt.GetSize()-1;
							for( int ipt=0; ipt<n_pts; ipt++ )
							{
								int next_pt = ipt+1;
								if( path_end == 0 )
									next_pt = n_pts - ipt - 1;
								int x = path->pt[next_pt].x;
								int y = path->pt[next_pt].y;
								int layer = path->layer;
								int width = path->width;

								if( ipass == 0 || ipass == 2 )
									AppendSegment( net, ic, x, y, layer, CSegWidthInfo(width) );
								else if( ipass == 1 )
									InsertSegment( net, ic, is, x, y, layer, CConnectionWidthInfo( width, CInheritableInfo::E_USE_PARENT, CInheritableInfo::E_USE_PARENT ), 0 );

								is++;
							}
							// force all vias
							if( next_node->type == NVIA )
							{
								// use via width from node and default hole width
								c->vtx[is].via_width_attrib.m_via_width = next_node->via_w;
								c->vtx[is].via_width_attrib.Update();

								ForceVia( net, ic, is, FALSE );
							}
							str.Format( "-n%d", next_inode );
							mess += str;
						}
						if( ipass == 2 )
						{
							int tee_id = GetNewTeeID();
							cconnect * c = &net->connect[ic];
							int end_v = c->nsegs;
							cvertex * v = &c->vtx[end_v];
							v->tee_ID = tee_id;
						}
						mess += "\r\n";
						if( log && bVerbose )
							log->AddLine( mess );
						// at this point, all imported vias are forced
						// unforce the ones between segments on different layers that don't
						// connect to copper areas
						for( int iv=1; iv<c->nsegs; iv++ )
						{
							cvertex * v = &c->vtx[iv];
							cseg * pre_seg = &c->seg[iv-1];
							cseg * post_seg = &c->seg[iv];
							if( pre_seg->layer != post_seg->layer && !TestPointInArea( net, v->x, v->y, LAY_PAD_THRU, NULL ) )
								UnforceVia( net, ic, iv, FALSE );
						}

					}
				}
			}
		}
	}
	// now hook up branches to traces
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		if( c->end_pin == cconnect::NO_END )
		{
			cvertex * end_v = &c->vtx[c->nsegs];
			cseg * end_seg = &c->seg[c->nsegs-1];
			int tee_id = end_v->tee_ID;
			if( tee_id )
			{
				//  find trace with corresponding vertex
				for( int icc=0; icc<net->nconnects; icc++ )
				{
					cconnect * trace_c = &net->connect[icc];
					for( int iv=1; iv<trace_c->nsegs; iv++ )
					{
						cvertex * trace_v = &trace_c->vtx[iv];
						cseg * trace_seg = &trace_c->seg[iv-1];
						if( trace_v->x == end_v->x && trace_v->y == end_v->y
							&& ( trace_v->via_w() || end_v->via_w()
							|| end_seg->layer == trace_seg->layer ) )
						{
							// make a tee-vertex and connect branch
							if( trace_v->tee_ID )
								tee_id = trace_v->tee_ID;

							trace_v->tee_ID = tee_id;
							end_v->tee_ID = tee_id;

							end_v->force_via_flag = FALSE;
							end_v->SetNoVia();

							ReconcileVia( net, icc, iv );
						}
					}
				}
			}
		}
	}
	// test for unrouted paths
	BOOL bFailed = FALSE;
	for( int ipath=0; ipath<paths->GetSize(); ipath++ )
	{
		cpath * path = &(*paths)[ipath];
		if( path->n_used == 0 )
		{
			if( log )
			{
				CString str;
				mess.Format( "error: path %d failed to route", ipath );
				cnode * node = &(*nodes)[path->GetInode(0)];

				CString type_str = "pin";
				if( node->type == NVIA )
					type_str = "via";
				else if( node->type == NJUNCTION )
					type_str = "junction";

				cnode * node_end = &(*nodes)[path->GetInode(1)];
				str.Format( ", %s at x=%d y=%d",
					type_str, node->x/NM_PER_MIL, node->y/NM_PER_MIL );
				mess += str;
				node = &(*nodes)[path->GetInode(1)];

				type_str = "pin";
				if( node->type == NVIA )
					type_str = "via";
				else if( node->type == NJUNCTION )
					type_str = "junction";

				str.Format( " to %s at x=%d y=%d, layer %d\r\n",
					type_str, node->x/NM_PER_MIL, node->y/NM_PER_MIL,
					path->layer );
				mess += str;

				log->AddLine( mess );
			}
			bFailed = TRUE;
		}
	}
	if( !bFailed && log )
		log->AddLine( "success: all paths routed\r\n" );
	SetAreaConnections( net );
}
