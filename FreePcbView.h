// FreePcbView.h : interface of the CFreePcbView class
//
/////////////////////////////////////////////////////////////////////////////


#if !defined(AFX_FREEPCBVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_)
#define AFX_FREEPCBVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_

#pragma once
#include "stdafx.h"
#include "DisplayList.h"
#include "FreePcbDoc.h"
#include "ids.h"
#include "CommonView.h"

class CFreePcbView; 

// cursor modes
enum {
	CUR_NONE_SELECTED = 0,		// nothing selected
	CUR_SMCUTOUT_CORNER_SELECTED,	// corner of board outline sel.  
	CUR_SMCUTOUT_SIDE_SELECTED,	// edge of board outline sel.
	CUR_BOARD_CORNER_SELECTED,	// corner of board outline sel.
	CUR_BOARD_SIDE_SELECTED,	// edge of board outline sel.
	CUR_PART_SELECTED,			// part selected
	CUR_REF_SELECTED,			// ref text in part selected
	CUR_VALUE_SELECTED,			// value in part selected
	CUR_PAD_SELECTED,			// pad in part selected
	CUR_SEG_SELECTED,			// trace segment selected
	CUR_RAT_SELECTED,			// unrouted trace segment selected
	CUR_VTX_SELECTED,			// trace vertex selected
	// CPT2 merging with CUR_VTX_SELECTED: CUR_END_VTX_SELECTED,		// end vertex of stub trace selected
	CUR_TEE_SELECTED,			// CPT2
	CUR_CONNECT_SELECTED,		// entire connection selected
	CUR_NET_SELECTED,			// entire net selected
	CUR_TEXT_SELECTED,			// free text selected
	CUR_AREA_CORNER_SELECTED,	// corner of copper area selected
	CUR_AREA_SIDE_SELECTED,		// edge of copper area selected
	CUR_DRE_SELECTED,			// DRC error selected
	CUR_GROUP_SELECTED,			// multiple parts selected
	CUR_NUM_SELECTED_MODES,		// number of SELECTED modes
	CUR_DRAG_PART,		// dragging part
	CUR_DRAG_REF,		// dragging ref text of part
	// CUR_DRAG_VALUE,		// dragging value of part // CPT2 assimilated to CUR_DRAG_REF
	CUR_DRAG_RAT,		// dragging ratline for trace segment
	CUR_DRAG_VTX,		// dragging trace vertex
	CUR_DRAG_VTX_INSERT,	// dragging new vertex being inserted
	CUR_DRAG_TEE,			// CPT2 dragging tee
	// CUR_DRAG_END_VTX,	// dragging end vertex being moved // CPT2 phasing out
	CUR_DRAG_TEXT,		// dragging text box
	CUR_ADD_AREA,		// setting starting point for copper area
	CUR_DRAG_POLY_1,	// dragging first corner for polyline // CPT2 replaces old CUR_DRAG_AREA_1, CUR_DRAG_AREA_CUTOUT_1, CUR_DRAG_SMCUTOUT_1, CUR_DRAG_BOARD_1
	CUR_DRAG_POLY,		// dragging next corner for polyline // CPT2 similarly
	CUR_DRAG_POLY_INSERT,	// dragging polyline corner being inserted // CPT2 sim.
	CUR_DRAG_POLY_MOVE,		// dragging polyline corner being moved // CPT2 sim.
	CUR_ADD_POLY_CUTOUT,	// setting starting point for polyline cutout
	CUR_ADD_SMCUTOUT,		// setting starting point of solder mask cutout
	CUR_ADD_BOARD,		// dragging starting point of board outline
	CUR_DRAG_STUB,		// dragging ratline to next stub endpoint
	CUR_DRAG_NEW_RAT,	// dragging ratline to new connection. CPT2 new name (clearer I think)
	CUR_DRAG_RAT_PIN,	// dragging ratline to new end pin of trace
	CUR_MOVE_ORIGIN,	// dragging new origin
	CUR_DRAG_GROUP,		// dragging a group of parts/segments
	CUR_DRAG_GROUP_ADD,	// dragging a group being added
	CUR_DRAG_MEASURE_1,	// dragging the start of measurement tool
	CUR_DRAG_MEASURE_2,	// dragging the end of measurement tool
	CUR_MOVE_SEGMENT,	// move a segment, leaving it connected to its ends
	CUR_NUM_MODES		// number of modes
};

