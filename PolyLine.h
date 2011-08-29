// PolyLine.h ... definition of CPolyLine class
//
// A polyline contains one or more contours, where each contour
// is defined by a list of corners and side-styles
// There may be multiple contours in a polyline.
// The last contour may be open or closed, any others must be closed.
// All of the corners and side-styles are concatenated into 2 arrays,
// separated by setting the end_contour flag of the last corner of 
// each contour.
//
// When used for copper areas, the first contour is the outer edge 
// of the area, subsequent ones are "holes" in the copper.
//
// If a CDisplayList pointer is provided, the polyline can draw itself 

#pragma once
#include "DisplayList.h"
#include "gpc_232.h"
#include "Cuid.h"
#include "ids.h"

class polygon;
class CPolyLine;

struct undo_poly {
	CPolyLine * poly;
	int uid;
	id root_id;
	int layer;
	int width;
	int sel_box;
	int hatch;
	int ncorners;
	// array of undo_corners starts here
};

struct undo_corner {
	int uid;
	int x, y;
	int end_contour;
	int side_uid;
	int side_style;			// style is for following side
};

class CArc {
public: 
	enum{ MAX_STEP = 50*25400 };	// max step is 20 mils
	enum{ MIN_STEPS = 18 };		// min step is 5 degrees
	int style;
	int xi, yi, xf, yf;
	int n_steps;	// number of straight-line segments in gpc_poly 
	BOOL bFound;
};

class CPolyCorner
{
public:
	CPolyCorner();
	~CPolyCorner();
	int x;
	int y;
	BOOL end_contour;	// flags the end of a closed contour
	int utility;
	int m_uid;
	dl_element * dl_corner_sel;
};

class CPolySide
{
public:
	CPolySide();
	~CPolySide();
	int m_style;
	int m_uid;
	dl_element * dl_side;
	dl_element * dl_side_sel;
};

class CPolyLine
{
public:
	enum { STRAIGHT, ARC_CW, ARC_CCW };	// side styles
	enum { NO_HATCH, DIAGONAL_FULL, DIAGONAL_EDGE }; // hatch styles
	enum { DEF_SIZE = 50, DEF_ADD = 50 };	// number of array elements to add at a time

	// constructors/destructor
	CPolyLine();
	~CPolyLine();

	// functions for modifying polyline
	void Start( int layer, int w, int sel_box, int x, int y,
		int hatch, id * id, void * ptr );
	void AppendCorner( int x, int y, int style = STRAIGHT, BOOL bDraw=TRUE );
	void InsertCorner( int ic, int x, int y );
	void DeleteCorner( int ic, BOOL bDraw=TRUE );
	BOOL MoveCorner( int ic, int x, int y, BOOL bEnforceCircularArcs=FALSE );
	void Close( int style = STRAIGHT, BOOL bDraw=TRUE );
	void RemoveContour( int icont );

	// drawing functions
	void SetDlist( CDisplayList * dl );
	void HighlightSide( int is );
	void HighlightCorner( int ic );
	void StartDraggingToInsertCorner( CDC * pDC, int ic, int x, int y, int crosshair = 1 );
	void StartDraggingToMoveCorner( CDC * pDC, int ic, int x, int y, int crosshair = 1 );
	void CancelDraggingToInsertCorner( int ic );
	void CancelDraggingToMoveCorner( int ic );
	void Undraw();
	void Draw( CDisplayList * dl = NULL );
	void Hatch();
	void MakeVisible( BOOL visible = TRUE );
	void MoveOrigin( int x_off, int y_off );
	void SetSideVisible( int is, int visible );

	// misc. functions
	CRect GetBounds();
	CRect GetCornerBounds();
	CRect GetCornerBounds( int icont );
	void Copy( CPolyLine * src );
	BOOL TestPointInside( int x, int y );
	BOOL TestPointInsideContour( int icont, int x, int y );
	int TestIntersection( CPolyLine * poly );
	void AppendArc( int xi, int yi, int xf, int yf, int xc, int yc, int num );

