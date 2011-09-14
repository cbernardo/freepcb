// Net.cpp: implementation of the following classes:
//	cpin - a pin in a net (eg. U1.6)
//	cconnect - a connection in a net, usually a trace between pins
//	cseg - a segment of a connection, either copper or a ratline
//	cvertex - a vertex in a connection (ie. a pin or a point between segments)
//	carea - a copper area in a net
//

#include "stdafx.h"
#include <math.h>
#include <stdlib.h>
#include "DlgMyMessageBox.h"
#include "gerber.h"
#include "utility.h"
#include "php_polygon.h"
#include "Cuid.h"
#include "NetList.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

extern Cuid pcb_cuid;

//********************** cpin implementation ****************************

// default constructor, should always be followed by Initialize()
cpin::cpin()
{
//**	m_uid = -1;
	part = NULL; 
	m_net = NULL;
}

cpin::~cpin()
{
//**	pcb_cuid.ReleaseUID( m_uid );
}

void cpin::Initialize( cnet * net )
{
//**	m_uid = pcb_cuid.GetNewUID();
	m_net = net;
}


//*********************** CVertex implementation ************************
CVertex::CVertex( cnet * net )
{
//		m_uid = pcb_cuid.GetNewUID();
	m_net = net;
	m_dlist = net->m_dlist;
}

CVertex::~CVertex()
{
//		pcb_cuid.ReleaseUID( m_uid );
}

//*********************** cconnect implementation ***********************

// constructor
cconnect::cconnect( cnet* net )
{ 
	m_uid = pcb_cuid.GetNewUID();
	m_net = net;
	m_dlist = net->m_dlist;
	start_pin = end_pin = -1;
	locked = 0;
	utility = 0;
	seg.SetSize( 0 );
	vtx.SetSize( 0 );
}

// destructor
cconnect::~cconnect()
{
	pcb_cuid.ReleaseUID( m_uid );
	vtx.RemoveAll();
	seg.RemoveAll();
}

// replace the UID with a different one
// if uid = -1, get a new one, otherwise use uid
// release the old one and request uid if needed
void cconnect::ReplaceUID( int uid )
{
	pcb_cuid.ReleaseUID( m_uid );
	if( uid < 0 )
	{
		m_uid = pcb_cuid.GetNewUID();
	}
	else
	{
		if( pcb_cuid.CheckUID( uid ) )
		{
			pcb_cuid.RequestUID( uid );
		}
		m_uid = uid;
	}
}

// clear segment and vertex arrays
void cconnect::ClearArrays()
{ 
	seg.RemoveAll(); 
	vtx.SetSize(0); 
}


// return number of segments in connection
int cconnect::NumSegs()
{ 
	return seg.GetSize(); 
}

// set sizes of segment and vertex arrays
void cconnect::SetNumSegs( int n )
{ 
	seg.SetSize(n); 
	vtx.SetSize(n+1); 
}

// get start pin, or NULL if doesn't start on a pin
cpin * cconnect::StartPin()
{
	if( start_pin < 0 )
		return NULL;
	else
		return &m_net->pin[start_pin];
}

// get end pin, or NULL if doesn't end on a pin
cpin * cconnect::EndPin()
{
	if( end_pin < 0 )
		return NULL;
	else
		return &m_net->pin[end_pin];
}

// return pointer to first vertex in connection
cvertex * cconnect::FirstVtx()
{ 
	if( vtx.GetSize() == 0 )
		return NULL;
	else
		return &vtx[0]; 
}

// return pointer to last vertex in connection
cvertex * cconnect::LastVtx()
{ 
	int nv = vtx.GetSize();
	if( nv == 0 )
		return NULL;
	else
		return &vtx[nv-1]; 
}

// return reference to seg[is]
cseg& cconnect::SegByIndex( int is )
{ 
	return seg[is]; 
}

cseg * cconnect::SegByUID( int uid, int * index )
{
	CIterator_cseg iter_seg( this );
	for( cseg * s=iter_seg.GetFirst(); s; s=iter_seg.GetNext() )
	{
		if( s->m_uid == uid )
		{
			if( index )
				*index = iter_seg.GetIndex();
			return s;
		}
	}
	if( index )
		*index = -1;
	return NULL;
}

