#include "stdafx.h"
#include <math.h>
#include <stdlib.h>
#include "PcbItem.h"
#include "NetListNew.h"
#include "FreePcbDoc.h"
#include "PartListNew.h"

class CFreePcbDoc;
class cpcb_item;

int cpcb_item::next_uid = 1;

cpcb_item::cpcb_item(CFreePcbDoc *_doc)
{
	carray_list = NULL; 
	m_uid = next_uid++;
	doc = _doc;
	if (doc)
		doc->items.Add(this);
	dl_el = dl_sel = NULL;
	utility = 0;
	bDrawn = false;
}

cpcb_item::~cpcb_item()
{
	// When an item is destroyed, references to it are automatically removed from any carrays to which it belongs:
	for (carray_link *link = carray_list, *next; link; link = next)
	{
		carray<cpcb_item> *arr = (carray<cpcb_item>*) link->arr;
		int off = link->off;
		ASSERT(arr->heap[off]==this);				// For now...
		arr->heap[off] = (cpcb_item*) arr->free;	// Add "off" to arr's free-offset list
		arr->free = off;
		arr->flags[off>>3] &= ~(1 << (off&7));
		arr->nItems--;
		next = link->next;
		delete link;
	}
}

/*
void cpcb_item::GarbageCollect() {
	// Part of my proposed new architecture is to take some of the hassle out of memory management for pcb-items by using garbage collection.
	// Each time an item is constructed, it is added to the "items" array.  If an item goes out of use, one does not have to "delete" it;
	// just unhook it from its parent entity or array.  When garbage collection time comes, we'll first clear the utility bits on all members of
	// "items".  Then we'll scan through the doc's netlist (recursing through connects, segs, vtxs, tees, areas), partlist (recursing through pins), 
	// textlist, and otherlist, marking utility bits as we go.  At the end, items with clear utility bits can be deleted.
	citer<cpcb_item> ii (&items);
	for (cpcb_item *i = ii.First(); i; i = ii.Next())
		i->utility = 0;

	citer<cnet2> in (&m_nlist->nets);
	for (cnet2 *n = in.First(); n; n = in.Next()) {
		n->utility = 1;
		citer<cconnect2> ic (&n->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next()) {
			c->utility = 1;
			citer<cvertex2> iv (&c->vtxs);
			for (cvertex2 *v = iv.First(); v; v = iv.Next()) {
				v->utility = 1;
				if (v->tee) v->tee->utility = 1;
				}
			citer<cseg2> is (&c->segs);
			for (cseg2 *s = is.First(); s; s = is.Next())
				s->utility = 1;
			}
		citer<carea2> ia (&n->areas);
		for (carea2 *a = ia.First(); a; a = ia.Next())
			a->MarkConstituents();
		}
	citer<cpart2> ip (&m_plist->parts);
	for (cpart2 *p = ip.First(); p; p = ip.Next()) {
		p->utility = 1;
		citer<cpin2> ipin (&p->pins);
		for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
			pin->utility = 1;
		}
	citer<ctext> it (&m_tlist->texts);
	for (ctext *t = it.First(); t; t = it.Next()) 
		t->utility = 1;
	citer<cpcb_item> ii2 (&others);
	for (cpcb_item *i = ii2.First(); i; i = ii2.Next()) {
		if (cpolyline *p = i->ToPolyline())
			p->MarkConstituents();
		else
			p->utility = 1;

	// Do the deletions of unused items!
	for (cpcb_item *i = ii.First(); i; i = ii.Next())
		if (!i.utility)
			delete i;
	}

*/

/**********************************************************************************************/
/*  RELATED TO cseg2/cvertex2/cconnect2                                                          */
/**********************************************************************************************/

cvertex2::cvertex2(cconnect2 *c, int _x, int _y):				// Added args
	cpcb_item(c->doc)
{
	m_con = c;
	m_con->vtxs.Add(this);
	m_net = c->m_net;
	x = _x, y = _y;
	tee = NULL;
	pin = NULL;
	preSeg = postSeg = NULL;
	force_via_flag = via_w = via_hole_w = 0;
	dl_hole = NULL;
}

bool cvertex2::Remove()
{
	// Derived from old cnet::RemoveVertex() functions.  Remove vertex from the network.  If it's a tee-vertex, all associated vertices must be
	// axed as well.  If it's an end vertex of another type, remove vertex and the single attached seg.  If it's a middle vertex, merge the two adjacent
	// segs.
	Undraw();																	// To get rid of selector drawing-el (?)
	if (tee)
	{
		citer<cvertex2> iv (&tee->vtxs);
		for (cvertex2 *v = iv.First(); v; v = iv.Next())
		{
			v->tee = NULL;
			if (v!=this) v->Remove();
		}
	// Fall thru and remove this just like any other end vertex...
	}

	int nSegsInConnect = m_con->segs.GetSize();
	if (nSegsInConnect<2) 
		{ m_con->Remove(); return true; }

	m_con->vtxs.Remove(this);
	if (!preSeg)
		postSeg->RemoveBreak();
	else if (!postSeg)
		preSeg->RemoveBreak();
	else 
	{
		if (preSeg->m_layer != postSeg->m_layer)
			postSeg->SetLayer(LAY_RAT_LINE);
		preSeg->RemoveMerge(1);
		m_con->MergeUnroutedSegments();
	}

	return false;
}

bool cvertex2::IsViaNeeded()
{
	if (tee) 
	{
		int layer0 = -1;
		citer<cvertex2> iv (&tee->vtxs);
		for (cvertex2 *v = iv.First(); v; v = iv.Next())
		{
			if (v->force_via_flag) return true;							// If any constituent vtx has a forced via, the tee needs one too
			int layer = v->preSeg? v->preSeg->m_layer: LAY_RAT_LINE;
			if (layer!=LAY_RAT_LINE)
				if (layer0==-1) layer0 = layer;
				else if (layer!=layer0) return true;
			layer = v->postSeg? v->postSeg->m_layer: LAY_RAT_LINE;
			if (layer!=LAY_RAT_LINE)
				if (layer0==-1) layer0 = layer;
				else if (layer!=layer0) return true;
		}
		return false;
	}

	// Non-tee:
	if (pin) return false;
	if (force_via_flag) return true;
	if (!preSeg || !postSeg) return false;					// CPT2 TODO: check...
	if (preSeg->m_layer == postSeg->m_layer) return false;
	return preSeg->m_layer!=LAY_RAT_LINE && postSeg->m_layer!=LAY_RAT_LINE;
}

void cvertex2::ReconcileVia()
{
	// CPT2.  Gets an appropriate width for a via on this vertex, if any.  NB does not do any drawing.
	if (IsViaNeeded()) 
	{
		// CPT r295. Calculate via width, based on the widths of the adjacent segs and possibly the net defaults.
		via_w = via_hole_w = 0;
		if (preSeg)
			m_net->CalcViaWidths(preSeg->m_width, &via_w, &via_hole_w);
		if (postSeg) 
		{
			int vw2, vhw2;
			m_net->CalcViaWidths(postSeg->m_width, &vw2, &vhw2);
			if (vw2 > via_w) 
				via_w = vw2, via_hole_w = vhw2;
		}
		if (via_w<=0 || via_hole_w<=0)
			// Use defaults...  CPT2 TODO:  net gets its default widths from netlist when constructed if no other val is specified.
			via_w = m_net->def_via_w,
			via_hole_w = m_net->def_via_hole_w;
		// End CPT
	}
	else
		// via not needed
		via_w = via_hole_w = 0;

	if (tee)
		tee->ReconcileVia();
}

int cvertex2::GetViaConnectionStatus( int layer )
{
	// Derived from old CNetList function.
	int status = VIA_NO_CONNECT;

	// check for end vertices of traces to pads.  CPT2 TODO:  I think I got the intent of the old routine, but need to check.
	if (!preSeg && pin) return status;
	if (!postSeg && pin) return status;

	// check for normal via pad (?)
	if( via_w == 0 && !tee ) return status;

	// check for via pad at end of branch
	if( tee && !postSeg && preSeg->m_layer == layer )
		if( !IsViaNeeded() )
			return status;
	// CPT2 do the same at beginning of connect, given the new freedom with endpoints
	if (tee && !preSeg && postSeg->m_layer == layer)
		if (!IsViaNeeded())
			return status;

	// check for trace connection to via pad
	if( preSeg && preSeg->m_layer == layer )
		status |= VIA_TRACE;
	if( postSeg && postSeg->m_layer == layer )
		status |= VIA_TRACE;

	// see if it connects to any area in this net on this layer
	citer<carea2> ia (&m_net->areas);
	for( carea2 *a = ia.First(); a; a = ia.Next() )
		if( a->Layer() == layer )
		{
			// area is on this layer, loop through via connections to area
			citer<cvertex2> iv (&a->vtxs);
			for( cvertex2 *v = iv.First(); v; v = iv.Next() )
				if( v == this )
					// via connects to area
					status |= VIA_AREA;
		}
	return status;
}

int cvertex2::Draw()
{
	cnetlist * nl = m_net->m_nlist;
	CDisplayList *dl = doc->m_dlist;
	Undraw();							// undraw previous via and selection box 
	if (tee)
		return NOERR;					// CPT2.  Rather than draw the individual vertices, just draw the tee itself.
	if (pin)
		return NOERR;					// CPT2.  Similarly, draw the pin and its selector, not the vtx.

	// draw via if via_w > 0
	if( via_w )
	{
		int n_layers = nl->GetNumCopperLayers();
		dl_els.SetSize( n_layers );
		for( int il=0; il<n_layers; il++ )
		{
			int layer = LAY_TOP_COPPER + il;
			dl_els[il] = dl->AddMain( this, layer, DL_CIRC, 1, via_w, 0, 0,
									x, y, 0, 0, 0, 0 );
		}
		dl_hole = dl->AddMain( this, LAY_PAD_THRU, DL_HOLE, 1, via_hole_w, 0, 0,
							x, y, 0, 0, 0, 0 );
	}

	// draw selection box for vertex, using LAY_THRU_PAD if via, or layer of adjacent
	// segments if no via
	int sel_layer;
	if( via_w )
		sel_layer = LAY_SELECTION;
	else if( preSeg )
		sel_layer = preSeg->m_layer;
	else
		sel_layer = postSeg->m_layer;
	dl_sel = dl->AddSelector( this, sel_layer, DL_HOLLOW_RECT, 
		1, 0, 0, x-10*PCBU_PER_MIL, y-10*PCBU_PER_MIL, x+10*PCBU_PER_MIL, y+10*PCBU_PER_MIL, 0, 0 );
	bDrawn = true;
	return NOERR;
}

