#include "stdafx.h"
#include <math.h>
#include <stdlib.h>
#include "PcbItem.h"
#include "NetListNew.h"
#include "FreePcbDoc.h"
#include "PartListNew.h"
#include "TextListNew.h"

extern BOOL bDontShowSelfIntersectionWarning;		// CPT2 TODO make these "sticky" by putting settings into default.cfg.
extern BOOL bDontShowSelfIntersectionArcsWarning;
extern BOOL bDontShowIntersectionWarning;
extern BOOL bDontShowIntersectionArcsWarning;

class CFreePcbDoc;
class cpcb_item;

int cpcb_item::next_uid = 1;

cpcb_item::cpcb_item(CFreePcbDoc *_doc)
{
	carray_list = NULL; 
	m_uid = next_uid++;
	doc = _doc;
	if (doc)
		doc->items.Add(this);						// NB if doc is NULL, this will not be garbage-collected (must delete by hand)
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

void cpcb_item::SetDoc(CFreePcbDoc *_doc)
{
	if (doc) 
		doc->items.Remove(this);
	doc = _doc;
	if (doc)
		doc->items.Add(this);
}

void cpcb_item::MustRedraw()
{
	if (bDrawn)
		Undraw();
	if (doc)
		doc->redraw.Add(this);
}


carea2 *cpcb_item::GetArea() 
{
	if (cpolyline *p = GetPolyline())
		return p->ToArea();
	return NULL;
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
	dl_hole = dl_thermal = NULL;
	SetNeedsThermal();
}

bool cvertex2::IsValid()
{
	if (!m_net->IsValid()) return false;
	if (!m_con->IsValid()) return false;
	return m_con->vtxs.Contains(this);
}

bool cvertex2::IsVia() 
	{ return via_w>0 || tee && tee->via_w>0; }

int cvertex2::GetLayer()
{
	// Return LAY_PAD_THRU for vias.  Otherwise return the layer of whichever emerging seg isn't a ratline.  Failing that, return LAY_RAT_LINE.
	if (tee) return tee->GetLayer();
	if (pin) return pin->GetLayer();
	if (via_w) return LAY_PAD_THRU;
	if (preSeg && preSeg->m_layer != LAY_RAT_LINE) return preSeg->m_layer;
	if (postSeg && postSeg->m_layer != LAY_RAT_LINE) return postSeg->m_layer;
	return LAY_RAT_LINE;
}

void cvertex2::GetStatusStr( CString * str )
{
	int u = doc->m_units;
	CString type_str, x_str, y_str, via_w_str, via_hole_str, s;
	if( pin )
		type_str.LoadStringA(IDS_PinVertex);	// should never happen
	else if( tee )
		s.LoadStringA(IDS_TVertex),
		type_str.Format( s, tee->UID() );
	else if( !preSeg || !postSeg )
		type_str.LoadStringA(IDS_EndVertex);
	else
		type_str.LoadStringA(IDS_Vertex);
	::MakeCStringFromDimension( &x_str, x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
	::MakeCStringFromDimension( &y_str, y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
	if( via_w )
	{
		::MakeCStringFromDimension( &via_w_str, via_w, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		::MakeCStringFromDimension( &via_hole_str, via_hole_w, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		CString s ((LPCSTR) IDS_XYVia);
		str->Format( s, type_str, x_str, y_str, via_w_str, via_hole_str );
		// CPT2 added (used to be there, liked it)
		if (force_via_flag)
			str->Append(" (F)");
	}
	else
	{
		CString s ((LPCSTR) IDS_XYNoVia);
		str->Format( s,	type_str, x_str, y_str );
	}
}

void cvertex2::GetTypeStatusStr( CString * str )
{
	if( pin )
		str->Format( "%s.%s", pin->part->ref_des, pin->pin_name );
	else if( tee )
		str->LoadStringA(IDS_Tee);
	/* CPT2. Having connects described as "from end vertex to end vertex" seems overlengthy.  How about "from vtx to vtx"?
	else if( type == V_END )
	{
		str->LoadStringA(IDS_EndVertex);
	}
	*/
	else
		str->LoadStringA(IDS_Vertex);
}


bool cvertex2::Remove()
{
	// Derived from old cnet::RemoveVertex() functions.  Remove vertex from the network.  If it's a tee-vertex, all associated vertices must be
	// axed as well.  If it's an end vertex of another type, remove vertex and the single attached seg.  If it's a middle vertex, merge the two adjacent
	// segs.
	if (tee)
	{
		tee->Remove(true);														// Removes tee and all its constituents.
		return false;
	}

	Undraw();																	// To get rid of selector drawing-el (?)
	int nSegsInConnect = m_con->segs.GetSize();
	if (nSegsInConnect<2) 
		{ m_con->Remove(); return true; }

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

void cvertex2::SetConnect(cconnect2 *c)
{
	// CPT2 new.  Attach this to the given connect.  If it's already attached to a connect, detach it from the old one.  Does not alter preSeg/postSeg
	if (m_con==c) return;
	if (m_con)
		m_con->vtxs.Remove(this);
	c->vtxs.Add(this);
	m_con = c;
}

bool cvertex2::IsLooseEnd() 
	{ return (!preSeg || !postSeg) && !pin && !tee && via_w==0; }

void cvertex2::ForceVia()
{
	force_via_flag = 1;
	ReconcileVia();
}

void cvertex2::UnforceVia()
{
	force_via_flag = 0;
	ReconcileVia();
}

bool cvertex2::IsViaNeeded()
{
	if (tee) return tee->IsViaNeeded();
	if (pin) return false;
	if (force_via_flag) return true;
	if (!preSeg || !postSeg) return false;					// CPT2 TODO: check...
	if (preSeg->m_layer == postSeg->m_layer) return false;
	return preSeg->m_layer!=LAY_RAT_LINE && postSeg->m_layer!=LAY_RAT_LINE;
}

void cvertex2::SetViaWidth()
{
	// CPT2.  We've determined that "this" needs a via, so determine the appropriate width based on incoming segs
	if (force_via_flag && via_w)
		return;
	MustRedraw();
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
		// Not likely.  Use defaults...
		via_w = m_net->def_via_w,
		via_hole_w = m_net->def_via_hole_w;
}

void cvertex2::ReconcileVia()
{
	// CPT2.  Gets an appropriate width for a via on this vertex, if any.  NB DOESN'T DO ANY DRAWING ANY MORE (see proposed policy in notes.txt)
	if (tee)
		{ tee->ReconcileVia(); return; }

	if (IsViaNeeded()) 
		SetViaWidth();
	else if (via_w || via_hole_w)
		via_w = via_hole_w = 0,
		MustRedraw();
}

int cvertex2::GetViaConnectionStatus( int layer )
{
#ifndef CPT2
	// Derived from old CNetList function. CPT2 TODO possibly obsolete
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
		if( a->GetLayer() == layer )
		{
			// area is on this layer, loop through via connections to area
			citer<cvertex2> iv (&a->vtxs);
			for( cvertex2 *v = iv.First(); v; v = iv.Next() )
				if( v == this )
					// via connects to area
					status |= VIA_AREA;
		}
	return status;
#endif
	return 0;
}

bool cvertex2::TestHit(int _x, int _y, int _layer)
{
	// CPT2 Semi-supplants cnet2::TestHitOnVertex.  This routine also behaves appropriately if the vertex is part of a tee or associated with a pin.
	if (pin)
		return pin->TestHit(_x, _y, _layer);

	// First check that the layer is OK:
	if (_layer>0)
	{
		int layer = GetLayer();
		if (layer != LAY_PAD_THRU && layer != LAY_RAT_LINE && layer != _layer)
			return false; 
	}
	// Determine a test radius (perhaps this is a little obsessive...)
	int test_w = max( via_w, 10*NM_PER_MIL );
	if (preSeg)
		test_w = max( test_w, preSeg->m_width );
	if (postSeg)
		test_w = max( test_w, postSeg->m_width );
	if (tee)
	{
		test_w = max( test_w, tee->via_w );
		citer<cvertex2> iv (&tee->vtxs);
		for (cvertex2 *v = iv.First(); v; v = iv.Next()) 
		{
			if (v->preSeg) 
				test_w = max(test_w, v->preSeg->m_width);
			if (v->postSeg)
				test_w = max(test_w, v->postSeg->m_width);
		}
	}

	double dx = x - _x, dy = y - _y;
	double d = sqrt( dx*dx + dy*dy );
	return d < test_w/2;
}

void cvertex2::Move( int _x, int _y )
{
	if (tee)
		{ tee->Move(_x, _y); return; }
	doc->m_dlist->StopDragging();
	m_con->MustRedraw();						// CPT2 crude but probably adequate.
	x = _x;
	y = _y;
	ReconcileVia();
	SetNeedsThermal();
}

cconnect2 * cvertex2::SplitConnect()
{
	// Split a connection into two connections sharing a tee-vertex
	// return pointer to the new connection
	// both connections will end at the tee-vertex
	// CPT2 adapted from old cnet::SplitConnectAtVertex.  Assumes that this is in the middle of a seg (therefore not part of a tee)
	ASSERT(preSeg && postSeg);
	m_net->MustRedraw();										// CPT2 crude but probably adequate.
	cconnect2 *old_c = m_con;
	cconnect2 * new_c = new cconnect2(m_net);
	cvertex2 *new_v = new cvertex2(new_c, x, y);
	new_c->head = new_v;
	new_c->tail = old_c->tail;
	new_v->postSeg = postSeg;
	postSeg->preVtx = new_v;
	// Now reassign the second half of connect's vtxs and segs to new_c
	for (cseg2 *s = postSeg; s; s = s->postVtx->postSeg)
		s->SetConnect(new_c),
		s->postVtx->SetConnect(new_c);
	// Truncate old_c
	old_c->tail = this;
	this->postSeg = NULL;
	// Bind this and new_v together in a new tee
	ctee *t = new ctee(m_net);
	t->Add(this);
	t->Add(new_v);
	return new_c;
}

cseg2 * cvertex2::AddRatlineToPin( cpin2 *pin )
{
	// Add a ratline from a vertex to a pin
	// CPT2 Adapted from cnet::AddRatlineFromVtxToPin().  NOW RETURNS THE RATLINE SEG.
	cconnect2 * c = m_con;
	if (IsTraceVtx())
	{
		// normal internal vertex in a trace 
		// split trace and make this vertex a tee.  SplitConnect() also calls m_net->MustRedraw()
		SplitConnect();
		// Create new connect and attach its first end to the new tee
		cconnect2 *new_c = new cconnect2 (m_net);
		cvertex2 *new_v1 = new cvertex2 (new_c, x, y);
		new_c->Start(new_v1);
		this->tee->Add(new_v1);
		new_c->AppendSegment(pin->x, pin->y, LAY_RAT_LINE, 0);
		new_c->tail->pin = pin;
		return new_v1->postSeg;
	}
	else if( !tee )
	{
		// loose endpoint vertex, just extend it to the pin
		c->MustRedraw();
		cseg2 *ret;
		if (!postSeg)
			c->AppendSegment(pin->x, pin->y, LAY_RAT_LINE, 0),
			c->tail->pin = pin,
			ret = c->tail->preSeg;
		else
			c->PrependSegment(pin->x, pin->y, LAY_RAT_LINE, 0),
			c->head->pin = pin,
			ret = c->head->postSeg;
		return ret;
	}
	else 
	{
		// new connection from tee-vertex to pin
		cconnect2 *new_c = new cconnect2 (m_net);
		cvertex2 *new_v1 = new cvertex2 (new_c, x, y);
		new_c->Start(new_v1);
		tee->Add(new_v1);
		new_c->AppendSegment(pin->x, pin->y, LAY_RAT_LINE, 0);
		new_c->tail->pin = pin;
		new_c->MustRedraw();
		return new_v1->postSeg;
	}
}

void cvertex2::StartDraggingStub( CDC * pDC, int _x, int _y, int layer1, int w, 
								   int layer_no_via, int crosshair, int inflection_mode )
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;
	dl->CancelHighlight();
	// CPT2 The following doesn't seem right to me, plus it's a lot of trouble, so I commented it out.
	// For instance if "this" is a forced via, why hide it?
	/*
	// Hide this vertex (if a via) and its thermals.
	SetVisible(false);
	citer<carea2> ia (&m_net->areas);
	for (carea2 *a = ia.First(); a; a = ia.Next()) 
		for (int i=0; i < a->dl_via_thermal.GetSize(); i++)
			if (a->dl_via_thermal[i] -> item == this)
				dl->Set_visible(a->dl_via_thermal[i], 0);
	*/
	// start dragging, start point is preceding vertex
	dl->StartDraggingLine( pDC, _x, _y, this->x, this->y, layer1, 
		w, layer_no_via, w*3/2, w,												// CPT r295.  Put in approx args for via_w and via_hole_w.  Fairly bogus... 
		crosshair, DSS_STRAIGHT, inflection_mode );
}

// CPT2 NB CancelDraggingStub() is pretty useless at this point, so it's eliminated.  (Just need to do a dlist->StopDragging().)

// Start dragging vertex to reposition it
//
void cvertex2::StartDragging( CDC * pDC, int x, int y, int crosshair )
{
	// cancel previous selection and make segments and via invisible
	CDisplayList *dl = doc->m_dlist;
	cconnect2 *c = m_con;
	cnet2 *net = m_net;
	dl->CancelHighlight();
	if (preSeg)
		dl->Set_visible(preSeg->dl_el, 0);				// CPT2 TODO change name to SetVisible (et al)
	if (postSeg)
		dl->Set_visible(postSeg->dl_el, 0);
	SetVisible( FALSE );

	// start dragging
	if( preSeg && postSeg )
	{
		// vertex with segments on both sides
		int xi = preSeg->preVtx->x;
		int yi = preSeg->preVtx->y;
		int xf = postSeg->postVtx->x;
		int yf = postSeg->postVtx->y;
		int layer1 = preSeg->m_layer;
		int layer2 = postSeg->m_layer;
		int w1 = preSeg->m_width;
		int w2 = postSeg->m_width;
		dl->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, layer1, 
									layer2, w1, w2, DSS_STRAIGHT, DSS_STRAIGHT, 
									0, 0, 0, 0, crosshair );
	}
	else
	{
		// end-vertex, only drag one segment
		cseg2 *s = preSeg? preSeg: postSeg;
		cvertex2 *other = preSeg? preSeg->preVtx: postSeg->postVtx;
		int xi = other->x;
		int yi = other->y;
		int layer1 = s->m_layer;
		int w1 = s->m_width;
		dl->StartDraggingLine( pDC, x, y, xi, yi, layer1, 
									w1, layer1, 0, 0, crosshair, DSS_STRAIGHT, 0 );
	}
}

void cvertex2::CancelDragging()
{
	// make segments and via visible
	CDisplayList *dl = doc->m_dlist;
	if (preSeg)
		dl->Set_visible(preSeg->dl_el, 1);
	if (postSeg)
		dl->Set_visible(postSeg->dl_el, 1);
	SetVisible( TRUE );
	dl->StopDragging();
}


int cvertex2::Draw()
{
	cnetlist * nl = m_net->m_nlist;
	CDisplayList *dl = doc->m_dlist;
	if (tee)
		return NOERR;					// CPT2.  Rather than draw the individual vertices, just draw the tee itself.
	if (pin)
		return NOERR;					// CPT2.  Similarly, draw the pin and its selector, not the vtx.
	if (bDrawn)
		return ALREADY_DRAWN;

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
		// CPT2.  Trying a wider selector for vias (depending on hole size)
		dl_sel = dl->AddSelector( this, LAY_SELECTION, DL_HOLLOW_RECT, 
			1, 0, 0, x-via_w/2, y-via_w/2, x+via_w/2, y+via_w/2, 0, 0 );
		// CPT2.  Now draw a thermal, if the bNeedsThermal flag is set
		if (bNeedsThermal)
			dl_thermal = dl->Add( this, dl_element::DL_OTHER, LAY_RAT_LINE, DL_X, 1,
								2*via_w/3, 0, 0, x, y, 0, 0, 0, 0 );
	}
	else
	{
		// draw selection box for vertex, using layer of adjacent
		// segments.  CPT2 TODO figure out why the layer of the selector matters...
		int sel_layer;
		if( preSeg )
			sel_layer = preSeg->m_layer;
		else
			sel_layer = postSeg->m_layer;
		dl_sel = dl->AddSelector( this, sel_layer, DL_HOLLOW_RECT, 
			1, 0, 0, x-10*PCBU_PER_MIL, y-10*PCBU_PER_MIL, x+10*PCBU_PER_MIL, y+10*PCBU_PER_MIL, 0, 0 );
	}

	bDrawn = true;
	return NOERR;
}

void cvertex2::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl || !IsDrawn()) return;
	for( int i=0; i<dl_els.GetSize(); i++ )
		//	m_con->m_net->m_dlist->Remove( dl_el[i] );				// CPT2.  Was this way.  Not sure why...
		dl->Remove( dl_els[i] );
	dl_els.RemoveAll();
	dl->Remove( dl_sel );
	dl->Remove( dl_hole );
	dl->Remove( dl_thermal );
	dl_sel = dl_hole = dl_thermal = NULL;
	bDrawn = false;
}

void cvertex2::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;
	if (tee)
		// Since vertices within a tee don't have selectors, it's unlikely that Highlight() will be called on them.
		// But just in case...
		{ tee->Highlight(); return; }
	if (pin)
		// Similarly for pin vertices:  highlight the pin, not the vtx
		{ pin->Highlight(); return; }

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

void cvertex2::SetVisible( bool bVis )
{
	for( int il=0; il<dl_els.GetSize(); il++ )
		if( dl_els[il] )
			dl_els[il]->visible = bVis;
	if( dl_hole )
		dl_hole->visible = bVis;
	if (dl_thermal)
		dl_thermal->visible = bVis;
}

bool cvertex2::SetNeedsThermal()
{
	// CPT2 new, but related to the old CNetList::SetAreaConnections.  Sets bNeedsThermal to true if some net area overlaps this vertex.
	bool oldVal = bNeedsThermal;
	bNeedsThermal = false;
	citer<carea2> ia (&m_net->areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		if (a->TestPointInside(x, y))
			{ bNeedsThermal = true; break; }
	if (bNeedsThermal != oldVal)
		MustRedraw();
	return bNeedsThermal;
}



ctee::ctee(cnet2 *n)
	: cpcb_item (n->doc)
{ 
	via_w = via_hole_w = 0; 
	n->tees.Add(this);
	dl_hole = dl_thermal = NULL;
}

int ctee::GetLayer()
{
	// If this has a via, return LAY_PAD_THRU.
	// Otherwise search the component vtxs for an emerging segment that has a layer other than LAY_RAT_LINE.  Return the first such layer if found.
	// By default return LAY_RAT_LINE
	if (via_w) 
		return LAY_PAD_THRU;
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		if (v->preSeg && v->preSeg->m_layer != LAY_RAT_LINE)
			return v->preSeg->m_layer;
		else if (v->postSeg && v->postSeg->m_layer != LAY_RAT_LINE)
			return v->postSeg->m_layer;
	return LAY_RAT_LINE;
}

void ctee::GetStatusStr( CString * str )
{
	cvertex2 *v = vtxs.First();
	if (!v) 
		{ *str = "???"; return; }

	int u = doc->m_units;
	CString type_str, x_str, y_str, via_w_str, via_hole_str, s;
	s.LoadStringA(IDS_TVertex),
	type_str.Format( s, UID() );
	::MakeCStringFromDimension( &x_str, v->x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
	::MakeCStringFromDimension( &y_str, v->y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
	if( via_w )
	{
		::MakeCStringFromDimension( &via_w_str, via_w, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		::MakeCStringFromDimension( &via_hole_str, via_hole_w, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		s.LoadStringA( IDS_XYVia );
		str->Format( s, type_str, x_str, y_str, via_w_str, via_hole_str );
	}
	else
		s.LoadStringA( IDS_XYNoVia ),
		str->Format( s,	type_str, x_str, y_str );
	if (v->force_via_flag)
		str->Append(" (F)");

}


void ctee::Remove(bool bRemoveVtxs)
{
	// Disconnect this from everything:  that is, remove references to this from all vertices in vtxs, and then clear out this->vtxs.  The
	// garbage collector will later delete this.
	Undraw();
	cnet2 *net = vtxs.First()? vtxs.First()->m_net: NULL;
	if (net) net->tees.Remove(this);
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
	{
		v->tee = NULL;
		if (bRemoveVtxs) v->Remove();
	}
	vtxs.RemoveAll();
}

void ctee::Add(cvertex2 *v)
{
	// New.  Attach v to this->vtxs, and set v->tee.  Check first if v needs detaching from some other tee.
	if (v->tee && v->tee!=this)
		v->tee->Remove(v);
	v->tee = this;
	vtxs.Add(v);
}

bool ctee::Adjust()
{
	// Typically called after a segment/connect attached to a tee-vertex is removed.  At that point we see if there are still 3+ vertices 
	// remaining within the tee;  if so, do nothing and return false.  If there's 0-1 vertex in the tee (not that likely, I think), just 
	// remove the tee from the network and return true. If there are 2 attached connects, combine them before removing the tee & returning true.
	int nItems = vtxs.GetSize();
	if (nItems > 2) 
		{ ReconcileVia(); return false; }
	if (nItems == 0) 
		{ Remove(); return true; }
	citer<cvertex2> iv (&vtxs);
	cvertex2 *v0 = iv.First(), *v1 = iv.Next();
	if (nItems == 1)
	{
		Remove();
		v0->MustRedraw();
		v0->ReconcileVia();
		return true;
	}
	cconnect2 *c0 = v0->m_con, *c1 = v1->m_con;
	if (c0==c1) 
		// Remaining connect is a circle!  A tee is still required in that case. 
		{ ReconcileVia(); return false; }
	c0->CombineWith(c1, v0, v1);						// Calls c0->MustRedraw()
	Remove();
	v0->ReconcileVia();
	return true;
}

void ctee::ForceVia()
{
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->force_via_flag = 1;
	ReconcileVia();
}

void ctee::UnforceVia()
{
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->force_via_flag = 0;
	ReconcileVia();
}

bool ctee::IsViaNeeded()
{
	int layer0 = -1;
	citer<cvertex2> iv (&vtxs);
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

void ctee::ReconcileVia()
{
	cvertex2 *v = vtxs.First();
	if (v && v->force_via_flag && via_w>0)
		return;														// Don't tinker with preexisting via width for forced tee vias...
	MustRedraw();
	via_w = via_hole_w = 0;
	if (!IsViaNeeded())
		return;
	// Set via_w and via_hole_w, based on the max of the widths for the constituent vtxs (which have to be set first)
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->SetViaWidth();
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		if (v->via_w > via_w)
			via_w = v->via_w, via_hole_w = v->via_hole_w;
}

void ctee::Remove(cvertex2 *v, bool fAdjust) 
{
	Undraw();
	vtxs.Remove(v);
	v->tee = NULL;
	if (fAdjust)
		Adjust(),
		ReconcileVia();
}

void ctee::Move(int _x, int _y) 
{
	// CPT2 new
	cvertex2 *first = vtxs.First();
	cnet2 *net = first->m_net;
	net->MustRedraw();								// Maybe overkill but simple.
	doc->m_dlist->StopDragging();
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->x = _x, v->y = _y;
	ReconcileVia();
}

int ctee::Draw()
{
	cvertex2 *v = vtxs.First();
	int x = v->x, y = v->y;
	if (!v) 
		return NOERR;				// Weird...
	cnetlist * nl = v->m_net->m_nlist;
	CDisplayList *dl = doc->m_dlist;
	if (!dl) 
		return NO_DLIST;
	if (bDrawn) 
		return ALREADY_DRAWN;
	// Make darn sure that none of the constituent vertices is drawn
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->Undraw();
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
	if (!dl || !IsDrawn()) return;
	for( int i=0; i<dl_els.GetSize(); i++ )
		dl->Remove( dl_els[i] );
	dl_els.RemoveAll();
	dl->Remove( dl_sel );
	dl->Remove( dl_hole );
	dl_sel = NULL;
	dl_hole = NULL;
	bDrawn = false;
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

void ctee::SetVisible( bool bVis )
{
	for( int il=0; il<dl_els.GetSize(); il++ )
		if( dl_els[il] )
			dl_els[il]->visible = bVis;
	if( dl_hole )
		dl_hole->visible = bVis;
	if (dl_thermal)
		dl_thermal->visible = bVis;
}

void ctee::StartDragging( CDC * pDC, int x, int y, int crosshair )
{
	// cancel previous selection and make incoming segments and via invisible.  Also create a DragRatlineArray for the drawing list.
	CDisplayList *dl = doc->m_dlist;
	cnet2 *net = GetNet();
	dl->CancelHighlight();
	dl->MakeDragRatlineArray( vtxs.GetSize(), 0 );
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
	{
		CPoint pi, pf = CPoint(0,0);
		if (v->preSeg)
			dl->Set_visible(v->preSeg->dl_el, 0),				// CPT2 TODO change name to SetVisible (et al)
			pi.x = v->preSeg->preVtx->x,
			pi.y = v->preSeg->preVtx->y;
		else if (v->postSeg)
			dl->Set_visible(v->postSeg->dl_el, 0),
			pi.x = v->postSeg->postVtx->x,
			pi.y = v->postSeg->postVtx->y;
		dl->AddDragRatline( pi, pf );
	}
	SetVisible( FALSE );
	dl->StartDraggingArray( pDC, 0, 0, 0, LAY_RAT_LINE );
}

void ctee::CancelDragging()
{
	// make emerging segments and via visible
	CDisplayList *dl = doc->m_dlist;
	dl->StopDragging();
	SetVisible( TRUE );
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		if (v->preSeg)
			dl->Set_visible(v->preSeg->dl_el, 1);
		else if (v->postSeg)
			dl->Set_visible(v->postSeg->dl_el, 1);
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

bool cseg2::IsValid()
{
	if (!m_net->IsValid()) return false;
	if (!m_con->IsValid()) return false;
	return m_con->segs.Contains(this);
}

// CPT:  added width param.  If this is 0 (the default) replace it with this->m_width
void cseg2::GetStatusStr( CString * str, int width )
{
	int u = doc->m_units;
	if (width==0) width = m_width;
	CString w_str;
	::MakeCStringFromDimension( &w_str, width, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
	CString s ((LPCSTR) IDS_SegmentW);
	str->Format( s, w_str );
}

void cseg2::SetConnect(cconnect2 *c)
{
	// CPT2 new.  Attach this to the given connect.  If it's already attached to a connect, detach it from the old one.  Does not alter preVtx/postVtx
	if (m_con==c) return;
	if (m_con)
		m_con->segs.Remove(this);
	c->segs.Add(this);
	m_con = c;
}

void cseg2::SetWidth( int w, int via_w, int via_hole_w )
{
	MustRedraw();
	if( m_layer != LAY_RAT_LINE && w )
		m_width = w;
	if( preVtx->via_w && via_w )
		preVtx->via_w = via_w,
		preVtx->via_hole_w = via_hole_w;
	if( postVtx->via_w && via_w )
		postVtx->via_w = via_w,
		postVtx->via_hole_w = via_hole_w;
}

int cseg2::SetLayer( int layer )
{
	// Set segment layer (must be a copper layer, not the ratline layer)
	// returns 1 if unable to comply due to SMT pad
	// CPT2 converted from old CNetList::ChangeSegmentLayer
	// check layer settings of adjacent vertices to make sure this is legal

	// check starting pad layer
	int pad_layer = preVtx->pin? preVtx->pin->pad_layer: layer;
	if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
		return 1;
	// check destination pad layer
	pad_layer = postVtx->pin? postVtx->pin->pad_layer: layer;
	if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
		return 1; 
	// change segment layer
	MustRedraw();
	m_layer = layer;
	// now adjust vias
	preVtx->ReconcileVia();
	postVtx->ReconcileVia();
	return 0;
}


void cseg2::Divide( cvertex2 *v, cseg2 *s, int dir )
{
	// Divide "this" segment into two subsegments, with v the vertex in the middle.  If dir==0,
	// new segment "s" becomes the first subseg, stretching from this->preVtx to v, and "this" 
	// becomes the second subseg, stretching from v to this->postVtx.  If dir==1, s becomes the
	// second subseg and "this" becomes the first.
	// Assumes v and s have been created with m_con members equal to this->m_con.
	m_con->MustRedraw();
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

bool cseg2::InsertSegment(int x, int y, int _layer, int _width, int dir )
{
	// CPT2, derived from CNetList::InsertSegment().  Used when "this" is a ratline that is getting partially routed.  this will be split at
	// a new vertex (position (x,y)), and either the first or second half will be converted to layer/width, depending on dir.  However, if (x,y)
	// is close to the final point of this (tolerance 10 nm), we'll just convert this to a single seg with the given layer/width and return false.
	// NB NO DRAWING in accordance with my proposed new policy.
	const int TOL = 10;													// CPT2 TODO this seems mighty small...
	cvertex2 *dest = dir==0? postVtx: preVtx;
	bool bHitDest = abs(x - dest->x) + abs(y - dest->y) < TOL;
	if (dest->pin)
		bHitDest &= dest->TestHit(x, y, _layer);						// Have to make sure layer is OK in case of SMT pins.
	if (bHitDest)
	{
		MustRedraw();
		m_layer = _layer;
		m_width = _width;
		preVtx->ReconcileVia();
		postVtx->ReconcileVia();
		return false;
	}

	cvertex2 *v = new cvertex2(m_con, x, y);
	cseg2 *s = new cseg2(m_con, _layer, _width);
	Divide(v, s, dir);													// Calls m_con->MustRedraw()
	s->preVtx->ReconcileVia();
	s->postVtx->ReconcileVia();
	return true;
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
	m_layer = layer;
	m_width = width;
	m_selected = 0;

	// now adjust vias
	preVtx->ReconcileVia();
	postVtx->ReconcileVia();
	MustRedraw();
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
			bUnroute = double(yf-yi)*(yf-(yi+dy)) <= 0;			// "double" to prevent overflow issues
		else        
			bUnroute = double(yf-yi)*((yf+dy)-yi) <= 0;
	}
	if (dy==0 && abs(yi-yf) < abs(dx/10)) 
	{
		if (end==0) 
			bUnroute = double(xf-xi)*(xf-(xi+dx)) <= 0;
		else        
			bUnroute = double(xf-xi)*((xf+dx)-xi) <= 0;
	}
	if (!bUnroute) 
		return false;

	m_layer = LAY_RAT_LINE;
	m_width = 0;

	MustRedraw();
	preVtx->ReconcileVia();
	postVtx->ReconcileVia();
	return true;
}

bool cseg2::RemoveMerge(int end)
{
	// Replaces old CNetList::RemoveSegment(), cnet::RemoveSegAndVertexByIndex(), RemoveVertexAndSegByIndex().
	// If this is a segment in the middle of a connect, combine it with its neighbor:  "end" indicates which vertex is
	//   going to get eliminated in the process (0 => remove preVtx, 1 => remove postVtx).
	// If this is an end-segment and "end" indicates the terminval vertex, then we just shorten the connect;  in other words, we call this->RemoveBreak().
	// Routine returns true if the entire connection has been destroyed because connect has just 1 segment.
	if (m_con->NumSegs()<2) 
		{ m_con->Remove(); return true; }

	if (!preVtx->preSeg && end==0 || !postVtx->postSeg && end==1)
		// Seg is at end of trace!
		return RemoveBreak();
	m_con->MustRedraw();
	m_con->segs.Remove(this);
	if (end==0) 
		m_con->vtxs.Remove(preVtx),
		preVtx->preSeg->postVtx = postVtx,
		postVtx->preSeg = preVtx->preSeg;
	else
		m_con->vtxs.Remove(postVtx),
		postVtx->postSeg->preVtx = preVtx,
		preVtx->postSeg = postVtx->postSeg;
	preVtx->ReconcileVia();
	postVtx->ReconcileVia();
	return false;
}

bool cseg2::RemoveBreak()
{
	// Replaces old cnet::RemoveSegmentAdjustTees().  Remove segment from its connect, and if it's in the middle somewhere then we'll have
	// to split the old connect into 2.  If it's at an end point, just remove the segment, plus check for tees at the end point and adjust as needed.
	// Return true if the entire connect is destroyed, otherwise return false.
	if (m_con->NumSegs()<2) 
		{ m_con->Remove(); return true; }
	
	m_con->MustRedraw();
	m_con->segs.Remove(this);
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
	newCon->MustRedraw();
	for (cvertex2 *v = postVtx; 1; v = v->postSeg->postVtx)
	{
		v->SetConnect(newCon);
		if (!v->postSeg) break;
		v->postSeg->SetConnect(newCon);
	}
	newCon->head = postVtx;
	newCon->tail = m_con->tail;
	m_con->tail = preVtx;
	postVtx->preSeg = preVtx->postSeg = NULL;
	preVtx->ReconcileVia();
	postVtx->ReconcileVia();

	ctee *headT = m_con->head->tee;
	if (headT && newCon->tail->tee == headT)
		// It's just possible that the original connect was a circle.  Having removed "this", it's now two connects with a single tee uniting their 
		// endpoints.  By calling Adjust() on that tee we'll get the two connects united into a single line.
		headT->Adjust();
	return false;
}

void cseg2::StartDragging( CDC * pDC, int x, int y, int layer1, int layer2, int w, 
								   int layer_no_via, int dir, int crosshair )
{
	// cancel previous selection and make segment invisible
	CDisplayList *dl = doc->m_dlist;
	dl->CancelHighlight();
	dl->Set_visible(dl_el, 0);
	// CPT2 TODO:  Rationalize the whole 3-layer business in the arg list:
	dl->StartDraggingLineVertex( pDC, x, y, preVtx->x, preVtx->y, postVtx->x, postVtx->y, 
								layer1,	layer2, w, 1, DSS_STRAIGHT, DSS_STRAIGHT, 
								layer_no_via, w*3/2, w, dir, crosshair );		// CPT r295 put in approximate args for via_w and via_hole_w.  This
																				// whole business seems a bit bogus to me...
}

void cseg2::CancelDragging()
{
	// make segment visible
	CDisplayList *dl = doc->m_dlist;
	dl->Set_visible(dl_el, 1);
	dl->StopDragging();
}

void cseg2::StartDraggingNewVertex( CDC * pDC, int x, int y, int layer, int w, int crosshair )
{
	// Start dragging new vertex in existing trace segment.  CPT2 converted from CNetList func.
	// cancel previous selection and make segment invisible
	CDisplayList *dl = doc->m_dlist;
	dl->CancelHighlight();
	dl->Set_visible(dl_el, 0);
	// start dragging
	dl->StartDraggingLineVertex( pDC, x, y, preVtx->x, preVtx->y, postVtx->x, postVtx->y, 
								layer, layer, w, w, DSS_STRAIGHT, DSS_STRAIGHT, 
								layer, 0, 0, 0, crosshair );
}

void cseg2::CancelDraggingNewVertex()
{
	// CPT2 converted from CNetList func.
	// make segment visible
	CDisplayList *dl = doc->m_dlist;
	dl->Set_visible(dl_el, 1);
	dl->StopDragging();
}

void cseg2::StartMoving( CDC * pDC, int x, int y, int crosshair )
{
	// cancel previous selection and make segments and via invisible
	cseg2 *preSeg = preVtx->preSeg, *postSeg = postVtx->postSeg;
	CDisplayList *dl = doc->m_dlist;
	dl->CancelHighlight();
	dl->Set_visible(dl_el, 0);
	preVtx->SetVisible( FALSE );
	postVtx->SetVisible( FALSE );
	if (preSeg)
		dl->Set_visible(preSeg->dl_el, 0);
	if (postSeg)
		dl->Set_visible(postSeg->dl_el, 0);

	int xb = 0, yb = 0;
	if (preSeg)
		xb = preSeg->preVtx->x,
		yb = preSeg->preVtx->y;
	int xi = preVtx->x;
	int yi = preVtx->y;
	int xf = postVtx->x;
	int yf = postVtx->y;
	int xe = 0, ye = 0;
	if (postSeg)
		xe = postSeg->postVtx->x,
		ye = postSeg->postVtx->y;
	int layer0 = 0, layer1, layer2 = 0;
	int w0 = 0, w1, w2 = 0;
	if (preSeg)
		layer0 = preSeg->m_layer,
		w0 = preSeg->m_width;
	layer1 = m_layer;
	w1 = m_width;
	if (postSeg)
		layer2 = postSeg->m_layer,
		w2 = postSeg->m_width;
	dl->StartDraggingLineSegment( pDC, x, y, xb, yb, xi, yi, xf, yf, xe, ye,
								layer0, layer1, layer2,
								w0,		w1,		w2,
								preSeg? DSS_STRAIGHT: DSS_NONE, DSS_STRAIGHT, postSeg? DSS_STRAIGHT: DSS_NONE,
								0, 0, 0, 
								crosshair );
}

void cseg2::CancelMoving()
{
	// CPT2 derived from CNetList::CancelMovingSegment()
	// make segments and via visible
	CDisplayList *dl = doc->m_dlist;
	cseg2 *preSeg = preVtx->preSeg, *postSeg = postVtx->postSeg;
	if (preSeg)
		dl->Set_visible(preSeg->dl_el, 1);
	dl->Set_visible(dl_el, 1);
	if (postSeg)
		dl->Set_visible(postSeg->dl_el, 1);
	preVtx->SetVisible(true);
	postVtx->SetVisible(true);
	dl->StopDragging();
}


int cseg2::Draw()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) 
		return NO_DLIST;
	if (bDrawn) 
		return ALREADY_DRAWN;
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
	bDrawn = true;
	return NOERR;
}

int cseg2::DrawWithEndpoints()
{
	int err = Draw();
	if (err!=NOERR) return err;
	if (preVtx->tee)
		preVtx->tee->Draw();
	else
		preVtx->Draw();
	if (postVtx->tee)
		postVtx->tee->Draw();
	else
		postVtx->Draw();
	return NOERR;
}

void cseg2::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl || !bDrawn) return;
	dl->Remove( dl_el );
	dl->Remove( dl_sel );
	dl_el = NULL;
	dl_sel = NULL;
	bDrawn = false;
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

void cseg2::SetVisible( bool bVis )
{
	doc->m_dlist->Set_visible( dl_el, bVis );
}


cconnect2::cconnect2( cnet2 * _net ) 
	: cpcb_item (_net->doc)
{
	m_net = _net;
	m_net->connects.Add(this);
	locked = 0;
}

bool cconnect2::IsValid()
	{ return m_net->connects.Contains(this); }

void cconnect2::GetStatusStr( CString * str )
{
	CString net_str, type_str, locked_str, from_str, to_str;
	m_net->GetStatusStr( &net_str );
	if( NumSegs() == 1 && head->postSeg->m_layer == LAY_RAT_LINE )
		type_str.LoadStringA(IDS_Ratline);
	else
		type_str.LoadStringA(IDS_Trace);
	locked_str = "";
	if( head->pin && tail->pin && locked)
		locked_str = " (L)";
	head->GetTypeStatusStr( &from_str );
	tail->GetTypeStatusStr( &to_str );
	CString s ((LPCSTR) IDS_FromTo);
	str->Format( s, net_str, type_str, from_str, to_str, locked_str );
}

void cconnect2::SetWidth( int w, int via_w, int via_hole_w )
{
	citer<cseg2> is (&segs);
	for (cseg2 *s = is.First(); s; s = is.Next())
		s->SetWidth(w, via_w, via_hole_w);
}

void cconnect2::Remove()
{
	// CPT2. User wants to delete connection, so detach it from the network (garbage collector will later delete this object and its
	// constituents for good).
	// Any tee structures attached at either end get cleaned up.
	Undraw();
	m_net->connects.Remove(this);
	if (head->tee && head->tee==tail->tee)
		// Connect is a loop, tricky special case.  The "false" argument below prevents 
		// the calling of ctee::Adjust(), which might cause a crash
		head->tee->Remove(head, false);					
	else if (head->tee)
		head->tee->Remove(head);
	if (tail->tee)
		tail->tee->Remove(tail);
}


void cconnect2::Start( cvertex2 *v )
{
	// CPT2 new.  After this routine, connect will consist of a single vertex (v) and no segs.
	MustRedraw();
	vtxs.RemoveAll();
	segs.RemoveAll();
	vtxs.Add(v);
	head = tail = v;
	v->preSeg = v->postSeg = NULL;
}

void cconnect2::AppendSegAndVertex( cseg2 *s, cvertex2 *v, cvertex2 *after )
{
	// Append s+v into this connection after vertex "after".  Assumes that s+v were constructed so that they point to "this"
	MustRedraw();
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
	MustRedraw();
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
void cconnect2::AppendSegment( int x, int y, int layer, int width )
{
	// add new vertex and segment at end of connect.  NB does not do any drawing or undrawing
	cseg2 *s = new cseg2 (this, layer, width);
	cvertex2 *v = new cvertex2 (this, x, y);
	AppendSegAndVertex( s, v, tail );
}

void cconnect2::PrependSegment( int x, int y, int layer, int width )
{
	// Similar to AppendSegment, but inserts at the connect's beginning
	cseg2 *s = new cseg2 (this, layer, width);
	cvertex2 *v = new cvertex2 (this, x, y);
	PrependVertexAndSeg( v, s );
}

void cconnect2::ReverseDirection()
{
	// CPT2.  I'm going to see if it works not to call MustRedraw().
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
	MustRedraw();
	c2->Undraw();
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
	v1->force_via_flag |= v2->force_via_flag;
	v1->via_w = max(v1->via_w, v2->via_w);
	v1->via_hole_w = max(v1->via_hole_w, v2->via_hole_w);
}

void cconnect2::MergeUnroutedSegments() 
{
	if (segs.GetSize() < 2) return;
	MustRedraw();
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
}

int cconnect2::Draw()
{
	// Draw individual constituents.  The bDrawn flag for the object itself is probably irrelevant.
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return NO_DLIST;

	citer<cseg2> is (&segs);
	for (cseg2 *s = is.First(); s; s = is.Next())
		s->Draw();
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
	{
		v->Draw();												// CPT2. NB does NOT reconcile vias (assumes it's been done)
		if (v->tee) 
			v->tee->Draw();
	}
	bDrawn = true;
	return NOERR;
}

void cconnect2::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl || !IsDrawn() ) return;
	citer<cseg2> is (&segs);
	for (cseg2 *s = is.First(); s; s = is.Next())
		s->Undraw();
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->Undraw();
	bDrawn = false;
}

void cconnect2::Highlight()
{
	// Highlight connection, using thin lines so that segment layer colors can be seen.
	citer<cseg2> is (&segs);
	for (cseg2 *s = is.First(); s; s = is.Next())
		s->Highlight(true);
	// Highlight all vtxs (this will take care of tees and pins too)
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->Highlight();

}

/**********************************************************************************************/
/*  RELATED TO cpin2/cpart2                                                                     */
/**********************************************************************************************/

cpin2::cpin2(cpart2 *_part, cpadstack *_ps, cnet2 *_net)					// CPT2. Added args
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
	dl_hole = dl_thermal = NULL;
	bNeedsThermal = false;
}

bool cpin2::IsValid()
{
	if (!part->IsValid()) return false;
	return part->pins.Contains(this);
}

void cpin2::Disconnect() 
{
	// Detach pin from its part, and whichever net it's attached to.  Rip out any attached connections completely.
	part->pins.Remove(this);
	part = NULL;
	if (!net) return;
	citer<cconnect2> ic (&net->connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
		if (c->head->pin == this || c->tail->pin == this)
			c->Remove();
	net->pins.Remove(this);
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

void cpin2::GetVtxs(carray<cvertex2> *vtxs)
{
	// CPT2 new.  Fill "vtxs" with the vertices that are associated with this pin
	vtxs->RemoveAll();
	if (!net) return;
	citer<cconnect2> ic (&net->connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
	{
		if (c->head->pin == this)
			vtxs->Add(c->head);
		if (c->tail->pin == this)
			vtxs->Add(c->tail);
	}
}

bool cpin2::TestHit( int _x, int _y, int _layer )
{
	// CPT2 Derived from CPartList::TestHitOnPad
	if( !part )
		return FALSE;
	if( !part->shape )
		return FALSE;

	cpad * p;
	if( ps->hole_size == 0 )
	{
		// SMT pad
		if( _layer == LAY_TOP_COPPER && part->side == 0 )
			p = &ps->top;
		else if( _layer == LAY_BOTTOM_COPPER && part->side == 1 )
			p = &ps->top;
		else
			return FALSE;
	}
	else
	{
		// TH pad
		if( _layer == LAY_TOP_COPPER && part->side == 0 )
			p = &ps->top;
		else if( _layer == LAY_TOP_COPPER && part->side == 1 )
			p = &ps->bottom;
		else if( _layer == LAY_BOTTOM_COPPER && part->side == 1 )
			p = &ps->top;
		else if( _layer == LAY_BOTTOM_COPPER && part->side == 0 )
			p = &ps->bottom;
		else
			p = &ps->inner;
	}
	double dx = abs( x - _x );
	double dy = abs( y - _y );
	double dist = sqrt( dx*dx + dy*dy );
	if( dist < ps->hole_size/2 )
		return true;

	switch( p->shape )
	{
	case PAD_NONE: 
		return false;
	case PAD_ROUND: 
		return dist < (p->size_h/2);
	case PAD_SQUARE:
		return dx < (p->size_h/2) && dy < (p->size_h/2);
	case PAD_RECT:
	case PAD_RRECT:
	case PAD_OVAL:
		int pad_angle = part->angle + ps->angle;
		if( pad_angle > 270 )
			pad_angle -= 360;
		if( pad_angle == 0 || pad_angle == 180 )
			return dx < (p->size_l) && dy < (p->size_h/2);
		else
			return dx < (p->size_h/2) && dy < (p->size_l);
	}
	return false;
}


void cpin2::SetPosition()												
{
	// CPT2 New, but derived from CPartList::GetPinPoint().
	// Derives correct values for this->(x,y) based on the part's position and on the padstack position.
	// NB Doesn't call MustRedraw().
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

	SetNeedsThermal();
}

bool cpin2::SetNeedsThermal()
{
	// CPT2 new, but related to the old CNetList::SetAreaConnections.  Sets bNeedsThermal to true if some net area overlaps this pin
	bool bOldVal = bNeedsThermal;
	bNeedsThermal = false;
	if (net) 
	{
		citer<carea2> ia (&net->areas);
		for (carea2 *a = ia.First(); a; a = ia.Next())
			if (a->TestPointInside(x, y))
				{ bNeedsThermal = true; break; }
	}
	if (bNeedsThermal!=bOldVal)
		MustRedraw();
	return bNeedsThermal;
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

cseg2 *cpin2::AddRatlineToPin( cpin2 *p2 )
{
	// Add new connection to net, consisting of one unrouted segment
	// from this to p2.
	// CPT2 Adapted from old cnet::AddConnectFromPinToPin.  NOW RETURNS THE RATLINE SEG, AND DOES NO DRAWING.
	cpin2 *p1 = this;
	if (!p1->part || !p2->part)
		return NULL;								// Seems mighty unlikely.
	if (!p1->part->shape || !p2->part->shape)
		return NULL;								// Ditto

	// create connection with a single vertex
	cconnect2 *c = new cconnect2 (net);
	cvertex2 *v1 = new cvertex2 (c, p1->x, p1->y);
	c->Start(v1);
	c->AppendSegment(p2->x, p2->y, LAY_RAT_LINE, 0);
	v1->pin = p1;
	c->tail->pin = p2;
	return c->head->postSeg;
}


cpart2::cpart2( cpartlist * pl )			// CPT2 TODO.  Will probably add more args...
	: cpcb_item (pl->m_doc)
{ 
	m_pl = pl;
	if (pl)
		pl->parts.Add(this);
	x = y = side = angle = 0;
	glued = false;
	m_ref_vis = true;
	m_ref = new creftext(this, 0, 0, 0, false, false, 0, 0, 0, doc->m_smfontutil, &CString(""));
	m_value_vis = false;
	m_value = new cvaluetext(this, 0, 0, 0, false, false, 0, 0, 0, doc->m_smfontutil, &CString(""));
	m_tl = new ctextlist(doc);
	shape = NULL;
}

bool cpart2::IsValid()
	{ return doc->m_plist->parts.Contains(this); }

void cpart2::Move( int _x, int _y, int _angle, int _side )
{
	MustRedraw();
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
}

// Part moved, so unroute starting and ending segments of connections
// to this part, and update positions of endpoints
// Undraw and Redraw any changed connections
// CPT:  added dx and dy params indicating how much the part moved (both are 1 by default).  If, say, dx==0
// and an attached seg is vertical, then we don't have to unroute it.
// CPT2:  Derived from CNetList::PartMoved.

void cpart2::PartMoved( int dx, int dy )
{
	// first, mark all nets unmodified
	doc->m_nlist->nets.SetUtility(0);

	// find nets that connect to this part, and unroute segments adjacent to pins.  
	// CPT2 I'll let the subroutines take care of calling MustUndraw() for the appropriate objects.
	citer<cpin2> ip (&pins);
	for (cpin2 *pin = ip.First(); pin; pin = ip.Next())
	{
		cnet2 *net = pin->net;
		if (!net || net->utility) continue;
		citer<cconnect2> ic (&net->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
		{
			if (c->segs.GetSize()==0) continue;				// Unlikely but hey...
			if (c->head->pin && c->head->pin->part == this)
			{
				cvertex2 *head = c->head;
				head->postSeg->Unroute( dx, dy, 0 );
				head->x = head->pin->x;
				head->y = head->pin->y;
				net->utility = 1;	// mark net modified
				// CPT2 TODO decide if it's really safe to dump the following:
				/*
				if( part->shape->m_padstack[pin_index1].hole_size )
					// through-hole pad
					v0->pad_layer = LAY_PAD_THRU;
				else if( part->side == 0 && part->shape->m_padstack[pin_index1].top.shape != PAD_NONE
					|| part->side == 1 && part->shape->m_padstack[pin_index1].bottom.shape != PAD_NONE )
					// SMT pad on top
					v0->pad_layer = LAY_TOP_COPPER;
				else
					// SMT pad on bottom
					v0->pad_layer = LAY_BOTTOM_COPPER;
				if( part->pin[pin_index1].net != net )
					part->pin[pin_index1].net = net;
				*/
			}
			if (c->tail->pin && c->tail->pin->part == this)
			{
				// end pin is on part, unroute last segment
				cvertex2 *tail = c->tail;
				tail->preSeg->Unroute( dx, dy, 1 );
				tail->x = tail->pin->x;
				tail->y = tail->pin->y;
				net->utility = 1;	// mark net modified
				// CPT2 TODO similar to issue in previous if-statement
			}
		}
	}
}

void cpart2::Remove(bool bEraseTraces, bool bErasePart)
{
	// CPT2 mostly new.  Implements my new system for deleting parts, where one can either erase all traces emanating from it, or not.
	// Also added bErasePart param (true by default);  calling Remove(true, false) allows one to delete attached traces only.
	if (bErasePart)
		Undraw(),
		m_pl->parts.Remove(this);

	// Figure out which nets are going to be affected by this part's disappearance, and mark 'em for redrawing
	m_pl->m_nlist->nets.SetUtility(0);
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
		if (pin->net)
			pin->net->utility = 1,
			pin->net->MustRedraw();

	// Now go through the pins again.  If bEraseTraces, rip out connects attached to each one.  Otherwise, gather a list of vertices associated
	// with each one (such vertices will later get united into a tee).  TODO for each pin, consider maintaining a list of vtxs associated with it
	// at all times?
	carray<cvertex2> vtxs;
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		if (!pin->net) continue;
		if (bEraseTraces)
		{
			citer<cconnect2> ic (&pin->net->connects);
			for (cconnect2 *c = ic.First(); c; c = ic.Next())
				if (c->head->pin==pin || c->tail->pin==pin)
					c->Remove();
		}
		else
		{
			// Make a list of vertices that are attached to the current pin.
			pin->GetVtxs(&vtxs);
			if (vtxs.GetSize()==0) 
				continue;
			if (vtxs.GetSize()==1)
			{
				// 1 vertex on this pin.  If the incoming seg is a ratline, erase it.  Otherwise, keep it and just clear the vtx's "pin" value.
				cvertex2 *v = vtxs.First();
				if (v->preSeg && v->preSeg->m_layer == LAY_RAT_LINE)
					v->preSeg->RemoveBreak();
				else if (v->postSeg && v->postSeg->m_layer == LAY_RAT_LINE)
					v->postSeg->RemoveBreak();
				else
					v->pin = NULL;
			}
			// 2+ vertices on this pin.  Go thru the list, null out their "pin" values, and assign them instead to a new tee structure.
			ctee *tee = new ctee(pin->net);
			citer<cvertex2> iv (&vtxs);
			for (cvertex2 *v = iv.First(); v; v = iv.Next())
				v->pin = NULL,
				tee->Add(v);
			tee->Adjust();
		}
	}
}

void cpart2::SetData(CShape * _shape, CString * _ref_des, CString * _value_text, CString * _package, 
					 int _x, int _y, int _side, int _angle, int _visible, int _glued  )
{
	// Derived from old CPartList::SetPartData.  Initializes data members, including "pins".  Ref and value-text related
	// members are initialized according to _shape if possible.
	// CPT2 Delete the following?
	// CDisplayList * old_dlist = m_dlist;
	// m_dlist = NULL;		// cancel further drawing

	// now copy data into part
	MustRedraw();
	visible = _visible;
	ref_des = *_ref_des;
	m_ref->m_str = *_ref_des;
	if (_value_text)
		value_text = m_value->m_str = *_value_text;
	else
		value_text = m_value->m_str = "";
	if( _package )
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
		m_ref->Move(0, 0, 0, 0, 0);
		m_value->Move(0, 0, 0, 0, 0);
	}
	else
	{
		shape = _shape;
		InitPins();
		Move( x, y, angle, side );	// force setting pin positions

		ctext *txt = shape->m_ref;
		int layer = cpartlist::FootprintLayer2Layer( txt->m_layer );
		m_ref->Move( txt->m_x, txt->m_y, txt->m_angle, 
			false, false, layer, txt->m_font_size, txt->m_stroke_width );
		txt = shape->m_value;
		layer = cpartlist::FootprintLayer2Layer( txt->m_layer );
		m_value->Move( txt->m_x, txt->m_y, txt->m_angle, 
			false, false, layer, txt->m_font_size, txt->m_stroke_width );
		citer<ctext> it (&shape->m_tl->texts);
		for (txt = it.First(); txt; txt = it.Next())
		{
			layer = cpartlist::FootprintLayer2Layer( txt->m_layer );
			m_tl->AddText(txt->m_x, txt->m_y, txt->m_angle, txt->m_bMirror, txt->m_bNegative, 
							layer, txt->m_font_size, txt->m_stroke_width, &txt->m_str);
		}
	}

	m_outline_stroke.SetSize(0);
	m_ref->m_stroke.SetSize(0);
	m_value->m_stroke.SetSize(0);
}

void cpart2::SetValue(CString *_value, int x, int y, int angle, int size, int w, 
						 BOOL vis, int layer )
{
	value_text = *_value;
	m_value->m_layer = layer;
	m_value->Move(x, y, angle, size, w);
	m_value_vis = vis;
}

// Move ref text with given id to new position and angle
// x and y are in absolute world coords
// angle is relative to part angle
// CPT2 derived from CPartList function.  "_angle", "_size", and "_w" are now -1 by default (=> no change)
// TODO consolidate with value-text functions.
void cpart2::MoveRefText( int _x, int _y, int _angle, int _size, int _w )
{
	// get position of new text box origin relative to part
	CPoint part_org, tb_org;
	tb_org.x = _x - x;
	tb_org.y = _y - y;
	
	// correct for rotation of part
	RotatePoint( &tb_org, 360-angle, zero );
	
	// correct for part on bottom of board (reverse relative x-axis)
	if( side == 1 )
		tb_org.x = -tb_org.x;
	
	// reset ref text parameters
	m_ref->MustRedraw();
	m_ref->m_x = tb_org.x;
	m_ref->m_y = tb_org.y;
	if (_angle!=-1) 
		m_ref->m_angle = _angle % 360;
	if (_size!=-1)
		m_ref->m_font_size = _size;
	if (_w!=-1)
		m_ref->m_stroke_width = _w;
}

void cpart2::ResizeRefText(int size, int width, BOOL vis )
{
	m_ref->Move(m_ref->m_x, m_ref->m_y, m_ref->m_angle, size, width);
	m_ref_vis = vis;
}

// Move value text with given id to new position and angle
// x and y are in absolute world coords
// angle is relative to part angle
// CPT2 derived from CPartList function.  "_angle", "_size", and "_w" are now -1 by default (=> no change)
void cpart2::MoveValueText( int _x, int _y, int _angle, int _size, int _w )
{
	// get position of new text box origin relative to part
	CPoint part_org, tb_org;
	tb_org.x = _x - x;
	tb_org.y = _y - y;
	
	// correct for rotation of part
	RotatePoint( &tb_org, 360-angle, zero );
	
	// correct for part on bottom of board (reverse relative x-axis)
	if( side == 1 )
		tb_org.x = -tb_org.x;
	
	// reset value text parameters
	m_value->MustRedraw();
	m_value->m_x = tb_org.x;
	m_value->m_y = tb_org.y;
	if (_angle!=-1) 
		m_value->m_angle = _angle % 360;
	if (_size!=-1)
		m_value->m_font_size = _size;
	if (_w!=-1)
		m_value->m_stroke_width = _w;
}


void cpart2::InitPins()
{
	// CPT2 New.  Initialize pins carray based on the padstacks in the shape.
	ASSERT(shape);
	citer<cpadstack> ips (&shape->m_padstack);
	for (cpadstack *ps = ips.First(); ps; ps = ips.Next())
		pins.Add( new cpin2(this, ps, NULL) );
}

cpin2 *cpart2::GetPinByName(CString *name)
{
	// CPT2 new.
	citer<cpin2> ip (&pins);
	for (cpin2 *p = ip.First(); p; p = ip.Next())
		if (p->pin_name == *name)
			return p;
	return NULL;
}

CPoint cpart2::GetRefPoint()
{
	CPoint ref_pt;

	// move origin of text box to position relative to part
	ref_pt.x = m_ref->m_x;
	ref_pt.y = m_ref->m_y;
	// flip if part on bottom
	if( side )
		ref_pt.x = -ref_pt.x;
	// rotate with part about part origin
	RotatePoint( &ref_pt, angle, zero );
	ref_pt.x += x;
	ref_pt.y += y;
	return ref_pt;
}


CPoint cpart2::GetValuePoint()
{
	CPoint val_pt;
	// move origin of text box to position relative to part
	val_pt.x = m_value->m_x;
	val_pt.y = m_value->m_y;
	// flip if part on bottom
	if( side )
		val_pt.x = -val_pt.x;
	// rotate with part about part origin
	RotatePoint( &val_pt, angle, zero );
	val_pt.x += x;
	val_pt.y += y;
	return val_pt;
}


// get bounding rect of value text relative to part origin 
// works even if part isn't drawn
//
CRect cpart2::GetValueRect()
{
	m_value->GenerateStrokes();
	return m_value->m_br;
}

int cpart2::GetBoundingRect( CRect * part_r )
{
	CRect r;
	if( !shape || !dl_sel ) 
		return 0;
	CDisplayList *dl = doc->m_dlist;
	r.left = min( dl->Get_x( dl_sel ), dl->Get_xf( dl_sel ) );
	r.right = max( dl->Get_x( dl_sel ), dl->Get_xf( dl_sel ) );
	r.bottom = min( dl->Get_y( dl_sel ), dl->Get_yf( dl_sel ) );
	r.top = max( dl->Get_y( dl_sel ), dl->Get_yf( dl_sel ) );
	*part_r = r;
	return 1;
}

void cpart2::FootprintChanged(CShape *_shape)
{
	// CPT2.  Loosely derived from CPartList::PartFootprintChanged && CNetList::PartFootprintChanged.
	// Setup pins corresponding to the new shape, reusing old pins where possible.  Mark used pins with
	// a utility value of 1.  Pins that are unused at the end get axed.
	MustRedraw();
	carray<cnet2> nets;									// Maintain a list of attached nets so that we can redraw them afterwards
	citer<cpin2> ip (&pins);
	for (cpin2 *p = ip.First(); p; p = ip.Next())
		if (p->net)
			nets.Add(p->net),
			p->net->MustRedraw();

	shape = _shape;
	pins.SetUtility(0);
	int cPins = shape->m_padstack.GetSize();
	carray<cvertex2> vtxs;
	citer<cpadstack> ips (&shape->m_padstack);
	for (cpadstack *ps = ips.First(); ps; ps = ips.Next())
	{
		cpin2 *old = GetPinByName(&ps->name);
		cpin2 *p;
		int oldX, oldY, oldLayer;
		if (old)
		{
			oldX = old->x, oldY = old->y;
			oldLayer = old->pad_layer;
			old->ps = ps;
			p = old;
			if( ps->hole_size ) 
				p->pad_layer = LAY_PAD_THRU;
			else if( side == 0 && ps->top.shape != PAD_NONE || side == 1 && ps->bottom.shape != PAD_NONE )
				p->pad_layer = LAY_TOP_COPPER;
			else
				p->pad_layer = LAY_BOTTOM_COPPER;
			p->SetPosition();
			bool bLayerChange = p->pad_layer!=oldLayer && p->pad_layer!=LAY_PAD_THRU;
			if (oldX != p->x || oldY != p->y || bLayerChange)
			{
				// Pin's position has changed.  Find vertices associated with it, and unroute their adjacent segs as needed
				p->GetVtxs(&vtxs);
				citer<cvertex2> iv (&vtxs);
				for (cvertex2 *v = iv.First(); v; v = iv.Next())
					if (v->preSeg && v->preSeg->m_layer!=LAY_RAT_LINE)
						v->preSeg->Unroute();
					else if (v->postSeg && v->postSeg->m_layer!=LAY_RAT_LINE)
						v->postSeg->Unroute();
			}
		}
		else
			p = new cpin2(this, ps, NULL),
			p->SetPosition();
		p->utility = 1;
	}

	// Look for disused pins (utility==0) and dump 'em.  NB Disconnect() rips out all attached traces completely.  TODO change or allow user an option to 
	// keep traces?
	for (cpin2 *p = ip.First(); p; p = ip.Next())
		if (!p->utility) 
			p->Disconnect();

	// Dump texts from the old footprint and add texts from the new one:
	m_tl->RemoveAllTexts();
	citer<ctext> it (&shape->m_tl->texts);
	for (ctext *txt = it.First(); txt; txt = it.Next()) 
	{
		int layer = cpartlist::FootprintLayer2Layer( txt->m_layer );
		m_tl->AddText(txt->m_x, txt->m_y, txt->m_angle, txt->m_bMirror, txt->m_bNegative, 
							layer, txt->m_font_size, txt->m_stroke_width, &txt->m_str);
	}
}

void cpart2::OptimizeConnections(BOOL bBelowPinCount, int pin_count, BOOL bVisibleNetsOnly )
{
	if (!shape) return;

	// find nets which connect to this part, and mark them all unoptimized
	citer<cpin2> ip (&pins);
	for (cpin2 *pin = ip.First(); pin; pin = ip.Next())
		if (pin->net)
			pin->net->utility = 0;

	// optimize each net and mark it optimized so it won't be repeated
	for (cpin2 *pin = ip.First(); pin; pin = ip.Next())
		if (pin->net && !pin->net->utility)
			pin->net->OptimizeConnections(bBelowPinCount, pin_count, bVisibleNetsOnly),
			pin->net->utility = 1;
}

int cpart2::Draw()
{
	// CPT2 TODO:  Think about the CDisplayList arg business
	CDisplayList *dl = doc->m_dlist;
	if( !dl )
		return NO_DLIST;
	if( !shape )
		return NO_FOOTPRINT;
	if (bDrawn)
		return ALREADY_DRAWN;

	// draw selection rectangle (layer = top or bottom copper, depending on side)
	CRect sel;
	if( !side )
	{
		// part on top
		sel.left = shape->m_sel_xi; 
		sel.right = shape->m_sel_xf;
		sel.bottom = shape->m_sel_yi;
		sel.top = shape->m_sel_yf;
	}
	else
	{
		// part on bottom
		sel.right = - shape->m_sel_xi;
		sel.left = - shape->m_sel_xf;
		sel.bottom = shape->m_sel_yi;
		sel.top = shape->m_sel_yf;
	}
	if( angle > 0 )
		RotateRect( &sel, angle, zero );
	dl_sel = dl->AddSelector( this, LAY_SELECTION, DL_HOLLOW_RECT, 1,
		0, 0, x + sel.left, y + sel.bottom, x + sel.right, y + sel.top, x, y );
	dl->Set_sel_vert( dl_sel, 0 );													// CPT2 TODO function should be in dl_element
	if( angle == 90 || angle ==  270 )
		dl->Set_sel_vert( dl_sel, 1 );

	// draw ref designator text
	m_ref->dl_el = m_ref->dl_sel = NULL;
	if( m_ref_vis && m_ref->m_font_size )
		m_ref->DrawRelativeTo(this);
	else
		m_ref->Undraw();
	
	// draw value text
	m_value->dl_el = m_value->dl_sel = NULL;
	if( m_value_vis && m_value->m_font_size )
		m_value->DrawRelativeTo(this);
	else
		m_value->Undraw();

	// draw part outline (code similar to but sadly not identical to cpolyline::Draw())
	CPoint si, sf;
	m_outline_stroke.SetSize(0);
	citer<coutline> ip (&shape->m_outline_poly);
	for (cpolyline *poly = ip.First(); poly; poly = ip.Next())
	{
		int shape_layer = poly->GetLayer();
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
					s->m_style = STRAIGHT;
				int g_type = DL_LINE;
				if( s->m_style == STRAIGHT )
					g_type = DL_LINE;
				else if( s->m_style == ARC_CW )
					g_type = DL_ARC_CW;
				else if( s->m_style == ARC_CCW )
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

	// draw footprint texts
	citer<ctext> it (&m_tl->texts);
	for (ctext *t = it.First(); t; t = it.Next())
		t->DrawRelativeTo(this, false);

	// draw padstacks and save absolute position of pins
	CPoint pin_pt;
	CPoint pad_pi;
	CPoint pad_pf;
	int nLayers = doc->m_plist->m_layers;
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next()) 
	{
		// CPT2 TODO check:  is pin->x/y always getting set correctly?
		// set layer for pads
		cpadstack * ps = pin->ps;
		pin->dl_els.SetSize(nLayers);
		cpad * p;
		int pad_layer;
		// iterate through all copper layers 
		for( int il=0; il<nLayers; il++ )
		{
			pin_pt.x = ps->x_rel;
			pin_pt.y = ps->y_rel;
			pad_layer = il + LAY_TOP_COPPER;
			pin->dl_els[il] = NULL;
			// get appropriate pad
			cpad * p = NULL;
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
				pin->dl_sel = dl->AddSelector( pin, LAY_SELECTION, 
					DL_HOLLOW_RECT, 1, 1, 0,
					pin->x-ps->hole_size/2, pin->y-ps->hole_size/2,  
					pin->x+ps->hole_size/2, pin->y+ps->hole_size/2,  
					0, 0 );
		}
		else
			pin->dl_hole = NULL;

		if (pin->bNeedsThermal)
			pin->dl_thermal = dl->Add( this, dl_element::DL_OTHER, LAY_RAT_LINE, DL_X, 1,
				2*ps->hole_size/3, 0, 0, pin->x, pin->y, 0, 0, 0, 0 );
		else
			pin->dl_thermal = NULL;
	}

	bDrawn = true;
	return NOERR;
}

void cpart2::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl || !IsDrawn() ) return;
	if (!shape)
		{ bDrawn = false; return; }

	// undraw selection rectangle
	dl->Remove( dl_sel );
	dl_sel = 0;

	// undraw ref text and value text
	m_ref->Undraw();
	m_value->Undraw();

	// undraw part outline (this also includes footprint free text)
	for( int i=0; i<m_outline_stroke.GetSize(); i++ )
	{
		dl->Remove( m_outline_stroke[i].dl_el );
		m_outline_stroke[i].dl_el = 0;
	}

	// undraw footprint texts
	citer<ctext> it (&m_tl->texts);
	for (ctext *t = it.First(); t; t = it.Next())
		t->Undraw();

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
		dl->Remove( pin->dl_thermal );
		pin->dl_hole = pin->dl_sel = pin->dl_thermal = NULL;
	}

	bDrawn = false;
}

void cpart2::Highlight( )
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	if (!shape) return;
	dl->Highlight( DL_HOLLOW_RECT, 
				dl->Get_x( dl_sel ), dl->Get_y( dl_sel ),
				dl->Get_xf( dl_sel ), dl->Get_yf( dl_sel ), 1 );

	// Also highlight ref and value texts if possible
	if (dl_element *ref_sel = m_ref->dl_sel)
		dl->Highlight( DL_HOLLOW_RECT, 
				dl->Get_x( ref_sel ), dl->Get_y( ref_sel ),
				dl->Get_xf( ref_sel ), dl->Get_yf( ref_sel ), 1 );
	if (dl_element *val_sel = m_value->dl_sel)
		dl->Highlight( DL_HOLLOW_RECT, 
				dl->Get_x( val_sel ), dl->Get_y( val_sel ),
				dl->Get_xf( val_sel ), dl->Get_yf( val_sel ), 1 );
	
}

void cpart2::SetVisible(bool bVisible)
{
	// CPT2, derived from CPartList::MakePartVisible()
	// Make part visible or invisible, including thermal reliefs
	// outline strokes
	for( int i=0; i<m_outline_stroke.GetSize(); i++ )
	{
		dl_element * el = m_outline_stroke[i].dl_el;
		el->visible = bVisible;
	}
	// pins
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		// pin pads
		if (pin->dl_hole)
			pin->dl_hole->visible = bVisible;
		for( int i=0; i < pin->dl_els.GetSize(); i++ )
			if( pin->dl_els[i] )
				pin->dl_els[i]->visible = bVisible;
		if (pin->dl_thermal)
			pin->dl_thermal->visible = bVisible;
	}
	m_ref->SetVisible( bVisible );
	m_value->SetVisible( bVisible );
	// CPT2 TODO also set visibility for texts within footprint
}


void cpart2::StartDragging( CDC * pDC, BOOL bRatlines, BOOL bBelowPinCount, int pin_count ) 
{
	// CPT2, derived from CPartList::StartDraggingPart
	// make part invisible
	CDisplayList *dl = doc->m_dlist;
	SetVisible( FALSE );
	dl->CancelHighlight();

	// create drag lines
	CPoint zero(0,0);
	dl->MakeDragLineArray( 2*pins.GetSize() + 4 );
	int xi = shape->m_sel_xi;
	int xf = shape->m_sel_xf;
	if( side )
		xi = -xi,
		xf = -xf;
	int yi = shape->m_sel_yi;
	int yf = shape->m_sel_yf;
	CPoint p1( xi, yi );
	CPoint p2( xf, yi );
	CPoint p3( xf, yf );
	CPoint p4( xi, yf );
	RotatePoint( &p1, angle, zero );
	RotatePoint( &p2, angle, zero );
	RotatePoint( &p3, angle, zero );
	RotatePoint( &p4, angle, zero );
	dl->AddDragLine( p1, p2 ); 
	dl->AddDragLine( p2, p3 ); 
	dl->AddDragLine( p3, p4 ); 
	dl->AddDragLine( p4, p1 );
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		// CPT2 old code used to recalculate pin's x/y-position.  I'm going to try relying on the pin's preexisting x/y member values.  Note
		// that these values must be made relative to the part origin, however.
		// make X for each pin
		int d = pin->ps->top.size_h/2;
		int xRel = pin->x - x, yRel = pin->y - y;
		CPoint p1 (xRel - d, yRel - d);
		CPoint p2 (xRel + d, yRel - d);
		CPoint p3 (xRel + d, yRel + d);
		CPoint p4 (xRel - d, yRel + d);
		// draw X
		dl->AddDragLine( p1, p3 ); 
		dl->AddDragLine( p2, p4 );
	}

	// create drag lines for ratlines (OR OTHER SEGS --- CPT2) connected to pins
	if( bRatlines ) 
	{
		dl->MakeDragRatlineArray( 2*pins.GetSize(), 1 );
		// CPT2 Make a list of segs that have an endpoint (or 2) attached to one of the part's pins.  For efficiency, don't scan any net more than once
		m_pl->m_nlist->nets.SetUtility(0);
		carray<cseg2> segs;
		for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
		{
			if (!pin->net) continue;
			if (pin->net->utility) continue;
			pin->net->utility = 1;
			citer<cconnect2> ic (&pin->net->connects);
			for (cconnect2 *c = ic.First(); c; c = ic.Next())
			{
				if (c->head->pin && c->head->pin->part == this)
					segs.Add(c->head->postSeg);
				if (c->tail->pin && c->tail->pin->part == this)
					segs.Add(c->tail->preSeg);
			}
		}
		// Loop through the new list of segs that are attached to part's pins:
		citer<cseg2> is (&segs);
		for (cseg2 *s = is.First(); s; s = is.Next())
		{
			s->SetVisible(false);
			// CPT2 old code spent effort worrying about hiding vias on the end of the seg opposite from the pin.  
			// I'm going to try ignoring that issue...
			// As in the old code, I'll suppress drag-ratlines if the seg has both ends attached to the part's pins.  The old code
			// suppressed segs that belonged to single-seg connects with a loose end.  I'm going to try ignoring that issue too:
			CPoint vtxPt, pinPt;
			if (s->preVtx->pin && s->preVtx->pin->part==this)
				if (s->postVtx->pin && s->postVtx->pin->part==this)
					continue;												// Both ends of s are pins on "this"...
				else
					vtxPt = CPoint (s->postVtx->x, s->postVtx->y),
					pinPt = CPoint (s->preVtx->x - x, s->preVtx->y - y);	// NB coords relative to part
			else if (s->postVtx->pin && s->postVtx->pin->part==this)
				vtxPt = CPoint (s->preVtx->x, s->preVtx->y),
				pinPt = CPoint (s->postVtx->x - x, s->postVtx->y - y);		// NB coords relative to part
			dl->AddDragRatline( vtxPt, pinPt );
		}
	}
	
	int vert = 0;
	if( angle == 90 || angle == 270 )
		vert = 1;
	dl->StartDraggingArray( pDC, x, y, vert, LAY_RAT_LINE );				// CPT2, was LAY_SELECTION: came out invisible (at least with my color settings)
}

void cpart2::CancelDragging()
{
	// CPT2 derived from CPartList::CancelDraggingPart()
	// make part visible again
	SetVisible(true);

	// get any connecting segments and make visible
	CDisplayList *dl = doc->m_dlist;
	m_pl->m_nlist->nets.SetUtility(0);
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		if (!pin->net || !pin->net->bVisible || pin->net->utility)
			continue;
		pin->net->utility = 1;
		citer<cconnect2> ic (&pin->net->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
		{
			if (c->head->pin && c->head->pin->part==this)
				c->head->postSeg->SetVisible(true);
			if (c->tail->pin && c->tail->pin->part==this)
				c->tail->preSeg->SetVisible(true);
		}
	}
	dl->StopDragging();
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

bool ccorner::IsValid() 
{
	if (!contour->poly->IsValid()) return false;
	if (!contour->IsValid()) return false;
	return contour->corners.Contains(this);
}

bool ccorner::IsAreaCorner() { return contour->poly->IsArea(); }
bool ccorner::IsBoardCorner() { return contour->poly->IsBoard(); }
bool ccorner::IsSmCorner() { return contour->poly->IsSmCutout(); }
bool ccorner::IsOutlineCorner() { return contour->poly->IsOutline(); }
cnet2 *ccorner::GetNet() { return contour->GetNet(); }
int ccorner::GetLayer() { return contour->poly->m_layer; }
cpolyline *ccorner::GetPolyline() { return contour->poly; }


int ccorner::GetTypeBit() 
{														// Later:  put in .cpp file.  There wouldn't be this nonsense in Java...
	if (contour->poly->IsArea()) return bitAreaCorner;
	if (contour->poly->IsSmCutout()) return bitSmCorner;
	if (contour->poly->IsBoard()) return bitBoardCorner;
	return bitOther;
}

bool ccorner::IsOnCutout()
	// Return true if this side lies on a secondary contour within the polyline
	{ return contour->poly->main!=contour; }

void ccorner::Remove() 
{
	// CPT2 derived loosely from old CPolyLine::DeleteCorner
	cpolyline *poly = GetPolyline();
	poly->MustRedraw();
	contour->corners.Remove(this);
	if (contour->NumCorners() < 2)
		contour->Remove();
	else if (contour->NumCorners() == 2 && contour->head==contour->tail)
		contour->Remove();
	else if (!preSide)
	{
		// First corner of open contour
		contour->sides.Remove(postSide);
		contour->head = postSide->postCorner;
		contour->head->preSide = NULL;
	}
	else if (!postSide)
	{
		// Final corner of open contour
		contour->sides.Remove(preSide);
		contour->tail = preSide->preCorner;
		contour->tail->postSide = NULL;
	}
	else 
	{
		// Normal middle corner
		ccorner *next = postSide->postCorner;
		if (this == contour->head)
			contour->head = contour->tail = next;
		contour->sides.Remove(postSide);
		preSide->postCorner = next;
		next->preSide = preSide;
	}
}

void ccorner::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	if( !dl_sel ) return;
	dl->Highlight( DL_HOLLOW_RECT, 
		dl->Get_x( dl_sel ), dl->Get_y( dl_sel ),
		dl->Get_xf( dl_sel ), dl->Get_yf( dl_sel ),
		dl->Get_w( dl_sel ) );
}

bool ccorner::Move( int _x, int _y, BOOL bEnforceCircularArcs )
{
	// move corner of polyline
	// if bEnforceCircularArcs == TRUE, convert adjacent sides to STRAIGHT if angle not
	// a multiple of 45 degrees and return TRUE, otherwise return FALSE
	// CPT2 derived from CPolyLine::MoveCorner().
	cpolyline *poly = contour->poly;
	poly->MustRedraw();
	x = _x;
	y = _y;
	BOOL bReturn = FALSE;
	if( bEnforceCircularArcs )
	{
		ccorner *prev = preSide? preSide->preCorner: NULL;
		ccorner *next = postSide? postSide->postCorner: NULL;
		if (prev && abs(prev->x - x) != abs(prev->y - y))
			preSide->m_style = STRAIGHT,
			bReturn = TRUE;
		if (next && abs(next->x - x) != abs(next->y - y))
			postSide->m_style = STRAIGHT,
			bReturn = TRUE;
	}
	return bReturn;
}

void ccorner::StartDragging( CDC * pDC, int x, int y, int crosshair )
{
	CDisplayList *dl = doc->m_dlist;
	ASSERT(dl);
	dl->CancelHighlight();
	// Hide adjacent sides (if present) and area hatching
	if (preSide)
		dl->Set_visible(preSide->dl_el, 0);
	if (postSide)
		dl->Set_visible(postSide->dl_el, 0);
	carea2 *a = GetArea();
	for( int ih=0; ih < a->m_nhatch; ih++ )
		dl->Set_visible( a->dl_hatch[ih], 0 );

	// see if corner is the first or last corner of an open contour
	if (!preSide || !postSide)
	{
		int style, xi, yi;
		cside *side;
		if (!preSide)
			style = postSide->m_style==ARC_CW? ARC_CCW: ARC_CW,		// Reverse arc since we are drawing from corner 1 to 0
			xi = postSide->postCorner->x,
			yi = postSide->postCorner->y,
			side = postSide;
		else
			style = preSide->m_style,
			xi = preSide->preCorner->x,
			yi = preSide->preCorner->y,
			side = preSide;
		dl->StartDraggingArc( pDC, style, this->x, this->y, xi, yi, LAY_SELECTION, 1, crosshair );
	}
	else
	{
		int style1, style2;
		if( preSide->m_style == STRAIGHT )
			style1 = DSS_STRAIGHT;
		else if( preSide->m_style == ARC_CW )
			style1 = DSS_ARC_CW;
		else
			style1 = DSS_ARC_CCW;
		if( postSide->m_style == STRAIGHT )
			style2 = DSS_STRAIGHT;
		else if( postSide->m_style == ARC_CW )
			style2 = DSS_ARC_CW;
		else
			style2 = DSS_ARC_CCW;
		int xi = preSide->preCorner->x;
		int yi = preSide->preCorner->y;
		int xf = postSide->postCorner->x;
		int yf = postSide->postCorner->y;
		dl->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, 
			LAY_RAT_LINE, LAY_RAT_LINE,							// CPT2: Used to have LAY_SELECTION instead of LAY_RAT_LINE, sometimes invisible as a result 
			1, 1, style1, style2, 0, 0, 0, 0, crosshair );
	}
}

void ccorner::CancelDragging()
{
	// CPT2:  Derived from CPolyLine::CancelDraggingToMoveCorner.  Stop dragging, make sides and hatching visible again
	CDisplayList *dl = doc->m_dlist;
	ASSERT( dl );
	dl->StopDragging();
	if (preSide)
		dl->Set_visible(preSide->dl_el, 1);
	if (postSide)
		dl->Set_visible(postSide->dl_el, 1);
	carea2 *a = GetArea();
	for( int ih=0; ih < a->m_nhatch; ih++ )
		dl->Set_visible( a->dl_hatch[ih], 1 );
}


cside::cside(ccontour *_contour, int _style)
	: cpcb_item(_contour->doc)
{ 
	contour = _contour;
	contour->sides.Add(this);
	m_style = _style;
	preCorner = postCorner = NULL;
}

bool cside::IsValid() 
{
	if (!contour->poly->IsValid()) return false;
	if (!contour->IsValid()) return false;
	return contour->sides.Contains(this);
}

bool cside::IsAreaSide() { return contour->poly->IsArea(); }
bool cside::IsBoardSide() { return contour->poly->IsBoard(); }
bool cside::IsSmSide() { return contour->poly->IsSmCutout(); }
bool cside::IsOutlineSide() { return contour->poly->IsOutline(); }
int cside::GetLayer() { return contour->poly->m_layer; }

int cside::GetTypeBit() 
{
	if (contour->poly->IsArea()) return bitAreaSide;
	if (contour->poly->IsSmCutout()) return bitSmSide;
	if (contour->poly->IsBoard()) return bitBoardSide;
	return bitOther;
}

cnet2 *cside::GetNet() { return contour->GetNet(); }

cpolyline *cside::GetPolyline() { return contour->poly; }

bool cside::IsOnCutout()
	// Return true if this side lies on a secondary contour within the polyline
	{ return contour->poly->main!=contour; }

void cside::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	if( !dl_sel ) return;
	int s;
	if( m_style == CPolyLine::STRAIGHT )
		s = DL_LINE;
	else if( m_style == CPolyLine::ARC_CW )
		s = DL_ARC_CW;
	else if( m_style == CPolyLine::ARC_CCW )
		s = DL_ARC_CCW;
	dl->Highlight( s, 
		dl->Get_x( dl_sel ), dl->Get_y( dl_sel ),
		dl->Get_xf( dl_sel ), dl->Get_yf( dl_sel ),
		dl->Get_w( dl_sel ) );
}

