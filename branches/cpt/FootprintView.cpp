// FootprintView.cpp : implementation of the CFootprintView class
//

// CPT2 TODO units on entry maybe unsatisfactory


#include "stdafx.h"
#include "DlgFpText.h"
#include "DlgAssignNet.h"
#include "DlgSetSegmentWidth.h"
#include "DlgEditBoardCorner.h"
#include "DlgAddArea.h"
#include "MyToolBar.h"
#include <Mmsystem.h>
#include <sys/timeb.h>
#include <time.h>
#include <math.h>
#include "FootprintView.h" 
#include "DlgAddPart.h"
#include "DlgAddPin.h"
#include "DlgSaveFootprint.h"
#include "DlgAddPoly.h"
#include "DlgImportFootprint.h"
#include "DlgWizQuad.h"
#include "FootprintView.h"
#include "DlgLibraryManager.h" 
#include "DlgMoveOrigin.h"
#include "DlgCentroid.h"
#include "DlgGlue.h"
#include "DlgHole.h"
#include "DlgSlot.h"
#include "afx.h"
#include "DlgGridVals.h"		// CPT

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ZOOM_RATIO 1.4

extern CFreePcbApp theApp;

// NB: these must be changed if context menu is edited
enum {
	CONTEXT_FP_NONE = 0,
	CONTEXT_FP_PAD,
	CONTEXT_FP_SIDE,
	CONTEXT_FP_CORNER,
	CONTEXT_FP_REF,
	CONTEXT_FP_TEXT,
	CONTEXT_FP_CENTROID,
	CONTEXT_FP_ADHESIVE,
	CONTEXT_FP_VALUE
};

int CFootprintView::sel_mask_btn_bits[16] = { 0 };

/////////////////////////////////////////////////////////////////////////////
// CFootprintView

IMPLEMENT_DYNCREATE(CFootprintView, CView)

BEGIN_MESSAGE_MAP(CFootprintView, CView)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SYSKEYDOWN()
	ON_WM_SYSKEYUP()
	ON_WM_MOUSEWHEEL()
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
//	ON_WM_SYSCHAR()
//ON_WM_SYSCOMMAND()
ON_WM_CONTEXTMENU()
ON_COMMAND(ID_ADD_PIN, OnAddPin)
ON_COMMAND(ID_FOOTPRINT_FILE_SAVE_AS, OnFootprintFileSaveAs)
ON_COMMAND(ID_ADD_POLYLINE, OnAddOutline)
ON_COMMAND(ID_FOOTPRINT_FILE_IMPORT, OnFootprintFileImport)
ON_COMMAND(ID_FOOTPRINT_FILE_CLOSE, OnFootprintFileClose)
ON_COMMAND(ID_FOOTPRINT_FILE_NEW, OnFootprintFileNew)
ON_COMMAND(ID_VIEW_ENTIREFOOTPRINT, OnViewEntireFootprint)
ON_COMMAND(ID_VIEW_REVEALVALUETEXT, OnValueReveal)
//ON_COMMAND(ID_FP_EDIT_UNDO, OnFpEditUndo)
ON_WM_ERASEBKGND()
ON_COMMAND(ID_FP_PAD_MOVE, OnPadMove)
ON_COMMAND(ID_FP_PAD_ROTATE, OnPadRotate)
ON_COMMAND(ID_FP_PAD_EDIT, OnPadEdit)
ON_COMMAND(ID_FP_PAD_DELETE, OnPadDelete)
ON_COMMAND(ID_FP_INSERTCORNER, OnOutlineSideAddCorner)
ON_COMMAND(ID_FP_CONVERTTOSTRAIGHT, OnOutlineSideConvertToStraightLine)
ON_COMMAND(ID_FP_CONVERTTOARC, OnOutlineSideConvertToArcCw)
ON_COMMAND(ID_FP_CONVERTTOARCCCW, OnOutlineSideConvertToArcCcw)
ON_COMMAND(ID_FP_DELETESIDE, OnOutlineSideDelete)
ON_COMMAND(ID_FP_DELETEOUTLINE, OnOutlineDelete)
ON_COMMAND(ID_FP_MOVECORNER, OnOutlineCornerMove)
ON_COMMAND(ID_FP_SETPOSITION, OnOutlineCornerEdit)
ON_COMMAND(ID_FP_DELETECORNER, OnOutlineCornerDelete)
ON_COMMAND(ID_FP_DELETEOUTLINE, OnOutlineDelete)
ON_COMMAND(ID_FP_POLYLINEPROPERTIES, OnOutlineEdit)
ON_COMMAND(ID_FP_TOOLS_RETURN, OnFootprintFileClose)
ON_COMMAND(ID_FP_TOOLS_FOOTPRINTWIZARD, OnFpToolsFootprintWizard)
ON_COMMAND(ID_TOOLS_FOOTPRINTLIBRARYMANAGER, OnToolsFootprintLibraryManager)
ON_COMMAND(ID_FP_ADDTEXT, OnAddText)
ON_COMMAND(ID_FP_TEXT_EDIT, OnFpTextEdit)
ON_COMMAND(ID_FP_TEXT_ROTATE, OnFpTextRotate)
ON_COMMAND(ID_FP_TEXT_MOVE, OnFpTextMove)
ON_COMMAND(ID_FP_TEXT_DELETE, OnFpTextDelete)
ON_COMMAND(ID_FP_ADD_PIN, OnAddPin)
ON_COMMAND(ID_FP_ADD_POLY, OnAddOutline)
ON_COMMAND(ID_FP_ADD_TEXT, OnAddText)
ON_COMMAND(ID_NONE_RETURNTOPCB, OnFootprintFileClose)
ON_COMMAND(ID_NONE_ADDADHESIVE, OnAddAdhesive)
ON_COMMAND(ID_TOOLS_MOVEORIGIN_FP, OnToolsMoveOriginFP)
ON_COMMAND(ID_NONE_MOVEORIGIN, OnToolsMoveOriginFP)
ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
ON_COMMAND(ID_ADD_ADHESIVESPOT, OnAddAdhesive)
ON_COMMAND(ID_CENTROID_SETPARAMETERS, OnCentroidEdit)
ON_COMMAND(ID_CENTROID_MOVE, OnCentroidMove)
ON_COMMAND(ID_ADD_SLOT, OnAddSlot)
// ON_COMMAND(ID_ADD_VALUETEXT, OnAddValueText)
ON_COMMAND(ID_ADD_HOLE, OnAddHole)
ON_COMMAND(ID_FP_EDIT, OnFpTextEdit)
ON_COMMAND(ID_FP_MOVE32923, OnFpTextMove)
ON_COMMAND(ID_ADHESIVE_EDIT, OnAdhesiveEdit)
ON_COMMAND(ID_ADHESIVE_MOVE, OnAdhesiveMove)
ON_COMMAND(ID_ADHESIVE_DELETE, OnAdhesiveDelete)
ON_COMMAND(ID_CENTROID_ROTATEAXIS, OnCentroidRotateAxis)
// CPT
ON_COMMAND(ID_VIEW_FPVISIBLEGRIDVALUES, OnViewVisibleGrid)
ON_COMMAND(ID_VIEW_FPPLACEMENTGRIDVALUES, OnViewPlacementGrid)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFootprintView construction/destruction

// GetDocument() is not available at this point, so initialization of the document
// is in InitInstance()
//
CFootprintView::CFootprintView()
{
	m_active_layer = LAY_FP_TOP_COPPER;
	m_cursor_mode = -1;			// CPT.  Ensures that SetFKText() will get called by InitInstance(),  no matter what.
	// set up array of mask ids
	// CPT2.  Set for new system.
	sel_mask_btn_bits[FP_SEL_MASK_REF] = bitRefText;
	sel_mask_btn_bits[FP_SEL_MASK_VALUE] = bitValueText;
	sel_mask_btn_bits[FP_SEL_MASK_PINS] = bitPadstack;
	sel_mask_btn_bits[FP_SEL_MASK_SIDES] = bitOutlineSide;
	sel_mask_btn_bits[FP_SEL_MASK_CORNERS] = bitOutlineCorner;
	sel_mask_btn_bits[FP_SEL_MASK_TEXT] = bitText;
	sel_mask_btn_bits[FP_SEL_MASK_CENTROID] = bitCentroid;
	sel_mask_btn_bits[FP_SEL_MASK_GLUE] = bitGlue;
	m_sel_mask = 0xffffffff;
	m_sel_mask_bits = 0xffffffff;
	m_fp = NULL;							// CPT2 r317 new.
}

// Initialize data for view
// Should only be called after the document is created
// Don't try to draw window until this function has been called
// Enter with fp = pointer to footprint to be edited, or NULL 
//
void CFootprintView::InitInstance( CShape * fp )
{
	BaseInit();
	EnableUndo( FALSE );
	EnableRedo( FALSE );

	// set up footprint to be edited (if provided).  Correct the value of m_doc->m_edit_footprint (probably equal to "fp" previously)
	if( fp )
	{
		m_fp = new CShape( fp );
		if( m_fp->m_units == NM || m_fp->m_units == MM )
			m_units = MM;
		else
			m_units = MIL;
		CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
		frm->m_wndMyToolBar.SetUnits(m_units );
		OnViewEntireFootprint();
	}
	else
	{
		CString s ((LPCSTR) IDS_Untitled);
		m_fp = new CShape( theApp.m_doc, &s );
	}
	m_doc->m_edit_footprint = m_fp;
	SetWindowTitle( &m_fp->m_name );
	EnableRevealValue();					// CPT

	// set up footprint library map (if necessary)
	if( *m_doc->m_footlibfoldermap.GetDefaultFolder() == "" )
		m_doc->MakeLibraryMaps( &m_doc->m_full_lib_dir );

	// initialize window
	m_dlist->RemoveAll();
	m_fp->Draw();
	FootprintModified( FALSE );
	m_doc->m_footprint_name_changed = FALSE;
	ClearUndo();
	ClearRedo();
	ShowSelectStatus();
	ShowActiveLayer();
	Invalidate( FALSE );
}


CFootprintView::~CFootprintView()
{
	//	delete m_fp;
}


/////////////////////////////////////////////////////////////////////////////
// CFootprintView drawing

void CFootprintView::OnDraw(CDC* pDC)
{
	if( !m_doc )
		// don't try to draw until InitInstance() has been called
		return;

	// get client rectangle
	GetClientRect( &m_client_r );
	// draw stuff on left pane
	DrawLeftPane(pDC);
	// draw function keys on bottom pane
	DrawBottomPane(pDC);

	// clip to pcb drawing region
	pDC->SelectClipRgn( &m_pcb_rgn );

	// now draw the display list
	SetDCToWorldCoords( pDC );
	m_dlist->Draw( pDC );
}

/////////////////////////////////////////////////////////////////////////////
// CFootprintView printing

BOOL CFootprintView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CFootprintView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CFootprintView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

