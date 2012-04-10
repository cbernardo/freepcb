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

extern Cuid pcb_cuid;		// global UID gnerator
extern CFreePcbDoc * pcb;	// global pointer to document

//********************** cpin implementation ****************************

// default constructor, should always be followed by Initialize()
cpin::cpin()
{
	m_uid = pcb_cuid.GetNewUID();
	part = NULL; 
	m_net = NULL;
	utility = 0;
}

// copy constructor
// don't copy UID or display elements
cpin::cpin( cpin& src )
{
	ref_des = src.ref_des;
	pin_name = src.pin_name;
	part = src.part;
	m_net = src.m_net;
	utility = 0;
}

cpin::~cpin()
{
}

// copy all variables except m_uid
// this form needed by CArray.InsertAt()
cpin& cpin::operator=( const cpin& rhs )
{
	ref_des = rhs.ref_des;
	pin_name = rhs.pin_name;
	part = rhs.part;
	m_net = rhs.m_net;
	utility = 0;
	return *this;
}

// copy all variables except m_uid
cpin& cpin::operator=( cpin& rhs )
{
	ref_des = rhs.ref_des;
	pin_name = rhs.pin_name;
	part = rhs.part;
	m_net = rhs.m_net;
	utility = 0;
	return *this;
}

void cpin::Initialize( cnet * net )
{
	m_net = net;
}

// return index of this pin in net, or -1 if not found
int cpin::Index()
{
	for( int ip=0; ip<m_net->NumPins(); ip++ )
	{
		if( m_net->PinByIndex(ip) == this )
			return ip;
	}
	return -1;
}

// return UID
int cpin::UID()
{
	return m_uid;
}

// get pointer to this pin on part
part_pin * cpin::GetPartPin()
{
	if( part )
	{
		if( part->shape )
		{
			int ipp = part->shape->GetPinIndexByName(pin_name);
			{
				if( ipp != -1 )
				{
					return &part->pin[ipp];
				}
			}
		}
	}
	return NULL;
}

// get Id of this pin on part
id cpin::GetPartPinId()
{
	part_pin * pp = GetPartPin();
	if( pp )
		return pp->Id();
	else
		return id();
}


//***************************** carea implementation **********************

