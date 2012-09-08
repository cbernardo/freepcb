// Polyline.h --- header file for classes related most closely to polylines, all of which are descendants of CPcbItem:
//  CCorner, CSide, CContour, CPolyline, CArea, CSmCutout, CBoard, COutline

#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "PcbItem.h"
#include "DisplayList.h"
#include "Undo.h"
#include "stdafx.h"


class CCorner: public CPcbItem
{
public:
	int x, y;
	CContour *contour;				// CPT2.
	CSide *preSide, *postSide;		// CPT2

	CCorner(CContour *_contour, int _x, int _y);
	CCorner(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();	
	bool IsCorner() { return true; }
	bool IsAreaCorner();
	bool IsBoardCorner();
	bool IsSmCorner(); 
	bool IsOutlineCorner();
	CCorner *ToCorner() { return this; }
	int GetTypeBit();
	CNet *GetNet();
	int GetLayer();
	CPolyline *GetPolyline();
	CUndoItem *MakeUndoItem()
		{ return new CUCorner(this); }

	bool IsOnCutout();
	void Highlight();
	bool Move( int x, int y, BOOL bEnforceCircularArcs=FALSE );
	void StartDragging( CDC * pDC, int x, int y, int crosshair = 1 );
	void CancelDragging();
	void Remove();
};

class CSide: public CPcbItem
{
public:
	int m_style;
	CContour *contour;
	CCorner *preCorner, *postCorner;

	CSide(CContour *_contour, int _style);
	CSide(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsSide() { return true; }
	bool IsAreaSide();
	bool IsBoardSide();
	bool IsSmSide();
	bool IsOutlineSide();
	CSide *ToSide() { return this; }
	int GetTypeBit();
	CNet *GetNet();
	int GetLayer();	
	CPolyline *GetPolyline();
	bool IsOnCutout();
	CUndoItem *MakeUndoItem()
		{ return new CUSide(this); }
	char GetDirectionLabel();

	void InsertCorner(int x, int y);
	bool Remove( CHeap<CPolyline> *arr );
	void Highlight();
	void SetVisible();
	void SetStyle();
	void StartDraggingNewCorner( CDC * pDC, int x, int y, int crosshair );
	void CancelDraggingNewCorner( );
};

class CContour: public CPcbItem
{
public:
	// All CPT2.
	CHeap<CCorner> corners;	// Array of corners.  NB entries not necessarily in geometrical order
	CHeap<CSide> sides;		// Array of sides
	CCorner *head, *tail;		// For when we need to trace thru geometrically.
	CPolyline *poly;			// The parent polyline

	CContour(CPolyline *_poly, bool bMain);
	CContour(CPolyline *_poly, CContour *src);								// Copy constructor
	CContour(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsContour() { return true; }
	CContour *ToContour() { return this; }
	int GetTypeBit() { return bitContour; }
	int GetLayer();
	CNet *GetNet();
	CPolyline *GetPolyline() { return poly; }
	CUndoItem *MakeUndoItem()
		{ return new CUContour(this); }
	void SaveUndoInfo();
	void Highlight();

	int NumCorners() { return corners.GetSize(); } 
	void AppendSideAndCorner( CSide *s, CCorner *c, CCorner *after );
	void AppendCorner(int x, int y, int style = STRAIGHT);
	void Close(int style = STRAIGHT);
	void Unclose();
	CRect GetCornerBounds();
	bool TestPointInside( int x, int y );
	void SetPoly( CPolyline *_poly );
	void Remove();
};

class CArc {
	// Helper class for gpc conversion routines in CPolyline.
public: 
	enum{ MAX_STEP = 50*25400 };	// max step is 20 mils
	enum{ MIN_STEPS = 18 };		// min step is 5 degrees
	int style;
	int xi, yi, xf, yf;
	int n_steps;	// number of straight-line segments in gpc_poly 
	BOOL bFound;
};

class CPolyline: public CPcbItem
{
public:
	CContour *main;							// CPT2.  The primary, outer contour.
	CHeap<CContour> contours;				// CPT2.  All contours, including main
	int m_layer;							// layer to draw on
	int m_w;								// line width
	int m_sel_box;							// corner selection box width/2
	int m_hatch;							// hatch style, see enum above
	int m_nhatch;							// number of hatch lines
	CArray <CDLElement*>  dl_hatch;			// hatch lines.  Note that we use CArray with dl-elements generally
	gpc_polygon * m_gpc_poly;				// polygon in gpc format

public:
	// constructors/destructor
	CPolyline(CFreePcbDoc *_doc);
	CPolyline(CPolyline *src, bool bCopyContours=true);
	CPolyline(CFreePcbDoc *_doc, int _uid);
	~CPolyline();

