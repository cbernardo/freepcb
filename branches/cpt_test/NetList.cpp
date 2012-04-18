// NetList.cpp: implementation of the CNetList class.
#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

extern Cuid pcb_cuid;

//#define PROFILE		// profiles calls to OptimizeConnections() for "GND"  
BOOL bDontShowSelfIntersectionWarning = FALSE;
BOOL bDontShowSelfIntersectionArcsWarning = FALSE;
BOOL bDontShowIntersectionWarning = FALSE;
BOOL bDontShowIntersectionArcsWarning = FALSE;

//************************* CNetList implementation *************************
//
CNetList::CNetList( CDisplayList * dlist, CPartList * plist )
{
	m_dlist = dlist;			// attach display list
	m_plist = plist;			// attach part list
	m_pos_i = -1;				// intialize index to iterators
	m_bSMT_connect = FALSE;
}

CNetList::~CNetList()
{
	RemoveAllNets();
}

// Add new net to netlist
//
cnet * CNetList::AddNet( CString name, int max_pins, int def_w, int def_via_w, int def_via_hole_w )
{
	// create new net
	cnet * new_net = new cnet( m_dlist, this );

	// set default trace width
	new_net->def_w = def_w;
	new_net->def_via_w = def_via_w;
	new_net->def_via_hole_w = def_via_hole_w;

	// set name
	new_net->name = name;

	// visible by default
	new_net->visible = 1;

	// add name and pointer to map
	m_map.SetAt( name, (void*)new_net );
	m_uid_map.SetAt( new_net->m_id.U1(), new_net );

	return new_net;
} 


// Remove net from list
//
void CNetList::RemoveNet( cnet * net )
{
	if( m_plist )
	{
		// remove pointers to net from pins on part
		for( int ip=0; ip<net->NumPins(); ip++ )
		{
			cpart * pin_part = net->pin[ip].part;
			if( pin_part )
			{
				CShape * s = pin_part->shape; 
				if( s )
				{
					int pin_index = s->GetPinIndexByName( net->pin[ip].pin_name );
					if( pin_index >= 0 )
						pin_part->pin[pin_index].net = NULL;
				}
			}
		}
	}
	// destroy connections
	CIterator_cconnect iter_con( net );
	for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
	{
		iter_con.OnRemove( c );
		net->RemoveConnect( c );
	}
	m_map.RemoveKey( net->name );
	m_uid_map.RemoveKey( net->m_id.U1()  );
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

// set utility parameters of all nets and subelements
//
void CNetList::MarkAllNets( int utility )
{
	CIterator_cnet iter_net(this);
	for( cnet * net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
	{
		net->utility = utility;
		net->utility2 = utility;
		CIterator_cpin iter_pin( net );
		for( cpin * p=iter_pin.GetFirst(); p; p=iter_pin.GetNext() )
			p->utility = utility;
		CIterator_cconnect iter_con( net );
		for( cconnect *c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
		{
			c->utility = utility;
			CIterator_cseg iter_seg( c );
			for( cseg * s=iter_seg.GetFirst(); s; s=iter_seg.GetNext() )
			{
				s->utility = utility;
			}
			CIterator_cvertex iter_vtx( c );
			for( cvertex * v=iter_vtx.GetFirst(); v; v=iter_vtx.GetNext() )
			{
				v->utility = utility;
				v->utility2 = utility;
			}
		}
		CIterator_carea iter_area( net );
		for( carea * a=iter_area.GetFirst(); a; a=iter_area.GetNext() )
		{
			a->utility = utility;
			a->SetUtility( utility );
			for( int is=0; is<a->NumSides(); is++ )
			{
				a->SetUtility( is, utility );
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
	CIterator_cnet iter_net(this);
	for( cnet * net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
	{
		CIterator_cconnect iter_con( net );
		for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
		{
			c->Undraw();
			CIterator_cvertex iter_vtx( c );
			for( cvertex * v=iter_vtx.GetFirst(); v; v=iter_vtx.GetNext() )
			{
				v->x += x_off;
				v->y += y_off;
			}
			c->Draw();
		}
		CIterator_carea iter_area( net );
		for( carea * a=iter_area.GetFirst(); a; a=iter_area.GetNext() )
		{
			a->MoveOrigin( x_off, y_off );
			int ia = iter_area.GetIndex();
			SetAreaConnections( net, ia );
		}
	}
}


// Remove pin from net (by part and pin_name), including all connections to pin
//
void CNetList::RemoveNetPin( cpart * part, CString * pin_name, BOOL bSetAreas )
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
	for( int ip=0; ip<net->NumPins(); ip++ )
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
	net->RemovePin( net_pin, bSetAreas );
}

// Remove connections to part->pin from part->pin->net
// set part->pin->net pointer and net->pin->part pointer to NULL
//
void CNetList::DisconnectNetPin( cpart * part, CString * pin_name, BOOL bSetAreas )
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
		return;
	}
	// find pin in pin list for net
	int net_pin = -1;
	for( int ip=0; ip<net->NumPins(); ip++ )
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
	// now remove all connections to/from this pin
	int ic = 0;
	while( ic<net->NumCons() )
	{
		cconnect * c = net->connect[ic];
		if( c->start_pin == net_pin || c->end_pin == net_pin )
			RemoveNetConnect( net, ic, FALSE );
		else
			ic++;
	}
	// now remove link to net from part
	part->pin[pin_index].net = NULL;
	// now remove link to part from net
	net->pin[net_pin].part = NULL;
	// adjust connections to areas
	if( net->NumAreas() && bSetAreas )
		SetAreaConnections( net );
}


// Disconnect pin from net (by reference designator and pin number)
// Use this if the part may not actually exist in the partlist,
// or the pin may not exist in the part 
//
void CNetList::DisconnectNetPin( cnet * net, CString * ref_des, CString * pin_name, BOOL bSetAreas )
{
	// find pin in pin list for net
	int net_pin = -1;
	for( int ip=0; ip<net->NumPins(); ip++ )
	{
		if( net->pin[ip].ref_des == *ref_des && net->pin[ip].pin_name == *pin_name )
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
	// now remove all connections to/from this pin
	CIterator_cconnect iter_con( net );
	for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
	{
		if( c->start_pin == net_pin || c->end_pin == net_pin )
			net->RemoveConnectAdjustTees( c );;
	}
	// now remove link to net from part pin (if it exists)
	cpart * part = net->pin[net_pin].part;
	if( part )
	{
		if( part->shape )
		{
			int pin_index = part->shape->GetPinIndexByName( *pin_name );
			if( pin_index != -1 )
				part->pin[pin_index].net = NULL;
		}
	}
	net->pin[net_pin].part = NULL;
	// adjust connections to areas
	if( net->NumAreas() && bSetAreas )
		SetAreaConnections( net );
}

// return pin index or -1 if not found
//
int CNetList::GetNetPinIndex( cnet * net, CString * ref_des, CString * pin_name )
{
	// find pin in pin list for net
	int net_pin = -1;
	for( int ip=0; ip<net->NumPins(); ip++ )
	{
		if( net->pin[ip].ref_des == *ref_des && net->pin[ip].pin_name == *pin_name )
		{
			net_pin = ip;
			break;
		}
	}
	return net_pin;
}

// test for hit on end-pad of connection
// if dir == 0, check end pad
// if dir == 1, check start pad
//
BOOL CNetList::TestHitOnConnectionEndPad( int x, int y, cnet * net, int ic, 
										 int layer, int dir )
{
	int ip;
	cconnect * c = net->connect[ic];
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
	for( int ic=net->NumCons()-1; ic>=0; ic-- )   
	{
		BOOL bConnectionRemoved = FALSE;
		cconnect * c = net->connect[ic];
		c->Undraw();
		for( int is=c->NumSegs()-1; is>=0; is-- )
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
				int layer = c->seg[is].m_layer;
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
					pre_layer = c->seg[is-1].m_layer;	// preceding layer
					if( c->vtx[is].tee_ID )
						pre_type = TEE;				// starts on a tee-vertex
					else if( c->vtx[is].via_w )
						pre_type = VIA;				// starts on a via
					else
						pre_type = SEGMENT;			// starts on a segment
				}
				// analyze end of segment
				if( is == c->NumSegs()-1 && c->end_pin == cconnect::NO_END )
				{
					// last segment of stub trace
					if( c->vtx[is+1].tee_ID )
						post_type = TEE;			// ends on a tee-vertex
					else if( c->vtx[is+1].via_w )
						post_type = VIA;			// ends on a via
					else
						post_type = END_STUB;		// ends a stub (no via or tee)
				}
				else if( is == c->NumSegs()-1 )
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
					post_layer = c->seg[is+1].m_layer;	
					if( c->vtx[is+1].tee_ID )
						post_type = TEE;				// ends on a tee-vertex
					else if( c->vtx[is+1].via_w )
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
					if( logstr )
					{
						CString str;
						CString str_pin_1 = "end-vertex", str_pin_2 = "end-vertex";
						if( c->FirstVtx()->GetType() == cvertex::V_TEE || c->FirstVtx()->GetType() == cvertex::V_SLAVE )
							str_pin_1.Format( "T(%d)", abs( c->FirstVtx()->tee_ID ) );
						else if( c->start_pin != cconnect::NO_END )
							str_pin_1 = net->pin[c->start_pin].ref_des + "." + net->pin[c->start_pin].pin_name;
						if( c->LastVtx()->GetType() == cvertex::V_TEE || c->LastVtx()->GetType() == cvertex::V_SLAVE )
							str_pin_1.Format( "T(%d)", abs( c->LastVtx()->tee_ID ) );
						else if( c->end_pin != cconnect::NO_END )
							str_pin_2 = net->pin[c->end_pin].ref_des + "." + net->pin[c->end_pin].pin_name;
						str.Format( "net %s: trace from %s to %s: removing zero-length segment\r\n",
								net->name, str_pin_1, str_pin_2 ); 
						*logstr += str;
					}
				}
			}
		}
		// see if there are any segments left
		if( c->NumSegs() == 0 )
		{
			// no, remove connection
			bConnectionRemoved = TRUE;
			net->connect.RemoveAt(ic);
		}
		else
		{
			// look for segments on same layer, with same width, 
			// not separated by a tee or via
			for( int is=c->NumSegs()-2; is>=0; is-- ) 
			{
				if( c->seg[is].m_layer == c->seg[is+1].m_layer 
					&& c->seg[is].m_width == c->seg[is+1].m_width
					&& c->vtx[is+1].via_w == 0
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
							CString str_pin_1 = "end-vertex", str_pin_2 = "end-vertex";
							if( c->FirstVtx()->GetType() == cvertex::V_TEE || c->FirstVtx()->GetType() == cvertex::V_SLAVE )
								str_pin_1.Format( "T(%d)", abs( c->FirstVtx()->tee_ID ) );
							else if( c->start_pin != cconnect::NO_END )
								str_pin_1 = net->pin[c->start_pin].ref_des + "." + net->pin[c->start_pin].pin_name;
							if( c->LastVtx()->GetType() == cvertex::V_TEE || c->LastVtx()->GetType() == cvertex::V_SLAVE )
								str_pin_1.Format( "T(%d)", abs( c->LastVtx()->tee_ID ) );
							else if( c->end_pin != cconnect::NO_END )
								str_pin_2 = net->pin[c->end_pin].ref_des + "." + net->pin[c->end_pin].pin_name;
							str.Format( "net %s: trace from %s to %s: combining colinear segments\r\n",
									net->name, str_pin_1, str_pin_2 ); 
							*logstr += str;
						}
						c->vtx.RemoveAt(is+1);
						c->seg.RemoveAt(is+1);
					}
				}
			}
			// now check for non-branch stubs with a single unrouted segment and no end-via
			if( c->end_pin == cconnect::NO_END && c->NumSegs() == 1 )
			{
				cvertex * end_v = &c->vtx[c->NumSegs()];
				cseg * end_s = &c->seg[c->NumSegs()-1];
				if( end_v->tee_ID == 0 && end_v->via_w == 0 && end_s->m_layer == LAY_RAT_LINE )
				{
					if( logstr )
					{
						CString str;
						str.Format( "net %s: stub trace from %s.%s: single unrouted segment and no end via, removed\r\n",
							net->name, 
							net->pin[c->start_pin].ref_des, net->pin[c->start_pin].pin_name ); 
						*logstr += str;
					}
					bConnectionRemoved = TRUE;
					net->connect.RemoveAt(ic);
				}
			}
			if( !bConnectionRemoved && m_dlist )
				c->Draw();
		}
	}

	// 
//**	RenumberConnections( net );
}

