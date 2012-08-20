// Implementation of class DRErrorList
//
#include "stdafx.h"

//******************************************************
//
// class DRError
//
// constructor
//
/*
DRError::DRError()
{
}

DRError::~DRError()
{
}



//******************************************************
//
// class DRErrorList
//
// constructor
//
/*
DRErrorList::DRErrorList()
{
	m_dlist = NULL;
}

DRErrorList::~DRErrorList()
{
	Clear();
}

// Set the DisplayList for error symbols
//
void DRErrorList::SetLists( CPartList * pl, CNetList * nl, CDisplayList * dl )
{
	m_plist = pl;
	m_nlist = nl;
	m_dlist = dl;
}

// clear all entries from DRE list
//
void DRErrorList::Clear()
{
	while( !list.IsEmpty() )
	{
		void * ptr = list.GetHead();
		DRError * dre = (DRError*)ptr;
		delete dre;
		list.RemoveHead();
	}
}

// Remove entry from list
//
void DRErrorList::Remove( DRError * dre )
{
	POSITION pos = list.Find( dre );
	if( pos )
	{
		list.RemoveAt( pos );
		delete dre;
	}
}

// Add error to list
//
DRError * DRErrorList::Add( long index, int type, CString * str, 
						   CString * name1, CString * name2, id id1, id id2,
						   int x1, int y1, int x2, int y2, int w, int layer )
{
#define DRE_MIN_SIZE 50*NM_PER_MIL  

	POSITION pos;
	void * ptr;

	// find center of coords
	int x = x1;
	int y = y1;
	if( name2 )
	{
		x = (x1 + x2)/2;
		y = (y1 + y2)/2;
	}
	// first check for redundant error
	// CPT: allow repeats of COPPERAREA_BROKEN errors
	if( !list.IsEmpty()  && type != DRError::COPPERAREA_BROKEN )	
	{
		for( pos = list.GetHeadPosition(); pos != NULL; )
		{
			ptr = list.GetNext( pos );
			DRError * dre = (DRError*)ptr;
			// compare current error with error from list
			// if identical id's, don't add it
			if( dre->id1 == id1 && dre->id2 == id2 )
				return NULL;
			if( dre->id1 == id2 && dre->id2 == id1 )
				return NULL;
			// if same traces at same point, don't add it
			if( name2 )
			{
				if( id1.T1() == ID_NET && id1.T2() == ID_CONNECT
					&& dre->id1.T1() == ID_NET && dre->id1.T2() == ID_CONNECT
					&& id1.I2() == dre->id1.I2() 
					&& id2.T1() == ID_NET && id2.T2() == ID_CONNECT
					&& dre->id2.T1() == ID_NET && dre->id2.T2() == ID_CONNECT
					&& id2.I2() == dre->id2.I2() 
					&& x == dre->x && y == dre->y )
					return NULL;
				if( id2.T1() == ID_NET && id2.T2() == ID_CONNECT
					&& dre->id1.T1() == ID_NET && dre->id1.T2() == ID_CONNECT
					&& id2.I2() == dre->id1.I2() 
					&& id1.T1() == ID_NET && id1.T2() == ID_CONNECT
					&& dre->id2.T1() == ID_NET && dre->id2.T2() == ID_CONNECT
					&& id1.I2() == dre->id2.I2() 
					&& x == dre->x && y == dre->y )
					return NULL;
			}
			// if RING_PAD error on same pad, don't add it
			if( type == DRError::RING_PAD && dre->m_id.T3() == DRError::RING_PAD )
			{
				if( *name1 == dre->name1 
					&& id1.I2() == dre->id1.I2() )
				{
					// same pad, ignore it
					return NULL;
				}
			}
			// if BOARDEDGE_PAD or BOARDEDGE_PADHOLE error on same pad, don't add it
			if( (type == DRError::BOARDEDGE_PAD || type == DRError::BOARDEDGE_PADHOLE)
				&& (dre->m_id.T3() == DRError::BOARDEDGE_PAD || dre->m_id.T3() == DRError::BOARDEDGE_PADHOLE) )
			{
				if( *name1 == dre->name1 
					&& id1.I2() == dre->id1.I2() )
				{
					// same pad, ignore it
					return NULL;
				}
			}
			// if RING_VIA error on same via, don't add it
			if( type == DRError::RING_VIA && dre->m_id.T3() == DRError::RING_VIA )
			{
				if( *name1 == dre->name1 
					&& id1.I2() == dre->id1.I2() 
					&& id1.I3() == dre->id1.I3() )
				{
					// same via, ignore it
					return NULL;
				}
			}
			// if BOARDEDGE_VIA or BOARDEDGE_VIAHOLE error on same via, don't add it
			if( (type == DRError::BOARDEDGE_VIA || type == DRError::BOARDEDGE_VIAHOLE)
				&& (dre->m_id.T3() == DRError::BOARDEDGE_VIA || dre->m_id.T3() == DRError::BOARDEDGE_VIAHOLE) )
			{
				if( *name1 == dre->name1 
					&& id1.I2() == dre->id1.I2() 
					&& id1.I3() == dre->id1.I3() )
				{
					// same via, ignore it
					return NULL;
				}
			}
			// if BOARDEDGE_SEG on same trace at same point, don't add it
			if( type == DRError::BOARDEDGE_SEG && dre->m_id.T3() == DRError::BOARDEDGE_SEG )
			{
				if( *name1 == dre->name1 
					&& x == dre->x 
					&& y == dre->y ) 
				{
					return NULL;
				}
			}
			// if BOARDEDGE_COPPERAREA on same area at same point, don't add it
			if( type == DRError::BOARDEDGE_COPPERAREA 
				&& dre->m_id.T3() == DRError::BOARDEDGE_COPPERAREA )
			{
				if( *name1 == dre->name1 
					&& id1.I2() == dre->id1.I2() 
					&& x == dre->x  
					&& y == dre->y ) 
				{
					return NULL;
				}
			}
			if( type == DRError::COPPERAREA_COPPERAREA 
				&& dre->m_id.T3() == DRError::COPPERAREA_COPPERAREA )		
			{
				if( *name1 == dre->name1 
					&& *name2 == dre->name2
					&& id1.I2() == dre->id1.I2() 
					&& id2.I2() == dre->id2.I2() 
					&& x == dre->x 
					&& y == dre->y )
					return NULL;
			}
		}
	}

	DRError * dre = new DRError;
	// make id
	dre->m_id.SetT1( ID_DRC );
	dre->m_id.SetT2( ID_DRE );
	dre->m_id.SetI2( (unsigned int)dre );
	dre->m_id.SetT3( type );
	dre->m_id.SetI3( index );
	// set rest of params
	dre->name1 = *name1;
	if( name2 )
		dre->name2 = *name2;
	dre->str = *str;
	dre->id1 = id1;
	if( name2 )
		dre->id2 = id2;
	dre->x = x;
	dre->y = y;
	dre->layer = layer;
	if( m_dlist )
	{
		int d = DRE_MIN_SIZE/2;
		if( name2 )
			d = max( DRE_MIN_SIZE, Distance( x1, y1, x2, y2 ) )/2;
		if( w )
			d = max( d, w/2 );
		int ww = 2*d;
		dre->dl_el = m_dlist->Add( dre->m_id, (void*)dre, LAY_DRC_ERROR, 
			DL_HOLLOW_CIRC, 1, ww, 0, 0, x, y, 0, 0, x, y ); 
		dre->m_id.SetT2( ID_SEL_DRE );
		dre->dl_sel = m_dlist->AddSelector( dre->m_id, (void*)dre, LAY_DRC_ERROR, 
			DL_HOLLOW_CIRC, 1, ww, 0, x, y, 0, 0, x, y ); 
	}
	list.AddTail( dre );
	return dre;
}

// returns the graphic element for pad
// if none, returns the element for the pad's hole
//
dl_element * GetPadElement( part_pin * pin, int layer )
{
	dl_element * el = NULL;

	int il = layer - LAY_TOP_COPPER;
	if( layer > 0 && pin->dl_els.GetSize() > il )
		el = pin->dl_els[il];
	if( el == NULL )
		el = pin->dl_hole;
	return el;
}

// Highlight the error in the layout window
//
void DRErrorList::Highlight( DRError * dre )
{
	m_dlist->Add( dre->m_id, dre, LAY_HILITE, dre->dl_el->gtype, 1,
		m_dlist->Get_w( dre->dl_el ),
		m_dlist->Get_holew( dre->dl_el ), 0,
		m_dlist->Get_x( dre->dl_el ),
		m_dlist->Get_y( dre->dl_el ),
		m_dlist->Get_xf( dre->dl_el ),
		m_dlist->Get_yf( dre->dl_el ),
		m_dlist->Get_x_org( dre->dl_el ),
		m_dlist->Get_y_org( dre->dl_el ) );

	dl_element * dl1 = NULL; 
	dl_element * dl2 = NULL;

	if(    dre->m_id.T3() == DRError::PAD_PAD 
		|| dre->m_id.T3() == DRError::PAD_PADHOLE
		|| dre->m_id.T3() == DRError::PADHOLE_PADHOLE 
		|| dre->m_id.T3() == DRError::COPPERGRAPHIC_PAD
		|| dre->m_id.T3() == DRError::COPPERGRAPHIC_PADHOLE 
		)
	{
		// add highlights for pads
		cpart * part1 = m_plist->GetPartByName( dre->name1 );
		if( part1 && dre->id1.I2() < part1->pin.GetSize() )
//			dl1 = part1->pin[dre->id1.I2()].dl_el; 
			dl1 = GetPadElement( &part1->pin[dre->id1.I2()], dre->layer ); 
		cpart * part2 = m_plist->GetPartByName( dre->name2 );
		if( part2 && dre->id2.I2() < part2->pin.GetSize() )
//			dl2 = part2->pin[dre->id2.I2()].dl_el;
			dl2 = GetPadElement( &part2->pin[dre->id2.I2()], dre->layer );
	}
	else if(    dre->m_id.T3() == DRError::SEG_PAD 
			 || dre->m_id.T3() == DRError::SEG_PADHOLE
			 )
	{
		// add highlights for trace and pad
		cnet * net = m_nlist->GetNetPtrByName( &dre->name1 );
		if( net && dre->id1.I2() < net->NumCons() 
			&& dre->id1.I3() < net->ConByIndex(dre->id1.I2())->NumSegs() )
			dl1 = net->ConByIndex(dre->id1.I2())->SegByIndex(dre->id1.I3()).dl_el; 
		cpart * part = m_plist->GetPartByName( dre->name2 );
		if( part && dre->id2.I2() < part->pin.GetSize() )
			dl2 = GetPadElement( &part->pin[dre->id2.I2()], dre->layer );
	}
	else if(    dre->m_id.T3() == DRError::VIA_PAD
			 || dre->m_id.T3() == DRError::VIA_PADHOLE
			 || dre->m_id.T3() == DRError::VIAHOLE_PAD
			 || dre->m_id.T3() == DRError::VIAHOLE_PADHOLE
			 )
	{
		// add highlights for via and pad
		cnet * net = m_nlist->GetNetPtrByName( &dre->name1 );
		if( net && dre->id1.I2() < net->NumCons() 
			&& dre->id1.I3() <= net->ConByIndex(dre->id1.I2())->NumSegs() )
			dl1 = net->ConByIndex(dre->id1.I2())->VtxByIndex(dre->id1.I3()).dl_el[0]; 
		cpart * part = m_plist->GetPartByName( dre->name2 );
		if( part && dre->id2.I2() < part->pin.GetSize() )
			dl2 = GetPadElement( &part->pin[dre->id2.I2()], dre->layer );
	}
	else if( dre->m_id.T3() == DRError::SEG_SEG )
	{
		// add highlights for traces
		cnet * net1 = m_nlist->GetNetPtrByName( &dre->name1 );
		if( net1 && dre->id1.I2() < net1->NumCons() 
			&& dre->id1.I3() < net1->ConByIndex(dre->id1.I2())->NumSegs() )
			dl1 = net1->ConByIndex(dre->id1.I2())->SegByIndex(dre->id1.I3()).dl_el; 
		cnet * net2 = m_nlist->GetNetPtrByName( &dre->name2 );
		if( net2 && dre->id1.I2() < net2->NumCons() 
			&& dre->id2.I3() < net2->ConByIndex(dre->id2.I2())->NumSegs() )
			dl2 = net2->ConByIndex(dre->id2.I2())->SegByIndex(dre->id2.I3()).dl_el; 
	}
	else if(    dre->m_id.T3() == DRError::SEG_VIA
			 || dre->m_id.T3() == DRError::SEG_VIAHOLE
			 )
	{
		// add highlights for trace and via
		cnet * net1 = m_nlist->GetNetPtrByName( &dre->name1 );
		if( net1 && dre->id1.I2() < net1->NumCons() 
			&& dre->id1.I3() < net1->ConByIndex(dre->id1.I2())->NumSegs() )
			dl1 = net1->ConByIndex(dre->id1.I2())->SegByIndex(dre->id1.I3()).dl_el; 
		cnet * net2 = m_nlist->GetNetPtrByName( &dre->name2 );
		if( net2 && dre->id2.I2() < net2->NumCons() 
			&& dre->id2.I3() <= net2->ConByIndex(dre->id2.I2())->NumSegs() )
			dl2 = net2->ConByIndex(dre->id2.I2())->VtxByIndex(dre->id2.I3()).dl_el[0]; 
	}
	else if(    dre->m_id.T3() == DRError::VIA_VIA
			 || dre->m_id.T3() == DRError::VIA_VIAHOLE
			 || dre->m_id.T3() == DRError::VIAHOLE_VIAHOLE
			 )
	{
		// add highlights for vias
		cnet * net1 = m_nlist->GetNetPtrByName( &dre->name1 );
		if( net1 && dre->id1.I2() < net1->NumCons() 
			&& dre->id1.I3() <= net1->ConByIndex(dre->id1.I2())->NumSegs() )
			dl1 = net1->ConByIndex(dre->id1.I2())->VtxByIndex(dre->id1.I3()).dl_el[0]; 
		cnet * net2 = m_nlist->GetNetPtrByName( &dre->name2 );
		if( net2 && dre->id1.I2() < net2->NumCons() 
			&& dre->id2.I3() <= net2->ConByIndex(dre->id2.I2())->NumSegs() )
			dl2 = net2->ConByIndex(dre->id2.I2())->VtxByIndex(dre->id2.I3()).dl_el[0]; 
	}
	else if( dre->m_id.T3() == DRError::TRACE_WIDTH 
		|| dre->m_id.T3() == DRError::BOARDEDGE_SEG )
	{
		// add highlight for trace segment
		cnet * net1 = m_nlist->GetNetPtrByName( &dre->name1 );
		if( net1 && dre->id1.I2() < net1->NumCons() 
			&& dre->id1.I3() < net1->ConByIndex(dre->id1.I2())->NumSegs() )
			dl1 = net1->ConByIndex(dre->id1.I2())->SegByIndex(dre->id1.I3()).dl_el; 
	}
	else if( dre->m_id.T3() == DRError::RING_PAD 
		|| dre->m_id.T3() == DRError::BOARDEDGE_PAD 
		|| dre->m_id.T3() == DRError::BOARDEDGE_PADHOLE )
	{
		// add highlight for pad
		cpart * part = m_plist->GetPartByName( dre->name1 );
		if( part && dre->id1.I2() < part->pin.GetSize() )
			dl1 = GetPadElement( &part->pin[dre->id1.I2()], dre->layer );
	}
	else if( dre->m_id.T3() == DRError::RING_VIA
		|| dre->m_id.T3() == DRError::BOARDEDGE_VIA 
		|| dre->m_id.T3() == DRError::BOARDEDGE_VIAHOLE )
	{
		// add highlight for via
		cnet * net1 = m_nlist->GetNetPtrByName( &dre->name1 );
		if( net1 && dre->id1.I2() < net1->NumCons() 
			&& dre->id1.I3() <= net1->ConByIndex(dre->id1.I2())->NumSegs() )
			dl1 = net1->ConByIndex(dre->id1.I2())->VtxByIndex(dre->id1.I3()).dl_el[0]; 
	}
	else if( dre->m_id.T3() == DRError::BOARDEDGE_COPPERAREA )
	{
		// add highlight for copper area side
		cnet * net1 = m_nlist->GetNetPtrByName( &dre->name1 );
		if( net1 && dre->id1.I2() < net1->NumAreas() )
			net1->area[dre->id1.I2()].HighlightSide(dre->id1.I3()); 
	}
	else if( dre->m_id.T3() == DRError::COPPERAREA_COPPERAREA )
	{
		// add highlights for copper area sides
		cnet * net1 = m_nlist->GetNetPtrByName( &dre->name1 );
		if( net1 && dre->id1.I2() < net1->NumAreas() )
			net1->area[dre->id1.I2()].HighlightSide(dre->id1.I3()); 
		cnet * net2 = m_nlist->GetNetPtrByName( &dre->name2 );
		if( net2 && dre->id2.I2() < net2->NumAreas() )
			net2->area[dre->id2.I2()].HighlightSide(dre->id2.I3()); 
	}
	else if( dre->m_id.T3() == DRError::COPPERAREA_INSIDE_COPPERAREA )
	{
	}
	else if( dre->m_id.T3() == DRError::UNROUTED )
	{
		// add highlights for pad
		cpart * part1 = m_plist->GetPartByName( dre->name1 );
		if( part1 && dre->id1.I2() < part1->pin.GetSize() )
			dl1 = GetPadElement( &part1->pin[dre->id1.I2()], dre->layer );
		cpart * part2 = m_plist->GetPartByName( dre->name2 );
		if( part2 && dre->id2.I2() < part2->pin.GetSize() )
			dl2 = GetPadElement( &part2->pin[dre->id2.I2()], dre->layer );
	}
	else if (dre->m_id.T3() == DRError::COPPERAREA_BROKEN)
		;															// CPT

	else
		ASSERT(0);
	if( dl1 )
	{
		m_dlist->Add( dre->m_id, dre, LAY_HILITE, dl1->gtype, 1,
			m_dlist->Get_w( dl1 ),
			m_dlist->Get_holew( dl1 ),
			m_dlist->Get_x( dl1 ),
			m_dlist->Get_y( dl1 ),
			m_dlist->Get_xf( dl1 ),
			m_dlist->Get_yf( dl1 ),
			m_dlist->Get_x_org( dl1 ),
			m_dlist->Get_y_org( dl1 ),
			m_dlist->Get_radius( dl1 ) );
	}
	if( dl2 )
	{
		m_dlist->Add( dre->m_id, dre, LAY_HILITE, dl2->gtype, 1,
			m_dlist->Get_w( dl2 ),
			m_dlist->Get_holew( dl2 ),
			m_dlist->Get_x( dl2 ),
			m_dlist->Get_y( dl2 ),
			m_dlist->Get_xf( dl2 ),
			m_dlist->Get_yf( dl2 ),
			m_dlist->Get_x_org( dl2 ),
			m_dlist->Get_y_org( dl2 ),
			m_dlist->Get_radius( dl2 ) );
	}
}

// Make symbol for the error a solid circle with diameter = 250 mils
//
void DRErrorList::MakeSolidCircles()
{
#ifndef CPT2
	POSITION pos;
	void * ptr;

	if( !list.IsEmpty() )
	{
		for( pos = list.GetHeadPosition(); pos != NULL; )
		{
			ptr = list.GetNext( pos );
			DRError * dre = (DRError*)ptr;
			if( dre->dl_el )
			{
				if( dre->dl_el->gtype == DL_HOLLOW_CIRC )
				{
					m_dlist->Set_gtype( dre->dl_el, DL_CIRC );
					m_dlist->Set_holew( dre->dl_el, m_dlist->Get_w( dre->dl_el ) );
					m_dlist->Set_w( dre->dl_el, 250*NM_PER_MIL );
				}
			}
		}
	}
#endif
}

// Make symbol for error a ring 
//
void DRErrorList::MakeHollowCircles()
{
#ifndef CPT2
	POSITION pos;
	void * ptr;

	if( !list.IsEmpty() )
	{
		for( pos = list.GetHeadPosition(); pos != NULL; )
		{
			ptr = list.GetNext( pos );
			DRError * dre = (DRError*)ptr;
			if( dre->dl_el )
			{
				if( dre->dl_el->gtype == DL_CIRC )
				{
					m_dlist->Set_gtype( dre->dl_el, DL_HOLLOW_CIRC );
					m_dlist->Set_w( dre->dl_el, m_dlist->Get_holew( dre->dl_el ) );
					m_dlist->Set_holew( dre->dl_el, 0 );
				}
			}
		}
	}
#endif
}

*/