int cconnect::SegIndexByPtr( cseg * seg )
{
	CIterator_cseg iter_seg( this );
	for( cseg * s=iter_seg.GetFirst(); s; s=iter_seg.GetNext() )
	{
		if( s == seg )
			return iter_seg.GetIndex();
	}
	return -1;
}

int cconnect::SegUIDByPtr( cseg * s )
{
	return s->m_uid;
}


// return reference to vtx[iv]
cvertex& cconnect::VtxByIndex( int iv )
{ 
	return vtx[iv]; 
}

// return vertex with given uid, can also return index
cvertex * cconnect::VtxByUID( int uid, int * index )
{
	CIterator_cvertex iter_vtx( this );
	for( cvertex * v=iter_vtx.GetFirst(); v; v=iter_vtx.GetNext() )
	{
		if( v->m_uid == uid )
		{
			if( index )
				*index = iter_vtx.GetIndex();
			return v;
		}
	}
	if( index )
		*index = -1;
	return NULL;
}

int cconnect::VtxIndexByPtr( cvertex * vtx )
{
	CIterator_cvertex iter_vtx( this );
	for( cvertex * v=iter_vtx.GetFirst(); v; v=iter_vtx.GetNext() )
	{
		if( v == vtx )
			return iter_vtx.GetIndex();
	}
	return NULL;
}

int cconnect::VtxUIDByPtr( cvertex * v )
{
	return v->m_uid;
}


// insert vertex into connection
//
cvertex& cconnect::InsertVertexByIndex( int iv, const cvertex& new_vtx )
{
	vtx.InsertAt(iv, new_vtx );
	cvertex * v = &VtxByIndex( iv );
	v->Initialize(this);
	return *v;
}

// insert a new segment and new vertex into connection
// new vertex is at vtx[is+1], dividing old seg[is] into seg[is] and seg[is+1]
// parameters are copied from new_seg and new_vtx
// if dir = 0, params are copied from new_seg to seg[is+1]
// if dir = 1, params are copied from seg[is] to seg[is+1], and new_seg to seg[is], and seg[is]->seg[is+1]
//
void cconnect::InsertSegAndVtxByIndex(int is, int dir, 
					const cseg& new_seg, const cvertex& new_vtx )
{
	cseg old_seg;
	pcb_cuid.ReleaseUID( old_seg.UID() );
	if( NumSegs() == 0 )
		ASSERT(0);
	old_seg = seg[is];		// copy old seg[is];

	if( dir == 0 )
	{
		// route forward
		seg.InsertAt(is, new_seg );		// insert segment with new params
		SegByIndex(is).Initialize(this);	// and new uid
	}
	else
	{
		// route backward
		seg.InsertAt(is, old_seg );	// insert segment with old params
		seg[is].ReplaceUID( old_seg.m_uid );	
		seg[is+1] = new_seg;	// assign new params to next segment
		SegByIndex(is+1).Initialize(this);
	}
	vtx.InsertAt(is+1, new_vtx );
	VtxByIndex(is+1).Initialize(this);
	old_seg.m_uid = -1;			// so UID won't be released

}

// append new seg and vertex to end of connection (used for stubs)
//
void cconnect::AppendSegAndVertex( const cseg& new_seg, 
									  const cvertex& new_vtx )
{
	seg.Add( new_seg );
	vtx.Add( new_vtx );
	int ns = NumSegs();
	SegByIndex( ns-1 ).Initialize(this);
	VtxByIndex( ns ).Initialize(this);
}

// insert new starting vertex and segment into connection
//
void cconnect::PrependVertexAndSeg( const cvertex& new_vtx,
							const cseg& new_seg )
{
	vtx.InsertAt( 0, new_vtx );
	seg.InsertAt( 0, new_seg );
	SegByIndex( 0 ).Initialize(this);
	VtxByIndex( 0 ).Initialize(this);
}

void cconnect::RemoveSegAndVertexByIndex( int is )
{
	cvertex * old_v = &vtx[is+1];
	if( old_v->GetType() == cvertex::V_PIN )
		end_pin = NO_END;
	vtx.RemoveAt( is+1 );
	seg.RemoveAt( is );
}