// function key options
enum {
	FK_NONE = 0,
	FK_MOVE_PART,
	FK_MOVE_REF,
	FK_MOVE_VALUE,
	FK_ROTATE_PART,				// CPT2 disused, but preserved in order to avoid having to change string table.  Now use FK_ROTATE_CW and FK_ROTATE_CCW
	FK_ROTATE_PART_CCW,			// CPT2 disused
	FK_ROTATE_REF,				// CPT2 disused
	FK_ROTATE_REF_CCW,			// CPT2 disused
	FK_ROTATE_VALUE,			// CPT2 disused
	FK_ROTATE_VALUE_CCW,		// CPT2 disused
	FK_SIDE,
	FK_ROUTE,
	FK_UNROUTE,
	FK_REROUTE,
	FK_COMPLETE,
	FK_ADD_PART,
	FK_ADD_NET,
	FK_ADD_TEXT,
	FK_ADD_GRAPHIC,
	FK_REDO_RATLINES,
	FK_ADD_AREA,
	FK_DELETE_PART,
	FK_DELETE_VERTEX,
	FK_MOVE_VERTEX,
	FK_MOVE_CORNER,
	FK_ADD_CORNER,
	FK_DELETE_CORNER,
	FK_ADD_CONNECT,
	FK_DETACH_NET,
	FK_ATTACH_NET,
	FK_DELETE_CONNECT,
	FK_FORCE_VIA,
	FK_SET_WIDTH,
	FK_LOCK_CONNECT,
	FK_UNLOCK_CONNECT,
	FK_MOVE_TEXT,
	FK_ROTATE_TEXT,					// CPT2 disused
	FK_DELETE_TEXT,
	FK_POLY_STRAIGHT,
	FK_POLY_ARC_CW,
	FK_POLY_ARC_CCW,
	FK_EDIT_PART,
	FK_EDIT_FOOTPRINT,
	FK_GLUE_PART,
	FK_UNGLUE_PART,
	FK_UNDO,
	FK_SET_SIZE,
	FK_SET_PARAMS,
	FK_START_TRACE,
	FK_EDIT_TEXT,
	FK_SET_POSITION,
	FK_DELETE_OUTLINE,
	FK_DELETE_AREA,
	FK_DELETE_CUTOUT,			// CPT2 now used only for deleting smcutouts (not area cutouts)
	FK_ADD_SEGMENT,				// CPT2 supplanted by FK_START_TRACE.  But not removing definition (otherwise string-table of fk-texts has to change too)
	FK_ADD_VIA,
	FK_DELETE_VIA,
	FK_DELETE_SEGMENT,
	FK_UNROUTE_TRACE,
	FK_CHANGE_PIN,
	FK_AREA_CUTOUT,
	FK_CHANGE_LAYER,
	FK_EDIT_NET,
	FK_MOVE_GROUP,
	FK_DELETE_GROUP,
	FK_ROTATE_GROUP,			// CPT2 disused
	FK_VERTEX_PROPERTIES,
	FK_ADD_VERTEX,
	FK_SIDE_STYLE,
	FK_EDIT_AREA,
	FK_MOVE_SEGMENT,

    // CPT
    FK_ACTIVE_WIDTH_UP,
    FK_ACTIVE_WIDTH_DOWN,
	FK_RGRID_UP,
	FK_RGRID_DOWN,
	FK_SET_TRACE_WIDTH,
	FK_ROTATE_CW,
	FK_ROTATE_CCW,
	FK_ADD_SMCUTOUT,
	FK_ADD_BOARD,
	FK_REMOVE_CONTOUR,
	FK_EXCLUDE_RGN,
	FK_EDIT_CUTOUT,
	// end CPT

	FK_NUM_OPTIONS,
	FK_ARROW
};

