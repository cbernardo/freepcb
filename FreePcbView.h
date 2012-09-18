// FreePcbView.h : interface of the CFreePcbView class
//
/////////////////////////////////////////////////////////////////////////////


#if !defined(AFX_FREEPCBVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_)
#define AFX_FREEPCBVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_

#pragma once
#include "stdafx.h"
#include "DisplayList.h"
#include "FreePcbDoc.h"
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
	CUR_AREA_START_TRACE,	// CPT2 new, user is selecting a point to start a new stub trace from within an area
	CUR_DRAG_AREA_STUB, // CPT2 new, user is now dragging a stub that starts from within an area.  Code is very similar to the CUR_DRAG_STUB case
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
	FK_NEXT_DRE,
	FK_PREV_DRE,
	FK_FIRST_DRE,
	FK_LAST_DRE,
	// end CPT

	FK_NUM_OPTIONS,
	FK_ARROW
};


// snap modes
// CPT2 TODO the code for implementing SM_GRID_LINES is totally dysfunctional, and since I don't really understand it I just disabled it
enum {	SM_GRID_POINTS,	// snap to grid points
		SM_GRID_LINES	// snap to grid lines
};

class CFreePcbView : public CCommonView
{
public:
	// member variables
	// parameters related to mouse motion and dragging
	CPoint m_to_pt;								// for dragging segment, endpoint of this segment
	CPoint m_last_pt;							// for dragging segment
	CPoint m_next_pt;							// for dragging segment
	int m_disable_context_menu;					// flag to disable context menu on right-click, if right-click handled some other way
	// grid stuff
	int m_snap_mode;							// snap mode
	int m_inflection_mode;						// inflection mode for routing

	// selected and highlighted items
	static int sel_mask_btn_bits[16];	// CPT2.  New system for masking selections.  Each left-pane button corresponds to 1+ bits for types of pcb-items...

	// Related to routing
	int m_dir;						// routing direction: 0 = forward, 1 = back
    int m_active_width;             // Width for upcoming segs during routing mode (in nm)
	int m_start_layer;				// CPT2 new.  If user starts a stub from an area, this is the area's layer

	// mode for drawing new polyline segments
	int m_polyline_style;			// STRAIGHT, ARC_CW or ARC_CCW
	int m_polyline_hatch;			// NONE, DIAGONAL_FULL or DIAGONAL_EDGE
	int m_polyline_layer;			// layer being drawn

	// CPT2: logging stuff
	bool m_bHandlingKeyPress;
	bool m_bReplayMode, m_bReplaying;
	CArray<CPoint> m_replay_uid_translate;
	CStdioFile m_log_file;
	CString m_log_status;
	int m_replay_ct;

public:
	DECLARE_DYNCREATE(CFreePcbView)
	CFreePcbView();
	virtual ~CFreePcbView();

	CFreePcbDoc* GetDocument();
	void InitInstance();
	void OnNewProject();					// CPT.  Used to be called InitializeView().

	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

