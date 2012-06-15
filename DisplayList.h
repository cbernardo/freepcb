// DisplayList.h : header file for CDisplayList class
//
#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "afxwin.h"
#include "ids.h"
#include "layers.h"
#include "LinkList.h"
#include "rgb.h"
#include "smfontutil.h"

//#define DL_MAX_LAYERS 32
#define DL_MAGIC		2674

#define PCBU_PER_WU		25400	// conversion from PCB units to world units

// graphics element types
enum 
{
	DL_NONE = 0,
	DL_LINE,			// line segment with round end-caps  
	DL_CIRC,			// filled circle
	DL_DONUT,			// annulus
	DL_SQUARE,			// filled square
	DL_RECT,			// filled rectangle
	DL_RRECT,			// filled rounded rectangle
	DL_OVAL,			// filled oval
	DL_OCTAGON,			// filled octagon
	DL_HOLE,			// hole, shown as circle
	DL_HOLLOW_CIRC,		// circle outline
	DL_HOLLOW_RECT,		// rectangle outline
	DL_HOLLOW_RRECT,	// rounded rectangle outline
	DL_HOLLOW_OVAL,		// oval outline
	DL_HOLLOW_OCTAGON,	// octagon outline
	DL_RECT_X,			// rectangle outline with X
	DL_POINT,			// shape to highlight a point
	DL_ARC_CW,			// arc with clockwise curve
	DL_ARC_CCW,			// arc with counter-clockwise curve
	DL_CURVE_CW,		// circular clockwise curve
	DL_CURVE_CCW,		// circular counter-clockwise curve
	DL_CENTROID,		// circle and X
	DL_X				// X
};

// dragging line shapes
enum
{
	DS_NONE = 0,
	DS_LINE_VERTEX,			// vertex between two lines
	DS_LINE,				// line
	DS_ARC_STRAIGHT,		// straight line (used when drawing polylines)
	DS_ARC_CW,				// clockwise arc (used when drawing polylines)
	DS_ARC_CCW,				// counterclockwise arc 
	DS_CURVE_CW,			// clockwise curve
	DS_CURVE_CCW,			// counterclockwise curve 
	DS_SEGMENT				// line segment (with rubber banding of linking segments)
};

// styles of line segment when dragging line or line vertex
enum
{
	DSS_STRAIGHT = 100,		// straight line
	DSS_ARC_CW,				// clockwise arc
	DSS_ARC_CCW,			// counterclockwise arc
	DSS_NONE				// piece is missing (used in Move Segment for missing final segment)
};

// inflection modes for DS_LINE and DS_LINE_VERTEX
enum
{
	IM_NONE = 0,
	IM_90_45,
	IM_45_90,
	IM_90
};


struct CDrawInfo
{
	CDC      *DC;
	CDC      *DC_Master;

	CPen      erase_pen;
	CBrush    erase_brush;

	CPen      line_pen;
	CBrush    fill_brush;

    COLORREF  layer_color[2];
    int       size_of_2_pixels;

    mutable int nlines;

    CDrawInfo() : nlines(0) {}
};


#include "DrawingElement.h"

// CPT:  new system to replace Brian's "drawing jobs"
class CDisplayLayer {
public:
	dl_element *elements;					// A doubly-linked list of display elements.

	CDisplayLayer()
		{ elements = NULL; }
	void Add(dl_element* el) {
		// Add el to the head of the linked list.
		if (elements) elements->prev = el;
		el->next = elements;
		elements = el;
		el->displayLayer = this;
		}
	void Draw(CDrawInfo &di, bool bHiliteSegs);
};

class CHitInfo
{
	// Was a struct within Brian's CDL_job class, for some reason.  Now it's an independent class.
public:
	int layer;
	id  ID;
	void *ptr;
	int priority;
	double dist;			// CPT r294
};

// end CPT

class CDisplayList
{
private:
    friend dl_element;

	// display-list parameters for each layer
	CDisplayLayer layers[MAX_LAYERS];	// CPT
	C_RGB m_rgb[MAX_LAYERS];            // layer colors
	int m_layer_in_order[MAX_LAYERS];	// array of layers in draw order
	int m_order_for_layer[MAX_LAYERS];	// draw order for each layer

	// window parameters
	int m_pcbu_per_wu;	// i.e. nm per world unit
	CRect m_client_r;	// client rect (pixels)
	CRect m_screen_r;	// client rect (screen coords)
	int m_pane_org_x;	// left border of drawing pane (pixels)
	int m_pane_org_y;	// bottom border of drawing pane (pixels)
	int m_bottom_pane_h;	// height of bottom pane
	CDC *memDC;			// pointer to memory DC
	CDC *memDC2;		// CPT experimental

public:
	BOOL m_vis[MAX_LAYERS];		// layer visibility flags