void cconnect::Undraw()
{
	if( m_dlist )
	{
		CIterator_cseg iter_seg(  this );
		for( cseg * s=iter_seg.GetFirst(); s; s=iter_seg.GetNext() )
		{
			if( s->dl_el )
				m_dlist->Remove( s->dl_el );
			if( s->dl_sel )
				m_dlist->Remove( s->dl_sel );
			s->dl_el = NULL;
			s->dl_sel = NULL;
		}
		CIterator_cvertex iter_vtx( this );
		for( cvertex * v=iter_vtx.GetFirst(); v; v=iter_vtx.GetNext() )
		{
			v->Undraw();
		}
	}
}

void cconnect::Draw()
{
	if( m_dlist )
	{
		Undraw();
		int ic = m_net->ConIndexByPtr( this );
		CIterator_cseg iter_seg( this );
		for( cseg * s=iter_seg.GetFirst(); s; s=iter_seg.GetNext() )
		{
			int is = iter_seg.GetIndex();
			cvertex *pre_v = &vtx[is];
			cvertex *post_v = &vtx[is+1];
			id s_id = m_net->m_id;
			s_id.SetT2( ID_CONNECT );
			s_id.SetU2( m_uid );
			s_id.SetI2( ic );
			s_id.SetT3( ID_SEG );
			s_id.SetU3( s->m_uid );
			s_id.SetI3( is );
			int v = 1;
			if( s->layer == LAY_RAT_LINE )
				v = m_net->visible;
			int shape = DL_LINE;
			if( s->curve == cseg::CW )
				shape = DL_CURVE_CW;
			else if( s->curve == cseg::CCW )
				shape = DL_CURVE_CCW;
 			s->dl_el = m_dlist->Add( s_id, m_net, s->layer, shape, v, 
				s->width, 0, 0, pre_v->x, pre_v->y, post_v->x, post_v->y,
				0, 0 );
			s_id.SetT3( ID_SEL_SEG );
			s->dl_sel = m_dlist->AddSelector( s_id, m_net, s->layer, DL_LINE, v, 
				s->width, 0, pre_v->x, pre_v->y, post_v->x, post_v->y,
				0, 0 );
		}
		int nvtx;
		if( end_pin == cconnect::NO_END )
			nvtx = NumSegs() + 1;
		else
			nvtx = NumSegs();
		for( int iv=1; iv<nvtx; iv++ )
			m_net->m_nlist->ReconcileVia( m_net, ic, iv );
		// if tee stub, reconcile via of tee vertex
		if( end_pin == cconnect::NO_END )
		{
			if( int id = vtx[NumSegs()].tee_ID )
			{
				int tee_ic;
				int tee_iv;
				BOOL bFound = m_net->m_nlist->FindTeeVertexInNet( m_net, id, &tee_ic, &tee_iv );
				if( bFound )
					m_net->m_nlist->ReconcileVia( m_net, tee_ic, tee_iv );
			}
		}
	}
}


//**************************** cseg implementation ************************

// normal constructor
cseg::cseg()
{
	m_uid = pcb_cuid.GetNewUID();
	curve = STRAIGHT;
	layer = 0;
	width = 0;
	selected = 0;
	dl_el = 0;
	dl_sel = 0;
	utility = 0;
	// these may be filled in after construction with Initialize()
	m_con = 0;
	m_dlist = 0;  
	m_net = 0;
}

// copy constructor
// don't copy UID or display elements
cseg::cseg( cseg& src )
{
	m_uid = src.m_uid;
	layer = src.layer;
	width = src.width;
	curve = src.curve;
	selected = src.selected;
	utility = src.utility;
	dl_el = NULL;
	dl_sel = NULL;
	m_dlist = src.m_dlist;
	m_net = src.m_net;
	m_con = src.m_con;
}

// destructor
cseg::~cseg()
{
	pcb_cuid.ReleaseUID( m_uid );
	if( m_dlist )
	{
		if( dl_el )
			m_dlist->Remove( dl_el );
		if( dl_sel )
			m_dlist->Remove( dl_sel );
	}
}

// assignment from const rhs (needed by CArray::InsertAt)
// copy all variables except m_uid
cseg& cseg::operator=( const cseg& rhs )
{
	if( this != &rhs )
	{
		layer = rhs.layer;
		width = rhs.width;
		curve = rhs.curve;
		selected = rhs.selected;
		dl_el = rhs.dl_el;		
		dl_sel = rhs.dl_sel;
		m_dlist = rhs.m_dlist;
		m_net = rhs.m_net;
		m_con = rhs.m_con;
		utility = rhs.utility;
	}
	return *this;
}