// Left mouse button pressed down, we should probably do something
//
void CFootprintView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	bool bShiftKeyDown = (nFlags & MK_SHIFT) != 0;
	m_last_click = point;
	CDC * pDC = NULL;						// !! remember to ReleaseDC() at end, if necessary
	// CPT:  not sure why we need pDC at all (some clauses below change its clipping, but does it really matter?). CPT2 TODO figure it out
	m_lastKeyWasArrow = FALSE;				// cancel series of arrow keys
	if (CheckBottomPaneClick(point) || CheckLeftPaneClick(point)) 
	{
		CView::OnLButtonDown(nFlags, point); 
		return;
	}

	// clicked in PCB pane
	if(	CurNone() || CurSelected() )
	{
		// we are not dragging anything, see if new item selected
		// CPT reworked so that we have masks, and also (r294) so that my feature #45 with repeated clicks works.
		// CPT2 made adaptations parallel to those in CFreePcbView.  TODO allow groups one day?
		m_sel.RemoveAll();
		CPoint p = m_dlist->WindowToPCB( point );
		int nHits = m_hit_info.GetCount();										// Might reuse the previous contents of m_hit_info...
		if( bShiftKeyDown )
		 	nHits = m_dlist->TestSelect(p.x, p.y, &m_hit_info, 0xffffffff, 0);	// NB: No masking of results
		else if (m_sel_offset==-1)
			// Series of clicks is just beginning: calculate m_hit_info, and select the zero-th of those (highest priority)
			nHits = m_dlist->TestSelect(p.x, p.y, &m_hit_info, m_sel_mask_bits, false),
			m_sel_offset = nHits==0? -1: 0;
		else if (nHits==1)
			m_sel_offset = -1;					// Unselect if there's just one hit nearby, already selected.
		else if (m_sel_offset == nHits-1)
			m_sel_offset = 0;					// Cycle round...
		else
			m_sel_offset++;						// Try next member of m_hit_info

		if( bShiftKeyDown )
			if( nHits>0 )
				m_sel_offset = SelectObjPopup( point );
			else
				m_sel_offset = -1;

		if( m_sel_offset >= 0 )
		{
			// Something to select!
			CPcbItem *item = m_hit_info[m_sel_offset].item;
			m_sel_prev = item;											// CPT
			if( item->IsFootItem() )									// Pretty unlikely that this test would fail...
				SelectItem(item);
			else
				CancelSelection();
		}
		else
			// nothing selected.
			CancelSelection();
	}

	else if( m_cursor_mode == CUR_FP_DRAG_PAD )
	{
		// we were dragging pad, move it
		if( !m_dragging_new_item )
			PushUndo();
		CPoint p = m_last_cursor_point;
		m_dlist->StopDragging();
		CPadstack *first = m_pad_row.First();
		int dx = p.x - first->x_rel;
		int dy = p.y - first->y_rel;
		CIter<CPadstack> ips (&m_pad_row);
		for (CPadstack *ps = ips.First(); ps; ps = ips.Next())
			ps->x_rel += dx,
			ps->y_rel += dy;
		if( m_pad_row.GetSize() == 1 )
		{
			// only rotate if single pad (not row)
			int old_angle = first->angle;
			int angle = (old_angle + m_dlist->GetDragAngle()) % 360;
			first->angle = angle;
		}
		m_dragging_new_item = FALSE;
		FootprintModified( TRUE );
		SetCursorMode( CUR_FP_PAD_SELECTED );
		first->Highlight();
	}

	else if( m_cursor_mode == CUR_FP_DRAG_POLY_MOVE )
	{
		// move corner of polyline
		PushUndo();
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p = m_last_cursor_point;
		CCorner *c = m_sel.First()->ToCorner();
		CPolyline *poly = c->GetPolyline();
		m_dlist->StopDragging();
		BOOL bEnforceCircularArcs = FALSE;
		if( poly->m_layer >= LAY_FP_TOP_COPPER && poly->m_layer <= LAY_FP_BOTTOM_COPPER )
			bEnforceCircularArcs = TRUE;
		BOOL bMod = c->Move( p.x, p.y, bEnforceCircularArcs );
		if( bMod )
		{
			CString s ((LPCSTR) IDS_ArcsWithEndpointsNotAt45DegreeAngles);
			AfxMessageBox( s );
		}
		FootprintModified( TRUE );
		HighlightSelection();
	}

	else if( m_cursor_mode == CUR_FP_DRAG_POLY_INSERT )
	{
		// insert new corner into polyline
		PushUndo();
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p = m_last_cursor_point;
		CSide *s = m_sel.First()->ToSide();
		s->CancelDraggingNewCorner();
		s->InsertCorner( p.x, p.y );
		FootprintModified( TRUE );
		SelectItem(s->preCorner);					// CPT2 Selects the new corner
	}

	else if( m_cursor_mode == CUR_FP_ADD_POLY )
	{
		// Place first corner of polyline
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p = m_last_cursor_point;
		COutline *o = new COutline(m_fp, m_polyline_layer, m_polyline_width);
		m_fp->m_outlines.Add(o);
		m_sel.Add(o);
		CContour *ctr = new CContour(o, true);
		CCorner *c = new CCorner(ctr, p.x, p.y);					// Constructor sets contour's head and tail
		m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_FP_SELECTION, 1, 2 );
		SetCursorMode( CUR_FP_DRAG_POLY_1 );
		m_drag_contour = ctr;
		m_poly_drag_mode = CUR_FP_ADD_POLY;
		FootprintModified( TRUE );
		m_snap_angle_ref = m_last_cursor_point;
	}

	else if( m_cursor_mode == CUR_FP_DRAG_POLY_1 )
	{
		// Place second corner of polyline
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p = m_last_cursor_point;
		PushUndo();
		CContour *ctr = m_drag_contour;
		CCorner *c = new CCorner(ctr, p.x, p.y);
		CSide *s = new CSide(ctr, m_polyline_style);
		ctr->AppendSideAndCorner(s, c, ctr->tail);
		FootprintModified( TRUE );
		m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_FP_SELECTION, 1, 2 );
		SetCursorMode( CUR_FP_DRAG_POLY );
		m_snap_angle_ref = m_last_cursor_point;
	}

	else if( m_cursor_mode == CUR_FP_DRAG_POLY )
	{
		// Place subsequent corners of polyline
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p = m_last_cursor_point;
		PushUndo();
		CContour *ctr = m_drag_contour;
		ctr->GetPolyline()->Undraw();
		CCorner *head = ctr->head;
		if( p.x == head->x && p.y == head->y )
		{
			// cursor point is back at first point, close contour and finish up
			ctr->Close( m_polyline_style );
			FootprintModified( TRUE );
			m_doc->m_dlist->StopDragging();
			CancelSelection();
		}
		else
		{
			// add cursor point
			// ctr->GetPolyline()->MustRedraw();
			CCorner *c = new CCorner(ctr, p.x, p.y);
			CSide *s = new CSide(ctr, m_polyline_style);
			ctr->AppendSideAndCorner(s, c, ctr->tail);
			FootprintModified( TRUE );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_snap_angle_ref = m_last_cursor_point;
		}
	}

	else if( m_cursor_mode == CUR_FP_DRAG_TEXT )
	{
		if( !m_dragging_new_item )
			PushUndo();	// if new item, PushUndo() has already been called
		CPoint p = m_last_cursor_point;
		m_dlist->StopDragging();
		CText *t = m_sel.First()->ToText();
		int old_angle = t->m_angle;
		int angle = (old_angle + m_dlist->GetDragAngle()) % 360;
		BOOL negative = t->m_bNegative;
		// int old_mirror = m_sel_text->m_mirror;							// CPT2 let CText::Draw() take care of auto-mirroring.  bMirror will always be false
		// int mirror = (old_mirror + m_dlist->GetDragSide())%2;
		int layer = t->m_layer;
		t->Move( p.x, p.y, angle, false, negative, layer );
		FootprintModified( TRUE );
		HighlightSelection();
		m_dragging_new_item = FALSE;
	}

	else if( m_cursor_mode == CUR_FP_MOVE_ORIGIN )
	{
		CPoint p = m_last_cursor_point;
		m_dlist->StopDragging();
		PushUndo();
		MoveOrigin( p.x, p.y );
		FootprintModified( TRUE );
		CancelSelection();
		OnViewEntireFootprint();
	}

	else if( m_cursor_mode == CUR_FP_DRAG_CENTROID )
	{
		CPoint p = m_last_cursor_point;
		CCentroid *c = m_sel.First()->ToCentroid();
		c->CancelDragging();
		PushUndo();
		c->m_x = p.x;
		c->m_y = p.y;
		c->m_type = CENTROID_DEFINED;
		FootprintModified( TRUE );
		HighlightSelection();
	}

	else if( m_cursor_mode == CUR_FP_DRAG_ADHESIVE )
	{
		CPoint p = m_last_cursor_point;
		CGlue *g = m_sel.First()->ToGlue();
		g->CancelDragging();
		PushUndo();
		g->x = p.x, g->y = p.y;						// We can assumes that g->type is already GLUE_POS_DEFINED
		FootprintModified( TRUE );
		HighlightSelection();
		m_dragging_new_item = FALSE;
	}

	ShowSelectStatus();

	Invalidate( FALSE );
	if( pDC )
		ReleaseDC( pDC );
	CView::OnLButtonDown(nFlags, point);
}

// left double-click
//
void CFootprintView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	OnLButtonDown(nFlags, point);					// CPT r296.  We want double clicks to behave like 2 quick single clicks.
	CView::OnLButtonDblClk(nFlags, point);
}

// right mouse button
//
void CFootprintView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	// ALSO USED TO CANCEL DRAGGING WHEN THE ESC KEY IS HIT.  (Sub-optimal system?)
	m_disable_context_menu = 1;
	if( m_cursor_mode == CUR_FP_DRAG_PAD )	
	{
		m_fp->CancelDraggingPadRow( &m_pad_row );
		if( m_dragging_new_item )
		{
			UndoNoRedo();
			CancelSelection();
		}
		else
			SelectItem( m_pad_row.First() );
	}
	else if( m_cursor_mode == CUR_FP_ADD_POLY )
	{
		m_dlist->StopDragging();
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_FP_DRAG_POLY_1 )
	{
		m_dlist->StopDragging();
		OnOutlineDelete();
	}
	else if( m_cursor_mode == CUR_FP_DRAG_POLY && m_polyline_closed_flag && m_drag_contour->NumCorners()<3
			 || m_cursor_mode == CUR_FP_DRAG_POLY && !m_polyline_closed_flag && m_drag_contour->NumCorners()<2 )
	{
		m_dlist->StopDragging();
		OnOutlineDelete();
	}
	else if( m_cursor_mode == CUR_FP_DRAG_POLY )
	{
		m_dlist->StopDragging();
		if( m_polyline_closed_flag )
		{
			PushUndo();
			m_drag_contour->Close( m_polyline_style );
			FootprintModified( TRUE );
			CancelSelection();
		}
	}
	else if( m_cursor_mode == CUR_FP_DRAG_POLY_INSERT )
	{
		CSide *s = m_sel.First()->ToSide();
		s->CancelDraggingNewCorner();
		m_fp->Draw();
		HighlightSelection();
	}
	else if( m_cursor_mode == CUR_FP_DRAG_POLY_MOVE )
	{
		CCorner *c = m_sel.First()->ToCorner();
		c->CancelDragging();
		m_fp->Draw();
		HighlightSelection();
	}
	else if( m_cursor_mode == CUR_FP_DRAG_TEXT )
	{
		CText *t = m_sel.First()->ToText();
		t->CancelDragging();
		if( m_dragging_new_item )
		{
			m_fp->m_tl->texts.Remove(t);
			m_fp->Draw();
			CancelSelection();
		}
		else
			HighlightSelection();
	}
	else if( m_cursor_mode == CUR_FP_MOVE_ORIGIN )
	{
		m_dlist->StopDragging();
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_FP_DRAG_CENTROID )
	{
		m_sel.First()->ToCentroid()->CancelDragging();
		HighlightSelection();
	}
	else if( m_cursor_mode == CUR_FP_DRAG_ADHESIVE )
	{
		CGlue *g = m_sel.First()->ToGlue();
		g->CancelDragging();
		UndoNoRedo();						// restore state before dragging
		if( m_dragging_new_item )
			CancelSelection();
		else
			SelectItem(g);
	}
	else
	{
		m_disable_context_menu = 0;
	}

	m_dragging_new_item = FALSE;
	m_sel_offset = -1;					// CPT2.  Sequence of left-clicks is over...
	Invalidate( FALSE );
	ShowSelectStatus();
	CView::OnRButtonDown(nFlags, point);
}


