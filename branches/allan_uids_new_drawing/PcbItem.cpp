#include "stdafx.h"
#include <math.h>
#include <stdlib.h>
#include "PcbItem.h"
#include "NetListNew.h"
#include "FreePcbDoc.h"

class CFreePcbDoc;
class cpcb_item;

int cpcb_item::next_uid = 1;

cpcb_item::cpcb_item(CFreePcbDoc *_doc)
{
	carray_list = NULL; 
	m_uid = next_uid++;
	doc = _doc;
	doc->items.Add(this);
	dl_el = dl_sel = NULL;
	utility = 0;
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

	int nSegsInConnect = m_con->segs.GetNItems();
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
	if (IsViaNeeded()) 
	{
		// CPT r295. Calculate via width, based on the widths of the adjacent segs and possibly the net defaults.
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

ctee::ctee(cnet2 *n)
	: cpcb_item (n->doc)
	{ via_w = via_hole_w = 0; }

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
	int nItems = vtxs.GetNItems();
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


cseg2::cseg2(cconnect2 *c, int _layer, int _width)							// CPT2 added args.  Replaces Initialize()
	: cpcb_item( c->doc )
{
	m_con = c;
	m_con->segs.Add(this);
	m_net = c->m_net;
	m_layer = _layer;
	m_width = _width;
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
	int nSegsInConnect = m_con->segs.GetNItems();
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
	int nSegsInConnect = m_con->segs.GetNItems();
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

cconnect2::cconnect2( cnet2 * _net ) 
	: cpcb_item (_net->doc)
{
	m_net = _net;
	m_net->connects.Add(this);
	locked = 0;
	m_bDrawn = FALSE;
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
	ctee *headT = head->tee, *tailT = tail->tee;
	if (bSetAreaConnections)
		m_net->SetAreaConnections();
}


void cconnect2::AppendSegAndVertex( cseg2 *s, cvertex2 *v, cvertex2 *after)
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
	nextSeg->preVtx = v;
	if (tail==after) 
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
// this is mainly used for stub traces
// returns index to new segment
void cconnect2::AppendSegment(int x, int y, int layer, int width )
{
	// add new vertex and segment
	BOOL bWasDrawn = m_bDrawn;
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
	if (segs.GetNItems() < 2) return;
	bool bWasDrawn = m_bDrawn;
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



/**********************************************************************************************/
/*  RELATED TO cpin2/cpart2                                                                     */
/**********************************************************************************************/

cpin2::cpin2(cpart2 *_part, cnet2 *_net)					// CPT2. Added args
	: cpcb_item (_part->doc)
{
	part = _part;
	part->pins.Add(this);
	net = _net;
	if (net)
		net->pins.Add(this);
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


/**********************************************************************************************/
/*  RELATED TO cpolyline/carea2/csmcutout                                                      */
/**********************************************************************************************/

ccorner::ccorner(ccontour *_contour, int _x, int _y)
	: cpcb_item (_contour->doc)
{
	contour = _contour; 
	contour->corners.Add(this);
	x = _x; y = _y; 
}

int ccorner::GetTypeBit() 
{														// Later:  put in .cpp file.  There wouldn't be this nonsense in Java...
	if (contour->poly->IsArea()) return bitAreaCorner;
	if (contour->poly->IsSmCutout()) return bitSmCorner;
	if (contour->poly->IsBoard()) return bitBoardCorner;
	return bitOther;
}

cside::cside(ccontour *_contour)
	: cpcb_item(_contour->doc)
{ 
	contour = _contour;
	contour->sides.Add(this);
}

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
	else
		poly->contours.Add(this);
}



void cpolyline::MarkConstituents()
{
	// Mark the utility flags on this polyline and on its constituent contours, sides, and corners.
	utility = 1;
	main->utility = 1;
	citer<cside> is (&main->sides);
	for (cside *s = is.First(); s; s = is.Next())
		s->utility = 1;
	citer<ccorner> ic (&main->corners);
	for (ccorner *c = ic.First(); c; c = ic.Next())
		c->utility = 1;
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

carea2::carea2(cnet2 *_net)
	: cpolyline(_net->doc)
{ 
	m_net = _net;
	m_net->areas.Add(this);
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

coutline::coutline(CFreePcbDoc *_doc) 
	: cpolyline(_doc)
{
	doc->others.Add(this);
}



/**********************************************************************************************/
/*  RELATED TO cnet2                                                                           */
/**********************************************************************************************/

cnet2::cnet2( cnetlist *nlist, CString _name, int _def_w, int _def_via_w, int _def_via_hole_w )
	: cpcb_item (nlist->m_doc)
{
	name = _name;
	def_w = _def_w;
	def_via_w = _def_via_w; def_via_hole_w = _def_via_hole_w;
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

/**********************************************************************************************/
/*  OTHERS: ctext, cadhesive, ccentroid                                                       */
/**********************************************************************************************/