// default constructor, should be followed by call to Initialize()
carea::carea()
{
	m_net = NULL;
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

// carea const assignment operator
// doesn't actually assign but required for CArray<carea,carea>.InsertAt()
carea &carea::operator=( const carea &a )
{
	return *this;
}

// initialize pointers and id
void carea::Initialize( CDisplayList * dlist, cnet * net )
{
	m_dlist = dlist;
	m_net = net;
	SetDlist( dlist );
	Id();
}

// initialize and return id
id& carea::Id()
{
	id a_id = m_net->Id();
	a_id.Set( id::NOP, id::NOP, ID_AREA, m_uid );
	CPolyLine::SetId( &a_id );
	return CPolyLine::Id();
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
// don't copy display elements
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
	if( m_dlist )
	{
		if( dl_el )
			m_dlist->Remove( dl_el );
		if( dl_sel )
			m_dlist->Remove( dl_sel );
	}
}

// assignment from const rhs (needed by CArray::InsertAt)
cseg& cseg::operator=( const cseg& rhs )
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

// set up pointers
void cseg::Initialize( cconnect * c )
{
	m_con = c;
	m_net = c->m_net;
	m_dlist = m_net->m_dlist;
}

// get id
id cseg::Id()
{
	id s_id = m_con->Id();
	s_id.SetSubSubType( ID_SEL_SEG, m_uid );
	return s_id;
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

// create string describing segment
void cseg::GetStatusStr( CString * str )
{
	int u = pcb->m_units;
	CString w_str;
	::MakeCStringFromDimension( &w_str, width, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
	str->Format( "segment, w %s", w_str );
}


//**************************** cvertex implementation *********************
cvertex::cvertex()
{
	// constructor
	m_uid = pcb_cuid.GetNewUID();
	m_con = 0;		// this must set with Initialize()
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
// don't copy display elements
cvertex::cvertex( cvertex& v )
{
	// constructor
	m_uid = v.m_uid;
	x = v.x;
	y = v.y;
	pad_layer = v.pad_layer;
	force_via_flag = v.force_via_flag;
	via_w = v.via_w;
	via_hole_w = v.via_hole_w;
	tee_ID = v.tee_ID;
	utility = v.utility;
	utility2 = v.utility2;
	m_con = v.m_con;
	m_dlist = m_con->m_dlist;
}

cvertex::~cvertex()
{
	// destructor
	if( m_dlist )
	{
		for( int il=0; il<dl_el.GetSize(); il++ )
			m_con->m_net->m_dlist->Remove( dl_el[il] );
		if( dl_sel )
			m_con->m_net->m_dlist->Remove( dl_sel );
		if( dl_hole )
			m_con->m_net->m_dlist->Remove( dl_hole );
	}
}

// assignment from const
// don't copy m_con, m_dlist, UID or display elements
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
// don't copy m_con, m_dlist, UID or display elements
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
	m_dlist = m_con->m_dlist;
}

int cvertex::UID()
{ 
	return m_uid; 
};

id cvertex::Id()
{
	id v_id = m_con->Id();
	int iv;
	m_con->VtxByUID( m_uid, &iv );
	v_id.SetSubSubType( ID_SEL_VERTEX, m_uid, iv );
	return v_id;
}

// return index of vertex in connection, or -1 if not found
//
int cvertex::Index()
{
	if( !m_con )
		return -1;
	return m_con->VtxIndexByPtr( this );
}

void cvertex::Draw()
{
	cconnect * c = m_con;
	cnet * net = m_con->m_net;
	CNetList * nl = net->m_nlist;

	// undraw previous via and selection box 
	Undraw();

	// draw via if (v->via_w) > 0
	id vid = Id();
	if( via_w )
	{
		// draw via
		vid.SetT3( ID_VERTEX );
		int n_layers = nl->GetNumCopperLayers();
		dl_el.SetSize( n_layers );
		for( int il=0; il<n_layers; il++ )
		{
			int layer = LAY_TOP_COPPER + il;
			dl_el[il] = m_dlist->Add( vid, net, layer, DL_CIRC, 1, 
				via_w, 0, 0,
				x, y, 0, 0, 0, 0 );
		}
		dl_hole = m_dlist->Add( vid, net, LAY_PAD_THRU, DL_HOLE, 1, 
				via_hole_w, 0, 0,
				x, y, 0, 0, 0, 0 );
	}

	// draw selection box for vertex, using LAY_THRU_PAD if via or layer of adjacent
	// segments if no via
	vid.SetT3( ID_SEL_VERTEX );
	int sel_layer;
	if( via_w )
		sel_layer = LAY_SELECTION;
	else if( vid.I3() > 0 )
		sel_layer = c->SegByIndex( vid.I3()-1 ).layer;
	else
		sel_layer = c->SegByIndex( vid.I3() ).layer;
	dl_sel = m_dlist->AddSelector( vid, net, sel_layer, DL_HOLLOW_RECT, 
		1, 0, 0, x-10*PCBU_PER_MIL, y-10*PCBU_PER_MIL, x+10*PCBU_PER_MIL, y+10*PCBU_PER_MIL, 0, 0 );
	return;
}



void cvertex::Undraw()
{
	if( m_dlist )
	{
		if( dl_el.GetSize() )
		{
			for( int i=0; i<dl_el.GetSize(); i++ )
			{
				m_con->m_net->m_dlist->Remove( dl_el[i] );
			}
			dl_el.RemoveAll();
		}
		m_con->m_net->m_dlist->Remove( dl_sel );
		m_con->m_net->m_dlist->Remove( dl_hole );
	}
	dl_sel = NULL;
	dl_hole = NULL;
}

// return type of vertex
cvertex::VType cvertex::GetType()
{
	if( pad_layer != 0 )
		return V_PIN;
	else if( tee_ID > 0 )
		return V_TEE;
	else if( tee_ID < 0 )
		return V_SLAVE;
	else if( this == m_con->LastVtx() )
		return V_END;
	else if( this == m_con->FirstVtx() )
		return V_END;
	else
		return V_TRACE;
}

// if vertex is a pin, return it
cpin * cvertex::GetNetPin()
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

// if vertex is a pin, return index to it, else return -1
int cvertex::GetNetPinIndex()
{
	if( GetType() == V_PIN )
	{
		if( this == m_con->FirstVtx() )
			return m_con->start_pin;
		else if( this == m_con->LastVtx() )
			return m_con->end_pin;
	}
	return -1;
}

// Get string describing vertex type
void cvertex::GetTypeStatusStr( CString * str )
{
	VType type = GetType();
	if( type == V_PIN )
	{
		cpin * pin = GetNetPin();
		str->Format( "%s.%s", pin->ref_des, pin->pin_name );
	}
	else if( type == V_TEE || type == V_SLAVE )
	{
		str->Format( "T(%d)", abs(tee_ID) );
	}
	else if( type == V_END )
	{
		*str = "stub-end";
	}
	else
	{
		*str = "vertex";	
	}
}

void cvertex::GetStatusStr( CString * str )
{
	int u = pcb->m_units;
	CString type_str, x_str, y_str, via_w_str, via_hole_str;
	VType type = GetType();
	if( type == V_PIN )
	{
		type_str = "pin-vertex";	// should never happen
	}
	else if( type == V_TEE || type == V_SLAVE )
	{
		type_str.Format( "T-vertex(%d)", tee_ID );
	}
	else if( type == V_END )
	{
		type_str = "end-vertex";
	}
	else
	{
		type_str = "vertex";
	}
	::MakeCStringFromDimension( &x_str, x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
	::MakeCStringFromDimension( &y_str, y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
	if( via_w )
	{
		::MakeCStringFromDimension( &via_w_str, via_w, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		::MakeCStringFromDimension( &via_hole_str, via_hole_w, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		str->Format( "%s, x %s, y %s, via %s/%s",
			type_str,
			x_str,
			y_str,
			via_w_str,
			via_hole_str
			);
	}
	else
	{
		str->Format( "%s, x %s, y %s, no via",
			type_str,
			x_str,
			y_str
			);
	}
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
	vtx.RemoveAll();
	seg.RemoveAll();
}

// clear segment and vertex arrays
void cconnect::ClearArrays()
{ 
	seg.RemoveAll(); 
	vtx.SetSize(0); 
}

// get id
id cconnect::Id()
{
	id c_id = m_net->Id();
	int ic;
	m_net->ConByUID( m_uid, &ic );
	c_id.Set( id::NOP, id::NOP, ID_CONNECT, m_uid, ic ); 
	return c_id;
}

// get string describing the connection
void cconnect::GetStatusStr( CString * str )
{
	CString net_str, type_str, locked_str, from_str, to_str;
	m_net->GetStatusStr( &net_str );
	if( NumSegs() == 1 && seg[0].layer == LAY_RAT_LINE )
	{
		type_str = "ratline";
	}
	else
	{
		type_str = "trace";
	}
	locked_str = "";
	if( FirstVtx()->GetType() == cvertex::V_PIN 
		&& LastVtx()->GetType() == cvertex::V_PIN 
		&& locked == TRUE )
	{
		locked_str = " (L)";
	}
	FirstVtx()->GetTypeStatusStr( &from_str );
	LastVtx()->GetTypeStatusStr( &to_str );
	str->Format( "%s, %s from %s to %s%s", net_str, type_str,
		from_str, to_str, locked_str );
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

// get index to vertex, or -1 if not found
//
int cconnect::VtxIndexByPtr( cvertex * vtx )
{
	CIterator_cvertex iter_vtx( this );
	for( cvertex * v=iter_vtx.GetFirst(); v; v=iter_vtx.GetNext() )
	{
		if( v == vtx )
			return iter_vtx.GetIndex();
	}
	return -1;
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
// new vertex will inserted at vtx[is+1], 
// dividing old seg[is] into seg[is] and seg[is+1],
// and shifting following segments and vertices up by one
//
// parameters are copied from new_seg and new_vtx
//
// if dir = 0, params are copied from new_seg->seg[is]
// if dir = 1, params are copied from seg[is+1]->seg[is] and new_seg to seg[is+1]
//
void cconnect::InsertSegAndVtxByIndex(int is, int dir, 
					const cseg& new_seg, const cvertex& new_vtx )
{
	cseg old_seg;
	if( NumSegs() == 0 )
		ASSERT(0);
	old_seg = seg[is];		// copy old seg[is];

	if( dir == 0 )
	{
		// route forward
		seg.InsertAt(is, new_seg );		
		SegByIndex(is).Initialize(this);	
	}
	else
	{
		// route backward
		seg.InsertAt(is, old_seg );	
		seg[is+1] = new_seg;	
		SegByIndex(is+1).Initialize(this);
	}
	vtx.InsertAt(is+1, new_vtx );
	VtxByIndex(is+1).Initialize(this);
}

// append new seg and vertex to end of connection (used for stubs)
//
void cconnect::AppendSegAndVertex( const cseg& new_seg, 
									  const cvertex& new_vtx )
{
	seg.Add( new_seg );
	vtx.Add( new_vtx );
	int ns = NumSegs();
	cseg * s = &SegByIndex( ns-1 );
	cvertex * v = &VtxByIndex( ns );
	s->Initialize(this);
	v->Initialize(this);
}

// insert new starting vertex and segment into connection
//
void cconnect::PrependVertexAndSeg( const cvertex& new_vtx,
							const cseg& new_seg )
{
	vtx.InsertAt( 0, new_vtx );
	seg.InsertAt( 0, new_seg );
	cseg * s = &SegByIndex( 0 );
	cvertex * v = &VtxByIndex( 0 );
	s->Initialize(this);
	v->Initialize(this);
}

void cconnect::RemoveSegAndVertexByIndex( int is )
{
	cvertex * v = &vtx[is+1];
	if( is == NumSegs()-1 && v->GetType() == cvertex::V_PIN )
		end_pin = NO_END;
	vtx.RemoveAt( is+1 );
	seg.RemoveAt( is );
}

void cconnect::ReverseDirection()
{
	int ns = NumSegs();
	int nsteps = (ns+1)/2;
	for( int i=0; i<nsteps; i++ )
	{
		cseg s;
		cvertex v;
		v = vtx[i];
		s = seg[i];
		vtx[i] = vtx[ns-i];
		vtx[ns-i] = v;
		s = seg[i];
		seg[i] = seg[ns-i-1];
		seg[ns-i-1] = s;
	}
	int t;
	t = start_pin;
	start_pin = end_pin;
	end_pin = t;
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
			s_id.SetSubType( ID_CONNECT, m_uid, ic );
			s_id.SetSubSubType( ID_SEG, s->m_uid, is );
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
		int nvtx = NumSegs()+1;
		for( int iv=0; iv<nvtx; iv++ )
			m_net->m_nlist->ReconcileVia( m_net, ic, iv );
	}
}


//************************* cnet implementation ***************************

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
}

// return size of arrays
int cnet::NumCons(){ return connect.GetSize(); };
int cnet::NumPins(){ return pin.GetSize(); };
int cnet::NumAreas(){ return area.GetSize(); };

// create string showing net status
void cnet::GetStatusStr( CString * str )
{
	str->Format( "Net %s", name ); 
}

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

cpin * cnet::PinByUID( int uid, int * index )
{
	CIterator_cpin iter_pin( this );
	for( cpin * p=iter_pin.GetFirst(); p; p=iter_pin.GetNext() )
	{
		if( p->m_uid == uid )
		{
			if( index )
				*index = iter_pin.GetIndex();
			return p;
		}
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
	CIterator_cconnect iter_con( this );
	for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
	{
		if( c->start_pin == net_pin_index || c->end_pin == net_pin_index )
			RemoveConnectAdjustTees( c );
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

// add new empty connection (no segments or vertices)
//
cconnect * cnet::AddConnect( int * ic )
{
	cconnect  * c = new cconnect( this );
	connect.Add( c );
   	if( ic )
		*ic = this->NumCons() - 1;
	return c;
}

// add connection consisting of one slave vertex
//
cconnect * cnet::AddConnectFromTraceVtx( id& vtx_id, int * ic_ptr )
{
	// if the vertex to start from is not already a tee-vertex, make it one
	cconnect * start_c = ConByUID( vtx_id.U2() );
	cvertex * start_v = start_c->VtxByUID( vtx_id.U3() );
	int start_tee_ID = -1;
	if( start_v->GetType() == cvertex::V_TEE )
	{
		// already a tee-vertex
		start_tee_ID = start_v->tee_ID;
	}
	else if( start_v->GetType() == cvertex::V_SLAVE )
	{
		// already slaved to a tee-vertex
		start_tee_ID = -start_v->tee_ID;
	}
	else if( start_v->GetType() == cvertex::V_TRACE )
	{
		// need to split connection and make this a tee-vertex
		SplitConnectAtVtx( vtx_id );
	}
	else
	{
		// should't happen
		ASSERT(0);
	}

	// add empty connection
	cconnect * c = AddConnect( ic_ptr );
	// add single slave vertex
	cvertex new_vtx;
	new_vtx.x = start_v->x;
	new_vtx.y = start_v->y;
	new_vtx.tee_ID = -start_v->tee_ID;
	cvertex * v = &c->InsertVertexByIndex( 0, new_vtx );
	return c;
}

// add connection consisting of one vertex at the starting pin
//
cconnect * cnet::AddConnectFromPin( int p1, int * ic_ptr )
{
	int ic;
	if( pin[p1].part == 0 )
		return NULL;
	cconnect * c = AddConnect( ic_ptr );
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
	return c;
}

// Add new connection to net, consisting of one unrouted segment
// p1 and p2 are indexes into pin array for this net
// returns connection, or NULL if fails
// if ic_ptr is provided, sets *ic_ptr to the index of the connection
//
cconnect * cnet::AddConnectFromPinToPin( int p1, int p2, int * ic_ptr )
{
	// check for valid pins
	cpart * part1 = pin[p1].part;
	cpart * part2 = pin[p2].part;
	if( part1 == 0 || part2 == 0 )
		return NULL;
	CShape * shape1 = part1->shape;
	CShape * shape2 = part2->shape;
	if( shape1 == 0 || shape2 == 0 )
		return NULL;
	int pin_index1 = shape1->GetPinIndexByName( pin[p1].pin_name );
	int pin_index2 = shape2->GetPinIndexByName( pin[p2].pin_name );
	if( pin_index1 == -1 || pin_index2 == -1 )
		return NULL;

	// create connection with a single vertex
	CPoint pi, pf;
	pi = m_nlist->m_plist->GetPinPoint( pin[p1].part, pin[p1].pin_name );
	pf = m_nlist->m_plist->GetPinPoint( pin[p2].part, pin[p2].pin_name );

	int ic;
	cconnect * c = AddConnectFromPin( p1, &ic );

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
	if( ic_ptr )
		*ic_ptr = ic;
	return c;
}

// remove a connection, don't change others 
//
void cnet::RemoveConnect( cconnect * c )
{
	// notify iterators of pending removal
	CIterator_cconnect iter_con( this );
	iter_con.OnRemove( c );
	// remove connection
	int ic = ConIndexByPtr( c );
	c->Undraw();
	delete c;
	connect.RemoveAt( ic );
}

// remove a connection, 
// adjust any iterators, 
// if connected to other traces through tees, adjust them,
// may result in removal of other connections
//
void cnet::RemoveConnectAdjustTees( cconnect * c )
{
	// notify iterators of pending removal
	CIterator_cconnect iter_con( this );
	iter_con.OnRemove( c );
	// see if any tees to check
	int tee_1 = c->FirstVtx()->tee_ID;
	int tee_2 = c->LastVtx()->tee_ID;
	// remove connection
	int ic = ConIndexByPtr( c );
	c->Undraw();
	delete c;
	connect.RemoveAt( ic );
	// now adjust tees
	if( tee_1 )
		AdjustTees( tee_1 );
	if( tee_2 )
		AdjustTees( tee_2 );
}

// check connections to a tee and modify if necessary
// usually called when a connection to a tee has been removed
//
void cnet::AdjustTees( int tee_id )
{
	// loop through all connections and repair or remove tees
	int num_ids = 0;
	cvertex * first_tee_v = NULL;
	cvertex * last_tee_v = NULL;
	BOOL bMasterFound = FALSE;
	CIterator_cconnect iter_con( this );
	for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
	{
		// test first and last vertices for tee_id
		for( int ii=0; ii<2; ii++ )
		{
			cvertex * v = c->FirstVtx();
			if( ii > 0 )
				v = c->LastVtx();
			// check for master tee
			if( v->tee_ID == abs(tee_id) )
				bMasterFound = TRUE;
			// count number of connections to tee
			if( abs(v->tee_ID) == abs(tee_id) )
			{
				num_ids++;
				if( first_tee_v == NULL )
				{
					first_tee_v = v;
				}
				last_tee_v = v;
			}
		}
	}
	if( num_ids == 0 )
	{
		// probably shouldn't happen, but ignore it
	}
	else if( num_ids == 1 )
	{
		// just eliminate tee
		first_tee_v->tee_ID = 0;
	}
	else if( num_ids == 2 )
	{
		// remove the tee and merge the two connections
		cconnect * c1 = first_tee_v->m_con;
		cconnect * c2 = last_tee_v->m_con;
		if( c1 == c2 )
		{
			// special case of circular connection
			c1->FirstVtx()->tee_ID = 0;
			c1->LastVtx()->tee_ID = 0;
			RemoveConnect( c1 );
		}
		else
		{
			c1->Undraw();
			c2->Undraw();
			if( abs(c1->FirstVtx()->tee_ID) == abs(tee_id) )
				c1->ReverseDirection();
			if( abs(c2->FirstVtx()->tee_ID) != abs(tee_id) )
				c2->ReverseDirection();
			ConcatenateConnections( c1, c2 );
			c1->Draw();
		}
	}
	else if( num_ids > 2 && bMasterFound == FALSE )
	{
		// still need the tee, just make new master
		first_tee_v->tee_ID = tee_id;
	}
}

// return cconnect with given index
cconnect * cnet::ConByIndex( int ic )
{
	if( ic >= 0 && ic < connect.GetSize() )
		return connect[ic];
	else
		return NULL;
};

// return cconnect with given UID
// also returns index
cconnect * cnet::ConByUID( int uid, int * index )
{
	CIterator_cconnect iter_con(this);
	cconnect * c = iter_con.GetFirst();
	while( c )
	{
		if( c->m_uid == uid )
		{
			if( index )
				*index = iter_con.GetIndex();
			return c;
		}
		c = iter_con.GetNext();
	}
	return NULL;
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
void cnet::RecreateConnectFromUndo( undo_con * un_con, undo_seg * un_seg, undo_vtx * un_vtx )
{
	int nc;
	if( un_con->nsegs )
	{
		// add new connect and set fron undo record
		cconnect * c = AddConnect( &nc );
		c->start_pin = un_con->start_pin;
		c->end_pin = un_con->end_pin;
		c->m_uid = un_con->uid;
		c->locked = un_con->locked;
		c->seg.SetSize( un_con->nsegs );
		c->vtx.SetSize( un_con->nsegs+1 );
		for( int iv=0; iv<un_con->nsegs+1; iv++ )
		{
			cvertex * v = &c->vtx[iv];
			v->Initialize( c );
			v->m_uid = un_vtx[iv].uid;
			v->x = un_vtx[iv].x;
			v->y = un_vtx[iv].y;
			v->pad_layer = un_vtx[iv].pad_layer;
			v->force_via_flag = un_vtx[iv].force_via_flag;
			v->via_w = un_vtx[iv].via_w;
			v->via_hole_w = un_vtx[iv].via_hole_w;
			v->tee_ID = un_vtx[iv].tee_ID;
		}
		for( int is=0; is<un_con->nsegs; is++ )
		{
			cseg * s = &c->seg[is];
			s->Initialize( c );
			s->m_uid = un_seg[is].uid;
			s->layer = un_seg[is].layer;
			s->width = un_seg[is].width;
		}
		c->Draw();
	}
}

// Concatenate two connections into one
// Assumes that the last vertex of c1 = the first vertex of c2
// Removes c2
//
void cnet::ConcatenateConnections( cconnect * c1, cconnect * c2 )
{
	int ns1 = c1->NumSegs();
	int ns2 = c2->NumSegs();
	for( int is=0; is<ns2; is++ )
	{
		c1->AppendSegAndVertex( c2->seg[is], c2->vtx[is+1] );
	}
	// make sure that the junction vertex is not a tee
	cvertex * vj = &c1->VtxByIndex( ns1 );
	vj->tee_ID = 0;
	// update end pin
	c1->end_pin = c2->end_pin;
	// remove c2
	RemoveConnect( c2 );
}

// remove a segment from connection
// if this splits the connection, make new one
// handle any tee-vertices
//
void cnet::RemoveSegmentAdjustTees( cseg * s )
{
	cconnect * c = s->m_con;
	c->Undraw();
	int tee_1 = s->GetPreVtx().tee_ID;
	int tee_2 = s->GetPostVtx().tee_ID;
	s->GetPreVtx().tee_ID = 0;
	s->GetPostVtx().tee_ID = 0;
	int is = s->GetIndex();
	int ns = c->NumSegs();
	if( ns == 1 )
	{
		RemoveConnectAdjustTees( c );
	}
	else if( is == 0 )
	{
		// remove first segment and vertex
		c->seg.RemoveAt( 0 );
		c->vtx.RemoveAt( 0 );
		c->start_pin = cconnect::NO_END;
		c->Draw();
	}
	else if( is == ns-1 )
	{
		// remove last segment
		c->seg.RemoveAt( ns-1 );
		c->vtx.RemoveAt( ns );
		c->end_pin = cconnect::NO_END;
		c->Draw();
	}
	else 
	{
		// split connection in two
		// create new connection for end-part of old connection
		cconnect * c_new = AddConnect();
		c_new->start_pin = cconnect::NO_END;
		c_new->end_pin = c->end_pin;
		c_new->InsertVertexByIndex( 0, c->VtxByIndex(is+1) );
		for( int i=0; i<(ns-is-1); i++ )
		{
			c_new->AppendSegAndVertex( c->SegByIndex(is+i+1), c->VtxByIndex(is+i+2) );
		}
		// shorten old connection to eliminate s and following segments and vertices
		for( int i=is; i<ns; i++ )
		{
			c->RemoveSegAndVertexByIndex( is );
		}
		c->Draw();
		c_new->Draw();
	}
	if( tee_1 )
		AdjustTees( tee_1 );
	if( tee_2 )
		AdjustTees( tee_2 );
}


// Split a connection into two connections sharing a tee-vertex
// enter with:
//	vtx_id - id of vertex to be split at
// if the vertex is not already a tee-vertex, make a new one
// return pointer to the new connection
// both connections will end at the tee-vertex

cconnect * cnet::SplitConnectAtVtx( id vtx_id )
{
	cconnect * old_c = ConByUID( vtx_id.U2() );
	old_c->Undraw();
	cconnect * new_c = AddConnect();	// add empty connection
	new_c->start_pin = old_c->end_pin;	
	int ivsplit;
	cvertex * v_split = old_c->VtxByUID( vtx_id.U3(), &ivsplit );
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
	// convert both connections to stubs, ending at a shared tee-vertex
	int tee_ID = v_split->tee_ID;
	if( tee_ID == 0 )
		tee_ID = m_nlist->GetNewTeeID();
	old_c->LastVtx()->tee_ID = tee_ID;		// master
	new_c->LastVtx()->tee_ID = -tee_ID;		// slave
	new_c->Draw();
	old_c->Draw();
	return new_c;
}

// Add a new connection from a vertex to a pin
//
cconnect * cnet::AddConnectFromTraceVtxToPin( id vtx_id, int pin_index, int * ic_ptr )
{
	cconnect * split_c = SplitConnectAtVtx( vtx_id );
	cvertex * tee_v = split_c->LastVtx();
	int tee_ID = abs( tee_v->tee_ID );
	if( tee_ID == 0 )
		ASSERT(0);
	cconnect * c = AddConnectFromPin( pin_index, ic_ptr );

	// add first segment and second vertex, same as tee vertex
	cseg new_seg;
	new_seg.layer = LAY_RAT_LINE;
	new_seg.width = 0;
	new_seg.selected = 0;

	c->AppendSegAndVertex( new_seg, *tee_v );
	return c;
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