#if 0	// AMW r278: replaced by string table
// function key menu strings
const char fk_str[FK_NUM_OPTIONS*2+2][32] = 
{ 
	"",			"",
	" Move",	" Part",
	" Move",	" Ref Text",
	" Move",	" Value",
	" Rotate",	" Part CW",
	" Rotate",	" Part CCW",
	" Rotate",	" CW",
	" Rotate",	" CCW",
	" Rotate",	" CW",
	" Rotate",	" CCW",
	" Change",	" Side",
	" Route",	" Segment",
	" Unroute",	" Segment",
	" Reroute",	" Segment",
	" Complete"," Segment",
	" Add",		" Part",
	" Add",		" Net",
	" Add",		" Text",
	" Add",		" Graphics",
	" Recalc.",	" Ratlines",
	" Add",		" Area",
	" Delete",  " Part",
	" Delete",  " Vertex",
	" Move",	" Vertex",
	" Move",	" Corner",
	" Add",		" Corner",
	" Delete",	" Corner",
	" Draw",	" Ratline",
	" Detach",	" Net",
	" Set",		" Net",
	" Delete",	" Trace",  
	" Force",	" Via",
	" Set",		" Width",
	" Lock",	" Connect",
	" Unlock",	" Connect",
	" Move",	" Text",
	" Rotate",	" Text",
	" Delete",	" Text",
	" Straight"," Line",
	" Arc",		" (CW)",
	" Arc",		" (CCW)",
	" Edit",	" Part",
	" Edit",	" Footprint",
	" Glue",	" Part",
	" Unglue",	" Part",
	" Undo",	"",
	" Set",		" Size",
	" Set",		" Params",
	" Start",	" Trace",	
	" Edit",	" Text",
	" Set",		" Position",
	" Delete",	" Outline",
	" Delete",	" Area",
	" Delete",	" Cutout",
	" Start",	" Trace",	
	" Add",		" Via",
	" Delete",	" Via",
	" Delete",	" Segment",
	" Unroute",	" Trace",
	" Change",	" Pin",
	" Add",		" Cutout",
	" Change",	" Layer",
	" Edit",	" Net",
	" Move",	" Group",
	" Delete",	" Group",
	" Rotate",	" Group",
	" Edit Via"," Or Vertex",  
	" Add",		" Vertex",
	" Set Side"," Style",
	" Edit",	" Area",
	" Move",	" Segment",

	// CPT
    " Increase",    " Width",
    " Decrease",    " Width",
	" Increase",    " Grid",
	" Decrease",    " Grid",
	// end CPT

	" ****",	" ****"
};
#endif

// snap modes
enum {	SM_GRID_POINTS,	// snap to grid points
		SM_GRID_LINES	// snap to grid lines
};

/* CPT: added to string rsrc table
// selection mask menu strings
const char sel_mask_str[NUM_SEL_MASKS][32] = 
{
	"parts",
	"ref des",
	"value",
	"pins",
	"traces/ratlines",
	"vertices/vias",
	"copper areas",
	"text",
	"sm cutouts",
	"board outline",
	"DRC errors"
};
*/

// descriptor for undo/redo
struct undo_descriptor {
	CFreePcbView * view;	// the view class
	CUndoList * list;		// undo or redo list
	int type;				// type of operation
	CString name1, name2;	// can be used for parts, nets, etc.
	int int1, int2;			// parameter
	CString str1;			// parameter
	void * ptr;				// careful with this
};

// group descriptor
struct undo_group_descriptor {
	CFreePcbView * view;	// the view class
	CUndoList * list;		// undo or redo list
	int type;				// type of operation
	CArray<CString> str;	// array strings with names of items in group
	CArray<id> m_ids;		// array of item ids
};