	void SetFKText( int mode );
	int ShowSelectStatus();
	int ShowActiveLayer();
	void CancelHighlight();						// AMW r272
	void CancelSelection();
	void CancelHighlightNet();
	void SetWidth( int mode );
	void ChangeTraceLayer( int mode, int old_layer=0 );
	void MoveOrigin( int x_off, int y_off );
	void SelectItemsInRect( CRect r, BOOL bAddToGroup );
	void FinishRouting( CSeg *rat );								// CPT2.  Helper for when user completes routing ratline
	void StartDraggingGroup( BOOL bAdd=FALSE, int x=0, int y=0 );
	void CancelDraggingGroup();
	void MoveGroup( int dx, int dy );
	void RotateGroup();
	void DeleteGroup();
	void SaveUndoInfoForGroup();														// CPT2 Preserved the name, but converted the func to the new system
	BOOL GluedPartsInGroup();
	void UngluePartsInGroup();
	void PasteSingle(int flags, int dx, int dy, int g_num, int ref_off, CMapPtrToPtr &smap,
		int &min_x, int &min_y, int &min_d, CHeap<CPolyline> &pastedPolys);				// Helper for OnGroupPaste().
	int SegmentMovable();
	BOOL CurNone();
	BOOL CurSelected();
	BOOL CurDragging();
	BOOL CurDraggingRouting();
	BOOL CurDraggingPlacement();
	void SnapCursorPoint( CPoint wp, UINT nFlags );
	void OnExternalChangeFootprint( CShape * fp );
	void FinishArrowKey(int x, int y, int dx, int dy);										// CPT2 new helper for HandleKeyPress().
	void HandleKeyPress(UINT nChar, UINT nRepCnt, UINT nFlags);
	void TryToReselectCorner( int x, int y );
	void OnVertexStartTrace(bool bResetActiveWidth);										// CPT2 versions with an extra param added
	void OnRatlineRoute(bool bResetActiveWidth);											// CPT2 ditto

#ifdef CPT2_LOG
	void Log(int function, int *param1=0, int *param2=0, int *param3=0);
	void Log(int function, long *param1, long *param2=0, long *param3=0);
	void Log(CString line, bool bSaveState);
	void LogXtra(int *xtra1, int *xtra2=0, int *xtra3=0, int *xtra4=0);
	void LogXtra(long *xtra1, long *xtra2=0, long *xtra3=0, long *xtra4=0);
	void LogXtra(double *xtra1, double *xtra2=0, double *xtra3=0, double *xtra4=0);
	void LogXtra(bool *xtra1, bool *xtra2=0, bool *xtra3=0, bool *xtra4=0);
	void LogXtra(CString *str);
	void LogItem(CPcbItem **item);
	void LogCancel();
	void LogNetListInfo(netlist_info *nli);
	void LogPartListInfo(partlist_info *pli);
	void LogShape(CShape *fp);
	void Replay();
	void ReplayLoad();
	int ReplayUidTranslate(int logUid);
#else
	void Log(int function, int *param1=0, int *param2=0, int *param3=0) { }
	void Log(int function, long *param1, long *param2=0, long *param3=0) { }
	void Log(CString line, bool bSaveState) { }
	void LogXtra(int *xtra1, int *xtra2=0, int *xtra3=0, int *xtra4=0) { }
	void LogXtra(long *xtra1, long *xtra2=0, long *xtra3=0, long *xtra4=0) { }
	void LogXtra(double *xtra1, double *xtra2=0, double *xtra3=0, double *xtra4=0) { }
	void LogXtra(bool *xtra1) { }
	void LogXtra(CString *str) { }
	void LogItem(CPcbItem **item) { }
	void LogCancel() { }
	void LogNetListInfo(netlist_info *nli) { }
	void LogPartListInfo(partlist_info *pli) { }
	void LogShape(CShape *fp) { }
	void Replay() { }
	void ReplayLoad() { }
#endif

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
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
	afx_msg void OnOptimize();
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
	afx_msg void OnPadStartTrace();
	afx_msg void OnSegmentDelete();
	afx_msg void OnAddArea();
	afx_msg void OnPolyAddCutout();
	afx_msg void OnPolyDeleteCutout();
	afx_msg void OnVertexAddVia();
	afx_msg void OnVertexRemoveVia();
	afx_msg void OnRefProperties();
	afx_msg void OnVertexProperties();
	afx_msg void OnTeeProperties();
	afx_msg void OnUnrouteTrace();
	afx_msg void OnViewEntireBoard();
	afx_msg void OnViewAllElements();
	afx_msg void OnPartEditFootprint();
	afx_msg void OnPartEditThisFootprint();
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnViewFindpart();
	afx_msg void OnFootprintWizard();
	afx_msg void OnFootprintEditor();
	afx_msg void OnCheckPartsAndNets();
	afx_msg void OnToolsDrc();
	afx_msg void OnToolsClearDrc();
	afx_msg void OnToolsRepeatDrc();
	afx_msg void OnToolsShowDRCErrorlist();
	afx_msg void OnViewAll();
	afx_msg void OnAddSoldermaskCutout();
	afx_msg void OnSmEdit();					// CPT2 new
	afx_msg void OnPartChangeSide();
	afx_msg void OnPartRotateCW();
	afx_msg void OnAddNet();
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
	afx_msg void OnAreaStartTrace();
	afx_msg void OnGroupSaveToFile();
	afx_msg void OnGroupCopy();
	afx_msg void OnGroupCut();
	afx_msg void OnGroupDelete();
	afx_msg void OnGroupPaste();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditCut();
	void OnGroupRotate( int bCcw );						// CPT2 newly implemented cw vs. ccw option
	afx_msg void OnGroupRotate()
		{ OnGroupRotate(false); }
	afx_msg void OnGroupRotateCCW()	
		{ OnGroupRotate(true); }
	afx_msg void OnRefShowPart();
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

	void FinishAddPoly();
	void FinishAddPolyCutout();
	void SetMainMenu();
	void HighlightSelection();
};
// end CPT


#endif // !defined(AFX_FREEPCBVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_)