// Key on keyboard pressed down
//
void CFootprintView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	m_sel_offset = -1;							// CPT.  Indicates that user has interrupted a series of mouse clicks.
	HandleKeyPress( nChar, nRepCnt, nFlags );

	// don't pass through SysKey F10
	if( nChar != 121 )
		CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CFootprintView::FinishArrowKey(int x, int y, int dx, int dy) {
	// CPT: Helper for HandleKeyPress() below.  When user hits an arrow key, that routine moves the
	// relevant part, then calls here to redisplay and tidy up.
	m_dlist->CancelHighlight();
	if (!m_lastKeyWasArrow)
		m_totalArrowMoveX = 0,
		m_totalArrowMoveY = 0,
		m_lastKeyWasArrow = true;
	m_totalArrowMoveX += dx;
	m_totalArrowMoveY += dy;
	if (x==INT_MAX)
		// Show dx/dy only
		ShowRelativeDistance(m_totalArrowMoveX, m_totalArrowMoveY);
	else
		ShowRelativeDistance(x, y, m_totalArrowMoveX, m_totalArrowMoveY);
	FootprintModified( TRUE );
	m_sel.First()->Highlight();							// Works well enough since fp editor only allows single item selection (for now)
}

// Key on keyboard pressed down
//
void CFootprintView::HandleKeyPress(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// CPT
	bool bShiftKeyDown = (GetKeyState(VK_SHIFT)&0x8000) != 0;
	bool bCtrlKeyDown = (GetKeyState(VK_CONTROL)&0x8000) != 0;
	if (HandleLayerKey(nChar, bShiftKeyDown, bCtrlKeyDown, 0))
		return;
	if (nChar==VK_OEM_2 || nChar==VK_DIVIDE) {
		// CPT new hotkeys. Slash key => toggle units
		UnitToggle(bShiftKeyDown);
		return;
	}

	int fk = FK_FP_NONE;
	int dx=0, dy=0;
	if( nChar >= 112 && nChar <= 123 )		// Function key 
	{
		if (bCtrlKeyDown)
			{ HandleCtrlFKey(nChar); return; }
		fk = m_fkey_option[nChar-112];
	}

	if( nChar >= 37 && nChar <= 40 )
	{
		// CPT:  added arrow key functionality
		fk = FK_ARROW;
		int d;
		if( bShiftKeyDown && m_units == MM )
			d = 10000;		// 0.01 mm
		else if( bShiftKeyDown && m_units == MIL )
			d = 25400;		// 1 mil
		else if (bCtrlKeyDown && nChar==VK_UP) {
			PlacementGridUp(); 
			return;
			}
		else if (bCtrlKeyDown && nChar==VK_DOWN) {
			PlacementGridDown(); 
			return;
			}
		else
			d = m_doc->m_fp_part_grid_spacing;
		if( nChar == 37 )
			dx -= d;
		else if( nChar == 39 )
			dx += d;
		else if( nChar == 38 )
			dy += d;
		else if( nChar == 40 )
			dy -= d;
	}
	else
		m_lastKeyWasArrow = FALSE;

	CDC *pDC = GetDC();						// CPT:  is this business really necessary?
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );

	// get cursor position and convert to PCB coords
	CPoint p;
	GetCursorPos( &p );		// cursor pos in screen coords
	p = m_dlist->ScreenToPCB( p );	// convert to PCB coords

	// now handle key-press
	switch( m_cursor_mode )
	{
	case  CUR_FP_NONE_SELECTED:
		if( fk == FK_FP_ADD_PAD )
			OnAddPin();
		else if( fk == FK_FP_ADD_TEXT )
			OnAddText();
		else if( fk == FK_FP_ADD_OUTLINE )
			OnAddOutline();
		else if (fk == FK_FP_ADD_ADHESIVE)
			OnAddAdhesive();
		break;

	case CUR_FP_PAD_SELECTED:
		{
			if (fk==FK_ARROW) 
			{
				CPadstack *ps = m_sel.First()->ToPadstack();
				PushUndo();
				ps->x_rel += dx;
				ps->y_rel += dy;
				FinishArrowKey(ps->x_rel, ps->y_rel, dx, dy);
			}
			else if( fk == FK_FP_DELETE_PAD || nChar == 46 )
				OnPadDelete();
			else if( fk == FK_FP_EDIT_PAD )
				OnPadEdit();
			else if( fk == FK_FP_MOVE_PAD )
				OnPadMove();
			else if (fk == FK_FP_ROTATE_PAD)
				OnPadRotate();
			break;
		}

	case CUR_FP_REF_SELECTED:
	case CUR_FP_VALUE_SELECTED:
	case CUR_FP_TEXT_SELECTED:
		if (fk==FK_ARROW) 
		{
			CText *t = m_sel.First()->ToText();
			PushUndo();
			t->Move(t->m_x + dx, t->m_y + dy, t->m_angle);
			FinishArrowKey(t->m_x, t->m_y, dx, dy);
		}
		else if( fk == FK_FP_EDIT_PROPERTIES )
			OnFpTextEdit();
		else if( fk == FK_FP_MOVE_REF || fk == FK_FP_MOVE_VALUE || fk == FK_FP_MOVE_TEXT )
			OnFpTextMove();
		else if (fk == FK_FP_ROTATE_TEXT)
			OnFpTextRotate();															// CPT2 new F-key.
		else if( fk == FK_FP_DELETE_TEXT || nChar == 46 )
			OnFpTextDelete();
		break;

	case CUR_FP_POLY_CORNER_SELECTED:
		if (fk==FK_ARROW) 
		{
			CCorner *c = m_sel.First()->ToCorner();
			PushUndo();
			c->Move( c->x + dx, c->y + dy );
			FinishArrowKey(c->x, c->y, dx, dy);
		}
		else if( fk == FK_FP_SET_POSITION )
			OnOutlineCornerEdit();
		else if( fk == FK_FP_MOVE_CORNER )
			OnOutlineCornerMove();
		else if( fk == FK_FP_DELETE_CORNER || nChar == 46 )
		{
			OnOutlineCornerDelete();
			FootprintModified( TRUE );
		}
		else if( fk == FK_FP_DELETE_OUTLINE )
		{
			OnOutlineDelete();
			FootprintModified( TRUE );
		}
		else if (fk == FK_FP_EDIT_OUTLINE )
			OnOutlineEdit();
		break;

	case CUR_FP_POLY_SIDE_SELECTED:
		if (fk==FK_ARROW) 
		{
			CSide *s = m_sel.First()->ToSide();
			PushUndo();
			int x1 = s->preCorner->x, y1 = s->preCorner->y;
			int x2 = s->postCorner->x, y2 = s->postCorner->y;
			s->preCorner->Move( x1+dx, y1+dy );
			s->postCorner->Move( x2+dx, y2+dy );
			FinishArrowKey( INT_MAX, INT_MAX, dx, dy);
		}
		else if( fk == FK_FP_POLY_STRAIGHT )
			OnOutlineSideConvertToStraightLine();
		else if( fk == FK_FP_POLY_ARC_CW )
			OnOutlineSideConvertToArcCw();
		else if( fk == FK_FP_POLY_ARC_CCW )
			OnOutlineSideConvertToArcCcw();
		else if( fk == FK_FP_ADD_CORNER )
			OnOutlineSideAddCorner();
		else if( fk == FK_FP_DELETE_OUTLINE )
			OnOutlineDelete();
		else if (fk == FK_FP_EDIT_OUTLINE )
			OnOutlineEdit();
		else if (fk == FK_FP_DELETE_SIDE || nChar == 46)						// CPT2 new.
			OnOutlineSideDelete();
		break;

	case CUR_FP_CENTROID_SELECTED:
		if (fk==FK_ARROW) 
		{
			CCentroid *c = m_sel.First()->ToCentroid();
			PushUndo();
			c->m_x += dx;
			c->m_y += dy;
			c->m_type = CENTROID_DEFINED;
			FinishArrowKey(c->m_x, c->m_y, dx, dy);
		}
		else if( fk == FK_FP_EDIT_CENTROID )
			OnCentroidEdit();
		else if( fk == FK_FP_ROTATE_CENTROID )
			OnCentroidRotateAxis();
		else if( fk == FK_FP_MOVE_CENTROID )
			OnCentroidMove();
		break;

	case CUR_FP_ADHESIVE_SELECTED:
		if (fk==FK_ARROW) 
		{
			CGlue *g = m_sel.First()->ToGlue();
			PushUndo();
			if (g->type == GLUE_POS_CENTROID)
			{
				CCentroid *c = m_fp->m_centroid;
				g->type = GLUE_POS_DEFINED;
				g->x = c->m_x; g->y = c->m_y;
			}
			g->x += dx;
			g->y += dy;
			FinishArrowKey(g->x, g->y, dx, dy);
		}
		else if( fk == FK_FP_EDIT_ADHESIVE )
			OnAdhesiveEdit();
		else if( fk == FK_FP_MOVE_ADHESIVE )
			OnAdhesiveMove();
		else if( fk == FK_FP_DELETE_ADHESIVE || nChar == 46 )
			OnAdhesiveDelete();
		break;

	case CUR_FP_DRAG_PAD:
		if( fk == FK_FP_ROTATE_PAD )
			m_dlist->IncrementDragAngle( pDC );
		break;

	case CUR_FP_DRAG_TEXT:
		if( fk == FK_FP_ROTATE_TEXT )
			m_dlist->IncrementDragAngle( pDC );
		break;

	case CUR_FP_DRAG_POLY_1:
	case CUR_FP_DRAG_POLY:
		if( fk == FK_FP_POLY_STRAIGHT )
		{
			m_polyline_style = CPolyline::STRAIGHT;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_FP_POLY_ARC_CW )
		{
			m_polyline_style = CPolyline::ARC_CW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_FP_POLY_ARC_CCW )
		{
			m_polyline_style = CPolyline::ARC_CCW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		break;

	default: 
		break;
	}	// end switch

	if( nChar == VK_HOME )
		// Home key pressed
		OnViewEntireFootprint();
	else if( nChar == 27 )
		// ESC key.  CPT: modified to do what happens in FreePcbView:
		if( CurSelected() )
			CancelSelection();
		else
			OnRButtonDown( 0, NULL );
	HandlePanAndZoom(nChar, p);

	ReleaseDC( pDC );
	if( !m_lastKeyWasArrow )
		ShowSelectStatus();
}

// Mouse moved
//
void CFootprintView::OnMouseMove(UINT nFlags, CPoint point) 
{
	// CPT:  determine whether a series of mouse clicks by the user is truly over (see if we've moved away significantly from m_last_click).
	// If so m_sel_offset is reset to -1.
	if (m_sel_offset!=-1 && (abs(point.x-m_last_click.x) > 1 || abs(point.y-m_last_click.y) > 1))
		m_sel_offset = -1;
	// end CPT
	m_last_mouse_point = m_dlist->WindowToPCB( point );
	SnapCursorPoint( m_last_mouse_point, 0 );
}