// assignment
cseg& cseg::operator=( cseg& rhs )
{
	if( this != &rhs )
	{
		m_uid = rhs.m_uid;
		layer = rhs.layer;
		width = rhs.width;
		curve = rhs.curve;
		selected = rhs.selected;
		dl_el = rhs.dl_el;		
		dl_sel = rhs.dl_sel;
		m_dlist = rhs.m_dlist;
		m_net = rhs.m_net;
		m_con = rhs.m_con;
		utility = rhs.utility;
	}
	return *this;
}

// set up pointers and UID
void cseg::Initialize( cconnect * c )
{
	m_con = c;
	m_net = c->m_net;
	m_dlist = m_net->m_dlist;
}

// replace the UID with a different one
// release the old UID
// if uid = -1, get a new one
// otherwise, replace with uid provided
// request assignment of uid if necessary
void cseg::ReplaceUID( int uid )
{
	m_uid = pcb_cuid.PrepareToReplaceUID( m_uid, uid );
}

// get index of this segment in cconnect
int cseg::GetIndex()
{
	CIterator_cseg iter_seg( m_con );
	for( cseg * s=iter_seg.GetFirst(); s; s=iter_seg.GetNext() )
	{
		if( s == this )
			return iter_seg.GetIndex();
	}
	return -1;
}

// get vertex preceding this segment
cvertex& cseg::GetPreVtx()
{
	int is = GetIndex();
	return m_con->VtxByIndex(is);
}

// get vertex following this segment
cvertex& cseg::GetPostVtx()
{
	int is = GetIndex();
	return m_con->VtxByIndex(is+1);
}


//**************************** cvertex implementation *********************
cvertex::cvertex()
{
	// constructor
	m_uid = pcb_cuid.GetNewUID();
	m_dlist = 0;	// this must set with Initialize()
	m_con = 0;		// this must set with Initialize()
	m_net = 0;		// this must set with Initialize()
	x = 0; y = 0;
	pad_layer = 0;			// only for first or last 
	force_via_flag = 0;		// only used for end of stub trace
	via_w = 0; 
	via_hole_w = 0;
	dl_sel = 0;
	dl_hole = 0;
	tee_ID = 0;
	utility = 0;
	utility2 = 0;
}

// copy constructor
// don't copy UID or display elements
cvertex::cvertex( cvertex& v )
{
	// constructor
	m_uid = pcb_cuid.GetNewUID();
	x = v.x;
	y = v.y;
	pad_layer = v.pad_layer;
	force_via_flag = v.force_via_flag;
	via_w = v.via_w;
	via_hole_w = v.via_hole_w;
	tee_ID = v.tee_ID;
	utility = v.utility;
	utility2 = v.utility2;
	m_dlist = v.m_dlist;
	m_con = v.m_con;	
	m_net = v.m_net;	
}

cvertex::~cvertex()
{
	// destructor
	pcb_cuid.ReleaseUID( m_uid );
	if( m_dlist )
	{
		for( int il=0; il<dl_el.GetSize(); il++ )
			m_dlist->Remove( dl_el[il] );
		if( dl_sel )
			m_dlist->Remove( dl_sel );
		if( dl_hole )
			m_dlist->Remove( dl_hole );
	}
}

// assignment from const
// don't copy UID or display elements
cvertex & cvertex::operator=( const cvertex &v )	
{
	x = v.x;
	y = v.y;
	pad_layer = v.pad_layer;
	force_via_flag = v.force_via_flag;
	via_w = v.via_w;
	via_hole_w = v.via_hole_w;
	tee_ID = v.tee_ID;
	utility = v.utility;
	utility2 = v.utility2;
	dl_hole = NULL;
	dl_sel = NULL; 
	dl_el.SetSize(0);
	return *this;
};

// assignment from const
// don't copy UID or display elements
cvertex & cvertex::operator=( cvertex &v )	
{
	x = v.x;
	y = v.y;
	pad_layer = v.pad_layer;
	force_via_flag = v.force_via_flag;
	via_w = v.via_w;
	via_hole_w = v.via_hole_w;
	tee_ID = v.tee_ID;
	utility = v.utility;
	utility2 = v.utility2;
	dl_hole = NULL;
	dl_sel = NULL; 
	dl_el.SetSize(0);
	return *this;
};