void cside::InsertCorner(int x, int y)
{
	// CPT2 new.  Add an intermediate corner into this side.  "this" gets reused as the second of the 2 half-sides, and the style of both are made straight.
	ccorner *c = new ccorner(contour, x, y);
	cside *s = new cside(contour, STRAIGHT);
	m_style = STRAIGHT;
	contour->AppendSideAndCorner(s, c, preCorner);
}

void cside::StartDraggingNewCorner( CDC * pDC, int x, int y, int crosshair )
{
	// CPT2, derived from CPolyLine::StartDraggingToInsertCorner
	CDisplayList *dl = doc->m_dlist;
	ASSERT( dl );

	int xi = preCorner->x, yi = preCorner->y;
	int xf = postCorner->x, yf = postCorner->y;
	dl->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, 
		LAY_RAT_LINE, LAY_RAT_LINE, 1, 1, DSS_STRAIGHT, DSS_STRAIGHT,
		0, 0, 0, 0, crosshair );
	dl->CancelHighlight();
	dl->Set_visible( dl_el, 0 );
	cpolyline *p = GetPolyline();
	for( int ih=0; ih < p->m_nhatch; ih++ )
		dl->Set_visible( p->dl_hatch[ih], 0 );
}

//
void cside::CancelDraggingNewCorner()
{
	// CPT2 derived from CPolyLine::CancelDraggingToInsertCorner
	CDisplayList *dl = doc->m_dlist;
	ASSERT( dl );

	dl->StopDragging();
	dl->Set_visible( dl_el, 1 );
	cpolyline *p = GetPolyline();
	for( int ih=0; ih < p->m_nhatch; ih++ )
		dl->Set_visible( p->dl_hatch[ih], 1 );
}