	double m_scale;		// world units per pixel
	int m_org_x;		// world x-coord of left side of screen (world units)
	int m_org_y;		// world y-coord of bottom of screen (world units)
	int m_max_x;		// world x_coord of right side of screen (world units)
	int m_max_y;		// world y_coord of top of screen (world units)

private:
	int w_ext_x, w_ext_y;	// window extents (world units)
	int v_ext_x, v_ext_y;	// viewport extents (pixels)
	double m_wu_per_pixel_x;	// ratio w_ext_x/v_ext_x (world units per pixel)
	double m_wu_per_pixel_y;	// ratio w_ext_y/v_ext_y (world units per pixel)
	double m_pcbu_per_pixel_x;
	double m_pcbu_per_pixel_y;

	int m_ratline_w;

	// general dragging parameters
	int m_drag_angle;	// angle of rotation of selection rectangle (starts at 0)
	int m_drag_side;	// 0 = no change, 1 = switch to opposite
	int m_drag_vert;	// 1 if item being dragged is a vertical part

	// parameters for dragging polyline sides and trace segments
	// that can be modified while dragging
	int m_drag_flag;		// 1 if dragging something
	int m_drag_shape;		// shape
	int m_last_drag_shape;	// last shape drawn
	int m_drag_x;			// last cursor position for dragged shape
	int m_drag_y;
	int m_drag_xb;			// start of rubberband drag line "before" selected line
	int m_drag_yb;
	int m_drag_xi;			// start of rubberband drag line
	int m_drag_yi;
	int m_drag_xf;			// end of rubberband drag line
	int m_drag_yf;
	int m_drag_xe;			// start of rubberband drag line at end of trio
	int m_drag_ye;
	int m_drag_layer_0;		// line layer
	int m_drag_w0;			// line width
	int m_drag_style0;		// line style
	int m_drag_layer_1;		// line layer
	int m_drag_w1;			// line width
	int m_drag_style1;		// line style
	int m_inflection_mode;	// inflection mode
	int m_last_inflection_mode;	// last mode drawn
	// extra parameters when dragging vertex between 2 line segments
	int m_drag_style2;
	int m_drag_layer_2;
	int m_drag_w2;
	// parameters used to draw leading via if necessary
	int m_drag_layer_no_via;
	int m_drag_via_w;
	int m_drag_via_holew;
	int m_drag_via_drawn;

	// arrays of lines and ratlines being dragged
	// these can be rotated and flipped while being dragged
	int m_drag_layer;				// layer
	int m_drag_max_lines;			// max size of array for line segments
	int m_drag_num_lines;			// number of line segments to drag
	CPoint * m_drag_line_pt;		// array of relative coords for line endpoints
	int m_drag_max_ratlines;		// max size of ratline array
	int m_drag_num_ratlines;		// number of ratlines to drag
	CPoint * m_drag_ratline_start_pt;	// absolute coords for ratline start points
	CPoint * m_drag_ratline_end_pt;		// relative coords for ratline endpoints
	int m_drag_ratline_width;
	CArray<CPoint> m_drag_ratline_drag_pt;
	CArray<CPoint> m_drag_ratline_stat_pt;

	// cursor parameters
	int m_cross_hairs;	// 0 = none, 1 = cross-hairs, 2 = diagonals
	CPoint m_cross_left, m_cross_right, m_cross_top, m_cross_bottom; // end-points
	CPoint m_cross_topleft, m_cross_topright, m_cross_botleft, m_cross_botright;

	// grid
	int m_visual_grid_on;
	double m_visual_grid_spacing;	// in world units

	// font
	SMFontUtil * m_fontutil;


    dl_element * CreateDLE( int gtype );

public:
	CDisplayList( int pcbu_per_wu, SMFontUtil * fontutil );
	~CDisplayList();

	SMFontUtil * GetSMFontUtil(){ return m_fontutil; };
	void SetVisibleGrid( BOOL on, double grid );
	void SetMapping( CRect *client_r, CRect *screen_r, int pane_org_x, int pane_bottom_h, double scale, int org_x, int org_y );
	void SetDCToWorldCoords( CDC *pDC, CDC *mDC, CDC *mDC2, int pcbu_org_x, int pcbu_org_y );		// CPT added mDC2 (experimental)
	void SetLayerRGB( int layer, C_RGB color );
	void SetLayerVisible( int layer, BOOL vis );
	void SetLayerDrawOrder( int layer, int order )
			{ m_layer_in_order[order] = layer; m_order_for_layer[layer] = order; };

	void Scale_pcbu_to_wu(CRect &rect);
	void Scale_wu_to_pixels(CRect &rect);

	dl_element * CreateDLE( id id, void * ptr, int layer, int gtype, int visible,
	                        int w, int holew, int clearancew,
	                        int x, int y, int xf, int yf, int xo, int yo, int radius,
	                        int orig_layer );

    dl_element * MorphDLE( dl_element *pFrom, int to_gtype );

    // Create and add elements
	dl_element * Add( id id, void * ptr, int glayer, int gtype, int visible,
						int w, int holew, int clearancew,
						int x, int y, int xf, int yf, int xo, int yo,
						int radius=0,
						int orig_layer=LAY_SELECTION );

	dl_element * AddSelector( id id, void * ptr, int glayer, int gtype, int visible,
								int w, int holew,
								int x, int y, int xf, int yf, int xo, int yo,
								int radius=0 );

