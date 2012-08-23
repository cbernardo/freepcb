// FootprintView.h : interface of the CFootprintView class
//
/////////////////////////////////////////////////////////////////////////////


#if !defined(AFX_FOOTPRINTVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_)
#define AFX_FOOTPRINTVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_

#pragma once
#include "stdafx.h"
#include <afxcoll.h>
#include <afxtempl.h>
#include "DisplayList.h"
#include "FreePcbDoc.h"
#include "CommonView.h"

class CFootprintView;

// cursor modes
enum {
	CUR_FP_NONE_SELECTED = 0,	// nothing selected
	CUR_FP_PAD_SELECTED,		// pad selected
	CUR_FP_REF_SELECTED,		// ref selected
	CUR_FP_VALUE_SELECTED,		// value selected
	CUR_FP_POLY_CORNER_SELECTED, // outline poly corner selected
	CUR_FP_POLY_SIDE_SELECTED,	// outline poly side selected
	CUR_FP_TEXT_SELECTED,		// text string selected
	CUR_FP_CENTROID_SELECTED,	// centroid
	CUR_FP_ADHESIVE_SELECTED,	// glue spot
	CUR_FP_NUM_SELECTED_MODES,	// number of selection modes
	CUR_FP_DRAG_PAD,			// dragging pad to move it
	// CUR_FP_DRAG_REF,			// dragging ref text to move it     // CPT2 Deprecated (use CUR_FP_DRAG_TEXT)
	// CUR_FP_DRAG_VALUE,		// dragging value text to move it	// CPT2 Deprecated (use CUR_FP_DRAG_TEXT)
	CUR_FP_ADD_POLY,			// dragging first corner of new poly
	CUR_FP_DRAG_POLY_1,			// dragging second corner of new poly
	CUR_FP_DRAG_POLY,			// dragging next corner of new poly
	CUR_FP_DRAG_POLY_MOVE,		// dragging corner to move it
	CUR_FP_DRAG_POLY_INSERT,	// dragging corner to insert it
	CUR_FP_DRAG_TEXT,			// dragging text to move it
	CUR_FP_MOVE_ORIGIN,			// dragging origin
	CUR_FP_DRAG_CENTROID,		// dragging centroid
	CUR_FP_DRAG_ADHESIVE,		// dragging glue spot
	CUR_FP_NUM_MODES			// number of cursor modes
};

// function key options
enum {
	FK_FP_NONE = 0,
	FK_FP_MOVE_PAD,
	FK_FP_MOVE_REF,
	FK_FP_ROTATE_PAD,
	FK_FP_ROTATE_REF,
	FK_FP_SIDE,
	FK_FP_ROUTE,				// CPT2 odd that this is here...
	FK_FP_UNROUTE,
	FK_FP_REROUTE,
	FK_FP_COMPLETE,
	FK_FP_ADD_PAD,
	FK_FP_ADD_NET,
	FK_FP_ADD_TEXT,
	FK_FP_ADD_OUTLINE,
	FK_FP_REDO_RATLINES,
	FK_FP_ADD_AREA,
	FK_FP_DELETE_PAD,
	FK_FP_DELETE_VERTEX,
	FK_FP_MOVE_VERTEX,
	FK_FP_MOVE_CORNER,
	FK_FP_ADD_CORNER,
	FK_FP_DELETE_CORNER,
	FK_FP_ADD_CONNECT,
	FK_FP_DETACH_NET,
	FK_FP_ATTACH_NET,
	FK_FP_DELETE_CONNECT,
	FK_FP_FORCE_VIA,
	FK_FP_SET_WIDTH,
	FK_FP_LOCK_CONNECT,
	FK_FP_UNLOCK_CONNECT,
	FK_FP_MOVE_TEXT,
	FK_FP_ROTATE_TEXT,
	FK_FP_DELETE_TEXT,
	FK_FP_POLY_STRAIGHT,
	FK_FP_POLY_ARC_CW,
	FK_FP_POLY_ARC_CCW,
	FK_FP_EDIT_PAD,
	FK_FP_GLUE_PART,
	FK_FP_UNGLUE_PART,
	FK_FP_UNDO,
	FK_FP_EDIT_PROPERTIES,
	FK_FP_START_STUB,
	FK_FP_EDIT_TEXT,
	FK_FP_SET_POSITION,
	FK_FP_DELETE_OUTLINE,
	FK_FP_EDIT_CENTROID,
	FK_FP_MOVE_CENTROID,
	FK_FP_MOVE_VALUE,
	FK_FP_ROTATE_VALUE,
	FK_FP_CLOSE,
	FK_FP_ROTATE_CENTROID,
	FK_FP_EDIT_ADHESIVE,
	FK_FP_MOVE_ADHESIVE,
	FK_FP_DELETE_ADHESIVE,
	FK_FP_ADD_ADHESIVE,								// CPT2 added
	FK_FP_EDIT_OUTLINE,								// CPT2 added
	FK_FP_DELETE_SIDE,
	FK_FP_NUM_OPTIONS
};