ccontour::ccontour(cpolyline *_poly, bool bMain)
	: cpcb_item (_poly->doc)
{
	poly = _poly;
	if (bMain) 
		poly->main = this;
	poly->contours.Add(this);
	head = tail = NULL;
}

ccontour::ccontour(cpolyline *_poly, ccontour *src)
	: cpcb_item (_poly->doc)
{
	// Create a new contour with the same points/sides as "src", but belonging to poly "_poly"
	poly = _poly;
	poly->contours.Add(this);
	corners.RemoveAll();
	sides.RemoveAll();
	head = tail = NULL;
	if (src->corners.GetSize()==0) 
		return;

	// Loop thru src's corners and sides in geometrical order.  Must take care since src's head and tail may be the same.
	cside *preSide = NULL;
	for (ccorner *c = src->head; 1; c = c->postSide->postCorner)
	{
		ccorner *c2 = new ccorner(this, c->x, c->y);
		corners.Add(c2);
		c2->preSide = preSide;
		if (preSide)
			preSide->postCorner = c2;
		if (c==src->head) 
			head = c2;
		cside *s = c->postSide;
		if (!s) 
			{ tail = c2; break; }
		cside *s2 = new cside(this, s->m_style);
		sides.Add(s2);
		c2->postSide = s2;
		s2->preCorner = c2;
		if (s->postCorner == src->head)
		{ 
			tail = head; 
			tail->preSide = s2; s2->postCorner = tail; 
			break; 
		}
		preSide = s2;
	}
}

