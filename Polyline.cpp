// Polyline.cpp --- source file for classes related most closely to polylines, all of which are descendants of CPcbItem:
//  CCorner, CSide, CContour, CPolyline, CArea, CSmCutout, CBoard, COutline

#include "stdafx.h"
#include <math.h>
#include <stdlib.h>
#include "Polyline.h"
#include "Net.h"
#include "Part.h"

CCorner::CCorner(CContour *_contour, int _x, int _y)
	: CPcbItem (_contour->doc)
{
	contour = _contour;
	if (contour->corners.IsEmpty())
		contour->head = contour->tail = this;
	contour->corners.Add(this);
	x = _x; y = _y; 
	preSide = postSide = NULL;
}

CCorner::CCorner(CFreePcbDoc *_doc, int _uid):
	CPcbItem(_doc, _uid)
{
	contour = NULL;
	preSide = postSide = NULL;
}

bool CCorner::IsOnPcb() 
{
	if (!contour->poly->IsOnPcb()) return false;
	if (!contour->IsOnPcb()) return false;
	return contour->corners.Contains(this);
}

bool CCorner::IsAreaCorner() { return contour->poly->IsArea(); }
bool CCorner::IsBoardCorner() { return contour->poly->IsBoard(); }
bool CCorner::IsSmCorner() { return contour->poly->IsSmCutout(); }
bool CCorner::IsOutlineCorner() { return contour->poly->IsOutline(); }
CNet *CCorner::GetNet() { return contour->GetNet(); }
int CCorner::GetLayer() { return contour->poly->m_layer; }
CPolyline *CCorner::GetPolyline() { return contour->poly; }


int CCorner::GetTypeBit() 
{														// Later:  put in .cpp file.  There wouldn't be this nonsense in Java...
	if (contour->poly->IsArea()) return bitAreaCorner;
	if (contour->poly->IsSmCutout()) return bitSmCorner;
	if (contour->poly->IsBoard()) return bitBoardCorner;
	if (contour->poly->IsOutline()) return bitOutlineCorner;
	return bitOther;
}

bool CCorner::IsOnCutout()
	// Return true if this side lies on a secondary contour within the polyline
	{ return contour->poly->main!=contour; }

void CCorner::Remove() 
{
	// CPT2 derived loosely from old CPolyLine::DeleteCorner
	CPolyline *poly = GetPolyline();
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
		CCorner *next = postSide->postCorner;
		if (this == contour->head)
			contour->head = contour->tail = next;
		contour->sides.Remove(postSide);
		preSide->postCorner = next;
		next->preSide = preSide;
	}
}

void CCorner::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	if( !dl_sel ) return;
	dl->Highlight( DL_HOLLOW_RECT, 
		dl->Get_x( dl_sel ), dl->Get_y( dl_sel ),
		dl->Get_xf( dl_sel ), dl->Get_yf( dl_sel ),
		dl->Get_w( dl_sel ) );
}

bool CCorner::Move( int _x, int _y, BOOL bEnforceCircularArcs )
{
	// move corner of polyline
	// if bEnforceCircularArcs == TRUE, convert adjacent sides to STRAIGHT if angle not
	// a multiple of 45 degrees and return TRUE, otherwise return FALSE
	// CPT2 derived from CPolyLine::MoveCorner().
	CPolyline *poly = contour->poly;
	poly->MustRedraw();
	x = _x;
	y = _y;
	BOOL bReturn = FALSE;
	if( bEnforceCircularArcs )
	{
		CCorner *prev = preSide? preSide->preCorner: NULL;
		CCorner *next = postSide? postSide->postCorner: NULL;
		if (prev && abs(prev->x - x) != abs(prev->y - y))
			preSide->m_style = STRAIGHT,
			bReturn = TRUE;
		if (next && abs(next->x - x) != abs(next->y - y))
			postSide->m_style = STRAIGHT,
			bReturn = TRUE;
	}
	return bReturn;
}

void CCorner::StartDragging( CDC * pDC, int x, int y, int crosshair )
{
	CDisplayList *dl = doc->m_dlist;
	ASSERT(dl);
	dl->CancelHighlight();
	// Hide adjacent sides (if present) and area hatching
	if (preSide)
		dl->Set_visible(preSide->dl_el, 0);
	if (postSide)
		dl->Set_visible(postSide->dl_el, 0);
	CPolyline *poly = GetPolyline();
	for( int ih=0; ih < poly->m_nhatch; ih++ )
		dl->Set_visible( poly->dl_hatch[ih], 0 );

	// see if corner is the first or last corner of an open contour
	if (!preSide || !postSide)
	{
		int style, xi, yi;
		CSide *side;
		if (!preSide)
		{
			style = postSide->m_style;
			if (style==ARC_CW) 
				style = ARC_CCW;
			else if (style==ARC_CCW) 
				style = ARC_CW;								// Reverse arc since we are drawing from corner 1 to 0
			xi = postSide->postCorner->x;
			yi = postSide->postCorner->y;
			side = postSide;
		}
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
			LAY_SELECTION, LAY_SELECTION, 1, 1, style1, style2, 0, 0, 0, 0, crosshair );
	}
}

void CCorner::CancelDragging()
{
	// CPT2:  Derived from CPolyLine::CancelDraggingToMoveCorner.  Stop dragging, make sides and hatching visible again
	CDisplayList *dl = doc->m_dlist;
	ASSERT( dl );
	dl->StopDragging();
	if (preSide)
		dl->Set_visible(preSide->dl_el, 1);
	if (postSide)
		dl->Set_visible(postSide->dl_el, 1);
	CPolyline *poly = GetPolyline();
	for( int ih=0; ih < poly->m_nhatch; ih++ )
		dl->Set_visible( poly->dl_hatch[ih], 1 );
}


CSide::CSide(CContour *_contour, int _style)
	: CPcbItem(_contour->doc)
{ 
	contour = _contour;
	contour->sides.Add(this);
	m_style = _style;
	preCorner = postCorner = NULL;
}

CSide::CSide(CFreePcbDoc *_doc, int _uid):
	CPcbItem(_doc, _uid)
{
	contour = NULL;
	preCorner = postCorner = NULL;
}
bool CSide::IsOnPcb() 
{
	if (!contour->IsOnPcb()) return false;
	return contour->sides.Contains(this);
}