void cvertex2::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;
	for( int i=0; i<dl_els.GetSize(); i++ )
		//	m_con->m_net->m_dlist->Remove( dl_el[i] );				// CPT2.  Was this way.  Not sure why...
		dl->Remove( dl_els[i] );
	dl_els.RemoveAll();
	dl->Remove( dl_sel );
	dl->Remove( dl_hole );
	dl_sel = NULL;
	dl_hole = NULL;
}

void cvertex2::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;
	if (tee)
		// Since vertices within a tee don't have selectors, it's unlikely that Highlight() will be called on them.
		// But just in case...
		{ tee->Highlight(); return; }

	// highlite square width is adjacent seg-width*2, via_width or 20 mils
	// whichever is greatest
	int w = 0;
	if (preSeg)
		w = 2 * preSeg->m_width;
	if (postSeg)
		w = max(w, 2 * postSeg->m_width);
	w = max( w, via_w );
	w = max( w, 20*PCBU_PER_MIL );
	dl->Highlight( DL_HOLLOW_RECT, x - w/2, y - w/2, x + w/2, y + w/2, 0 );
}



ctee::ctee(cnet2 *n)
	: cpcb_item (n->doc)
{ 
	via_w = via_hole_w = 0; 
	doc->m_nlist->tees.Add(this);
	dl_hole = NULL;
}

void ctee::Remove()
{
	// Disconnect this from everything:  that is, remove references to this from all vertices in vtxs, and then clear out this->vtxs.  The
	// garbage collector will later delete this.
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->tee = NULL;
	vtxs.RemoveAll();
	Undraw();
}

bool ctee::Adjust()
{
	// Typically called after a segment/connect attached to a tee-vertex is removed.  At that point we see if there are still 3+ vertices 
	// remaining within the tee;  if so, do nothing and return false.  If there's 0-1 vertex in the tee (not that likely, I think), just 
	// remove the tee from the network and return true. If there are 2 attached connects, combine them before removing the tee & returning true.
	int nItems = vtxs.GetSize();
	if (nItems > 2) return false;
	if (nItems < 2) 
		{ Remove(); return true; }
	citer<cvertex2> iv (&vtxs);
	cvertex2 *v0 = iv.First(), *v1 = iv.Next();
	cconnect2 *c0 = v0->m_con, *c1 = v1->m_con;
	if (c0==c1) return false;					// Remaining connect is a circle!  A tee is still required in that case. 
	c0->CombineWith(c1, v0, v1);
	Remove();
	return true;
}

void ctee::ReconcileVia()
{
	// Set via_w and via_hole_w, based on the max of the widths for the constituent vtxs.
	citer<cvertex2> iv (&vtxs);
	via_w = via_hole_w = 0;
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		if (v->via_w > via_w)
			via_w = v->via_w, via_hole_w = v->via_hole_w;
}

void ctee::Remove(cvertex2 *v) 
{
	vtxs.Remove(v);
	Adjust();
	ReconcileVia();
}

int ctee::Draw()
{
	cvertex2 *v = vtxs.First();
	int x = v->x, y = v->y;
	if (!v) return NOERR;				// Weird...
	cnetlist * nl = v->m_net->m_nlist;
	CDisplayList *dl = doc->m_dlist;
	Undraw();							// undraw previous via and selection box 
	// draw via if via_w > 0
	if( via_w )
	{
		int n_layers = nl->GetNumCopperLayers();
		dl_els.SetSize( n_layers );
		for( int il=0; il<n_layers; il++ )
		{
			int layer = LAY_TOP_COPPER + il;
			dl_els[il] = dl->AddMain( this, layer, DL_CIRC, 1, via_w, 0, 0,
								      x, y, 0, 0, 0, 0 );
		}
		dl_hole = dl->AddMain( this, LAY_PAD_THRU, DL_HOLE, 1, via_hole_w, 0, 0,
							   x, y, 0, 0, 0, 0 );
	}

	// draw selection box for vertex, using LAY_THRU_PAD if via, or layer of adjacent
	// segments if no via
	int sel_layer;
	if( via_w )
		sel_layer = LAY_SELECTION;
	else if( v->preSeg )
		sel_layer = v->preSeg->m_layer;
	else
		sel_layer = v->postSeg->m_layer;
	dl_sel = dl->AddSelector( this, sel_layer, DL_HOLLOW_RECT, 
		1, 0, 0, x-10*PCBU_PER_MIL, y-10*PCBU_PER_MIL, x+10*PCBU_PER_MIL, y+10*PCBU_PER_MIL, 0, 0 );
	bDrawn = true;
	return NOERR;
}

void ctee::Undraw()
{
	// CPT2.  Practically identical to cvertex2::Undraw()
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;
	for( int i=0; i<dl_els.GetSize(); i++ )
		dl->Remove( dl_els[i] );
	dl_els.RemoveAll();
	dl->Remove( dl_sel );
	dl->Remove( dl_hole );
	dl_sel = NULL;
	dl_hole = NULL;
}

void ctee::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;

	// highlite square width is adjacent seg-width*2, via_width or 20 mils
	// whichever is greatest
	int w = 0;
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
	{
		if (v->preSeg)
			w = max(w, 2 * v->preSeg->m_width);
		if (v->postSeg)
			w = max(w, 2 * v->postSeg->m_width);
	}
	w = max( w, via_w );
	w = max( w, 20*PCBU_PER_MIL );
	int x = vtxs.First()->x, y = vtxs.First()->y;
	dl->Highlight( DL_HOLLOW_RECT, x - w/2, y - w/2, x + w/2, y + w/2, 0 );
}



cseg2::cseg2(cconnect2 *c, int _layer, int _width)							// CPT2 added args.  Replaces Initialize()
	: cpcb_item( c->doc )
{
	m_con = c;
	m_con->segs.Add(this);
	m_net = c->m_net;
	m_layer = _layer;
	m_width = _width;
	m_curve = m_selected = 0;
	preVtx = postVtx = NULL;
}

void cseg2::SetWidth( int w, int via_w, int via_hole_w )
{
	if( m_layer != LAY_RAT_LINE && w )
		m_width = w,
		Draw();								// CPT2 are we sure we want this?
	if( preVtx->via_w && via_w )
		preVtx->via_w = via_w,
		preVtx->via_hole_w = via_hole_w,
		preVtx->Draw();						// CPT2 ditto
	if( postVtx->via_w && via_w )
		postVtx->via_w = via_w,
		postVtx->via_hole_w = via_hole_w,
		postVtx->Draw();					// CPT2 ditto
}

void cseg2::Divide( cvertex2 *v, cseg2 *s, int dir )
{
	// Divide "this" segment into two subsegments, with v the vertex in the middle.  If dir==0,
	// new segment "s" becomes the first subseg, stretching from this->preVtx to v, and "this" 
	// becomes the second subseg, stretching from v to this->postVtx.  If dir==1, s becomes the
	// second subseg and "this" becomes the first.
	// Assumes v and s have been created with m_con members equal to this->m_con.
	m_con->segs.Add(s);
	m_con->vtxs.Add(v);
	if (dir==0)
		s->preVtx = preVtx,	s->postVtx = v,
		preVtx->postSeg = s,
		v->preSeg = s, v->postSeg = this,
		preVtx = v;
	else
		s->preVtx = v, s->postVtx = postVtx,
		postVtx->preSeg = s,
		v->preSeg = this, v->postSeg = s,
		postVtx = v;
}

int cseg2::Route( int layer, int width )
{
	// Old CNetList::RouteSegment()
	// Convert segment from unrouted to routed
	// returns 1 if segment can't be routed on given layer due to connection to SMT pad.
	// Adds/removes vias as necessary
	// First check layer settings of adjacent vertices to make sure this is legal:
	if (preVtx->pin)
	{
		int pad_layer = preVtx->pin->pad_layer;
		if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
			return 1;
	}
	if (postVtx->pin)
	{
		int pad_layer = postVtx->pin->pad_layer;
		if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
			return 1;
	}

	// modify segment parameters
	Undraw();
	m_layer = layer;
	m_width = width;
	m_selected = 0;
	Draw();

	// now adjust vias
	preVtx->ReconcileVia();
	postVtx->ReconcileVia();
	return 0;
}

void cseg2::Unroute(int dx, int dy, int end)
{
	// Old CNetList::UnrouteSegment().  Ditched the return value, which was confusing and unused.
	bool bUnrouted = UnrouteWithoutMerge( dx, dy, end );
	if (bUnrouted)
		m_con->MergeUnroutedSegments();
}

bool cseg2::UnrouteWithoutMerge(int dx, int dy, int end) 
{
	// Old CNetList::UnrouteSegmentWithoutMerge().
	// Unroute segment, but don't merge with adjacent unrouted segments
	// ?? Assume that there will be an eventual call to MergeUnroutedSegments() to set vias
	// dx, dy and end arguments are 1/1/0 by default.  If, say, dx==0, dy==100000, end==1, then caller is about
	// to move a group/part to which the segment's end #1 is attached (by distance (0,100000)).  If the segment is itself vertical
	// (or nearly so) then we don't really need to unroute, and we return false.
	bool bUnroute = true;
	int xi = preVtx->x, yi = preVtx->y;
	int xf = postVtx->x, yf = postVtx->y;
	if (dx==0 && abs(xi-xf) < abs(dy/10))
	{
		// Segment is almost vertical, and one of its endpoints is going to move vertically
		// If end 0 is going to move, we will unroute only if it's getting moved to the opposite side of end 1.  Analogously if end 1 is moving.
		if (end==0) 
			bUnroute = (yf-yi)*(yf-(yi+dy)) <= 0;
		else        
			bUnroute = (yf-yi)*((yf+dy)-yi) <= 0;
	}
	if (dy==0 && abs(yi-yf) < abs(dx/10)) 
	{
		if (end==0) 
			bUnroute = (xf-xi)*(xf-(xi+dx)) <= 0;
		else        
			bUnroute = (xf-xi)*((xf+dx)-xi) <= 0;
	}
	if (!bUnroute) 
		return false;

	m_layer = LAY_RAT_LINE;
	m_width = 0;

	// redraw segment.  TODO should we do the Undraw() + bWasDrawn business?
	if( IsDrawn() )
		Draw();
	preVtx->ReconcileVia();
	preVtx->ReconcileVia();
	return true;
}