	// undo functions
	int SizeOfUndoRecord();
	void SetFromUndo( undo_poly * un_poly );
	void CreateUndoRecord( undo_poly * un_poly );

	// access functions
	int GetUID();
	int GetNumCorners();
	int GetNumSides();
	int GetClosed();
	int GetNumContours();
	int GetContour( int ic );
	int GetContourStart( int icont );
	int GetContourEnd( int icont );
	int GetContourSize( int icont );
	int GetCornerIndexByUID( int uid );
	int GetSideIndexByUID( int uid );
	int GetX( int ic );
	int GetY( int ic );
	int GetEndContour( int ic );
	int GetUtility(){ return utility; };
	int GetUtility( int ic ){ return corner[ic].utility; };
	int GetLayer();
	int GetW();
	int GetCornerUID( int ic ){ return corner[ic].m_uid; };
	int GetSideUID( int is );
	int GetSideStyle( int is );
	id  GetId();
	void * GetPtr(){ return m_ptr; };
	int GetSelBoxSize();
	CDisplayList * GetDisplayList(){ return m_dlist; };
	int GetHatch(){ return m_hatch; }

	void SetCornerUID( int ic, int uid );
	void SetSideUID( int is, int uid );
	void SetClosed( BOOL bClosed );
	void SetX( int ic, int x );
	void SetY( int ic, int y );
	void SetEndContour( int ic, BOOL end_contour );
	void SetUtility( int u ){ utility = u; };
	void SetUtility( int ic, int utility ){ corner[ic].utility = utility; };
	void SetLayer( int layer );
	void SetW( int w );
	void SetSideStyle( int is, int style );
	void SetId( id * id );
	void SetPtr( void * ptr ){ m_ptr = ptr; };
	void SetSelBoxSize( int sel_box );
	void SetHatch( int hatch ){ Undraw(); m_hatch = hatch; Draw(); };
	void SetDisplayList( CDisplayList * dl );

	// GPC functions
	int MakeGpcPoly( int icontour=0, CArray<CArc> * arc_array=NULL );
	int FreeGpcPoly();
	gpc_polygon * GetGpcPoly(){ return m_gpc_poly; };
	int NormalizeWithGpc( CArray<CPolyLine*> * pa=NULL, BOOL bRetainArcs=FALSE );
	int RestoreArcs( CArray<CArc> * arc_array, CArray<CPolyLine*> * pa=NULL );
	CPolyLine * MakePolylineForPad( int type, int x, int y, int w, int l, int r, int angle );
	void AddContourForPadClearance( int type, int x, int y, int w, 
						int l, int r, int angle, int fill_clearance,
						int hole_w, int hole_clearance, BOOL bThermal=FALSE, int spoke_w=0 );
	void ClipGpcPolygon( gpc_op op, CPolyLine * poly );

	// PHP functions
	int MakePhpPoly();
	void FreePhpPoly();
	void ClipPhpPolygon( int php_op, CPolyLine * poly );

private:
	CDisplayList * m_dlist;		// display list 
	id m_root_id;	// root id
	int m_uid;		// use this uid if root_id doesn't provide one
	void * m_ptr;	// pointer to parent object (or NULL)
	int m_layer;	// layer to draw on
	int m_w;		// line width
	int m_sel_box;	// corner selection box width/2
	int m_ncorners;	// number of corners
	int utility;
	CArray <CPolyCorner> corner;	// array of corners
	CArray <CPolySide> side;	// array of sides
	int m_hatch;	// hatch style, see enum above
	int m_nhatch;	// number of hatch lines
	CArray <dl_element*>  dl_hatch;	// hatch lines	
	gpc_polygon * m_gpc_poly;	// polygon in gpc format
	polygon * m_php_poly;
	BOOL bDrawn;
};
