// Polyline.h --- header file for classes related most closely to polylines, all of which are descendants of cpcb_item:
//  ccorner, cside, ccontour, cpolyline, carea, csmcutout, cboard, coutline

#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "PcbItem.h"
#include "DisplayList.h"
#include "UndoNew.h"
#include "stdafx.h"


class ccorner: public cpcb_item
{
public:
	int x, y;
	ccontour *contour;				// CPT2.
	// dl_element * dl_corner_sel;  // Use cpcb_item::dl_sel
	cside *preSide, *postSide;		// CPT2

	ccorner(ccontour *_contour, int _x, int _y);		// Done in cpp
	ccorner(CFreePcbDoc *_doc, int _uid);

	bool IsValid();										// Done in cpp
	bool IsCorner() { return true; }
	bool IsAreaCorner();								// Done in cpp
	bool IsBoardCorner();								// Done in cpp
	bool IsSmCorner(); 									// Done in cpp
	bool IsOutlineCorner(); 							// Done in cpp
	ccorner *ToCorner() { return this; }
	int GetTypeBit();									// Done in cpp
	cnet2 *GetNet();									// Done in cpp
	int GetLayer();										// Done in cpp
	cpolyline *GetPolyline();							// Done in cpp
	cundo_item *MakeUndoItem()
		{ return new cucorner(this); }

	bool IsOnCutout();									// Done in cpp
	void Highlight();									// Done in cpp, derived from old CPolyLine::HighlightCorner().  See also CNetList::HighlightAreaCorner.
	bool Move( int x, int y, BOOL bEnforceCircularArcs=FALSE );			// Done in cpp, derived from CNetList::MoveAreaCorner/CPolyLine::MoveCorner
	void StartDragging( CDC * pDC, int x, int y, int crosshair = 1 );	// Done in cpp, derived from CPolyLine::StartDraggingToMoveCorner
	void CancelDragging();												// Done in cpp, derived from CPolyLine::CancelDraggingToMoveCorner
	void Remove();
};

class cside: public cpcb_item
{
public:
	int m_style;					// TODO change to style?
	ccontour *contour;				// CPT2
	// dl_element * dl_side;		// Use base dl_el
	// dl_element * dl_side_sel;	// Use base dl_sel
	ccorner *preCorner, *postCorner;

	cside(ccontour *_contour, int _style);
	cside(CFreePcbDoc *_doc, int _uid);

	bool IsValid();
	bool IsSide() { return true; }
	bool IsAreaSide();								// Done in cpp
	bool IsBoardSide();								// Done in cpp
	bool IsSmSide(); 								// Done in cpp
	bool IsOutlineSide(); 							// Done in cpp
	cside *ToSide() { return this; }
	int GetTypeBit();								// Done in cpp
	cnet2 *GetNet();								// Done in cpp
	int GetLayer();									// Done in cpp
	cpolyline *GetPolyline();						// Done in cpp
	bool IsOnCutout();								// Done in cpp
	cundo_item *MakeUndoItem()
		{ return new cuside(this); }
	char GetDirectionLabel();						// Done in cpp

	void InsertCorner(int x, int y);				// Done in cpp
	bool Remove( carray<cpolyline> *arr );			// CPT2 new, done in cpp
	void Highlight();		// Done in cpp, derived from cpolyline::HighlightSide()
	void SetVisible();		// CPT2
	void SetStyle();
	void StartDraggingNewCorner( CDC * pDC, int x, int y, int crosshair );  // Done in cpp, derived from CPolyLine::StartDraggingToInsertCorner
	void CancelDraggingNewCorner( );										// Done in cpp, derived from CPolyLine::CancelDraggingToInsertCorner
};

class ccontour: public cpcb_item
{
public:
	// All CPT2.
	carray<ccorner> corners;	// Array of corners.  NB entries not necessarily in geometrical order
	carray<cside> sides;		// Array of sides
	ccorner *head, *tail;		// For when we need to trace thru geometrically.
	cpolyline *poly;			// The parent polyline

	ccontour(cpolyline *_poly, bool bMain);									// Done in cpp
	ccontour(cpolyline *_poly, ccontour *src);								// Copy constructor, done in cpp
	ccontour(CFreePcbDoc *_doc, int _uid);

	bool IsValid();															// Done in cpp
	bool IsContour() { return true; }
	ccontour *ToContour() { return this; }
	int GetTypeBit() { return bitContour; }
	int GetLayer();															// Done in cpp
	cnet2 *GetNet();														// Done in cpp
	cpolyline *GetPolyline() { return poly; }
	cundo_item *MakeUndoItem()
		{ return new cucontour(this); }
	void SaveUndoInfo();