bool cseg2::RemoveMerge(int end)
{
	// Replaces old CNetList::RemoveSegment(), cnet::RemoveSegAndVertexByIndex(), RemoveVertexAndSegByIndex().
	// If this is a segment in the middle of a connect, combine it with its neighbor:  "end" indicates which vertex is
	//   going to get eliminated in the process (0 => remove preVtx, 1 => remove postVtx).
	// If this is an end-segment and "end" indicates the terminval vertex, then we just shorten the connect;  in other words, we call this->RemoveBreak().
	// Routine returns true if the entire connection has been destroyed because connect has just 1 segment.
	int nSegsInConnect = m_con->segs.GetSize();
	if (nSegsInConnect<2) 
		{ m_con->Remove(); return true; }

	if (!preVtx->preSeg && end==0 || !postVtx->postSeg && end==1)
		// Seg is at end of trace!
		return RemoveBreak();
	m_con->segs.Remove(this);
	Undraw();
	if (end==0) 
		m_con->vtxs.Remove(preVtx),
		preVtx->preSeg->postVtx = postVtx,
		postVtx->preSeg = preVtx->preSeg;
	else
		m_con->vtxs.Remove(postVtx),
		postVtx->postSeg->preVtx = preVtx,
		preVtx->postSeg = postVtx->postSeg;
	return false;
}

bool cseg2::RemoveBreak()
{
	// Replaces old cnet::RemoveSegmentAdjustTees().  Remove segment from its connect, and if it's in the middle somewhere then we'll have
	// to split the old connect into 2.  If it's at an end point, just remove the segment, plus check for tees at the end point and adjust as needed.
	int nSegsInConnect = m_con->segs.GetSize();
	if (nSegsInConnect<2) 
		{ m_con->Remove(); return true; }
	
	m_con->segs.Remove(this);
	Undraw();
	if (!preVtx->preSeg)
	{
		// this is an initial segment
		m_con->vtxs.Remove(preVtx);
		m_con->head = postVtx;
		postVtx->preSeg = NULL;
		if (preVtx->tee)
			preVtx->tee->Remove(preVtx);
		postVtx->ReconcileVia();
		// If new initial seg is a ratline, remove it too.
		if (postVtx->postSeg->m_layer == LAY_RAT_LINE)
			return postVtx->postSeg->RemoveBreak();
		return false;
	}
	if (!postVtx->postSeg)
	{
		// this is a final segment
		m_con->vtxs.Remove(postVtx);
		m_con->tail = preVtx;
		preVtx->postSeg = NULL;
		if (postVtx->tee)
			postVtx->tee->Remove(postVtx);
		preVtx->ReconcileVia();
		// If new final seg is a ratline, remove it too
		if (preVtx->preSeg->m_layer == LAY_RAT_LINE)
			return preVtx->preSeg->RemoveBreak();
		return false;
	}
	
	// Remove middle segment.  Create new connect for the latter half of the existing connect.
	cconnect2 *newCon = new cconnect2(m_net);
	for (cvertex2 *v = postVtx; 1; v = v->postSeg->postVtx)
	{
		m_con->vtxs.Remove(v);
		newCon->vtxs.Add(v);
		if (!v->postSeg) break;
		m_con->segs.Remove(v->postSeg);
		newCon->segs.Add(v->postSeg);
	}
	newCon->head = postVtx;
	newCon->tail = m_con->tail;
	m_con->tail = preVtx;

	ctee *headT = m_con->head->tee;
	if (headT && newCon->tail->tee == headT)
		// It's just possible that the original connect was a circle.  Having removed "this", it's now two connects with a single tee uniting their 
		// endpoints.  By calling Adjust() on that tee we'll get the two connects united into a single line.
		headT->Adjust();
	return false;
}

int cseg2::Draw()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return NO_DLIST;
	if( dl_el )
		Undraw();
	int vis = m_layer == LAY_RAT_LINE? m_net->bVisible: 1;
	int shape = DL_LINE;
	if( m_curve == CW )
		shape = DL_CURVE_CW;
	else if( m_curve == CCW )
		shape = DL_CURVE_CCW;
	dl_el = dl->AddMain( this, m_layer, shape, vis, 
		m_width, 0, 0, preVtx->x, preVtx->y, postVtx->x, postVtx->y, 0, 0 );
	dl_sel = dl->AddSelector( this, m_layer, DL_LINE, vis, 
		m_width, 0, preVtx->x, preVtx->y, postVtx->x, postVtx->y, 0, 0 );
	// mark parent connection as at least partially drawn
	m_con->bDrawn = TRUE;
	bDrawn = true;
	return NOERR;
}

void cseg2::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;
	dl->Remove( dl_el );
	dl->Remove( dl_sel );
	dl_el = NULL;
	dl_sel = NULL;
}

void cseg2::Highlight( bool bThin )
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;
	int w = bThin? 1: dl->Get_w(dl_el);
	dl->Highlight( DL_LINE, dl->Get_x(dl_el), dl->Get_y(dl_el),
							dl->Get_xf(dl_el), dl->Get_yf(dl_el),
							w, dl_el->layer );
}


cconnect2::cconnect2( cnet2 * _net ) 
	: cpcb_item (_net->doc)
{
	m_net = _net;
	m_net->connects.Add(this);
	locked = 0;
}

void cconnect2::SetWidth( int w, int via_w, int via_hole_w )
{
	citer<cseg2> is (&segs);
	for (cseg2 *s= is.First(); s; s = is.Next())
		s->SetWidth(w, via_w, via_hole_w);
}

void cconnect2::Remove(bool bSetAreaConnections)
{
	// CPT2. User wants to delete connection, so detach it from the network (garbage collector will later delete this object and its
	// constituents for good).
	// Any tee structures attached at either end get cleaned up.  bSetAreaConnections is true by default.
	Undraw();
	m_net->connects.Remove(this);
	if (head->tee)
		head->tee->Remove(head);
	if (tail->tee)
		tail->tee->Remove(tail);
	if (bSetAreaConnections)
		m_net->SetAreaConnections();
}


void cconnect2::Start( cvertex2 *v )
{
	// CPT2 new.  After this routine, connect will consist of a single vertex (v) and no segs.
	vtxs.RemoveAll();
	segs.RemoveAll();
	vtxs.Add(v);
	head = tail = v;
	v->preSeg = v->postSeg = NULL;
}

void cconnect2::AppendSegAndVertex( cseg2 *s, cvertex2 *v, cvertex2 *after )
{
	// Append s+v into this connection after vertex "after".  Assumes that s+v were constructed so that they point to "this"
	vtxs.Add(v);
	segs.Add(s);
	cseg2 *nextSeg = after->postSeg;
	after->postSeg = s;
	s->preVtx = after;
	s->postVtx = v;
	v->preSeg = s;
	v->postSeg = nextSeg;
	if (nextSeg)
		nextSeg->preVtx = v;
	else 
		tail = v;
}

void cconnect2::PrependVertexAndSeg( cvertex2 *v, cseg2 *s )
{
	// Similar to AppendSegAndVertex, but used at the start of the connection.
	vtxs.Add(v);
	segs.Add(s);
	v->preSeg = NULL;
	v->postSeg = s;
	s->preVtx = v;
	s->postVtx = head;
	head->preSeg = s;
	head = v;
}

// Append new segment to connection 
void cconnect2::AppendSegment(int x, int y, int layer, int width )
{
	// add new vertex and segment
	BOOL bWasDrawn = IsDrawn();
	Undraw();
	cseg2 *s = new cseg2 (this, layer, width);
	cvertex2 *v = new cvertex2 (this, x, y);
	AppendSegAndVertex( s, v, tail );
	if( bWasDrawn )
		Draw();
}

void cconnect2::ReverseDirection()
{
	// CPT2.  I'm going to see if it works not to undraw and redraw this.
	for (cseg2 *s = head->postSeg, *prev = NULL, *next; s; prev = s, s = next)
	{
		next = s->postVtx->postSeg;
		cvertex2 *tmp = s->preVtx;
		s->preVtx = s->postVtx;
		s->postVtx = tmp;
		s->postVtx->preSeg = s;
		s->postVtx->postSeg = prev;
		s->preVtx->preSeg = next;
		s->preVtx->postSeg = s;
	}
	cvertex2 *tmp = head;
	head = tail;
	tail = tmp;
}

void cconnect2::CombineWith(cconnect2 *c2, cvertex2 *v1, cvertex2 *v2) 
{
	// Combine "this" with connect c2.  To do this, identify vertex v1 (from one end of "this") with v2 (from one end of c2), reversing
	// connects as necessary.  After removing v2 and reassigning c2's remaining segs and vertices to this, removes c2.
	if (head==v1)
		ReverseDirection();
	else 
		ASSERT(tail==v1);
	if (c2->tail==v2)
		c2->ReverseDirection();
	else 
		ASSERT(c2->head==v2);
	v1->postSeg = v2->postSeg;
	v1->postSeg->preVtx = v1;
	tail = c2->tail;
	c2->vtxs.Remove(v2);				// v2 is now garbage, ripe for the collector...
	citer<cseg2> is (&c2->segs);
	for (cseg2 *s = is.First(); s; s = is.Next())
		s->m_con = this;
	citer<cvertex2> iv (&c2->vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->m_con = this;
	vtxs.Add(&c2->vtxs);
	segs.Add(&c2->segs);
	c2->m_net->connects.Remove(c2);		// c2 is garbage...
}

void cconnect2::MergeUnroutedSegments() 
{
	if (segs.GetSize() < 2) return;
	bool bWasDrawn = IsDrawn();
	Undraw();
	cvertex2 *v = head->postSeg->postVtx, *next;
	for ( ; v->postSeg; v = next)
	{
		next = v->postSeg->postVtx;
		if (v->preSeg->m_layer == LAY_RAT_LINE && v->postSeg->m_layer == LAY_RAT_LINE
			&& !v->tee && !v->force_via_flag)
		{
			// v is between 2 unrouted segs, so remove v and v->postSeg.
			v->preSeg->postVtx = next;
			next->preSeg = v->preSeg;
			segs.Remove(v->postSeg);
			vtxs.Remove(v);
		}
	}
	if (bWasDrawn)
		Draw();
}

int cconnect2::Draw()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return NO_DLIST;

	Undraw();
	citer<cseg2> is (&segs);
	for (cseg2 *s = is.First(); s; s = is.Next())
		s->Draw();
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->Draw();												// CPT2. NB does NOT reconcile vias (assumes it's been done)
	bDrawn = TRUE;
	return NOERR;
}