// Set function key shortcut text
//
void CFootprintView::SetFKText( int mode )
{
	for( int i=0; i<12; i++ )
	{
		m_fkey_option[i] = 0;
		m_fkey_command[i] = 0;
	}

	switch( mode )
	{
	case CUR_FP_NONE_SELECTED:
		m_fkey_option[1] = FK_FP_ADD_TEXT;
		m_fkey_option[2] = FK_FP_ADD_OUTLINE;
		m_fkey_option[3] = FK_FP_ADD_PAD;
		m_fkey_option[4] = FK_FP_ADD_ADHESIVE;
		break;

	case CUR_FP_PAD_SELECTED:
		m_fkey_option[0] = FK_FP_EDIT_PAD;
		m_fkey_option[2] = FK_FP_ROTATE_PAD;
		m_fkey_option[3] = FK_FP_MOVE_PAD;
		m_fkey_option[6] = FK_FP_DELETE_PAD;
		break;

	case CUR_FP_REF_SELECTED:
		m_fkey_option[0] = FK_FP_EDIT_PROPERTIES;
		m_fkey_option[2] = FK_FP_ROTATE_TEXT;
		m_fkey_option[3] = FK_FP_MOVE_REF;
		break;

	case CUR_FP_VALUE_SELECTED:
		m_fkey_option[0] = FK_FP_EDIT_PROPERTIES;
		m_fkey_option[2] = FK_FP_ROTATE_TEXT;
		m_fkey_option[3] = FK_FP_MOVE_VALUE;
		break;

	case CUR_FP_POLY_CORNER_SELECTED:
		m_fkey_option[0] = FK_FP_SET_POSITION;
		m_fkey_option[3] = FK_FP_MOVE_CORNER;
		m_fkey_option[4] = FK_FP_EDIT_OUTLINE;
		m_fkey_option[5] = FK_FP_DELETE_CORNER;
		m_fkey_option[7] = FK_FP_DELETE_OUTLINE;
		break;

	case CUR_FP_POLY_SIDE_SELECTED:
		{
			CSide *s = m_sel.First()->ToSide();
			m_fkey_option[0] = FK_FP_POLY_STRAIGHT;
			m_fkey_option[1] = FK_FP_POLY_ARC_CW;
			m_fkey_option[2] = FK_FP_POLY_ARC_CCW;
			if( s->m_style == CPolyline::STRAIGHT )
				m_fkey_option[3] = FK_FP_ADD_CORNER;
			m_fkey_option[4] = FK_FP_EDIT_OUTLINE;
			m_fkey_option[5] = FK_FP_DELETE_SIDE;
			m_fkey_option[7] = FK_FP_DELETE_OUTLINE;
			break;
		}

	case CUR_FP_TEXT_SELECTED:
		m_fkey_option[0] = FK_FP_EDIT_PROPERTIES;
		m_fkey_option[2] = FK_FP_ROTATE_TEXT;
		m_fkey_option[3] = FK_FP_MOVE_TEXT;
		m_fkey_option[6] = FK_FP_DELETE_TEXT;
		break;

	case CUR_FP_CENTROID_SELECTED:
		m_fkey_option[0] = FK_FP_EDIT_CENTROID;
		m_fkey_option[2] = FK_FP_ROTATE_CENTROID;
		m_fkey_option[3] = FK_FP_MOVE_CENTROID;
		break;

	case CUR_FP_ADHESIVE_SELECTED:
		m_fkey_option[0] = FK_FP_EDIT_ADHESIVE;
		m_fkey_option[3] = FK_FP_MOVE_ADHESIVE;
		m_fkey_option[6] = FK_FP_DELETE_ADHESIVE;
		break;

	case CUR_FP_DRAG_PAD:
		if( m_pad_row.GetSize() == 1 )						// CPT2 TODO. Remove this restriction?
			m_fkey_option[2] = FK_FP_ROTATE_PAD;
		break;

	case CUR_FP_DRAG_POLY_1:
		m_fkey_option[0] = FK_FP_POLY_STRAIGHT;
		m_fkey_option[1] = FK_FP_POLY_ARC_CW;
		m_fkey_option[2] = FK_FP_POLY_ARC_CCW;
		break;

	case CUR_FP_DRAG_POLY:
		m_fkey_option[0] = FK_FP_POLY_STRAIGHT;
		m_fkey_option[1] = FK_FP_POLY_ARC_CW;
		m_fkey_option[2] = FK_FP_POLY_ARC_CCW;
		break;

	case CUR_FP_DRAG_TEXT:
		m_fkey_option[2] = FK_FP_ROTATE_TEXT;
		break;
	}

	// CPT: Lefthanded mode support:  if set, reverse key meanings
	if (m_doc->m_bLefthanded) 
		for (int lo=0, hi=8, tmp; lo<hi; lo++, hi--)
			tmp = m_fkey_option[lo], 
			m_fkey_option[lo] = m_fkey_option[hi],
			m_fkey_option[hi] = tmp;

	for( int i=0; i<12; i++ )
	{
		// CPT: now we store resource string id's rather than do a strcpy() as before
		int index = 2*m_fkey_option[i];
		m_fkey_rsrc[2*i] = IDS_FkFpStr+index;
		m_fkey_rsrc[2*i+1] = IDS_FkFpStr+index+1;
	}

	Invalidate( FALSE );
}

// display selected item in status bar.  CPT2 converted
//
int CFootprintView::ShowSelectStatus()
{
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	CString str, s0, x_str, y_str, w_str, type_str, style_str;

	switch( m_cursor_mode )
	{
	case CUR_FP_NONE_SELECTED: 
		str.LoadStringA(IDS_NoSelection);
		break;

	case CUR_FP_PAD_SELECTED: 
		{
			CPadstack *ps = m_sel.First()->ToPadstack();
			::MakeCStringFromDimension( &x_str, ps->x_rel, m_units, TRUE, TRUE, TRUE, 3 );
			::MakeCStringFromDimension( &y_str, ps->y_rel, m_units, TRUE, TRUE, TRUE, 3 );
			s0.LoadStringA(IDS_PinXY);
			str.Format( s0, ps->name, x_str, y_str );
			break;
		}

	case CUR_FP_DRAG_PAD:
		{
			CPadstack *ps = m_sel.First()->ToPadstack();
			s0.LoadStringA(IDS_MovingPin);
			str.Format( s0, ps->name );
			break;
		}

	case CUR_FP_POLY_CORNER_SELECTED: 
		{
			CCorner *c = m_sel.First()->ToCorner();
			::MakeCStringFromDimension( &x_str, c->x, m_units, TRUE, TRUE, TRUE, 3 );
			::MakeCStringFromDimension( &y_str, c->y, m_units, TRUE, TRUE, TRUE, 3 );
			s0.LoadStringA(IDS_PolylineCornerXY);
			str.Format( s0, c->GetPolyline()->UID(), c->UID(), x_str, y_str );
			break;
		}

	case CUR_FP_POLY_SIDE_SELECTED: 
		{
			CSide *s = m_sel.First()->ToSide();
			if( s->m_style == CPolyline::STRAIGHT )
				style_str.LoadStringA(IDS_straight);
			else if( s->m_style == CPolyline::ARC_CW )
				style_str.LoadStringA(IDS_ArcCw);
			else if( s->m_style == CPolyline::ARC_CCW )
				style_str.LoadStringA(IDS_ArcCcw);
			s0.LoadStringA(IDS_PolylineSideStyle);
			str.Format( s0, s->GetPolyline()->UID(), s->UID(), style_str );
			break;
		}

	case CUR_FP_CENTROID_SELECTED:
		{
			CCentroid *c = m_sel.First()->ToCentroid();
			if( c->m_type == CENTROID_DEFAULT )
				type_str.LoadStringA(IDS_DefaultPosition); 
			else
				type_str.LoadStringA(IDS_Defined);
			::MakeCStringFromDimension( &x_str, c->m_x, m_units, TRUE, TRUE, TRUE, 3 );
			::MakeCStringFromDimension( &y_str, c->m_y, m_units, TRUE, TRUE, TRUE, 3 );
			s0.LoadStringA(IDS_CentroidXYAngle);
			str.Format( s0, type_str, x_str, y_str, c->m_angle );
			break;
		}

	case CUR_FP_ADHESIVE_SELECTED:
		{
			CGlue *g = m_sel.First()->ToGlue();
			int w = g->w;
			if( w > 0 )
				::MakeCStringFromDimension( &w_str, w, m_units, TRUE, TRUE, TRUE, 3 );
			else
			{
				w_str.LoadStringA(IDS_ProjectDefault);
				w = 15*NM_PER_MIL;
			}
			::MakeCStringFromDimension( &x_str, g->x, m_units, TRUE, TRUE, TRUE, 3 );
			::MakeCStringFromDimension( &y_str, g->y, m_units, TRUE, TRUE, TRUE, 3 );
			if( g->type == GLUE_POS_DEFINED )
				s0.LoadStringA(IDS_AdhesiveSpotWXY),
				str.Format( s0, g->UID(), w_str, x_str, y_str );
			else
				s0.LoadStringA(IDS_AdhesiveSpotWAtCentroid),
				str.Format( s0, g->UID(), w_str );
		}
		break;

	case CUR_FP_DRAG_POLY_MOVE:
		{
			CCorner *c = m_sel.First()->ToCorner();
			s0.LoadStringA(IDS_MovingCornerOfPolyline);
			str.Format( s0, c->GetPolyline()->UID(), c->UID() );
			break;
		}
	}

	pMain->DrawStatus( 3, &str );
	return 0;
}

// cancel selection
//
void CFootprintView::CancelSelection()
{
	m_dlist->CancelHighlight();
	m_sel.RemoveAll();
	m_dragging_new_item = FALSE;
	SetCursorMode( CUR_FP_NONE_SELECTED );
}

// context-sensitive menu invoked by right-click
//
void CFootprintView::OnContextMenu(CWnd* pWnd, CPoint point )
{
	if( m_disable_context_menu )
	{
		// right-click already handled, don't pop up menu
		m_disable_context_menu = 0;
		return;
	}

	// CPT2 new system for potentially selecting a new item when user right-clicks.
	RightClickSelect(point);

	// OK, pop-up context menu
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_FP_CONTEXT));
	CMenu* pPopup = NULL;
	switch( m_cursor_mode )
	{
	case CUR_FP_NONE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_NONE);
		ASSERT(pPopup != NULL);
		break;
	case CUR_FP_PAD_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_PAD);
		ASSERT(pPopup != NULL);
		break;

	case CUR_FP_POLY_SIDE_SELECTED:
		{
			pPopup = menu.GetSubMenu(CONTEXT_FP_SIDE);
			ASSERT(pPopup != NULL);
			CSide *s = m_sel.First()->ToSide();
			int style = s->m_style;
			if( style == CPolyline::STRAIGHT )
			{
				int xi = s->preCorner->x, yi = s->preCorner->y;
				int xf = s->postCorner->x, yf = s->postCorner->y;
				if( xi == xf || yi == yf )
				{
					pPopup->EnableMenuItem( ID_FP_CONVERTTOARC, MF_GRAYED );
					pPopup->EnableMenuItem( ID_FP_CONVERTTOARCCCW, MF_GRAYED );
				}
				pPopup->EnableMenuItem( ID_FP_CONVERTTOSTRAIGHT, MF_GRAYED );
			}
			else if( style == CPolyline::ARC_CW )
			{
				pPopup->EnableMenuItem( ID_FP_CONVERTTOARC, MF_GRAYED );
				pPopup->EnableMenuItem( ID_FP_INSERTCORNER, MF_GRAYED );
			}
			else if( style == CPolyline::ARC_CCW )
			{
				pPopup->EnableMenuItem( ID_FP_CONVERTTOARCCCW, MF_GRAYED );
				pPopup->EnableMenuItem( ID_FP_INSERTCORNER, MF_GRAYED );
			}
			break;
		}

	case CUR_FP_POLY_CORNER_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_CORNER);
		ASSERT(pPopup != NULL);
		/* CPT2 lifted restriction
			if( m_fp.m_outlines[m_sel_id.I2()].NumCorners() < 4 )
				pPopup->EnableMenuItem( ID_FP_DELETECORNER, MF_GRAYED );
		*/
		break;

	case CUR_FP_REF_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_REF);
		ASSERT(pPopup != NULL);
		break;
	case CUR_FP_VALUE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_VALUE);
		ASSERT(pPopup != NULL);
		break;
	case CUR_FP_TEXT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_TEXT);
		ASSERT(pPopup != NULL);
		break;
	case CUR_FP_CENTROID_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_CENTROID);
		ASSERT(pPopup != NULL);
		break;
	case CUR_FP_ADHESIVE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_ADHESIVE);
		ASSERT(pPopup != NULL);
		break;
	}

	if (pPopup)
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );

}