void cvertex::Initialize( cconnect * c )
{
	m_con = c;
	m_net = c->m_net;
	m_dlist = m_net->m_dlist;
}

// replace the old UID with a different one
// if the old UID is valid, release it
// if uid = -1, get a new one, otherwise use uid
//
void cvertex::ReplaceUID( int uid )
{
	m_uid = pcb_cuid.PrepareToReplaceUID( m_uid, uid );
}

void cvertex::Undraw()
{
	if( dl_el.GetSize() )
	{
		for( int i=0; i<dl_el.GetSize(); i++ )
		{
			m_dlist->Remove( dl_el[i] );
		}
		dl_el.RemoveAll();
	}
	m_dlist->Remove( dl_sel );
	m_dlist->Remove( dl_hole );
	dl_sel = NULL;
	dl_hole = NULL;
}

// return type of vertex
cvertex::Type cvertex::GetType()
{
	if( pad_layer != 0 )
		return V_PIN;
	else if( tee_ID > 0 )
		return V_TEE;
	else if( this == m_con->LastVtx() )
		return V_END;
	else if( this == m_con->FirstVtx() )
		return V_END;
	else
		return V_ERR;
}

// if vertex is a pin, return it
cpin * cvertex::NetPin()
{
	if( GetType() == V_PIN )
	{
		if( this == m_con->FirstVtx() )
			return m_con->StartPin();
		else if( this == m_con->LastVtx() )
			return m_con->EndPin();
	}
	return NULL;
}

// if vertex is a pin, return index to it
int cvertex::NetPinIndex()
{
	if( GetType() == V_PIN )
	{
		if( this == m_con->FirstVtx() )
			return m_con->start_pin;
		else if( this == m_con->LastVtx() )
			return m_con->end_pin;
	}
	return NULL;
}



//***************************** carea implementation **********************

// default constructor, must be followed by call to Initialize()
carea::carea()
{
	m_net = NULL;
	m_id.Clear();
	m_dlist = 0;
	npins = 0;
	nvias = 0;
	utility = 0;
	utility = 0;
	utility2 = 0;
}

// carea copy constructor 
// doesn't actually copy but required for CArray<carea,carea>.InsertAt()
carea::carea( const carea& s )
{
	m_net = NULL;
	m_id.Clear();
	npins = 0;
	nvias = 0;
	npins = 0;
	SetDlist( m_dlist );
}

carea::~carea()
{
	if( m_dlist )
	{
		for( int ip=0; ip<npins; ip++ )
			m_dlist->Remove( dl_thermal[ip] );
		for( int is=0; is<nvias; is++ )
			m_dlist->Remove( dl_via_thermal[is] );
	}
}

// carea assignment operator
// doesn't actually assign but required for CArray<carea,carea>.InsertAt()
carea &carea::operator=( carea &a )
{
	return *this;
}

// carea assignment operator
// doesn't actually assign but required for CArray<carea,carea>.InsertAt()
carea &carea::operator=( const carea &a )
{
	return *this;
}


void carea::Initialize( CDisplayList * dlist, cnet * net )
{
	m_dlist = dlist;
	m_net = net;
	m_id = net->m_id;			// level 1 of id is from net
	m_id.SetT2( ID_AREA );		// level 2 is from area
	m_id.SetU2( m_uid );
	SetDlist( dlist );
	SetId( &m_id );
}

//************************* CNet implementation ***************************

cnet::cnet( CDisplayList * dlist, CNetList * nlist )
{ 
	m_dlist = dlist;
	m_nlist = nlist;
	m_id.Clear();
	m_uid = pcb_cuid.GetNewUID();
	m_id.SetT1( ID_NET );
	m_id.SetT2( ID_NONE );
	m_id.SetU1(m_uid);
}
cnet::~cnet()
{
	CIterator_cconnect iter_con( this );
	for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
	{
		delete c;
	}
	connect.RemoveAll();
	area.RemoveAll();
	pin.RemoveAll();
	pcb_cuid.ReleaseUID( m_uid );
}