bool CSide::IsAreaSide() { return contour->poly->IsArea(); }
bool CSide::IsBoardSide() { return contour->poly->IsBoard(); }
bool CSide::IsSmSide() { return contour->poly->IsSmCutout(); }
bool CSide::IsOutlineSide() { return contour->poly->IsOutline(); }
int CSide::GetLayer() { return contour->poly->m_layer; }

int CSide::GetTypeBit() 
{
	if (contour->poly->IsArea()) return bitAreaSide;
	if (contour->poly->IsSmCutout()) return bitSmSide;
	if (contour->poly->IsBoard()) return bitBoardSide;
	if (contour->poly->IsOutline()) return bitOutlineSide;
	return bitOther;
}

CNet *CSide::GetNet() { return contour->GetNet(); }

CPolyline *CSide::GetPolyline() { return contour->poly; }

bool CSide::IsOnCutout()
	// Return true if this side lies on a secondary contour within the polyline
	{ return contour->poly->main!=contour; }

char CSide::GetDirectionLabel()
{
	// CPT2 new.  Used for making labels when user does a shift-click.  Return char is '-', '/', '|', or '\' depending on the direction of the side
	double dx = preCorner->x - postCorner->x;
	double dy = preCorner->y - postCorner->y;
	double slope = dx==0? 1000: dy/dx;
	if (fabs(slope) > 3)
		return '|';
	else if (fabs(slope) < .33)
		return '-';
	else if (slope > 0)
		return '/';
	else
		return '\\';
}

void CSide::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	if( !dl_sel ) return;
	int s;
	if( m_style == CPolyline::STRAIGHT )
		s = DL_LINE;
	else if( m_style == CPolyline::ARC_CW )
		s = DL_ARC_CW;
	else if( m_style == CPolyline::ARC_CCW )
		s = DL_ARC_CCW;
	dl->Highlight( s, 
		dl->Get_x( dl_sel ), dl->Get_y( dl_sel ),
		dl->Get_xf( dl_sel ), dl->Get_yf( dl_sel ),
		dl->Get_w( dl_sel ) );
}

void CSide::InsertCorner(int x, int y)
{
	// CPT2 new.  Add an intermediate corner into this side.  "this" gets reused as the second of the 2 half-sides, and the styles of both are made straight.
	CCorner *c = new CCorner(contour, x, y);
	CSide *s = new CSide(contour, STRAIGHT);
	m_style = STRAIGHT;
	contour->AppendSideAndCorner(s, c, preCorner);
}

bool CSide::Remove( CHeap<CPolyline> *arr ) 
{
	// CPT2 new.  Remove this side from its parent contour (which must be the main contour in the polyline).  This can easily result in a whole 
	// new polyline getting created (by means of virtual function CPolyline::CreateCompatible()).  Said new polyline gets added to "arr".  It can
	// also result in the whole original polyline disappearing, in which case it is removed from "arr".
	CPolyline *poly = GetPolyline();
	if (poly->contours.GetSize() > 1) return false;
	contour->sides.Remove(this);
	preCorner->postSide = postCorner->preSide = NULL;
	if (contour->sides.IsEmpty())
		arr->Remove(poly);
	else if (contour->head == contour->tail) 
		// Break a closed contour.
		contour->head = postCorner,
		contour->tail = preCorner;
	else if (!preCorner->preSide)
		// Eliminate 1st seg of open contour.
		contour->head = postCorner,
		contour->corners.Remove(preCorner);
	else if (!postCorner->postSide)
		// Eliminate last seg of open contour.
		contour->tail = preCorner,
		contour->corners.Remove(postCorner);
	else
	{
		// Break!
		CPolyline *poly2 = poly->CreateCompatible();
		arr->Add(poly2);
		CContour *ctr2 = new CContour(poly2, true);
		ctr2->head = postCorner;
		ctr2->tail = contour->tail;
		contour->tail = preCorner;
		for (CCorner *c = postCorner; 1; c = c->postSide->postCorner)
		{
			contour->corners.Remove(c);
			ctr2->corners.Add(c);
			c->contour = ctr2;
			if (!c->postSide) break;
			contour->sides.Remove(c->postSide);
			ctr2->sides.Add(c->postSide);
			c->postSide->contour = ctr2;
		}
	}
	return true;
}

void CSide::StartDraggingNewCorner( CDC * pDC, int x, int y, int crosshair )
{
	// CPT2, derived from CPolyLine::StartDraggingToInsertCorner
	CDisplayList *dl = doc->m_dlist;
	ASSERT( dl );

	int xi = preCorner->x, yi = preCorner->y;
	int xf = postCorner->x, yf = postCorner->y;
	dl->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, 
		LAY_SELECTION, LAY_SELECTION, 1, 1, DSS_STRAIGHT, DSS_STRAIGHT,
		0, 0, 0, 0, crosshair );
	dl->CancelHighlight();
	dl->Set_visible( dl_el, 0 );
	CPolyline *p = GetPolyline();
	for( int ih=0; ih < p->m_nhatch; ih++ )
		dl->Set_visible( p->dl_hatch[ih], 0 );
}

//
void CSide::CancelDraggingNewCorner()
{
	// CPT2 derived from CPolyLine::CancelDraggingToInsertCorner
	CDisplayList *dl = doc->m_dlist;
	ASSERT( dl );

	dl->StopDragging();
	dl->Set_visible( dl_el, 1 );
	CPolyline *p = GetPolyline();
	for( int ih=0; ih < p->m_nhatch; ih++ )
		dl->Set_visible( p->dl_hatch[ih], 1 );
}





CContour::CContour(CPolyline *_poly, bool bMain)
	: CPcbItem (_poly->doc)
{
	poly = _poly;
	if (bMain) 
		poly->main = this;
	poly->contours.Add(this);
	head = tail = NULL;
}

CContour::CContour(CFreePcbDoc *_doc, int _uid):
	CPcbItem(_doc, _uid)
{
	poly = NULL;
	head = tail = NULL;
}

