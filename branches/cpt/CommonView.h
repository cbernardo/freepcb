// CommonView.h:  header for CommonView.cpp, which contains the common
// base class for CFreePcbView & CFootprintView

#pragma once
#include "stdafx.h"
#include <afxcoll.h>
#include <afxtempl.h>
#include "DisplayList.h"
#include "FreePcbDoc.h"

#define ZOOM_RATIO 1.4
// constants for function key menu
#define FKEY_OFFSET_X 4
#define FKEY_OFFSET_Y 4
#define	FKEY_R_W m_fkey_w // CPT:   No longer constant! 
#define FKEY_R_H 30
#define FKEY_STEP (FKEY_R_W+5)
#define FKEY_GAP 20
#define FKEY_SEP_W 16

// selection masks
enum {	SEL_MASK_PARTS = 0,
		SEL_MASK_REF,
		SEL_MASK_VALUE,
		SEL_MASK_PINS,
		SEL_MASK_CON,
		SEL_MASK_VIA,
		SEL_MASK_AREAS,
		SEL_MASK_TEXT,
		SEL_MASK_SM,
		SEL_MASK_BOARD,
		SEL_MASK_DRC,
		NUM_SEL_MASKS
};


// Footprint selection masks
enum { FP_SEL_MASK_REF = 0,
	   FP_SEL_MASK_VALUE,
	   FP_SEL_MASK_PINS,
	   FP_SEL_MASK_SIDES,
	   FP_SEL_MASK_CORNERS,
	   FP_SEL_MASK_TEXT,
	   FP_SEL_MASK_CENTROID,
	   FP_SEL_MASK_GLUE,
	   NUM_FP_SEL_MASKS
};


class CCommonView : public CView {
public:
	CFreePcbDoc * m_doc;	// the document
	CDisplayList * m_dlist;	// the display list

	// Windows font
	CFont m_small_font;
	// cursor mode
	int m_cursor_mode;
	BOOL m_lastKeyWasArrow;	// (used to be globals)
	BOOL m_lastKeyWasGroupRotate;
	int m_totalArrowMoveX, m_totalArrowMoveY;
	int m_lastArrowPosX, m_lastArrowPosY;			// CPT2 new:  each time an item is moved with arrow keys, save its position here

	int m_debug_flag;

	// selected items
	CHeap<CPcbItem> m_sel;	// CPT2.  Will replace the following 2 items
	int m_sel_offset;		// CPT:  new system for processing repeated clicks in the same place --- see CDisplayList::TestSelect()
	int m_cursor_mode_prev;	// CPT: ditto
	CPcbItem *m_sel_prev;	// CPT2 was void*
	int m_poly_drag_mode;	// CPT2.  Equal to CUR_ADD_AREA, CUR_ADD_AREA_CUTOUT, CUR_ADD_SMCUTOUT, CUR_ADD_BOARD
	CContour *m_drag_contour;	// When user drags out a new contour (main contour or cutout), this points to the contour in question.
	long long groupAverageX, groupAverageY;

	// active layer for placement and (perhaps) routing
	int m_active_layer;

	// display coordinate mapping
	double m_pcbu_per_pixel;				// pcb units per pixel
	double m_org_x;							// x-coord of left side of screen in pcb units
	double m_org_y;							// y-coord of bottom of screen in pcb units
	double m_lastSpaceDx, m_lastSpaceDy;	// CPT2:  How much did the screen move when space bar was last hit?
	// grids
	CPoint m_snap_angle_ref;				// reference point for snap angle

	// mouse
	CPoint m_from_pt;				// for dragging mode, origin
	CPoint m_last_mouse_point;		// last mouse position
	CPoint m_last_cursor_point;		// last cursor position (may be different from mouse because of snapping)
	CPoint m_last_click;			// CPT:  last point where user clicked
	BOOL m_bLButtonDown;
	BOOL m_bDraggingRect;
	CPoint m_start_pt;
	CRect m_drag_rect, m_last_drag_rect;
	BOOL m_bDontDrawDragRect;				// CPT true after an autoscroll but before repainting occurs
	DWORD m_last_autoscroll;				// Tick count when an autoscroll last occurred.
	CRect m_sel_rect;						// rectangle used for selection
	BOOL m_dragging_new_item;				// flag to indicate that a newly-created item is being dragged, as opposed to an existing item
											// if so, right-clicking or ESC will delete item not restore it
	CArray<CHitInfo> m_hit_info;			// CPT r294: info about items near where user is clicking, now a member applicable throughout the class.
	CNet *m_highlight_net;

	// function key shortcuts
	int m_fkey_option[12];
	int m_fkey_command[12];
	int m_fkey_rsrc[24];		// CPT:  used to have a table of char[]'s, now we have a table of string rsrc id's