// return size of arrays
int cnet::NumCons(){ return connect.GetSize(); };
int cnet::NumPins(){ return pin.GetSize(); };
int cnet::NumAreas(){ return area.GetSize(); };

// methods for pins
void cnet::AddPin( CString * ref_des, CString * pin_name, BOOL set_areas )
{
	// set size of pin array
	int old_size = pin.GetSize();
	pin.SetSize(old_size + 1 );

	// add pin to array
	pin[old_size].Initialize( this );
	pin[old_size].ref_des = *ref_des;
	pin[old_size].pin_name = *pin_name;
	pin[old_size].part = NULL;

	// now lookup part and hook to net if successful
	cpart * part = m_nlist->m_plist->GetPart( *ref_des );
	if( part )
	{
		// hook part to net
		pin[old_size].part = part;
		if( part->shape )
		{
			int pin_index = part->shape->GetPinIndexByName( *pin_name );
			if( pin_index >= 0 )
			{
				// hook net to part
				part->pin[pin_index].net = this;
			}
		}
	}

	// adjust connections to areas
	if( NumAreas() && set_areas )
		m_nlist->SetAreaConnections( this );
}

cpin * cnet::PinByIndex( int ip )
{
	if( ip < 0 || ip >= NumPins() )
		return NULL;
	else
		return &pin[ip];
}

#if 0
cpin * cnet::PinByUID( int uid )
{
	CIterator_cpin iter_pin( this );
	for( cpin * p=iter_pin.GetFirst(); p; p=iter_pin.GetNext() )
	{
		if( p->m_uid == uid )
			return p;
	}
	return NULL;
}

void cnet::RemovePinByUID( int uid, BOOL bSetAreas )
{
	CIterator_cpin iter_pin( this );
	for( cpin * p=iter_pin.GetFirst(); p; p=iter_pin.GetNext() )
	{
		if( p->m_uid == uid )
		{
			iter_pin.OnRemove( p );
			RemovePin( iter_pin.GetIndex(), bSetAreas );
			// adjust connections to areas
			if( NumAreas() && bSetAreas )
				m_nlist->SetAreaConnections( this );
			return;
		}
	}
	ASSERT(0);
}
#endif


void cnet::RemovePin( CString * ref_des, CString * pin_name, BOOL bSetAreas )
{
	// find pin in pin list for net
	int net_pin = -1;
	for( int ip=0; ip<NumPins(); ip++ )
	{
		if( pin[ip].ref_des == *ref_des && pin[ip].pin_name == *pin_name )
		{
			net_pin = ip;
			break;
		}
	}
	if( net_pin == -1 )
	{
		// pin not found
		ASSERT(0);
	}
	RemovePin( net_pin, bSetAreas );
}

void cnet::RemovePin( int net_pin_index, BOOL bSetAreas )
{
	// now remove all connections to/from this pin
	int ic = 0;
	while( ic<NumCons() )
	{
		cconnect * c = connect[ic];
		if( c->start_pin == net_pin_index || c->end_pin == net_pin_index )
			m_nlist->RemoveNetConnect( this, ic, FALSE );
		else
			ic++;
	}
	// now remove link to net from part pin (if it exists)
	cpart * part = pin[net_pin_index].part;
	if( part )
	{
		if( part->shape )
		{
			int part_pin_index = part->shape->GetPinIndexByName( pin[net_pin_index].pin_name );
			if( part_pin_index != -1 )
				part->pin[part_pin_index].net = 0;
		}
	}
	// now remove pin from net
	pin.RemoveAt(net_pin_index);
	// now adjust pin numbers in remaining connections
	for( ic=0; ic<NumCons(); ic++ )
	{
		cconnect * c = connect[ic];
		if( c->start_pin > net_pin_index )
			c->start_pin--;
		if( c->end_pin > net_pin_index )
			c->end_pin--;
	}
	// adjust connections to areas
	if( NumAreas() && bSetAreas )
		m_nlist->SetAreaConnections( this );
}

// add new connection with no segments or vertices
//
cconnect * cnet::AddConnect()
{
	cconnect  * c = new cconnect( this );
	connect.Add( c );	
	return c;
}