bool ccontour::IsValid() 
	{ return poly->contours.Contains(this); }

cnet2 *ccontour::GetNet() { return poly->GetNet(); }

int ccontour::GetLayer() { return poly->m_layer; }

void ccontour::AppendSideAndCorner( cside *s, ccorner *c, ccorner *after )
{
	// Append s+c into this connection after corner "after".  Assumes that s+c were constructed so that they point to "this"
	// Very similar to cconnect2::AppendSegAndVertex.
	if (poly)
		poly->MustRedraw();
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
	if (poly)
		poly->MustRedraw();
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

void ccontour::SetPoly( cpolyline *_poly )
{
	// CPT2 new.  Reassign this to a different polyline as a secondary contour (used when creating new cutouts).    
	if (poly)
		poly->MustRedraw(),
		poly->contours.Remove(this);
	poly = _poly;
	poly->contours.Add(this);
	poly->MustRedraw();
}

void ccontour::Remove()
{
	if (this ==  poly->main)
		poly->Remove();
	else
		poly->contours.Remove(this);
}


cpolyline::cpolyline(CFreePcbDoc *_doc)
	: cpcb_item (_doc)
{ 
	main = NULL;
	m_layer = m_w = m_sel_box = m_hatch = m_nhatch = 0;
	m_gpc_poly = new gpc_polygon;
	m_gpc_poly->num_contours = 0;
	m_php_poly = NULL;			// CPT2 TODO for now. 
}

cpolyline::cpolyline(cpolyline *src, bool bCopyContours, bool bTemporary)
	: cpcb_item (src->doc)
{
	main = NULL;
	m_layer = src->m_layer;
	m_w = src->m_w;
	m_sel_box = src->m_sel_box;
	m_hatch = src->m_hatch;
	m_nhatch = src->m_nhatch;					// CPT2.  I guess...
	m_gpc_poly = NULL, m_php_poly = NULL;
	m_bTemporary = bTemporary;
	if (!src->main || !bCopyContours)
		return;
	citer<ccontour> ictr (&src->contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		ccontour *ctr2 = new ccontour(this, ctr);
		contours.Add(ctr2);
		if (ctr==src->main) 
			main = ctr2;
	}
}

void cpolyline::MarkConstituents()
{
	// Mark the utility flags on this polyline and on its constituent contours, sides, and corners.
	utility = 1;
	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		ctr->utility = 1;
		ctr->sides.SetUtility(1);
		ctr->corners.SetUtility(1);
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

// Test a polyline for self-intersection.
// Returns:
//	-1 if arcs intersect other sides
//	 0 if no intersecting sides
//	 1 if intersecting sides, but no intersecting arcs
// Also sets utility2 flag of area with return value
//
int cpolyline::TestPolygon()
{	
	ASSERT(IsClosed());

	// first, check for sides intersecting other sides, especially arcs 
	BOOL bInt = FALSE;
	BOOL bArcInt = FALSE;
	// make bounding rect for each contour.  CPT2 for each contour, store an index into "cr" in the utility field.
	CArray<CRect> cr;
	int i=0;
	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		CRect r = ctr->GetCornerBounds();
		cr.Add(r);
		ctr->utility = i++;
	}
	for (ccontour *ctr1 = ictr.First(); ctr1; ctr1 = ictr.Next())
	{
		citer<cside> is1 (&ctr1->sides);
		for (cside *s1 = is1.First(); s1; s1 = is1.Next())
		{
			CRect r1 = cr[ctr1->utility];
			cside *s1prev = s1->preCorner->preSide;
			cside *s1next = s1->postCorner->postSide;
			int style1 = s1->m_style;
			int x1i = s1->preCorner->x, y1i = s1->preCorner->y;
			int x1f = s1->postCorner->x, y1f = s1->postCorner->y;
			citer<ccontour> ictr2 (&contours, ctr1);
			for (ccontour *ctr2 = ictr2.Next(); ctr2; ctr2 = ictr2.Next())
			{
				CRect r2 = cr[ctr2->utility];
				if( r1.left > r2.right
					|| r1.bottom > r2.top
					|| r2.left > r1.right
					|| r2.bottom > r1.top )
						// rectangles don't overlap, do nothing
						continue;
				citer<cside> is2 (&ctr2->sides);
				for (cside *s2 = is2.First(); s2; s2 = is2.Next())
				{
					if (s1==s2 || s1prev==s2 || s1next==s2) continue;
					int style2 = s2->m_style;
					int x2i = s2->preCorner->x, y2i = s2->preCorner->y;
					int x2f = s2->postCorner->x, y2f = s2->postCorner->y;
					int ret = FindSegmentIntersections( x1i, y1i, x1f, y1f, style1, x2i, y2i, x2f, y2f, style2 );
					if( !ret ) continue;
					// intersection between non-adjacent sides
					bInt = TRUE;
					if( style1 != CPolyLine::STRAIGHT || style2 != CPolyLine::STRAIGHT )
					{
						bArcInt = TRUE;
						goto finish;
					}
				}
			}
		}
	}

	finish:
	if( bArcInt )
		utility2 = -1;
	else if( bInt )
		utility2 = 1;
	else 
		utility2 = 0;
	return utility2;
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


int cpolyline::Draw()
{
	int i_start_contour = 0;
	CDisplayList *dl = doc->m_dlist;
	if (!dl) 
		return NO_DLIST;
	if (bDrawn) 
		return ALREADY_DRAWN;

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
					1, m_w*4, 0, xi, yi, xf, yf, 0, 0 );				// CPT2 Quadrupled the selector width for visibility when highlighting
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

	bDrawn = false;
}

void cpolyline::Highlight()
{
	citer<ccontour> ic (&contours);
	for (ccontour *c = ic.First(); c; c = ic.Next())
		c->Highlight();
}

int cpolyline::TestIntersection( cpolyline *poly2, bool bCheckArcIntersections )
{
	// Test for intersection of 2 copper areas
	// returns: 0 if no intersection
	//			1 if intersection
	//			2 if arcs intersect.  But if bCheckArcIntersections is false, return 1 as soon as any intersection whatever is found
	// CPT2 adapted from CNetList::TestAreaIntersection.  By adding the bCheckArcIntersections param, this covers CNetList::TestAreaIntersections too
	// TODO I don't think this will work if poly2 is totally within this.  We might consider using 
	//   the CMyBitmap class, or gpc?
	// First see if polygons are on same layer
	cpolyline *poly1 = this;
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
	citer<ccontour> ictr1 (&poly1->contours);
	for (ccontour *ctr1 = ictr1.First(); ctr1; ctr1 = ictr1.Next())
	{
		citer<cside> is1 (&ctr1->sides);
		for (cside *s1 = is1.First(); s1; s1 = is1.Next())
		{
			int xi1 = s1->preCorner->x, yi1 = s1->preCorner->y;
			int xf1 = s1->postCorner->x, yf1 = s1->postCorner->y;
			int style1 = s1->m_style;
			citer<ccontour> ictr2 (&poly2->contours);
			for (ccontour *ctr2 = ictr2.First(); ctr2; ctr2 = ictr2.Next())
			{
				citer<cside> is2 (&ctr2->sides);
				for (cside *s2 = is2.First(); s2; s2 = is2.Next())
				{
					int xi2 = s2->preCorner->x, yi2 = s2->preCorner->y;
					int xf2 = s2->postCorner->x, yf2 = s2->postCorner->y;
					int style2 = s2->m_style;
					int n_int = FindSegmentIntersections( xi1, yi1, xf1, yf1, style1,
									xi2, yi2, xf2, yf2, style2 );
					if( n_int )
					{
						bInt = TRUE;
						if (!bCheckArcIntersections)
							return 1;
						if( style1 != CPolyLine::STRAIGHT || style2 != CPolyLine::STRAIGHT )
							return 2;
						break;
					}
				}
			}
		}
	}

	if( !bInt )
		return 0;
	// An intersection found, but no arc-intersections:
	return 1;
}

int cpolyline::CombinePolyline( cpolyline *poly2 )
{
	// CPT2. Adapted from CNetList::CombineAreas.  Unions "poly2" onto "this" polyline.  Does not deal with the removal of poly2 from the net in
	// the case that this and poly2 are copper areas:
	// that's the caller's job.  Assumes that the intersection check has already taken place.
	// Returns 1 if we actually combine 'em, or 0 in the unusual cases where they're actually disjoint.
	cpolyline * poly1 = this;
	CArray<CArc> arc_array1;
	CArray<CArc> arc_array2;
	poly1->MakeGpcPoly( NULL, &arc_array1 );
	poly2->MakeGpcPoly( NULL, &arc_array2 );
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

	// if no intersection, free new gpc and return.  Somewhat unlikely since an intersection check has usually already occurred, but if areas
	// meet at a single point it could happen.
	if( n_union_ext_cont == n_ext_cont1 + n_ext_cont2 )
	{
		gpc_free_polygon( union_gpc );
		delete union_gpc;
		return 0;
	}

	// intersection, as expected.  Replace poly1/this with combined area
	poly1->MustRedraw();
	poly2->Undraw();
	contours.RemoveAll();
	for( int ic=0; ic<union_gpc->num_contours; ic++ )
	{
		ccontour *ctr = new ccontour(this, !union_gpc->hole[ic]);		// NB the new contour will be this->main if the current gpc contour is a non-hole
		for( int i=0; i<union_gpc->contour[ic].num_vertices; i++ )
		{
			int x = union_gpc->contour[ic].vertex[i].x;
			int y = union_gpc->contour[ic].vertex[i].y;
			ccorner *c = new ccorner(ctr, x, y);			// Constructor adds corner to ctr->corners (and may also set ctr->head/tail if
															// it was previously empty)
			if (i>0)
			{
				cside *s = new cside(ctr, STRAIGHT);
				ctr->AppendSideAndCorner(s, c, ctr->tail);
			}
		}
		ctr->Close(STRAIGHT);
	}

	utility = 1;
	RestoreArcs( &arc_array1 ); 
	RestoreArcs( &arc_array2 );
	gpc_free_polygon( union_gpc );
	delete union_gpc;
	return 1;
}

void cpolyline::RestoreArcs( CArray<CArc> *arc_array, carray<cpolyline> *pa )
{
	// Restore arcs to a polygon where they were replaced with steps
	// If pa != NULL, also use polygons in pa array
	// CPT2 converted.
	carray<cpolyline> pa2;
	pa2.Add(this);
	if (pa)
		pa2.Add(pa);

	// find arcs and replace them
	BOOL bFound;
	int arc_start;
	int arc_end;
	for( int iarc=0; iarc<arc_array->GetSize(); iarc++ )
	{
		int arc_xi = (*arc_array)[iarc].xi;
		int arc_yi = (*arc_array)[iarc].yi;
		int arc_xf = (*arc_array)[iarc].xf;
		int arc_yf = (*arc_array)[iarc].yf;
		int n_steps = (*arc_array)[iarc].n_steps;
		int style = (*arc_array)[iarc].style;
		ccorner *arc_start = NULL, *arc_end = NULL;
		// loop through polys
		citer<cpolyline> ip (&pa2);
		for (cpolyline *p = ip.First(); p; p = ip.Next())
		{
			citer<ccontour> ictr (&p->contours);
			for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
			{
				if (ctr->corners.GetSize() <= n_steps) continue;
				citer<ccorner> ic (&ctr->corners);
				for (ccorner *c = ic.First(); c; c = ic.Next())
				{
					if (c->x != arc_xi || c->y != arc_yi) continue;
					// Check if we find a corner at location (arc_xf,arc_yf), either n_steps positions forward from c or n_steps positions back
					ccorner *c2 = c;
					for (int i=0; i<n_steps; i++)
						c2 = c2->postSide->postCorner;
					if (c2->x == arc_xf && c2->y == arc_yf) 
					{
						arc_start = c; arc_end = c2;
						goto arc_found;
					}
					c2 = c;
					for (int i=0; i<n_steps; i++)
						c2 = c2->preSide->preCorner;
					if (c2->x == arc_xf && c2->y == arc_yf)
					{
						arc_start = c2; arc_end = c;
						style = style==ARC_CW? ARC_CCW: ARC_CW;						// Arc has been reversed, so flip cw & ccw
						goto arc_found;
					}
				}
			}
		}
		continue;

		arc_found:
		(*arc_array)[iarc].bFound = TRUE;											// Necessary?
		ccontour *ctr = arc_start->contour;
		ctr->poly->MustRedraw();
		for (ccorner *c = arc_start; c != arc_end; c = c->postSide->postCorner)
		{
			if (c != arc_start) 
				ctr->corners.Remove(c);
			ctr->sides.Remove(c->postSide);
		}
		cside *new_side = new cside(ctr, style);
		arc_start->postSide = new_side;
		new_side->preCorner = arc_start;
		arc_end->preSide = new_side;
		new_side->postCorner = arc_end;
	}
}


#define pi  3.14159265359

void TestGpc()
{
	// CPT2 TODO.  For testing only.
	gpc_polygon * gpc = new gpc_polygon;
	gpc->num_contours = 0;
	gpc->hole = NULL;
	gpc->contour = NULL;
	gpc_polygon * gpc2 = new gpc_polygon;
	gpc2->num_contours = 0;
	gpc2->hole = NULL;
	gpc2->contour = NULL;
	gpc_vertex_list * g_v_list = new gpc_vertex_list;
	g_v_list->vertex = (gpc_vertex*)calloc( sizeof(gpc_vertex), 6 );
	g_v_list->num_vertices = 6;
	static int x[] = { 100, 200, 100, 100, 0,   100 };
	static int y[] = { 100, 0,   0,   100, 200, 200 };
	for (int i=0; i<6; i++)
		g_v_list->vertex[i].x = x[i],
		g_v_list->vertex[i].y = y[i];
	// add vertex_list to gpc
	gpc_add_contour( gpc, g_v_list, 0 );
	// now clip m_gpc_poly with gpc, put new poly into result
	gpc_polygon * result = new gpc_polygon;
	gpc_polygon_clip( GPC_UNION, gpc2, gpc, result );
}

void cpolyline::MakeGpcPoly( ccontour *ctr0, CArray<CArc> * arc_array )
{
	// make a gpc_polygon for a closed polyline contour
	// approximates arcs with multiple straight-line segments
	// if ctr0 == NULL, make polygon with all contours,
	// combining intersecting contours if possible.
	// Otherwise just do the one contour.  (Kind of an ungainly system, but hey...)
	// returns data on arcs in arc_array
	if( m_gpc_poly->num_contours )
		FreeGpcPoly();
	if (ctr0 && ctr0->head!=ctr0->tail)
		return;														// Open contour, error (indicated by the absence of an allocated gpc-poly)
	if (!ctr0 && !IsClosed())
		return;														// Main contour is open, error

	// initialize m_gpc_poly
	m_gpc_poly->num_contours = 0;
	m_gpc_poly->hole = NULL;
	m_gpc_poly->contour = NULL;
	int n_arcs = 0;
	if( arc_array )
		arc_array->SetSize(0);
	int iarc = 0;

	// Create a temporary carray of contours in which the external (main) contour is guaranteed to come first:
	carray<ccontour> tmp;
	tmp.Add(main);
	citer<ccontour> ictr0 (&contours);
	for (ccontour *ctr = ictr0.First(); ctr; ctr = ictr0.Next())
		if (ctr!=main)
			tmp.Add(ctr);
	// Now go thru the contours in the preferred order.
	citer<ccontour> ictr (&tmp);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		if (ctr0 && ctr!=ctr0) 
			continue;
		// make gpc_polygon for this contour
		gpc_polygon * gpc = new gpc_polygon;
		gpc->num_contours = 0;
		gpc->hole = NULL;
		gpc->contour = NULL;

		ASSERT( ctr->sides.GetSize()>0 );
		// first, calculate number of vertices in contour (including extra vertices inserted into arcs)
		int n_vertices = 0;
		citer<cside> is (&ctr->sides);
		for (cside *s = is.First(); s; s = is.Next())
		{
			if (s->m_style == STRAIGHT)
				n_vertices++;
			else
			{
				// Arc!
				int x1 = s->preCorner->x, y1 = s->preCorner->y;
				int x2 = s->postCorner->x, y2 = s->postCorner->y;
				int n;	// number of steps for arcs
				n = (abs(x2-x1)+abs(y2-y1))/(CArc::MAX_STEP);
				n = max( n, CArc::MIN_STEPS );	// or at most 5 degrees of arc
				n_vertices += n;
				n_arcs++;
			}
		}

		// now create gpc_vertex_list for this contour
		gpc_vertex_list * g_v_list = new gpc_vertex_list;
		g_v_list->vertex = (gpc_vertex*)calloc( sizeof(gpc_vertex), n_vertices );
		g_v_list->num_vertices = n_vertices;
		int ivtx = 0;
		ccorner *c = ctr->head;
		do
		{
			int style = c->postSide->m_style;
			int x1 = c->x, y1 = c->y;
			int x2 = c->postSide->postCorner->x, y2 = c->postSide->postCorner->y;
			if( style == STRAIGHT )
			{
				g_v_list->vertex[ivtx].x = x1;
				g_v_list->vertex[ivtx].y = y1;
				ivtx++;
			}
			else
			{
				// style is arc_cw or arc_ccw
				int n = (abs(x2-x1)+abs(y2-y1))/(CArc::MAX_STEP);	// number of steps for arcs
				n = max( n, CArc::MIN_STEPS );						// or at most 5 degrees of arc
				double xo, yo, theta1, theta2, a, b;
				a = fabs( (double)(x1 - x2) );
				b = fabs( (double)(y1 - y2) );
				if( style == CPolyLine::ARC_CW )
				{
					// clockwise arc (ie.quadrant of ellipse)
					int i=0, j=0;
					if( x2 > x1 && y2 > y1 )
					{
						// first quadrant, draw second quadrant of ellipse
						xo = x2;	
						yo = y1;
						theta1 = pi;
						theta2 = pi/2.0;
					}
					else if( x2 < x1 && y2 > y1 )
					{
						// second quadrant, draw third quadrant of ellipse
						xo = x1;	
						yo = y2;
						theta1 = 3.0*pi/2.0;
						theta2 = pi;
					}
					else if( x2 < x1 && y2 < y1 )	
					{
						// third quadrant, draw fourth quadrant of ellipse
						xo = x2;	
						yo = y1;
						theta1 = 2.0*pi;
						theta2 = 3.0*pi/2.0;
					}
					else
					{
						xo = x1;	// fourth quadrant, draw first quadrant of ellipse
						yo = y2;
						theta1 = pi/2.0;
						theta2 = 0.0;
					}
				}
				else
				{
					// counter-clockwise arc
					int i=0, j=0;
					if( x2 > x1 && y2 > y1 )
					{
						xo = x1;	// first quadrant, draw fourth quadrant of ellipse
						yo = y2;
						theta1 = 3.0*pi/2.0;
						theta2 = 2.0*pi;
					}
					else if( x2 < x1 && y2 > y1 )
					{
						xo = x2;	// second quadrant
						yo = y1;
						theta1 = 0.0;
						theta2 = pi/2.0;
					}
					else if( x2 < x1 && y2 < y1 )	
					{
						xo = x1;	// third quadrant
						yo = y2;
						theta1 = pi/2.0;
						theta2 = pi;
					}
					else
					{
						xo = x2;	// fourth quadrant
						yo = y1;
						theta1 = pi;
						theta2 = 3.0*pi/2.0;
					}
				}
				// now write steps for arc
				if( arc_array )
				{
					arc_array->SetSize(iarc+1);
					(*arc_array)[iarc].style = style;
					(*arc_array)[iarc].n_steps = n;
					(*arc_array)[iarc].xi = x1;
					(*arc_array)[iarc].yi = y1;
					(*arc_array)[iarc].xf = x2;
					(*arc_array)[iarc].yf = y2;
					iarc++;
				}
				for( int is=0; is<n; is++ )
				{
					double theta = theta1 + ((theta2-theta1)*(double)is)/n;
					double x = xo + a*cos(theta);
					double y = yo + b*sin(theta);
					if( is == 0 )
					{
						x = x1;
						y = y1;
					}
					g_v_list->vertex[ivtx].x = x;
					g_v_list->vertex[ivtx].y = y;
					ivtx++;
				}
			}
			c = c->postSide->postCorner;
		} while (c != ctr->head);
		ASSERT( n_vertices == ivtx );

		// add vertex_list to gpc
		gpc_add_contour( gpc, g_v_list, 0 );
		// now clip m_gpc_poly with gpc, put new poly into result
		gpc_polygon * result = new gpc_polygon;
		if( !ctr0 && ctr!=main )
			gpc_polygon_clip( GPC_DIFF, m_gpc_poly, gpc, result );	// hole
		else
			gpc_polygon_clip( GPC_UNION, m_gpc_poly, gpc, result );	// outside
		// now copy result to m_gpc_poly
		gpc_free_polygon( m_gpc_poly );
		delete m_gpc_poly;
		m_gpc_poly = result;
		gpc_free_polygon( gpc );
		delete gpc;
		free( g_v_list->vertex );
		free( g_v_list );
	}
}

