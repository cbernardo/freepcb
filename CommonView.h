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
	// Constructor, low-level stuff:
	CCommonView();
	CFreePcbDoc* GetDocument();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	int SetDCToWorldCoords( CDC * pDC );
	afx_msg void OnSize(UINT nType, int cx, int cy);

	CFreePcbDoc * m_Doc;	// the document
	CDisplayList * m_dlist;	// the display list

	// Windows fonts
	CFont m_small_font;
	// cursor mode
	int m_cursor_mode;		// see enum above
	BOOL m_lastKeyWasArrow;	// (used to be globals)
	int m_totalArrowMoveX;
	int m_totalArrowMoveY;
	BOOL m_lastKeyWasGroupRotate;

	int m_debug_flag;
	// flag to indicate that a newly-created item is being dragged,
	// as opposed to an existing item
	// if so, right-clicking or ESC will delete item not restore it
	BOOL m_dragging_new_item;

	// selected items
	id m_sel_id;			// id of selected item
	int m_sel_layer;		// layer of selected item
	int m_sel_offset;		// CPT:  new system for processing repeated clicks in the same place --- see CDisplayList::TestSelect()
	id m_sel_id_prev;		// CPT: ditto.  See e.g. CFreePcbView::OnLButtonUp().  Also used when user repeatedly hits 'N' or 'T'
	void *m_sel_prev;		// CPT: ditto
	int m_cursor_mode_prev;	// CPT: ditto


	// active layer for placement and (perhaps) routing
	int m_active_layer;

	// display coordinate mapping
	double m_pcbu_per_pixel;	// pcb units per pixel
	double m_org_x;				// x-coord of left side of screen in pcb units
	double m_org_y;				// y-coord of bottom of screen in pcb units

	// grids
	CPoint m_snap_angle_ref;		// reference point for snap angle

	// mouse
	CPoint m_from_pt;			// for dragging mode, origin
	CPoint m_last_mouse_point;	// last mouse position
	CPoint m_last_cursor_point;	// last cursor position (may be different from mouse)
	CPoint m_last_click;		// CPT:  last point where user clicked

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
	BOOL m_left_pane_invalid;	// flag to erase and redraw left pane

	// memory DC and bitmap
	BOOL m_memDC_created;
	CDC m_memDC;
	CBitmap m_bitmap;
	CRect m_bitmap_rect;
	CBitmap * m_old_bitmap;

	// units (mil or mm)
	int m_units;

	// selection mask
	int m_sel_mask;
	id m_mask_id[NUM_SEL_MASKS];

	virtual bool IsFreePcbView() = 0;
	// Initialization functions:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void SetDList() = 0;
	void BaseInit();

	// Layers:
	virtual int GetNLayers() = 0;
	virtual int GetTopCopperLayer() = 0;
	virtual int GetLayerRGB(int l, int i) = 0;
	virtual int GetLayerVis(int layer) = 0;
	virtual int GetLayerNum(int i)
		{ return i; }
	virtual void GetLayerLabel(int i, CString &label) = 0;
	virtual int ToggleLayerVis(int i) = 0;
	virtual int GetLeftPaneKeyID() = 0;
	// Masks:
	virtual int GetNMasks() = 0;
	void SetSelMaskArray( int mask );
	virtual int GetMaskNamesID() = 0;
	// Display:
	int ShowCursor();
	void ShowRelativeDistance( int dx, int dy );
	void ShowRelativeDistance( int x, int y, int dx, int dy );
	void InvalidateLeftPane() { m_left_pane_invalid = TRUE; }
	void DrawLeftPane(CDC *pDC);
	void DrawBottomPane(CDC *pDC);
	// User input response:
	virtual void CancelSelection() = 0;
	bool CheckBottomPaneClick(CPoint &point);
	bool CheckLeftPaneClick(CPoint &point);
	virtual BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	virtual void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) = 0;
	virtual void HandleKeyPress(UINT nChar, UINT nRepCnt, UINT nFlags) = 0;
	bool HandleLayerKey(UINT nChar, bool bShiftKeyDown, bool bCtrlKeyDown, CDC *pDC);
	virtual void HandleShiftLayerKey(int layer, CDC *pDC) { }
	virtual void HandleNoShiftLayerKey(int layer, CDC *pDC) { }
	void HandleCtrlFKey(int nChar);
	void HandlePanAndZoom(int nChar, CPoint &p);

	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	void PlacementGridUp();
	void PlacementGridDown();
	void SnapToAngle(CPoint &wp, int grid_spacing);
	void SnapToGridPoint(CPoint &wp, int grid_spacing);
	void SnapToGridLine(CPoint &wp, int grid_spacing);
	};

#ifndef _DEBUG  // debug version in FreePcbView.cpp
inline CFreePcbDoc* CCommonView::GetDocument()
   { return (CFreePcbDoc*)m_pDocument; }
#endif