void cconnect2::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl || IsDrawn() ) return;

	citer<cseg2> is (&segs);
	for (cseg2 *s = is.First(); s; s = is.Next())
		s->Undraw();
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->Undraw();
	bDrawn = false;
}


/**********************************************************************************************/
/*  RELATED TO cpin2/cpart2                                                                     */
/**********************************************************************************************/

cpin2::cpin2(cpart2 *_part, padstack *_ps, cnet2 *_net)					// CPT2. Added args
	: cpcb_item (_part->doc)
{
	part = _part;
	part->pins.Add(this);
	ps = _ps;
	if( ps->hole_size ) 
		pad_layer = LAY_PAD_THRU;
	else if( part->side == 0 && ps->top.shape != PAD_NONE || part->side == 1 && ps->bottom.shape != PAD_NONE )
		pad_layer = LAY_TOP_COPPER;
	else
		pad_layer = LAY_BOTTOM_COPPER;
	pin_name = ps->name;
	net = _net;
	if (net)
		net->pins.Add(this);
	dl_hole = NULL;
}

void cpin2::Disconnect(bool bSetAreas) 
{
	// Detach pin from whichever net it's attached to.  Rip out any attached connections completely.
	if (!net) return;
	citer<cconnect2> ic (&net->connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
		if (c->head->pin == this || c->tail->pin == this)
			c->Remove(false);
	net->pins.Remove(this);
	if (bSetAreas)
		net->SetAreaConnections();
	net = NULL;
}

int cpin2::GetWidth()													// CPT2. Derived from CPartList::GetPinWidth()
{
	ASSERT(part->shape!=NULL);
	int w = ps->top.size_h;
	w = max( w, ps->bottom.size_h );
	w = max( w, ps->hole_size );
	return w;
}

void cpin2::SetPosition()												
{
	// CPT2 New, but derived from CPartList::GetPinPoint().
	// Derives correct values for this->(x,y) based on the part's position and on the padstack position
	ASSERT( part->shape!=NULL );

	// get pin coords relative to part origin
	CPoint pp;
	pp.x = ps->x_rel;
	pp.y = ps->y_rel;
	// flip if part on bottom
	if( part->side )
	{
		pp.x = -pp.x;
	}
	// rotate if necess.
	int angle = part->angle;
	if( angle > 0 )
	{
		CPoint org (0,0);
		RotatePoint( &pp, angle, org );
	}
	// add coords of part origin
	x = part->x + pp.x;
	y = part->y + pp.y;
}

void cpin2::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	if( !dl_sel ) return;							// CPT2 Necessary check?  NB selector was created drawn by cpart2::Draw
	dl->Highlight( DL_RECT_X, 
				dl->Get_x(dl_sel), dl->Get_y(dl_sel),
				dl->Get_xf(dl_sel), dl->Get_yf(dl_sel), 
				1, pad_layer );
}



cpart2::cpart2( cpartlist * pl )			// CPT2 TODO.  Will probably add more args...
	: cpcb_item (pl->m_doc)
{ 
	m_pl = pl;
	pl->parts.Add(this);
	// zero out pointers
	dl_ref_sel = 0;
	shape = 0;

	x = y = side = angle = 0;
	glued = false;
	m_ref_vis = true;
	m_ref_xi = m_ref_yi = m_ref_angle = m_ref_w = m_ref_layer = 0;
	m_value_vis = false;
	m_value_xi = m_value_yi = m_value_angle = m_value_size = m_value_w = m_value_layer = 0;
	dl_ref_sel = dl_value_sel = NULL;
	shape = NULL;
}

void cpart2::Move( int _x, int _y, int _angle, int _side )
{
	bool bWasDrawn = IsDrawn();
	Undraw();
	// move part
	x = _x;
	y = _y;
	angle = _angle % 360;
	side = _side;
	// calculate new pin positions
	if( shape )
	{
		citer<cpin2> ip (&pins);
		for (cpin2* p = ip.First(); p; p = ip.Next())
			p->SetPosition();
	}
	if (shape && bWasDrawn)
		Draw();
}

void cpart2::Remove()						// CPT2. Derived from PartList::RemovePart().
{
	Undraw();
	m_pl->parts.Remove(this);
}

void cpart2::SetData(CShape * _shape, CString * _ref_des, CString * _package, 
					 int _x, int _y, int _side, int _angle, int _visible, int _glued, bool _ref_vis  )
{
	// Derived from old CPartList::SetPartData.  Initializes data members, including "pins".  Ref and value-text related
	// members are initialized according to _shape if possible.
	bool bWasDrawn = IsDrawn(); 
	Undraw();
	// CPT2 Delete the following?
	// CDisplayList * old_dlist = m_dlist;
	// m_dlist = NULL;		// cancel further drawing

	// now copy data into part
	visible = _visible;
	ref_des = *_ref_des;
	if( package )
		package = *_package;
	else
		package = "";
	x = _x;
	y = _y;
	side = _side;
	angle = _angle;
	glued = _glued;

	if( !_shape )
	{
		shape = NULL;
		m_ref_xi = 0;
		m_ref_yi = 0;
		m_ref_angle = 0;
		m_ref_size = 0;
		m_ref_w = 0;
		m_value_xi = 0;
		m_value_yi = 0;
		m_value_angle = 0;
		m_value_size = 0;
		m_value_w = 0;
	}
	else
	{
		shape = _shape;
		InitPins();
		Move( x, y, angle, side );	// force setting pin positions
		m_ref_xi = shape->m_ref_xi;
		m_ref_yi = shape->m_ref_yi;
		m_ref_angle = shape->m_ref_angle;
		m_ref_size = shape->m_ref_size;
		m_ref_w = shape->m_ref_w;
		m_ref_layer = shape->m_ref->m_layer;		
		m_value_xi = shape->m_value_xi;
		m_value_yi = shape->m_value_yi;
		m_value_angle = shape->m_value_angle;
		m_value_size = shape->m_value_size;
		m_value_w = shape->m_value_w;
		m_value_layer = shape->m_value->m_layer;		
	}

	m_outline_stroke.SetSize(0);
	ref_text_stroke.SetSize(0);
	value_stroke.SetSize(0);
	m_ref_vis = _ref_vis;					// CPT

	// m_dlist = old_dlist;
	if( shape && bWasDrawn )
		Draw();
}

void cpart2::SetValue(CString *_value, int x, int y, int angle, int size, int w, 
						 BOOL vis, int layer )
{
	bool bWasDrawn = IsDrawn();
	if (shape)
		Undraw();
	value = *_value;
	m_value_xi = x;
	m_value_yi = y;
	m_value_angle = angle;
	m_value_size = size;
	m_value_w = w;
	m_value_vis = vis;
	m_value_layer = layer;
	if( shape && bWasDrawn )
		Draw();
}

void cpart2::ResizeRefText(int size, int width, BOOL vis )
{
	bool bWasDrawn = IsDrawn();
	if (shape)
		Undraw();
	m_ref_size = size;
	m_ref_w = width;	
	m_ref_vis = vis;
	if (shape && bWasDrawn)
		Draw();
}

void cpart2::InitPins()
{
	// CPT2 New.  Initialize pins carray based on the padstacks in the shape.
	ASSERT(shape);
	int cPins = shape->m_padstack.GetCount();
	for (int i=0; i<cPins; i++)
	{
		padstack *ps = &shape->m_padstack[i];
		cpin2 *p = new cpin2(this, ps, NULL);
		pins.Add(p);
	}
}



extern int GenerateStrokesForPartString( CString * str, int layer, BOOL bMirror,					// CPT2 In old PartList.cpp
								  int size, int rel_angle, int rel_xi, int rel_yi, int w, 
								  int x, int y, int angle, int side,
								  CArray<stroke> * strokes, CRect * br,
								  CDisplayList * dlist, SMFontUtil * sm );

// get bounding rect of value text relative to part origin 
// works even if part isn't drawn
//
CRect cpart2::GetValueRect()
{
	CArray<stroke> m_stroke;
	CRect br;
	BOOL bMirror = (m_value_layer == LAY_SILK_BOTTOM || m_value_layer == LAY_BOTTOM_COPPER) ;
	int nstrokes = GenerateStrokesForPartString( &value, 0, bMirror, m_value_size,
		m_value_angle, m_value_xi, m_value_yi, m_value_w,
		x, y, angle, side,
		&m_stroke, &br, NULL, m_pl->m_dlist->GetSMFontUtil() );
	return br;
}

