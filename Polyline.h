// Polyline.h --- header file for classes related most closely to polylines, all of which are descendants of cpcb_item:
//  ccorner, cside, ccontour, cpolyline, carea, csmcutout, cboard, coutline

#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "PcbItem.h"
#include "DisplayList.h"
#include "Undo.h"
#include "stdafx.h"


class ccorner: public cpcb_item
{
public:
	int x, y;
	ccontour *contour;				// CPT2.
	cside *preSide, *postSide;		// CPT2

	ccorner(ccontour *_contour, int _x, int _y);
	ccorner(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();	
	bool IsCorner() { return true; }
	bool IsAreaCorner();
	bool IsBoardCorner();
	bool IsSmCorner(); 
	bool IsOutlineCorner();
	ccorner *ToCorner() { return this; }
	int GetTypeBit();
	cnet2 *GetNet();
	int GetLayer();
	cpolyline *GetPolyline();
	cundo_item *MakeUndoItem()
		{ return new cucorner(this); }

	bool IsOnCutout();
	void Highlight();
	bool Move( int x, int y, BOOL bEnforceCircularArcs=FALSE );
	void StartDragging( CDC * pDC, int x, int y, int crosshair = 1 );
	void CancelDragging();
	void Remove();
};

class cside: public cpcb_item
{
public:
	int m_style;
	ccontour *contour;
	ccorner *preCorner, *postCorner;

	cside(ccontour *_contour, int _style);
	cside(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsSide() { return true; }
	bool IsAreaSide();
	bool IsBoardSide();
	bool IsSmSide();
	bool IsOutlineSide();
	cside *ToSide() { return this; }
	int GetTypeBit();
	cnet2 *GetNet();
	int GetLayer();	
	cpolyline *GetPolyline();
	bool IsOnCutout();
	cundo_item *MakeUndoItem()
		{ return new cuside(this); }
	char GetDirectionLabel();

	void InsertCorner(int x, int y);
	bool Remove( carray<cpolyline> *arr );
	void Highlight();
	void SetVisible();
	void SetStyle();
	void StartDraggingNewCorner( CDC * pDC, int x, int y, int crosshair );
	void CancelDraggingNewCorner( );
};

class ccontour: public cpcb_item
{
public:
	// All CPT2.
	carray<ccorner> corners;	// Array of corners.  NB entries not necessarily in geometrical order
	carray<cside> sides;		// Array of sides
	ccorner *head, *tail;		// For when we need to trace thru geometrically.
	cpolyline *poly;			// The parent polyline