	// window parameters
	CPoint m_client_origin;	// coordinates of (0,0) in screen coords
	CRect m_client_r;		// in device coords
	int m_left_pane_w;		// width of pane at left of screen for layer selection, etc.
	int m_bottom_pane_h;	// height of pane at bottom of screen for key assignments, etc.
	int m_fkey_w;			// CPT: Width of f-key boxes.
	CRgn m_pcb_rgn;			// region for the pcb

	// memory DC and bitmap
	BOOL m_memDC_created;
	CDC m_memDC;
	CBitmap m_bitmap;
	CRect m_bitmap_rect;
	// CBitmap * m_old_bitmap;
	HBITMAP m_old_bitmap;
	CDC m_memDC2;				// CPT experimental
	CBitmap m_bitmap2;			// ditto
	HBITMAP m_old_bitmap2;		// ditto

	// units (mil or mm)
	int m_units;

	// selection mask
	int m_sel_mask;							// CPT2.  As before, this indicates which buttons in the left pane are on and which off
	int m_sel_mask_bits;					// CPT2.  This value is a function of the previous:  each left-pane button corresponds to 1 or more bits whose
											// meanings are given in CPcbItem's enum of type-bits.  (See CPcbItem::GetTypeBit()).

	// Constructor, low-level stuff:
	CCommonView();
	CFreePcbDoc* GetDocument();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual bool IsFreePcbView() = 0;
	// Initialization functions:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void SetDList() = 0;
	void BaseInit();

	// Layers:
	virtual int GetNLayers() = 0;
	virtual int GetTopCopperLayer() = 0;
	virtual int GetLayerRGB(int l, int i) = 0;
	C_RGB GetLayerRGB(int layer)
		{ return C_RGB(GetLayerRGB(layer,0), GetLayerRGB(layer,1), GetLayerRGB(layer,2)); }
	virtual int GetLayerVis(int layer) = 0;
	virtual int GetLayerNum(int i)
		{ return i; }
	virtual void GetLayerLabel(int i, CString &label) = 0;
	virtual int ToggleLayerVis(int i) = 0;
	virtual int GetLeftPaneKeyID() = 0;
	// Masks:
	virtual int GetNMasks() = 0;
	virtual int GetMaskNamesID() = 0;
	virtual int GetMaskBtnBits(int i) = 0;				// CPT2
	// Display:
	int SetDCToWorldCoords( CDC * pDC );
	void SetCursorMode( int mode );
	int ShowCursor();
	void ShowRelativeDistance( int dx, int dy );
	void ShowRelativeDistance( int x, int y, int dx, int dy );
	void DrawLeftPane(CDC *pDC);
	void DrawBottomPane(CDC *pDC);
	// User input response:
	void SelectItem(CPcbItem *item);
	void ToggleSelectionState(CPcbItem *item);					// CPT2 updated arg
	virtual void CancelSelection() = 0;
	virtual void CancelHighlightNet() { }						// Overridden in CFreePcbView, but does nothing in CFootprintView.
	bool CheckBottomPaneClick(CPoint &point);
	bool CheckLeftPaneClick(CPoint &point);
	void RightClickSelect(CPoint &point);
	virtual BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	virtual void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) = 0;
	virtual void HandleKeyPress(UINT nChar, UINT nRepCnt, UINT nFlags) = 0;
	bool HandleLayerKey(UINT nChar, bool bShiftKeyDown, bool bCtrlKeyDown, CDC *pDC);
	virtual void HandleShiftLayerKey(int layer, CDC *pDC) { }
	virtual void HandleNoShiftLayerKey(int layer, CDC *pDC) { }
	bool HandlePanAndZoom(int nChar, CPoint &p);
	void HandleCtrlFKey(int nChar);
	int SelectObjPopup( CPoint const &point );
	void FindGroupCenter();

	// CPT2 r317, made the following 10 virtual
	virtual void SetFKText( int mode ) = 0;
	virtual int ShowSelectStatus() = 0;
	virtual BOOL CurNone() = 0;
	virtual BOOL CurSelected() = 0;
	virtual BOOL CurDragging() = 0;
	virtual BOOL CurDraggingPlacement() = 0;
	virtual void SnapCursorPoint( CPoint wp, UINT nFlags ) = 0;
	virtual void CancelHighlight() = 0;
	virtual	void HighlightSelection() = 0;
	virtual void SetMainMenu() { }

	void PlacementGridUp();
	void PlacementGridDown();
	void AngleUp();
	void AngleDown();
	void SnapToAngle(CPoint &wp, int grid_spacing);
	void SnapToGridPoint(CPoint &wp, int grid_spacing);
	void SnapToGridLine(CPoint &wp, int grid_spacing);
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg virtual void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	};

#ifndef _DEBUG  // debug version in FreePcbView.cpp
inline CFreePcbDoc* CCommonView::GetDocument()
   { return (CFreePcbDoc*)m_pDocument; }
#endif