class CFootprintView : public CCommonView
{
public: // create from serialization only
	CFootprintView();
	DECLARE_DYNCREATE(CFootprintView)

// member variables
public:
	carray<cpadstack> m_pad_row;					// CPT2 new, gets filled by DlgAddPin with one or more new padstacks.

	// mode for drawing new polyline segments
	BOOL m_polyline_closed_flag;
	int m_polyline_style;	// STRAIGHT, ARC_CW or ARC_CCW
	int m_polyline_width;
	int m_polyline_layer;

	// flag to disable context menu on right-click,
	// if right-click handled some other way
	int m_disable_context_menu;

	// selected item
	static int sel_mask_btn_bits[16];	// CPT2.  New system for masking selections.  Each left-pane button corresponds to 1+ bits for types of pcb-items...

	// footprint
	cshape *m_fp;	// footprint being edited

	// undo stack
	CArray<cshape*> undo_stack;
	CArray<cshape*> redo_stack;

// Operations
public:
	void InitInstance( cshape * fp );

// Overrides
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CFootprintView();
	// CPoint ScreenToPCB( CPoint point );		// CPT eliminated
	// CPoint PCBToScreen( CPoint point );
	// CPoint WindowToPCB( CPoint point );
	void FinishArrowKey(int x, int y, int dx, int dy);   // CPT helper function for HandleKeyPress()
	void HandleKeyPress(UINT nChar, UINT nRepCnt, UINT nFlags);
	int ShowActiveLayer();
	void SetWindowTitle( CString * str );
	int SetWidth( int mode );
	int GetWidthsForSegment( int * w, int * via_w, int * via_hole_w );
	void FootprintModified( BOOL flag, BOOL clear_redo=TRUE );
	void FootprintNameChanged( CString * str );
	void MoveOrigin( int x, int y );
	void ClearUndo();
	void ClearRedo();
	void PushUndo();
	void PushRedo();
	void Undo();
	void UndoNoRedo();
	void Redo();
	void EnableUndo( BOOL bEnable );
	void EnableRedo( BOOL bEnable );
	void EnableRevealValue();				// CPT

// Generated message map functions
protected:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnPadMove();							// CPT2 changed args
	void OnPadMoveRow();								// CPT2 new, takes care of dragging a row of padstacks
	afx_msg void OnPadEdit();							// CPT2 changed arg
	afx_msg void OnPadDelete();							// CPT2 changed arg
	afx_msg void OnPadRotate();							// CPT2 new.
	afx_msg void OnOutlineCornerMove();
	afx_msg void OnOutlineCornerEdit();
	afx_msg void OnOutlineCornerDelete();
	afx_msg void OnOutlineSideAddCorner();
	afx_msg void OnOutlineDelete();
	LONG OnChangeVisibleGrid( UINT wp, LONG lp );
	LONG OnChangePlacementGrid( UINT wp, LONG lp );
	LONG OnChangeSnapAngle( UINT wp, LONG lp );
	LONG OnChangeUnits( UINT wp, LONG lp );
	afx_msg void OnOutlineSideConvertToStraightLine();
	afx_msg void OnOutlineSideConvertToArcCw();
	afx_msg void OnOutlineSideConvertToArcCcw();
	void OnOutlineSideConvert(int style);				// CPT2 factored out the common base of the previous 3 functions
	afx_msg void OnAddPin();
	afx_msg void OnFootprintFileSaveAs();
	afx_msg void OnAddOutline();
	afx_msg void OnOutlineEdit();
	afx_msg void OnFootprintFileImport();
	afx_msg void OnFootprintFileClose();
	afx_msg void OnFootprintFileNew();
	afx_msg void OnViewEntireFootprint();
	afx_msg void OnEditUndo();
	afx_msg void OnFpToolsFootprintWizard();
	afx_msg void OnToolsFootprintLibraryManager();
	afx_msg void OnAddText();
	afx_msg void OnFpTextEdit();
	afx_msg void OnFpTextMove();
	afx_msg void OnFpTextRotate();
	afx_msg void OnFpTextDelete();
	afx_msg void OnToolsMoveOriginFP();
	afx_msg void OnEditRedo();
	afx_msg void OnCentroidEdit();
	afx_msg void OnCentroidMove();
	afx_msg void OnAddSlot();
	afx_msg void OnAddHole();
	afx_msg void OnValueReveal();						// CPT
	afx_msg void OnAddAdhesive();
	afx_msg void OnAdhesiveEdit();
	afx_msg void OnAdhesiveMove();
	afx_msg void OnAdhesiveDrag();
	afx_msg void OnAdhesiveDelete();
	afx_msg void OnCentroidRotateAxis();
	// CPT
	void UnitToggle(bool bShiftKeyDown);
	afx_msg void OnViewPlacementGrid();
	afx_msg void OnViewVisibleGrid();
	afx_msg void OnOutlineSideDelete();					// CPT2 new.  TODO add to context menu.