// Delete pad
//
void CFootprintView::OnPadDelete()
{
	CPadstack *ps = m_sel.First()->ToPadstack();
	PushUndo();
	m_fp->m_padstacks.Remove(ps);
	FootprintModified( TRUE );
	CancelSelection();
}

// edit pad
//
void CFootprintView::OnPadEdit()
{
	// save original position and angle of pad, in case we decide
	// to drag the pad, and then cancel dragging
	CPadstack *ps = m_sel.First()->ToPadstack();
	int orig_x = ps->x_rel, orig_y = ps->y_rel; 
	int orig_angle = ps->angle;
	// save undo info, since dialog may make lots of changes
	PushUndo();
	// now launch dialog
	CDlgAddPin dlg;
	dlg.InitDialog( m_fp, ps, &m_pad_row, m_units );
	m_dlist->CancelHighlight();
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		if( dlg.m_drag_flag )
		{
			// if dragging, move pad back to original position and start
			CPadstack *ps = m_pad_row.First();
			ps->x_rel = orig_x; 
			ps->y_rel = orig_y;
			ps->angle = orig_angle;
			m_fp->Draw();
			OnPadMoveRow();
			return;
		}
		else
			// not dragging, just redraw and mark modified flag
			FootprintModified( TRUE );
		SelectItem( m_pad_row.First() );								// CPT2 TODO when group selection is enabled, this must change...
	}
	else
		Undo();	// restore to original state
	Invalidate( FALSE );
}

// move pad, don't push undo, this will be done when move completed
//
void CFootprintView::OnPadMove()
{
	// CPT2 converted.  Piggybacks on OnPadMoveRow
	CPadstack *ps = m_sel.First()->ToPadstack();
	m_pad_row.RemoveAll();
	m_pad_row.Add(ps);
	OnPadMoveRow();
}