// add connection consisting of one vertex at the starting pin
//
int cnet::AddConnectFromPin( int p1 )
{
	if( pin[p1].part == 0 )
		return -1;
	cconnect * c = AddConnect();
	c->start_pin = p1;
	c->end_pin = cconnect::NO_END;

	// add single vertex
	cvertex new_vtx;
	CPoint pi;
	pi = m_nlist->m_plist->GetPinPoint( pin[p1].part, pin[p1].pin_name );
	new_vtx.x = pi.x;
	new_vtx.y = pi.y;
	new_vtx.pad_layer = m_nlist->m_plist->GetPinLayer( pin[p1].part, &pin[p1].pin_name );
	cvertex * v = &c->InsertVertexByIndex( 0, new_vtx );
	return ConIndexByPtr( c );
}
// Add new connection to net, consisting of one unrouted segment
// p1 and p2 are indexes into pin array for this net
// returns index to connection, or -1 if fails
//
int cnet::AddConnectFromPinToPin( int p1, int p2 )
{
	// check for valid pins
	cpart * part1 = pin[p1].part;
	cpart * part2 = pin[p2].part;
	if( part1 == 0 || part2 == 0 )
		return -1;
	CShape * shape1 = part1->shape;
	CShape * shape2 = part2->shape;
	if( shape1 == 0 || shape2 == 0 )
		return -1;
	int pin_index1 = shape1->GetPinIndexByName( pin[p1].pin_name );
	int pin_index2 = shape2->GetPinIndexByName( pin[p2].pin_name );
	if( pin_index1 == -1 || pin_index2 == -1 )
		return -1;

	// create connection with a single vertex
	CPoint pi, pf;
	pi = m_nlist->m_plist->GetPinPoint( pin[p1].part, pin[p1].pin_name );
	pf = m_nlist->m_plist->GetPinPoint( pin[p2].part, pin[p2].pin_name );

	int ic = AddConnectFromPin( p1 );
	cconnect * c = ConByIndex( ic );

	// add first segment and second vertex
	cvertex new_vtx;
	new_vtx.x = pf.x;
	new_vtx.y = pf.y;
	new_vtx.pad_layer = m_nlist->m_plist->GetPinLayer( pin[p2].part, &pin[p2].pin_name );

	cseg new_seg;
	new_seg.layer = LAY_RAT_LINE;
	new_seg.width = 0;
	new_seg.selected = 0;

	c->AppendSegAndVertex( new_seg, new_vtx );
	c->end_pin = p2;
	cseg * s = &c->SegByIndex( 0 );

	// create id for this segment
	id id( ID_NET, this->UID(), ID_CONNECT, c->UID(), ic, ID_SEG, s->UID(), 0 );

	if( m_dlist )
	{
		// draw graphic elements for segment
		s->dl_el = m_dlist->Add( id, this, LAY_RAT_LINE, DL_LINE, 
			visible, 0, 0, 0, pi.x, pi.y, pf.x, pf.y, 0, 0 ); 
		id.SetT3( ID_SEL_SEG );
		s->dl_sel = m_dlist->AddSelector( id, this, LAY_RAT_LINE, DL_LINE,
			visible, 0, 0, pi.x, pi.y, pf.x, pf.y, 0, 0 ); 
	}

	return ic;
}


// remove connection
// if called from an iterated loop, calling function should
// also call CIterator_cconnect::Remove() to adjust iterator(s)
//
void cnet::RemoveConnect( cconnect * c )
{
	int ic = ConIndexByPtr( c );
	connect.RemoveAt( ic );
	delete c;
}

// return cconnect with given index
cconnect * cnet::ConByIndex( int ic )
{
	if( ic >= 0 && ic < connect.GetSize() )
		return connect[ic];
	else
		return NULL;
};

// return index of cconnect with given UID
int cnet::ConIndexByUID( int uid )
{
	CIterator_cconnect iter_con(this);
	cconnect * c = iter_con.GetFirst();
	while( c )
	{
		if( c->m_uid == uid )
		{
			return iter_con.GetIndex();
		}
		c = iter_con.GetNext();
	}
	return -1;
};

// return cconnect with given UID
// also returns index
cconnect * cnet::ConByUID( int uid, int * index )
{
	int ic = ConIndexByUID( uid );
	if( ic == -1 )
		return NULL;
	else 
	{
		if( index )
			*index = ic;
		return connect[ic];
	}
}