int cpart2::Draw()
{
	// CPT2 TODO:  Think about the CDisplayList arg business
	CDisplayList *dl = doc->m_dlist;
	if( !dl )
		return NO_DLIST;
	if( !shape )
		return NO_FOOTPRINT;
	if( IsDrawn() )
		Undraw( );				// ideally, should be undrawn when changes made, not now
								// CPT2 TODO think about this comment ^

	// draw selection rectangle (layer = top or bottom copper, depending on side)
	CRect sel;
	int sel_layer;
	if( !side )
	{
		// part on top
		sel.left = shape->m_sel_xi; 
		sel.right = shape->m_sel_xf;
		sel.bottom = shape->m_sel_yi;
		sel.top = shape->m_sel_yf;
		sel_layer = LAY_SELECTION;
	}
	else
	{
		// part on bottom
		sel.right = - shape->m_sel_xi;
		sel.left = - shape->m_sel_xf;
		sel.bottom = shape->m_sel_yi;
		sel.top = shape->m_sel_yf;
		sel_layer = LAY_SELECTION;
	}
	if( angle > 0 )
		RotateRect( &sel, angle, zero );
	dl_sel = dl->AddSelector( this, sel_layer, DL_HOLLOW_RECT, 1,
		0, 0, x + sel.left, y + sel.bottom, x + sel.right, y + sel.top, x, y );
	dl->Set_sel_vert( dl_sel, 0 );													// CPT2 TODO function should be in dl_element
	if( angle == 90 || angle ==  270 )
		dl->Set_sel_vert( dl_sel, 1 );

	CArray<stroke> strokes;	// used for text.  CPT2 renamed (m_ was confusing)
	CRect br;
	CPoint si, sf;

	// draw ref designator text
	dl_ref_sel = NULL;
	if( m_ref_vis && m_ref_size )
	{
		int layer = m_ref_layer;
		layer = FlipLayer( side, layer );
		BOOL bMirror = (layer == LAY_SILK_BOTTOM || layer == LAY_BOTTOM_COPPER) ;	// CPT2 changed m_ref_layer to layer.  Correct?
		int nstrokes = ::GenerateStrokesForPartString( &ref_des, 
			layer, bMirror, m_ref_size,
			m_ref_angle, m_ref_xi, m_ref_yi, m_ref_w,
			x, y, angle, side,
			&strokes, &br, dl, dl->GetSMFontUtil() );
		if( nstrokes )
		{
			int xmin = br.left;
			int xmax = br.right;
			int ymin = br.bottom;
			int ymax = br.top;
			ref_text_stroke.SetSize( nstrokes );
			for( int is=0; is<nstrokes; is++ )
			{
				strokes[is].dl_el = dl->Add( this, dl_element::DL_REF,  
					layer, DL_LINE, 1, strokes[is].w, 0, 0,
					strokes[is].xi, strokes[is].yi, 
					strokes[is].xf, strokes[is].yf, 0, 0 );
				ref_text_stroke[is] = strokes[is];
			}
			// draw selection rectangle for ref text
			dl_ref_sel = dl->AddSelector( this, layer, DL_HOLLOW_RECT, 1,		// CPT2 changed m_ref_layer to layer.  Correct?
				0, 0, xmin, ymin, xmax, ymax, xmin, ymin );
		}
	}
	else
	{
		for( int is=0; is<ref_text_stroke.GetSize(); is++ )
			dl->Remove( ref_text_stroke[is].dl_el );
		ref_text_stroke.SetSize(0);
	}

	// draw value text
	dl_value_sel = NULL;
	if( m_value_vis && m_value_size )
	{
		int layer = m_value_layer;
		layer = FlipLayer( side, layer );
		BOOL bMirror = (layer == LAY_SILK_BOTTOM || layer == LAY_BOTTOM_COPPER) ;
		int nstrokes = ::GenerateStrokesForPartString( &value, 
			layer, bMirror, m_value_size,
			m_value_angle, m_value_xi, m_value_yi, m_value_w,
			x, y, angle, side,
			&strokes, &br, dl, dl->GetSMFontUtil() );

		if( nstrokes )
		{
			int xmin = br.left;
			int xmax = br.right;
			int ymin = br.bottom;
			int ymax = br.top;
			value_stroke.SetSize( nstrokes );
			for( int is=0; is<nstrokes; is++ )
			{
				strokes[is].dl_el = dl->Add( this, dl_element::DL_VALUE,
					strokes[is].layer, DL_LINE, 1, strokes[is].w, 0, 0,
					strokes[is].xi, strokes[is].yi, 
					strokes[is].xf, strokes[is].yf, 0, 0 );
				value_stroke[is] = strokes[is];
			}

			// draw selection rectangle for value
			dl_value_sel = dl->AddSelector( this, dl_element::DL_VALUE_SEL, m_value_layer, DL_HOLLOW_RECT, 1,
				0, 0, xmin, ymin, xmax, ymax, xmin, ymin );
		}
	}
	else
	{
		for( int is=0; is<value_stroke.GetSize(); is++ )
			dl->Remove( value_stroke[is].dl_el );
		value_stroke.SetSize(0);
	}

	// draw part outline (code similar to but sadly not identical to cpolyline::Draw())
	m_outline_stroke.SetSize(0);
	citer<cpolyline> ip (&shape->m_outline_poly);
	for (cpolyline *poly = ip.First(); poly; poly = ip.Next())
	{
		int shape_layer = poly->Layer();
		int poly_layer = cpartlist::FootprintLayer2Layer( shape_layer );
		poly_layer = FlipLayer( side, poly_layer );
		int w = poly->W();
		citer<ccontour> ictr (&poly->contours);
		for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		{
			// Draw polyline sides
			citer<cside> is (&ctr->sides);
			for (cside *s = is.First(); s; s = is.Next())
			{
				si.x = s->preCorner->x, si.y = s->preCorner->y;
				sf.x = s->postCorner->x, sf.y = s->postCorner->y;
				if( si.x == sf.x || si.y == sf.y )
					// if endpoints not angled, make side STRAIGHT
					s->m_style = cpolyline::STRAIGHT;
				int g_type = DL_LINE;
				if( s->m_style == cpolyline::STRAIGHT )
					g_type = DL_LINE;
				else if( s->m_style == cpolyline::ARC_CW )
					g_type = DL_ARC_CW;
				else if( s->m_style == cpolyline::ARC_CCW )
					g_type = DL_ARC_CCW;
				// flip if part on bottom
				if( side )
				{
					si.x = -si.x;
					sf.x = -sf.x;
					if( g_type == DL_ARC_CW )
						g_type = DL_ARC_CCW;
					else if( g_type == DL_ARC_CCW )
						g_type = DL_ARC_CW;
				}
				// rotate with part and draw
				RotatePoint( &si, angle, zero );
				RotatePoint( &sf, angle, zero );
				stroke str;
				str.layer = poly_layer;
				str.xi = x+si.x;
				str.xf = x+sf.x;
				str.yi = y+si.y;
				str.yf = y+sf.y;
				str.type = g_type;
				str.w = w;
				str.dl_el = dl->AddMain( this, poly_layer,					// CPT2 TODO Is AddMain() appropriate?
					g_type, 1, w, 0, 0, x+si.x, y+si.y, x+sf.x, y+sf.y, 0, 0 );
				m_outline_stroke.Add(str);
			}
		}
	}

#if 0																							// CPT2 TODO coming soon...
	// draw text
	for( int it=0; it<shape->m_tl->text_ptr.GetSize(); it++ )
	{
		CText * t = part->shape->m_tl->text_ptr[it];
		int nstrokes = 0;
		CArray<stroke> m_stroke;
		m_stroke.SetSize( 1000 );

		double x_scale = (double)t->m_font_size/22.0;
		double y_scale = (double)t->m_font_size/22.0;
		double y_offset = 9.0*y_scale;
		i = 0;
		double xc = 0.0;
		CPoint si, sf;
		int w = t->m_stroke_width;
		int xmin = INT_MAX;
		int xmax = INT_MIN;
		int ymin = INT_MAX;
		int ymax = INT_MIN;

		int text_layer = FootprintLayer2Layer( t->m_layer );
		text_layer = FlipLayer( part->side, text_layer );
		nstrokes = ::GenerateStrokesForPartString( &t->m_str, 
			text_layer, t->m_mirror, t->m_font_size,
			t->m_angle, t->m_x, t->m_y, t->m_stroke_width,
			part->x, part->y, part->angle, part->side,
			&m_stroke, &br, m_dlist, m_dlist->GetSMFontUtil() );

		xmin = min( xmin, br.left );
		xmax = max( xmax, br.right );
		ymin = min( ymin, br.bottom );
		ymax = max( ymax, br.top );
		id.SetT1( ID_PART );
		id.SetT2( ID_FP_TXT );
		id.SetI2( it );
		id.SetT3( ID_STROKE );
		for( int is=0; is<nstrokes; is++ )
		{
			id.SetI3( is );
			m_stroke[is].dl_el = m_dlist->Add( id, this, 
				text_layer, DL_LINE, 1, m_stroke[is].w, 0, 0,
				m_stroke[is].xi, m_stroke[is].yi, 
				m_stroke[is].xf, m_stroke[is].yf, 0, 0 );
			part->m_outline_stroke.Add( m_stroke[is] );
		}
	}
#endif

	// draw padstacks and save absolute position of pins
	CPoint pin_pt;
	CPoint pad_pi;
	CPoint pad_pf;
	int nLayers = doc->m_plist->m_layers;
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next()) 
	{
		// set layer for pads
		padstack * ps = pin->ps;
		pin->dl_els.SetSize(nLayers);
		pad * p;
		int pad_layer;
		// iterate through all copper layers 
		pad * any_pad = NULL;
		for( int il=0; il<nLayers; il++ )
		{
			pin_pt.x = ps->x_rel;
			pin_pt.y = ps->y_rel;
			pad_layer = il + LAY_TOP_COPPER;
			pin->dl_els[il] = NULL;
			// get appropriate pad
			pad * p = NULL;
			if( pad_layer == LAY_TOP_COPPER && side == 0 )
				p = &ps->top;
			else if( pad_layer == LAY_TOP_COPPER && side == 1 )
				p = &ps->bottom;
			else if( pad_layer == LAY_BOTTOM_COPPER && side == 0 )
				p = &ps->bottom;
			else if( pad_layer == LAY_BOTTOM_COPPER && side == 1 )
				p = &ps->top;
			else if( ps->hole_size )
				p = &ps->inner;
			int sel_layer = pad_layer;
			if( ps->hole_size )
				sel_layer = LAY_SELECTION;
			if( p )
			{
				if( p->shape != PAD_NONE )
					any_pad = p;

				// draw pad
				dl_element * pad_el = NULL;
				if( p->shape == PAD_NONE )
					;
				else if( p->shape == PAD_ROUND )
				{
					// flip if part on bottom
					if( side )
						pin_pt.x = -pin_pt.x;
					// rotate
					if( angle > 0 )
						RotatePoint( &pin_pt, angle, zero );
					// add to display list
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = dl->AddMain( pin, pad_layer, 
						DL_CIRC, 1, 
						p->size_h, 0, 0,
						x + pin_pt.x, y + pin_pt.y, 0, 0, 0, 0 );
					if( !pin->dl_sel )
						pin->dl_sel = dl->AddSelector( pin, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							pin->x-p->size_h/2,	pin->y-p->size_h/2, 
							pin->x+p->size_h/2,	pin->y+p->size_h/2, 0, 0 );
				}

				else if( p->shape == PAD_SQUARE )
				{
					// flip if part on bottom
					if( side )
						pin_pt.x = -pin_pt.x;
					// rotate
					if( angle > 0 )
						RotatePoint( &pin_pt, angle, zero );
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = dl->AddMain( pin, pad_layer, 
						DL_SQUARE, 1, 
						p->size_h,
						0, 0,
						pin->x, pin->y, 0, 0, 0, 0 );
					if( !pin->dl_sel )
						pin->dl_sel = dl->AddSelector( pin, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							pin->x-p->size_h/2,	pin->y-p->size_h/2, 
							pin->x+p->size_h/2,	pin->y+p->size_h/2, 0, 0 );
				}

				else if( p->shape == PAD_RECT 
					|| p->shape == PAD_RRECT 
					|| p->shape == PAD_OVAL )
				{
					int gtype;
					if( p->shape == PAD_RECT )
						gtype = DL_RECT;
					else if( p->shape == PAD_RRECT )
						gtype = DL_RRECT;
					else
						gtype = DL_OVAL;
					pad_pi.x = pin_pt.x - p->size_l;
					pad_pi.y = pin_pt.y - p->size_h/2;
					pad_pf.x = pin_pt.x + p->size_r;
					pad_pf.y = pin_pt.y + p->size_h/2;
					// rotate pad about pin if necessary
					if( ps->angle > 0 )
					{
						RotatePoint( &pad_pi, ps->angle, pin_pt );
						RotatePoint( &pad_pf, ps->angle, pin_pt );
					}
					// flip if part on bottom
					if( side )
					{
						pin_pt.x = -pin_pt.x;
						pad_pi.x = -pad_pi.x;
						pad_pf.x = -pad_pf.x;
					}
					// rotate part about 
					if( angle > 0 )
					{
						RotatePoint( &pin_pt, angle, zero );
						RotatePoint( &pad_pi, angle, zero );
						RotatePoint( &pad_pf, angle, zero );
					}
					int radius = p->radius;
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = dl->AddMain( pin, pad_layer, 
						gtype, 1, 
						0, 0, 0, 
						x + pad_pi.x, y + pad_pi.y, 
						x + pad_pf.x, y + pad_pf.y, 
						x + pin_pt.x, y + pin_pt.y, 
						p->radius );
					if( !pin->dl_sel )
						pin->dl_sel = dl->AddSelector( pin, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							x + pad_pi.x, y + pad_pi.y, 
							x + pad_pf.x, y + pad_pf.y,
							0, 0 );
				}

				else if( p->shape == PAD_OCTAGON )
				{
					// flip if part on bottom
					if( side )
						pin_pt.x = -pin_pt.x;
					// rotate
					if( angle > 0 )
						RotatePoint( &pin_pt, angle, zero );
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = dl->AddMain( pin, pad_layer, 
						DL_OCTAGON, 1, 
						p->size_h, 0, 0,
						pin->x, pin->y, 0, 0, 0, 0 );
					if( !pin->dl_sel )
						pin->dl_sel = dl->AddSelector( pin, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							pin->x-p->size_h/2, pin->y-p->size_h/2, 
							pin->x+p->size_h/2, pin->y+p->size_h/2, 0, 0 );
				}

				pin->dl_els[il] = pad_el;
				pin->dl_hole = pad_el;
			}
		}
		
		// if through-hole pad, just draw hole and set pin->dl_hole
		if( ps->hole_size )
		{
			pin_pt.x = ps->x_rel;
			pin_pt.y = ps->y_rel;
			// flip if part on bottom
			if( side )
				pin_pt.x = -pin_pt.x;
			// rotate
			if( angle > 0 )
				RotatePoint( &pin_pt, angle, zero );
			// add to display list
			pin->x = x + pin_pt.x;
			pin->y = y + pin_pt.y;
			pin->dl_hole = dl->AddMain( pin, LAY_PAD_THRU, 
								DL_HOLE, 1,	
								ps->hole_size, 0, 0,
								pin->x, pin->y, 0, 0, 0, 0 );  
			if( !pin->dl_sel )
				// make selector for pin with hole only
				pin->dl_sel = dl->AddSelector( pin, sel_layer, 
					DL_HOLLOW_RECT, 1, 1, 0,
					pin->x-ps->hole_size/2, pin->y-ps->hole_size/2,  
					pin->x+ps->hole_size/2, pin->y+ps->hole_size/2,  
					0, 0 );
		}
		else
			pin->dl_hole = NULL;
	}

	bDrawn = true;
	return NOERR;
}

