//  DesignRules.cpp.  Source file for classes related to design-rule checking, including CDre (which descends from CPcbItem), CDreList, and DesignRules.

#include "stdafx.h"


CDre::CDre(CFreePcbDoc *_doc, int _index, int _type, CString *_str, CPcbItem *_item1, CPcbItem *_item2, 
		   int _x, int _y, int _w, int _layer) 
	: CPcbItem(_doc)
{ 
	index = _index;
	type = _type;
	str = *_str;
	item1 = _item1; 
	item2 = _item2;
	x = _x, y = _y;
	w = _w;
	layer = _layer;
}

CDre::CDre(CFreePcbDoc *_doc, int _uid):
	CPcbItem(_doc, _uid)
{ 
	item1 = item2 = NULL;
}

bool CDre::IsOnPcb()
{
	return doc->m_drelist->dres.Contains(this);
}

int CDre::Draw()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl )
		return NO_DLIST;
	if (bDrawn)
		return ALREADY_DRAWN;
	dl_el = dl->AddMain( this, LAY_DRC_ERROR, DL_HOLLOW_CIRC, 1, w, 0, 0, x, y, 0, 0, x, y ); 
	dl_sel = dl->AddSelector( this, LAY_DRC_ERROR, DL_HOLLOW_CIRC, 1, w, 0, x, y, 0, 0, x, y ); 
	bDrawn = true;
	return NOERR;
}

void CDre::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	if( !dl_sel ) return;
	dl->Highlight( DL_HOLLOW_CIRC, 
		dl->Get_x( dl_sel ), dl->Get_y( dl_sel ),
		dl->Get_xf( dl_sel ), dl->Get_yf( dl_sel ),
		dl->Get_w( dl_sel ) );
}

CDre *CDreList::Add( int type, CString * str, CPcbItem *item1, CPcbItem *item2,
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
	if( type != CDre::COPPERAREA_BROKEN )	
	{
		CIter<CDre> id (&dres);
		for (CDre *dre = id.First(); dre; dre = id.Next())
		{
			// compare current error with error from list
			if( dre->item1 == item1 && dre->item2 == item2 )
				return NULL;
			if( dre->item1 == item2 && dre->item2 == item1 )
				return NULL;
			// if same traces at same point, don't add it
			if( item2 && dre->item2 )
			{
				CConnect *old1 = dre->item1->GetConnect(), *old2 = dre->item2->GetConnect();
				CConnect *new1 = item1->GetConnect(), *new2 = item2->GetConnect();
				if (old1==new1 && old2==new2 && dre->x==x && dre->y==y)
					return NULL;
				if (old1==new2 && old2==new1 && dre->x==x && dre->y==y)
					return NULL;
			}

			// if RING_PAD error on same pad, don't add it
			if( type == CDre::RING_PAD && dre->type == CDre::RING_PAD )		// CPT2 second clause was dre->m_id.T3().  Check if I've translated right...
				if( item1 == dre->item1 )
					return NULL;

			// if BOARDEDGE_PAD or BOARDEDGE_PADHOLE error on same pad, don't add it
			if( (type == CDre::BOARDEDGE_PAD || type == CDre::BOARDEDGE_PADHOLE)
				&& (dre->type == CDre::BOARDEDGE_PAD || dre->type == CDre::BOARDEDGE_PADHOLE) )
					if( item1 == dre->item1 )
						return NULL;

			// if RING_VIA error on same via, don't add it
			if( type == CDre::RING_VIA && dre->type == CDre::RING_VIA )
				if( item1 == dre->item1 )
					return NULL;

			// if BOARDEDGE_VIA or BOARDEDGE_VIAHOLE error on same via, don't add it
			if( (type == CDre::BOARDEDGE_VIA || type == CDre::BOARDEDGE_VIAHOLE)
				&& (dre->type == CDre::BOARDEDGE_VIA || dre->type == CDre::BOARDEDGE_VIAHOLE) )
					if( item1 == dre->item1 )
						return NULL;

			// if BOARDEDGE_SEG on same trace at same point, don't add it
			if( type == CDre::BOARDEDGE_SEG && dre->type == CDre::BOARDEDGE_SEG )
				if( item1 == dre->item1 )													// Sufficient check?
					return NULL;
			
			// if BOARDEDGE_COPPERAREA on same area at same point, don't add it
			if( type == CDre::BOARDEDGE_COPPERAREA && dre->type == CDre::BOARDEDGE_COPPERAREA )
				if( item1->GetPolyline() == dre->item1->GetPolyline()						// CPT2 Are the "->GetPolyline()" invocations essential?
					&& x == dre->x  && y == dre->y ) 
					return NULL;
				
			if( type == CDre::COPPERAREA_COPPERAREA && dre->type == CDre::COPPERAREA_COPPERAREA )		
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

	CDre *dre = new CDre(doc, GetSize(), type, str, item1, item2, x, y, d, layer);
	dres.Add(dre);
	dre->MustRedraw();																				// CPT2 TODO maybe...
	return dre;
}

void CDreList::Remove(CDre *dre)
{
	dre->SaveUndoInfo();
	dre->Undraw();
	dres.Remove(dre);
}

void CDreList::Clear()
{
	// CPT2 new.  Undraw and then remove from "this" all design-rule-error objects.  NB also save undo info (it seemed best to incorporate
	// dre's into the undo system).
	CIter<CDre> id (&dres);
	for (CDre* d = id.First(); d; d = id.Next())
		d->SaveUndoInfo(),
		d->Undraw();
	dres.RemoveAll();
}

void CDreList::MakeSolidCircles()
{
	CDisplayList *dl = doc->m_dlist;
	CIter<CDre> id (&dres);
	for (CDre *d = id.First(); d; d = id.Next())
	{
		if (!d->dl_el || d->dl_el->gtype != DL_HOLLOW_CIRC || d->dl_el->holew) 
			continue;																// Weird...
		dl->Set_holew( d->dl_el, 10000000 );						// CPT2 has the effect that the circle is drawn solid.  An ugly hack, I know...
		d->dl_el->radius = d->dl_el->w;								// A safe place to store the normal width (I guess)
		dl->Set_w( d->dl_el, 250*NM_PER_MIL );
	}
}

void CDreList::MakeHollowCircles()
{
	// Return dre symbol to normal (a hollow ring)
	CDisplayList *dl = doc->m_dlist;
	CIter<CDre> id (&dres);
	for (CDre *d = id.First(); d; d = id.Next())
	{
		if (!d->dl_el || d->dl_el->gtype != DL_HOLLOW_CIRC || !d->dl_el->holew) 
			continue;																// Weird...
		d->dl_el->w = d->dl_el->radius;
		dl->Set_holew( d->dl_el, 0 );
	}
}
