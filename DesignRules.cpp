// Implementation of class DRErrorList
//
#include "stdafx.h"

//******************************************************
//
// class DRError
//
// constructor
//
DRError::DRError()
{
	dl_el = NULL;
	dl_sel = NULL;
}

DRError::~DRError()
{
	if( dl_el )
	{
		dl_el->Remove();
		dl_sel->Remove();
	}
}



//******************************************************
//
// class DRErrorList
//
// constructor
//
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
	if( !list.IsEmpty() )
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
				if( id1.type == ID_NET && id1.st == ID_CONNECT
					&& dre->id1.type == ID_NET && dre->id1.st == ID_CONNECT
					&& id1.i == dre->id1.i
					&& id2.type == ID_NET && id2.st == ID_CONNECT
					&& dre->id2.type == ID_NET && dre->id2.st == ID_CONNECT
					&& id2.i == dre->id2.i
					&& x == dre->x && y == dre->y )
					return NULL;
				if( id2.type == ID_NET && id2.st == ID_CONNECT
					&& dre->id1.type == ID_NET && dre->id1.st == ID_CONNECT
					&& id2.i == dre->id1.i
					&& id1.type == ID_NET && id1.st == ID_CONNECT
					&& dre->id2.type == ID_NET && dre->id2.st == ID_CONNECT
					&& id1.i == dre->id2.i
					&& x == dre->x && y == dre->y )
					return NULL;
			}
			// if RING_PAD error on same pad, don't add it
			if( type == DRError::RING_PAD && dre->m_id.sst == DRError::RING_PAD )
			{
				if( *name1 == dre->name1
					&& id1.i == dre->id1.i )
				{
					// same pad, ignore it
					return NULL;
				}
			}
			// if BOARDEDGE_PAD or BOARDEDGE_PADHOLE error on same pad, don't add it
			if( (type == DRError::BOARDEDGE_PAD || type == DRError::BOARDEDGE_PADHOLE)
				&& (dre->m_id.sst == DRError::BOARDEDGE_PAD || dre->m_id.sst == DRError::BOARDEDGE_PADHOLE) )
			{
				if( *name1 == dre->name1
					&& id1.i == dre->id1.i )
				{
					// same pad, ignore it
					return NULL;
				}
			}
			// if RING_VIA error on same via, don't add it
			if( type == DRError::RING_VIA && dre->m_id.sst == DRError::RING_VIA )
			{
				if( *name1 == dre->name1
					&& id1.i == dre->id1.i
					&& id1.ii == dre->id1.ii )
				{
					// same via, ignore it
					return NULL;
				}
			}
			// if BOARDEDGE_VIA or BOARDEDGE_VIAHOLE error on same via, don't add it
			if( (type == DRError::BOARDEDGE_VIA || type == DRError::BOARDEDGE_VIAHOLE)
				&& (dre->m_id.sst == DRError::BOARDEDGE_VIA || dre->m_id.sst == DRError::BOARDEDGE_VIAHOLE) )
			{
				if( *name1 == dre->name1
					&& id1.i == dre->id1.i
					&& id1.ii == dre->id1.ii )
				{
					// same via, ignore it
					return NULL;
				}
			}
			// if BOARDEDGE_TRACE on same trace at same point, don't add it
			if( type == DRError::BOARDEDGE_TRACE && dre->m_id.sst == DRError::BOARDEDGE_TRACE )
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
				&& dre->m_id.sst == DRError::BOARDEDGE_COPPERAREA )
			{
				if( *name1 == dre->name1
					&& id1.i == dre->id1.i
					&& x == dre->x
					&& y == dre->y )
				{
					return NULL;
				}
			}
			if( type == DRError::COPPERAREA_COPPERAREA
				&& dre->m_id.sst == DRError::COPPERAREA_COPPERAREA )
			{
				if( *name1 == dre->name1
					&& *name2 == dre->name2
					&& id1.i == dre->id1.i
					&& id2.i == dre->id2.i
					&& x == dre->x
					&& y == dre->y )
					return NULL;
			}
		}
	}

	DRError * dre = new DRError;
	// make id
	dre->m_id.type = ID_DRC;
	dre->m_id.st = ID_DRE;
	dre->m_id.i = (unsigned int)dre;
	dre->m_id.sst = type;
	dre->m_id.ii = index;
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
		dre->m_id.st = ID_SEL_DRE;
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
void DRErrorList::HighLight( DRError * dre )
{
	m_dlist->Add( dre->m_id, dre, LAY_HILITE, dre->dl_el->gtype, 1,
		m_dlist->Get_w( dre->dl_el ),
		m_dlist->Get_holew( dre->dl_el ),
		0, // no clearance needed
		m_dlist->Get_x( dre->dl_el ),
		m_dlist->Get_y( dre->dl_el ),
		m_dlist->Get_xf( dre->dl_el ),
		m_dlist->Get_yf( dre->dl_el ),
		m_dlist->Get_x_org( dre->dl_el ),
		m_dlist->Get_y_org( dre->dl_el ) );

	dl_element * dl1 = NULL;
	dl_element * dl2 = NULL;

	if(    dre->m_id.sst == DRError::PAD_PAD
		|| dre->m_id.sst == DRError::PAD_PADHOLE
		|| dre->m_id.sst == DRError::PADHOLE_PADHOLE
		)
	{
		// add highlights for pads
		cpart * part1 = m_plist->GetPart( dre->name1 );
		if( part1 && dre->id1.i < part1->pin.GetSize() )
//			dl1 = part1->pin[dre->id1.i].dl_el;
			dl1 = GetPadElement( &part1->pin[dre->id1.i], dre->layer );
		cpart * part2 = m_plist->GetPart( dre->name2 );
		if( part2 && dre->id2.i < part2->pin.GetSize() )
//			dl2 = part2->pin[dre->id2.i].dl_el;
			dl2 = GetPadElement( &part2->pin[dre->id2.i], dre->layer );
	}
	else if(    dre->m_id.sst == DRError::SEG_PAD
			 || dre->m_id.sst == DRError::SEG_PADHOLE
			 )
	{
		// add highlights for trace and pad
		cnet * net = m_nlist->GetNetPtrByName( dre->name1 );
		if( net && dre->id1.i < net->nconnects && dre->id1.ii < net->connect[dre->id1.i].nsegs )
			dl1 = net->connect[dre->id1.i].seg[dre->id1.ii].dl_el;
		cpart * part = m_plist->GetPart( dre->name2 );
		if( part && dre->id2.i < part->pin.GetSize() )
			dl2 = GetPadElement( &part->pin[dre->id2.i], dre->layer );
	}
	else if(    dre->m_id.sst == DRError::VIA_PAD
			 || dre->m_id.sst == DRError::VIA_PADHOLE
			 || dre->m_id.sst == DRError::VIAHOLE_PAD
			 || dre->m_id.sst == DRError::VIAHOLE_PADHOLE
			 )
	{
		// add highlights for via and pad
		cnet * net = m_nlist->GetNetPtrByName( dre->name1 );
		if( net && dre->id1.i < net->nconnects && dre->id1.ii <= net->connect[dre->id1.i].nsegs )
			dl1 = net->connect[dre->id1.i].vtx[dre->id1.ii].dl_el[0];
		cpart * part = m_plist->GetPart( dre->name2 );
		if( part && dre->id2.i < part->pin.GetSize() )
			dl2 = GetPadElement( &part->pin[dre->id2.i], dre->layer );
	}
	else if( dre->m_id.sst == DRError::SEG_SEG )
	{
		// add highlights for traces
		cnet * net1 = m_nlist->GetNetPtrByName( dre->name1 );
		if( net1 && dre->id1.i < net1->nconnects && dre->id1.ii < net1->connect[dre->id1.i].nsegs )
			dl1 = net1->connect[dre->id1.i].seg[dre->id1.ii].dl_el;
		cnet * net2 = m_nlist->GetNetPtrByName( dre->name2 );
		if( net2 && dre->id1.i < net2->nconnects && dre->id2.ii < net2->connect[dre->id2.i].nsegs )
			dl2 = net2->connect[dre->id2.i].seg[dre->id2.ii].dl_el;
	}
	else if(    dre->m_id.sst == DRError::SEG_VIA
			 || dre->m_id.sst == DRError::SEG_VIAHOLE
			 )
	{
		// add highlights for trace and via
		cnet * net1 = m_nlist->GetNetPtrByName( dre->name1 );
		if( net1 && dre->id1.i < net1->nconnects && dre->id1.ii < net1->connect[dre->id1.i].nsegs )
			dl1 = net1->connect[dre->id1.i].seg[dre->id1.ii].dl_el;
		cnet * net2 = m_nlist->GetNetPtrByName( dre->name2 );
		if( net2 && dre->id2.i < net2->nconnects && dre->id2.ii <= net2->connect[dre->id2.i].nsegs )
			dl2 = net2->connect[dre->id2.i].vtx[dre->id2.ii].dl_el[0];
	}
	else if(    dre->m_id.sst == DRError::VIA_VIA
			 || dre->m_id.sst == DRError::VIA_VIAHOLE
			 || dre->m_id.sst == DRError::VIAHOLE_VIAHOLE
			 )
	{
		// add highlights for vias
		cnet * net1 = m_nlist->GetNetPtrByName( dre->name1 );
		if( net1 && dre->id1.i < net1->nconnects && dre->id1.ii <= net1->connect[dre->id1.i].nsegs )
			dl1 = net1->connect[dre->id1.i].vtx[dre->id1.ii].dl_el[0];
		cnet * net2 = m_nlist->GetNetPtrByName( dre->name2 );
		if( net2 && dre->id1.i < net2->nconnects && dre->id2.ii <= net2->connect[dre->id2.i].nsegs )
			dl2 = net2->connect[dre->id2.i].vtx[dre->id2.ii].dl_el[0];
	}
	else if( dre->m_id.sst == DRError::TRACE_WIDTH
		|| dre->m_id.sst == DRError::BOARDEDGE_TRACE )
	{
		// add highlight for trace segment
		cnet * net1 = m_nlist->GetNetPtrByName( dre->name1 );
		if( net1 && dre->id1.i < net1->nconnects && dre->id1.ii < net1->connect[dre->id1.i].nsegs )
			dl1 = net1->connect[dre->id1.i].seg[dre->id1.ii].dl_el;
	}
	else if( dre->m_id.sst == DRError::RING_PAD
		|| dre->m_id.sst == DRError::BOARDEDGE_PAD
		|| dre->m_id.sst == DRError::BOARDEDGE_PADHOLE )
	{
		// add highlight for pad
		cpart * part = m_plist->GetPart( dre->name1 );
		if( part && dre->id1.i < part->pin.GetSize() )
			dl1 = GetPadElement( &part->pin[dre->id1.i], dre->layer );
	}
	else if( dre->m_id.sst == DRError::RING_VIA
		|| dre->m_id.sst == DRError::BOARDEDGE_VIA
		|| dre->m_id.sst == DRError::BOARDEDGE_VIAHOLE )
	{
		// add highlight for via
		cnet * net1 = m_nlist->GetNetPtrByName( dre->name1 );
		if( net1 && dre->id1.i < net1->nconnects && dre->id1.ii <= net1->connect[dre->id1.i].nsegs )
			dl1 = net1->connect[dre->id1.i].vtx[dre->id1.ii].dl_el[0];
	}
	else if( dre->m_id.sst == DRError::BOARDEDGE_COPPERAREA )
	{
		// add highlight for copper area side
		cnet * net1 = m_nlist->GetNetPtrByName( dre->name1 );
		if( net1 && dre->id1.i < net1->nareas )
			net1->area[dre->id1.i].poly->HighlightSide(dre->id1.ii);
	}
	else if( dre->m_id.sst == DRError::COPPERAREA_COPPERAREA )
	{
		// add highlights for copper area sides
		cnet * net1 = m_nlist->GetNetPtrByName( dre->name1 );
		if( net1 && dre->id1.i < net1->nareas )
			net1->area[dre->id1.i].poly->HighlightSide(dre->id1.ii);
		cnet * net2 = m_nlist->GetNetPtrByName( dre->name2 );
		if( net2 && dre->id2.i < net2->nareas )
			net2->area[dre->id2.i].poly->HighlightSide(dre->id2.ii);
	}
	else if( dre->m_id.sst == DRError::COPPERAREA_INSIDE_COPPERAREA )
	{
	}
	else if( dre->m_id.sst == DRError::UNROUTED )
	{
		// add highlights for pad
		cpart * part1 = m_plist->GetPart( dre->name1 );
		if( part1 && dre->id1.i < part1->pin.GetSize() )
			dl1 = GetPadElement( &part1->pin[dre->id1.i], dre->layer );
		cpart * part2 = m_plist->GetPart( dre->name2 );
		if( part2 && dre->id2.i < part2->pin.GetSize() )
			dl2 = GetPadElement( &part2->pin[dre->id2.i], dre->layer );
	}
	else
		ASSERT(0);

	if( dl1 )
	{
		m_dlist->Add( dre->m_id, dre, LAY_HILITE, dl1->gtype, 1,
			m_dlist->Get_w( dl1 ),
			m_dlist->Get_holew( dl1 ),
			0, // no clearance needed
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
			0, // no clearance needed
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
					dl_element *old_element = dre->dl_el;
					dl_element *old_sel     = dre->dl_sel;

					dre->dl_el = m_dlist->MorphDLE(old_element, DL_CIRC);
					m_dlist->Set_holew( dre->dl_el, m_dlist->Get_w( old_element ) );
					m_dlist->Set_w    ( dre->dl_el, 250*NM_PER_MIL );

                    dre->dl_sel = m_dlist->MorphDLE(old_sel, DL_CIRC);
					m_dlist->Set_holew( dre->dl_sel, m_dlist->Get_w( old_sel ) );
					m_dlist->Set_w    ( dre->dl_sel, 250*NM_PER_MIL );

					old_element->Remove();
                    old_sel->Remove();
				}
			}
		}
	}
}

// Make symbol for error a ring
//
void DRErrorList::MakeHollowCircles()
{
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
					dl_element *old_element = dre->dl_el;
					dl_element *old_sel     = dre->dl_sel;

					dre->dl_el = m_dlist->MorphDLE(old_element, DL_HOLLOW_CIRC);
					m_dlist->Set_w    ( dre->dl_el, m_dlist->Get_holew( old_element ) );
					m_dlist->Set_holew( dre->dl_el, 0 );

                    dre->dl_sel = m_dlist->MorphDLE(old_sel, DL_HOLLOW_CIRC);
					m_dlist->Set_w    ( dre->dl_sel, m_dlist->Get_holew( old_sel ) );
					m_dlist->Set_holew( dre->dl_sel, 0 );

					old_element->Remove();
                    old_sel->Remove();
				}
			}
		}
	}
}