void cpart2::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl || IsDrawn() ) return;
	if (!shape)
		{ bDrawn = false; return; }

	// undraw selection rectangle
	dl->Remove( dl_sel );
	dl_sel = 0;

	// undraw selection rectangle for ref text
	dl->Remove( dl_ref_sel );
	dl_ref_sel = 0;

	// undraw ref designator text
	int nstrokes = ref_text_stroke.GetSize();
	for( int i=0; i<nstrokes; i++ )
	{
		dl->Remove( ref_text_stroke[i].dl_el );
		ref_text_stroke[i].dl_el = 0;
	}

	// undraw selection rectangle for value
	dl->Remove( dl_value_sel );
	dl_value_sel = 0;

	// undraw  value text
	nstrokes = value_stroke.GetSize();
	for( int i=0; i<nstrokes; i++ )
	{
		dl->Remove( value_stroke[i].dl_el );
		value_stroke[i].dl_el = 0;
	}

	// undraw part outline (this also includes footprint free text)
	for( int i=0; i<m_outline_stroke.GetSize(); i++ )
	{
		dl->Remove( m_outline_stroke[i].dl_el );
		m_outline_stroke[i].dl_el = 0;
	}

	// undraw padstacks
	citer<cpin2> ip (&pins);
	for (cpin2 *pin = ip.First(); pin; pin = ip.Next())
	{
		if( pin->dl_els.GetSize()>0 )
		{
			for( int il=0; il<pin->dl_els.GetSize(); il++ )
				if( pin->dl_els[il] != pin->dl_hole )
					dl->Remove( pin->dl_els[il] );
			pin->dl_els.RemoveAll();
		}
		dl->Remove( pin->dl_hole );
		dl->Remove( pin->dl_sel );
		pin->dl_hole = NULL;
		pin->dl_sel = NULL;
	}

	bDrawn = FALSE;
}

void cpart2::Highlight( )
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	if (!shape) return;
	dl->Highlight( DL_HOLLOW_RECT, 
				dl->Get_x( dl_sel ), dl->Get_y( dl_sel ),
				dl->Get_xf( dl_sel ), dl->Get_yf( dl_sel ), 1 );
	// CPT2 TODO:  Deal with the reftext and valuetext issue
}


/**********************************************************************************************/
/*  RELATED TO cpolyline/carea2/csmcutout                                                      */
/**********************************************************************************************/

ccorner::ccorner(ccontour *_contour, int _x, int _y)
	: cpcb_item (_contour->doc)
{
	contour = _contour;
	if (contour->corners.IsEmpty())
		contour->head = contour->tail = this;
	contour->corners.Add(this);
	x = _x; y = _y; 
	preSide = postSide = NULL;
}

bool ccorner::IsAreaCorner() { return contour->poly->IsArea(); }
bool ccorner::IsBoardCorner() { return contour->poly->IsBoard(); }
bool ccorner::IsSmCorner() { return contour->poly->IsSmCutout(); }
bool ccorner::IsOutlineCorner() { return contour->poly->IsOutline(); }

int ccorner::GetTypeBit() 
{														// Later:  put in .cpp file.  There wouldn't be this nonsense in Java...
	if (contour->poly->IsArea()) return bitAreaCorner;
	if (contour->poly->IsSmCutout()) return bitSmCorner;
	if (contour->poly->IsBoard()) return bitBoardCorner;
	return bitOther;
}

cside::cside(ccontour *_contour, int _style)
	: cpcb_item(_contour->doc)
{ 
	contour = _contour;
	contour->sides.Add(this);
	m_style = _style;
	preCorner = postCorner = NULL;
}

bool cside::IsAreaSide() { return contour->poly->IsArea(); }
bool cside::IsBoardSide() { return contour->poly->IsBoard(); }
bool cside::IsSmSide() { return contour->poly->IsSmCutout(); }
bool cside::IsOutlineSide() { return contour->poly->IsOutline(); }

int cside::GetTypeBit() 
{
	if (contour->poly->IsArea()) return bitAreaCorner;
	if (contour->poly->IsSmCutout()) return bitSmCorner;
	if (contour->poly->IsBoard()) return bitBoardCorner;
	return bitOther;
}

bool cside::IsOnCutout()
	// Return true if this side lies on a secondary contour within the polyline
	{ return contour->poly->main!=contour; }


ccontour::ccontour(cpolyline *_poly, bool bMain)
	: cpcb_item (_poly->doc)
{
	poly = _poly;
	if (bMain) 
		poly->main = this;
	poly->contours.Add(this);
}

void ccontour::AppendSideAndCorner( cside *s, ccorner *c, ccorner *after )
{
	// Append s+c into this connection after corner "after".  Assumes that s+c were constructed so that they point to "this"
	// Very similar to cconnect2::AppendSegAndVertex.
	corners.Add(c);
	sides.Add(s);
	cside *nextSide = after->postSide;
	after->postSide = s;
	s->preCorner = after;
	s->postCorner = c;
	c->preSide = s;
	c->postSide = nextSide;
	if (nextSide)
		nextSide->preCorner = c;
	else 
		tail = c;
}

void ccontour::Close(int style)
{
	if (head==tail) return;
	cside *s = new cside(this, style);
	s->preCorner = tail;
	s->postCorner = head;
	tail->postSide = s;
	head->preSide = s;
	tail = head;
}

void ccontour::Unclose()
{
	// TODO.  Write something analogous...
}

CRect ccontour::GetCornerBounds()
{
	CRect r;
	r.left = r.bottom = INT_MAX;
	r.right = r.top = INT_MIN;
	citer<ccorner> ic (&corners);
	for (ccorner *c = ic.First(); c; c = ic.Next())
	{
		r.left = min( r.left, c->x );
		r.right = max( r.right, c->x );
		r.bottom = min( r.bottom, c->y );
		r.top = max( r.top, c->y );
	}
	return r;
}


void cpolyline::MarkConstituents()
{
	// Mark the utility flags on this polyline and on its constituent contours, sides, and corners.
	utility = 1;
	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		ctr->utility = 1;
		citer<cside> is (&ctr->sides);
		for (cside *s = is.First(); s; s = is.Next())
			s->utility = 1;
		citer<ccorner> ic (&ctr->corners);
		for (ccorner *c = ic.First(); c; c = ic.Next())
			c->utility = 1;	
	}
}

CRect cpolyline::GetCornerBounds()
{
	CRect r;
	r.left = r.bottom = INT_MAX;
	r.right = r.top = INT_MIN;
	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		CRect r2 = ctr->GetCornerBounds();
		r.left = min(r.left, r2.left);
		r.right = max(r.right, r2.right);
		r.bottom = min(r.bottom, r2.bottom);
		r.top = max(r.top, r2.top);
	}
	return r;
}