// Clean up all connections, then check tees and branch traces
//
void CNetList::CleanUpAllConnections( CString * logstr )
{
	CString str;

	CIterator_cnet iter_net(this);
	cnet * net = iter_net.GetFirst();
	while( net )
	{
		CleanUpConnections( net, logstr );
		net = iter_net.GetNext();
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
	net = iter_net.GetFirst();
	while( net )
	{
		// may have to iterate until no connections removed
		int n_removed = 999;
		while( n_removed != 0 )
		{
			n_removed = 0;
			for( int ic=net->NumCons()-1; ic>=0; ic-- )
			{
				cconnect * c = net->connect[ic];
				if( c->end_pin == cconnect::NO_END )
				{
					// no end pin, check for tee
					int end_id = c->vtx[c->NumSegs()].tee_ID;
					if( end_id )
					{
						end_id = abs(end_id);	// make +ve
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
						{
							RemoveNetConnect( net, ic, FALSE );
							n_removed++;
						}
					}
				}
				else
				{
					for( int iv=1; iv<c->NumSegs(); iv++ )
					{
						if( int id=c->vtx[iv].tee_ID )
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
		net = iter_net.GetNext();
	}
}


// Remove connection from net
// Leave pins in pin list for net
//
int CNetList::RemoveNetConnect( cnet * net, int ic, BOOL set_areas )
{
	id del_id_1;
	id del_id_2;
	cconnect * c = net->connect[ic];
	c->Undraw();
	// remove this connection
	net->RemoveConnectAdjustTees( c );
//**	RenumberConnections( net );
	// adjust connections to areas
	if( net->NumAreas() && set_areas )
		SetAreaConnections( net );
	return 0;
}

// Unroute all segments of a connection and merge if possible
// Preserves tees
//
int CNetList::UnrouteNetConnect( cnet * net, int ic )
{
	cconnect * c = net->connect[ic];
	for( int is=0; is<c->NumSegs(); is++ )
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
	for( int ip=0; ip<net->NumPins(); ip++ )
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
	cconnect * c = net->connect[ic];
	cpin * pin = &net->pin[pin_index];
	CPoint p = m_plist->GetPinPoint( part, *pin_name );
	int layer = m_plist->GetPinLayer( part, pin_name );
	if( end_flag )
	{
		// change end pin
		int is = c->NumSegs()-1;
		c->end_pin = pin_index;
		c->vtx[is+1].x = p.x;
		c->vtx[is+1].y = p.y;
		c->vtx[is+1].pad_layer = layer;
		c->seg[is].Draw();
	}
	else
	{
		// change start pin
		c->start_pin = pin_index;
		c->vtx[0].x = p.x;
		c->vtx[0].y = p.y;
		c->vtx[0].pad_layer = layer;
		c->seg[0].Draw();
	}
}



// Unroute segment
// return id of new segment (since seg[] array may change as a result)
//
id CNetList::UnrouteSegment( cnet * net, int ic, int is )
{
	cconnect * c = net->connect[ic];
	cseg * s = &c->seg[is];
	id seg_id = id(ID_NET, net->UID(), ID_CONNECT, c->UID(), ic, 
		ID_SEG, s->UID(), is );
	UnrouteSegmentWithoutMerge( net, ic, is );
	id mid = MergeUnroutedSegments( net, ic );
	if( mid.T1() == 0 )
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
	cconnect * c = net->connect[ic];
	BOOL bWasDrawn = c->m_bDrawn;
	id mid( ID_NET, net->UID(), ID_CONNECT, c->UID(), ic, ID_SEL_SEG, -1, -1 );

	if( c->NumSegs() == 1 )
		mid.Clear();

	c->Undraw();
	for( int is=c->NumSegs()-2; is>=0; is-- )
	{
		cseg * post_s = &c->seg[is+1];
		cseg * s = &c->seg[is];
		if( post_s->m_layer == LAY_RAT_LINE && s->m_layer == LAY_RAT_LINE
			&& c->vtx[is+1].tee_ID == 0 && c->vtx[is+1].force_via_flag == 0 )
		{
			// this segment and next are unrouted, 
			// remove next segment and interposed vertex
			c->seg.RemoveAt(is+1);
			c->vtx.RemoveAt(is+1);
			mid.SetI3( is );
			cseg *s = &c->seg[is];
			mid.SetU3( s->m_uid );
		}
	}
	if( mid.I3() == -1 )
		mid.Clear();
	if( bWasDrawn )
		c->Draw();
	return mid;
}

// Unroute segment, but don't merge with adjacent unrouted segments
// Assume that there will be an eventual call to MergeUnroutedSegments() to set vias
//
void CNetList::UnrouteSegmentWithoutMerge( cnet * net, int ic, int is )
{
	cconnect * c = net->connect[ic];  

	// unroute segment
	c->seg[is].m_layer = LAY_RAT_LINE;
	c->seg[is].m_width = 0;

	// redraw segment
	if( m_dlist )
	{
		if( c->seg[is].IsDrawn() )
		{
			c->seg[is].Draw();
		}
	}
	ReconcileVia( net, ic, is );
	ReconcileVia( net, ic, is+1 );
}

// Remove segment ... currently only used for last segment of stub trace
// If adjacent segments are unrouted, removes them too
// NB: May change connect[] array
// If stub trace and bHandleTee == FALSE, will only alter connections >= ic
//
void CNetList::RemoveSegment( cnet * net, int ic, int is, BOOL bHandleTee, BOOL bSetAreaConnections )
{
	int id = 0;
	cconnect * c = net->connect[ic];
	if( c->end_pin == cconnect::NO_END )
	{
		// stub trace, must be last segment
		if( is != (c->NumSegs()-1) )
		{
			ASSERT(0);
			return;
		}
		c->seg.RemoveAt(is);
		c->vtx.RemoveAt(is+1);
		if( c->NumSegs() == 0 )
		{
			net->connect.RemoveAt(ic);
//**			RenumberConnections( net );
		}
		else
		{
			if( c->seg[is-1].m_layer == LAY_RAT_LINE 
				&& c->vtx[is-1].via_w == 0
				&& c->vtx[is-1].tee_ID == 0 )
			{
				c->seg.RemoveAt(is-1);
				c->vtx.RemoveAt(is);
				if( c->NumSegs() == 0 )
				{
					net->connect.RemoveAt(ic);
//**					RenumberConnections( net );
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
		for( int iv=1; iv<=c->NumSegs(); iv++ )
		{
			cvertex * v = &c->vtx[iv];
			if( v->tee_ID )
				ASSERT(0);
		}
		// now convert connection to stub traces
		// first, make new stub trace from the end
		if( is < c->NumSegs()-1 )
		{
			net->AddConnectFromPin( c->end_pin );
			for( int iss=c->NumSegs()-1; iss>is; iss-- )
			{
//				AppendSegment( net, new_ic, 
			}
		}
	}
	// adjust connections to areas
	if( bSetAreaConnections && net->NumAreas() )
		SetAreaConnections( net );
}

#if 0
// renumber all ids and dl_elements for net connections
// should be used after deleting a connection
//
void CNetList::RenumberConnections( cnet * net )
{
	for( int ic=0; ic<net->NumCons(); ic++ )
	{
		RenumberConnection( net, ic );
	}
}

// renumber all ids and dl_elements for net connection
//
void CNetList::RenumberConnection( cnet * net, int ic )
{
	cconnect * c = net->connect[ic];
	for( int is=0; is<c->NumSegs(); is++ )
	{
		if( c->seg[is].dl_el )
		{
			c->seg[is].dl_el->id.SetI2( ic );
			c->seg[is].dl_el->id.SetI3( is );
		}
		if( c->seg[is].dl_sel )
		{
			c->seg[is].dl_sel->id.SetI2( ic );
			c->seg[is].dl_sel->id.SetI3( is );
		}
	}
	for( int iv=0; iv<=c->NumSegs(); iv++ )
	{
		for( int il=0; il<c->vtx[iv].dl_el.GetSize(); il++ )
		{
			if( c->vtx[iv].dl_el[il] )
			{
				c->vtx[iv].dl_el[il]->id.SetI2( ic );
				c->vtx[iv].dl_el[il]->id.SetI3( iv );
			}
		}
		if( c->vtx[iv].dl_sel )
		{
			c->vtx[iv].dl_sel->id.SetI2( ic );
			c->vtx[iv].dl_sel->id.SetI3( iv );
		}
		if( c->vtx[iv].dl_hole )
		{
			c->vtx[iv].dl_hole->id.SetI2( ic );
			c->vtx[iv].dl_hole->id.SetI3( iv );
		}
	}
}
#endif

// renumber the ids for graphical elements in areas
// should be called after deleting an area
//
void CNetList::RenumberAreas( cnet * net )
{
	id a_id;
	for( int ia=0; ia<net->NumAreas(); ia++ )
	{
		a_id = net->area[ia].Id();
		a_id.SetI2( ia );
		net->area[ia].SetId( &a_id );
		for( int ip=0; ip<net->area[ia].npins; ip++ )
		{
			carea * a = &net->area[ia];
			id a_id = m_dlist->Get_id( a->dl_thermal[ip] );
			a_id.SetI2( ia );
			m_dlist->Set_id( net->area[ia].dl_thermal[ip], &a_id );
		}
	}
}

// Set segment layer (must be a copper layer, not the ratline layer)
// returns 1 if unable to comply due to SMT pad
//
int CNetList::ChangeSegmentLayer( cnet * net, int ic, int iseg, int layer )
{
	cconnect * c = net->connect[ic];
	// check layer settings of adjacent vertices to make sure this is legal
	if( iseg == 0 )
	{
		// first segment, check starting pad layer
		int pad_layer = c->vtx[0].pad_layer;
		if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
			return 1;
	}
	if( iseg == (c->NumSegs() - 1) && c->end_pin != cconnect::NO_END )
	{
		// last segment, check destination pad layer
		int pad_layer = c->vtx[iseg+1].pad_layer;
		if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
			return 1;
	}
	// change segment layer
	cseg * s = &c->seg[iseg];
	s->Undraw();
	s->m_layer = layer;
	s->Draw();
	// now adjust vias
	cvertex * post_v = &s->GetPostVtx();
	ReconcileVia( net, ic, iseg );
	ReconcileVia( net, ic, iseg+1 );
	if( iseg == (c->NumSegs() - 1) && c->end_pin == cconnect::NO_END
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
int CNetList::RouteSegment( cnet * net, int ic, int iseg, int layer, int width )
{
	// check layer settings of adjacent vertices to make sure this is legal
	cconnect * c = net->connect[ic];
	cseg * s = &c->seg[iseg];
	if( iseg == 0 )
	{
		// first segment, check starting pad layer
		int pad_layer = c->vtx[0].pad_layer;
		if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
			return 1;
	}
	if( iseg == (c->NumSegs() - 1) )
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

	// modify segment parameters
	s->Undraw();
	s->m_layer = layer;
	s->m_width = width;
	s->m_selected = 0;
	s->Draw();

	// now adjust vias
	ReconcileVia( net, ic, iseg );
	ReconcileVia( net, ic, iseg+1 );
	return 0;
}


// Append new segment to connection 
// this is mainly used for stub traces
// returns index to new segment
//
int CNetList::AppendSegment( cnet * net, int ic, int x, int y, int layer, int width )
{
	// add new vertex and segment
	cconnect * c =net->connect[ic];
	BOOL bWasDrawn = c->m_bDrawn;
	c->Undraw();
	int nsegs = c->NumSegs();
	cseg new_seg;
	cvertex new_vtx;
	new_seg.m_layer = layer;
	new_seg.m_width = width;
	new_vtx.x = x;
	new_vtx.y = y;
	c->AppendSegAndVertex( new_seg, new_vtx );
	if( bWasDrawn )
		c->Draw();
	return nsegs;
}

// Insert new segment into connection, unless the new segment ends at the 
// endpoint of the old segment, then replace old segment 
// if dir=0 add forward in array, if dir=1 add backwards
// return 1 if segment inserted, 0 if replaced 
// tests position within +/- 10 nm.
//

int CNetList::InsertSegment( cnet * net, int ic, int iseg, int x, int y, int layer, int width,
						 int via_width, int via_hole_width, int dir )
{
	const int TOL = 10;

	// see whether we need to insert new segment or just modify old segment
	cconnect * c = net->connect[ic];
	BOOL bWasDrawn = c->m_bDrawn;
	c->Undraw();
	int insert_flag = 1;
	if( dir == 0 )
	{
		// routing forward
		if( (abs(x-c->vtx[iseg+1].x) + abs(y-c->vtx[iseg+1].y )) < TOL )
		{ 
			// new vertex is the same as end of old segment 
			if( iseg < (c->NumSegs()-1) )
			{
				// not the last segment
				if( layer == c->seg[iseg+1].m_layer )
				{
					// next segment routed on same layer, don't insert new seg
					insert_flag = 0;
				}
			}
			else if( iseg == (c->NumSegs()-1) )
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
				if( layer == c->seg[iseg-1].m_layer )
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

	if( insert_flag )
	{
		// copy parameters into temporary cseg and cvertex objects
		cseg new_seg;
		cvertex new_vtx;
		new_seg.m_layer = layer;
		new_seg.m_width = width;
		new_vtx.x = x;
		new_vtx.y = y;
		new_vtx.via_w = via_width;
		new_vtx.via_hole_w = via_hole_width;
		c->InsertSegAndVtxByIndex( iseg, dir, new_seg, new_vtx );
	}
	else
	{
		// don't insert, just modify old segment
		c->seg[iseg].m_selected = 0;
		c->seg[iseg].m_layer = layer;
		c->seg[iseg].m_width = width;
	}

	// clean up vias
	ReconcileVia( net, ic, iseg, FALSE );
	ReconcileVia( net, ic, iseg+1, FALSE );
	if( (iseg+1) < c->NumSegs() )
		ReconcileVia( net, ic, iseg+2, FALSE );

	// redraw connection
	if( bWasDrawn )
		c->Draw();
	return insert_flag;
}

// Set trace width for routed segment (ignores unrouted segs)
// If w = 0, ignore it
// If via_w = 0, ignore via_w and via_hole_w
//
int CNetList::SetSegmentWidth( cnet * net, int ic, int is, int w, int via_w, int via_hole_w )
{
	cconnect * c = net->connect[ic];
	if( c->seg[is].m_layer != LAY_RAT_LINE && w )
	{
		c->seg[is].m_width = w;
		c->seg[is].Draw();
	}
	if( c->vtx[is].via_w && via_w )
	{
		c->vtx[is].via_w = via_w;
		c->vtx[is].via_hole_w = via_hole_w;
		DrawVertex( net, ic, is );
	}
	if( c->vtx[is+1].via_w && via_w )
	{
		c->vtx[is+1].via_w = via_w;
		c->vtx[is+1].via_hole_w = via_hole_w;
		DrawVertex( net, ic, is+1 );
	}
	return 0;
}

int CNetList::SetConnectionWidth( cnet * net, int ic, int w, int via_w, int via_hole_w )
{
	cconnect * c = net->connect[ic];
	for( int is=0; is<c->NumSegs(); is++ )
	{
		SetSegmentWidth( net, ic, is, w, via_w, via_hole_w );
	}
	return 0;
}

int CNetList::SetNetWidth( cnet * net, int w, int via_w, int via_hole_w )
{
	for( int ic=0; ic<net->NumCons(); ic++ )
	{
		cconnect * c = net->connect[ic];
		for( int is=0; is<c->NumSegs(); is++ )
		{
			SetSegmentWidth( net, ic, is, w, via_w, via_hole_w );
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
		for( int ip=0; ip<net->NumPins(); ip++ )
		{
			if( net->pin[ip].ref_des == ref_des )
			{
				// found net->pin which attaches to part
				net->pin[ip].part = part;	// set net->pin->part
				if( part->shape )
				{
					int pin_index = part->shape->GetPinIndexByName( net->pin[ip].pin_name );
					if( pin_index != -1 )
					{
						// hook it up
						part->pin[pin_index].net = net;		// set part->pin->net
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
#if 0
	// get pin1 info
	int pin_index1 = part1->shape->GetPinIndexByName( *pin_name1 );
	CPoint pin_pt1 = m_plist->GetPinPoint( part1, *pin_name1 );
	int pin_lay1 = m_plist->GetPinLayer( part1, pin_name1 );
	cnet * net1 = m_plist->GetPinNet( part1, pin_name1 );
	int net1_pin_index = -1;
	if( net1 )
	{
		for( int ip=0; ip<net1->NumPins(); ip++ )
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
	int pin_lay2 = m_plist->GetPinLayer( part2, pin_name2 );
	cnet * net2 = m_plist->GetPinNet( part2, pin_name2 );
	int net2_pin_index = -1;
	if( net2 )
	{
		// find net2_pin_index for part2->pin2
		for( int ip=0; ip<net2->NumPins(); ip++ )
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
		for( int ic=0; ic<net1->NumCons(); ic++ )
		{
			cconnect * c = net1->connect[ic];
			cconnect * c2 = net2->connect[ic];
			int p1 = c->start_pin;
			int p2 = c->end_pin;
			int nsegs = c->NumSegs();
			if( nsegs )
			{
				if( p1 == net1_pin_index && p2 == net2_pin_index )
					continue;
				else if( p1 == net2_pin_index && p2 == net1_pin_index )
					continue;
				if( p1 == net1_pin_index )
				{
					// starting pin is on part, unroute first segment
					if( c->seg[0].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net1, ic, 0 );
						nsegs = c->NumSegs();
					}
					// modify vertex position and layer
					c->vtx[0].x = pin_pt2.x;
					c->vtx[0].y = pin_pt2.y;
					c->vtx[0].pad_layer = pin_lay2;
					// now draw
					m_dlist->Set_x( c->seg[0].dl_el, pin_pt2.x );
					m_dlist->Set_y( c->seg[0].dl_el, pin_pt2.y );
					m_dlist->Set_visible( c->seg[0].dl_el, net1->visible );
					m_dlist->Set_x( c->seg[0].dl_sel, pin_pt2.x );
					m_dlist->Set_y( c->seg[0].dl_sel, pin_pt2.y );
					m_dlist->Set_visible( c->seg[0].dl_sel, net1->visible );
				}
				if( p2 == net1_pin_index )
				{
					// ending pin is on part, unroute last segment
					if( c->seg[nsegs-1].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net1, ic, nsegs-1 );
						nsegs = c->NumSegs();
					}
					// modify vertex position and layer
					c->vtx[nsegs].x = pin_pt2.x;
					c->vtx[nsegs].y = pin_pt2.y;
					c->vtx[nsegs].pad_layer = pin_lay2;
					m_dlist->Set_xf( c->seg[nsegs-1].dl_el, pin_pt2.x );
					m_dlist->Set_yf( c->seg[nsegs-1].dl_el, pin_pt2.y );
					m_dlist->Set_visible( c->seg[nsegs-1].dl_el, net1->visible );
					m_dlist->Set_xf( c->seg[nsegs-1].dl_sel, pin_pt2.x );
					m_dlist->Set_yf( c->seg[nsegs-1].dl_sel, pin_pt2.y );
					m_dlist->Set_visible( c->seg[nsegs-1].dl_sel, net1->visible );
				}
				if( p1 == net2_pin_index )
				{
					// starting pin is on part, unroute first segment
					if( c2->seg[0].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net2, ic, 0 );
						nsegs = c2->NumSegs();
					}
					// modify vertex position and layer
					c2->vtx[0].x = pin_pt1.x;
					c2->vtx[0].y = pin_pt1.y;
					c2->vtx[0].pad_layer = pin_lay1;
					// now draw
					m_dlist->Set_x( c2->seg[0].dl_el, pin_pt1.x );
					m_dlist->Set_y( c2->seg[0].dl_el, pin_pt1.y );
					m_dlist->Set_visible( c2->seg[0].dl_el, net2->visible );
					m_dlist->Set_x( c2->seg[0].dl_sel, pin_pt1.x );
					m_dlist->Set_y( c2->seg[0].dl_sel, pin_pt1.y );
					m_dlist->Set_visible( c2->seg[0].dl_sel, net2->visible );
				}
				if( p2 == net2_pin_index )
				{
					// ending pin is on part, unroute last segment
					if( c2->seg[nsegs-1].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net2, ic, nsegs-1 );
						nsegs = c2->NumSegs();
					}
					// modify vertex position and layer
					c2->vtx[nsegs].x = pin_pt1.x;
					c2->vtx[nsegs].y = pin_pt1.y;
					c2->vtx[nsegs].pad_layer = pin_lay1;
					m_dlist->Set_xf( c2->seg[nsegs-1].dl_el, pin_pt1.x );
					m_dlist->Set_yf( c2->seg[nsegs-1].dl_el, pin_pt1.y );
					m_dlist->Set_visible( c2->seg[nsegs-1].dl_el, net2->visible );
					m_dlist->Set_xf( c2->seg[nsegs-1].dl_sel, pin_pt1.x );
					m_dlist->Set_yf( c2->seg[nsegs-1].dl_sel, pin_pt1.y );
					m_dlist->Set_visible( c2->seg[nsegs-1].dl_sel, net2->visible );
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
		for( int ic=0; ic<net1->NumCons(); ic++ )
		{
			cconnect * c1 = net1->connect[ic];
			int nsegs = c1->NumSegs();
			if( nsegs == 1)
			{
				int p1 = c1->start_pin;
				int p2 = c1->end_pin;
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
		for( int ic=0; ic<net1->NumCons(); ic++ )
		{
			cconnect * c1 = net1->connect[ic];
			int nsegs = c1->NumSegs();
			if( nsegs )
			{
				int p1 = c1->start_pin;
				int p2 = c1->end_pin;
				if( p1 == net1_pin_index )
				{
					// starting pin is on part, unroute first segment
					if( c1->seg[0].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net1, ic, 0 );
						nsegs = c1->NumSegs();
					}
					// modify vertex position and layer
					c1->vtx[0].x = pin_pt2.x;
					c1->vtx[0].y = pin_pt2.y;
					c1->vtx[0].pad_layer = pin_lay2;
					// now draw
					m_dlist->Set_x( c1->seg[0].dl_el, pin_pt2.x );
					m_dlist->Set_y( c1->seg[0].dl_el, pin_pt2.y );
					m_dlist->Set_visible( c1->seg[0].dl_el, net1->visible );
					m_dlist->Set_x( c1->seg[0].dl_sel, pin_pt2.x );
					m_dlist->Set_y( c1->seg[0].dl_sel, pin_pt2.y );
					m_dlist->Set_visible( c1->seg[0].dl_sel, net1->visible );
				}
				if( p2 == net1_pin_index )
				{
					// ending pin is on part, unroute last segment
					if( c1->seg[nsegs-1].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net1, ic, nsegs-1 );
						nsegs = c1->NumSegs();
					}
					// modify vertex position and layer
					c1->vtx[nsegs].x = pin_pt2.x;
					c1->vtx[nsegs].y = pin_pt2.y;
					c1->vtx[nsegs].pad_layer = pin_lay2;
					m_dlist->Set_xf( c1->seg[nsegs-1].dl_el, pin_pt2.x );
					m_dlist->Set_yf( c1->seg[nsegs-1].dl_el, pin_pt2.y );
					m_dlist->Set_visible( c1->seg[nsegs-1].dl_el, net1->visible );
					m_dlist->Set_xf( c1->seg[nsegs-1].dl_sel, pin_pt2.x );
					m_dlist->Set_yf( c1->seg[nsegs-1].dl_sel, pin_pt2.y );
					m_dlist->Set_visible( c1->seg[nsegs-1].dl_sel, net1->visible );
				}
			}
		}
		// reassign pin2 to net1
		net1->pin[net1_pin_index].part = part2;
		net1->pin[net1_pin_index].ref_des = part2->ref_des;
		net1->pin[net1_pin_index].pin_name = *pin_name2;
		part2->pin[pin_index2].net = net1;
	}
	else
	{
		// pin2 is unconnected
		part2->pin[pin_index2].net = NULL;
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
		for( int ic=0; ic<net2->NumCons(); ic++ )
		{
			cconnect * c2 = net2->connect[ic];
			int nsegs = c2->NumSegs();
			if( nsegs == 1 )
			{
				int p1 = c2->start_pin;
				int p2 = c2->end_pin;
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
		for( int ic=0; ic<net2->NumCons(); ic++ )
		{
			cconnect * c2 = net2->connect[ic];
			int nsegs = c2->NumSegs();
			if( nsegs )
			{
				int p1 = c2->start_pin;
				int p2 = c2->end_pin;
				if( p1 == net2_pin_index )
				{
					// starting pin is on part, unroute first segment
					if( c2->seg[0].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net2, ic, 0 );
						nsegs = c2->NumSegs();
					}
					// modify vertex position and layer
					c2->vtx[0].x = pin_pt1.x;
					c2->vtx[0].y = pin_pt1.y;
					c2->vtx[0].pad_layer = pin_lay1;
					// now draw
					m_dlist->Set_x( c2->seg[0].dl_el, pin_pt1.x );
					m_dlist->Set_y( c2->seg[0].dl_el, pin_pt1.y );
					m_dlist->Set_visible( c2->seg[0].dl_el, net2->visible );
					m_dlist->Set_x( c2->seg[0].dl_sel, pin_pt1.x );
					m_dlist->Set_y( c2->seg[0].dl_sel, pin_pt1.y );
					m_dlist->Set_visible( c2->seg[0].dl_sel, net2->visible );
				}
				if( p2 == net2_pin_index )
				{
					// ending pin is on part, unroute last segment
					if( c2->seg[nsegs-1].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net2, ic, nsegs-1 );
						nsegs = c2->NumSegs();
					}
					// modify vertex position and layer
					c2->vtx[nsegs].x = pin_pt1.x;
					c2->vtx[nsegs].y = pin_pt1.y;
					c2->vtx[nsegs].pad_layer = pin_lay1;
					m_dlist->Set_xf( c2->seg[nsegs-1].dl_el, pin_pt1.x );
					m_dlist->Set_yf( c2->seg[nsegs-1].dl_el, pin_pt1.y );
					m_dlist->Set_visible( c2->seg[nsegs-1].dl_el, net2->visible );
					m_dlist->Set_xf( c2->seg[nsegs-1].dl_sel, pin_pt1.x );
					m_dlist->Set_yf( c2->seg[nsegs-1].dl_sel, pin_pt1.y );
					m_dlist->Set_visible( c2->seg[nsegs-1].dl_sel, net2->visible );
				}
			}
		}
		// reassign pin1 to net2
		net2->pin[net2_pin_index].part = part1;
		net2->pin[net2_pin_index].ref_des = part1->ref_des;
		net2->pin[net2_pin_index].pin_name = *pin_name1;
		part1->pin[pin_index1].net = net2;
	}
	else
	{
		// pin2 is unconnected
		part1->pin[pin_index1].net = NULL;
	}
	SetAreaConnections( net1 );
	SetAreaConnections( net2 );
#endif
}


// Part moved, so unroute starting and ending segments of connections
// to this part, and update positions of endpoints
// Undraw and Redraw any changed connections
// 
int CNetList::PartMoved( cpart * part )
{
	// first, mark all nets and connections unmodified
	CIterator_cnet iter_net(this);
	cnet * net = iter_net.GetFirst();
	while( net )
	{
		net->utility = 0;
		for( int ic=0; ic<net->NumCons(); ic++ )
			net->connect[ic]->utility = 0;
		net = iter_net.GetNext();
	}
	// disable drawing/undrawing 
	CDisplayList * old_dlist = m_dlist;
	m_dlist = 0;

	// find nets that connect to this part
	for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ ) 
	{
		net = (cnet*)part->pin[ip].net;
		if( net && net->utility == 0 )
		{
			for( int ic=0; ic<net->NumCons(); ic++ )
			{
				cconnect * c = net->connect[ic];
				int nsegs = c->NumSegs();
				if( nsegs )
				{
					// check this connection
					int p1 = c->start_pin;
					int p2 = c->end_pin;
					cseg * s0 = &c->seg[0];
					cvertex * v0 = &c->vtx[0];
					if( p1 != cconnect::NO_END )
					{
						if( net->pin[p1].part == part )
						{
							// start pin is on part, unroute first segment
							CString pin_name1 = net->pin[p1].pin_name;
							int pin_index1 = part->shape->GetPinIndexByName( pin_name1 );
							net->utility = 1;	// mark net modified
							c->utility = 1;		// mark connection modified
							UnrouteSegment( net, ic, 0 );
							nsegs = c->NumSegs();
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
								part->pin[pin_index1].net = net;
						}
					}
					if( p2 != cconnect::NO_END )
					{
						if( net->pin[p2].part == part )
						{
							// end pin is on part, unroute last segment
							net->utility = 1;	// mark net modified
							c->utility = 1;		// mark connection modified
							UnrouteSegment( net, ic, nsegs-1 );
							nsegs = c->NumSegs();
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
								part->pin[pin_index2].net = net;
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
		for( net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
		{
			if( net->utility )
			{
				CIterator_cconnect iter_con( net );
				for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() ) 
				{
					if( c->utility )
						c->Draw();
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
		part->pin[ip].net = NULL;

	// find nets which connect to this part
	CIterator_cnet iter_net(this);
	cnet * net = iter_net.GetFirst();
	while( net )
	{
		// check each connection in net
		for( int ic=net->NumCons()-1; ic>=0; ic-- )
		{
			cconnect * c = net->connect[ic];
			int nsegs = c->NumSegs();
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
					part->pin[pin_index1].net = net;
					// see if position or pad type has changed
					int old_x = c->vtx[0].x;
					int old_y = c->vtx[0].y;
					int old_layer = c->seg[0].m_layer;
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
							nsegs = c->NumSegs();
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
					part->pin[pin_index2].net = net;
					// see if position has changed
					int old_x = c->vtx[nsegs].x;
					int old_y = c->vtx[nsegs].y;
					int old_layer = c->seg[nsegs-1].m_layer;
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
						if( c->seg[nsegs-1].m_layer != LAY_RAT_LINE )
						{
							UnrouteSegment( net, ic, nsegs-1 );
							nsegs = c->NumSegs();
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
			if( net->pin[ip].ref_des == part->ref_des )
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
						part->pin[pin_index].net = net;
						net->pin[ip].part = part;
					}
				}
			}
		}
		RemoveOrphanBranches( net, 0, TRUE );
		net = iter_net.GetNext();
	}
	return 0;
}

// Part deleted, so unroute and remove all connections to this part
// and remove all references from netlist
// 
int CNetList::PartDeleted( cpart * part, BOOL bSetAreas )
{
	// find nets which connect to this part, remove pins and adjust areas
	POSITION pos;
	CString name;
	void * ptr;

	CIterator_cnet iter_net(this);
	for( cnet * net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
		net->utility = 0;


	// find nets which connect to this part
	for( cnet * net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
	{
		for( int ip=0; ip<net->pin.GetSize(); )
		{
			if( net->pin[ip].ref_des == part->ref_des )
			{
				net->RemovePin( &net->pin[ip].ref_des, &net->pin[ip].pin_name, FALSE );
				net->utility = 1;	// mark for SetAreaConnections
			}
			else
				ip++;
		}
		RemoveOrphanBranches( net, 0, TRUE );
	}
	if( bSetAreas )
	{
		for( cnet * net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
			if( net->utility )
				SetAreaConnections( net );
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
			if( net->pin[ip].ref_des == *old_ref_des )
				net->pin[ip].ref_des = *new_ref_des;
		}
	}
}

// Part disconnected, so unroute and remove all connections to this part
// Also remove net pointers from part->pins
// and part pointer from net->pins
// Do not remove pins from netlist, however
// 
int CNetList::PartDisconnected( cpart * part, BOOL bSetAreas )
{
	// find nets which connect to this part, remove pins and adjust areas
	POSITION pos;
	CString name;
	void * ptr;

	CIterator_cnet iter_net(this);
	for( cnet * net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
		net->utility = 0;

	for( cnet * net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
	{
		for( int ip=0; ip<net->pin.GetSize(); ip++ )
		{
			if( net->pin[ip].ref_des == part->ref_des )
			{
				DisconnectNetPin( net, &net->pin[ip].ref_des, &net->pin[ip].pin_name, FALSE );
				net->utility = 1;
			}
		}
		RemoveOrphanBranches( net, 0, TRUE );
	}
	if( bSetAreas )
	{
		for( cnet * net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
			if( net->utility )
				SetAreaConnections( net );
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
void CNetList::OptimizeConnections( BOOL bBelowPinCount, int pin_count, BOOL bVisibleNetsOnly )
{
	// traverse map
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		if( !bVisibleNetsOnly || net->visible )
			OptimizeConnections( net, -1, bBelowPinCount, pin_count, bVisibleNetsOnly );
	}
}

// optimize all unrouted connections for a part
//
void CNetList::OptimizeConnections( cpart * part, BOOL bBelowPinCount, int pin_count, BOOL bVisibleNetsOnly )
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
					OptimizeConnections( net, -1, bBelowPinCount, pin_count, bVisibleNetsOnly );
					net->utility = 1;
				}
			}
		}
	}
}


// optimize the unrouted connections for a net
// if ic_track >= 0, returns new ic corresponding to old ic or -1 if unable
//
int CNetList::OptimizeConnections( cnet * net, int ic_track, BOOL bBelowPinCount, 
								  int pin_count, BOOL bVisibleNetsOnly )
{
#ifdef PROFILE
	StartTimer();	//****
#endif

	// see if we need to do this
	if( bVisibleNetsOnly && net->visible == 0 )
		return ic_track;
	if( bBelowPinCount && net->NumPins() >= pin_count )
		return ic_track;

	// record id of connection to track
	id id_track;
	if( ic_track != -1 )
		id_track = net->ConByIndex( ic_track )->Id();

	// get number of pins N and make grid[NxN] array and pair[N*2] array
	int npins = net->NumPins();
	char * grid = (char*)calloc( npins*npins, sizeof(char) );
	for( int ip=0; ip<npins; ip++ )
		grid[ip*npins+ip] = 1;
	CArray<int> pair;			// use CArray because size is unknown,
	pair.SetSize( 2*npins );	// although this should be plenty

	// first, flag ic_track if requested
	int ic_new = -1;
	if( ic_track >= 0 && ic_track < net->NumCons() )
	{
		for( int ic=0; ic<net->NumCons(); ic++ )
			net->connect[ic]->utility = 0;
		net->connect[ic_track]->utility = 1;		// flag connection
	}

	// go through net, deleting unrouted and unlocked connections
	CIterator_cconnect iter_con( net );
	for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
	{
		if( c->NumSegs() == 0 )
		{
			// this is an error, fix it
			ASSERT(0);
			net->RemoveConnectAdjustTees( c );
		}
		else
		{
			// flag routed or partially routed connections
			int routed = 0;
			if( c->NumSegs() > 1						// at least partially routed
				|| c->seg[0].m_layer != LAY_RAT_LINE	// first segment routed
				|| c->end_pin == cconnect::NO_END		// ends on stub or tee-vertex
				|| c->start_pin == cconnect::NO_END		// starts on stub or tee-vertex
				)
				routed = 1;
			if( !routed && !c->locked )
			{
				// unrouted and unlocked, so delete connection, adjust tees
				net->RemoveConnectAdjustTees( c );
			}
		}
	}

	// go through net again, recording pins of routed or locked connections
	CMap<int, int, int, int> tee_ids_analyzed;	// map of tee_id's analyzed
	for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
	{
		// record pins in pair[] and grid[]
		int p1 = c->start_pin;
		int p2 = c->end_pin;
		if( p1 != cconnect::NO_END && p2 != cconnect::NO_END )
		{
			// unrouted pin-pin connection, record pins as connected
			AddPinsToGrid( grid, p1, p2, npins );
		}
		else
		{
			// find connections between pins and tee-vertices
			CMap<int, int, int, int> tee_ids_connected;	// list of connected tee_ids
			CMap<int, int, int, int> pins_connected;	// list of connected pins
			// check for connection between pin and tee-vertex
			int tee_id = abs( c->FirstVtx()->tee_ID );
			int pin_ix = c->LastVtx()->GetNetPinIndex();
			if( tee_id == 0 || pin_ix == -1 )
			{
				tee_id = abs( c->LastVtx()->tee_ID );
				pin_ix = c->FirstVtx()->GetNetPinIndex();
			}
			if( tee_id > 0 && pin_ix != -1 )
			{
				// connection between pin and tee-vertex, see if this tee_id already analyzed
				int dummy;
				if( !tee_ids_analyzed.Lookup( tee_id, dummy ) )
				{
					// tee_id not found, start analyzing all connections with this tee_id,
					// record any connected tee_ids and pin_indexes
					tee_ids_analyzed.SetAt( tee_id, tee_id );
					tee_ids_connected.SetAt( tee_id, tee_id );
					pins_connected.SetAt( pin_ix, pin_ix );
					// start by looking for connected tees
					CIterator_cconnect iter_con2( net );
					for( cconnect * test_c=iter_con2.GetFirst(); test_c; test_c=iter_con2.GetNext() )
					{
						// check for tees at both ends
						int first_tee_id = abs( test_c->FirstVtx()->tee_ID );
						int last_tee_id = abs( test_c->LastVtx()->tee_ID );
						if( first_tee_id > 0 && last_tee_id > 0 )
						{
							// list the tee_ids as connected and analyzed
							if( tee_ids_connected.Lookup( first_tee_id, dummy ) )
							{
								tee_ids_connected.SetAt( last_tee_id, last_tee_id );
								tee_ids_analyzed.SetAt( last_tee_id, last_tee_id );
							}
							else if( tee_ids_connected.Lookup( last_tee_id, dummy ) )
							{
								tee_ids_connected.SetAt( first_tee_id, first_tee_id );
								tee_ids_analyzed.SetAt( first_tee_id, first_tee_id );
							}
						}
					}
					// now look for connected pins
					for( cconnect * test_c=iter_con2.GetFirst(); test_c; test_c=iter_con2.GetNext() )
					{
						// see if first vertex is a tee
						int first_tee_id = abs( test_c->FirstVtx()->tee_ID );
						if( first_tee_id > 0 && tee_ids_connected.Lookup( first_tee_id, dummy ) )
						{
							// first vertex is a connected tee, check last vertex
							int last_pin_ix = abs( test_c->LastVtx()->GetNetPinIndex() );
							if( last_pin_ix != -1 )
							{
								// last vertex is a pin, add to list of connected pins
								pins_connected.SetAt( last_pin_ix, last_pin_ix );
							}
						}
						// now repeat for last vertex
						int last_tee_id = abs( test_c->LastVtx()->tee_ID );
						if( last_tee_id > 0 && tee_ids_connected.Lookup( last_tee_id, dummy ) )
						{
							// last vertex is a connected tee, check first vertex
							int first_pin_ix = abs( test_c->FirstVtx()->GetNetPinIndex() );
							if( first_pin_ix != -1 )
							{
								// first vertex is a pin, add to list of connected pins
								pins_connected.SetAt( first_pin_ix, first_pin_ix );
							}
						}
					}
					// now loop through all pins and mark them as connected to first pin
					int m_pins = pins_connected.GetCount();
					if( m_pins > 1 )
					{
						POSITION pos = pins_connected.GetStartPosition();
						int    nKey, nValue;
						pins_connected.GetNextAssoc( pos, nKey, nValue );
						int p1 = nValue;
						while (pos != NULL)
						{
							pins_connected.GetNextAssoc( pos, nKey, nValue );
							int p2 = nValue;
							AddPinsToGrid( grid, p1, p2, npins );
						}
					}
				}
			}
		}
	}

	// find ic_track if still present
	if( ic_track >= 0 )
	{
		if( id_track.Resolve() )
		{
			ic_new = id_track.I2();
		}
	}

	//** TEMP
	if( net->visible == 0 )
	{
		free( grid );
		return ic_new;
	}

	// now add pins connected to copper areas
	for( int ia=0; ia<net->NumAreas(); ia++ )
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
				cconnect * c = net->connect[ic];
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
				cconnect * c = net->connect[ic];
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
		net->AddConnectFromPinToPin( p1, p2 );
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
	for( int ip=0; ip<net->NumPins(); ip++ )
	{
		CString ref_des = net->pin[ip].ref_des;
		CString pin_name = net->pin[ip].pin_name;
		cpart * part = m_plist->GetPart( ref_des );
		if( part )
		{
			if( part->shape )
			{
				int pin_index = part->shape->GetPinIndexByName( pin_name );
				if( pin_index != -1 )
					part->pin[pin_index].net = net;
			}
		}
	}
	return 0;
}

void CNetList::SetViaVisible( cnet * net, int ic, int iv, BOOL visible )
{
	cconnect * c = net->connect[ic];
	cvertex * v = &c->vtx[iv];
	for( int il=0; il<v->dl_el.GetSize(); il++ )
		if( v->dl_el[il] )
			v->dl_el[il]->visible = visible;
	if( v->dl_hole )
		v->dl_hole->visible = visible;
}

#if 0
// cancel dragging end vertex of a stub trace
//
void CNetList::CancelDraggingEndVertex( cnet * net, int ic, int ivtx )
{
	cconnect * c = net->connect[ic];
	m_dlist->StopDragging();
	c->seg[ivtx-1].dl_el->visible = 1;
	SetViaVisible( net, ic, ivtx, TRUE );
	for( int ia=0; ia<net->NumAreas(); ia++ )
		for( int iv=0; iv<net->area[ia].nvias; iv++ )
			if( net->area[ia].vcon[iv] == ic )
				if( net->area[ia].vtx[iv] == ivtx )
					if( net->area[ia].dl_via_thermal[iv] != 0 )
						m_dlist->Set_visible( net->area[ia].dl_via_thermal[iv], 1 );
}
#endif

// move vertex
//
void CNetList::MoveVertex( cnet * net, int ic, int ivtx, int x, int y )
{
	cconnect * c = net->connect[ic];
	BOOL bWasDrawn = c->m_bDrawn;
	c->Undraw();
	if( ivtx > c->NumSegs() )
		ASSERT(0);
	cvertex * v = &c->vtx[ivtx];
	m_dlist->StopDragging();
	v->x = x;
	v->y = y;
	if( v->GetType() == cvertex::V_TEE )
	{
		// this is a tee-vertex in a trace
		// move other slave vertices connected to it
		int tee_id = v->tee_ID;
		CIterator_cconnect iter_con( net );
		for( cconnect * test_c=iter_con.GetFirst(); test_c; test_c= iter_con.GetNext() )
		{
			int icc = iter_con.GetIndex();
			cvertex * test_v = test_c->FirstVtx();
			if( test_v->GetType() == cvertex::V_SLAVE && test_v->tee_ID == -tee_id )
			{
				MoveVertex( net, icc, 0, x, y );
			}
			test_v = test_c->LastVtx();
			if( test_v->GetType() == cvertex::V_SLAVE && test_v->tee_ID == -tee_id )
			{
				MoveVertex( net, icc, test_c->NumSegs(), x, y );
			}
		}
	}
	ReconcileVia( net, ic, ivtx );
	if( bWasDrawn )
		c->Draw();
	for( int ia=0; ia<net->NumAreas(); ia++ )
		SetAreaConnections( net, ia );
}

// Start dragging trace vertex
//
int CNetList::StartDraggingVertex( CDC * pDC, cnet * net, int ic, int ivtx,
								   int x, int y, int crosshair )
{
	// cancel previous selection and make segments and via invisible
	cconnect * c =net->connect[ic];
	cvertex * v = &c->vtx[ivtx];
	m_dlist->CancelHighLight();
	if( ivtx > 0 )
	{
		m_dlist->Set_visible(c->seg[ivtx-1].dl_el, 0);
	}
	if( ivtx < c->NumSegs() )
	{
		m_dlist->Set_visible(c->seg[ivtx].dl_el, 0);
	}
	SetViaVisible( net, ic, ivtx, FALSE );
	for( int ia=0; ia<net->NumAreas(); ia++ )
	{
		carea * a = &net->area[ia];
		for( int iv=0; iv<a->nvias; iv++ )
		{
			int vic = a->vcon[iv];
			int viv = a->vtx[iv];
			if( a->vcon[iv] == ic && a->vtx[iv] == ivtx && a->dl_via_thermal[iv] != 0 )
				m_dlist->Set_visible( net->area[ia].dl_via_thermal[iv], 0 );
		}
	}

	// if tee connection, also drag tee segment(s)
	if( v->tee_ID )
	{
		int ntsegs = 0;
		// find all tee segments
		cvertex * vv, * vv2;
		for( int icc=0; icc<net->NumCons(); icc++ )
		{
			cconnect * cc = net->connect[icc];
			for( int i=0; i<2; i++ )
			{
				if( i==0 )
					vv = cc->FirstVtx();
				else
					vv = cc->LastVtx();
				if( vv->GetType() == cvertex::V_SLAVE && abs(vv->tee_ID) == abs(v->tee_ID) )
				{
					ntsegs++;
				}
			}
		}
		m_dlist->MakeDragRatlineArray( ntsegs, 0 );
		// now add them one-by-one
		for( int icc=0; icc<net->NumCons(); icc++ )
		{
			cconnect * cc = net->connect[icc];
			if( cc->NumSegs() > 0 )
			{
				for( int i=0; i<2; i++ )
				{
					if( i==0 )
					{
						vv = cc->FirstVtx();	// vertex to drag
						vv2 = &cc->vtx[1];		// other end of segment
						if( vv->GetType() == cvertex::V_SLAVE && abs(vv->tee_ID) == abs(v->tee_ID) )
						{
							CPoint pi, pf;
							pi.x = vv2->x;
							pi.y = vv2->y;
							pf.x = 0;
							pf.y = 0;
							m_dlist->AddDragRatline( pi, pf );
							m_dlist->Set_visible( cc->seg[0].dl_el, 0 );
						}
					}
					else
					{
						vv = cc->LastVtx();					// vertex to drag
						vv2 = &cc->vtx[cc->NumVtxs()-2];	// other end of segment
						if( vv->GetType() == cvertex::V_SLAVE && abs(vv->tee_ID) == abs(v->tee_ID) )
						{
							CPoint pi, pf;
							pi.x = vv2->x;
							pi.y = vv2->y;
							pf.x = 0;
							pf.y = 0;
							m_dlist->AddDragRatline( pi, pf );
							m_dlist->Set_visible( cc->seg[cc->NumSegs()-1].dl_el, 0 );
						}
					}
				}
			}
		}
		m_dlist->StartDraggingArray( pDC, 0, 0, 0, LAY_RAT_LINE );
	}

	// start dragging
	if( ivtx < c->NumSegs() )
	{
		// vertex with segments on both sides
		int xi = c->vtx[ivtx-1].x;
		int yi = c->vtx[ivtx-1].y;
		int xf = c->vtx[ivtx+1].x;
		int yf = c->vtx[ivtx+1].y;
		int layer1 = c->seg[ivtx-1].m_layer;
		int layer2 = c->seg[ivtx].m_layer;
		int w1 = c->seg[ivtx-1].m_width;
		int w2 = c->seg[ivtx].m_width;
		m_dlist->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, layer1, 
									layer2, w1, w2, DSS_STRAIGHT, DSS_STRAIGHT, 
									0, 0, 0, 0, crosshair );
	}
	else
	{
		// end-vertex, only drag one segment
		int xi = c->vtx[ivtx-1].x;
		int yi = c->vtx[ivtx-1].y;
		int layer1 = c->seg[ivtx-1].m_layer;
		int w1 = c->seg[ivtx-1].m_width;
		m_dlist->StartDraggingLine( pDC, x, y, xi, yi, layer1, 
									w1, layer1, 0, 0, crosshair, DSS_STRAIGHT, 0 );
	}
	return 0;
}

// Start moving trace segment
//
int CNetList::StartMovingSegment( CDC * pDC, cnet * net, int ic, int ivtx,
								   int x, int y, int crosshair, int use_third_segment )
{
	// cancel previous selection and make segments and via invisible
	cconnect * c =net->connect[ic];
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
	for( int ia=0; ia<net->NumAreas(); ia++ )
	{
		carea * a = &net->area[ia];
		for( int iv=0; iv<a->nvias; iv++ )
		{
			int vic = a->vcon[iv];
			int viv = a->vtx[iv];
			if( a->vcon[iv] == ic && (a->vtx[iv] == ivtx - 1 || a->vtx[iv]== ivtx || a->vtx[iv] == ivtx + 1)  && a->dl_via_thermal[iv] != 0 )
				m_dlist->Set_visible( net->area[ia].dl_via_thermal[iv], 0 );
		}
	}

	// start dragging
	ASSERT(ivtx > 0);

	int	xb = c->vtx[ivtx-1].x;
	int	yb = c->vtx[ivtx-1].y;
	int xi = c->vtx[ivtx  ].x;
	int yi = c->vtx[ivtx  ].y;
	int xf = c->vtx[ivtx+1].x;
	int yf = c->vtx[ivtx+1].y;
	

	int layer0 = c->seg[ivtx-1].m_layer;
	int layer1 = c->seg[ivtx].m_layer;

	int w0 = c->seg[ivtx-1].m_width;
	int w1 = c->seg[ivtx].m_width;

	int xe = 0, ye = 0;
	int layer2 = 0;
	int w2 = 0;
	if(use_third_segment)
	{
		xe = c->vtx[ivtx+2].x;
		ye = c->vtx[ivtx+2].y;
		layer2 = c->seg[ivtx+1].m_layer;
		w2 = c->seg[ivtx+1].m_width;
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
								   int x, int y, int layer1, int layer2, int w, 
								   int layer_no_via, int via_w, int via_hole_w, int dir,
								   int crosshair )
{
	// cancel previous selection and make segment invisible
	cconnect * c = net->connect[ic];
	m_dlist->CancelHighLight();
	m_dlist->Set_visible(c->seg[iseg].dl_el, 0);
	// start dragging
	int xi = c->vtx[iseg].x;
	int yi = c->vtx[iseg].y;
	int xf = c->vtx[iseg+1].x;
	int yf = c->vtx[iseg+1].y;
	m_dlist->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, layer1, 
								layer2, w, 1, DSS_STRAIGHT, DSS_STRAIGHT, 
								layer_no_via, via_w, via_hole_w, dir, crosshair );
	return 0;
}

// Start dragging new vertex in existing trace segment
//
int CNetList::StartDraggingSegmentNewVertex( CDC * pDC, cnet * net, int ic, int iseg,
								   int x, int y, int layer, int w, int crosshair )
{
	// cancel previous selection and make segment invisible
	cconnect * c = net->connect[ic];
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
	cconnect * c = net->connect[ic];
	m_dlist->CancelHighLight();
	SetViaVisible( net, ic, iseg, FALSE );
	for( int ia=0; ia<net->NumAreas(); ia++ )
		for( int iv=0; iv<net->area[ia].nvias; iv++ )
			if( net->area[ia].vcon[iv] == ic )
				if( net->area[ia].dl_via_thermal[iv] != 0 )
					m_dlist->Set_visible( net->area[ia].dl_via_thermal[iv], 0 );
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
	cconnect * c = net->connect[ic];
	SetViaVisible( net, ic, iseg, TRUE );
	for( int ia=0; ia<net->NumAreas(); ia++ )
		for( int iv=0; iv<net->area[ia].nvias; iv++ )
			if( net->area[ia].vcon[iv] == ic )
				if( net->area[ia].dl_via_thermal[iv] != 0 )
					m_dlist->Set_visible( net->area[ia].dl_via_thermal[iv], 1 );
	m_dlist->StopDragging();
	SetAreaConnections( net );
}

// Start dragging copper area corner to move it
//
int CNetList::StartDraggingAreaCorner( CDC *pDC, cnet * net, int iarea, int icorner, int x, int y, int crosshair )
{
	net->area[iarea].StartDraggingToMoveCorner( pDC, icorner, x, y, crosshair );
	return 0;
}

// Start dragging inserted copper area corner
//
int CNetList::StartDraggingInsertedAreaCorner( CDC *pDC, cnet * net, int iarea, int icorner, int x, int y, int crosshair )
{
	net->area[iarea].StartDraggingToInsertCorner( pDC, icorner, x, y, crosshair );
	return 0;
}

// Cancel dragging inserted area corner
//
int CNetList::CancelDraggingInsertedAreaCorner( cnet * net, int iarea, int icorner )
{
	net->area[iarea].CancelDraggingToInsertCorner( icorner );
	return 0;
}

// Cancel dragging area corner
//
int CNetList::CancelDraggingAreaCorner( cnet * net, int iarea, int icorner )
{
	net->area[iarea].CancelDraggingToMoveCorner( icorner );
	return 0;
}

// Cancel dragging segment
//
int CNetList::CancelDraggingSegment( cnet * net, int ic, int iseg )
{
	// make segment visible
	cconnect * c = net->connect[ic];
	m_dlist->Set_visible(c->seg[iseg].dl_el, 1);
	m_dlist->StopDragging();
	return 0;
}

// Cancel dragging vertex
//
int CNetList::CancelDraggingVertex( cnet * net, int ic, int ivtx )
{
	// make segments and via visible
	cconnect * c = net->connect[ic];
	cvertex * v = &c->vtx[ivtx];
	m_dlist->Set_visible(c->seg[ivtx-1].dl_el, 1);
	if( ivtx < c->NumSegs() )
		m_dlist->Set_visible(c->seg[ivtx].dl_el, 1);
	SetViaVisible( net, ic, ivtx, TRUE );
	for( int ia=0; ia<net->NumAreas(); ia++ )
		for( int iv=0; iv<net->area[ia].nvias; iv++ )
			if( net->area[ia].vcon[iv] == ic )
				if( net->area[ia].dl_via_thermal[iv] != 0 )
					m_dlist->Set_visible( net->area[ia].dl_via_thermal[iv], 1 );
	// if tee, make connecting stubs visible
	if( v->tee_ID )
	{
		for( int icc=0; icc<net->NumCons(); icc++ )
		{
			cconnect * cc = net->connect[icc];
			if( cc != c && cc->end_pin == cconnect::NO_END )
			{
				cvertex * vv = &cc->vtx[cc->NumSegs()];
				if( vv->tee_ID == v->tee_ID )
				{
					m_dlist->Set_visible( cc->seg[cc->NumSegs()-1].dl_el, 1 );
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
	cconnect * c = net->connect[ic];
	cvertex * v = &c->vtx[ivtx];

	// make segments and via visible
	m_dlist->Set_visible(c->seg[ivtx-1].dl_el, 1);
	m_dlist->Set_visible(c->seg[ivtx].dl_el, 1);
	if( m_dlist->Dragging_third_segment() )
		m_dlist->Set_visible(c->seg[ivtx+1].dl_el, 1);
	SetViaVisible( net, ic, ivtx,   TRUE );
	SetViaVisible( net, ic, ivtx+1, TRUE );
	for( int ia=0; ia<net->NumAreas(); ia++ )
	{
		carea * a = &net->area[ia];
		for( int iv=0; iv<a->nvias; iv++ )
		{
			int vic = a->vcon[iv];
			int viv = a->vtx[iv];
			if( a->vcon[iv] == ic && (a->vtx[iv] == ivtx - 1 || a->vtx[iv]== ivtx || a->vtx[iv] == ivtx + 1)  && a->dl_via_thermal[iv] != 0 )
				m_dlist->Set_visible( net->area[ia].dl_via_thermal[iv], 1 );
		}
	}
	m_dlist->StopDragging();
	return 0;
}


// Cancel dragging vertex inserted into segment
//
int CNetList::CancelDraggingSegmentNewVertex( cnet * net, int ic, int iseg )
{
	// make segment visible
	cconnect * c = net->connect[ic];
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
	cconnect * c = net->connect[ic];
	cvertex * v = &c->vtx[iv];

	// check for end vertices of traces to pads
	if( iv == 0 )
		return status;
	if( c->end_pin != cconnect::NO_END  && iv == (c->NumSegs() + 1) )
		return status;

	// check for normal via pad
	if( v->via_w == 0 && v->tee_ID == 0 )
		return status;

	// check for via pad at end of branch
	if( v->tee_ID != 0 && iv == c->NumSegs() && c->seg[iv-1].m_layer == layer )
		if( !TeeViaNeeded( net, v->tee_ID, NULL ) )
			return status;

	// check for trace connection to via pad
	c = net->connect[ic];
	if( c->seg[iv-1].m_layer == layer )
		status |= VIA_TRACE;
	if( iv < c->NumSegs() )
		if( c->seg[iv].m_layer == layer )
			status |= VIA_TRACE;

	// see if it connects to any area in this net on this layer
	for( int ia=0; ia<net->NumAreas(); ia++ )
	{
		// next area
		carea * a = &net->area[ia];
		if( a->Layer() == layer )
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
	cconnect * c = net->connect[ic];
	cvertex * v = &c->vtx[iv];
	int w = v->via_w;
	int hole_w = v->via_hole_w;
	if( layer > LAY_BOTTOM_COPPER )
	{
		// inner layer
		if( con_status == VIA_NO_CONNECT )
		{
			w = 0;	// no connection, no pad
		}
		else if( v->tee_ID && iv == c->NumSegs() )
		{
			// end-vertex of branch, find tee-vertex that it connects to
			int tee_ic, tee_iv;
			if( FindTeeVertexInNet( net, v->tee_ID, &tee_ic, &tee_iv ) )
			{
				w = net->connect[tee_ic]->vtx[tee_iv].via_w;
				hole_w = net->connect[tee_ic]->vtx[tee_iv].via_hole_w;
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
BOOL CNetList::TestHitOnVertex( cnet * net, int layer, int x, int y, 
		cnet ** hit_net, int * hit_ic, int * hit_iv )
{
	// loop through all connections
	for( int ic=0; ic<net->NumCons(); ic++ )
	{
		cconnect * c = net->connect[ic];
		if( c->NumSegs() > 0 )
		{
		for( int iv=0; iv<=c->NumSegs(); iv++ )
		{
			cvertex * v = &c->vtx[iv];
			cseg * pre_s;
			cseg * post_s;
			if( iv == 0 )
			{
				post_s = &c->seg[iv];
				pre_s = post_s;
			}
			else if( iv == c->NumSegs() )
			{
				pre_s = &c->seg[iv-1];
				post_s = pre_s;
			}
			else
			{
				pre_s = &c->seg[iv-1];
				post_s = &c->seg[iv];
			}
			if( v->via_w > 0 || layer == 0 || layer == pre_s->m_layer || layer == post_s->m_layer
				|| (pre_s->m_layer == LAY_RAT_LINE && post_s->m_layer == LAY_RAT_LINE) )
			{
				int test_w = max( v->via_w, pre_s->m_width );
				test_w = max( test_w, post_s->m_width );
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
	}
	return FALSE;
}

// add empty copper area to net
// return index to area (zero-based)
//
int CNetList::AddArea( cnet * net, int layer, int x, int y, int hatch, BOOL bDraw )
{
	
	int old_nareas = net->NumAreas();
	net->area.SetSize( old_nareas+1 );
	carea * a = &net->area[old_nareas];
	a->Initialize( m_dlist, net );
	id area_id = a->Id();
	net->area[old_nareas].Start( layer, 1, 10*NM_PER_MIL, x, y, 
		hatch, &area_id, net, bDraw );
	return old_nareas;
}

// add empty copper area to net, inserting at net->area[iarea]
//
void CNetList::InsertArea( cnet * net, int iarea, int layer, int x, int y, int hatch, BOOL bDraw )
{
	// make new area and insert it into area array
	carea test;
	net->area.InsertAt( iarea, test ) ;
	carea * a = &net->area[iarea];
	a->Initialize( m_dlist, net );
	id area_id( ID_NET, net->UID(), ID_AREA, a->UID(), iarea );
	net->area[iarea].Start( layer, 1, 10*NM_PER_MIL, x, y,
		hatch, &area_id, net, bDraw );
}

// add corner to copper area, apply style to preceding side
//
int CNetList::AppendAreaCorner( cnet * net, int iarea, int x, int y, int style )
{
	carea * a = &net->area[iarea];
	a->AppendCorner( x, y, style );
	return 0;
}

// insert corner into copper area, apply style to preceding side
//
int CNetList::InsertAreaCorner( cnet * net, int iarea, int icorner, 
							int x, int y, int style )
{
	carea * a = &net->area[iarea];
	if( icorner == a->NumCorners() && !a->Closed() )
	{
		a->AppendCorner( x, y, style );
		ASSERT(0);	// this is now an error, should be using AppendAreaCorner
	}
	else
	{
		a->InsertCorner( icorner, x, y );
		a->SetSideStyle( icorner-1, style );
	}
	return 0;
}

// move copper area corner
//
void CNetList::MoveAreaCorner( cnet * net, int iarea, int icorner, int x, int y )
{
	net->area[iarea].MoveCorner( icorner, x, y );
}

// highlight
//
void CNetList::HighlightAreaCorner( cnet * net, int iarea, int icorner )
{
	net->area[iarea].HighlightCorner( icorner );
}

// get copper area corner coords
//
CPoint CNetList::GetAreaCorner( cnet * net, int iarea, int icorner )
{
	CPoint pt;
	pt.x = net->area[iarea].X( icorner );
	pt.y = net->area[iarea].Y( icorner );
	return pt;
}

// complete copper area contour by adding line to first corner
//
int CNetList::CompleteArea( cnet * net, int iarea, int style )
{
	carea * a = &net->area[iarea];
	if( a->NumCorners() > 2 )
	{
		a->Close( style );
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
		for( int ia=0; ia<net->NumAreas(); ia++ )
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
				if( (cnet*)part->pin[ipp].net == net )
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
	for( int ip=0; ip<area->dl_thermal.GetSize(); ip++ )
		m_dlist->Remove( area->dl_thermal[ip] );
	for( int is=0; is<area->dl_via_thermal.GetSize(); is++ )
		m_dlist->Remove( area->dl_via_thermal[is] );
	area->npins = 0;
	area->nvias = 0;
	area->pin.SetSize(0);
	area->dl_thermal.SetSize(0);
	area->vcon.SetSize(0);
	area->vtx.SetSize(0);
	area->dl_via_thermal.SetSize(0);

	// test all pins in net for being inside copper area 
	id id( ID_NET, net->UID(), ID_AREA, area->UID(), iarea, ID_PIN_X );
	int area_layer = area->Layer();	// layer of copper area
	for( int ip=0; ip<net->NumPins(); ip++ )
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
					int pin_layer = m_plist->GetPinLayer( part, &part_pin_name );
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
					if( area->TestPointInside( p.x, p.y ) )
					{
						// pin is inside copper area
						cnet * part_pin_net = part->pin[pin_index].net;
						if( part_pin_net != net )
							ASSERT(0);	// inconsistency between part->pin->net and net->pin->part
						area->pin.SetSize( area->npins+1 );
						area->pin[area->npins] = ip;
						id.SetI3( ip );
						int w = m_plist->GetPinWidth( part, &part_pin_name );
						if( m_dlist )
						{
							dl_element * dl = m_dlist->Add( id, net, LAY_RAT_LINE, DL_X, net->visible,
								2*w/3, 0, 0, p.x, p.y, 0, 0, 0, 0 );
							area->dl_thermal.SetAtGrow(area->npins, dl );
						}
						area->npins++;
					}
				}
			}
		}
	}
	// test all vias in traces for being inside copper area,
	// also test all end-vertices of non-branch stubs for being on same layer
	id.SetT3( ID_STUB_X );
	for( int ic=0; ic<net->NumCons(); ic++ ) 
	{
		cconnect * c = net->connect[ic];
		int nsegs = c->NumSegs();
		int nvtx = nsegs;
		if( c->end_pin == cconnect::NO_END )
			nvtx++;
		for( int iv=1; iv<nvtx; iv++ )
		{
			cvertex * v = &c->vtx[iv];
			if( v->via_w || c->seg[nsegs-1].m_layer == area->Layer() )
			{
				// via or on same layer as copper area
				int x = v->x;
				int y = v->y;
				if( area->TestPointInside( x, y ) )
				{
					// end point of trace is inside copper area
					area->vcon.SetSize( area->nvias+1 );
					area->vcon[area->nvias] = ic;
					area->vtx.SetSize( area->nvias+1 );
					area->vtx[area->nvias] = iv;
					id.SetI3( ic );
					int w = v->via_w;
					if( !w )
						w = c->seg[iv-1].m_width + 10*NM_PER_MIL;
					if( m_dlist )
					{
						dl_element * dl = m_dlist->Add( id, net, LAY_RAT_LINE, DL_X, net->visible,
							2*w/3, 0, 0, x, y, 0, 0, 0, 0 );
						area->dl_via_thermal.SetAtGrow(area->nvias, dl );
					}
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
	for( int ia=0; ia<net->NumAreas(); ia++ )
	{
		carea * a = &net->area[ia];
		if( (a->Layer() == layer || layer == LAY_PAD_THRU)&& a->TestPointInside( x, y ) )
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

cnet * CNetList::GetNetPtrByUID( int uid )
{
	CIterator_cnet iter_net( this );
	for( cnet * net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
	{
		if( net->m_id.U1() == uid )
			return net;
	}
	return NULL;
}

// Select copper area side
void CNetList::SelectAreaSide( cnet * net, int iarea, int iside )
{
	m_dlist->CancelHighLight();
	net->area[iarea].HighlightSide( iside );
}

// Select copper area corner
void CNetList::SelectAreaCorner( cnet * net, int iarea, int icorner )
{
	m_dlist->CancelHighLight();
	net->area[iarea].HighlightCorner( icorner );
}

// Set style for area side
void CNetList::SetAreaSideStyle( cnet * net, int iarea, int iside, int style )
{
	m_dlist->CancelHighLight();
	net->area[iarea].SetSideStyle( iside, style );
	net->area[iarea].HighlightSide( iside );
}


// Select all connections in net
void CNetList::HighlightNetConnections( cnet * net )
{
	for( int ic=0; ic<net->NumCons(); ic++ )
		HighlightConnection( net, ic );
}

// Select connection
void CNetList::HighlightConnection( cnet * net, int ic )
{
	cconnect * c = net->connect[ic];
	for( int is=0; is<c->seg.GetSize(); is++ )
		HighlightSegment( net, ic, is );
}

// Select segment
void CNetList::HighlightSegment( cnet * net, int ic, int iseg )
{
	cconnect * c = net->connect[ic];
	m_dlist->HighLight( DL_LINE, m_dlist->Get_x(c->seg[iseg].dl_el), 
								m_dlist->Get_y(c->seg[iseg].dl_el),
								m_dlist->Get_xf(c->seg[iseg].dl_el),
								m_dlist->Get_yf(c->seg[iseg].dl_el),
								m_dlist->Get_w(c->seg[iseg].dl_el),
								m_dlist->Get_layer(c->seg[iseg].dl_el) );

}

// Select vertex
void CNetList::HighlightVertex( cnet * net, int ic, int ivtx )
{
	cvertex * v = &net->ConByIndex(ic)->VtxByIndex(ivtx);
	v->Highlight();
}

// Highlight all sides of area
void CNetList::HighlightAreaSides( cnet * net, int ia )
{
	carea * a = &net->area[ia];
	int nsides = a->NumSides();
	for( int is=0; is<nsides; is++ )
		a->HighlightSide( is );
}

// Highlight entire net
void CNetList::HighlightNet( cnet * net)
{
	HighlightNetConnections( net );
	for( int ia=0;ia<net->NumAreas(); ia++ )
		HighlightAreaSides( net, ia );
}

// force a via on a vertex
int CNetList::ForceVia( cnet * net, int ic, int ivtx, BOOL set_areas )
{
	cconnect * c = net->connect[ic];
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
	cconnect * c = net->connect[ic];
	c->vtx[ivtx].force_via_flag = 0;
	ReconcileVia( net, ic, ivtx);
	if( set_areas )
		SetAreaConnections( net );
	return 0;
}

// Reconcile via with preceding and following segments
// if a via is needed, use defaults for adjacent segments 
//
int CNetList::ReconcileVia( cnet * net, int ic, int ivtx, BOOL bDrawVertex )
{
	cconnect * c = net->connect[ic];
	cvertex * v = &c->vtx[ivtx];
	cvertex * v_draw = v;
	BOOL via_needed = FALSE;
	// see if via needed
	if( v->GetType() == cvertex::V_PIN )
	{
		// never draw a pin vertex
	}
	else if( v->force_via_flag ) 
	{
		via_needed = TRUE;
	}
	else if( v->GetType() == cvertex::V_TEE || v->GetType() == cvertex::V_SLAVE )
	{
		// tee-vertex must be at end of a trace
		if( ivtx != 0 && ivtx != c->NumSegs() )
		{
			ASSERT(0);
		}
		else if( TeeViaNeeded( net, abs( v->tee_ID ), &v_draw ) )
		{
			via_needed = TRUE;
		}
	}
	else if( v->GetType() == cvertex::V_TRACE )
	{
		if( ivtx == 0 || ivtx == c->NumSegs() )
		{
			ASSERT(0);
		}
		else
		{
			c->vtx[ivtx].pad_layer = 0;
			cseg * s1 = &c->seg[ivtx-1];
			cseg * s2 = &c->seg[ivtx];
			if(	   s1->m_layer != s2->m_layer 
				&& s1->m_layer != LAY_RAT_LINE 
				&& s2->m_layer != LAY_RAT_LINE )
			{
				via_needed = TRUE;
			}
		}
	}

	if( via_needed )
	{
		// via needed, make sure it exists or create it
		if( v_draw->via_w == 0 || v_draw->via_hole_w == 0 )
		{
			// via doesn't already exist, set via width and hole width
			// using project or net defaults
			int w, via_w, via_hole_w;
			GetWidths( net, &w, &via_w, &via_hole_w );
			// set parameters for via
			v_draw->via_w = via_w;
			v_draw->via_hole_w = via_hole_w;
		}
	}
	else
	{
		// via not needed
		v_draw->via_w = 0;
		v_draw->via_hole_w = 0;
	}

	if( m_dlist && bDrawVertex && v->GetType() != cvertex::V_PIN )
	{
		v_draw->Draw();
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
			line.Format( "net: \"%s\" %d %d %d %d %d %d %d\n", 
							net->name, net->NumPins(), net->NumCons(), net->NumAreas(),
							net->def_w, net->def_via_w, net->def_via_hole_w,
							net->visible );
			file->WriteString( line );
			for( int ip=0; ip<net->NumPins(); ip++ )
			{
				line.Format( "  pin: %d %s.%s\n", ip+1, 
					net->pin[ip].ref_des, net->pin[ip].pin_name );
				file->WriteString( line );
			}
			for( int ic=0; ic<net->NumCons(); ic++ ) 
			{
				cconnect * c = net->connect[ic]; 
				line.Format( "  connect: %d %d %d %d %d\n", ic+1, 
					c->start_pin,
					c->end_pin, c->NumSegs(), c->locked );
				file->WriteString( line );
				int nsegs = c->NumSegs();
				for( int is=0; is<=nsegs; is++ )
				{
					v = &(c->vtx[is]);
					if( is<nsegs )
					{
						line.Format( "    vtx: %d %d %d %d %d %d %d %d\n", 
							is+1, v->x, v->y, v->pad_layer, v->force_via_flag, 
							v->via_w, v->via_hole_w, abs(v->tee_ID) );
						file->WriteString( line );
						s = &(c->seg[is]);
						line.Format( "    seg: %d %d %d 0 0\n", 
							is+1, s->m_layer, s->m_width );
						file->WriteString( line );
					}
					else
					{
						// last vertex
						line.Format( "    vtx: %d %d %d %d %d %d %d %d\n", 
							is+1, v->x, v->y, v->pad_layer, v->force_via_flag, 
							v->via_w, v->via_hole_w, abs(v->tee_ID) );
						file->WriteString( line );
					}
				}
			}
			for( int ia=0; ia<net->NumAreas(); ia++ )
			{
				line.Format( "  area: %d %d %d %d\n", ia+1, 
					net->area[ia].NumCorners(),
					net->area[ia].Layer(),
					net->area[ia].GetHatch()
					);
				file->WriteString( line );
				for( int icor=0; icor<net->area[ia].NumCorners(); icor++ )
				{
					line.Format( "    corner: %d %d %d %d %d\n", icor+1,
						net->area[ia].X( icor ),
						net->area[ia].Y( icor ),
						net->area[ia].SideStyle( icor ),
						net->area[ia].EndContour( icor )
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
			int def_width = my_atoi( &p[4] );
			int def_via_w = my_atoi( &p[5] );
			int def_via_hole_w = my_atoi( &p[6] );
			int visible = 1;
			if( np > 8 )
				visible = my_atoi( &p[7] );
			cnet * net = AddNet( net_name, npins, def_width, def_via_w, def_via_hole_w );
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
				net->AddPin( &ref_str, &pin_num_str );
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
				int start_pin = my_atoi( &p[1] );
				int end_pin = my_atoi( &p[2] );
				// check for fatal errors
				for( int ip=0; ip<2; ip++ )
				{
					// test start and end pins
					int ipin = start_pin;
					if( ip == 1 )
						ipin = end_pin;
					if( ipin != cconnect::NO_END )
					{
						CString test_ref_des = net->pin[ipin].ref_des;
						cpart * test_part = net->pin[ipin].part;
						if( !test_part )
						{
							CString * err_str = new CString( "fatal error in net \"" );
							*err_str += net_name + "\"";
							*err_str += "\r\n\rpart \"" + test_ref_des + "\" doesn't exist";
							throw err_str;
						}
						else if( !test_part->shape )
						{
							CString * err_str = new CString( "fatal error in net \"" );
							*err_str += net_name + "\"";
							*err_str += "\r\n\rpart \"" + test_ref_des + "\" doesn't haved a footprint";
							throw err_str;
						}
						else
						{
							CString test_pin_name = net->pin[ipin].pin_name;
							int pin_index = test_part->shape->GetPinIndexByName( test_pin_name );
							if( pin_index == -1 )
							{
								CString * err_str = new CString( "fatal error in net \"" );
								*err_str += net_name + "\"";
								*err_str += "\r\n\r\npin \"" + test_pin_name + "\"";
								*err_str += " doesn't exist in footprint \"" + test_part->shape->m_name + "\"";
								*err_str += " for part \"" + test_ref_des + "\"";
								throw err_str;
							}
						}
					}
				}

				int nsegs = my_atoi( &p[3] );
				int locked = my_atoi( &p[4] );
#if 0
				if( nc == -1 )
				{
					// invalid connection, remove it with this ugly code
					ic--;
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
#endif
				// read first vertex
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
				cvertex first_vtx;
				first_vtx.x = my_atoi( &p[1] ); 
				first_vtx.y = my_atoi( &p[2] ); 
				int file_layer = my_atoi( &p[3] ); 
				first_vtx.pad_layer = in_layer[file_layer];
				first_vtx.force_via_flag = my_atoi( &p[4] ); 
				first_vtx.via_w = my_atoi( &p[5] ); 
				first_vtx.via_hole_w = my_atoi( &p[6] );
				first_vtx.tee_ID = 0;
				if( np == 9 )
				{
					first_vtx.tee_ID = my_atoi( &p[7] );
					first_vtx.tee_ID = abs(first_vtx.tee_ID);
					if( first_vtx.tee_ID )
					{
						if( FindTeeID( first_vtx.tee_ID ) == -1 )
						{
							// not found, add it
							AddTeeID( first_vtx.tee_ID );
						}
						else
						{
							// already used, negate it
							first_vtx.tee_ID = -first_vtx.tee_ID;						}
					}
				}
				int nc;
				if( start_pin != cconnect::NO_END && end_pin != cconnect::NO_END )
				{
					net->AddConnectFromPinToPin( start_pin, end_pin, &nc );
				}
				else if( start_pin != cconnect::NO_END && end_pin == cconnect::NO_END )
				{
					net->AddConnectFromPin( start_pin, &nc );
				}
				else if( start_pin == cconnect::NO_END && end_pin != cconnect::NO_END )
				{
					// connect from vertex to pin
					cconnect * c = net->AddConnect( &nc);
					c->end_pin = end_pin;
					// add the first vertex
					cvertex * v = &c->InsertVertexByIndex( 0, first_vtx );
					// check for valid pin
					cpart * part = net->pin[end_pin].part;
					if( part == 0 )
						ASSERT(0);
					CShape * shape = part->shape;
					if( shape == 0 )
						ASSERT(0);
					int pin_index = shape->GetPinIndexByName( net->pin[end_pin].pin_name );
					if( pin_index == -1 )
						ASSERT(0);
					// get coords of end_pin
					CPoint pf;
					pf = m_plist->GetPinPoint( net->pin[end_pin].part, net->pin[end_pin].pin_name );

					// add first segment and second vertex
					cvertex new_vtx;
					new_vtx.x = pf.x;
					new_vtx.y = pf.y;
					new_vtx.pad_layer = m_plist->GetPinLayer( net->pin[end_pin].part, &net->pin[end_pin].pin_name );

					cseg new_seg;
					new_seg.m_layer = LAY_RAT_LINE;
					new_seg.m_width = 0;
					new_seg.m_selected = 0;

					c->AppendSegAndVertex( new_seg, new_vtx );
					c->end_pin = end_pin;
					cseg * s = &c->SegByIndex( 0 );
				}
				else if( start_pin == cconnect::NO_END && end_pin == cconnect::NO_END )
				{
					// connect from vertex to vertex
					cconnect * c = net->AddConnect( &nc);
					// add the first vertex
					cvertex * v = &c->InsertVertexByIndex( 0, first_vtx );
				}
				net->connect[ic]->locked = locked;

				// now add all segments
				int test_not_done = 1;
				int pre_via_w, pre_via_hole_w;
				for( int is=0; is<nsegs; is++ )
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
					int seg_width = my_atoi( &p[2] ); 
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
						int via_w = my_atoi( &p[5] ); 
						int via_hole_w = my_atoi( &p[6] );
						int tee_ID = 0;
						if( np == 9 )
						{
							tee_ID = abs( my_atoi( &p[7] ) );
							if( tee_ID )
							{
								if( FindTeeID( tee_ID ) == -1 )
								{
									// not found, add it
									AddTeeID( tee_ID );
								}
								else
								{
									// already used, negate it
									tee_ID = -tee_ID;						}
							}
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
							test_not_done = InsertSegment( net, ic, is, x, y, layer, seg_width, 0, 0, 0 );
						}
						else
						{
							AppendSegment( net, ic, x, y, layer, seg_width );
							// set widths of following vertex
							net->connect[ic]->vtx[is+1].via_w = via_w;
							net->connect[ic]->vtx[is+1].via_hole_w = via_hole_w;
						}
						//** this code is for bug in versions before 1.313
						if( force_via_flag )
						{
							if( end_pin == cconnect::NO_END && is == nsegs-1 )
								ForceVia( net, ic, is+1 );
							else if( read_version > 1.312001 )	// i.e. 1.313 or greater
								ForceVia( net, ic, is+1 );
						}
						net->connect[ic]->vtx[is+1].tee_ID = tee_ID;

						// if older version of fpc file, negate tee_ID of end_vertex
						// of stub trace to make compatible with version 1.360 and higher
						if( end_pin == cconnect::NO_END 
							&& is == nsegs-1 
							&& read_version < 1.360 
							&& tee_ID > 0 )
						{
							net->connect[ic]->vtx[is+1].tee_ID = -tee_ID;
						}
						//**

						if( is != 0 )
						{
							// set widths of preceding vertex
							net->connect[ic]->vtx[is].via_w = pre_via_w;
							net->connect[ic]->vtx[is].via_hole_w = pre_via_hole_w;
							if( m_dlist )
								DrawVertex( net, ic, is );
						}
						pre_via_w = via_w;
						pre_via_hole_w = via_hole_w;
					}
				}
#if 0
				}
#endif
				// connection created
				// if older version of fpc file, split at tees if needed
				if( read_version < 1.360 )
				{
					cconnect * c = net->ConByIndex( ic );
					// iterate through vertices in reverse
					for( int iv=c->NumVtxs()-2; iv>0; iv-- )
					{
						cvertex * v = &c->VtxByIndex( iv );
						if( v->tee_ID )
						{
							// split into 2 connections
							net->SplitConnectAtVtx( v->Id() );
							nconnects++;
							ic++;
						}
					}
				}
			} // end for(ic)

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
					int i_new_area;
					if( icor == 0 )
						i_new_area = AddArea( net, layer, x, y, hatch, FALSE );
					else
						AppendAreaCorner( net, ia, x, y, last_side_style );
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
			if( nareas )
			{
				for(  int ia=nareas-1; ia>=0; ia-- )
				{
					if( net->area[ia].NumCorners() < 3 )
					{
						RemoveArea( net, ia );
						nareas--;
					}
					
				}
				RenumberAreas( net );
			}
			CleanUpConnections( net );
			if( RemoveOrphanBranches( net, 0 ) )
			{
				//** we will hit this if FpcROUTE fails, so disabled				
//				ASSERT(0);
			}
		}
	}
}

// undraw via
//
void CNetList::UndrawVia( cnet * net, int ic, int iv )
{
	cconnect * c = net->connect[ic];
	cvertex * v = &c->vtx[iv];
	v->Undraw();
}

// draw vertex
//	i.e. draw selection box, draw via if needed
//
void CNetList::DrawVertex( cnet * net, int ic, int iv )
{
	cconnect * c = net->connect[ic];
	cvertex * v = &c->vtx[iv];
	v->Draw();
}

void CNetList::SetNetVisibility( cnet * net, BOOL visible )
{
	if( net->visible == visible )
		return;
	else if( visible )
	{
		// make segments visible and enable selection items
		for( int ic=0; ic<net->NumCons(); ic++ )
		{
			cconnect * c = net->connect[ic];
			for( int is=0; is<c->NumSegs(); is++ )
			{
				c->seg[is].dl_el->visible = TRUE;
				c->seg[is].dl_sel->visible = TRUE;
			}
		}
		// make thermals visible
		for( int ia=0; ia<net->NumAreas(); ia++ )
		{
			for( int ip=0; ip<net->area[ia].npins; ip++ )
			{
				net->area[ia].dl_thermal[ip]->visible = TRUE;
			}
		}
	}
	else
	{
		// make ratlines invisible and disable selection items
		for( int ic=0; ic<net->NumCons(); ic++ )
		{
			cconnect * c = net->connect[ic];
			for( int is=0; is<c->NumSegs(); is++ )
			{
				if( c->seg[is].m_layer == LAY_RAT_LINE )
				{
					c->seg[is].dl_el->visible = FALSE;
					c->seg[is].dl_sel->visible = FALSE;
				}
			}
		}
		// make thermals invisible
		for( int ia=0; ia<net->NumAreas(); ia++ )
		{
			for( int ip=0; ip<net->area[ia].npins; ip++ )
			{
				net->area[ia].dl_thermal[ip]->visible = FALSE;
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
		(*nl)[i].w = net->def_w;
		(*nl)[i].v_w = net->def_via_w;
		(*nl)[i].v_h_w = net->def_via_hole_w;
		(*nl)[i].apply_trace_width = FALSE;
		(*nl)[i].apply_via_width = FALSE;
		(*nl)[i].modified = FALSE;
		(*nl)[i].deleted = FALSE;
		(*nl)[i].ref_des.SetSize(0);
		(*nl)[i].pin_name.SetSize(0);
		// now make copy of pin arrays
		(*nl)[i].ref_des.SetSize( net->NumPins() );
		(*nl)[i].pin_name.SetSize( net->NumPins() );
		for( int ip=0; ip<net->NumPins(); ip++ )
		{
			(*nl)[i].ref_des[ip] = net->pin[ip].ref_des;
			(*nl)[i].pin_name[ip] = net->pin[ip].pin_name;
		}
		i++;
	}
}

// import netlist_info data back into netlist
//
void CNetList::ImportNetListInfo( netlist_info * nl, int flags, CDlgLog * log,
								 int def_w, int def_w_v, int def_w_v_h )
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
	CIterator_cnet iter_net(this);
	for( cnet * net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
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
			if( (*nl)[i].w == -1 )
				(*nl)[i].w = 0;
			if( (*nl)[i].v_w == -1 )
				(*nl)[i].v_w = 0;
			if( (*nl)[i].v_h_w == -1 )
				(*nl)[i].v_h_w = 0;
			net = AddNet( (*nl)[i].name, (*nl)[i].ref_des.GetSize(), 
				(*nl)[i].w, (*nl)[i].v_w, (*nl)[i].v_h_w );
			(*nl)[i].net = net;
		}
		else if( net == NULL && old_net != NULL )
		{
			// no net from netlist_info but existing net with same name
			// use existing net and modify it
			(*nl)[i].modified = TRUE;
			net = old_net;
			(*nl)[i].net = net;
			if( (*nl)[i].w != -1 )
				net->def_w = (*nl)[i].w;
			if( (*nl)[i].v_w != -1 )
				net->def_via_w = (*nl)[i].v_w;
			if( (*nl)[i].v_h_w != -1 )
				net->def_via_hole_w = (*nl)[i].v_h_w;
		}
		else
		{
			// net from netlist_info and existing net have the same name
			if( net != old_net )
				ASSERT(0);	// make sure that they are actually the same net
			// modify existing net parameters, unless undefined
			if( (*nl)[i].w != -1 )
				net->def_w = (*nl)[i].w;
			if( (*nl)[i].v_w != -1 )
				net->def_via_w = (*nl)[i].v_w;
			if( (*nl)[i].v_h_w != -1 )
				net->def_via_hole_w = (*nl)[i].v_h_w;
		}

		// now set pin lists
		net->name = (*nl)[i].name;
		// now loop through net pins, deleting any which were removed
		for( int ipn=0; ipn<net->NumPins(); )
		{
			CString ref_des = net->pin[ipn].ref_des;
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
						net->RemovePin( &ref_des, &pin_name );
					else if( !part->bPreserve )
						net->RemovePin( &ref_des, &pin_name );
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
					net->RemovePin( &ref_des, &pin_name );
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
				CIterator_cnet iter_net(this);
				for( cnet * test_net=iter_net.GetFirst(); test_net; test_net=iter_net.GetNext() )
				{
					if( test_net != net && test_net->NumPins() > 0 )
					{
						// test for duplicate pins
						for( int test_ip=test_net->NumPins()-1; test_ip>=0; test_ip-- )
						{
							if( test_net->pin[test_ip].ref_des == (*nl)[i].ref_des[ipl] 
							&& test_net->pin[test_ip].pin_name == (*nl)[i].pin_name[ipl] )
							{
								if( log )
								{
									mess.Format( "    Removing pin %s.%s from net \"%s\"\r\n", 
										test_net->pin[test_ip].ref_des,
										test_net->pin[test_ip].pin_name,
										test_net->name  );
									log->AddLine( mess );
								}
								test_net->RemovePin( test_ip );
							}
						}
					}
				}
				// now test for pin already present in net
				BOOL pin_present = FALSE;
				for( int ipp=0; ipp<net->NumPins(); ipp++ )
				{
					if( net->pin[ipp].ref_des == (*nl)[i].ref_des[ipl]
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
					net->AddPin( &(*nl)[i].ref_des[ipl], &(*nl)[i].pin_name[ipl] );
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
		cnet * net = (*nl)[i].net;
		if( net )
		{
			SetNetVisibility( net, (*nl)[i].visible ); 
			if( (*nl)[i].apply_trace_width )
			{
				int w = (*nl)[i].w;
				if( !w )
					w = def_w;
				SetNetWidth( net, w, 0, 0 ); 
			}
			if( (*nl)[i].apply_via_width )
			{
				int w_v = (*nl)[i].v_w;
				int w_v_h = (*nl)[i].v_h_w;
				if( !w_v )
					w_v = def_w_v;
				if( !w_v_h )
					w_v_h = def_w_v_h;
				SetNetWidth( net, 0, w_v, w_v_h ); 
			}
		}
	}
	CleanUpAllConnections();
}

// Copy all data from another netlist (except display elements)
//
void CNetList::Copy( CNetList * src_nl )
{
	RemoveAllNets();
	CIterator_cnet iter_net(src_nl);
	for( cnet * src_net=iter_net.GetFirst(); src_net; src_net=iter_net.GetNext() )
	{
		cnet * net = AddNet( src_net->name, src_net->NumPins(), 0, 0, 0 );
		net->pin.SetSize( src_net->NumPins() );
		for( int ip=0; ip<src_net->NumPins(); ip++ )
		{
			// add pin but don't modify part->pin->net
			cpin * src_pin = &src_net->pin[ip]; 
			cpin * pin = &net->pin[ip];
			*pin = *src_pin;
		}
		for( int ia=0; ia<src_net->NumAreas(); ia++ )
		{
			carea * src_a = &src_net->area[ia];
			AddArea( net, src_a->Layer(), src_a->X(0),
				src_a->Y(0), src_a->GetHatch() );
			carea * a = &net->area[ia];
			a->Copy( src_a );
			a->SetDisplayList( NULL );
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
		for( int ic=0; ic<src_net->NumCons(); ic++ )
		{
			cconnect * src_c = src_net->connect[ic];
			if( src_c->end_pin == cconnect::NO_END )
				net->AddConnectFromPin( src_c->start_pin );
			else
				net->AddConnectFromPinToPin( src_c->start_pin, src_c->end_pin );
			cconnect * c = net->connect[ic];
			c->seg.SetSize( src_c->NumSegs() );
			for( int is=0; is<src_c->NumSegs(); is++ )
			{
				c->seg[is] = src_c->seg[is];
				c->seg[is].dl_el = NULL;
				c->seg[is].dl_sel = NULL;
			}
			c->vtx.SetSize( c->NumSegs()+1 );
			for( int iv=0; iv<=c->NumSegs(); iv++ )
			{
				cvertex * v = &c->vtx[iv];
				cvertex * src_v = &src_c->vtx[iv];
				v->x = src_v->x;
				v->y = src_v->y;
				v->pad_layer = src_v->pad_layer;
				v->force_via_flag = src_v->force_via_flag;
				v->via_w = src_v->via_w;
				v->via_hole_w = src_v->via_hole_w;
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
	CIterator_cnet iter_net(this);
	cnet * net = iter_net.GetFirst();
	while( net )
	{
		for( int ic=0; ic<net->NumCons(); ic++ )
		{
			cconnect * c = net->connect[ic];
			for( int is=0; is<c->NumSegs(); is++ )
			{
				cseg * s = &c->seg[is];
				int old_layer = s->m_layer;
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
						s->m_layer = new_layer + LAY_TOP_COPPER;
				}
			}
			// check for first or last segments connected to SMT pins
			cvertex * v = &c->vtx[0];
			cseg * s = &c->seg[0];
			if( v->pad_layer && v->pad_layer != LAY_PAD_THRU && s->m_layer != v->pad_layer )
				UnrouteSegmentWithoutMerge( net, ic, 0 );
			if( c->end_pin != cconnect::NO_END )
			{
				v = &c->vtx[c->NumSegs()];
				s = &c->seg[c->NumSegs()-1];
				if( v->pad_layer && v->pad_layer != LAY_PAD_THRU && s->m_layer != v->pad_layer )
					UnrouteSegmentWithoutMerge( net, ic, c->NumSegs()-1 );
			}
			MergeUnroutedSegments( net, ic );
		}
		CleanUpConnections( net );
		for( int ia=net->NumAreas()-1; ia>=0; ia-- )
		{
			carea * a = &net->area[ia];
			int old_layer = a->Layer();
			int index = old_layer - LAY_TOP_COPPER;
			int new_layer = layer[index];
			if( new_layer == -1 )
			{
				// delete this area
				RemoveArea( net, ia );
			}
			else
			{
				a->SetLayer( new_layer + LAY_TOP_COPPER );
				a->Draw();
			}
		}
		CombineAllAreasInNet( net, TRUE, FALSE );
		net = iter_net.GetNext();
	}
	m_layers = n_new_layers;
}

// When net names change, try to restore traces and areas from previous netlist
//
void CNetList::RestoreConnectionsAndAreas( CNetList * old_nl, int flags, CDlgLog * log )
{
	// loop through old nets
	old_nl->MarkAllNets( 0 );
	CIterator_cnet iter_net(old_nl);
	cnet * old_net = iter_net.GetFirst();
	while( old_net )
	{
		if( flags & (KEEP_TRACES | KEEP_STUBS) )
		{
			BOOL bRestored = TRUE;	// flag to indicate at least one connection restored
			while( bRestored )
			{
				bRestored = FALSE;	
				// loop through old connections
				for( int old_ic=0; old_ic<old_net->NumCons(); old_ic++ )
				{
					cconnect * old_c = old_net->connect[old_ic];
					if( old_c->utility )
						continue;	// ignore if already flagged 
					if( old_c->NumSegs() == 1 && old_c->seg[0].m_layer == LAY_RAT_LINE )
					{
						old_c->utility = 1;
						continue;	// ignore pure ratline connections
					}
					// check net of starting pin
					cpin * old_start_pin = &old_net->pin[old_c->start_pin];
					cpart * new_start_part = m_plist->GetPart( old_start_pin->ref_des );
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
					int st_l = m_plist->GetPinLayer( new_start_part, &old_start_pin->pin_name );
					if( st_p.x != old_c->vtx[0].x || st_p.y != old_c->vtx[0].y )
					{
						old_c->utility = 1;
						continue;	// ignore if start pin position has changed
					}
					if( st_l != LAY_PAD_THRU && old_c->seg[0].m_layer != LAY_RAT_LINE && st_l != old_c->seg[0].m_layer )
					{
						old_c->utility = 1;
						continue;	// ignore if start pin layer doesn't match first segment
					}
					// see if we should move trace to new net
					cpin * old_end_pin = NULL;
					if( old_c->end_pin == cconnect::NO_END )
					{
						// stub trace
						int tee_ID = old_c->vtx[old_c->NumSegs()].tee_ID;
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
							cpart * new_end_part = m_plist->GetPart( old_end_pin->ref_des );
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
							int e_l = m_plist->GetPinLayer( new_end_part, &old_end_pin->pin_name );
							if( e_p.x != old_c->vtx[old_c->NumSegs()].x || e_p.y != old_c->vtx[old_c->NumSegs()].y )
							{
								old_c->utility = 1;
								continue;	// ignore if end pin position has changed
							}
							if( e_l != LAY_PAD_THRU && old_c->seg[old_c->NumSegs()-1].m_layer != LAY_RAT_LINE && e_l != old_c->seg[old_c->NumSegs()-1].m_layer )
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
							int tee_id = old_c->vtx[old_c->NumSegs()].tee_ID;
							if( !tee_id )
								line.Format( "  Moving stub trace from %s.%s to new net \"%s\"\r\n",
								old_start_pin->ref_des, old_start_pin->pin_name, new_start_net->name );
							else
								line.Format( "  Moving branch from %s.%s to new net \"%s\"\r\n",
								old_start_pin->ref_des, old_start_pin->pin_name, new_start_net->name );
						}
						else
						{
							// pin-pin trace
							line.Format( "  Moving trace from %s.%s to %s.%s to new net \"%s\"\r\n",
								old_start_pin->ref_des, old_start_pin->pin_name,
								old_end_pin->ref_des, old_end_pin->pin_name, 
								new_start_net->name );
						}
						log->AddLine( line );
					}
					cnet * net = new_start_net;
					int ic = -1;
					int new_start_pin_index = GetNetPinIndex( net, &old_start_pin->ref_des, &old_start_pin->pin_name );
					if( old_c->end_pin == cconnect::NO_END )
					{
						net->AddConnectFromPin( new_start_pin_index, &ic );
					}
					else
					{
						int new_end_pin_index = GetNetPinIndex( net, &old_end_pin->ref_des, &old_end_pin->pin_name );
						net->AddConnectFromPinToPin( new_start_pin_index, new_end_pin_index, &ic );
					}
					if( ic > -1 )
					{
						old_c->utility = 1;		// flag as already drawn
						bRestored = TRUE;		// and a trace was restored
						cconnect * c = net->connect[ic];
						c->Undraw();
						c->seg.SetSize( old_c->NumSegs() );
						for( int is=0; is<old_c->NumSegs(); is++ )
						{
							c->seg[is] = old_c->seg[is];
							c->seg[is].dl_el = NULL;
							c->seg[is].dl_sel = NULL;
						}
						c->vtx.SetSize( old_c->NumSegs()+1 );
						for( int iv=0; iv<=old_c->NumSegs(); iv++ )
						{
							cvertex * v = &c->vtx[iv];
							cvertex * src_v = &old_c->vtx[iv];
							v->x = src_v->x;
							v->y = src_v->y;
							v->pad_layer = src_v->pad_layer;
							v->force_via_flag = src_v->force_via_flag;
							v->via_w = src_v->via_w;
							v->via_hole_w = src_v->via_hole_w;
							v->tee_ID = src_v->tee_ID;
							if( v->tee_ID && iv < old_c->NumSegs() )
								AddTeeID( v->tee_ID );
							v->dl_el.RemoveAll();
							v->dl_sel = NULL;
							v->dl_hole = NULL;
						}
						c->Draw();
					}
				}
			}
		}

		if( flags & KEEP_AREAS )
		{
			// see if copper areas can be moved because all pins are on the same new net
			for( int ia=0; ia<old_net->NumAreas(); ia++ )
			{
				BOOL bMoveIt = TRUE;
				cnet * new_area_net = NULL;
				carea * old_a = &old_net->area[ia];
				for( int ip=0; ip<old_a->npins; ip++ )
				{
					int old_pin_index = old_a->pin[ip];
					cpin * old_pin = &old_net->pin[old_pin_index];
					cpart * new_pin_part = m_plist->GetPart( old_pin->ref_des );
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
					// now look at stubs that connect to the area
					for( int ic=0; ic<old_a->nvias; ic++ )
					{
						cconnect * old_con = old_net->connect[old_a->vcon[ic]];
						int old_pin_index = old_con->start_pin;
						cpin * old_pin = &old_net->pin[old_pin_index];
						cpart * new_pin_part = m_plist->GetPart( old_pin->ref_des );
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
					cnet * net = new_area_net;
					int ia = AddArea( net, old_a->Layer(), old_a->X(0),
						old_a->Y(0), old_a->GetHatch() );
					carea * a = &net->area[ia];
					a->Copy( old_a );
					id p_id( ID_NET, net->UID(), ID_AREA, a->UID(), ia );
					a->SetId( &p_id );
					a->SetPtr( net );
					a->Draw( m_dlist );
				}
			}
		}
		old_net = iter_net.GetNext();	
	}
}

undo_con * CNetList::CreateConnectUndoRecord( cnet * net, int icon, BOOL set_areas )
{
	// calculate size needed, get memory
	cconnect * c = net->connect[icon];
	int seg_offset = sizeof(undo_con);
	int vtx_offset = seg_offset + sizeof(undo_seg)*(c->NumSegs());
	int size = vtx_offset + sizeof(undo_vtx)*(c->NumSegs()+1);
	void * ptr = malloc( size );
	undo_con * con = (undo_con*)ptr;
	undo_seg * seg = (undo_seg*)(seg_offset+(UINT)ptr);
	undo_vtx * vtx = (undo_vtx*)(vtx_offset+(UINT)ptr);
	con->uid = c->m_uid;
	con->size = size;
	strcpy( con->net_name, net->name );
	con->start_pin = c->start_pin;
	con->end_pin = c->end_pin;
	con->nsegs = c->NumSegs();
	con->locked = c->locked;
	con->set_areas_flag = set_areas;
	con->seg_offset = seg_offset;
	con->vtx_offset = vtx_offset;
	for( int is=0; is<c->NumSegs(); is++ )
	{
		seg[is].uid = c->seg[is].m_uid;
		seg[is].layer = c->seg[is].m_layer;
		seg[is].width = c->seg[is].m_width;
	}
	for( int iv=0; iv<=con->nsegs; iv++ )
	{
		vtx[iv].uid = c->vtx[iv].m_uid;
		vtx[iv].x = c->vtx[iv].x;
		vtx[iv].y = c->vtx[iv].y;
		vtx[iv].pad_layer = c->vtx[iv].pad_layer;
		vtx[iv].force_via_flag = c->vtx[iv].force_via_flag;
		vtx[iv].tee_ID = c->vtx[iv].tee_ID;
		vtx[iv].via_w = c->vtx[iv].via_w;
		vtx[iv].via_hole_w = c->vtx[iv].via_hole_w;
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
			net->RecreateConnectFromUndo( con, seg, vtx );
		}
		else
		{
			ASSERT(0);	// illegal undo type
		}
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
	int size = sizeof(undo_net) + net->NumPins()*sizeof(undo_pin);
	undo_net * undo = (undo_net*)malloc( size );
	strcpy( undo->name, net->name );
	undo->npins = net->NumPins();
	undo_pin * un_pin = (undo_pin*)((UINT)undo + sizeof(undo_net));
	for( int ip=0; ip<net->NumPins(); ip++ )
	{
		strcpy( un_pin[ip].ref_des, net->pin[ip].ref_des );
		strcpy( un_pin[ip].pin_name, net->pin[ip].pin_name );
	}
	undo->nlist = this;
	undo->m_uid = net->m_uid;
	undo->m_id = net->m_id;
	undo->size = size;
	return undo;
}

// callback function for undoing modifications to a net or adding a net
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
			nl->OptimizeConnections( net, -1, FALSE, -1, FALSE );
		}
		else if( type == UNDO_NET_ADD )
		{
			// just delete the net that was added
			nl->RemoveNet( net );
		}
		else if( type == UNDO_NET_MODIFY )
		{
			// restore the net
			if( !net )
				ASSERT(0);
			CIterator_cconnect iter_con( net );
			for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
			{
				net->RemoveConnect( c );
			}
			// replace pin data
			net->pin.SetSize(0);
			for( int ip=0; ip<undo->npins; ip++ )
			{
				CString ref_str( un_pin[ip].ref_des );
				CString pin_name( un_pin[ip].pin_name );
				net->AddPin( &ref_str, &pin_name, FALSE );
			}
			nl->RehookPartsToNet( net );
		}
		else
			ASSERT(0);
		// adjust connections to areas
		if( net->NumAreas() )
			nl->SetAreaConnections( net );
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
		un_a->size = sizeof(undo_area);
		strcpy( un_a->net_name, net->name );
		un_a->nlist = this;
		un_a->iarea = iarea;
		return un_a;
	}

	carea * a = &net->area[iarea];
	int nc = a->NumCorners();
	un_a = (undo_area*)malloc(sizeof(undo_area)+sizeof(undo_poly)+nc*sizeof(undo_corner));
	un_a->size = sizeof(undo_area)+sizeof(undo_poly)+nc*sizeof(undo_corner);
	strcpy( un_a->net_name, net->name );
	un_a->nlist = this;
	un_a->iarea = iarea;
	un_a->m_id = a->Id();
	if( type == CNetList::UNDO_AREA_ADD || type == CNetList::UNDO_AREA_CLEAR_ALL )
	{
	}
	undo_poly * un_poly = (undo_poly*)((UINT)un_a + sizeof(undo_area));
	a->CreatePolyUndoRecord( un_poly );
	return un_a;
}

// callback function for undoing areas
// note that this is declared static, since it is a callback
//
void CNetList::AreaUndoCallback( int type, void * ptr, BOOL undo )
{
	if( undo )
	{
		undo_area * un_a = (undo_area*)ptr;
		CNetList * nl = un_a->nlist;
		CString temp = un_a->net_name;
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
			nl->RemoveArea( net, un_a->iarea );
		}
		else if( type == UNDO_AREA_MODIFY 
				|| type == UNDO_AREA_DELETE )
		{
			undo_poly * un_poly = (undo_poly*)((UINT)ptr+sizeof(undo_area));
			undo_corner * c = (undo_corner*)((UINT)ptr+sizeof(undo_area)+sizeof(undo_poly));
			if( type == UNDO_AREA_MODIFY )
			{
				// remove area
				nl->RemoveArea( net, un_a->iarea );
			}
			// now recreate area at its original iarea in net->area[iarea]
			nl->InsertArea( net, un_a->iarea, un_poly->layer, 
				c[0].x, c[0].y, un_poly->hatch, FALSE );
			carea * a = &net->area[un_a->iarea];
			// recreate CPolyLine
			a->SetFromUndo( un_poly );
			a->SetPtr( net );
			nl->RenumberAreas( net );
			a->Draw( nl->m_dlist );
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
			CString * ref_des = &net->pin[ip].ref_des;
			CString * pin_name = &net->pin[ip].pin_name;
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
						if( net->pin[iip].ref_des == *ref_des && net->pin[iip].pin_name == *pin_name )
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
						cconnect * c = net->connect[ic];
						if( c->start_pin == ip )
							c->start_pin = first_index;
						if( c->end_pin == ip )
							c->end_pin = first_index;
					}
					// remove pin
					net->RemovePin( ip );
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
			cconnect * c = net->connect[ic];
			CIterator_cseg iter_seg( c );
			for( cseg * s=iter_seg.GetFirst(); s; s=iter_seg.GetNext() )
			{
				if( s->m_con != c )
				{
					str.Format( "ERROR: Net \"%s\": connection %d segment %d with invalid ptr to connect\r\n",
						net->name, ic, iter_seg.GetIndex() );
					*logstr += str;
					nerrors++;
				}
			}
			CIterator_cvertex iter_vtx( c );
			for( cvertex * v=iter_vtx.GetFirst(); v; v=iter_vtx.GetNext() )
			{
				if( v->m_con != c )
				{
					str.Format( "ERROR: Net \"%s\": connection %d vertex %d with invalid ptr to connect\r\n",
						net->name, ic, iter_vtx.GetIndex() );
					*logstr += str;
					nerrors++;
				}
			}
			if( c->NumSegs() == 0 )
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
			cconnect * c = net->connect[ic];
			if( c->NumSegs() == 0 )
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
				for( int is=0; is<c->NumSegs(); is++ )
				{
					if( c->seg[is].m_layer == LAY_RAT_LINE )
					{
						bUnrouted = TRUE;
						break;
					}
				}
				if( bUnrouted )
				{
					CString start_pin, end_pin;
					int istart = c->start_pin;
					start_pin = net->pin[istart].ref_des + "." + net->pin[istart].pin_name;
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
						end_pin = net->pin[iend].ref_des + "." + net->pin[iend].pin_name;
						if( c->NumSegs() == 1 )
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
	carea * a = &net->area[iarea];
	// first, check for sides intersecting other sides, especially arcs 
	BOOL bInt = FALSE;
	BOOL bArcInt = FALSE;
	int n_cont = a->NumContours();
	// make bounding rect for each contour
	CArray<CRect> cr;
	cr.SetSize( n_cont );
	for( int icont=0; icont<n_cont; icont++ )
		cr[icont] = a->GetCornerBounds( icont );
	for( int icont=0; icont<n_cont; icont++ )
	{
		int is_start = a->ContourStart(icont);
		int is_end = a->ContourEnd(icont);
		for( int is=is_start; is<=is_end; is++ )
		{
			int is_prev = is - 1;
			if( is_prev < is_start )
				is_prev = is_end;
			int is_next = is + 1;
			if( is_next > is_end )
				is_next = is_start;
			int style = a->SideStyle( is );
			int x1i = a->X( is );
			int y1i = a->Y( is );
			int x1f = a->X( is_next );
			int y1f = a->Y( is_next );
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
					int is2_start = a->ContourStart(icont2);
					int is2_end = a->ContourEnd(icont2);
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
							int style2 = a->SideStyle( is2 );
							int x2i = a->X( is2 );
							int y2i = a->Y( is2 );
							int x2f = a->X( is2_next );
							int y2f = a->Y( is2_next );
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
		a->utility2 = -1;
	else if( bInt )
		a->utility2 = 1;
	else 
		a->utility2 = 0;
	return a->utility2;
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
	carea * a = &net->area[iarea];
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
	for( int ia=0; ia<net->NumAreas(); ia++ )
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
		a->Undraw();
		int n_poly = a->NormalizeWithGpc( pa, bRetainArcs );
		if( n_poly > 1 )
		{
			for( int ip=1; ip<n_poly; ip++ )
			{
				// create new copper area and copy poly into it
				CPolyLine * new_p = (*pa)[ip-1];
				int ia = AddArea( net, 0, 0, 0, 0 );
				carea * a = &net->area[ia];
				// remove the poly that was automatically created for the new area
				// and replace it with a poly from NormalizeWithGpc
#if 0	// this will no longer work
				delete net->area[ia].poly;
				a->poly = new_p;
				a->SetDisplayList( net->m_dlist );
				a->SetHatch( p->GetHatch() );
				a->SetLayer( p->Layer() );
				id p_id( ID_NET, net->UID(), ID_AREA, a->UID(), ia );
				a->SetId( &p_id );
				a->Draw();
				a->utility = 1;
#endif
			}
		}
		a->Draw();
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
	if( net->NumAreas() > 1 )
	{
		// start by testing all area polygons to set utility2 flags
		for( int ia=0; ia<net->NumAreas(); ia++ )
			TestAreaPolygon( net, ia );
		// now loop through all combinations
		BOOL message_shown = FALSE;
		for( int ia1=0; ia1<net->NumAreas()-1; ia1++ ) 
		{
			// legal polygon
			CRect b1 = net->area[ia1].GetCornerBounds();
			BOOL mod_ia1 = FALSE;
			for( int ia2=net->NumAreas()-1; ia2 > ia1; ia2-- )
			{
				if( net->area[ia1].Layer() == net->area[ia2].Layer() 
					&& net->area[ia1].utility2 != -1 && net->area[ia2].utility2 != -1 )
				{
					CRect b2 = net->area[ia2].GetCornerBounds();
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
	carea * a1 = &net->area[ia];
	CPolyLine * poly1 = a1;
	for( int ia2=0; ia2<net->NumAreas(); ia2++ )
	{
		if( ia != ia2 )
		{
			carea * a2 = &net->area[ia2];
			// see if polygons are on same layer
			CPolyLine * poly2 = a2;
			if( poly1->Layer() != poly2->Layer() )
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
			for( int icont1=0; icont1<poly1->NumContours(); icont1++ )
			{
				int is1 = poly1->ContourStart( icont1 );
				int ie1 = poly1->ContourEnd( icont1 );
				for( int ic1=is1; ic1<=ie1; ic1++ )
				{
					int xi1 = poly1->X(ic1);
					int yi1 = poly1->Y(ic1);
					int xf1, yf1, style1;
					if( ic1 < ie1 )
					{
						xf1 = poly1->X(ic1+1);
						yf1 = poly1->Y(ic1+1);
					}
					else
					{
						xf1 = poly1->X(is1);
						yf1 = poly1->Y(is1);
					}
					style1 = poly1->SideStyle( ic1 );
					for( int icont2=0; icont2<poly2->NumContours(); icont2++ )
					{
						int is2 = poly2->ContourStart( icont2 );
						int ie2 = poly2->ContourEnd( icont2 );
						for( int ic2=is2; ic2<=ie2; ic2++ )
						{
							int xi2 = poly2->X(ic2);
							int yi2 = poly2->Y(ic2);
							int xf2, yf2, style2;
							if( ic2 < ie2 )
							{
								xf2 = poly2->X(ic2+1);
								yf2 = poly2->Y(ic2+1);
							}
							else
							{
								xf2 = poly2->X(is2);
								yf2 = poly2->Y(is2);
							}
							style2 = poly2->SideStyle( ic2 );
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
	CPolyLine * poly1 = &net->area[ia1];
	CPolyLine * poly2 = &net->area[ia2];
	if( poly1->Layer() != poly2->Layer() )
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
	for( int icont1=0; icont1<poly1->NumContours(); icont1++ )
	{
		int is1 = poly1->ContourStart( icont1 );
		int ie1 = poly1->ContourEnd( icont1 );
		for( int ic1=is1; ic1<=ie1; ic1++ )
		{
			int xi1 = poly1->X(ic1);
			int yi1 = poly1->Y(ic1);
			int xf1, yf1, style1;
			if( ic1 < ie1 )
			{
				xf1 = poly1->X(ic1+1);
				yf1 = poly1->Y(ic1+1);
			}
			else
			{
				xf1 = poly1->X(is1);
				yf1 = poly1->Y(is1);
			}
			style1 = poly1->SideStyle( ic1 );
			for( int icont2=0; icont2<poly2->NumContours(); icont2++ )
			{
				int is2 = poly2->ContourStart( icont2 );
				int ie2 = poly2->ContourEnd( icont2 );
				for( int ic2=is2; ic2<=ie2; ic2++ )
				{
					int xi2 = poly2->X(ic2);
					int yi2 = poly2->Y(ic2);
					int xf2, yf2, style2;
					if( ic2 < ie2 )
					{
						xf2 = poly2->X(ic2+1);
						yf2 = poly2->Y(ic2+1);
					}
					else
					{
						xf2 = poly2->X(is2);
						yf2 = poly2->Y(is2);
					}
					style2 = poly2->SideStyle( ic2 );
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
	CPolyLine * poly1 = &net->area[ia1];
	CPolyLine * poly2 = &net->area[ia2];
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
	int hatch = net->area[ia1].GetHatch();
	id a_id = net->area[ia1].Id();
	int layer = net->area[ia1].Layer();
	int w = net->area[ia1].W();
	int sel_box = net->area[ia1].SelBoxSize();
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
					AppendAreaCorner( net, ia1, x, y, CPolyLine::STRAIGHT );
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
				AppendAreaCorner( net, ia1, x, y, CPolyLine::STRAIGHT );
			}
			CompleteArea( net, ia1, CPolyLine::STRAIGHT );
		}
	}
	net->area[ia1].utility = 1;
	net->area[ia1].RestoreArcs( &arc_array1 ); 
	net->area[ia1].RestoreArcs( &arc_array2 );
	net->area[ia1].Draw();
	gpc_free_polygon( union_gpc );
	delete union_gpc;
	return 1;
}

void CNetList::SetWidths( int w, int via_w, int via_hole_w )
{
	m_def_w = w; 
	m_def_via_w = via_w; 
	m_def_via_hole_w = via_hole_w;
}

void CNetList::GetWidths( cnet * net, int * w, int * via_w, int * via_hole_w )
{
	if( net->def_w == 0 )
		*w = m_def_w;
	else
		*w = net->def_w;

	if( net->def_via_w == 0 )
		*via_w = m_def_via_w;
	else
		*via_w = net->def_via_w;

	if( net->def_via_hole_w == 0 )
		*via_hole_w = m_def_via_hole_w;
	else
		*via_hole_w = net->def_via_hole_w;

}

// get bounding rectangle for all net elements
//
BOOL CNetList::GetNetBoundaries( CRect * r )
{
	BOOL bValid = FALSE;
	CIterator_cnet iter_net(this);
	cnet * net = iter_net.GetFirst();
	CRect br;
	br.bottom = INT_MAX;
	br.left = INT_MAX;
	br.top = INT_MIN;
	br.right = INT_MIN;
	while( net )
	{
		for( int ic=0; ic<net->NumCons(); ic++ )
		{
			for( int iv=0; iv<net->connect[ic]->vtx.GetSize(); iv++ )
			{
				cvertex * v = &net->connect[ic]->vtx[iv];
				br.bottom = min( br.bottom, v->y - v->via_w );
				br.top = max( br.top, v->y + v->via_w );
				br.left = min( br.left, v->x - v->via_w );
				br.right = max( br.right, v->x + v->via_w );
				bValid = TRUE;
			}
		}
		for( int ia=0; ia<net->NumAreas(); ia++ )
		{
			CRect r = net->area[ia].GetBounds();
			br.bottom = min( br.bottom, r.bottom );
			br.top = max( br.top, r.top );
			br.left = min( br.left, r.left );
			br.right = max( br.right, r.right );
			bValid = TRUE;
		}
		net = iter_net.GetNext();
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
	for( int icc=0; icc<net->NumCons(); icc++ )
	{
		cconnect * c = net->connect[icc];
		for( int ivv=0; ivv<=c->NumSegs(); ivv++ )
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
	CIterator_cnet iter_net(this);
	for( cnet * tnet=iter_net.GetFirst(); tnet; tnet=iter_net.GetNext() )
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
	for( int ic=0; ic<net->NumCons(); ic++ )
	{
		cconnect * c = net->connect[ic];
		if( c->vtx[c->NumSegs()].tee_ID == id )
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


// Remove tee-vertex from net
// Don't change stubs connected to it
// return connection number of tee vertex or -1 if not found
//
int CNetList::RemoveTee( cnet * net, int id )
{
	int tee_ic = -1;
	for( int ic=net->NumCons()-1; ic>=0; ic-- )
	{
		cconnect * c = net->connect[ic];
		for( int iv=0; iv<=c->NumSegs(); iv++ )
		{
			cvertex * v = &c->vtx[iv];
			if( v->tee_ID == id )
			{
				if( iv < c->NumSegs() )
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
// also returns pointer to master vertex for tee
//
BOOL CNetList::TeeViaNeeded( cnet * net, int id, cvertex ** v_master )
{
	int layer = 0;
	for( int ic=0; ic<net->NumCons(); ic++ )
	{
		cconnect * c = net->connect[ic];
		int num_segs = c->NumSegs();
		if( num_segs > 0 )
		{
			for( int i=0; i<2; i++ )
			{
				int iv = 0;
				if( i == 1 )
					iv = c->NumSegs();
				cvertex * v = &c->vtx[iv];
				if( abs(v->tee_ID) == abs(id) )
				{
					if( v->tee_ID > 0 && v_master != NULL )
						*v_master = v;
					if( iv > 0 )
					{
						// test layer of segment pre-via
						int seg_layer = c->seg[iv-1].m_layer;	
						if( seg_layer >= LAY_TOP_COPPER )
						{
							if( layer == 0 )
								layer = seg_layer;
							else if( layer != seg_layer )
								return TRUE;
						}
					}
					if( iv == 0 )
					{
						// test layer of segment post-via
						int seg_layer = c->seg[iv].m_layer;	
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
		for( int ic=net->NumCons()-1; ic>=0; ic-- )
		{
			cconnect * c = net->connect[ic];
			c->utility = 0;		
			BOOL bFixed = FALSE;
			int id = c->vtx[c->NumSegs()].tee_ID;
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
						for( int tic=ic+1; tic<net->NumCons(); tic++ )
						{
							cconnect *tc = net->connect[tic];
							if( tc->end_pin == cconnect::NO_END && tc->vtx[tc->NumSegs()].tee_ID == id )
							{
								// matching branch found, merge them
								// add ratline to start pin of branch
								AppendSegment( net, ic, tc->vtx[0].x, tc->vtx[0].y, LAY_RAT_LINE, 0 );
								c->end_pin = tc->start_pin;
								c->vtx[c->NumSegs()].pad_layer = tc->vtx[0].pad_layer;
								for( int tis=tc->NumSegs()-1; tis>=0; tis-- )
								{
									if( tis > 0 )
									{
										int test = InsertSegment( net, ic, c->NumSegs()-1, 
											tc->vtx[tis].x, tc->vtx[tis].y,
											tc->seg[tis].m_layer, tc->seg[tis].m_width, 
											0, 0, 0 );
										if( !test )
											ASSERT(0);
										c->vtx[c->NumSegs()-1] = tc->vtx[tis];
										tc->vtx[tis].tee_ID = 0;
									}
									else
									{
										RouteSegment( net, ic, c->NumSegs()-1, 
											tc->seg[0].m_layer, tc->seg[tis].m_width );
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
		for( int ic=net->NumCons()-1; ic>=0; ic-- )
		{
			cconnect * c = net->connect[ic];
			if( c->utility )
			{
				c->vtx[c->NumSegs()].tee_ID = 0;
				if( bRemoveSegs )
				{
					RemoveNetConnect( net, ic, FALSE );
					bRemoved = TRUE;
					test_id = 0;
				}
				else
					ReconcileVia( net, ic, c->NumSegs() );
			}
		}
	}
	// SetAreaConnections( net );
	return bFound;
}

#if 0
//
void CNetList::ApplyClearancesToArea( cnet *net, int ia, int flags, 
					int fill_clearance, int min_silkscreen_stroke_wid, 
					int thermal_wid, int hole_clearance )
{
	// get area layer
	int layer = net->area[ia].Layer();
	net->area[ia].Undraw();

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
						net->area[ia].AddContourForPadClearance( PAD_ROUND, pad_x, pad_y, 
							pad_hole, pad_hole, pad_r, pad_angle, fill_clearance, pad_hole, hole_clearance );
					}
					else if( pad_type != PAD_NONE )
					{
						if( pad_connect & CPartList::AREA_CONNECT ) 
						{
							if( !(flags & GERBER_NO_PIN_THERMALS) )
							{
								// make thermal for pad
								net->area[ia].AddContourForPadClearance( pad_type, pad_x, pad_y, 
									pad_w, pad_l, pad_r, pad_angle, fill_clearance, pad_hole, hole_clearance, 
									TRUE, thermal_wid );
							}
						}
						else
						{
							// make clearance for pad
							net->area[ia].AddContourForPadClearance( pad_type, pad_x, pad_y, 
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
	CIterator_cnet iter_net(this);
	cnet * t_net = iter_net.GetFirst();
	while( t_net )
	{
		for( int ic=0; ic<t_net->NumCons(); ic++ )
		{
			int nsegs = t_net->connect[ic]->NumSegs();
			cconnect * c = t_net->connect[ic]; 
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
				double w = (double)fill_clearance + (double)(s->width)/2.0;
				int test = GetViaConnectionStatus( t_net, ic, is+1, layer );
				// flash the via clearance if necessary
				if( post_vtx->via_w && layer >= LAY_TOP_COPPER )
				{
					// via exists and this is a copper layer 
					if( layer > LAY_BOTTOM_COPPER && test == CNetList::VIA_NO_CONNECT )
					{
						// inner layer and no trace or thermal, just make hole clearance
						net->area[ia].AddContourForPadClearance( PAD_ROUND, xf, yf, 
							0, 0, 0, 0, 0, post_vtx->via_hole_w, hole_clearance );
					}
					else if( !(test & VIA_AREA) )
					{
						// outer layer and no thermal, make pad clearance
						net->area[ia].AddContourForPadClearance( PAD_ROUND, xf, yf, 
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
							net->area[ia].AddContourForPadClearance( PAD_ROUND, post_vtx->x, post_vtx->y, 
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
							net->area[ia].AddContourForPadClearance( PAD_ROUND, post_vtx->x, post_vtx->y, 
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
						net->area[ia].AppendCorner( x, y, CPolyLine::STRAIGHT );
						angle += angle_step;
					}
					// create points around end of segment
					angle = seg_angle - PI/2.0;
					for( int i=npoints/2; i<npoints; i++ )
					{
						x = xf + w*cos(angle);
						y = yf + w*sin(angle);
						net->area[ia].AppendCorner( x, y, CPolyLine::STRAIGHT );
						angle += angle_step;
					}
					net->area[ia].Close( CPolyLine::STRAIGHT );
				}
			}
		}
		t_net = iter_net.GetNext();
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
#endif

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
	for( int ic=net->NumCons()-1; ic>=0; ic-- )
		RemoveNetConnect( net, ic, FALSE );
	SetAreaConnections( net );

	// add all pins in net to list of nodes
	for( int ip=0; ip<net->NumPins(); ip++ ) 
	{
		cpin * net_pin = &net->pin[ip];
		int layer = m_plist->GetPinLayer( net_pin->part, &net_pin->pin_name );
		CPoint p = m_plist->GetPinPoint( net_pin->part, net_pin->pin_name );
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
								{
									net->AddConnectFromPin( prev_node->pin_index, &ic );
								}
								else if( ipass == 1 )
								{
									int last_path_index = g_best_path_index[0];
									int last_path_end = g_best_path_end[0];
									cpath * last_path = &(*paths)[last_path_index];
									int last_inode = last_path->GetInode( last_path_end );
									cnode * last_node = &(*nodes)[last_inode];
									net->AddConnectFromPinToPin( prev_node->pin_index, last_node->pin_index, &ic );
								}
								str.Format( "n%d", prev_inode );
								mess += str;
							}
							if( ic > -1 )
								c = net->connect[ic];
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
									AppendSegment( net, ic, x, y, layer, width );
								else if( ipass == 1 )
									InsertSegment( net, ic, is, x, y, layer, width, 0, 0, 0 );
								is++;
							}
							// force all vias
							if( next_node->type == NVIA )
							{
								// use via width from node and default hole width
								int w, via_w, via_hole_w;
								GetWidths( net, &w, &via_w, &via_hole_w );
								c->vtx[is].via_w = next_node->via_w;
								c->vtx[is].via_hole_w = via_hole_w;
								ForceVia( net, ic, is, FALSE );
							}
							str.Format( "-n%d", next_inode );
							mess += str;
						}
						if( ipass == 2 )
						{
							int tee_id = GetNewTeeID();
							cconnect * c = net->connect[ic];
							int end_v = c->NumSegs();
							cvertex * v = &c->vtx[end_v];
							v->tee_ID = tee_id;
						}
						mess += "\r\n";
						if( log && bVerbose )
							log->AddLine( mess );
						// at this point, all imported vias are forced
						// unforce the ones between segments on different layers that don't 
						// connect to copper areas
						for( int iv=1; iv<c->NumSegs(); iv++ )
						{
							cvertex * v = &c->vtx[iv];
							cseg * pre_seg = &c->seg[iv-1];
							cseg * post_seg = &c->seg[iv];
							if( pre_seg->m_layer != post_seg->m_layer && !TestPointInArea( net, v->x, v->y, LAY_PAD_THRU, NULL ) )
								UnforceVia( net, ic, iv, FALSE );
						}

					}
				}
			}
		}
	}
	// now hook up branches to traces
	for( int ic=0; ic<net->NumCons(); ic++ )
	{
		cconnect * c = net->connect[ic];
		if( c->end_pin == cconnect::NO_END )
		{
			cvertex * end_v = &c->vtx[c->NumSegs()];
			cseg * end_seg = &c->seg[c->NumSegs()-1];
			int tee_id = end_v->tee_ID;
			if( tee_id )
			{
				//  find trace with corresponding vertex
				for( int icc=0; icc<net->NumCons(); icc++ )
				{
					cconnect * trace_c = net->connect[icc];
					for( int iv=1; iv<trace_c->NumSegs(); iv++ )
					{
						cvertex * trace_v = &trace_c->vtx[iv];
						cseg * trace_seg = &trace_c->seg[iv-1];
						if( trace_v->x == end_v->x && trace_v->y == end_v->y
							&& ( trace_v->via_w || end_v->via_w 
							|| end_seg->m_layer == trace_seg->m_layer ) ) 
						{
							// make a tee-vertex and connect branch
							if( trace_v->tee_ID )
								tee_id = trace_v->tee_ID;
							trace_v->tee_ID = tee_id;
							end_v->tee_ID = tee_id;
							end_v->force_via_flag = FALSE;
							end_v->via_w = 0;
							end_v->via_hole_w = 0;
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

