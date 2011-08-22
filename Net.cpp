// Net.cpp: implementation of the CNetList class.
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

//************************* CNet implementation ***************************

cnet::cnet( CDisplayList * dlist, CNetList * nlist )
{ 
	m_dlist = dlist;
	m_nlist = nlist;
	id.Clear();
	id.type = ID_NET;
	id.st = ID_NONE;
	id.uid = pcb_cuid.GetNewUID();
}
cnet::~cnet()
{
	pcb_cuid.ReleaseUID( id.uid );
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

// add new connection
//
cconnect * cnet::AddNewConnect()
{
	cconnect  * c = new cconnect( this );
	connect.Add( c );	
	return c;
}

// remove connection
// if called from an iterated loop, calling function should
// also call CIterator_cconnect::Remove() to adjust iterator(s)
//
void cnet::RemoveConnect( cconnect * c )
{
	int ic = GetConnectIndexByPtr( c );
	connect.RemoveAt( ic );
	delete c;
}

// return cconnect with given index
cconnect * cnet::GetConnectByIndex( int ic )
{
	if( ic >= 0 && ic < connect.GetSize() )
		return connect[ic];
	else
		return NULL;
};

// return index of cconnect with given UID
int cnet::GetConnectIndexByUID( int uid )
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

// return index of cconnect with given UID 
cconnect * cnet::GetConnectByUID( int uid )
{
	int ic = GetConnectIndexByUID( uid );
	if( ic == -1 )
		return NULL;
	else 
		return connect[ic];
}

// return index of cconnect with given pointer 
int cnet::GetConnectIndexByPtr( cconnect * con )
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
void cnet::RecreateConnectFromUndo( undo_con * con, undo_seg * seg, undo_vtx * vtx )
{
	int nc;
	if( con->nsegs )
	{
		// add new connect
		if( con->end_pin != cconnect::NO_END )
			nc = m_nlist->AddNetConnect( this, con->start_pin, con->end_pin );
		else
			nc = m_nlist->AddNetStub( this, con->start_pin );
		cconnect * c = connect[nc];
		// now replace all connect parameters from undo record
		c->ReplaceUID( con->uid );
		// insert segments and vertices into connection
		for( int is=0; is<con->nsegs; is++ )
		{
			if( con->end_pin != cconnect::NO_END )
			{
				// pin-pin trace
				m_nlist->InsertSegment( this, nc, is, vtx[is+1].x, vtx[is+1].y,
					seg[is].layer, seg[is].width, seg[is].via_w, seg[is].via_hole_w, 0 );
			}
			else
			{
				// stub trace
				m_nlist->AppendSegment( this, nc, vtx[is+1].x, vtx[is+1].y,
					seg[is].layer, seg[is].width );
			}
		}
		// now finish importing undo data into segments and vertices
		for( int is=0; is<con->nsegs; is++ )
		{
			c->seg[is].ReplaceUID( seg[is].uid );
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
		connect[nc]->locked = con->locked; 
		c->Draw();
	}
}