	void Highlight()														// CPT2 TODO used?
	{
		citer<cside> is (&sides);
		for (cside *s = is.First(); s; s = is.Next())
			s->Highlight();
	}
	
	int NumCorners() { return corners.GetSize(); } 
	void AppendSideAndCorner( cside *s, ccorner *c, ccorner *after );		// Done in cpp
	void AppendCorner(int x, int y, int style = STRAIGHT);					// Done in cpp
	void Close(int style = STRAIGHT);										// Done in cpp
	void Unclose();															// Done in cpp
	CRect GetCornerBounds();												// Done in cpp
	bool TestPointInside( int x, int y );									// Done in cpp
	void SetPoly( cpolyline *_poly );										// Done in cpp
	void Remove();															// Done in cpp, derived from CPolyLine::RemoveContour
};

class CArc {
	// Helper class for gpc conversion routines in cpolyline.
public: 
	enum{ MAX_STEP = 50*25400 };	// max step is 20 mils
	enum{ MIN_STEPS = 18 };		// min step is 5 degrees
	int style;
	int xi, yi, xf, yf;
	int n_steps;	// number of straight-line segments in gpc_poly 
	BOOL bFound;
};

class cpolyline: public cpcb_item
{
public:
	ccontour *main;				// CPT2.  The primary, outer contour.
	carray<ccontour> contours;	// CPT2.  All contours, including main
	int m_layer;				// layer to draw on
	int m_w;					// line width
	int m_sel_box;				// corner selection box width/2
	int m_hatch;				// hatch style, see enum above
	int m_nhatch;				// number of hatch lines
	CArray <dl_element*>  dl_hatch;	// hatch lines.  Use CArray with dl-elements generally?
	gpc_polygon * m_gpc_poly;	// polygon in gpc format

public:
	// constructors/destructor
	cpolyline(CFreePcbDoc *_doc);														// Done in cpp
	cpolyline(cpolyline *src, bool bCopyContours=true);									// Done in cpp
	cpolyline(CFreePcbDoc *_doc, int _uid);

	bool IsPolyline() { return true; }
	cpolyline *ToPolyline() { return this; }
	int GetLayer() { return m_layer; }
	cpolyline *GetPolyline() { return this; }

	// functions for creating and modifying polyline
	virtual void Remove() { }														// CPT2 new.  Check out the versions in derived classes.
	void MarkConstituents(int util);												// CPT2. Done in cpp.

	int Draw();																					// Done in cpp.  Old arg removed.
	void Undraw();																				// Done in cpp
	void Hatch();																				// Done in cpp
	void Highlight();																			// Done in cpp
	void SetVisible( BOOL visible = TRUE );

	// misc. functions
	CRect GetBounds();								// Done in cpp
	CRect GetCornerBounds();						// Done in cpp
	bool TestPointInside( int x, int y );			// Done in cpp

	// access functions
	int NumCorners();														// Total corners in all contours.  Done in cpp.
	int NumSides();				
	void GetSidesInRect( CRect *r, carray<cpcb_item> *arr );				// CPT2 new, done in cpp.
	bool IsClosed();
	int NumContours() { return contours.GetSize(); }
	int W() { return m_w; }
	int SelBoxSize();
	int GetHatch() { return m_hatch; }

	gpc_polygon * GetGpcPoly() { return m_gpc_poly; }

	void SetLayer( int layer );
	void SetW( int w );
	void SetSelBoxSize( int sel_box ) { m_sel_box = sel_box; }
	void SetHatch( int hatch )
		{ Undraw(); m_hatch = hatch; Draw(); }
	bool SetClosed( bool bClosed );														// CPT2 descended from CPolyLine function
	void Offset(int dx, int dy);														// Done in cpp
	void Copy(cpolyline *src);															// CPT2 new, done in cpp
	void AddSidesTo(carray<cpcb_item> *arr);											// CPT2 new, done in cpp