	ccontour(cpolyline *_poly, bool bMain);
	ccontour(cpolyline *_poly, ccontour *src);								// Copy constructor
	ccontour(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsContour() { return true; }
	ccontour *ToContour() { return this; }
	int GetTypeBit() { return bitContour; }
	int GetLayer();
	cnet2 *GetNet();
	cpolyline *GetPolyline() { return poly; }
	cundo_item *MakeUndoItem()
		{ return new cucontour(this); }
	void SaveUndoInfo();

	int NumCorners() { return corners.GetSize(); } 
	void AppendSideAndCorner( cside *s, ccorner *c, ccorner *after );
	void AppendCorner(int x, int y, int style = STRAIGHT);
	void Close(int style = STRAIGHT);
	void Unclose();
	CRect GetCornerBounds();
	bool TestPointInside( int x, int y );
	void SetPoly( cpolyline *_poly );
	void Remove();
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
	ccontour *main;							// CPT2.  The primary, outer contour.
	carray<ccontour> contours;				// CPT2.  All contours, including main
	int m_layer;							// layer to draw on
	int m_w;								// line width
	int m_sel_box;							// corner selection box width/2
	int m_hatch;							// hatch style, see enum above
	int m_nhatch;							// number of hatch lines
	CArray <dl_element*>  dl_hatch;			// hatch lines.  Note that we use CArray with dl-elements generally
	gpc_polygon * m_gpc_poly;				// polygon in gpc format

public:
	// constructors/destructor
	cpolyline(CFreePcbDoc *_doc);
	cpolyline(cpolyline *src, bool bCopyContours=true);
	cpolyline(CFreePcbDoc *_doc, int _uid);
	~cpolyline();

	bool IsPolyline() { return true; }
	cpolyline *ToPolyline() { return this; }
	int GetLayer() { return m_layer; }
	cpolyline *GetPolyline() { return this; }

	// functions for creating and modifying polyline
	virtual void Remove() { }														// CPT2 new.  Check out the versions in derived classes.
	void MarkConstituents(int util);

	int Draw();	
	void Undraw();
	void Hatch();
	void Highlight();
	void SetVisible( BOOL visible = TRUE );

	// misc. functions
	CRect GetBounds();
	CRect GetCornerBounds();
	bool TestPointInside( int x, int y );

	// access functions
	int NumCorners();														// Total corners in all contours.
	int NumSides();				
	void GetSidesInRect( CRect *r, carray<cpcb_item> *arr );				// CPT2 new.
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
	bool SetClosed( bool bClosed );	
	void Offset(int dx, int dy);
	void Copy(cpolyline *src);
	void AddSidesTo(carray<cpcb_item> *arr);

	// Functions for normalizing and combining intersecting polylines
	virtual void GetCompatiblePolylines( carray<cpolyline> *arr ) { }
	virtual cpolyline *CreateCompatible() { return NULL; }
	void MakeGpcPoly( ccontour *ctr = NULL, CArray<CArc> * arc_array=NULL );
	void FreeGpcPoly();
	void NormalizeWithGpc( bool bRetainArcs=false );									// First version was in class carea2, am now generalizing
	int TestPolygon();
	int TestIntersection( cpolyline * poly2, bool bTestArcIntersections=true );
	bool TestIntersections();															// First version was in carea2, now generalizing
	int CombinePolyline( cpolyline *poly2 );
	void RestoreArcs( CArray<CArc> *arc_array, carray<cpolyline> *pa=NULL );
	int ClipPolygon( bool bMessageBoxArc, bool bMessageBoxInt, bool bRetainArcs=true);	// Generalization of old carea2 func.
	static void CombinePolylines( carray<cpolyline> *pa, BOOL bMessageBox );			// Generalization of old CNetList::CombineAllAreasInNet()
																						// Testing it out minus the old bUseUtility param.
	virtual bool PolygonModified( bool bMessageBoxArc, bool bMessageBoxInt );			// Generalization of old carea2 func.
																						// Virtual, because at least for now cboard is overriding it (to
																						// do nothing)
};

// carea2: describes a copper area in a net.
class carea2 : public cpolyline
{
public:
	cnet2 * m_net;		// parent net

	carea2(cnet2 *_net, int layer, int hatch, int w, int sel_box);
	carea2(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsArea() { return true; }
	carea2 *ToArea() { return this; }
	int GetTypeBit() { return bitArea; }
	cnet2 *GetNet() { return m_net; }
	cundo_item *MakeUndoItem()
		{ return new cuarea(this); }
	void SaveUndoInfo();
	
	int Draw();

	void GetCompatiblePolylines( carray<cpolyline> *arr );
	cpolyline *CreateCompatible();
	void Remove();
	void SetNet( cnet2 *net );
};


class csmcutout : public cpolyline
{
	// Represents solder-mask cutouts, which are always closed polylines
public:
	csmcutout(CFreePcbDoc *_doc, int layer, int hatch);
	csmcutout(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsSmCutout() { return true; }
	csmcutout *ToSmCutout() { return this; }
	int GetTypeBit() { return bitSmCutout; }
	cundo_item *MakeUndoItem()
		{ return new cusmcutout(this); }
	void SaveUndoInfo();
	void Remove();
	cpolyline *CreateCompatible();
	void GetCompatiblePolylines( carray<cpolyline> *arr );
};


class cboard : public cpolyline
{
public:
	// Represents board outlines.
	cboard(CFreePcbDoc *_doc);
	cboard(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsBoard() { return true; }
	cboard *ToBoard() { return this; }
	int GetTypeBit() { return bitBoard; }
	cundo_item *MakeUndoItem()
		{ return new cuboard(this); }
	void SaveUndoInfo();
	void Remove();
	void GetCompatiblePolylines( carray<cpolyline> *arr );
	bool PolygonModified( bool bMessageBoxArc, bool bMessageBoxInt ) { return true; }	// Overriding virtual func in cpolyline.
};

class coutline : public cpolyline
{
public:
	// Represents outlines within footprints.  These don't have to be closed.
	cshape *shape;

	coutline(cshape *_shape, int layer, int w);
	coutline(coutline *src, cshape *_shape);
	coutline(CFreePcbDoc *_doc, int uid)
		: cpolyline(_doc, uid) 
		{ shape = NULL; }

	virtual bool IsOnPcb();
	bool IsOutline() { return true; }
	coutline *ToOutline() { return this; }
	int GetTypeBit() { return bitOutline; }
	cundo_item *MakeUndoItem()
		{ return new cuoutline(this); }
	void SaveUndoInfo();
	cpolyline *CreateCompatible();
};