    // Add elements
	void Add( dl_element * element );

    // Remove elements
	void RemoveAll();
	void RemoveAllFromLayer( int layer );
	id Remove( dl_element * element );

	void Draw( CDC * pDC );
	int HighLight( int gtype, int x, int y, int xf, int yf, int w, int orig_layer=LAY_SELECTION );
	int CancelHighLight();
	// CPT r294.  Changed args quite a bit:  exclude-ids are out; hit_info is now a CArray; new bCtrl param added (depends on ctrl-key state).
	// Now always sorts hit_info, and returns the number of hits, not the highest-priority index.
	int TestSelect( int x, int y, CArray<CHitInfo> *hit_info,
					id * include_id, int n_include_ids, bool bCtrl = false );
	int StartDraggingArray( CDC * pDC, int x, int y, int vert, int layer, int crosshair = 1 );
	int StartDraggingRatLine( CDC * pDC, int x, int y, int xf, int yf, int layer,
		int w, int crosshair = 1 );
	int StartDraggingRectangle( CDC * pDC, int x, int y, int xi, int yi,
										int xf, int yf, int vert, int layer );
	int StartDraggingLineVertex( CDC * pDC, int x, int y, int xi, int yi,
									int xf, int yf,
									int layer1, int layer2, int w1, int w2,
									int style1, int style2,
									int layer_no_via, int via_w, int via_holew, int dir,
									int crosshair );
	int StartDraggingLineSegment( CDC * pDC, int x, int y,
									int xb, int yb,
									int xi, int yi,
									int xf, int yf,
									int xe, int ye,
									int layer0, int layer1, int layer2,
									int w0,		int w1,		int w2,
									int style0, int style1, int style2,
									int layer_no_via, int via_w, int via_holew,
									int crosshair );
	int StartDraggingLine( CDC * pDC, int x, int y, int xi, int yi, int layer1, int w,
									int layer_no_via, int via_w, int via_holew,
									int crosshair, int style, int inflection_mode );
	int StartDraggingArc( CDC * pDC, int style, int x, int y, int xi, int yi,
									int layer, int w, int crosshair );
	void SetDragArcStyle( int style );
	void Drag( CDC * pDC, int x, int y );
	int StopDragging();
	BOOL Dragging_third_segment() { return m_drag_style2 != DSS_NONE; };
	void ChangeRoutingLayer( CDC * pDC, int layer1, int layer2, int w );
	void IncrementDragAngle( CDC * pDC );
	int MakeDragLineArray( int num_lines );
	int MakeDragRatlineArray( int num_ratlines, int width );
	int AddDragLine( CPoint pi, CPoint pf );
	int AddDragRatline( CPoint pi, CPoint pf );
	int GetDragAngle();
	void FlipDragSide( CDC * pDC );
	int GetDragSide();
	void SetUpCrosshairs( int type, int x, int y );
	void SetInflectionMode( int mode ){ m_inflection_mode = mode; };
	CPoint ScreenToPCB( CPoint point );
	CPoint PCBToScreen( CPoint point );
	CPoint WindowToPCB( CPoint point );

	void UpdateRatlineWidth( int width );

	// set element parameters
	void Set_visible( dl_element * el, int visible );
	void Set_sel_vert( dl_element * el, int sel_vert );
	void Set_w( dl_element * el, int w );
	void Set_clearance( dl_element * el, int clearance );
	void Set_holew( dl_element * el, int holew );
	void Set_x_org( dl_element * el, int x_org );
	void Set_y_org( dl_element * el, int y_org );
	void Set_x( dl_element * el, int x );
	void Set_y( dl_element * el, int y );
	void Set_xf( dl_element * el, int xf );
	void Set_yf( dl_element * el, int yf );
	void Set_id( dl_element * el, id * id );
	void Set_layer( dl_element * el, int layer );
	void Set_radius( dl_element * el, int radius );
	void Set_mode( dl_element * el, int mode );
	void Set_pass( dl_element * el, int pass );
	void Set_gtype( dl_element * el, int gtype );
	void Move( dl_element * el, int dx, int dy );

	// get element parameters
	void * Get_ptr( dl_element * el );
	int Get_gtype( dl_element * el );
	int Get_visible( dl_element * el );
	int Get_sel_vert( dl_element * el );
	int Get_w( dl_element * el );
	int Get_holew( dl_element * el );
	int Get_x_org( dl_element * el );
	int Get_y_org( dl_element * el );
	int Get_x( dl_element * el );
	int Get_y( dl_element * el );
	int Get_xf( dl_element * el );
	int Get_yf( dl_element * el );
	int Get_radius( dl_element * el );
	int Get_layer( dl_element * el );
	COLORREF GetLayerColor( int layer );
	int Get_mode( dl_element * el );
	int Get_pass( dl_element * el );
	void Get_Endpoints(CPoint *cpi, CPoint *cpf);
	id Get_id( dl_element * el );

	int TestForHits( double x, double y, CArray<CHitInfo> *hitInfo );			// CPT:  previously in Brian's CDL_job.  r294: altered args.
};