void cpolyline::FreeGpcPoly()
{
	if( m_gpc_poly->num_contours )
	{
		delete m_gpc_poly->contour->vertex;
		delete m_gpc_poly->contour;
		delete m_gpc_poly->hole;
	}
	m_gpc_poly->num_contours = 0;
}



carea2::carea2(cnet2 *_net, int _layer, int _hatch, int _w, int _sel_box)
	: cpolyline(_net->doc)
{ 
	m_layer = _layer;
	m_hatch = _hatch;
	m_net = _net;
	m_net->areas.Add(this);
	m_w = _w;
	m_sel_box = _sel_box;
}

bool carea2::IsValid()
{
	if (!m_net->IsValid()) return false;
	return m_net->areas.Contains(this);
}

void carea2::Remove()
{
	// CPT2. User wants to delete area, so detach it from the network (garbage collector will later delete this object and its
	// constituents for good).
	Undraw();
	m_net->areas.Remove(this);
	m_net->SetThermals();
}

void carea2::SetNet( cnet2 *net )
{
	// CPT2 new.  Assign this area to the given net.  Does not deal with potential changes to thermals.
	if (m_net) 
		m_net->areas.Remove(this);
	m_net = net;
	m_net->areas.Add(this);
}

bool carea2::TestIntersections()
{
	// CPT2.  Covers old CNetList::TestAreaIntersections().  Returns true if "this" intersects any other area within the same net.
	// Invokes cpolyline::TestIntersection() with bTestArcIntersections set to false for efficiency.
	citer<carea2> ia (&m_net->areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
	{
		if (a==this) continue;
		if (TestIntersection(a, false)) return true;
	}
	return false;
}

void carea2::NormalizeWithGpc( carray<carea2> *pa, bool bRetainArcs )
{
	// Use the General Polygon Clipping Library to clip contours
	// If this results in new polygons, return them in carray pa.
	// If bRetainArcs == TRUE, try to retain arcs in polys
	// CPT2 converted.  NB does not do any drawing/undrawing.  TODO perhaps one day it would be nice to
	// figure out a way to generalize this to any type of cpolyline.
	CArray<CArc> arc_array;
	if( bRetainArcs )
		MakeGpcPoly( NULL, &arc_array );
	else
		MakeGpcPoly( NULL, NULL );

	// now, recreate poly
	// first, find outside contours and create new carea2's if necessary
	int n_ext_cont = 0;
	for( int ic=0; ic<m_gpc_poly->num_contours; ic++ )
	{
		if( m_gpc_poly->hole[ic] ) continue;
		carea2 *poly;
		n_ext_cont++;
		if( n_ext_cont == 1 )
			poly = this,
			poly->MustRedraw(),
			contours.RemoveAll();
		else if (pa)
			poly = new carea2(m_net, m_layer, m_hatch, m_w, m_sel_box),
			poly->MustRedraw(),
			pa->Add(poly);
		else 
			break;
		ccontour *ctr = new ccontour(poly, true);						// NB the new contour will be poly->main
		for( int i=0; i<m_gpc_poly->contour[ic].num_vertices; i++ )
		{
			int x = m_gpc_poly->contour[ic].vertex[i].x;
			int y = m_gpc_poly->contour[ic].vertex[i].y;
			ccorner *c = new ccorner(ctr, x, y);						// Constructor adds corner to ctr->corners and will also set ctr->head/tail
			if (i>0)
			{
				cside *s = new cside(ctr, STRAIGHT);
				ctr->AppendSideAndCorner(s, c, ctr->tail);
			}
		}
		ctr->Close(STRAIGHT);
		}

	// now add cutouts to the carea2(s)
	citer<carea2> ia (pa);
	for( int ic=0; ic<m_gpc_poly->num_contours; ic++ ) 
	{
		if( !m_gpc_poly->hole[ic] ) continue;
		// Find external poly containing this cutout.
		carea2* ext_poly = NULL;
		if( n_ext_cont == 1 )
			ext_poly = this;
		else
			// find the polygon that contains this hole
			for( int i=0; i<m_gpc_poly->contour[ic].num_vertices && !ext_poly; i++ )
			{
				int x = m_gpc_poly->contour[ic].vertex[i].x;
				int y = m_gpc_poly->contour[ic].vertex[i].y;
				if( TestPointInside( x, y ) )
					ext_poly = this;
				else
					for( carea2 *a = ia.First(); a; a = ia.Next() )
						if( a->TestPointInside( x, y ) )
						{
							ext_poly = a;
							break;
						}
			}
		ASSERT( ext_poly );
		ccontour *ctr = new ccontour(ext_poly, false);						// NB the new contour will not be the main one
		for( int i=0; i<m_gpc_poly->contour[ic].num_vertices; i++ )
		{
			int x = m_gpc_poly->contour[ic].vertex[i].x;
			int y = m_gpc_poly->contour[ic].vertex[i].y;
			ccorner *c = new ccorner(ctr, x, y);							// Constructor adds corner to ctr->corners and will also set ctr->head/tail
			if (i>0)
			{
				cside *s = new cside(ctr, STRAIGHT);
				ctr->AppendSideAndCorner(s, c, ctr->tail);
			}
		}
		ctr->Close(STRAIGHT);
	}
	if( bRetainArcs )
		// The type-casting of the second argument is a bit distasteful but should be safe (I think):
		RestoreArcs( &arc_array, (carray<cpolyline>*) pa );
	FreeGpcPoly();
}


// Process an area that has been modified, by clipping its polygon against itself.
// This may change the number and order of copper areas in the net.
// If bMessageBoxInt == TRUE, shows message when clipping occurs.
// If bMessageBoxArc == TRUE, shows message when clipping can't be done due to arcs.
// Returns:
//	-1 if arcs intersect other sides, so polygon can't be clipped
//	 0 if no intersecting sides
//	 1 if intersecting sides
// Also sets area->utility flags if areas are modified
// CPT2 converted.  Subroutines take care of calling MustRedraw(), as appropriate.
//
int carea2::ClipPolygon( bool bMessageBoxArc, bool bMessageBoxInt, bool bRetainArcs )
{	
	int test = TestPolygon();				// this sets utility2 flag
	if( test == -1 && !bRetainArcs )
		test = 1;
	if( test == -1 )
	{
		// arc intersections, don't clip unless bRetainArcs == FALSE
		if( bMessageBoxArc && !bDontShowSelfIntersectionArcsWarning )
		{
			CString str, s ((LPCSTR) IDS_AreaOfNetHasArcsIntersectingOtherSides);
			str.Format( s, UID(), m_net->name );
			CDlgMyMessageBox dlg;
			dlg.Initialize( str );
			dlg.DoModal();
			bDontShowSelfIntersectionArcsWarning = dlg.bDontShowBoxState;
		}
		return -1;	// arcs intersect with other sides, error
	}

	// mark all areas as unmodified except this one
	m_net->areas.SetUtility(0);
	utility = 1;

	if( test == 1 )
	{
		// non-arc intersections, clip the polygon
		if( bMessageBoxInt && bDontShowSelfIntersectionWarning == FALSE)
		{
			CString str, s ((LPCSTR) IDS_AreaOfNetIsSelfIntersectingAndWillBeClipped);
			str.Format( s, UID(), m_net->name );
			CDlgMyMessageBox dlg;
			dlg.Initialize( str );
			dlg.DoModal();
			bDontShowSelfIntersectionWarning = dlg.bDontShowBoxState;
		}
	}
//** TODO test for cutouts outside of area	
	// CPT the next line "if (test==1)" was commented out --- surely a mistake??  If we run NormalizeWithGpc() below every time, weird things 
	// seem to happen.  For instance, if we add a new vertex to an area edge-segment and the vertex is on the segment, it may get eliminated...
	if( test == 1 )
	{
		carray<carea2> aa;
		NormalizeWithGpc( &aa, bRetainArcs );				// NB Routine will change this, and will put any extra new areas into aa.
		m_net->areas.Add( &aa );
	}
	return test;
}


int carea2::PolygonModified( bool bMessageBoxArc, bool bMessageBoxInt )
{	
	// Process an area that has been modified, by clipping its polygon against
	// itself and the polygons for any other areas on the same net.
	// This may change the number and order of copper areas in the net.
	// If bMessageBox == TRUE, shows message boxes when clipping occurs.
	// Returns:
	//	-1 if arcs intersect other sides, so polygon can't be clipped
	//	 0 if no intersecting sides
	//	 1 if intersecting sides, polygon clipped
	//
	// clip polygon against itself
	// CPT2 converted.  Subroutines take care of calling MustRedraw() on affected area(s), as needed.
	int test = ClipPolygon( bMessageBoxArc, bMessageBoxInt );
	if( test == -1 )
		return test;
	// now see if we need to clip against other areas
	BOOL bCheckAllAreas = FALSE;
	if( test == 1 )
		bCheckAllAreas = TRUE;
	else
		bCheckAllAreas = TestIntersections();
	if( bCheckAllAreas )
		m_net->CombineAllAreas( bMessageBoxInt, TRUE );			// CPT2 TODO. Check into this bUseUtility param.
	m_net->SetThermals();
	return bCheckAllAreas? 1: 0;
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

cnet2::cnet2( CFreePcbDoc *_doc, CString _name, int _def_w, int _def_via_w, int _def_via_hole_w )
	: cpcb_item (_doc)
{
	m_nlist = doc->m_nlist;
	m_nlist->nets.Add(this);
	name = _name;
	def_w = _def_w;
	def_via_w = _def_via_w; def_via_hole_w = _def_via_hole_w;
	bVisible = true;	
}

bool cnet2::IsValid()
	{ return doc->m_nlist->nets.Contains(this); }

void cnet2::GetStatusStr( CString * str )
{
	CString s ((LPCSTR) IDS_Net2);
	str->Format( s, name ); 
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
		// CPT2 TODO what about vias (+thermals)?
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
		// CPT2 TODO what about vias (+thermals)?
	}
}

void cnet2::Highlight(cpcb_item *exclude)
{
	// CPT2 Derived from old CNetList::HighlightConnection(), plus areas get highlighted too.  (Didn't see a reason to separate out the connection highlighting
	// routine at this point...)
	// AMW r272: thin segment highlights and vertex highlights
	// AMW r274: exclude item(s) indicated by exclude_id
	// if except is a segment, just exclude it
	// if exclude is a vertex, exclude it and adjacent segments
	cseg2 *x_seg1 = NULL, *x_seg2 = NULL;
	if (!exclude) 
		;
	else if (exclude->IsSeg())
		x_seg1 = exclude->ToSeg();
	else if (cvertex2 *v = exclude->ToVertex())
		x_seg1 = v->preSeg,
		x_seg2 = v->postSeg;

	citer<cconnect2> ic (&connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
	{
		// Highlight all segs in net, minus exclusions, with _thin_ lines
		citer<cseg2> is (&c->segs);
		for (cseg2 *s = is.First(); s; s = is.Next())
			if (s != x_seg1 && s != x_seg2)
				s->Highlight(true);
		// Highlight all vtxs, minus exclusions (this will take care of tees and pins too)
		citer<cvertex2> iv (&c->vtxs);
		for (cvertex2 *v = iv.First(); v; v = iv.Next())
			if (v != exclude)
				v->Highlight();
	}

	citer<carea2> ia (&areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		a->Highlight();
}

cpin2 *cnet2::AddPin( CString * ref_des, CString * pin_name )
{
	// CPT2.  TODO no check if pin was previously assigned to another net.  Change?  Plus, deal with set_areas
	cpin2 *pin = doc->m_plist->GetPinByNames(ref_des, pin_name);
	if (!pin) return NULL;
	pins.Add(pin);
	pin->net = this;
	return pin;
}

void cnet2::AddPin( cpin2 *pin )
{
	pins.Add(pin);
	pin->net = this;
}

void cnet2::GetWidth( int * w, int * via_w, int * via_hole_w )
{
	// Get net's trace and via widths.  If no net-specific default value set, use the board defaults.
	// CPT2.  Derived from old CFreePcbView::GetWidthsForSegment().  Args may now be null (via_w and via_hole_w are by default)
	if (w)
	{
		*w = doc->m_trace_w;
		if( def_w )
			*w = def_w;
	}
	if (via_w)
	{
		*via_w = doc->m_via_w;
		if( def_via_w )
			*via_w = def_via_w;
	}
	if (via_hole_w)
	{
		*via_hole_w = doc->m_via_hole_w;
		if( def_via_hole_w )
			*via_hole_w = def_via_hole_w;
	}
}

void cnet2::CalcViaWidths(int w, int *via_w, int *via_hole_w) 
{
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

void cnet2::SetThermals() 
{
	// CPT2.  New system for thermals.  Run SetNeedsThermal() for all vertices and pins in the net.
	citer<cconnect2> ic (&connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
	{
		citer<cvertex2> iv (&c->vtxs);
		for (cvertex2 *v = iv.First(); v; v = iv.Next())
			v->SetNeedsThermal();
	}
	citer<cpin2> ip (&pins);
	for (cpin2 *p = ip.First(); p; p = ip.Next())
		p->SetNeedsThermal();
}

cvertex2 *cnet2::TestHitOnVertex(int layer, int x, int y) 
{
	// TODO maybe obsolete.  Frequently cvertex2::TestForHit() does the trick.
	// Test for a hit on a vertex in a trace within this net/layer.
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

void cnet2::OptimizeConnections( BOOL bBelowPinCount, int pin_count, BOOL bVisibleNetsOnly )
{
	// CPT2 TODO.  Derive from CNetList::OptimizeConnections.  I think we can ditch the ic_track argument of the old routine. 
#ifdef PROFILE
	StartTimer();	//****
#endif
#ifndef CPT2
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

	// go through net, deleting unrouted and unlocked connections 
	// unless they end on a tee or end-vertex
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


	// AMW r289: Programming note:
	// The following code figures out which pins are connected to each other through
	// existing traces, tees and copper areas, and therefore don't need new ratlines.
	// Previously, when every connection had to start on a pin,
	// this was pretty easy to do since the levels of branching were limited.
	// Now that traces can be routed between other traces and copper areas pretty much ad lib,
	// it is more complicated. Probably the best approach would be some sort of recursive
	// tree-following or grid-following algorithm, but since I am lazy and modifying existing
	// code, I am using a different method of starting with a pin,
	// iterating through all the connections in the net (multiple times if necessary),
	// and building maps of the connected pins, tees and copper areas, so I can eventually
	// identify every connected pin. Hopefully, this won't be too inefficient.
	int dummy;
	CMap<int, int, int, int> pins_analyzed;  // list of all pins analyzed
	CIterator_cpin iter_cpin(net);
	for( cpin * pin=iter_cpin.GetFirst(); pin; pin=iter_cpin.GetNext() )
	{
		// skip pins that have already been analyzed through connections to earlier pins
		int ipin = pin->Index();
		if( pins_analyzed.Lookup( ipin, dummy ) )
			continue;
		pins_analyzed.SetAt( ipin, ipin );

		// look for all connections to this pin, or to any pin, tee or area connected to this pin
		CMap<int, int, int, int> cons_eliminated;  // list of connections that don't connect to this pin
		CMap<int, int, int, int> cons_connected;   // list of connections that do connect to this pin
		CMap<int, int, int, int> tee_ids_connected;	// list of tee_ids connected to this pin
		CMap<int, int, int, int> pins_connected;	// list of pins connected to this pin
		CMap<int, int, int, int> areas_connected;	// list of areas connected to this pin
		int num_new_connections = 1;
		while( num_new_connections )	// iterate as long as we are still finding new connections
		{
			num_new_connections = 0;
			for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
			{
				int ic = c->Index();
				if( cons_eliminated.Lookup( ic, dummy ) )	// already eliminated
					continue;
				if( cons_connected.Lookup( ic, dummy ) )	// already analyzed
					continue;
				// see if this connection can be eliminated, 
				// or is connected to a pin, tee or area that connects to the pin being analyzed
				bool bConEliminated = TRUE;
				bool bConConnected = FALSE;
				CIterator_cvertex iter_vtx(c);
				for( cvertex * v=iter_vtx.GetFirst(); v; v=iter_vtx.GetNext() )
				{
					if( v->GetType() == cvertex::V_PIN )
					{
						// vertex is a pin
						cpin * v_pin = v->GetNetPin();
						if( v_pin == pin )
						{
							// pin being analyzed
							bConEliminated = FALSE;
							bConConnected = TRUE;
							break;
						}
						else if( pins_connected.Lookup( v_pin->Index(), dummy ) )
						{
							// pin connected to pin being analyzed
							bConEliminated = FALSE;
							bConConnected = TRUE;
							break;
						}
						else
						{
							// other pin, might connect in later iterations
							bConEliminated = FALSE;
						}
					}
					else if( v->GetType() == cvertex::V_TEE || v->GetType() == cvertex::V_SLAVE )
					{
						// vertex is a tee, might connect
						bConEliminated = FALSE;
						if( tee_ids_connected.Lookup( abs(v->tee_ID), dummy ) )
							bConConnected = TRUE;	// does connect
					}
					CArray<int> ca;		// array of indices to copper areas
					if( v->GetConnectedAreas( &ca ) > 0 )
					{
						// connected to copper area(s), might connect
						bConEliminated = FALSE;
						for( int iarray=0; iarray<ca.GetSize(); iarray++ )
						{
							// get copper area
							int ia = ca[iarray];
							if( areas_connected.Lookup( ia, dummy ) )
								bConConnected = TRUE;	// does connect
						}
					}
				}
				if( bConEliminated )
				{
					// don't need to look at this connection any more
					cons_eliminated.SetAt( ic, ic );
				}
				else if( bConConnected )
				{
					cons_connected.SetAt( ic, ic );
					num_new_connections++;
					// add pins. tees and areas to maps of connected items
					for( cvertex * v=iter_vtx.GetFirst(); v; v=iter_vtx.GetNext() )
					{
						if( v->GetType() == cvertex::V_PIN )
						{
							// vertex is a pin
							cpin * v_pin = v->GetNetPin();
							if( v_pin != pin )
							{
								// connected
								pins_connected.SetAt( v_pin->Index(), v_pin->Index() );
								pins_analyzed.SetAt( v_pin->Index(), v_pin->Index() );
							}
						}
						else if( v->GetType() == cvertex::V_TEE || v->GetType() == cvertex::V_SLAVE )
						{
							// vertex is a tee
							tee_ids_connected.SetAt( abs(v->tee_ID), abs(v->tee_ID) );
						}
						CArray<int> ca;		// array of indices to copper areas
						if( v->GetConnectedAreas( &ca ) > 0 )
						{
							// vertex connected to copper area(s), add to map
							for( int iarray=0; iarray<ca.GetSize(); iarray++ )
							{
								// get copper area(s)
								int ia = ca[iarray];
								areas_connected.SetAt( ia, ia );
								// get pins attached to copper area
								carea* a = net->AreaByIndex(ia);
								for( int ip=0; ip<a->NumPins(); ip++ )
								{
									cpin * pin = a->PinByIndex(ip);
									ipin = pin->Index();
									pins_connected.SetAt( ipin, ipin );
								}
							}
						}
					}
				}
			}	// end connection loop
		}	// end while loop

		// now loop through all connected pins and mark them as connected
		int m_pins = pins_connected.GetCount();
		if( m_pins > 0 )
		{
			int p1 = pin->Index();
			POSITION pos = pins_connected.GetStartPosition();
			int    nKey, nValue;
			while (pos != NULL)
			{
				pins_connected.GetNextAssoc( pos, nKey, nValue );
				int p2 = nValue;
				AddPinsToGrid( grid, p1, p2, npins );
			}
		}

	}	// end loop through net pins


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

//#if 0
	// add new optimized connections
	for( int ic=0; ic<n_optimized; ic++ )
	{
		// make new connection with a single unrouted segment
		int p1 = pair[ic*2];
		int p2 = pair[ic*2+1];
		net->AddConnectFromPinToPin( p1, p2 );
	}
//#endif

	free( grid );

	// find ic_track if still present, and return index
	int ic_new = -1;
	if( ic_track >= 0 )
	{
		if( id_track.Resolve() )
		{
			ic_new = id_track.I2();
		}
	}

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
#endif
}

void cnet2::CombineAllAreas( BOOL bMessageBox, BOOL bUseUtility )
{
	// Checks all copper areas in net for intersections, combining them if found
	// If bUseUtility == TRUE, don't check areas if both utility flags are 0
	// Sets utility flag = 1 for any areas modified
	// If an area has self-intersecting arcs, doesn't try to combine it
	// CPT2 converted.  The subroutines are in charge of calling MustRedraw() for affected areas.
	if (NumAreas()<2) return;

	// start by testing all area polygons to set utility2 flags.
	citer<carea2> ia (&areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		a->TestPolygon();
	// now loop through all combinations
	BOOL message_shown = FALSE, mod_a1;
	citer<carea2> ia1 (&areas);
	for (carea2 *a1 = ia1.First(); a1; a1 = ia1.Next())
	{
		do {
			CRect b1 = a1->GetCornerBounds();
			mod_a1 = FALSE;
			citer<carea2> ia2 (&areas, a1);
			ia2.Next();														// Advance to area AFTER a1.
			for (carea2 *a2 = ia2.Next(); a2; a2 = ia2.Next())
			{
				if (a1->GetLayer() != a2->GetLayer()) continue;
				if (a1->utility2 == -1 || a2->utility2 == -1) continue;
				CRect b2 = a2->GetCornerBounds();
				if ( b1.left > b2.right || b1.right < b2.left
						|| b1.bottom > b2.top || b1.top < b2.bottom ) continue;
				if( bUseUtility && !a1->utility && !a2->utility ) continue;
				// check a2 against a1 
				int ret = a1->TestIntersection( a2, true );
				if( ret == 1 )
				{
					ret = a1->CombinePolyline( a2 );
					if (ret == 0) continue;
					areas.Remove( a2 );
					if( ret == 1 && bMessageBox && !bDontShowIntersectionWarning )
					{
						CString str, s ((LPCSTR) IDS_AreasOfNetIntersectAndWillBeCombined);
						str.Format( s, a1->UID(), a2->UID(), name );							// CPT2.  Just to provide a number, give 'em the uid's
						CDlgMyMessageBox dlg;
						dlg.Initialize( str );
						dlg.DoModal();
						bDontShowIntersectionWarning = dlg.bDontShowBoxState;
					}
					mod_a1 = TRUE;
				}
				else if( ret == 2 )
				{
					if( bMessageBox && !bDontShowIntersectionArcsWarning )
					{
						CString str, s ((LPCSTR) IDS_AreasOfNetIntersectButSomeOfTheIntersectingSidesAreArcs);
						str.Format( s, a1->UID(), a2->UID(), name );
						CDlgMyMessageBox dlg;
						dlg.Initialize( str );
						dlg.DoModal();
						bDontShowIntersectionArcsWarning = dlg.bDontShowBoxState;
					}
				}
			}
		}
		while (mod_a1);		// if a1 is modified, we need to check it against all the other areas again.
	}
}


int cnet2::Draw() 
{
	// CPT2 TODO decide what to do if anything about bDrawn flag.
	citer<cconnect2> ic (&connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
		c->Draw();
	citer<carea2> ia (&areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		a->Draw();
	// Also deal with tees that are in the net.  (The subsidiary vtxs are never drawn.)
	citer<ctee> it (&tees);
	for (ctee *t = it.First(); t; t = it.Next())
		t->Draw();
	
	return NOERR;
}

void cnet2::Undraw() 
{
	// CPT2 Undraw constituents one by one.  The bDrawn flag for the net itself is probably irrelevant.
	citer<cconnect2> ic (&connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
		c->Undraw();
	citer<carea2> ia (&areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		a->Undraw();
	// Also deal with tees that are in the net.  (The subsidiary vtxs are never drawn.)
	citer<ctee> it (&tees);
	for (ctee *t = it.First(); t; t = it.Next())
		t->Undraw();
}

/**********************************************************************************************/
/*  OTHERS: ctext, cadhesive, ccentroid                                                       */
/**********************************************************************************************/

ctext::ctext( CFreePcbDoc *_doc, int _x, int _y, int _angle, 
	BOOL _bMirror, BOOL _bNegative, int _layer, int _font_size, 
	int _stroke_width, SMFontUtil *_smfontutil, CString * _str )			// CPT2 Removed selType/selSubtype args.  Will use derived creftext and cvaluetext
	: cpcb_item (_doc)														// classes in place of this business.
{
	m_x = _x, m_y = _y;
	m_angle = _angle;
	m_bMirror = _bMirror; m_bNegative = _bNegative;
	m_layer = _layer;
	m_font_size = _font_size;
	m_stroke_width = _stroke_width;
	m_smfontutil = _smfontutil;
	m_str = *_str;
}

bool ctext::IsValid()
	{ return doc->m_tlist->texts.Contains(this); }

void ctext::Copy( ctext *other )
{
	// CPT2 new.  Copy the contents of "other" into "this"
	MustRedraw();
	m_x = other->m_x;
	m_y = other->m_y;
	m_angle = other->m_angle;
	m_bMirror = other->m_bMirror; 
	m_bNegative = other->m_bNegative;
	m_layer = other->m_layer;
	m_font_size = other->m_font_size;
	m_stroke_width = other->m_stroke_width;
	m_smfontutil = other->m_smfontutil;
	m_str = other->m_str;
}

void ctext::Move( int x, int y, int angle, BOOL mirror, BOOL negative, int layer, int size, int w )
{
	MustRedraw();
	m_x = x;
	m_y = y;
	m_angle = angle;
	m_layer = layer;
	m_bMirror = mirror;
	m_bNegative = negative;
	if (size>=0) m_font_size = size;
	if (w>=0) m_stroke_width = w;
}

void ctext::Move(int x, int y, int angle, int size, int w) 
{
	// CPT:  extra version of Move(); appropriate for ref and value-texts, where the layer doesn't change and the mirroring is by default
	bool bMirror = m_layer==LAY_FP_SILK_BOTTOM || m_layer==LAY_FP_BOTTOM_COPPER;
	Move(x, y, angle, bMirror, false, m_layer, size, w);
}

void ctext::Resize(int size, int w)
	{ Move (m_x, m_y, m_angle, m_bMirror, m_bNegative, m_layer, size, w); }


void ctext::GenerateStrokes() {
	// CPT2 new.  Helper for Draw(), though it might also be called independently of drawing.
	// Generate strokes and put them in m_stroke.  Also setup the bounding rectangle member m_br.
	// TODO consider caching
	m_stroke.SetSize( 1000 );
	CPoint si, sf;
	double x_scale = (double)m_font_size/22.0;
	double y_scale = (double)m_font_size/22.0;
	double y_offset = 9.0*y_scale;
	int i = 0;
	double xc = 0.0;
	int xmin = INT_MAX;
	int xmax = INT_MIN;
	int ymin = INT_MAX;
	int ymax = INT_MIN;
	int nChars = m_str.GetLength();
	// CPT2.  Setup font (in case m_smfontutil is NULL).  This font business is a bit of a pain...
	SMFontUtil *smf = m_smfontutil;
	if (!smf)
		smf = ((CFreePcbApp*)AfxGetApp())->m_doc->m_smfontutil;

	for( int ic=0; ic<nChars; ic++ )
	{
		// get stroke info for character
		int xi, yi, xf, yf;
		double coord[64][4];
		double min_x, min_y, max_x, max_y;
		int nstrokes;
		if( !m_bMirror )
			nstrokes = smf->GetCharStrokes( m_str[ic], SIMPLEX, &min_x, &min_y, &max_x, &max_y,
				coord, 64 );
		else
			nstrokes = smf->GetCharStrokes( m_str[nChars-ic-1], SIMPLEX, &min_x, &min_y, &max_x, &max_y,
				coord, 64 );
		// loop through strokes and create stroke structures
		for( int is=0; is<nstrokes; is++ )
		{
			if( m_bMirror )
			{
				xi = (max_x - coord[is][0])*x_scale;
				yi = coord[is][1]*y_scale + y_offset;
				xf = (max_x - coord[is][2])*x_scale;
				yf = coord[is][3]*y_scale + y_offset;
			}
			else
			{
				xi = (coord[is][0] - min_x)*x_scale;
				yi = coord[is][1]*y_scale + y_offset;
				xf = (coord[is][2] - min_x)*x_scale;
				yf = coord[is][3]*y_scale + y_offset;
			}

			// get stroke relative to x,y
			si.x = xi + xc;
			sf.x = xf + xc;
			si.y = yi;
			sf.y = yf;
			// rotate
			RotatePoint( &si, m_angle, zero );
			RotatePoint( &sf, m_angle, zero );
			// add m_x, m_y, and fill in stroke structure
			stroke * s = &m_stroke[i];
			s->w = m_stroke_width;
			s->xi = m_x + si.x;
			s->yi = m_y + si.y;
			s->xf = m_x + sf.x;
			s->yf = m_y + sf.y;
			// update bounding rectangle
			ymin = min( ymin, s->yi - s->w );
			ymin = min( ymin, s->yf - s->w );
			ymax = max( ymax, s->yi + s->w );
			ymax = max( ymax, s->yf + s->w );
			xmin = min( xmin, s->xi - s->w );
			xmin = min( xmin, s->xf - s->w );
			xmax = max( xmax, s->xi + s->w );
			xmax = max( xmax, s->xf + s->w );
			// Next stroke...
			i++;
			if( i >= m_stroke.GetSize() )
				m_stroke.SetSize( i + 100 );
		}
		if( nstrokes > 0 )
			xc += (max_x - min_x + 8.0)*x_scale;
		else
			xc += 16.0*x_scale;
	}

	// Wrap up
	m_stroke.SetSize( i );
	m_br.left = xmin - m_stroke_width/2;
	m_br.right = xmax + m_stroke_width/2;
	m_br.bottom = ymin - m_stroke_width/2;
	m_br.top = ymax + m_stroke_width/2;
}

void ctext::GenerateStrokesRelativeTo(cpart2 *part) {
	// CPT2 new.  Helper for DrawRelativeTo(), though it might also be called independently of drawing.
	// Somewhat descended from the old GenerateStrokesFromPartString() in PartList.cpp.
	// Used for texts (including reftexts and valuetexts) whose position is relative to "part".
	// Generate strokes and put them in m_stroke.  Also setup the bounding rectangle member m_br.
	// TODO consider caching
	SMFontUtil *smf = m_smfontutil;
	if (smf==NULL) 
		// May happen if "this" is a text belonging to a footprint
		smf = part->doc->m_smfontutil;
	m_stroke.SetSize( 1000 );
	CPoint si, sf;
	double x_scale = (double)m_font_size/22.0;
	double y_scale = (double)m_font_size/22.0;
	double y_offset = 9.0*y_scale;
	// Adjust layer value if part is on bottom
	int layer = m_layer;
	if (part->side)
		if (layer==LAY_SILK_TOP) layer = LAY_SILK_BOTTOM;
		else if (layer==LAY_TOP_COPPER) layer = LAY_BOTTOM_COPPER;
		else if (layer==LAY_SILK_BOTTOM) layer = LAY_SILK_TOP;
		else if (layer==LAY_BOTTOM_COPPER) layer = LAY_TOP_COPPER;
	int i = 0;
	double xc = 0.0;
	int xmin = INT_MAX;
	int xmax = INT_MIN;
	int ymin = INT_MAX;
	int ymax = INT_MIN;
	int nChars = m_str.GetLength();

	for( int ic=0; ic<nChars; ic++ )
	{
		// get stroke info for character
		int xi, yi, xf, yf;
		double coord[64][4];
		double min_x, min_y, max_x, max_y;
		int nstrokes;
		if( !m_bMirror )
			nstrokes = smf->GetCharStrokes( m_str[ic], SIMPLEX, &min_x, &min_y, &max_x, &max_y,
				coord, 64 );
		else
			nstrokes = smf->GetCharStrokes( m_str[nChars-ic-1], SIMPLEX, &min_x, &min_y, &max_x, &max_y,
				coord, 64 );
		// loop through strokes and create stroke structures
		for( int is=0; is<nstrokes; is++ )
		{
			if( m_bMirror )
			{
				xi = (max_x - coord[is][0])*x_scale;
				yi = coord[is][1]*y_scale + y_offset;
				xf = (max_x - coord[is][2])*x_scale;
				yf = coord[is][3]*y_scale + y_offset;
			}
			else
			{
				xi = (coord[is][0] - min_x)*x_scale;
				yi = coord[is][1]*y_scale + y_offset;
				xf = (coord[is][2] - min_x)*x_scale;
				yf = coord[is][3]*y_scale + y_offset;
			}
			// Get stroke points relative to text box origin
			si.x = xi + xc;
			sf.x = xf + xc;
			si.y = yi;
			sf.y = yf;
			// rotate about text box origin
			RotatePoint( &si, m_angle, zero );
			RotatePoint( &sf, m_angle, zero );
			// move origin of text box to position relative to part
			si.x += m_x;
			sf.x += m_x;
			si.y += m_y;
			sf.y += m_y;
			// flip if part on bottom
			if( part->side )
				si.x = -si.x,
				sf.x = -sf.x;
			// rotate with part about part origin
			RotatePoint( &si, part->angle, zero );
			RotatePoint( &sf, part->angle, zero );
			// add part's (x,y), then add stroke to array
			stroke * s = &m_stroke[i];
			s->w = m_stroke_width;
			s->xi = part->x + si.x;
			s->yi = part->y + si.y;
			s->xf = part->x + sf.x;
			s->yf = part->y + sf.y;
			s->layer = layer;
			// update bounding rectangle
			ymin = min( ymin, s->yi - s->w );
			ymin = min( ymin, s->yf - s->w );
			ymax = max( ymax, s->yi + s->w );
			ymax = max( ymax, s->yf + s->w );
			xmin = min( xmin, s->xi - s->w );
			xmin = min( xmin, s->xf - s->w );
			xmax = max( xmax, s->xi + s->w );
			xmax = max( xmax, s->xf + s->w );
			// Next stroke...
			i++;
			if( i >= m_stroke.GetSize() )
				m_stroke.SetSize( i + 100 );
		}
		if( nstrokes > 0 )
			xc += (max_x - min_x + 8.0)*x_scale;
		else
			xc += 16.0*x_scale;
	}

	// Wrap up
	m_stroke.SetSize( i );
	m_br.left = xmin - m_stroke_width/2;
	m_br.right = xmax + m_stroke_width/2;
	m_br.bottom = ymin - m_stroke_width/2;
	m_br.top = ymax + m_stroke_width/2;
}


int ctext::Draw() 
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl )
		return NO_DLIST;
	if (bDrawn)
		return ALREADY_DRAWN;

	GenerateStrokes();													// Fills m_stroke and m_br
	// Now draw each stroke
	for( int is=0; is<m_stroke.GetSize(); is++ )
		m_stroke[is].dl_el = dl->AddMain( this, m_layer, 
			DL_LINE, 1, m_stroke[is].w, 0, 0,
			m_stroke[is].xi, m_stroke[is].yi, 
			m_stroke[is].xf, m_stroke[is].yf, 0, 0 );
	// draw selection rectangle for text
	dl_sel = dl->AddSelector( this, m_layer, DL_HOLLOW_RECT, 1,	
		0, 0, m_br.left, m_br.bottom, m_br.right, m_br.top, m_br.left, m_br.bottom );
	bDrawn = true;
	return NOERR;
}

int ctext::DrawRelativeTo(cpart2 *part, bool bSelector) 
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl )
		return NO_DLIST;
	if (bDrawn)
		return ALREADY_DRAWN;
	if (m_str.GetLength()==0)
		return EMPTY_TEXT;

	GenerateStrokesRelativeTo( part );													// Fills m_stroke and m_br
	// Now draw each stroke
	for( int is=0; is<m_stroke.GetSize(); is++ )
		m_stroke[is].dl_el = dl->AddMain( this, m_stroke[is].layer, 
			DL_LINE, 1, m_stroke[is].w, 0, 0,
			m_stroke[is].xi, m_stroke[is].yi, 
			m_stroke[is].xf, m_stroke[is].yf, 0, 0 );
	// draw selection rectangle for text
	if (bSelector)
		dl_sel = dl->AddSelector( this, m_layer, DL_HOLLOW_RECT, 1,	
									0, 0, m_br.left, m_br.bottom, m_br.right, m_br.top, m_br.left, m_br.bottom );
	bDrawn = true;
	return NOERR;
}

void ctext::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl || !IsDrawn() ) return;

	dl->Remove( dl_sel );
	dl_sel = NULL;
	for( int i=0; i<m_stroke.GetSize(); i++ )
		dl->Remove( m_stroke[i].dl_el );
	m_stroke.RemoveAll();
	// m_smfontutil = NULL;							// indicate that strokes have been removed.  CPT2 Removed, caused a crash
	bDrawn = false;
}

void ctext::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;
	dl->Highlight( DL_HOLLOW_RECT, 
		dl->Get_x(dl_sel), dl->Get_y(dl_sel),
		dl->Get_xf(dl_sel), dl->Get_yf(dl_sel), 1 );
}

void ctext::SetVisible(bool bVis)
{
	for( int is=0; is<m_stroke.GetSize(); is++ )
		m_stroke[is].dl_el->visible = bVis;
}


creftext::creftext( cpart2 *_part, int x, int y, int angle, 
	BOOL bMirror, BOOL bNegative, int layer, int font_size, 
	int stroke_width, SMFontUtil * smfontutil, CString * str_ptr ) :
		ctext(_part->doc, x, y, angle, bMirror, bNegative, layer, font_size,
			stroke_width, smfontutil, str_ptr) 
		{ part = _part; }

bool creftext::IsValid() { return part && part->IsValid(); }

cvaluetext::cvaluetext( cpart2 *_part, int x, int y, int angle, 
	BOOL bMirror, BOOL bNegative, int layer, int font_size, 
	int stroke_width, SMFontUtil * smfontutil, CString * str_ptr ) :
		ctext(_part->doc, x, y, angle, bMirror, bNegative, layer, font_size,
			stroke_width, smfontutil, str_ptr) 
		{ part = _part; }

bool cvaluetext::IsValid() { return part && part->IsValid(); }