cdre *cdrelist::Add( int type, CString * str, cpcb_item *item1, cpcb_item *item2,
		int x1, int y1, int x2, int y2, int w, int layer )
{
	// CPT2 converted from DRErrorList function
	// find center of coords
	int x = x1;
	int y = y1;
	if( item2 )
	{
		x = (x1 + x2)/2;
		y = (y1 + y2)/2;
	}

	// first check for redundant error.  CPT: allow repeats of COPPERAREA_BROKEN errors
	if( type != cdre::COPPERAREA_BROKEN )	
	{
		citer<cdre> id (&dres);
		for (cdre *dre = id.First(); dre; dre = id.Next())
		{
			// compare current error with error from list
			if( dre->item1 == item1 && dre->item2 == item2 )
				return NULL;
			if( dre->item1 == item2 && dre->item2 == item1 )
				return NULL;
			// if same traces at same point, don't add it
			if( item2 && dre->item2 )
			{
				cconnect2 *old1 = dre->item1->GetConnect(), *old2 = dre->item2->GetConnect();
				cconnect2 *new1 = item1->GetConnect(), *new2 = item2->GetConnect();
				if (old1==new1 && old2==new2 && dre->x==x && dre->y==y)
					return NULL;
				if (old1==new2 && old2==new1 && dre->x==x && dre->y==y)
					return NULL;
			}

			// if RING_PAD error on same pad, don't add it
			if( type == cdre::RING_PAD && dre->type == cdre::RING_PAD )		// CPT2 second clause was dre->m_id.T3().  Check if I've translated right...
				if( item1 == dre->item1 )
					return NULL;

			// if BOARDEDGE_PAD or BOARDEDGE_PADHOLE error on same pad, don't add it
			if( (type == cdre::BOARDEDGE_PAD || type == cdre::BOARDEDGE_PADHOLE)
				&& (dre->type == cdre::BOARDEDGE_PAD || dre->type == cdre::BOARDEDGE_PADHOLE) )
					if( item1 == dre->item1 )
						return NULL;

			// if RING_VIA error on same via, don't add it
			if( type == cdre::RING_VIA && dre->type == cdre::RING_VIA )
				if( item1 == dre->item1 )
					return NULL;

			// if BOARDEDGE_VIA or BOARDEDGE_VIAHOLE error on same via, don't add it
			if( (type == cdre::BOARDEDGE_VIA || type == cdre::BOARDEDGE_VIAHOLE)
				&& (dre->type == cdre::BOARDEDGE_VIA || dre->type == cdre::BOARDEDGE_VIAHOLE) )
					if( item1 == dre->item1 )
						return NULL;

			// if BOARDEDGE_SEG on same trace at same point, don't add it
			if( type == cdre::BOARDEDGE_SEG && dre->type == cdre::BOARDEDGE_SEG )
				if( item1 == dre->item1 )													// Sufficient check?
					return NULL;
			
			// if BOARDEDGE_COPPERAREA on same area at same point, don't add it
			if( type == cdre::BOARDEDGE_COPPERAREA && dre->type == cdre::BOARDEDGE_COPPERAREA )
				if( item1->GetPolyline() == dre->item1->GetPolyline()						// CPT2 Are the "->GetPolyline()" invocations essential?
					&& x == dre->x  && y == dre->y ) 
					return NULL;
				
			if( type == cdre::COPPERAREA_COPPERAREA && dre->type == cdre::COPPERAREA_COPPERAREA )		
				if( x == dre->x && y == dre->y)
					if( item1 == dre->item1 && item2 == dre->item2 || 
					    item1 == dre->item2 && item2 == dre->item1) 
							return NULL;
		}
	}

	int d = 50*NM_PER_MIL;																		// Minimum width
	if( item2 )
		d = max( d, Distance( x1, y1, x2, y2 ) );
	if( w )
		d = max( d, w );

	cdre *dre = new cdre(doc, GetSize(), type, str, item1, item2, x, y, d, layer);
	dres.Add(dre);
	dre->MustRedraw();																				// CPT2 TODO maybe...
	return dre;
}