CRect cpolyline::GetBounds()
{
	CRect r = GetCornerBounds();
	r.left -= m_w/2;
	r.right += m_w/2;
	r.bottom -= m_w/2;
	r.top += m_w/2;
	return r;
}

void cpolyline::Offset(int dx, int dy) 
{
	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		citer<ccorner> ic (&ctr->corners);
		for (ccorner *c = ic.First(); c; c = ic.Next())
			c->x += dx,
			c->y += dy;
	}
}

bool cpolyline::TestPointInside(int x, int y) 
{
	enum { MAXPTS = 100 };
	ASSERT( IsClosed() );

	// define line passing through (x,y), with slope = 2/3;
	// get intersection points
	double xx[MAXPTS], yy[MAXPTS];
	double slope = (double)2.0/3.0;
	double a = y - slope*x;
	int nloops = 0;
	int npts;
	// make this a loop so if my homebrew algorithm screws up, we try it again
	do
	{
		// now find all intersection points of line with polyline sides
		npts = 0;
		citer<ccontour> ic (&contours);
		for (ccontour *c = ic.First(); c; c = ic.Next())
		{
			cside *s0 = c->head->postSide, *s = s0;
			do {
				double x, y, x2, y2;
				int ok = FindLineSegmentIntersection( a, slope, 
					s->preCorner->x, s->preCorner->y,
					s->postCorner->x, s->postCorner->y,
					s->m_style,
					&x, &y, &x2, &y2 );
				if( ok )
				{
					xx[npts] = (int)x;
					yy[npts] = (int)y;
					npts++;
					ASSERT( npts<MAXPTS );	// overflow
				}
				if( ok == 2 )
				{
					xx[npts] = (int)x2;
					yy[npts] = (int)y2;
					npts++;
					ASSERT( npts<MAXPTS );	// overflow
				}
				s = s->postCorner->postSide;
			}
			while (s!=s0);
		}
		nloops++;
		a += PCBU_PER_MIL/100;
	} while( npts%2 != 0 && nloops < 3 );
	ASSERT( npts%2==0 );	// odd number of intersection points, error

	// count intersection points to right of (x,y), if odd (x,y) is inside polyline
	int ncount = 0;
	for( int ip=0; ip<npts; ip++ )
	{
		if( xx[ip] == x && yy[ip] == y )
			return FALSE;	// (x,y) is on a side, call it outside
		else if( xx[ip] > x )
			ncount++;
	}
	if( ncount%2 )
		return TRUE;
	else
		return FALSE;
}

void cpolyline::Hatch()
{
	if( m_hatch == NO_HATCH )
	{
		m_nhatch = 0;
		return;
	}

	CDisplayList *dl = doc->m_dlist;			// CPT2 TODO.  Think about.  Make it an arg for the function?
	if (!dl || !IsClosed()) return;
	enum {
		MAXPTS = 100,
		MAXLINES = 1000
	};
	dl_hatch.SetSize( MAXLINES, MAXLINES );

	int xx[MAXPTS], yy[MAXPTS];
	CRect r = GetCornerBounds();
	int slope_flag = 1 - 2*(m_layer%2);	// 1 or -1
	double slope = 0.707106*slope_flag;
	int spacing;
	if( m_hatch == DIAGONAL_EDGE )
		spacing = 10*PCBU_PER_MIL;
	else
		spacing = 50*PCBU_PER_MIL;
	int max_a, min_a;
	if( slope_flag == 1 )
	{
		max_a = (int)(r.top - slope*r.left);
		min_a = (int)(r.bottom - slope*r.right);
	}
	else
	{
		max_a = (int)(r.top - slope*r.right);
		min_a = (int)(r.bottom - slope*r.left);
	}
	min_a = (min_a/spacing)*spacing;
	int offset;
	if( m_layer < (LAY_TOP_COPPER+2) )
		offset = 0;
	else if( m_layer < (LAY_TOP_COPPER+4) )
		offset = spacing/2;
	else if( m_layer < (LAY_TOP_COPPER+6) )
		offset = spacing/4;
	else if( m_layer < (LAY_TOP_COPPER+8) )
		offset = 3*spacing/4;
	else if( m_layer < (LAY_TOP_COPPER+10) )
		offset = 1*spacing/8;
	else if( m_layer < (LAY_TOP_COPPER+12) )
		offset = 3*spacing/8;
	else if( m_layer < (LAY_TOP_COPPER+14) )
		offset = 5*spacing/8;
	else if( m_layer < (LAY_TOP_COPPER+16) )
		offset = 7*spacing/8;
	else
		ASSERT(0);
	min_a += offset;

	// now calculate and draw hatch lines
	int nhatch = 0;
	// loop through hatch lines
	for( int a=min_a; a<max_a; a+=spacing )
	{
		// get intersection points for this hatch line
		int nloops = 0;
		int npts;
		// make this a loop in case my homebrew hatching algorithm screws up
		do
		{
			npts = 0;
			citer<ccontour> ictr (&contours);
			for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
			{
				citer<cside> is (&ctr->sides);
				for (cside *s = is.First(); s; s = is.Next())
				{
					double x, y, x2, y2;
					int ok = FindLineSegmentIntersection( a, slope, 
								s->preCorner->x, s->preCorner->y,
								s->postCorner->x, s->postCorner->y,
								s->m_style,
								&x, &y, &x2, &y2 );
					if( ok )
					{
						xx[npts] = (int)x;
						yy[npts] = (int)y;
						npts++;
						ASSERT( npts<MAXPTS );	// overflow
					}
					if( ok == 2 )
					{
						xx[npts] = (int)x2;
						yy[npts] = (int)y2;
						npts++;
						ASSERT( npts<MAXPTS );	// overflow
					}
				}
			}
			nloops++;
			a += PCBU_PER_MIL/100;
		} while( npts%2 != 0 && nloops < 3 );
		ASSERT( npts%2==0 );	// odd number of intersection points, error

		// sort points in order of descending x (if more than 2)
		if( npts>2 )
		{
			for( int istart=0; istart<(npts-1); istart++ )
			{
				int max_x = INT_MIN;
				int imax;
				for( int i=istart; i<npts; i++ )
				{
					if( xx[i] > max_x )
					{
						max_x = xx[i];
						imax = i;
					}
				}
				int temp = xx[istart];
				xx[istart] = xx[imax];
				xx[imax] = temp;
				temp = yy[istart];
				yy[istart] = yy[imax];
				yy[imax] = temp;
			}
		}

		// draw lines
		for( int ip=0; ip<npts; ip+=2 )
		{
			double dx = xx[ip+1] - xx[ip];
			if( m_hatch == DIAGONAL_FULL || fabs(dx) < 40*NM_PER_MIL )
			{
				dl_element * el = dl->Add( this, dl_element::DL_HATCH, m_layer, DL_LINE, 1, 0, 0, 0,
					xx[ip], yy[ip], xx[ip+1], yy[ip+1], 0, 0 );
				dl_hatch.SetAtGrow(nhatch, el);
				nhatch++;
			}
			else
			{
				double dy = yy[ip+1] - yy[ip];	
				double slope = dy/dx;
				if( dx > 0 )
					dx = 20*NM_PER_MIL;
				else
					dx = -20*NM_PER_MIL;
				double x1 = xx[ip] + dx;
				double x2 = xx[ip+1] - dx;
				double y1 = yy[ip] + dx*slope;
				double y2 = yy[ip+1] - dx*slope;
				dl_element * el = dl->Add( this, dl_element::DL_HATCH, m_layer, DL_LINE, 1, 0, 0, 0,
					xx[ip], yy[ip], x1, y1, 0, 0 );
				dl_hatch.SetAtGrow(nhatch, el);
				el = dl->Add( this, dl_element::DL_HATCH, m_layer, DL_LINE, 1, 0, 0, 0, 
					xx[ip+1], yy[ip+1], x2, y2, 0, 0 );
				dl_hatch.SetAtGrow(nhatch+1, el);
				nhatch += 2;
			}
		}
	}
	m_nhatch = nhatch;
	dl_hatch.SetSize( m_nhatch );
}


int cpolyline::Draw( /* CDisplayList * dl */ )
{
	// CPT2 TODO.  Think about dl argument.
	int i_start_contour = 0;
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return NO_DLIST;
	if( IsDrawn() ) Undraw(); 

	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		// Draw corner selectors:
		if( m_sel_box )
		{
			citer<ccorner> ic (&ctr->corners);
			for (ccorner *c = ic.First(); c; c = ic.Next())
			{
				int x = c->x, y = c->y;
				c->dl_sel = dl->AddSelector( c, m_layer, DL_HOLLOW_RECT, 
					1, 0, 0, x-m_sel_box, y-m_sel_box, 
					x+m_sel_box, y+m_sel_box, 0, 0 );
			}
		}

		// Draw sides and side selectors
		citer<cside> is (&ctr->sides);
		for (cside *s = is.First(); s; s = is.Next())
		{
			int xi = s->preCorner->x, yi = s->preCorner->y;
			int xf = s->postCorner->x, yf = s->postCorner->y;
			if( xi == xf || yi == yf )
				// if endpoints not angled, make side STRAIGHT
				s->m_style = STRAIGHT;
			int g_type = DL_LINE;
			if( s->m_style == STRAIGHT )
				g_type = DL_LINE;
			else if( s->m_style == ARC_CW )
				g_type = DL_ARC_CW;
			else if( s->m_style == ARC_CCW )
				g_type = DL_ARC_CCW;
			s->dl_el = dl->AddMain( s, m_layer, g_type, 
				1, m_w, 0, 0, xi, yi, xf, yf, 0, 0 );
			if( m_sel_box )
				s->dl_sel = dl->AddSelector( s, m_layer, g_type, 
					1, m_w, 0, xi, yi, xf, yf, 0, 0 );
		}
	}

	if( m_hatch )
		Hatch();
	bDrawn = true;
	return NOERR;
}

// undraw polyline by removing all graphic elements from display list
//
void cpolyline::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl || !IsDrawn()) return;

	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		citer<cside> is (&ctr->sides);
		for (cside *s = is.First(); s; s = is.Next())
		{
			dl->Remove( s->dl_el );
			dl->Remove( s->dl_sel );
			s->dl_el = s->dl_sel = NULL;
		}
		citer<ccorner> ic (&ctr->corners);
		for (ccorner *c = ic.First(); c; c = ic.Next())
		{
			dl->Remove( c->dl_sel );
			c->dl_sel = NULL;
		}
		for( int i=0; i<dl_hatch.GetSize(); i++ )
			dl->Remove( dl_hatch[i] );

		// remove pointers
		dl_hatch.RemoveAll();
		m_nhatch = 0;
	}

	bDrawn = FALSE;
}