class CFreePcbView : public CCommonView
{
public:
	enum {		
		// undo types
		UNDO_PART = 1,			// redo for ADD
		UNDO_PART_AND_NETS,		// redo for DELETE and MODIFY
		UNDO_2_PARTS_AND_NETS,	// redo
		UNDO_NET,				// debug flag
		UNDO_NET_AND_CONNECTIONS,	// redo for MODIFY
		UNDO_CONNECTION,		// debug flag
		UNDO_AREA,				// redo for ADD, DELETE, MODIFY
		UNDO_ALL_AREAS_IN_NET,	// redo
		UNDO_ALL_AREAS_IN_2_NETS,	// redo
		UNDO_NET_AND_CONNECTIONS_AND_AREA,	// debug flag
		UNDO_NET_AND_CONNECTIONS_AND_AREAS,	// ASSERT
		UNDO_ALL_NETS_AND_CONNECTIONS_AND_AREAS, // debug flag
		UNDO_ALL_NETS,			// debug flag
		UNDO_MOVE_ORIGIN,		// redo
		UNDO_ALL_BOARD_OUTLINES,	// redo
		UNDO_ALL_SM_CUTOUTS,		// redo
		UNDO_TEXT,					// redo
		UNDO_GROUP,
		// lower-level
		UNDO_BOARD_OUTLINE_CLEAR_ALL,	
		UNDO_BOARD,		
		UNDO_SM_CUTOUT_CLEAR_ALL,
		UNDO_SM_CUTOUT,
		UNDO_GROUP_MODIFY,
		UNDO_GROUP_DELETE,
		UNDO_GROUP_ADD
	};

public: // create from serialization only
	CFreePcbView();
	DECLARE_DYNCREATE(CFreePcbView)

// Attributes
public:
	CFreePcbDoc* GetDocument();

// member variables
public:
	// flag to indicate that a newly-created item is being dragged,
	// as opposed to an existing item
	// if so, right-clicking or ESC will delete item not restore it
	BOOL m_dragging_new_item;

	// parameters for dragging selection rectangle
	BOOL m_bLButtonDown;
	BOOL m_bDraggingRect;
	CPoint m_start_pt;
	CRect m_drag_rect, m_last_drag_rect;
	BOOL m_bDontDrawDragRect;					// CPT true after an autoscroll but before repainting occurs
	CRect m_sel_rect;							// rectangle used for selection

	// mode for drawing new polyline segments
	int m_polyline_style;			// STRAIGHT, ARC_CW or ARC_CCW
	int m_polyline_hatch;			// NONE, DIAGONAL_FULL or DIAGONAL_EDGE
	int m_polyline_layer;			// layer being drawn

	// flag to disable context menu on right-click,
	// if right-click handled some other way
	int m_disable_context_menu;

	// selected items
	// CPT2.  THE FOLLOWING 6 ITEMS WILL ALL BE SUPPLANTED BY CCommonView::m_sel
	cpart * m_sel_part;		// pointer to part, if selected
	cnet * m_sel_net;		// pointer to net, if selected
	CText * m_sel_text;		// pointer to text, if selected
	DRError * m_sel_dre;	// pointer to DRC error, if selected
	CArray<id> m_sel_ids;	// array of multiple selections
	CArray<void*> m_sel_ptrs;	// array of pointers to selected items
	static int sel_mask_btn_bits[16];	// CPT2.  New system for masking selections.  Each left-pane button corresponds to 1+ bits for types of pcb-items...

	// highlight flags
	cnet2 *m_highlight_net;			// CPT2.  Replaces:
	// bool m_bNetHighlighted;	    // current net is highlighted (not selected)

#define m_sel_ic m_sel_id.I2()							// index of selected connection
#define m_sel_ia m_sel_id.I2()							// index of selected area
#define m_sel_is m_sel_id.I3()						// index of selected side, segment, or corner
#define m_sel_iv m_sel_id.I3()						// index of selected vertex

#define m_sel_con (m_sel_net->ConByIndex(m_sel_ic))	// selected connection

#define m_sel_seg (&m_sel_con->SegByIndex(m_sel_is))			// selected side or segment
#define m_sel_prev_seg (&m_sel_con->SegByIndex(m_sel_is-1))			// selected side or segment
#define m_sel_next_seg (&m_sel_con->SegByIndex(m_sel_is+1))			// selected side or segment

#define m_sel_vtx (&m_sel_con->VtxByIndex(m_sel_is))			// selected vertex
#define m_sel_prev_vtx (&m_sel_con->VtxByIndex(m_sel_is-1))	// last vertex
#define m_sel_next_vtx (&m_sel_con->VtxByIndex(m_sel_is+1))	// next vertex
#define m_sel_next_next_vtx (&m_sel_con->VtxByIndex(m_sel_is+2))	// next vertex after that

#define m_sel_con_last_vtx (&m_sel_con->VtxByIndex(m_sel_con->NumSegs()))

#define m_sel_con_start_pin (&m_sel_net->pin[m_sel_con->start_pin])
#define m_sel_con_end_pin (&m_sel_net->pin[m_sel_con->end_pin])