void cdrelist::Remove(cdre *dre)
{
	dre->SaveUndoInfo();
	dre->Undraw();
	dres.Remove(dre);
}

void cdrelist::Clear()
{
	// CPT2 new.  Undraw and then remove from "this" all design-rule-error objects.  NB also save undo info (it seemed best to incorporate
	// dre's into the undo system).
	citer<cdre> id (&dres);
	for (cdre* d = id.First(); d; d = id.Next())
		d->SaveUndoInfo(),
		d->Undraw();
	dres.RemoveAll();
}

void cdrelist::MakeSolidCircles()
{
	CDisplayList *dl = doc->m_dlist;
	citer<cdre> id (&dres);
	for (cdre *d = id.First(); d; d = id.Next())
	{
		if (!d->dl_el || d->dl_el->gtype != DL_HOLLOW_CIRC || d->dl_el->holew) 
			continue;																// Weird...
		dl->Set_holew( d->dl_el, 10000000 );						// CPT2 has the effect that the circle is drawn solid.  An ugly hack, I know...
		d->dl_el->radius = d->dl_el->w;								// A safe place to store the normal width (I guess)
		dl->Set_w( d->dl_el, 250*NM_PER_MIL );
	}
}

void cdrelist::MakeHollowCircles()
{
	// Return dre symbol to normal (a hollow ring)
	CDisplayList *dl = doc->m_dlist;
	citer<cdre> id (&dres);
	for (cdre *d = id.First(); d; d = id.Next())
	{
		if (!d->dl_el || d->dl_el->gtype != DL_HOLLOW_CIRC || !d->dl_el->holew) 
			continue;																// Weird...
		d->dl_el->w = d->dl_el->radius;
		dl->Set_holew( d->dl_el, 0 );
	}
}