	// Functions for normalizing and combining intersecting polylines
	virtual void GetCompatiblePolylines( carray<cpolyline> *arr ) { }					// CPT2 new, done in cpp
	virtual cpolyline *CreateCompatible() { return NULL; }								// CPT2 new, done in cpp
	void MakeGpcPoly( ccontour *ctr = NULL, CArray<CArc> * arc_array=NULL );			// Done in cpp, derived from CPolyline function
	void FreeGpcPoly();																	// Done in cpp.
	void NormalizeWithGpc( bool bRetainArcs=false );									// Done in cpp.  First version was in class carea2, am now generalizing
	int TestPolygon();																	// Done in cpp, derived from CNetList::TestAreaPolygon()
	int TestIntersection( cpolyline * poly2, bool bTestArcIntersections=true );			// Done in cpp, derived from CNetList::TestAreaIntersection().
	bool TestIntersections();															// Done in cpp.  First version was in carea2, now generalizing;
																						//   covers the old CNetList::TestAreaIntersections().
	int CombinePolyline( cpolyline *poly2 );											// Done in cpp, derived from CNetList::CombineAreas
	void RestoreArcs( CArray<CArc> *arc_array, carray<cpolyline> *pa=NULL );			// Done in cpp.  Originally in CPolyLine
	int ClipPolygon( bool bMessageBoxArc, bool bMessageBoxInt, bool bRetainArcs=true);	// Done in cpp.  Generalization of old carea2 func.
	static void CombinePolylines( carray<cpolyline> *pa, BOOL bMessageBox );			// Done in cpp, generalization of old CNetList::CombineAllAreasInNet()
																						// Testing it out minus the old bUseUtility param.
	virtual bool PolygonModified( bool bMessageBoxArc, bool bMessageBoxInt );			// Done in cpp.  Generalization of old carea2 func.
																						// Virtual, because at least for now cboard is overriding it (to
																						// do nothing)
	// PHP functions.  CPT2 TODO apparently obsolete.
	int MakePhpPoly();
	void FreePhpPoly();
	void ClipPhpPolygon( int php_op, cpolyline * poly );
};

// carea2: describes a copper area in a net.
class carea2 : public cpolyline
{
public:
	cnet2 * m_net;		// parent net

	carea2(cnet2 *_net, int layer, int hatch, int w, int sel_box);	// Done in cpp
	carea2(CFreePcbDoc *_doc, int _uid);

	bool IsValid();
	bool IsArea() { return true; }
	carea2 *ToArea() { return this; }
	int GetTypeBit() { return bitArea; }
	cnet2 *GetNet() { return m_net; }
	cundo_item *MakeUndoItem()
		{ return new cuarea(this); }
	void SaveUndoInfo();
	
	int Draw();

	void GetCompatiblePolylines( carray<cpolyline> *arr );			// CPT2 new, done in cpp
	cpolyline *CreateCompatible();									// CPT2 new, done in cpp
	void Remove();													// Done in cpp
	// int Complete( int style );									// CPT2 TODO consider dumping
	void SetNet( cnet2 *net );										// Done in cpp
};


class csmcutout : public cpolyline
{
	// Represents solder-mask cutouts, which are always closed polylines
public:
	csmcutout(CFreePcbDoc *_doc, int layer, int hatch);									// Done in cpp
	csmcutout(CFreePcbDoc *_doc, int _uid);
	~csmcutout() { }

	bool IsValid();																		// Done in cpp
	bool IsSmCutout() { return true; }
	csmcutout *ToSmCutout() { return this; }
	int GetTypeBit() { return bitSmCutout; }
	cundo_item *MakeUndoItem()
		{ return new cusmcutout(this); }
	void SaveUndoInfo();
	void Remove();																		// Done in cpp
	cpolyline *CreateCompatible();														// CPT2 new, done in cpp
	void GetCompatiblePolylines( carray<cpolyline> *arr );								// CPT2 new, done in cpp
};


class cboard : public cpolyline
{
public:
	// Represents board outlines.
	cboard(CFreePcbDoc *_doc);					// Done in cpp
	cboard(CFreePcbDoc *_doc, int _uid);

	bool IsValid();								// Done in cpp
	bool IsBoard() { return true; }
	cboard *ToBoard() { return this; }
	int GetTypeBit() { return bitBoard; }
	cundo_item *MakeUndoItem()
		{ return new cuboard(this); }
	void SaveUndoInfo();
	void Remove();																		// Done in cpp
	void GetCompatiblePolylines( carray<cpolyline> *arr );								// Done in cpp.
	bool PolygonModified( bool bMessageBoxArc, bool bMessageBoxInt ) { return true; }	// Overriding virtual func in cpolyline.
};

class coutline : public cpolyline
{
public:
	// Represents outlines within footprints.  These don't have to be closed.
	coutline(CFreePcbDoc *_doc, int layer, int w);		// Done in cpp
	coutline(coutline *src):
		cpolyline(src) { }

	bool IsValid();										// Done in cpp
	bool IsOutline() { return true; }
	coutline *ToOutline() { return this; }
	int GetTypeBit() { return bitOutline; }
	cpolyline *CreateCompatible();						// CPT2 new, done in cpp
};