// return index of cconnect with given pointer 
int cnet::ConIndexByPtr( cconnect * con )
{
	CIterator_cconnect iter_con(this);
	for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
	{
		if( c == con )
		{
			return iter_con.GetIndex();
		}
	}
	return -1;
}

// create a connection from undo info
void cnet::RecreateConnectFromUndo( undo_con * un_con, undo_seg * un_seg, undo_vtx * vtx )
{
	int nc;
	if( un_con->nsegs )
	{
		// add new connect
		if( un_con->end_pin != cconnect::NO_END )
			nc = AddConnectFromPinToPin( un_con->start_pin, un_con->end_pin );
		else
			nc = AddConnectFromPin( un_con->start_pin );
		cconnect * c = connect[nc];
		// now replace all connect parameters from undo record
		c->ReplaceUID( un_con->uid );
		// insert segments and vertices into connection
		for( int is=0; is<un_con->nsegs; is++ )
		{
			if( un_con->end_pin != cconnect::NO_END )
			{
				// pin-pin trace
				m_nlist->InsertSegment( this, nc, is, vtx[is+1].x, vtx[is+1].y,
					un_seg[is].layer, un_seg[is].width, un_seg[is].via_w, un_seg[is].via_hole_w, 0 );
			}
			else
			{
				// stub trace
				m_nlist->AppendSegment( this, nc, vtx[is+1].x, vtx[is+1].y,
					un_seg[is].layer, un_seg[is].width );
			}
		}
		// now finish importing undo data into segments and vertices
		for( int is=0; is<un_con->nsegs; is++ )
		{
			c->seg[is].ReplaceUID( un_seg[is].uid );
			if( is == 0 )
			{
				c->vtx[is].ReplaceUID( vtx[is].uid );
			}
			c->vtx[is+1].ReplaceUID( vtx[is+1].uid );
			c->vtx[is+1].via_w = vtx[is+1].via_w;
			c->vtx[is+1].via_hole_w = vtx[is+1].via_hole_w;
			if( vtx[is+1].force_via_flag )
			{
				m_nlist->ForceVia( this, nc, is+1, FALSE );
			}
			c->vtx[is+1].tee_ID = vtx[is+1].tee_ID;
			if( vtx[is+1].tee_ID )
				m_nlist->AddTeeID( vtx[is+1].tee_ID );
			m_nlist->ReconcileVia( this, nc, is+1 );
		}
		// other parameters
		connect[nc]->locked = un_con->locked; 
		c->Draw();
	}
}

// Split a connection into two connections sharing a tee-vertex
// enter with:
//	con_id - id of connection to be split
//	vtx_id - id of vertex to be split at, can't already be a tee-vertex
// return pointer to the new connection
cconnect * cnet::SplitConnectAtVertex( id vtx_id )
{
	cconnect * old_c = ConByUID( vtx_id.U2() );
	old_c->Undraw();
	cconnect * new_c = AddConnect();
	new_c->start_pin = old_c->end_pin;
	int ivsplit;
	old_c->VtxByUID( vtx_id.U3(), &ivsplit );
	for( int iv=old_c->NumSegs(); iv>=ivsplit; iv-- )
	{
		if( iv == old_c->NumSegs() )
			new_c->InsertVertexByIndex( 0, old_c->VtxByIndex(iv) );
		else
		{
			new_c->AppendSegAndVertex( old_c->seg[iv], old_c->vtx[iv] );
			old_c->RemoveSegAndVertexByIndex( iv );
		}
	}
	m_nlist->GetNewTeeID();

	return new_c;
}

// Add a new connection from a vertex to a pin
BOOL cnet::AddConnectionFromVertexToPin( id vtx_id, id pin_id )
{
	SplitConnectAtVertex( vtx_id );
	return FALSE;
}


// get pointer to area from UID
carea * cnet::AreaByUID( int uid, int * index )
{
	CIterator_carea iter_area( this );
	for( carea * a=iter_area.GetFirst(); a; a=iter_area.GetNext() )
	{
		if( a->UID() == uid )
		{
			*index =iter_area.GetIndex();
			return a;
		}
	}
	return NULL;
}

// get pointer to area from Index
carea * cnet::AreaByIndex( int ia )
{
	if( ia < 0 || ia > NumAreas()-1 )
		return NULL;
	return &area[ia];
}