void CFootprintView::OnPadMoveRow() 
{
	// CPT2 new, takes over the job of the old OnPadMove(int i, int num).  Starts dragging the row of pad(s) contained in this->m_pad_row.
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to first pad
	CPoint p;
	p.x = m_pad_row.First()->x_rel;
	p.y = m_pad_row.First()->y_rel;
	CPoint cur_p = m_dlist->PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	m_from_pt = p;
	// start dragging
	SelectItem(m_pad_row.First());
	m_fp->StartDraggingPadRow( pDC, &m_pad_row );
	SetCursorMode( CUR_FP_DRAG_PAD );
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

void CFootprintView::OnPadRotate()
{
	// CPT2 done.  TODO add to context menu
	CPadstack *ps = m_sel.First()->ToPadstack();
	PushUndo();
	ps->angle = (ps->angle + 90) % 360;
	m_dlist->CancelHighlight();
	FootprintModified( TRUE );
	HighlightSelection();
}


void CFootprintView::OnOutlineDelete()
{
	PushUndo();
	COutline *o = m_sel.First()->GetPolyline()->ToOutline();
	m_fp->m_outlines.Remove(o);
	FootprintModified( TRUE );
	CancelSelection();
}

// move an outline polyline corner
//
void CFootprintView::OnOutlineCornerMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CCorner *c = m_sel.First()->ToCorner();
	// CPT changed m_from_pt setup:
	m_from_pt.x = c->x;
	m_from_pt.y = c->y;
	c->StartDragging(pDC, m_from_pt.x, m_from_pt.y );
	// End CPT
	SetCursorMode( CUR_FP_DRAG_POLY_MOVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// edit an outline polyline corner
//
void CFootprintView::OnOutlineCornerEdit()
{
	DlgEditBoardCorner dlg;
	CString str ((LPCSTR) IDS_CornerPosition);
	CCorner *c = m_sel.First()->ToCorner();
	dlg.Init( &str, m_units, c->x, c->y );
	int ret = dlg.DoModal();
	if( ret != IDOK ) 
		return;
	PushUndo();
	c->Move(dlg.X(), dlg.Y());
	FootprintModified( TRUE );
	HighlightSelection();
}

// delete an outline polyline board corner
//
void CFootprintView::OnOutlineCornerDelete()
{
	CCorner *c = m_sel.First()->ToCorner();
	CPolyline *poly = c->GetPolyline();
	if (poly->NumCorners()<3 || poly->NumCorners()==3 && poly->IsClosed())
		{ OnOutlineDelete(); return; }
	PushUndo();
	c->Remove();
	FootprintModified( TRUE );
	CancelSelection();
}

// insert a new corner in a side of a polyline
//
void CFootprintView::OnOutlineSideAddCorner()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	CSide *s = m_sel.First()->ToSide();
	s->StartDraggingNewCorner( pDC, p.x, p.y, 2 );
	SetCursorMode( CUR_FP_DRAG_POLY_INSERT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFootprintView::OnOutlineSideConvert(int style)
{
	CSide *s = m_sel.First()->ToSide();
	CPolyline *poly = s->GetPolyline();
	PushUndo();
	s->m_style = style;
	FootprintModified( TRUE );
	HighlightSelection();
}

void CFootprintView::OnOutlineSideConvertToStraightLine()
	{ OnOutlineSideConvert( CPolyline::STRAIGHT ); }

void CFootprintView::OnOutlineSideConvertToArcCw()
	{ OnOutlineSideConvert( CPolyline::ARC_CW ); }

void CFootprintView::OnOutlineSideConvertToArcCcw()
	{ OnOutlineSideConvert( CPolyline::ARC_CCW ); }

void CFootprintView::OnOutlineSideDelete()
{
	// CPT2 r317 new feature.
	CSide *s = m_sel.First()->ToSide();
	PushUndo();
	// NB the following typecast from CHeap<COutline>* to CHeap<CPolyline>* is distasteful but hopefully safe:
	s->Remove( (CHeap<CPolyline>*) &m_fp->m_outlines );
	FootprintModified( TRUE );
	CancelSelection();
}

// detect state where nothing is selected or being dragged
//
BOOL CFootprintView::CurNone()
{
	return m_cursor_mode == CUR_FP_NONE_SELECTED;
}

// detect any selected state
//
BOOL CFootprintView::CurSelected()
{	
	return m_cursor_mode > CUR_FP_NONE_SELECTED && m_cursor_mode < CUR_FP_NUM_SELECTED_MODES;
}

// detect any dragging state
//
BOOL CFootprintView::CurDragging()
{
	return m_cursor_mode > CUR_FP_NUM_SELECTED_MODES;	
}

// detect states using placement grid
//
BOOL CFootprintView::CurDraggingPlacement()
{
	return m_cursor_mode == CUR_FP_DRAG_PAD
		|| m_cursor_mode == CUR_FP_DRAG_TEXT 
		|| m_cursor_mode == CUR_FP_DRAG_POLY_1 
		|| m_cursor_mode == CUR_FP_DRAG_POLY 
		|| m_cursor_mode == CUR_FP_DRAG_POLY_MOVE 
		|| m_cursor_mode == CUR_FP_DRAG_POLY_INSERT;
}

// snap cursor if required and set m_last_cursor_point
//
void CFootprintView::SnapCursorPoint( CPoint wp, UINT nFlags )
{
	// CPT2 r317.  Now a virtual function in CCommonView.  In this class arg "nFlags" is unused.
	if( CurDragging() )
	{	
		int grid_spacing;
		grid_spacing = m_doc->m_fp_part_grid_spacing;

		// snap angle if needed
		if( m_doc->m_fp_snap_angle && (wp != m_snap_angle_ref) 
			&& ( m_cursor_mode == CUR_FP_DRAG_POLY_1 
			|| m_cursor_mode == CUR_FP_DRAG_POLY ) )
			// snap to angle only if the starting point is on-grid
			SnapToAngle(wp, grid_spacing);
		SnapToGridPoint(wp, grid_spacing);

		// CPT removed redundant "if (CurDragging())"
		// update drag operation
		if( wp != m_last_cursor_point )
		{
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			m_dlist->Drag( pDC, wp.x, wp.y );
			ReleaseDC( pDC );
		}

		if( m_cursor_mode == CUR_FP_DRAG_PAD || 
			m_cursor_mode == CUR_FP_DRAG_TEXT ||
			m_cursor_mode == CUR_FP_DRAG_ADHESIVE ||
			m_cursor_mode == CUR_FP_DRAG_POLY_MOVE || 
			m_cursor_mode == CUR_FP_DRAG_CENTROID)
				if (!m_dragging_new_item)
					ShowRelativeDistance( wp.x - m_from_pt.x, wp.y - m_from_pt.y );
	}
	else
//		m_dragging_new_item = FALSE;	// just in case
		if( m_dragging_new_item )
			ASSERT(0);	// debugging, this shouldn't happen
	// update cursor position
	m_last_cursor_point = wp;
	ShowCursor();
}

LONG CFootprintView::OnChangeVisibleGrid( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
		m_doc->m_fp_visual_grid_spacing = fabs( m_doc->m_fp_visible_grid[lp] );
	else
		ASSERT(0);
	m_dlist->SetVisibleGrid( TRUE, m_doc->m_fp_visual_grid_spacing );
	Invalidate( FALSE );
	SetFocus();
	return 0;
}

LONG CFootprintView::OnChangePlacementGrid( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
		m_doc->m_fp_part_grid_spacing = fabs( m_doc->m_fp_part_grid[lp] );
	else
		ASSERT(0);
	SetFocus();
	return 0;
}

LONG CFootprintView::OnChangeSnapAngle( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
	{
		if( lp == 0 )
			m_doc->m_fp_snap_angle = 45;
		else if( lp == 1 )
			m_doc->m_fp_snap_angle = 90;
		else
			m_doc->m_fp_snap_angle = 0;
	}
	else
		ASSERT(0);
	SetFocus();
	return 0;
}

LONG CFootprintView::OnChangeUnits( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
	{
		if( lp == 0 )
			m_units = MIL;
		else if( lp == 1 )
			m_units = MM;
	}
	else
		ASSERT(0);
	// CPT: FootprintModified(TRUE);
	SetFocus();
	Invalidate(false);
	return 0;
}

void CFootprintView::OnAddPin()
{
	PushUndo();
	CDlgAddPin dlg;
	dlg.InitDialog( m_fp, NULL, &m_pad_row, m_units );
	int ret = dlg.DoModal();
	if (ret != IDOK)
		return;
	// if OK, footprint has been undrawn by dialog
	// and new pin(s) added to footprint + m_pad_row
	if( dlg.m_drag_flag )
	{
		// if dragging, move new pad(s) to cursor position
		CPoint p;
		GetCursorPos( &p );		// cursor pos in screen coords
		p = m_dlist->ScreenToPCB( p );	// convert to PCB coords
		int dx = p.x - m_pad_row.First()->x_rel;
		int dy = p.y - m_pad_row.First()->y_rel;
		CIter<CPadstack> ips (&m_pad_row);
		for (CPadstack *ps = ips.First(); ps; ps = ips.Next())
			ps->x_rel += dx,
			ps->y_rel += dy;
		m_fp->Draw();
		// now start dragging
		m_dragging_new_item = TRUE;
		OnPadMoveRow();
		return;
	}
	else
		FootprintModified( true ),
		SelectItem( m_pad_row.First());			// CPT2 TODO when multi-selection is enabled, improve this...
}

void CFootprintView::OnFootprintFileSaveAs()
{
	CString str_name = m_fp->m_name;

	CRect r;
	BOOL bOK = m_fp->GenerateSelectionRectangle( &r );
	if( !bOK )
	{
		CString s ((LPCSTR) IDS_UnableToSaveEmptyFootprint);
		AfxMessageBox( s, MB_OK );
		return;
	}
	m_fp->Draw();

	// now save it
	CDlgSaveFootprint dlg;
	dlg.Initialize( &str_name, m_fp, m_units, "",
		m_doc->m_slist, &m_doc->m_footlibfoldermap, m_doc->m_dlg_log );	
	int ret = dlg.DoModal();
	if( ret == IDOK )	
	{
		FootprintModified( FALSE );
		ClearUndo();
		ClearRedo();
		FootprintNameChanged( &m_fp->m_name );
	}
}

void CFootprintView::OnAddOutline()
{
	CDlgAddPoly dlg;
	dlg.Initialize( TRUE, -1, m_units, -1, TRUE, &m_fp->m_padstacks );
	int ret = dlg.DoModal();
	if (ret!=IDOK) 
		return;
	// start new outline by dragging first point
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_dlist->CancelHighlight();
	m_polyline_closed_flag = dlg.GetClosedFlag();
	m_polyline_style = CPolyline::STRAIGHT;
	m_polyline_width = dlg.GetWidth();
	m_polyline_layer = dlg.GetLayer();
	m_dlist->StartDraggingArray( pDC, p.x, p.y, 0, LAY_FP_SELECTION );
	SetCursorMode( CUR_FP_ADD_POLY );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFootprintView::OnOutlineEdit()
{
	CPolyline *poly = m_sel.First()->GetPolyline();
	CDlgAddPoly dlg;
	dlg.Initialize( FALSE, poly->m_layer, m_units, poly->m_w, poly->IsClosed(), &m_fp->m_padstacks );
	int ret = dlg.DoModal();
	if( ret != IDOK )
		return;
	// change polyline properties
	PushUndo();
	poly->m_w = dlg.GetWidth();
	poly->m_layer = dlg.GetLayer();
	poly->SetClosed( dlg.GetClosedFlag() );
	FootprintModified( TRUE );
	HighlightSelection();
}

void CFootprintView::OnFootprintFileImport()
{
	CDlgImportFootprint dlg;
	dlg.InitInstance( m_doc->m_slist, &m_doc->m_footlibfoldermap, m_doc->m_dlg_log );
	int ret = dlg.DoModal();

	// now import if OK
	if( ret == IDOK && dlg.m_footprint_name != "" && dlg.m_shape->m_name != "" )
	{
		m_dlist->CancelHighlight();					// CPT
		m_fp->Copy( dlg.m_shape );
		m_fp->Draw();

		// update window title and units
		SetWindowTitle( &m_fp->m_name );
		m_doc->m_footprint_name_changed = TRUE;
		m_doc->m_footprint_modified = FALSE;
		CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
		m_units = m_fp->m_units;
		frm->m_wndMyToolBar.SetUnits( m_units );
		ClearUndo();
		ClearRedo();
		OnViewEntireFootprint();
		EnableRevealValue();
	}
	Invalidate( FALSE );
}

void CFootprintView::OnFootprintFileClose()
{
	// set units
	m_fp->m_units = m_units;

	// reset selection rectangle
	CRect br = m_fp->GetAllPadBounds();
	CIter<COutline> io (&m_fp->m_outlines);
	for (COutline *o = io.First(); o; o = io.Next())
	{
		CRect polyr = o->GetBounds();
		br.left = min( br.left, polyr.left ); 
		br.bottom = min( br.bottom, polyr.bottom ); 
		br.right = max( br.right, polyr.right ); 
		br.top = max( br.top, polyr.top ); 
	}
	m_fp->m_sel_xi = br.left - 10*NM_PER_MIL;
	m_fp->m_sel_xf = br.right + 10*NM_PER_MIL;
	m_fp->m_sel_yi = br.bottom - 10*NM_PER_MIL;
	m_fp->m_sel_yf = br.top + 10*NM_PER_MIL;
	m_fp->Draw();

	if( m_doc->m_footprint_modified )
	{
		CString s ((LPCSTR) IDS_SaveFootprintBeforeExiting);
		int ret = AfxMessageBox( s, MB_YESNOCANCEL );
		m_doc->m_file_close_ret = ret;
		if( ret == IDCANCEL )
			return;
		else if( ret == IDYES )
			OnFootprintFileSaveAs();
	}
	ClearUndo();
	ClearRedo();
	theApp.OnViewPcbEditor();
}

void CFootprintView::OnFootprintFileNew()
{
	if( m_doc->m_footprint_modified ) 
	{
		CString s ((LPCSTR) IDS_SaveFootprint);
		int ret = AfxMessageBox( s, MB_YESNOCANCEL );
		if( ret == IDCANCEL )
			return;
		else if( ret == IDYES )
			OnFootprintFileSaveAs();
	}
	m_dlist->CancelHighlight();
	m_fp->Clear();
	m_fp->Draw();
	SetWindowTitle( &m_fp->m_name );
	FootprintModified( FALSE, TRUE );
	ClearUndo();
	ClearRedo();
	Invalidate( FALSE );
}

void CFootprintView::FootprintModified( BOOL flag, BOOL clear_redo )
{
	// if requested, clear redo stack (this is the default)
	if( clear_redo )
		ClearRedo();

	// OK, set state and window title
	m_doc->m_footprint_modified = flag;
	if( flag == TRUE )
	{
		// add "*" to end of window title
		if( m_doc->m_fp_window_title.Right(1) != "*" )
			m_doc->m_fp_window_title = m_doc->m_fp_window_title + "*";
	}
	else if( flag == FALSE )
	{
		// remove "*" from end of window title
		if( m_doc->m_fp_window_title.Right(1) == "*" )
			m_doc->m_fp_window_title = m_doc->m_fp_window_title.Left( m_doc->m_fp_window_title.GetLength()-1 );
	}
	CMainFrame * pMain = (CMainFrame*)AfxGetMainWnd();
	pMain->SetWindowText( m_doc->m_fp_window_title );
	m_fp->Draw();
	Invalidate(false);
}

void CFootprintView::FootprintNameChanged( CString * str )
{
	m_doc->m_footprint_name_changed = TRUE;
	SetWindowTitle( &m_fp->m_name );
}


void CFootprintView::OnViewEntireFootprint()
{
	CRect r, rRef, rVal;
	r = m_fp->GetBounds();
	// CPT:  better accounting for the ref-text and value boxes, which CShape::GetBounds() ignores
	m_fp->m_ref->GenerateStrokes();
	rRef = m_fp->m_ref->m_br;
	m_fp->m_value->GenerateStrokes();
	rVal = m_fp->m_value->m_br;
	r.left = min(r.left, rRef.left);
	r.left = min(r.left, rVal.left);
	r.right = max(r.right, rRef.right);
	r.right = max(r.right, rVal.right);
	r.bottom = min(r.bottom, rRef.bottom);
	r.bottom = min(r.bottom, rVal.bottom);
	r.top = max(r.top, rRef.top);
	r.top = max(r.top, rVal.top);

	int max_x = (9*r.right - r.left)/8;			// CPT: Before we had (3*right-left)/2, but given the new lines above, this change is appropriate.
	int min_x = (9*r.left - r.right)/8;
	int max_y = (9*r.top - r.bottom)/8;
	int min_y = (9*r.bottom - r.top)/8;
	// End CPT
	double win_x = m_client_r.right - m_left_pane_w;
	double win_y = m_client_r.bottom - m_bottom_pane_h;
	// reset window to enclose footprint
	double x_pcbu_per_pixel = (double)(max_x - min_x)/win_x; 
	double y_pcbu_per_pixel = (double)(max_y - min_y)/win_y;
	if( x_pcbu_per_pixel > y_pcbu_per_pixel )
		m_pcbu_per_pixel = x_pcbu_per_pixel;
	else
		m_pcbu_per_pixel = y_pcbu_per_pixel;
	m_org_x = (max_x + min_x)/2 - win_x*m_pcbu_per_pixel/2;
	m_org_y = (max_y + min_y)/2 - win_y*m_pcbu_per_pixel/2;
	CRect screen_r;
	GetWindowRect( &screen_r );
	m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, 
		m_org_x, m_org_y );
	Invalidate( FALSE );
}

void CFootprintView::ClearUndo()
{
	undo_stack.RemoveAll();
	EnableUndo( FALSE );
}

void CFootprintView::ClearRedo()
{
	redo_stack.RemoveAll();
	EnableRedo( FALSE );
}

void CFootprintView::PushUndo()
{
	// CPT2 as a convenience I've added a footprint undraw here:
	m_fp->Undraw();
	if( undo_stack.GetSize() > 100 )
		undo_stack.RemoveAt( 0 );
	CShape * sh = new CShape( m_fp );
	undo_stack.Add( sh );
	EnableUndo( TRUE );
}

void CFootprintView::PushRedo()
{
	if( redo_stack.GetSize() > 100 )
		redo_stack.RemoveAt( 0 );
	CShape * sh = new CShape (m_fp);
	redo_stack.Add( sh );
	EnableRedo( TRUE );
}

// normal undo, push redo info
//
void CFootprintView::Undo()
{
	PushRedo();
	UndoNoRedo();
}

// undo but don't push redo info
// may be used to undo a temporary state
//
void CFootprintView::UndoNoRedo()
{
	int n = undo_stack.GetSize();
	if( n )
	{
		CancelSelection();
		m_fp->Undraw();
		m_fp->Clear();
		CShape * sh = undo_stack[n-1];
		m_fp->Copy( sh );
		m_fp->Draw();
		undo_stack.SetSize( n-1 );
	}
	EnableUndo( undo_stack.GetSize() );
	FootprintModified( TRUE, FALSE );	// don't clear redo stack
	Invalidate( FALSE );
}

void CFootprintView::Redo()
{
	PushUndo();
	int n = redo_stack.GetSize();
	if( n )
	{
		CancelSelection();
		m_fp->Undraw();
		m_fp->Clear();
		CShape * sh = redo_stack[n-1];
		m_fp->Copy( sh );
		m_fp->Draw();
		redo_stack.SetSize( n-1 );
	}
	EnableRedo( redo_stack.GetSize() );
	FootprintModified( TRUE, FALSE ); 	// don't clear redo stack
	Invalidate( FALSE );
}

void CFootprintView::OnEditUndo()
{
	Undo();
}

void CFootprintView::OnEditRedo()
{
	Redo();
}

void CFootprintView::OnFpToolsFootprintWizard()
{
	// ask about saving
	if( m_doc->m_footprint_modified )
	{
		CString s ((LPCSTR) IDS_SaveFootprintBeforeLaunchingWizard);
		int ret = AfxMessageBox( s, MB_YESNOCANCEL );
		m_doc->m_file_close_ret = ret;
		if( ret == IDCANCEL )
			return;
		else if( ret == IDYES )
			OnFootprintFileSaveAs();
		else
			CancelHighlight();
	}

	// OK, launch wizard
	CDlgWizQuad dlg;
	dlg.Initialize( m_doc->m_slist, &m_doc->m_footlibfoldermap, 
		FALSE, m_doc->m_dlg_log );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// import wizard-created footprint
		m_fp->Undraw();
		m_fp->Clear();
		m_fp->Copy( dlg.m_footprint );
		SetWindowTitle( &m_fp->m_name );
		FootprintModified( TRUE );
		// switch to wizard units
		CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
		frm->m_wndMyToolBar.SetUnits( dlg.m_units );
		ClearUndo();
		ClearRedo();
		OnViewEntireFootprint();
		Invalidate( FALSE );
	}
}

void CFootprintView::SetWindowTitle( CString * str )
{
	CString s ((LPCSTR) IDS_FootprintEditor);
	m_doc->m_fp_window_title = s + *str;
	CMainFrame * pMain = (CMainFrame*)AfxGetMainWnd();
	pMain->SetWindowText( m_doc->m_fp_window_title );
}

void CFootprintView::OnToolsFootprintLibraryManager()
{
	CDlgLibraryManager dlg;
	dlg.Initialize( &m_doc->m_footlibfoldermap, m_doc->m_dlg_log );
	dlg.DoModal();
}

void CFootprintView::OnAddText()
{
	CDlgFpText dlg;
	dlg.Initialize( TRUE, FALSE, NULL, LAY_FP_SILK_TOP, m_units, 0, 0, 0, 0, 0 );
	int ret = dlg.DoModal();
	if (ret != IDOK)
		return;

	CPoint p;
	GetCursorPos( &p );		// cursor pos in screen coords
	p = m_dlist->ScreenToPCB( p );	// convert to PCB coords
	int x = dlg.m_bDrag? p.x: dlg.m_x;
	int y = dlg.m_bDrag? p.y: dlg.m_y;
	int angle = dlg.m_angle;
	int font_size = dlg.m_height;
	int stroke_width = dlg.m_width;
	int layer = dlg.m_layer;
	// BOOL mirror = (layer == LAY_FP_SILK_BOTTOM || layer == LAY_FP_BOTTOM_COPPER);		// CPT2 will have CText::Draw() take care of auto-mirroring
	CString str = dlg.m_str;

	PushUndo();
	CText *t = m_fp->m_tl->AddText( x, y, angle, FALSE, FALSE, 
		layer, font_size, stroke_width, &str, NULL, m_fp );
	FootprintModified(true);
	SelectItem(t);
	// set pDC to PCB coords
	if( dlg.m_bDrag )
	{
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		m_dragging_new_item = 1;
		t->StartDragging( pDC );
		SetCursorMode( CUR_FP_DRAG_TEXT );
		ReleaseDC( pDC );
	}

}

// move text
void CFootprintView::OnFpTextMove()
{
	CText *t = m_sel.First()->ToText();
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to ref
	CPoint p (t->m_x, t->m_y);
	m_from_pt = p;									// CPT
	CPoint cur_p = m_dlist->PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start dragging
	m_dragging_new_item = 0;
	t->StartDragging( pDC );
	SetCursorMode( CUR_FP_DRAG_TEXT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFootprintView::OnFpTextRotate()
{
	CText *t = m_sel.First()->ToText();
	PushUndo(); 
	t->m_angle = (t->m_angle + 90) % 360;
	FootprintModified( TRUE );
	HighlightSelection();
	Invalidate( FALSE );
}

void CFootprintView::OnFpTextDelete()
{
	CText *t = m_sel.First()->ToText();
	if (t->IsRefText() || t->IsValueText())
		return;
	PushUndo(); 
	m_fp->m_tl->texts.Remove(t);
	FootprintModified( TRUE );
	CancelSelection();
	Invalidate( FALSE );
}

void CFootprintView::OnFpTextEdit()
{
	// CPT2 converted.  Will also work for ref and value-text.
	CText *t = m_sel.First()->ToText();
	bool bFixedText = t->IsRefText() || t->IsValueText();
	bool bForbidZeroHeight = !t->IsValueText();
	CDlgFpText dlg;
	dlg.Initialize( FALSE, bFixedText, &t->m_str, t->m_layer, m_units, 
		t->m_angle, t->m_font_size, t->m_stroke_width, 
		t->m_x, t->m_y, bForbidZeroHeight );
	int ret = dlg.DoModal();
	if (ret != IDOK)
		return;
	CancelSelection();
	if( dlg.m_bDrag )
	{
		OnFpTextMove();
	}
	else
	{
		PushUndo();
		t->m_layer = dlg.m_layer;
		t->m_x = dlg.m_x;
		t->m_y = dlg.m_y;
		t->m_angle = dlg.m_angle;
		t->m_font_size = dlg.m_height;
		t->m_stroke_width = dlg.m_width;
		t->m_str = dlg.m_str;
		m_fp->Draw();
		if( dlg.m_height )
			HighlightSelection();
		else
			CancelSelection();
	}
	FootprintModified(TRUE);											// CPT
	EnableRevealValue();
}


void CFootprintView::OnValueReveal()
{
	// CPT:  new function for revealing hidden (i.e. size 0) value text
	PushUndo();
	CancelSelection();
	m_fp->GenerateValueParams();
	FootprintModified(true);
	EnableRevealValue();
	OnViewEntireFootprint();
}


// display active layer in status bar and change layer order for DisplayList
//
int CFootprintView::ShowActiveLayer()
{
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	CString str;
	if( m_active_layer == LAY_FP_TOP_COPPER ) 
	{
		str.LoadStringA( IDS_Top3 );
		m_dlist->SetLayerDrawOrder( LAY_FP_TOP_MASK, LAY_FP_TOP_MASK );
		m_dlist->SetLayerDrawOrder( LAY_FP_TOP_PASTE, LAY_FP_TOP_PASTE );
		m_dlist->SetLayerDrawOrder( LAY_FP_BOTTOM_MASK, LAY_FP_BOTTOM_MASK );
		m_dlist->SetLayerDrawOrder( LAY_FP_BOTTOM_PASTE, LAY_FP_BOTTOM_PASTE );
		m_dlist->SetLayerDrawOrder( LAY_FP_TOP_COPPER, LAY_FP_TOP_COPPER );
		m_dlist->SetLayerDrawOrder( LAY_FP_INNER_COPPER, LAY_FP_INNER_COPPER );
		m_dlist->SetLayerDrawOrder( LAY_FP_BOTTOM_COPPER, LAY_FP_BOTTOM_COPPER );
	}
	else if( m_active_layer == LAY_FP_INNER_COPPER )
	{
		str.LoadStringA( IDS_Inner );
		m_dlist->SetLayerDrawOrder( LAY_FP_TOP_MASK, LAY_FP_TOP_MASK );
		m_dlist->SetLayerDrawOrder( LAY_FP_TOP_PASTE, LAY_FP_TOP_PASTE );
		m_dlist->SetLayerDrawOrder( LAY_FP_BOTTOM_MASK, LAY_FP_BOTTOM_MASK );
		m_dlist->SetLayerDrawOrder( LAY_FP_BOTTOM_PASTE, LAY_FP_BOTTOM_PASTE );
		m_dlist->SetLayerDrawOrder( LAY_FP_INNER_COPPER, LAY_FP_TOP_COPPER );
		m_dlist->SetLayerDrawOrder( LAY_FP_TOP_COPPER, LAY_FP_INNER_COPPER );
		m_dlist->SetLayerDrawOrder( LAY_FP_BOTTOM_COPPER, LAY_FP_BOTTOM_COPPER );
	}
	else if( m_active_layer == LAY_FP_BOTTOM_COPPER )
	{
		str.LoadStringA( IDS_Bottom2 );
		m_dlist->SetLayerDrawOrder( LAY_FP_BOTTOM_MASK, LAY_FP_TOP_MASK );
		m_dlist->SetLayerDrawOrder( LAY_FP_BOTTOM_PASTE, LAY_FP_TOP_PASTE );
		m_dlist->SetLayerDrawOrder( LAY_FP_TOP_MASK, LAY_FP_BOTTOM_MASK );
		m_dlist->SetLayerDrawOrder( LAY_FP_TOP_PASTE, LAY_FP_BOTTOM_PASTE );
		m_dlist->SetLayerDrawOrder( LAY_FP_BOTTOM_COPPER, LAY_FP_TOP_COPPER );
		m_dlist->SetLayerDrawOrder( LAY_FP_TOP_COPPER, LAY_FP_INNER_COPPER );
		m_dlist->SetLayerDrawOrder( LAY_FP_INNER_COPPER, LAY_FP_BOTTOM_COPPER );
	}
	pMain->DrawStatus( 4, &str );
	Invalidate( FALSE );
	return 0;
}


void CFootprintView::OnToolsMoveOriginFP()
{
	CDlgMoveOrigin dlg;
	dlg.Initialize( m_units );
	int ret = dlg.DoModal();
	if( ret != IDOK )
		return;
	if( dlg.m_drag )
	{
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		m_dlist->CancelHighlight();
		SetCursorMode( CUR_FP_MOVE_ORIGIN );
		m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x, 
			m_last_cursor_point.y, 0, LAY_SELECTION, 2 );
		Invalidate( FALSE );
		ReleaseDC( pDC );
	}
	else
	{
		MoveOrigin( dlg.m_x, dlg.m_y );
		OnViewEntireFootprint();
		CancelSelection();
	}
}

void CFootprintView::MoveOrigin( int x, int y )
{
	m_fp->m_sel_xi -= x;
	m_fp->m_sel_xf -= x;
	m_fp->m_sel_yi -= y;
	m_fp->m_sel_yf -= y;
	m_fp->m_ref->m_x -= x;
	m_fp->m_ref->m_y -= y;
	m_fp->m_value->m_x -= x;
	m_fp->m_value->m_y -= y;
	m_fp->m_centroid->m_x -= x; 
	m_fp->m_centroid->m_y -= y;
	CIter<CPadstack> ips (&m_fp->m_padstacks);
	for (CPadstack *ps = ips.First(); ps; ps = ips.Next())
		ps->x_rel -= x,
		ps->y_rel -= y;
	CIter<COutline> io (&m_fp->m_outlines);
	for (COutline *o = io.First(); o; o = io.Next())
		o->Offset( -x, -y );
	m_fp->m_tl->MoveOrigin( -x, -y );
	CIter<CGlue> ig (&m_fp->m_glues);
	for (CGlue *g = ig.First(); g; g = ig.Next())
		if (g->type==GLUE_POS_DEFINED)
			g->x -= x,
			g->y -= y;
}

void CFootprintView::EnableUndo( BOOL bEnable )
{
	CWnd* pMain = AfxGetMainWnd();
	if (pMain != NULL)
	{
		CMenu* pMenu = pMain->GetMenu();
		CMenu* submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
		if( bEnable )
			submenu->EnableMenuItem( ID_EDIT_UNDO, MF_BYCOMMAND | MF_ENABLED );
		else
			submenu->EnableMenuItem( ID_EDIT_UNDO, MF_BYCOMMAND | MF_DISABLED |MF_GRAYED );
		pMain->DrawMenuBar();
	}
}

void CFootprintView::EnableRedo( BOOL bEnable )
{
	CWnd* pMain = AfxGetMainWnd();
	if (pMain != NULL)
	{
		CMenu* pMenu = pMain->GetMenu();
		CMenu* submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
		if( bEnable )
			submenu->EnableMenuItem( ID_EDIT_REDO, MF_BYCOMMAND | MF_ENABLED );
		else
			submenu->EnableMenuItem( ID_EDIT_REDO, MF_BYCOMMAND | MF_DISABLED |MF_GRAYED );
		pMain->DrawMenuBar();
	}
}

// CPT: providing a way to restore a "VALUE" that has been shrunk to size 0.  The menu item gets grayed or ungrayed depending on m_fp.m_value_size.
void CFootprintView::EnableRevealValue( )
{
	bool bEnable = m_fp->m_value->m_font_size==0;
	CWnd* pMain = AfxGetMainWnd();
	if (pMain != NULL)
	{
		CMenu* pMenu = pMain->GetMenu();
		CMenu* submenu = pMenu->GetSubMenu(2);	// "View" submenu
		if( bEnable )
			submenu->EnableMenuItem( ID_VIEW_REVEALVALUETEXT, MF_BYCOMMAND | MF_ENABLED );
		else
			submenu->EnableMenuItem( ID_VIEW_REVEALVALUETEXT, MF_BYCOMMAND | MF_DISABLED |MF_GRAYED );
		pMain->DrawMenuBar();
	}
}

void CFootprintView::OnCentroidEdit()
{
	CCentroid *c = m_sel.First()->ToCentroid();
	CDlgCentroid dlg;
	dlg.Initialize( c->m_type, m_units, c->m_x, c->m_y, c->m_angle );
	int ret = dlg.DoModal();
	if( ret != IDOK )
		return;
	PushUndo();
	m_dlist->CancelHighlight();
	c->m_type = dlg.m_type; 
	if( c->m_type == CENTROID_DEFAULT )
	{
		CPoint pt = m_fp->GetDefaultCentroid();
		c->m_x = pt.x; 
		c->m_y = pt.y;
	}
	else
	{
		c->m_x = dlg.m_x; 
		c->m_y = dlg.m_y;
	}
	c->m_angle = dlg.m_angle;
	m_fp->Draw();
	HighlightSelection();
	FootprintModified( TRUE );
	Invalidate( FALSE );
}

void CFootprintView::OnCentroidMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to centroid
	CCentroid *c = m_sel.First()->ToCentroid();
	CPoint p (c->m_x, c->m_y);
	m_from_pt = p;
	CPoint cur_p = m_dlist->PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start dragging
	m_dragging_new_item = 0;
	c->StartDragging( pDC );
	SetCursorMode( CUR_FP_DRAG_CENTROID );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFootprintView::OnCentroidRotateAxis()
{
	PushUndo();
	CCentroid *c = m_sel.First()->ToCentroid();
	c->m_angle = (c->m_angle + 90) % 360;
	m_fp->Draw();
	HighlightSelection();
	FootprintModified( TRUE );
	Invalidate( FALSE );
}


// CPT2 TODO.  Finish these 2...
void CFootprintView::OnAddSlot()
{
	CDlgSlot dlg;
	dlg.Initialize( m_units, 0, 0, 0, 0, 0 );
	dlg.DoModal();
}

void CFootprintView::OnAddHole()
{
	CDlgHole dlg;
	dlg.Initialize( m_units, 0, 0, 0 );
	dlg.DoModal();
}


void CFootprintView::OnAddAdhesive()
{
	CDlgGlue dlg;
	dlg.Initialize( GLUE_POS_CENTROID, m_units, 0, 0, 0 );
	int ret = dlg.DoModal();
	if( ret != IDOK )
		return;
	PushUndo();		// save state before creation of dot
	CGlue *g = new CGlue(m_fp, dlg.m_pos_type, dlg.m_w, dlg.m_x, dlg.m_y);
	m_fp->m_glues.Add(g);
	m_fp->Draw();
	SelectItem(g);
	if( dlg.m_bDrag )
	{
		m_dragging_new_item = TRUE;
		OnAdhesiveDrag();
	}
	else
		FootprintModified( TRUE );
	Invalidate( FALSE );		
}

void CFootprintView::OnAdhesiveEdit()
{
	CGlue *g = m_sel.First()->ToGlue();
	CDlgGlue dlg;
	dlg.Initialize( g->type, m_units, g->w, g->x, g->y );
	int ret = dlg.DoModal();
	if( ret != IDOK )
		return;
	PushUndo();
	g->w = dlg.m_w;							// 0 to use project default
	g->type = dlg.m_pos_type;				// position flag 
	g->x = dlg.m_x; g->y = dlg.m_y;
	/* CPT2 the following is defunct (CGlue::Draw() is in charge of positioning the glue at the last minute, if g->type==GLUE_POS_CENTROID)
	if( g->type == GLUE_POS_CENTROID )
	{
		// use centroid position
		g->x_rel = m_fp.m_centroid_x;
		g->y_rel = m_fp.m_centroid_y;
	}
	else
	{
		// use position from dialog
		g->x_rel = dlg.m_x;
		g->y_rel = dlg.m_y;
	}
	*/
	if ( dlg.m_bDrag )
	{
		// start dragging
		m_dragging_new_item = FALSE;
		OnAdhesiveDrag();
	}
	else
	{
		m_dlist->CancelHighlight();
		m_fp->Draw();
		HighlightSelection();
		FootprintModified( TRUE );
	}
	Invalidate( FALSE );
}


// move glue spot
//
void CFootprintView::OnAdhesiveMove()
{
	PushUndo(); 
	m_dragging_new_item = FALSE;
	OnAdhesiveDrag();
}

// used for both moving and adding glue spots
// on entry:
//	adhesive dot should already be added to footprint and selected
//	undo info already pushed
//	m_dragging_new_item already set
//
void CFootprintView::OnAdhesiveDrag()
{
	CGlue *g = m_sel.First()->ToGlue();
	if (g->type==GLUE_POS_CENTROID)
	{
		CCentroid *c = m_fp->m_centroid;
		g->x = c->m_x; g->y = c->m_y;
		g->type = GLUE_POS_DEFINED;
	}
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to dot
	CPoint p (g->x, g->y);
	m_from_pt = p;
	CPoint cur_p = m_dlist->PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start dragging
	g->StartDragging( pDC );
	SetCursorMode( CUR_FP_DRAG_ADHESIVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFootprintView::OnAdhesiveDelete()
{
	CGlue *g = m_sel.First()->ToGlue();
	PushUndo();
	m_fp->m_glues.Remove(g);
	m_fp->Draw();
	CancelSelection();
	FootprintModified( TRUE );
	Invalidate( FALSE );
}

// CPT

void CFootprintView::UnitToggle(bool bShiftKeyDown) {
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	frm->m_wndMyToolBar.UnitToggle(bShiftKeyDown, &(m_doc->m_fp_visible_grid), &(m_doc->m_fp_part_grid), 0);
	}

extern void ReadFileLines(CString &fname, CArray<CString> &lines);  // In FreePcbDoc.cpp
extern void WriteFileLines(CString &fname, CArray<CString> &lines);
extern void ReplaceLines(CArray<CString> &oldLines, CArray<CString> &newLines, char *key);

void CFootprintView::OnViewVisibleGrid() 
{
	CArray<double> &arr = m_doc->m_fp_visible_grid, &hidden = m_doc->m_fp_visible_grid_hidden;
	CDlgGridVals dlg (&arr, &hidden, IDS_EditFootprintVisibleGridValues);
	int ret = dlg.DoModal();
	if( ret != IDOK ) 
		return;
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	frm->m_wndMyToolBar.SetLists( &m_doc->m_fp_visible_grid, &m_doc->m_fp_part_grid, NULL,
			m_doc->m_fp_visual_grid_spacing, m_doc->m_fp_part_grid_spacing, 0, m_doc->m_fp_snap_angle, -1 );
	m_doc->ProjectModified(true);
	if (dlg.bSetDefault) 
	{
		CArray<CString> oldLines, newLines;
		CString fn = m_doc->m_app_dir + "\\" + "default.cfg";
		ReadFileLines(fn, oldLines);
		m_doc->CollectOptionsStrings(newLines);
		ReplaceLines(oldLines, newLines, "fp_visible_grid_item");
		ReplaceLines(oldLines, newLines, "fp_visible_grid_hidden");
		WriteFileLines(fn, oldLines);
	}
}

void CFootprintView::OnViewPlacementGrid() 
{
	CArray<double> &arr = m_doc->m_fp_part_grid, &hidden = m_doc->m_fp_part_grid_hidden;
	CDlgGridVals dlg (&arr, &hidden, IDS_EditFootprintPlacementGridValues);
	int ret = dlg.DoModal();
	if( ret != IDOK ) 
		return;
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	frm->m_wndMyToolBar.SetLists( &m_doc->m_fp_visible_grid, &m_doc->m_fp_part_grid, NULL,
			m_doc->m_fp_visual_grid_spacing, m_doc->m_fp_part_grid_spacing, 0, m_doc->m_fp_snap_angle, -1 );
	m_doc->ProjectModified(true);
	if (dlg.bSetDefault) 
	{
		CArray<CString> oldLines, newLines;
		CString fn = m_doc->m_app_dir + "\\" + "default.cfg";
		ReadFileLines(fn, oldLines);
		m_doc->CollectOptionsStrings(newLines);
		ReplaceLines(oldLines, newLines, "fp_placement_grid_item");
		ReplaceLines(oldLines, newLines, "fp_placement_grid_hidden");
		WriteFileLines(fn, oldLines);
	}
}

void CFootprintView::HighlightSelection()
{
	// CPT2 Was named HighlightGroup(), but renamed since it works regardless of how many items are in m_sel.
	// Now also sets the cursor mode based on what's in m_sel.  Also checks that the items in the selection are all valid, removing the invalid ones.
	// r317 Now a virtual function within CCommonView.
	CancelHighlight();
	CIter<CPcbItem> ii (&m_sel);
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
		if (i->IsOnPcb())
			i->Highlight();
		else
			m_sel.Remove(i);

	CPcbItem *first = m_sel.First();
	if (m_sel.GetSize()==0)
		SetCursorMode( CUR_FP_NONE_SELECTED );
	// else if (m_sel.GetSize()>=2)
	//  SetCursorMode( CUR_FP_GROUP_SELECTED );				// CPT2 TODO one of these days?
	else if( first->IsPadstack() )
		SetCursorMode( CUR_FP_PAD_SELECTED );
	else if( first->IsRefText() )
		SetCursorMode( CUR_FP_REF_SELECTED );
	else if( first->IsValueText() )
		SetCursorMode( CUR_FP_VALUE_SELECTED );
	else if( first->IsOutlineCorner() )
		SetCursorMode( CUR_FP_POLY_CORNER_SELECTED );
	else if( first->IsOutlineSide() )
		SetCursorMode( CUR_FP_POLY_SIDE_SELECTED );
	else if( first->IsText() )
		SetCursorMode( CUR_FP_TEXT_SELECTED );
	else if( first->IsCentroid() )
		SetCursorMode( CUR_FP_CENTROID_SELECTED );
	else if( first->IsGlue() )
		SetCursorMode( CUR_FP_ADHESIVE_SELECTED );
	else
		SetCursorMode( CUR_FP_NONE_SELECTED );				// CPT2 Unlikely I hope.

	m_lastKeyWasArrow = FALSE;
	m_lastKeyWasGroupRotate=false;
	Invalidate(false);
}

void CFootprintView::CancelHighlight()
{
	m_dlist->CancelHighlight();
	Invalidate(false);
}