	bool IsPolyline() { return true; }
	CPolyline *ToPolyline() { return this; }
	int GetLayer() { return m_layer; }
	CPolyline *GetPolyline() { return this; }

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
	int NumCorners();																		// Total corners in all contours.
	int NumSides();				
	void GetSidesInRect( CRect *r, CHeap<CPcbItem> *arr, bool bAllOrNothing=false );		// CPT2 new.
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
	void Copy(CPolyline *src);
	void AddSidesTo(CHeap<CPcbItem> *arr);

	// Functions for normalizing and combining intersecting polylines
	virtual void GetCompatiblePolylines( CHeap<CPolyline> *arr ) { }
	virtual CPolyline *CreateCompatible() { return NULL; }
	void MakeGpcPoly( CContour *ctr = NULL, CArray<CArc> * arc_array=NULL );
	void FreeGpcPoly();
	void NormalizeWithGpc( bool bRetainArcs=false );									// First version was in class CArea, am now generalizing
	int TestPolygon();
	int TestIntersection( CPolyline * poly2, bool bTestArcIntersections=true );
	bool TestIntersections();															// First version was in CArea, now generalizing
	int CombinePolyline( CPolyline *poly2 );
	void RestoreArcs( CArray<CArc> *arc_array, CHeap<CPolyline> *pa=NULL );
	int ClipPolygon( bool bMessageBoxArc, bool bMessageBoxInt, bool bRetainArcs=true);	// Generalization of old CArea func.
	static void CombinePolylines( CHeap<CPolyline> *pa, BOOL bMessageBox );			// Generalization of old CNetList::CombineAllAreasInNet()
																						// Testing it out minus the old bUseUtility param.
	virtual bool PolygonModified( bool bMessageBoxArc, bool bMessageBoxInt );			// Generalization of old CArea func.
																						// Virtual, because at least for now CBoard is overriding it (to
																						// do nothing)
};

// CArea: describes a copper area in a net.
class CArea : public CPolyline
{
public:
	CNet * m_net;		// parent net

	CArea(CNet *_net, int layer, int hatch, int w, int sel_box);
	CArea(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsArea() { return true; }
	CArea *ToArea() { return this; }
	int GetTypeBit() { return bitArea; }
	CNet *GetNet() { return m_net; }
	CUndoItem *MakeUndoItem()
		{ return new CUArea(this); }
	void SaveUndoInfo();
	
	int Draw();

	void GetCompatiblePolylines( CHeap<CPolyline> *arr );
	CPolyline *CreateCompatible();
	void Remove();
	void SetNet( CNet *net );
};


class CSmCutout : public CPolyline
{
	// Represents solder-mask cutouts, which are always closed polylines
public:
	CSmCutout(CFreePcbDoc *_doc, int layer, int hatch);
	CSmCutout(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsSmCutout() { return true; }
	CSmCutout *ToSmCutout() { return this; }
	int GetTypeBit() { return bitSmCutout; }
	CUndoItem *MakeUndoItem()
		{ return new CUSmCutout(this); }
	void SaveUndoInfo();
	void Remove();
	CPolyline *CreateCompatible();
	void GetCompatiblePolylines( CHeap<CPolyline> *arr );
};


class CBoard : public CPolyline
{
public:
	// Represents board outlines.
	CBoard(CFreePcbDoc *_doc);
	CBoard(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsBoard() { return true; }
	CBoard *ToBoard() { return this; }
	int GetTypeBit() { return bitBoard; }
	CUndoItem *MakeUndoItem()
		{ return new CUBoard(this); }
	void SaveUndoInfo();
	void Remove();
	void GetCompatiblePolylines( CHeap<CPolyline> *arr );
	bool PolygonModified( bool bMessageBoxArc, bool bMessageBoxInt ) { return true; }	// Overriding virtual func in CPolyline.
};

class COutline : public CPolyline
{
public:
	// Represents outlines within footprints.  These don't have to be closed.
	CShape *shape;

	COutline(CShape *_shape, int layer, int w);
	COutline(COutline *src, CShape *_shape);
	COutline(CFreePcbDoc *_doc, int uid)
		: CPolyline(_doc, uid) 
		{ shape = NULL; }

	virtual bool IsOnPcb();
	bool IsOutline() { return true; }
	COutline *ToOutline() { return this; }
	int GetTypeBit() { return bitOutline; }
	CUndoItem *MakeUndoItem()
		{ return new CUOutline(this); }
	void SaveUndoInfo();
	CPolyline *CreateCompatible();
};