CContour::CContour(CPolyline *_poly, CContour *src)
	: CPcbItem (_poly->doc)
{
	// Create a new contour with the same points/sides as "src", but belonging to poly "_poly".  Copies utility values (occasionally useful)
	poly = _poly;
	poly->contours.Add(this);
	corners.RemoveAll();
	sides.RemoveAll();
	head = tail = NULL;
	utility = src->utility;
	if (src->corners.GetSize()==0) 
		return;

	// Loop thru src's corners and sides in geometrical order.  Must take care since src's head and tail may be the same.
	CSide *preSide = NULL;
	for (CCorner *c = src->head; 1; c = c->postSide->postCorner)
	{
		CCorner *c2 = new CCorner(this, c->x, c->y);
		c2->utility = c->utility;
		corners.Add(c2);
		c2->preSide = preSide;
		if (preSide)
			preSide->postCorner = c2;
		if (c==src->head) 
			head = c2;
		CSide *s = c->postSide;
		if (!s) 
			{ tail = c2; break; }
		CSide *s2 = new CSide(this, s->m_style);
		s2->utility = s->utility;
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

bool CContour::IsOnPcb() 
	{ return poly->IsOnPcb() && poly->contours.Contains(this); }

CNet *CContour::GetNet() { return poly->GetNet(); }

int CContour::GetLayer() { return poly->m_layer; }

void CContour::SaveUndoInfo()
{
	doc->m_undo_items.Add( new CUContour(this) );
	CIter<CCorner> ic (&corners);
	for (CCorner *c = ic.First(); c; c = ic.Next())
		doc->m_undo_items.Add( new CUCorner(c) );
	CIter<CSide> is (&sides);
	for (CSide *s = is.First(); s; s = is.Next())
		doc->m_undo_items.Add( new CUSide(s) );
}

void CContour::AppendSideAndCorner( CSide *s, CCorner *c, CCorner *after )
{
	// Append s+c into this connection after corner "after".  Assumes that s+c were constructed so that they point to "this"
	// Very similar to CConnect::AppendSegAndVertex.
	if (poly)
		poly->MustRedraw();
	corners.Add(c);
	sides.Add(s);
	CSide *nextSide = after->postSide;
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

void CContour::AppendCorner( int x, int y, int style )
{
	// CPT2 convenience method, basically a wrapper around AppendSideAndCorner()
	if (poly)
		poly->MustRedraw();
	bool bWasEmpty = corners.IsEmpty();
	CCorner *c = new CCorner(this, x, y);
	if (bWasEmpty)
		// First corner only. CCorner::CCorner has now setup this->head/tail, so we're done
		return;
	CSide *s = new CSide(this, style);
	AppendSideAndCorner(s, c, tail);
}

void CContour::Close(int style)
{
	if (head==tail) return;
	if (poly)
		poly->MustRedraw();
	CSide *s = new CSide(this, style);
	s->preCorner = tail;
	s->postCorner = head;
	tail->postSide = s;
	head->preSide = s;
	tail = head;
}

void CContour::Unclose()
{
	if (head!=tail || sides.GetSize()<2) return;
	if (poly)
		poly->MustRedraw();
	tail = tail->preSide->preCorner;
	sides.Remove(tail->postSide);
	tail->postSide = NULL;
	head->preSide = NULL;
}

CRect CContour::GetCornerBounds()
{
	CRect r;
	r.left = r.bottom = INT_MAX;
	r.right = r.top = INT_MIN;
	CIter<CCorner> ic (&corners);
	for (CCorner *c = ic.First(); c; c = ic.Next())
	{
		r.left = min( r.left, c->x );
		r.right = max( r.right, c->x );
		r.bottom = min( r.bottom, c->y );
		r.top = max( r.top, c->y );
	}
	return r;
}

void CContour::SetPoly( CPolyline *_poly )
{
	// CPT2 new.  Reassign this to a different polyline as a secondary contour (used when creating new cutouts).    
	if (poly)
		poly->MustRedraw(),
		poly->contours.Remove(this);
	poly = _poly;
	poly->contours.Add(this);
	poly->MustRedraw();
}

void CContour::Remove()
{
	poly->MustRedraw();
	if (this ==  poly->main)
		poly->Remove();
	else
		poly->contours.Remove(this);
}

bool CContour::TestPointInside(int x, int y) 
{
	enum { MAXPTS = 100 };
	ASSERT( head==tail );

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
		// now find all intersection points of line with contour sides
		npts = 0;
		CIter<CSide> is (&sides);
		for (CSide *s = is.First(); s; s = is.Next())
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
		nloops++;
		a += PCBU_PER_MIL/100;
	} while( npts%2 != 0 && nloops < 3 );
	ASSERT( npts%2==0 );	// odd number of intersection points, error

	// count intersection points to right of (x,y), if odd (x,y) is inside polyline
	int ncount = 0;
	for( int ip=0; ip<npts; ip++ )
		if( xx[ip] == x && yy[ip] == y )
			return FALSE;	// (x,y) is on a side, call it outside
		else if( xx[ip] > x )
			ncount++;
	return ncount%2 != 0;
}



CPolyline::CPolyline(CFreePcbDoc *_doc)
	: CPcbItem (_doc)
{ 
	main = NULL;
	m_layer = m_w = m_sel_box = m_hatch = m_nhatch = 0;
	m_gpc_poly = new gpc_polygon;
	m_gpc_poly->num_contours = 0;
}

CPolyline::CPolyline(CFreePcbDoc *_doc, int _uid):
	CPcbItem(_doc, _uid)
{
	main = NULL;
	m_gpc_poly = new gpc_polygon;
	m_gpc_poly->num_contours = 0;
}

CPolyline::CPolyline(CPolyline *src, bool bCopyContours)
	: CPcbItem (src->doc)
{
	// Copy constructor.  bCopyContours arg (true by default) indicates whether to recreate all the constituent contours.  Also copies
	// utility value on this and all subconstituents (occasionally useful)
	main = NULL;
	m_layer = src->m_layer;
	m_w = src->m_w;
	m_sel_box = src->m_sel_box;
	m_hatch = src->m_hatch;
	m_nhatch = src->m_nhatch;					// CPT2.  I guess...
	m_gpc_poly = new gpc_polygon;
	m_gpc_poly->num_contours = 0;
	utility = src->utility;
	if (!src->main || !bCopyContours)
		return;
	CIter<CContour> ictr (&src->contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		CContour *ctr2 = new CContour(this, ctr);
		contours.Add(ctr2);
		if (ctr==src->main) 
			main = ctr2;
	}
}

CPolyline::~CPolyline()
{
	FreeGpcPoly();
	delete m_gpc_poly;
}

void CPolyline::MarkConstituents(int util)
{
	// Mark the utility flags on this polyline and on its constituent contours, sides, and corners.
	utility = util;
	CIter<CContour> ictr (&contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		ctr->utility = util;
		ctr->sides.SetUtility(util);
		ctr->corners.SetUtility(util);
	}
}

CRect CPolyline::GetCornerBounds()
{
	CRect r;
	r.left = r.bottom = INT_MAX;
	r.right = r.top = INT_MIN;
	CIter<CContour> ictr (&contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		CRect r2 = ctr->GetCornerBounds();
		r.left = min(r.left, r2.left);
		r.right = max(r.right, r2.right);
		r.bottom = min(r.bottom, r2.bottom);
		r.top = max(r.top, r2.top);
	}
	return r;
}

CRect CPolyline::GetBounds()
{
	CRect r = GetCornerBounds();
	r.left -= m_w/2;
	r.right += m_w/2;
	r.bottom -= m_w/2;
	r.top += m_w/2;
	return r;
}

void CPolyline::Offset(int dx, int dy) 
{
	CIter<CContour> ictr (&contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		CIter<CCorner> ic (&ctr->corners);
		for (CCorner *c = ic.First(); c; c = ic.Next())
			c->x += dx,
			c->y += dy;
	}
}

int CPolyline::NumCorners() 
{
	CIter<CContour> ictr (&contours);
	int ret = 0;
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		ret += ctr->corners.GetSize();
	return ret;
}

int CPolyline::NumSides() 
{
	CIter<CContour> ictr (&contours);
	int ret = 0;
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		ret += ctr->sides.GetSize();
	return ret;
}

bool CPolyline::SetClosed(bool bClose)
{
	if (contours.GetSize()>1) 
		return false;
	if (bClose)
		if (IsClosed()) 
			return false;
		else
			main->Close(STRAIGHT);
	else
		if (!IsClosed())
			return false;
		else
			main->Unclose();
	return true;
}

void CPolyline::Copy(CPolyline *src)
{
	// CPT2 new.  Give this polyline copies of the contours found in "src".  Used when copying to the clipboard
	contours.RemoveAll();
	CIter<CContour> ictr (&src->contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		if (ctr->NumCorners()<2) continue;
		CContour *ctr2 = new CContour(this, ctr==src->main);
		CCorner *c = ctr->head;
		ctr2->AppendCorner( c->x, c->y );
		for (c = c->postSide->postCorner; c!=ctr->tail; c = c->postSide->postCorner)
			ctr2->AppendCorner( c->x, c->y, c->preSide->m_style );
		if (ctr->head == ctr->tail)
			ctr2->Close( ctr->head->preSide->m_style );
	}
}

void CPolyline::AddSidesTo(CHeap<CPcbItem> *arr)
{
	// CPT2 new.  Append the sides within this polyline to "arr".
	CIter<CContour> ictr (&contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		// Typecast is surely safe.
		arr->Add((CHeap<CPcbItem>*) &ctr->sides);
}

bool CPolyline::TestPointInside(int x, int y) 
{
	// CPT2 TODO.  Optimize this by caching a bounding rectangle for each area.
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
		CIter<CContour> ic (&contours);
		for (CContour *c = ic.First(); c; c = ic.Next())
		{
			CIter<CSide> is (&c->sides);
			for (CSide *s = is.First(); s; s = is.Next())
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

	// count intersection points to right of (x,y), if odd (x,y) is inside polyline
	int ncount = 0;
	for( int ip=0; ip<npts; ip++ )
		if( xx[ip] == x && yy[ip] == y )
			return FALSE;	// (x,y) is on a side, call it outside
		else if( xx[ip] > x )
			ncount++;
	return ncount%2 != 0;
}

void CPolyline::GetSidesInRect( CRect *r, CHeap<CPcbItem> *arr)
{
	// CPT2 new, helper for CFreePcbView::SelectItemsInRect().
	CIter<CContour> ictr (&contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		CIter<CSide> is (&ctr->sides);
		for (CSide *s = is.First(); s; s = is.Next())
		{
			CPoint pre (s->preCorner->x, s->preCorner->y);
			CPoint post (s->postCorner->x, s->postCorner->y);
			if (r->PtInRect(pre) && r->PtInRect(post))
				arr->Add(s);
		}
	}
}

bool CPolyline::IsClosed()
{
	// CPT2.  Trying a new definition of closedness.  Every contour within the polyline must have head equal to tail, and further no contour
	// can consist of one corner.  This will work decently when user is dragging a new cutout contour and it is still incomplete --- Hatch() will
	// then consider the polyline open and bail out.
	CIter<CContour> ictr (&contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		if (ctr->head != ctr->tail || ctr->NumCorners()<2)
			return false;
	return true;
}

// Test a polyline for self-intersection.
// Returns:
//	-1 if arcs intersect other sides
//	 0 if no intersecting sides
//	 1 if intersecting sides, but no intersecting arcs
// Also sets utility flag of area with return value
//
int CPolyline::TestPolygon()
{	
	ASSERT(IsClosed());

	// first, check for sides intersecting other sides, especially arcs 
	BOOL bInt = FALSE;
	BOOL bArcInt = FALSE;
	// make bounding rect for each contour.  CPT2 for each contour, store an index into "cr" temporarily in the utility field.
	CArray<CRect> cr;
	int i=0;
	CIter<CContour> ictr (&contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		CRect r = ctr->GetCornerBounds();
		cr.Add(r);
		ctr->utility = i++;
	}
	for (CContour *ctr1 = ictr.First(); ctr1; ctr1 = ictr.Next())
	{
		CIter<CSide> is1 (&ctr1->sides);
		for (CSide *s1 = is1.First(); s1; s1 = is1.Next())
		{
			CRect r1 = cr[ctr1->utility];
			CSide *s1prev = s1->preCorner->preSide;
			CSide *s1next = s1->postCorner->postSide;
			int style1 = s1->m_style;
			int x1i = s1->preCorner->x, y1i = s1->preCorner->y;
			int x1f = s1->postCorner->x, y1f = s1->postCorner->y;
			CIter<CContour> ictr2 (&contours, ctr1);
			for (CContour *ctr2 = ictr2.Next(); ctr2; ctr2 = ictr2.Next())
			{
				CRect r2 = cr[ctr2->utility];
				if( r1.left > r2.right
					|| r1.bottom > r2.top
					|| r2.left > r1.right
					|| r2.bottom > r1.top )
						// rectangles don't overlap, do nothing
						continue;
				CIter<CSide> is2 (&ctr2->sides);
				for (CSide *s2 = is2.First(); s2; s2 = is2.Next())
				{
					if (s1==s2 || s1prev==s2 || s1next==s2) continue;
					int style2 = s2->m_style;
					int x2i = s2->preCorner->x, y2i = s2->preCorner->y;
					int x2f = s2->postCorner->x, y2f = s2->postCorner->y;
					int ret = FindSegmentIntersections( x1i, y1i, x1f, y1f, style1, x2i, y2i, x2f, y2f, style2 );
					if( !ret ) continue;
					// intersection between non-adjacent sides
					bInt = TRUE;
					if( style1 != CPolyline::STRAIGHT || style2 != CPolyline::STRAIGHT )
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
		utility = -1;
	else if( bInt )
		utility = 1;
	else 
		utility = 0;
	return utility;
}

void CPolyline::Hatch()
{
	if( m_hatch == NO_HATCH )
	{
		m_nhatch = 0;
		return;
	}

	CDisplayList *dl = doc->m_dlist;
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
			CIter<CContour> ictr (&contours);
			for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
			{
				CIter<CSide> is (&ctr->sides);
				for (CSide *s = is.First(); s; s = is.Next())
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
				CDLElement * el = dl->Add( this, CDLElement::DL_HATCH, m_layer, DL_LINE, 1, 0, 0, 0,
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
				CDLElement * el = dl->Add( this, CDLElement::DL_HATCH, m_layer, DL_LINE, 1, 0, 0, 0,
					xx[ip], yy[ip], x1, y1, 0, 0 );
				dl_hatch.SetAtGrow(nhatch, el);
				el = dl->Add( this, CDLElement::DL_HATCH, m_layer, DL_LINE, 1, 0, 0, 0, 
					xx[ip+1], yy[ip+1], x2, y2, 0, 0 );
				dl_hatch.SetAtGrow(nhatch+1, el);
				nhatch += 2;
			}
		}
	}
	m_nhatch = nhatch;
	dl_hatch.SetSize( m_nhatch );
}


int CPolyline::Draw()
{
	int i_start_contour = 0;
	CDisplayList *dl = doc->m_dlist;
	if (!dl) 
		return NO_DLIST;
	if (bDrawn) 
		return ALREADY_DRAWN;

	CIter<CContour> ictr (&contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		// Draw corner selectors:
		if( m_sel_box )
		{
			CIter<CCorner> ic (&ctr->corners);
			for (CCorner *c = ic.First(); c; c = ic.Next())
			{
				int x = c->x, y = c->y;
				c->dl_sel = dl->AddSelector( c, m_layer, DL_HOLLOW_RECT, 
					1, 0, 0, x-m_sel_box, y-m_sel_box, 
					x+m_sel_box, y+m_sel_box, 0, 0 );
			}
		}

		// Draw sides and side selectors
		CIter<CSide> is (&ctr->sides);
		int side_sel_w = IsOutline()? m_w: m_w*4;						// CPT2 For areas, smcutouts, and board-outlines, we want the side selectors to be 
																		// wider for improved visibility.  But not with fp-editor outlines...
		for (CSide *s = is.First(); s; s = is.Next())
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
					1, side_sel_w, 0, xi, yi, xf, yf, 0, 0 );
		}
	}

	if( m_hatch )
		Hatch();
	bDrawn = true;
	return NOERR;
}

// undraw polyline by removing all graphic elements from display list
//
void CPolyline::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl || !IsDrawn()) return;

	CIter<CContour> ictr (&contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		CIter<CSide> is (&ctr->sides);
		for (CSide *s = is.First(); s; s = is.Next())
		{
			dl->Remove( s->dl_el );
			dl->Remove( s->dl_sel );
			s->dl_el = s->dl_sel = NULL;
		}
		CIter<CCorner> ic (&ctr->corners);
		for (CCorner *c = ic.First(); c; c = ic.Next())
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

void CPolyline::Highlight()
{
	CIter<CContour> ic (&contours);
	for (CContour *c = ic.First(); c; c = ic.Next())
		c->Highlight();
}

void CPolyline::SetVisible( BOOL visible ) 
{	
	CDisplayList *dl = doc->m_dlist;
	CIter<CContour> ictr (&contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		CIter<CSide> is (&ctr->sides);
		for (CSide *s = is.First(); s; s = is.Next())
			dl->Set_visible( s->dl_el, visible ); 
	}
	for( int ih=0; ih<m_nhatch; ih++ )
		dl->Set_visible( dl_hatch[ih], visible ); 
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

void CPolyline::MakeGpcPoly( CContour *ctr0, CArray<CArc> * arc_array )
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

	// Create a temporary CHeap of contours in which the external (main) contour is guaranteed to come first:
	CHeap<CContour> tmp;
	tmp.Add(main);
	CIter<CContour> ictr0 (&contours);
	for (CContour *ctr = ictr0.First(); ctr; ctr = ictr0.Next())
		if (ctr!=main)
			tmp.Add(ctr);
	// Now go thru the contours in the preferred order.
	CIter<CContour> ictr (&tmp);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
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
		CIter<CSide> is (&ctr->sides);
		for (CSide *s = is.First(); s; s = is.Next())
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
		CCorner *c = ctr->head;
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
				if( style == CPolyline::ARC_CW )
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

void CPolyline::FreeGpcPoly()
{
	if( m_gpc_poly->num_contours )
	{
		delete m_gpc_poly->contour->vertex;
		delete m_gpc_poly->contour;
		delete m_gpc_poly->hole;
	}
	m_gpc_poly->num_contours = 0;
}

void CPolyline::NormalizeWithGpc( bool bRetainArcs )
{
	// Use the General Polygon Clipping Library to clip "this"
	// If this results in new polygons, create them (invoke CPolyline::CreateCompatible(), which
	// will attach the same net/membership-list as "this").
	// If bRetainArcs == TRUE, try to retain arcs in polys
	// CPT2 converted.  NB does not do any drawing/undrawing.  TODO perhaps one day it would be nice to
	// figure out a way to generalize this to any type of CPolyline.
	CArray<CArc> arc_array;
	if( bRetainArcs )
		MakeGpcPoly( NULL, &arc_array );
	else
		MakeGpcPoly( NULL, NULL );

	// now, recreate poly
	// first, find outside contours and create new CArea's if necessary
	int n_ext_cont = 0;
	CHeap<CPolyline> arr;
	for( int ic=0; ic<m_gpc_poly->num_contours; ic++ )
	{
		if( m_gpc_poly->hole[ic] ) continue;
		CPolyline *poly;
		n_ext_cont++;
		if( n_ext_cont == 1 )
			poly = this,
			poly->MustRedraw(),
			contours.RemoveAll();
		else
			poly = CreateCompatible(),
			poly->MustRedraw(),
			arr.Add(poly);
		CContour *ctr = new CContour(poly, true);						// NB the new contour will be poly->main
		for( int i=0; i<m_gpc_poly->contour[ic].num_vertices; i++ )
		{
			int x = m_gpc_poly->contour[ic].vertex[i].x;
			int y = m_gpc_poly->contour[ic].vertex[i].y;
			CCorner *c = new CCorner(ctr, x, y);						// Constructor adds corner to ctr->corners and will also set ctr->head/tail
			if (i>0)
			{
				CSide *s = new CSide(ctr, STRAIGHT);
				ctr->AppendSideAndCorner(s, c, ctr->tail);
			}
		}
		ctr->Close(STRAIGHT);
		}

	// now add cutouts to the cpolylines
	CIter<CPolyline> ip (&arr);
	for( int ic=0; ic<m_gpc_poly->num_contours; ic++ ) 
	{
		if( !m_gpc_poly->hole[ic] ) continue;
		// Find external poly containing this cutout.
		CPolyline *ext_poly = NULL;
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
					for( CPolyline *poly = ip.First(); poly; poly = ip.Next() )
						if( poly->TestPointInside( x, y ) )
							{ ext_poly = poly; break; }
			}
		ASSERT( ext_poly );

		CContour *ctr = new CContour(ext_poly, false);						// NB the new contour will not be the main one
		for( int i=0; i<m_gpc_poly->contour[ic].num_vertices; i++ )
		{
			int x = m_gpc_poly->contour[ic].vertex[i].x;
			int y = m_gpc_poly->contour[ic].vertex[i].y;
			CCorner *c = new CCorner(ctr, x, y);							// Constructor adds corner to ctr->corners; on iteration 0 it sets ctr->head/tail
			if (i>0)
			{
				CSide *s = new CSide(ctr, STRAIGHT);
				ctr->AppendSideAndCorner(s, c, ctr->tail);
			}
		}
		ctr->Close(STRAIGHT);
	}
	if( bRetainArcs )
		RestoreArcs( &arc_array, &arr );
	FreeGpcPoly();
}

int CPolyline::TestIntersection( CPolyline *poly2, bool bCheckArcIntersections )
{
	// Test for intersection of 2 copper areas
	// returns: 0 if no intersection
	//			1 if intersection
	//			2 if arcs intersect.  But if bCheckArcIntersections is false, return 1 as soon as any intersection whatever is found
	// CPT2 adapted from CNetList::TestAreaIntersection.  By adding the bCheckArcIntersections param, this covers CNetList::TestAreaIntersections too
	// TODO I don't think this will work if poly2 is totally within this.  We might consider using 
	//   the CMyBitmap class, or gpc?
	// First see if polygons are on same layer
	CPolyline *poly1 = this;
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
	CIter<CContour> ictr1 (&poly1->contours);
	for (CContour *ctr1 = ictr1.First(); ctr1; ctr1 = ictr1.Next())
	{
		CIter<CSide> is1 (&ctr1->sides);
		for (CSide *s1 = is1.First(); s1; s1 = is1.Next())
		{
			int xi1 = s1->preCorner->x, yi1 = s1->preCorner->y;
			int xf1 = s1->postCorner->x, yf1 = s1->postCorner->y;
			int style1 = s1->m_style;
			CIter<CContour> ictr2 (&poly2->contours);
			for (CContour *ctr2 = ictr2.First(); ctr2; ctr2 = ictr2.Next())
			{
				CIter<CSide> is2 (&ctr2->sides);
				for (CSide *s2 = is2.First(); s2; s2 = is2.Next())
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
						if( style1 != CPolyline::STRAIGHT || style2 != CPolyline::STRAIGHT )
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

bool CPolyline::TestIntersections()
{
	// CPT2.  Generalizes old CNetList::TestAreaIntersections().  Returns true if "this" intersects any other polyline of the same type + layer.
	// Invokes CPolyline::TestIntersection() with bTestArcIntersections set to false for efficiency.
	CHeap<CPolyline> ia;
	GetCompatiblePolylines(&ia);
	CIter<CPolyline> ip (&ia);
	for (CPolyline *p = ip.First(); p; p = ip.Next())
	{
		if (p==this) continue;
		if (TestIntersection(p, false)) return true;
	}
	return false;
}

int CPolyline::CombinePolyline( CPolyline *poly2 )
{
	// CPT2. Adapted from CNetList::CombineAreas.  Unions "poly2" onto "this" polyline.  Does not deal with the removal of poly2 from the net in
	// the case that this and poly2 are copper areas:
	// that's the caller's job.  Assumes that the intersection check has already taken place.
	// Returns 1 if we actually combine 'em, or 0 in the unusual cases where they're actually disjoint.
	CPolyline * poly1 = this;
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
		CContour *ctr = new CContour(this, !union_gpc->hole[ic]);		// NB the new contour will be this->main if the current gpc contour is a non-hole
		for( int i=0; i<union_gpc->contour[ic].num_vertices; i++ )
		{
			int x = union_gpc->contour[ic].vertex[i].x;
			int y = union_gpc->contour[ic].vertex[i].y;
			CCorner *c = new CCorner(ctr, x, y);			// Constructor adds corner to ctr->corners (and may also set ctr->head/tail if
															// it was previously empty)
			if (i>0)
			{
				CSide *s = new CSide(ctr, STRAIGHT);
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

int CPolyline::ClipPolygon( bool bMessageBoxArc, bool bMessageBoxInt, bool bRetainArcs )
{	
	// Process a polyline that has been modified, by clipping its polygon against itself.
	// This may change the number of polylines in the parent list.
	// If bMessageBoxInt == TRUE, shows message when clipping occurs.
	// If bMessageBoxArc == TRUE, shows message when clipping can't be done due to arcs.
	// Returns:
	//	-1 if arcs intersect other sides, so polygon can't be clipped
	//	 0 if no intersecting sides
	//	 1 if intersecting sides
	// Also sets poly->utility flags if polylines are modified
	// CPT2 converted and generalized from old area-based function.  Subroutines take care of calling MustRedraw(), as appropriate.
	int test = TestPolygon();				// this sets utility flag
	if( test == -1 && !bRetainArcs )
		test = 1;
	if( test == -1 )
	{
		// arc intersections, don't clip unless bRetainArcs == FALSE
		if( bMessageBoxArc )
		{
			CDlgMyMessageBox dlg;
			if (dlg.Initialize( SELF_INTERSECTION_ARC_WARNING, false, UID() ))
				dlg.DoModal();
		}
		return -1;	// arcs intersect with other sides, error
	}

	if( test == 1 )
	{
		// non-arc intersections, clip the polygon
		if( bMessageBoxInt )
		{
			CDlgMyMessageBox dlg;
			if (dlg.Initialize( SELF_INTERSECTION_WARNING, false, UID() ))
				dlg.DoModal();
		}
		NormalizeWithGpc( bRetainArcs );						// NB Routine will change "this", and will attach any new polylines in the appropriate places
	}
	return test;
}

void CPolyline::CombinePolylines( CHeap<CPolyline> *pa, BOOL bMessageBox )
{
	// Static function; checks all polylines in "pa" for intersections, combining them if found
	// CPT2 NB trying it out without the old bUseUtility param
	// Sets utility flag = 1 for any areas modified, and calls virtual function Remove() as necessary.
	// If an area has self-intersecting arcs, doesn't try to combine it
	// CPT2 converted, generalized from old CNetList::CombinAllAreasInNet().  The subroutines are in charge of calling MustRedraw() for affected areas.
	if (pa->GetSize()<2) return;

	// start by testing all polylines to set utility flags.
	CIter<CPolyline> ip (pa);
	for (CPolyline *p = ip.First(); p; p = ip.Next())
		p->TestPolygon();
	// now loop through all combinations
	BOOL message_shown = FALSE, mod_p1;											
	CIter<CPolyline> ip1 (pa);
	for (CPolyline *p1 = ip1.First(); p1; p1 = ip1.Next())
	{
		do {
			CRect b1 = p1->GetCornerBounds();
			mod_p1 = FALSE;
			CIter<CPolyline> ip2 (pa, p1);
			ip2.Next();														// Advance to polyline AFTER p1.
			for (CPolyline *p2 = ip2.Next(); p2; p2 = ip2.Next())
			{
				// if (p1->GetLayer() != p2->GetLayer()) continue;			// Should be safe to omit this check (pa was set up to contain polys all on one layer)
				if (p1->utility == -1 || p2->utility == -1) continue;
				CRect b2 = p2->GetCornerBounds();
				if ( b1.left > b2.right || b1.right < b2.left
						|| b1.bottom > b2.top || b1.top < b2.bottom ) continue;
				// CPT2 obsolete I think:  (plus, having dumped the old utility2 field), I have other uses for "utility" now)
				// if( bUseUtility && !p1->utility && !p2->utility ) continue;
				// check p2 against p1 
				int ret = p1->TestIntersection( p2, true );
				if( ret == 1 )
				{
					ret = p1->CombinePolyline( p2 );
					if (ret == 0) continue;
					pa->Remove( p2 );
					p2->Remove();												// Virtual func. undraws and detaches polyline from the appropriate arrays.
					if( ret == 1 && bMessageBox )
					{
						CDlgMyMessageBox dlg;
						if (dlg.Initialize( INTERSECTION_WARNING, false, p1->UID(), p2->UID() ))
							dlg.DoModal();
					}
					mod_p1 = TRUE;
				}
				else if( ret == 2 && bMessageBox )
				{
					CDlgMyMessageBox dlg;
					if (dlg.Initialize( INTERSECTION_ARC_WARNING, false, p1->UID(), p2->UID() ))
						dlg.DoModal();
				}
			}
		}
		while (mod_p1);		// if p1 is modified, we need to check it against all the other areas again.
	}
}


bool CPolyline::PolygonModified( bool bMessageBoxArc, bool bMessageBoxInt )
{	
	// Process a polyline that has been modified, by clipping its polygon against
	// itself and the polygons for any other areas on the same net.
	// This may wind up adding or removing polylines from the parent array that contains "this".
	// If bMessageBoxXXX == TRUE, shows message boxes when clipping occurs.
	// Returns false if an error occured, i.e. if arcs intersect other sides, so polygon can't be clipped.
	// CPT2 converted.  Subroutines take care of calling MustRedraw() on affected area(s), as needed.
	if (IsOnPcb())
	{
		int test = ClipPolygon( bMessageBoxArc, bMessageBoxInt );
		if( test == -1 )
		{
			// CPT2 always gives an error msg in case of intersecting arcs:
			CString s ((LPCSTR) IDS_ErrorUnableToClipPolygon);
			AfxMessageBox( s );
			return false;
		}
		// now see if we need to clip against other areas
		BOOL bCheckAllAreas = test==1 || TestIntersections();
		if( bCheckAllAreas )
		{
			CHeap<CPolyline> arr;
			GetCompatiblePolylines( &arr );
			CIter<CPolyline> ip (&arr);
			for (CPolyline *poly = ip.First(); poly; poly = ip.Next())
				poly->SaveUndoInfo();
			CombinePolylines( &arr, bMessageBoxInt );
		}
	}
	if (this->IsArea())
		GetNet()->SetThermals();
	return true;
}

void CPolyline::RestoreArcs( CArray<CArc> *arc_array, CHeap<CPolyline> *pa )
{
	// Restore arcs to a polygon where they were replaced with steps
	// If pa != NULL, also use polygons in pa array
	// CPT2 converted.
	CHeap<CPolyline> pa2;
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
		CCorner *arc_start = NULL, *arc_end = NULL;
		// loop through polys
		CIter<CPolyline> ip (&pa2);
		for (CPolyline *p = ip.First(); p; p = ip.Next())
		{
			CIter<CContour> ictr (&p->contours);
			for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
			{
				if (ctr->corners.GetSize() <= n_steps) continue;
				CIter<CCorner> ic (&ctr->corners);
				for (CCorner *c = ic.First(); c; c = ic.Next())
				{
					if (c->x != arc_xi || c->y != arc_yi) continue;
					// Check if we find a corner at location (arc_xf,arc_yf), either n_steps positions forward from c or n_steps positions back
					CCorner *c2 = c;
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
		CContour *ctr = arc_start->contour;
		ctr->poly->MustRedraw();
		for (CCorner *c = arc_start; c != arc_end; c = c->postSide->postCorner)
		{
			if (c != arc_start) 
				ctr->corners.Remove(c);
			ctr->sides.Remove(c->postSide);
		}
		CSide *new_side = new CSide(ctr, style);
		arc_start->postSide = new_side;
		new_side->preCorner = arc_start;
		arc_end->preSide = new_side;
		new_side->postCorner = arc_end;
		// As a safety precaution, set contour's head and tail equal to arc_start.  (It's conceivable that the old head/tail was in the midst
		// of the segments that are getting eliminated.)
		ctr->head = ctr->tail = arc_start;
	}
}




CArea::CArea(CNet *_net, int _layer, int _hatch, int _w, int _sel_box)
	: CPolyline(_net->doc)
{ 
	m_layer = _layer;
	m_hatch = _hatch;
	m_net = _net;
	m_net->areas.Add(this);
	m_w = _w;
	m_sel_box = _sel_box;
}

CArea::CArea(CFreePcbDoc *_doc, int _uid):
	CPolyline(_doc, _uid)
{
	m_net = NULL;
}

bool CArea::IsOnPcb()
{
	if (!m_net->IsOnPcb()) return false;
	return m_net->areas.Contains(this);
}

void CArea::SaveUndoInfo()
{
	doc->m_undo_items.Add( new CUArea(this) );
	CIter<CContour> ictr (&contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		ctr->SaveUndoInfo();
}

int CArea::Draw()
{
	if (!m_net->bVisible)
		return NOERR;
	return CPolyline::Draw();
}

void CArea::Remove()
{
	// CPT2. User wants to delete area, so detach it from the network (garbage collector will later delete this object and its
	// constituents for good).
	Undraw();
	m_net->areas.Remove(this);
	m_net->SetThermals();
}

void CArea::SetNet( CNet *net )
{
	// CPT2 new.  Assign this area to the given net.  Does not deal with potential changes to thermals.
	if (m_net) 
		m_net->areas.Remove(this);
	m_net = net;
	m_net->areas.Add(this);
}

void CArea::GetCompatiblePolylines( CHeap<CPolyline> *arr )
{
	// CPT2 new.  Virtual function in class CPolyline.  The idea is to put into "arr" all other polylines that might potentially be merged with
	// "this", in the event that they intersect.  For areas, this will be all the areas in the same net that are in the same layer.
	CIter<CArea> ia (&m_net->areas);
	for (CArea *a = ia.First(); a; a = ia.Next())
		if (a->m_layer == m_layer)
			arr->Add(a);
}

CPolyline *CArea::CreateCompatible() 
	// CPT2 new.  Virtual function in class CPolyline.  Returns a new polyline of the same specs as this (but with an empty contour array)
	{ return new CArea(m_net, m_layer, m_hatch, m_w, m_sel_box); }



CSmCutout::CSmCutout(CFreePcbDoc *_doc, int layer, int hatch)
	: CPolyline(_doc)
{ 
	doc->smcutouts.Add(this); 
	m_layer = layer; 
	m_w = 2*NM_PER_MIL;
	m_sel_box = 10*NM_PER_MIL;
	m_hatch = hatch;
}

CSmCutout::CSmCutout(CFreePcbDoc *_doc, int _uid):
	CPolyline(_doc, _uid)
	{ }

bool CSmCutout::IsOnPcb()
	{ return doc->smcutouts.Contains(this); }

void CSmCutout::SaveUndoInfo()
{
	doc->m_undo_items.Add( new CUSmCutout(this) );
	CIter<CContour> ictr (&contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		ctr->SaveUndoInfo();
}

void CSmCutout::GetCompatiblePolylines( CHeap<CPolyline> *arr )
{
	// CPT2 new.  Virtual function in class CPolyline.  The idea is to put into "arr" all other polylines that might potentially be merged with
	// "this", in the event that they intersect.  For smcutouts, this will be all the smc's in the doc in the same layer.
	CIter<CSmCutout> ism (&doc->smcutouts);
	for (CSmCutout *sm = ism.First(); sm; sm = ism.Next())
		if (sm->m_layer == m_layer)
			arr->Add(sm);
}

CPolyline *CSmCutout::CreateCompatible() 
	// CPT2 new.  Virtual function in class CPolyline.  Returns a new polyline of the same specs as this (but with an empty contour array)
	{ return new CSmCutout(doc, m_layer, m_hatch); }


void CSmCutout::Remove()
{
	Undraw();
	doc->smcutouts.Remove(this);
}


CBoard::CBoard(CFreePcbDoc *_doc) 
	: CPolyline(_doc)
{ 
	doc->boards.Add(this);
	m_layer = LAY_BOARD_OUTLINE; 
	m_w = 2*NM_PER_MIL;
	m_sel_box = 10*NM_PER_MIL;
	m_hatch = CPolyline::NO_HATCH;
}

CBoard::CBoard(CFreePcbDoc *_doc, int _uid):
	CPolyline(_doc, _uid)
	{ }

bool CBoard::IsOnPcb()
	{ return doc->boards.Contains(this); }

void CBoard::SaveUndoInfo()
{
	doc->m_undo_items.Add( new CUBoard(this) );
	CIter<CContour> ictr (&contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		ctr->SaveUndoInfo();
}

void CBoard::Remove()
{
	Undraw();
	doc->boards.Remove(this);
}

void CBoard::GetCompatiblePolylines( CHeap<CPolyline> *arr )
	// CPT2 new.  Virtual function in class CPolyline.  For class CBoard, this function is only useful (as far as I am aware)
	// as a subroutine within CFreePcbView::TryToReselectCorner().  Therefore it should suffice to do the following, though 
	// theoretically we could scan through doc->others and retrieve all other board outlines as well.
	{ arr->Add(this); }


COutline::COutline(CShape *_shape, int layer, int w)
	: CPolyline (_shape->doc)
{
	m_layer = layer;
	m_w = w;
	shape = _shape;
	shape->m_outlines.Add(this);
}

COutline::COutline(COutline *src, CShape *_shape)
	: CPolyline(src) 
{ 
	shape = _shape;
	shape->m_outlines.Add(this);
}

bool COutline::IsOnPcb() 
{
	return shape->IsOnPcb() && shape->m_outlines.Contains(this);
}

CPolyline *COutline::CreateCompatible() 
	// CPT2 new.  Virtual function in class CPolyline.  Returns a new polyline of the same specs as this (but with an empty contour array)
	{ return new COutline(shape, m_layer, m_w); }

void COutline::SaveUndoInfo()
{
	doc->m_undo_items.Add( new CUOutline(this) );
	CIter<CContour> ictr (&contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		ctr->SaveUndoInfo();
}