	// CPT:  virtual functions from CCommonView:
	bool IsFreePcbView() { return false; }
	void SetDList()
		{ m_dlist = m_doc->m_dlist_fp; }
	int GetNLayers()
		{ return m_doc->m_fp_num_layers; }
	int GetTopCopperLayer() 
		{ return LAY_FP_TOP_COPPER; }
	int GetLayerRGB(int layer, int i) 
		{ return m_doc->m_fp_rgb[layer][i]; }
	int GetLayerVis(int layer)
		{ return m_doc->m_fp_vis[layer]; }
	void GetLayerLabel(int i, CString &label) {
		label.LoadStringA(IDS_FpLayerStr+i);
		}
	int ToggleLayerVis(int i)
		{ return m_doc->m_fp_vis[i] = !m_doc->m_fp_vis[i]; }

	int GetLeftPaneKeyID() { return IDS_FpLeftPaneKey; }

	int GetNMasks() { return NUM_FP_SEL_MASKS; }
	int GetMaskNamesID() { return IDS_FpSelMaskStr; }
	int GetMaskBtnBits(int i) { return sel_mask_btn_bits[i]; }

	void HandleNoShiftLayerKey(int layer, CDC *pDC) {
		m_active_layer = layer;
		ShowActiveLayer();
		}
	void SetFKText( int mode );
	int ShowSelectStatus();
	void CancelSelection();
	BOOL CurNone();
	BOOL CurSelected();
	BOOL CurDragging();
	BOOL CurDraggingPlacement();
	void SnapCursorPoint( CPoint wp, UINT nFlags  );
	void HighlightSelection();
	void CancelHighlight();
};

#endif // !defined(AFX_FREEPCBVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_)