	// direction of routing
	int m_dir;			// 0 = forward, 1 = back

// CPT
    int m_active_width;             // Width for upcoming segs during routing mode (in nm)
	DWORD m_last_autoscroll;		// Tick count when an autoscroll last occurred.
// end CPT

	// grid stuff
	int m_snap_mode;			// snap mode
	int m_inflection_mode;		// inflection mode for routing

	// starting point for a new copper area 
	int m_area_start_x;
	int m_area_start_y;
	
	// mouse
	CPoint m_to_pt;				// for dragging segment, endpoint of this segment
	CPoint m_last_pt;			// for dragging segment
	CPoint m_next_pt;			// for dragging segment

// Operations
public:
	void InitInstance();

// Overrides
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CFreePcbView();
	void OnNewProject();					// CPT.  Used to be called InitializeView().

	void SetFKText( int mode );
	//BOOL SelectItem( id sid );
	// void SelectItem(cpcb_item *item);		// CPT2 --- r317, now in CCommonView
	int ShowSelectStatus();
	int ShowActiveLayer();
	void CancelHighlight();						// AMW r272
	void CancelSelection();
	void HighlightNet( cnet * net, id * exclude_id=NULL );
	void CancelHighlightNet();
	void SetWidth( int mode );
	// int GetWidthsForSegment( int * w, int * via_w, int * via_hole_w );	// CPT2 moved to cnet2::GetWidths
	void ChangeTraceLayer( int mode, int old_layer=0 );
	void MoveOrigin( int x_off, int y_off );
	void SelectItemsInRect( CRect r, BOOL bAddToGroup );
	void FinishRouting( cseg2 *rat );								// CPT2.  Helper for when user completes routing ratline
	void StartDraggingGroup( BOOL bAdd=FALSE, int x=0, int y=0 );
	void CancelDraggingGroup();
	void MoveGroup( int dx, int dy );
	void RotateGroup();
	void DeleteGroup(  CArray<void*> * grp_ptr, CArray<id> * grp_id );
	void FindGroupCenter();
	// int FindItemInGroup( void * ptr, id * tid );					// CPT2 obsolete
	BOOL GluedPartsInGroup();
	void UngluePartsInGroup();
	int SegmentMovable();
	BOOL CurNone();
	BOOL CurSelected();
	BOOL CurDragging();
	BOOL CurDraggingRouting();
	BOOL CurDraggingPlacement();
	void SnapCursorPoint( CPoint wp, UINT nFlags );
	// CPT2 Modified arg lists for the SaveUndo functions.  TODO revise (still blanked out)
	void SaveUndoInfoForPart( cpart2 * part, int type, CString * new_ref_des, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForPartAndNets( cpart2 * part, int type, CString * new_ref_des, BOOL new_event, CUndoList * list );
	void SaveUndoInfoFor2PartsAndNets( cpart2 * part1, cpart2 * part2, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForNet( cnet2 * net, int type, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForNetAndConnections( cnet2 * net, int type, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForConnection( cconnect2 *con, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForArea( carea2 *area, int type, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForAllAreasInNet( cnet2 * net, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForAllAreasIn2Nets( cnet2 * net1, cnet2 * net2, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForNetAndConnectionsAndArea( cnet2 * net, carea2 *area, int type, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForNetAndConnectionsAndAreas( cnet2 * net, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForAllNetsAndConnectionsAndAreas( BOOL new_event, CUndoList * list );
	void SaveUndoInfoForAllNets( BOOL new_event, CUndoList * list );
	void SaveUndoInfoForMoveOrigin( int x_off, int y_off, CUndoList * list );
	void SaveUndoInfoForPolylines( BOOL new_event, CUndoList * list );					// CPT2 will combine the actions of the following two functions:
	// void SaveUndoInfoForBoardOutlines( BOOL new_event, CUndoList * list );
	// void SaveUndoInfoForSMCutouts( BOOL new_event, CUndoList * list );
	void SaveUndoInfoForText( ctext * text, int type, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForText( undo_text * u_text, int type, BOOL new_event, CUndoList * list );
	void SaveUndoInfoForGroup( int type, carray<cpcb_item> *items, CUndoList * list );
	void *  CreateUndoDescriptor( CUndoList * list, int type,
		CString * name1, CString * name2, int int1, int int2, CString * str1, void * ptr );
	static void UndoCallback( int type, void * ptr, BOOL undo );
	void * CreateGroupDescriptor( CUndoList * list, CArray<void*> * grp_ptr, CArray<id> * grp_id, int type );
	static void UndoGroupCallback( int type, void * ptr, BOOL undo );
	void OnExternalChangeFootprint( CShape * fp );
	void FinishArrowKey(int x, int y, int dx, int dy);										// CPT2 new helper for HandleKeyPress().
	void HandleKeyPress(UINT nChar, UINT nRepCnt, UINT nFlags);
	void TryToReselectCorner( int x, int y );
	void ReselectNetItemIfConnectionsChanged( int new_ic );
	int SelectObjPopup( CPoint const &point );												// CPT r294: removed args (use m_hit_info instead)
	void OnVertexStartTrace(bool bResetActiveWidth);										// CPT2 versions with an extra param added
	void OnRatlineRoute(bool bResetActiveWidth);											// CPT2 ditto

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CFreePcbView)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnPartMove();
	afx_msg void OnTextAdd();
	afx_msg void OnTextDelete();
	afx_msg void OnTextMove();
	afx_msg void OnAddPart();					// CPT2, formerly in CFreePcbDoc.
	afx_msg void OnPartGlue();
	afx_msg void OnPartUnglue();
	afx_msg void OnPartProperties();			// CPT2, formerly in CFreePcbDoc.
	afx_msg void OnPartRefProperties();			// CPT2 new
	afx_msg void OnPartValueProperties();		// CPT2 new
	afx_msg void OnPartDelete();
	afx_msg void OnPartOptimize();
	afx_msg void OnRefMove();
	afx_msg void OnPadOptimize();
	afx_msg void OnPadAddToNet();
	afx_msg void OnPadDetachFromNet();
	afx_msg void OnPadStartRatline();
	afx_msg void OnSegmentSetWidth();
	afx_msg void OnSegmentUnroute();
	afx_msg void OnRatlineRoute();
	afx_msg void OnRatlineOptimize();
	afx_msg void OnVertexMove();
	afx_msg void OnTeeMove();
	afx_msg void OnVertexStartRatline();
	afx_msg void OnVertexStartTrace();
	afx_msg void OnVertexDelete();
	afx_msg void OnTeeDelete(); 
	afx_msg void OnRatlineComplete();
	afx_msg void OnRatlineSetWidth();
	afx_msg void OnDeleteConnection();
	afx_msg void OnRatlineLockConnection();
	afx_msg void OnRatlineUnlockConnection();
	afx_msg void OnRatlineChangeEndPin();
	afx_msg void OnTextEdit();
	afx_msg void OnAddBoardOutline();
	// CPT2:  Consolidated from OnBoard..., OnArea..., and OnSm... functions:
	afx_msg void OnPolyDelete();
	afx_msg void OnPolyCornerMove();
	afx_msg void OnPolyCornerEdit();
	afx_msg void OnPolyCornerDelete();
	afx_msg void OnPolySideAddCorner();
	afx_msg void OnPolySideConvertToStraightLine();
	afx_msg void OnPolySideConvertToArcCw();
	afx_msg void OnPolySideConvertToArcCcw();
	void OnPolySideConvert(int style);					// CPT2, factored out the base of the previous 3 functions.
	// afx_msg void OnBoardCornerMove();
	// afx_msg void OnBoardCornerEdit();
	// afx_msg void OnBoardCornerDelete();
	// afx_msg void OnBoardSideAddCorner();
	// afx_msg void OnBoardDeleteOutline();
	afx_msg void OnPadStartTrace();
	afx_msg void OnSegmentDelete();
	// afx_msg void OnEndVertexMove();					// CPT2 phased out special end-vertex routines
	// afx_msg void OnEndVertexAddSegments();
	// afx_msg void OnEndVertexAddConnection();
	// afx_msg void OnEndVertexDelete();
	// afx_msg void OnEndVertexEdit();
	// afx_msg void OnAreaCornerMove();
	// afx_msg void OnAreaCornerDelete();
	// afx_msg void OnAreaDelete();
	// afx_msg void OnAreaSideAddCorner();
	afx_msg void OnAddArea();
	afx_msg void OnPolyAddCutout();
	afx_msg void OnPolyDeleteCutout();
	afx_msg void OnVertexAddVia();
	afx_msg void OnVertexRemoveVia();
	afx_msg void OnRefProperties();
	afx_msg void OnVertexProperties();
	afx_msg void OnTeeProperties();
	// afx_msg void OnBoardSideConvertToStraightLine();
	// afx_msg void OnBoardSideConvertToArcCw();
	// afx_msg void OnBoardSideConvertToArcCcw();
	afx_msg void OnUnrouteTrace();
	afx_msg void OnViewEntireBoard();
	afx_msg void OnViewAllElements();
	afx_msg void OnAreaEdgeHatchStyle();
	afx_msg void OnPartEditFootprint();
	afx_msg void OnPartEditThisFootprint();
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnViewFindpart();
	afx_msg void OnFootprintWizard();
	afx_msg void OnFootprintEditor();
	afx_msg void OnCheckPartsAndNets();
	afx_msg void OnDrc();
	afx_msg void OnClearDrc();
	afx_msg void OnRepeatDrc();
	afx_msg void OnViewAll();
	afx_msg void OnAddSoldermaskCutout();
	// afx_msg void OnSmCornerMove();
	// afx_msg void OnSmCornerSetPosition();
	// afx_msg void OnSmCornerDeleteCorner();
	// afx_msg void OnSmCornerDeleteCutout();
	// afx_msg void OnSmSideInsertCorner();
	// afx_msg void OnSmSideDeleteCutout();
	afx_msg void OnSmEdit();					// CPT2 new
	afx_msg void OnPartChangeSide();
	afx_msg void OnPartRotateCW();
	afx_msg void OnNetSetWidth();
	afx_msg void OnConnectSetWidth();
	afx_msg void OnConnectUnroutetrace();
	afx_msg void OnSegmentChangeLayer();
	afx_msg void OnConnectChangeLayer();
	afx_msg void OnNetChangeLayer();
	afx_msg void OnNetEditnet();
	afx_msg void OnToolsMoveOrigin();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnGroupMove();
	afx_msg void OnAddSimilarArea();
	afx_msg void OnSegmentAddVertex();
	LONG OnChangeVisibleGrid( UINT wp, LONG lp );
	LONG OnChangePlacementGrid( UINT wp, LONG lp );
	LONG OnChangeRoutingGrid( UINT wp, LONG lp );
	LONG OnChangeSnapAngle( UINT wp, LONG lp );
	LONG OnChangeUnits( UINT wp, LONG lp );
	afx_msg void OnAreaEdit();
	afx_msg void OnAreaEdgeApplyClearances();
	afx_msg void OnGroupSaveToFile();
	afx_msg void OnGroupCopy();
	afx_msg void OnGroupCut();
	afx_msg void OnGroupDelete();
	afx_msg void OnGroupPaste();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditCut();
	void OnGroupRotate( bool bCcw );						// CPT2 newly implemented cw vs. ccw option
	afx_msg void OnGroupRotate()
		{ OnGroupRotate(false); }
	afx_msg void OnGroupRotateCCW()	
		{ OnGroupRotate(true); }
	afx_msg void OnRefShowPart();
	// afx_msg void OnAreaSideStyle();						// CPT2 deprecated
	afx_msg void OnPartRotateCCW();
	afx_msg void OnRefRotateCW();
	afx_msg void OnRefRotateCCW();
	afx_msg void OnTextRotateCW();			// CPT2 
	afx_msg void OnTextRotateCCW();			// CPT2
	afx_msg void OnSegmentMove();

// CPT:
	void OnPartRotate(int angle);			// Helper for OnPartRotateCW and OnPartRotateCCW.
	void OnRefRotate(int angle);			// Sim.
	void OnTextRotate(int angle);
    void ActiveWidthUp(CDC * pDC);
    void ActiveWidthDown(CDC * pDC);

	void RoutingGridUp();
	void RoutingGridDown();
	void UnitToggle(bool bShiftKeyDown);
	// bool ConvertSelectionToGroup(bool bChangeMode);			// CPT2 obsolete
	// void ConvertSelectionToGroupAndMove(int dx, int dy);		// CPT2 obsolete
	// void ConvertSingletonGroup();							// CPT2 obsolete
	void ToggleSelectionState(cpcb_item *item);					// CPT2 updated arg

	// CPT:  virtual functions from CCommonView:
	bool IsFreePcbView() { return true; }
	void SetDList()
		{ m_dlist = m_doc->m_dlist; }
	int GetNLayers()
		{ return m_doc->m_num_layers; }
	int GetTopCopperLayer() 
		{ return LAY_TOP_COPPER; }
	int GetLayerRGB(int layer, int i) 
		{ return m_doc->m_rgb[layer][i]; }
	int GetLayerVis(int layer)
		{ return m_doc->m_vis[layer]; }
	int GetLayerNum(int i) {
		// Given a line-number within the left pane, return the actual layer num
		// (may differ for copper layers)
		if( i == GetNLayers()-1 && m_doc->m_num_copper_layers > 1 )
			return LAY_BOTTOM_COPPER;
		if( i > LAY_TOP_COPPER )
			return i+1;
		return i;
		}
	void GetLayerLabel(int i, CString &label) {
		// Get the layer label for the i-th line in the left pane display
		CString s;
		char lc = layer_char[i-LAY_TOP_COPPER]; 
		if (i==LAY_TOP_COPPER)
			s.LoadStringA(IDS_TopCopper),
			label.Format(s, lc, lc);
		else if (i==GetNLayers()-1)
			s.LoadStringA(IDS_Bottom3),
			label.Format(s, lc, lc);
		else if (i==LAY_PAD_THRU)
			label.LoadStringA(IDS_DrilledHole2);
		else if (i>LAY_TOP_COPPER)
			s.LoadStringA(IDS_LayerStr+i+1),
			label.Format("%s. %c,C%c", s, lc, lc);
		else if (i>1)
			s.LoadStringA(IDS_LayerStr+i),
			label.Format("%s. CF%d", s, i-1);
		else
			label.LoadStringA(IDS_LayerStr+i);
		}
	int ToggleLayerVis(int i)
		{ return m_doc->m_vis[i] = !m_doc->m_vis[i]; }
	int GetLeftPaneKeyID() { return IDS_LeftPaneKey; }
	int GetNMasks() { return NUM_SEL_MASKS; }
	int GetMaskNamesID() { return IDS_SelMaskStr; }
	int GetMaskBtnBits(int i) { return sel_mask_btn_bits[i]; }

	void HandleShiftLayerKey(int layer, CDC *pDC);
	void HandleNoShiftLayerKey(int layer, CDC *pDC);

	void FinishAddPoly(ccontour *ctr);
	void FinishAddPolyCutout(ccontour *ctr);
	void SetMainMenu( BOOL bAll );
	void HighlightSelection();
};
// end CPT


#endif // !defined(AFX_FREEPCBVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_)