carea2::carea2(cnet2 *_net, int layer, int hatch)
	: cpolyline(_net->doc)
{ 
	m_layer = layer;
	m_hatch = hatch;
	m_net = _net;
	m_net->areas.Add(this);
}

void carea2::Remove()
{
	// CPT2. User wants to delete area, so detach it from the network (garbage collector will later delete this object and its
	// constituents for good).  bSetAreaConnections is true by default.
	Undraw();
	m_net->areas.Remove(this);
}

void carea2::SetConnections() 
{
	// set arrays of pins and other vtxs connected to area
	CDisplayList *dl = doc->m_dlist;
	for( int ip=0; ip < dl_thermal.GetSize(); ip++ )
		dl->Remove( dl_thermal[ip] );
	for( int is=0; is < dl_via_thermal.GetSize(); is++ )
		dl->Remove( dl_via_thermal[is] );
	pins.RemoveAll();
	vtxs.RemoveAll();
	dl_thermal.SetSize(0);
	dl_via_thermal.SetSize(0);

	// test all pins in net for being inside copper area 
	citer<cpin2> ip (&m_net->pins);
	for (cpin2 *pin = ip.First(); pin; pin = ip.Next())
		if (pin->part && pin->part->shape)
		{
			if (pin->pad_layer != LAY_PAD_THRU && pin->pad_layer != m_layer)
				continue;
			// See if pad is allowed to connect
			cpart2 *part = pin->part;
			pad *ppad = &pin->ps->inner;							// Default value
			if( part->side == 0 && m_layer == LAY_TOP_COPPER
				|| part->side == 1 && m_layer == LAY_BOTTOM_COPPER )
						ppad = &pin->ps->top;
			else if( part->side == 1 && m_layer == LAY_TOP_COPPER
					|| part->side == 0 && m_layer == LAY_BOTTOM_COPPER )
						ppad = &pin->ps->bottom;
			if( ppad->connect_flag == PAD_CONNECT_NEVER )
				continue;	// pad never allowed to connect
			if( ppad->connect_flag == PAD_CONNECT_DEFAULT && !pin->ps->hole_size && !doc->m_bSMT_copper_connect )
				continue;	// pad uses project option not to connect SMT pads
			if( pin->pad_layer != LAY_PAD_THRU && ppad->shape == PAD_NONE )
				continue;	// no SMT pad defined (this should not happen)
			if( TestPointInside( pin->x, pin->y ) )
			{
				// pin is inside copper area
				pins.Add(pin);
				if( dl )
				{
					int w = pin->GetWidth();
					id no_id;																			// CPT2 TODO.  Fix once dl->Add has the id removed
					dl_element * dle = dl->Add( no_id, m_net, LAY_RAT_LINE, DL_X, m_net->bVisible,		// dle will eventually point to pin
								2*w/3, 0, 0, pin->x, pin->y, 0, 0, 0, 0 );
					dl_thermal.Add( dle );
				}
			}
	}

	// test all vias in traces for being inside copper area,
	// also test all end-vertices of non-branch stubs for being on same layer
	// AMW r288: test all vertices of all traces, not just stubs
	citer<cconnect2> ic (&m_net->connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
	{
		citer<cvertex2> iv (&c->vtxs);
		for (cvertex2 *v = iv.First(); v; v = iv.Next())
		{
			if (v->pin) continue;
			bool bVtxMayConnectToArea = FALSE;
			int x_w = 0;
			if( v->via_w )
			{
				bVtxMayConnectToArea = TRUE;
				x_w = v->via_w;
			}
			if( v->preSeg && v->preSeg->m_layer == m_layer ) 
			{
				bVtxMayConnectToArea = TRUE;
				x_w = max( x_w, v->preSeg->m_width + 10*NM_PER_MIL );
			}
			if( v->postSeg && v->postSeg->m_layer == m_layer ) 
			{
				bVtxMayConnectToArea = TRUE;
				x_w = max( x_w, v->postSeg->m_width + 10*NM_PER_MIL );
			}
			if( bVtxMayConnectToArea && TestPointInside( v->x, v->y ) )
			{
				// vertex is inside copper area
				vtxs.Add(v);
				if( dl )
				{
					dl_element * dle = dl->Add( this, dl_element::DL_OTHER, LAY_RAT_LINE, DL_X, m_net->bVisible,
						2*x_w/3, 0, 0, v->x, v->y, 0, 0, 0, 0 );
					dl_via_thermal.Add(dle);
				}
			}
		}
	}
}

csmcutout::csmcutout(CFreePcbDoc *_doc)
	: cpolyline(_doc)
{
	doc->others.Add(this);
}

cboard::cboard(CFreePcbDoc *_doc) 
	: cpolyline(_doc)
{
	doc->others.Add(this);
}

coutline::coutline(CFreePcbDoc *_doc, int layer, int w)
	: cpolyline(_doc)
{
	if (doc)
		doc->others.Add(this);
	m_layer = layer;
	m_w = w;
}



/**********************************************************************************************/
/*  RELATED TO cnet2                                                                           */
/**********************************************************************************************/

cnet2::cnet2( cnetlist *nlist, CString _name, int _def_w, int _def_via_w, int _def_via_hole_w )
	: cpcb_item (nlist->m_doc)
{
	m_nlist = nlist;
	nlist->nets.Add(this);
	name = _name;
	def_w = _def_w;
	def_via_w = _def_via_w; def_via_hole_w = _def_via_hole_w;
	bVisible = true;	
}


void cnet2::SetVisible( bool _bVisible )
{
	if( bVisible == _bVisible )	return;
	bVisible = _bVisible;
	if( bVisible )
	{
		// make segments visible
		citer<cconnect2> ic (&connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next()) 
		{
			citer<cseg2> is (&c->segs);
			for (cseg2 *s = is.First(); s; s = is.Next())
				s->dl_el->visible = s->dl_sel->visible = true;
		}
		// make thermals visible
		citer<carea2> ia (&areas);
		for (carea2 *a = ia.First(); a; a = ia.Next())
		{
			citer<cpin2> ip (&a->pins);
			for (cpin2 *p = ip.First(); p; p = ip.Next())
				p->SetThermalVisible(a->m_layer, true);
		}
	}
	else
	{
		// make ratlines invisible and disable selection items
		citer<cconnect2> ic (&connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next()) 
		{
			citer<cseg2> is (&c->segs);
			for (cseg2 *s = is.First(); s; s = is.Next())
				if (s->m_layer == LAY_RAT_LINE)
					s->dl_el->visible = s->dl_sel->visible = true;
		}
		// make thermals invisible
		citer<carea2> ia (&areas);
		for (carea2 *a = ia.First(); a; a = ia.Next())
		{
			citer<cpin2> ip (&a->pins);
			for (cpin2 *p = ip.First(); p; p = ip.Next())
				p->SetThermalVisible(a->m_layer, false);
		}
	}
}

void cnet2::Highlight()
{
	HighlightConnections();
	citer<carea2> ia (&areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		a->Highlight();
}

cpin2 *cnet2::AddPin( CString * ref_des, CString * pin_name, BOOL set_areas )
{
	// CPT2.  TODO no check if pin was previously assigned to another net.  Change?  Plus, deal with set_areas
	cpin2 *pin = doc->m_plist->GetPinByNames(ref_des, pin_name);
	if (!pin) return NULL;
	pins.Add(pin);
	pin->net = this;
	return pin;
}

void cnet2::CalcViaWidths(int w, int *via_w, int *via_hole_w) {
	// CPT r295.  Helper for cvertex2::ReconcileVia().  
	// Given a segment width value "w", determine a matching via and via hole width.  Do this by first checking if w==net->def_w,
	//  and return the net's default via sizes if so;
	//  otherwise, scan thru m_doc->m_w and find the last entry in there <=w.  The corresponding members of m_doc->m_v_w
	// and m_doc->m_v_h_w are then the return values.
	CFreePcbDoc *doc = m_nlist->m_doc;
	int cWidths = doc->m_w.GetSize(), i;
	if (w == def_w || cWidths == 0)
		{ *via_w = def_via_w; *via_hole_w = def_via_hole_w; return; }
	for (i=1; i<cWidths; i++)
		if (doc->m_w.GetAt(i) > w) break;
	// i-1 = last entry in width table that's <= w:
	*via_w = doc->m_v_w.GetAt(i-1);
	*via_hole_w = doc->m_v_h_w.GetAt(i-1);
	}

cvertex2 *cnet2::TestHitOnVertex(int layer, int x, int y) 
{
	// Test for a hit on a vertex in a trace within this layer.
	// If layer == 0, ignore layer.  New return value: returns the hit vertex, or 0 if no vtx is near (x,y) on layer
	citer<cconnect2> ic (&connects);
	for( cconnect2 *c = ic.First(); c; c = ic.Next())
	{
		citer<cvertex2> iv (&c->vtxs);
		for( cvertex2 *v = iv.First(); v; v = iv.Next())
		{
			cseg2 *pre = v->preSeg? v->preSeg: v->postSeg;
			cseg2 *post = v->postSeg? v->postSeg: v->preSeg;
			if( v->via_w > 0 || layer == 0 || layer == pre->m_layer || layer == post->m_layer
				|| (pre->m_layer == LAY_RAT_LINE && post->m_layer == LAY_RAT_LINE) )
			{
				int test_w = max( v->via_w, pre->m_width );
				test_w = max( test_w, post->m_width );
				test_w = max( test_w, 10*NM_PER_MIL );		// in case widths are zero
				double dx = x - v->x, dy = y - v->y;
				double d = sqrt( dx*dx + dy*dy );
				if( d < test_w/2 )
					return v;
			}
		}
	}
	return NULL;
}

int cnet2::Draw() 
{
	citer<cconnect2> ic (&connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
		c->Draw();
	citer<carea2> ia (&areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		a->Draw();
	// Also deal with tees that are in the net.  (The subsidiary vtxs are never drawn.)
	citer<ctee> it (&m_nlist->tees);
	for (ctee *t = it.First(); t; t = it.Next())
	{
		cvertex2 *v = t->vtxs.First();
		if (v && v->m_net==this)
			t->Draw();
	}
	
	return NOERR;
}

/**********************************************************************************************/
/*  OTHERS: ctext, cadhesive, ccentroid                                                       */
/**********************************************************************************************/
