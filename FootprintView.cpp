// FootprintView.cpp : implementation of the CFootprintView class
//

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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ZOOM_RATIO 1.4

#define FKEY_OFFSET_X 4
#define FKEY_OFFSET_Y 4
#define	FKEY_R_W 70
#define FKEY_R_H 30
#define FKEY_STEP (FKEY_R_W+5)
#define FKEY_GAP 20
#define FKEY_SEP_W 16

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

/////////////////////////////////////////////////////////////////////////////
// CFootprintView

IMPLEMENT_DYNCREATE(CFootprintView, CView)

BEGIN_MESSAGE_MAP(CFootprintView, CView)
	//{{AFX_MSG_MAP(CFootprintView)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SYSKEYDOWN()
	ON_WM_SYSKEYUP()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
//	ON_WM_SYSCHAR()
//ON_WM_SYSCOMMAND()
ON_WM_CONTEXTMENU()
ON_COMMAND(ID_ADD_PIN, OnAddPin)
ON_COMMAND(ID_FOOTPRINT_FILE_SAVE_AS, OnFootprintFileSaveAs)
ON_COMMAND(ID_ADD_POLYLINE, OnAddPolyline)
ON_COMMAND(ID_FOOTPRINT_FILE_IMPORT, OnFootprintFileImport)
ON_COMMAND(ID_FOOTPRINT_FILE_CLOSE, OnFootprintFileClose)
ON_COMMAND(ID_FOOTPRINT_FILE_NEW, OnFootprintFileNew)
ON_COMMAND(ID_VIEW_ENTIREFOOTPRINT, OnViewEntireFootprint)
//ON_COMMAND(ID_FP_EDIT_UNDO, OnFpEditUndo)
ON_WM_ERASEBKGND()
ON_COMMAND(ID_FP_MOVE, OnFpMove)
ON_COMMAND(ID_FP_EDITPROPERTIES, OnFpEditproperties)
ON_COMMAND(ID_FP_DELETE, OnFpDelete)
ON_COMMAND(ID_FP_INSERTCORNER, OnPolylineSideAddCorner)
ON_COMMAND(ID_FP_CONVERTTOSTRAIGHT, OnPolylineSideConvertToStraightLine)
ON_COMMAND(ID_FP_CONVERTTOARC, OnPolylineSideConvertToArcCw)
ON_COMMAND(ID_FP_CONVERTTOARC32778, OnPolylineSideConvertToArcCcw)
ON_COMMAND(ID_FP_DELETEOUTLINE, OnPolylineDelete)
ON_COMMAND(ID_FP_MOVE32780, OnPolylineCornerMove)
ON_COMMAND(ID_FP_SETPOSITION, OnPolylineCornerEdit)
ON_COMMAND(ID_FP_DELETECORNER, OnPolylineCornerDelete)
ON_COMMAND(ID_FP_DELETEPOLYLINE, OnPolylineDelete)
ON_COMMAND(ID_FP_POLYLINEPROPERTIES, OnEditPolyline)
ON_COMMAND(ID_FP_MOVE_REF, OnRefMove)
ON_COMMAND(ID_FP_CHANGESIZE_REF, OnRefProperties)
ON_COMMAND(ID_FP_TOOLS_RETURN, OnFootprintFileClose)
ON_COMMAND(ID_FP_TOOLS_FOOTPRINTWIZARD, OnFpToolsFootprintwizard)
ON_COMMAND(ID_TOOLS_FOOTPRINTLIBRARYMANAGER, OnToolsFootprintLibraryManager)
ON_COMMAND(ID_ADD_TEXT32805, OnAddText)
ON_COMMAND(ID_FP_TEXT_EDIT, OnFpTextEdit)
ON_COMMAND(ID_FP_TEXT_MOVE, OnFpTextMove)
ON_COMMAND(ID_FP_TEXT_DELETE, OnFpTextDelete)
ON_COMMAND(ID_FP_ADD_PIN, OnAddPin)
ON_COMMAND(ID_FP_ADD_POLY, OnAddPolyline)
ON_COMMAND(ID_FP_ADD_TEXT, OnAddText)
ON_COMMAND(ID_NONE_RETURNTOPCB, OnFootprintFileClose)
ON_COMMAND(ID_TOOLS_MOVEORIGIN_FP, OnToolsMoveOriginFP)
ON_COMMAND(ID_NONE_MOVEORIGIN, OnToolsMoveOriginFP)
ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
ON_COMMAND(ID_ADD_ADHESIVESPOT, OnAddAdhesive)
ON_COMMAND(ID_CENTROID_SETPARAMETERS, OnCentroidEdit)
ON_COMMAND(ID_CENTROID_MOVE, OnCentroidMove)
ON_COMMAND(ID_ADD_SLOT, OnAddSlot)
ON_COMMAND(ID_ADD_VALUETEXT, OnAddValueText)
ON_COMMAND(ID_ADD_HOLE, OnAddHole)
ON_COMMAND(ID_FP_EDIT, OnValueEdit)
ON_COMMAND(ID_FP_MOVE32923, OnValueMove)
ON_COMMAND(ID_ADHESIVE_EDIT, OnAdhesiveEdit)
ON_COMMAND(ID_ADHESIVE_MOVE, OnAdhesiveMove)
ON_COMMAND(ID_ADHESIVE_DELETE, OnAdhesiveDelete)
ON_COMMAND(ID_CENTROID_ROTATEAXIS, OnCentroidRotateAxis)
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CFootprintView construction/destruction

// GetDocument() is not available at this point, so initialization of the document
// is in InitInstance()
//
CFootprintView::CFootprintView()
{
	m_small_font.CreateFont( 14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, 
		DEFAULT_PITCH | FF_DONTCARE, "Arial" );
#if 0
	m_small_font.CreateFont( 10, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, 
		DEFAULT_PITCH | FF_DONTCARE, "MS Sans Serif" );
#endif
	m_Doc = NULL;
	m_dlist = NULL;
	m_last_mouse_point.x = 0;
	m_last_mouse_point.y = 0;
	m_last_cursor_point.x = 0;
	m_last_cursor_point.y = 0;

	// CPT: left pane width customizable by changing resource string 
	CString s ((LPCSTR) IDS_LeftPaneWidth);
	m_left_pane_w = atoi(s);
	if (m_left_pane_w<=0) 
		m_left_pane_w = 125;
	// CPT: Likewise f-key box width 
	s.LoadStringA(IDS_FKeyWidth);
	m_fkey_w = atoi(s);
	if (m_fkey_w<=0) 
		m_fkey_w = 70;
	// end CPT

	m_bottom_pane_h = 40;	// the bottom pane on screen is this high (pixels)
	m_memDC_created = FALSE;
	m_dragging_new_item = FALSE;
	m_units = MIL;
	m_active_layer = LAY_FP_TOP_COPPER;
	m_cursor_mode = -1;			// CPT.  Ensures that SetFKText() will get called by InitInstance(),  no matter what.
}

// Initialize data for view
// Should only be called after the document is created
// Don't try to draw window until this function has been called
// Enter with fp = pointer to footprint to be edited, or NULL 
//
void CFootprintView::InitInstance( CShape * fp )
{
	m_Doc = GetDocument();
	m_dlist = m_Doc->m_dlist_fp;
	if( m_Doc == NULL || m_dlist == NULL )
		ASSERT(0);

	InitializeView();
	CRect screen_r;
	GetWindowRect( &screen_r );
	m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, 
		m_pcbu_per_pixel, m_org_x, m_org_y );
	for(int i=0; i<m_Doc->m_fp_num_layers; i++ )
	{
		m_dlist->SetLayerRGB( i, C_RGB(m_Doc->m_fp_rgb[i][0], m_Doc->m_fp_rgb[i][1], m_Doc->m_fp_rgb[i][2]) );
		m_dlist->SetLayerVisible( i, 1 );
	}

	// set up footprint to be edited (if provided)
	m_units = m_Doc->m_fp_units;
	if( fp )
	{
		m_fp.Copy( fp );
		if( m_fp.m_units == NM || m_fp.m_units == MM )
			m_units = MM;
		else
			m_units = MIL;
		CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
		frm->m_wndMyToolBar.SetUnits( m_units );
		OnViewEntireFootprint();
	}
	else
	{
		CString s ((LPCSTR) IDS_Untitled);
		m_fp.m_name = s;
	}
	SetWindowTitle( &m_fp.m_name );

	// set up footprint library map (if necessary)
	if( *m_Doc->m_footlibfoldermap.GetDefaultFolder() == "" )
		m_Doc->MakeLibraryMaps( &m_Doc->m_full_lib_dir );

	// initialize window
	m_dlist->RemoveAll();
	m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
	FootprintModified( FALSE );
	m_Doc->m_footprint_name_changed = FALSE;
	ClearUndo();
	ClearRedo();
	ShowSelectStatus();
	ShowActiveLayer();
	Invalidate( FALSE );
}

// Initialize view with application defaults
//
void CFootprintView::InitializeView()
{
	if( !m_dlist )
		ASSERT(0);

	// set defaults
	CancelSelection();
	m_debug_flag = 0;
	m_dragging_new_item = 0;

	// default screen coords
	m_pcbu_per_pixel = 5.0*PCBU_PER_MIL;	// 5 mils per pixel
	m_org_x = -100.0*PCBU_PER_MIL;			// lower left corner of window
	m_org_y = -100.0*PCBU_PER_MIL;

	// grid defaults
	m_Doc->m_fp_snap_angle = 45;
	CancelSelection();
	m_left_pane_invalid = TRUE;
	EnableUndo( FALSE );
	EnableRedo( FALSE );
	Invalidate( FALSE );

#if 0
	// visibility
	for( int il=0; il<m_Doc->m_num_layers; il++ )
		m_Doc->m_fp_vis[il] = 1;
#endif
}

CFootprintView::~CFootprintView()
{
	CEditShape * the_fp = &m_fp;
}

BOOL CFootprintView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CFootprintView drawing

void CFootprintView::OnDraw(CDC* pDC)
{
#define VSTEP 14

	if( !m_Doc )
	{
		// don't try to draw until InitInstance() has been called
		return;
	}

	// get client rectangle
	GetClientRect( &m_client_r );

	// draw stuff on left pane
	if( m_left_pane_invalid )
	{
		// erase previous contents if changed
		CBrush brush( RGB(255, 255, 255) );
		CPen pen( PS_SOLID, 1, RGB(255, 255, 255) );
		CBrush * old_brush = pDC->SelectObject( &brush );
		CPen * old_pen = pDC->SelectObject( &pen );
		// erase left pane
		CRect r = m_client_r;
		r.right = m_left_pane_w;
		r.bottom -= m_bottom_pane_h;
		pDC->Rectangle( &r );
		// erase bottom pane
		r = m_client_r;
		r.top = r.bottom - m_bottom_pane_h;
		pDC->Rectangle( &r );
		pDC->SelectObject( old_brush ); 
		pDC->SelectObject( old_pen );
		m_left_pane_invalid = FALSE;
	}
	CFont * old_font = pDC->SelectObject( &m_small_font );
	int y_off = 10;
	int x_off = 10;

	for( int i=0; i<m_Doc->m_fp_num_layers; i++ )  
	{
		// i = position index
		CRect r( x_off, i*VSTEP+y_off, x_off+12, i*VSTEP+12+y_off );
		CBrush brush( RGB(m_Doc->m_fp_rgb[i][0], m_Doc->m_fp_rgb[i][1], m_Doc->m_fp_rgb[i][2]) );
		if( m_Doc->m_fp_vis[i] )
		{
			// draw colored rectangle
			CBrush * old_brush = pDC->SelectObject( &brush );
			pDC->Rectangle( &r );
			pDC->SelectObject( old_brush );
		}
		else
		{
			// if layer is invisible, draw box with X
			pDC->Rectangle( &r );
			pDC->MoveTo( r.left, r.top );
			pDC->LineTo( r.right, r.bottom );
			pDC->MoveTo( r.left, r.bottom );
			pDC->LineTo( r.right, r.top );
		}
		r.left += 20;
		r.right += 120;
		r.bottom += 5;
		CString s;
		if( i == LAY_FP_PAD_THRU ) 
			pDC->DrawText( "drilled hole", -1, &r, 0 );					// CPT:  will be replaced when feature #31 is merged.
		else
			s.LoadStringA( IDS_FpLayerStr + i ),
			pDC->DrawText( s, -1, &r, 0 ); 
		if( i >= LAY_FP_TOP_COPPER && i <= LAY_FP_BOTTOM_COPPER ) 
		{
			CString num_str; 
			num_str.Format( "[%d*]", i-LAY_FP_TOP_COPPER+1 );
			CRect nr = r;
			nr.left = nr.right - 55;
			pDC->DrawText( num_str, -1, &nr, DT_TOP );
		}
		CRect ar = r;
		ar.left = 2;
		ar.right = 8;
		ar.bottom -= 5;
		if( i == m_active_layer )
		{
			// draw arrowhead
			pDC->MoveTo( ar.left, ar.top+1 );
			pDC->LineTo( ar.right-1, (ar.top+ar.bottom)/2 );
			pDC->LineTo( ar.left, ar.bottom-1 );
			pDC->LineTo( ar.left, ar.top+1 );
		}
		else
		{
			// erase arrowhead
			pDC->FillSolidRect( &ar, RGB(255,255,255) ); 
		}
	}
	CRect r( x_off, NUM_FP_LAYERS*VSTEP+y_off, x_off+120, NUM_FP_LAYERS*VSTEP+12+y_off );
	pDC->DrawText( "* Use numeric", -1, &r, DT_TOP );
	r.bottom += VSTEP;
	r.top += VSTEP;
	pDC->DrawText( "keys to display", -1, &r, DT_TOP );
	r.bottom += VSTEP;
	r.top += VSTEP;
	pDC->DrawText( "layer on top", -1, &r, DT_TOP );

	// draw function keys on bottom pane
	DrawBottomPane();

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

/////////////////////////////////////////////////////////////////////////////
// CFootprintView diagnostics

#ifdef _DEBUG
void CFootprintView::AssertValid() const
{
	CView::AssertValid();
}

void CFootprintView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CFreePcbDoc* CFootprintView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFreePcbDoc)));
	return (CFreePcbDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFootprintView message handlers

// Window was resized
//
void CFootprintView::OnSize(UINT nType, int cx, int cy) 
{

	CView::OnSize(nType, cx, cy);

	// update client rect and create clipping region
	GetClientRect( &m_client_r );
	m_pcb_rgn.DeleteObject();
	m_pcb_rgn.CreateRectRgn( m_left_pane_w, m_client_r.bottom-m_bottom_pane_h,
		m_client_r.right, m_client_r.top );

	// update display mapping for display list
	if( m_dlist )
	{
		CRect screen_r;
		GetWindowRect( &screen_r );
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, 
					m_org_x, m_org_y );
	}
	
	// create memory DC and DDB
	if( !m_memDC_created && m_client_r.right != 0 )
	{
		CDC * pDC = GetDC();
		m_memDC.CreateCompatibleDC( pDC );
		m_memDC_created = TRUE;
		m_bitmap.CreateCompatibleBitmap( pDC, m_client_r.right, m_client_r.bottom );
		m_old_bitmap = m_memDC.SelectObject( &m_bitmap );
		m_bitmap_rect = m_client_r;
		ReleaseDC( pDC );
	}
	else if( m_memDC_created && (m_bitmap_rect != m_client_r) )
	{
		CDC * pDC = GetDC();
		m_memDC.SelectObject( m_old_bitmap );
		m_bitmap.DeleteObject();
		m_bitmap.CreateCompatibleBitmap( pDC, m_client_r.right, m_client_r.bottom );
		m_old_bitmap = m_memDC.SelectObject( &m_bitmap );
		m_bitmap_rect = m_client_r;
		ReleaseDC( pDC );
	}
}

// Left mouse button pressed down, we should probably do something
//
void CFootprintView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CDC * pDC = NULL;	// !! remember to ReleaseDC() at end, if necessary
	CPoint tp = WindowToPCB( point );
	if( point.y > (m_client_r.bottom-m_bottom_pane_h) )
	{
		// clicked in bottom pane, test for hit on function key rectangle
		for( int i=0; i<8; i++ )
		{
			CRect r( FKEY_OFFSET_X+i*FKEY_STEP+(i/4)*FKEY_GAP, 
				m_client_r.bottom-FKEY_OFFSET_Y-FKEY_R_H, 
				FKEY_OFFSET_X+i*FKEY_STEP+(i/4)*FKEY_GAP+FKEY_R_W,
				m_client_r.bottom-FKEY_OFFSET_Y );
			if( r.PtInRect( point ) )
			{
				// fake function key pressed
				int nChar = i + 112;
				HandleKeyPress( nChar, 0, 0 );
				if( i == 7 )
					return;
			}
		}
	}
	else if( point.x < m_left_pane_w )
	{
		// clicked in left pane
		CRect r = m_client_r;
		int y_off = 10;
		int x_off = 10;
		for( int i=0; i<m_Doc->m_fp_num_layers; i++ )
		{
			// i = position index
			// get color square
			r.left = x_off;
			r.right = x_off+12;
			r.top = i*VSTEP+y_off;
			r.bottom = i*VSTEP+12+y_off;
			if( r.PtInRect( point ) && i > LAY_BACKGND )
			{
				// clicked in color square
				m_Doc->m_fp_vis[i] = !m_Doc->m_fp_vis[i];
				m_dlist->SetLayerVisible( i, m_Doc->m_fp_vis[i] );
				Invalidate( FALSE );
			}
			else
			{
				// get layer name rect
				r.left += 20;
				r.right += 120;
				r.bottom += 5;
				if( r.PtInRect( point ) )
				{
					// clicked on layer name
					switch( i )
					{
					case LAY_FP_TOP_COPPER: HandleKeyPress( '1', 0, 0 ); Invalidate( FALSE ); break;
					case LAY_FP_TOP_COPPER+1: HandleKeyPress( '2', 0, 0 ); Invalidate( FALSE ); break;
					case LAY_FP_TOP_COPPER+2: HandleKeyPress( '3', 0, 0 ); Invalidate( FALSE ); break;
					}
				}
			}
		}
		y_off = r.bottom + 2*VSTEP;
	}
	else if( point.x > m_left_pane_w )
	{
		// clicked in PCB pane
		if(	CurNone() || CurSelected() )
		{
			// we are not dragging anything, see if new item selected
			CPoint p = WindowToPCB( point );

#if 0
			id id;
//**			void * ptr = m_dlist->TestSelect( p.x, p.y, &id, &m_sel_layer, &m_sel_id );
			void * ptr = NULL;
			id.Clear();
			//**
#endif

			enum { MAX_HITS = 500 };
			CHitInfo hit_info[MAX_HITS];
			int num_hits;

			int idx = m_dlist->TestSelect(
				p.x, p.y,					  // Point
				hit_info, MAX_HITS, num_hits, // Hit Information
				&m_sel_id					  // Exclusions
			);

			// deselect previously selected item
			CancelSelection();

			if( idx >= 0 )
			{
				id id = hit_info[idx].ID;
				// now check for new selection
				if( id.IsAnyFootItem() )
				{
					// something was selected
					m_sel_id = id;
					if( id.IsFootPad() )
					{
						// pad selected
						m_fp.HighlightPad( id.I2() );
						SetCursorMode( CUR_FP_PAD_SELECTED );
						Invalidate( FALSE );
					}
					else if( id.IsFootRef() )
					{
						// ref text selected
						m_fp.m_ref_text.Highlight();
						SetCursorMode( CUR_FP_REF_SELECTED );
					}
					else if( id.IsFootValue() )
					{
						// value text selected
						m_fp.m_value_text.Highlight();
						SetCursorMode( CUR_FP_VALUE_SELECTED );
					}
					else if( id.IsFootPolyCorner() )
					{
						// corner selected
						int ip = m_sel_id.I2();
						int ic = m_sel_id.I3();
						m_fp.m_outline_poly[ip].HighlightCorner( ic );
						SetCursorMode( CUR_FP_POLY_CORNER_SELECTED );
					}
					else if( id.IsFootPolySide() )
					{
						// side selected
						int ip = m_sel_id.I2();
						int is = m_sel_id.I3();
						m_fp.m_outline_poly[ip].HighlightSide( is );
						SetCursorMode( CUR_FP_POLY_SIDE_SELECTED );
					}
					else if( id.IsFootText() )
					{
						// text selected
						m_sel_text = (CText*)hit_info[idx].ptr;
						SetCursorMode( CUR_FP_TEXT_SELECTED );
						m_fp.m_tl->HighlightText( m_sel_text );
					}
					else if( id.IsCentroid() )
					{
						// centroid selected
						SetCursorMode( CUR_FP_CENTROID_SELECTED );
						m_fp.SelectCentroid();
						Invalidate( FALSE );
					}
					else if( id.IsGlue() )
					{
						// glue spot selected
						SetCursorMode( CUR_FP_ADHESIVE_SELECTED );
						m_fp.SelectAdhesive( id.I2() );
						Invalidate( FALSE );
					}
				}
			}
			else
			{
				// nothing selected
				m_sel_id.Clear();
				SetCursorMode( CUR_FP_NONE_SELECTED );
			}
		}
		else if( m_cursor_mode == CUR_FP_DRAG_PAD )
		{
			// we were dragging pad, move it
			if( !m_dragging_new_item )
				PushUndo();
			int i = m_sel_id.I2();	// pin number (zero-based)
			CPoint p = m_last_cursor_point;
			m_dlist->StopDragging();
			int dx = p.x - m_fp.m_padstack[i].x_rel;
			int dy = p.y - m_fp.m_padstack[i].y_rel;
			for( int ip=i; ip<(i+m_drag_num_pads); ip++ )
			{
				m_fp.m_padstack[ip].x_rel += dx;
				m_fp.m_padstack[ip].y_rel += dy;
			}
			if( m_drag_num_pads == 1 )
			{
				// only rotate if single pad (not row)
				int old_angle = m_fp.m_padstack[m_sel_id.I2()].angle;
				int angle = old_angle + m_dlist->GetDragAngle();
				if( angle>270 )
					angle = angle - 360;
				m_fp.m_padstack[i].angle = angle;
			}
			m_dragging_new_item = FALSE;
			m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
			SetCursorMode( CUR_FP_PAD_SELECTED );
			m_fp.HighlightPad( m_sel_id.I2() );
			FootprintModified( TRUE );
		}
		else if( m_cursor_mode == CUR_FP_DRAG_REF )
		{
			// we were dragging ref, move it
			PushUndo();
			CPoint p = m_last_cursor_point;
			m_dlist->StopDragging();
			int old_angle = m_fp.m_ref_angle;
			int angle = old_angle + m_dlist->GetDragAngle();
			if( angle>270 )
				angle = angle - 360;
			m_fp.m_ref_xi = p.x;
			m_fp.m_ref_yi = p.y;
			m_fp.m_ref_angle = angle;
			m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
			SetCursorMode( CUR_FP_REF_SELECTED );
			m_fp.m_ref_text.Highlight();
			FootprintModified( TRUE );
		}
		else if( m_cursor_mode == CUR_FP_DRAG_VALUE )
		{
			// we were dragging value, move it
			PushUndo();
			CPoint p = m_last_cursor_point;
			m_dlist->StopDragging();
			int old_angle = m_fp.m_value_angle;
			int angle = old_angle + m_dlist->GetDragAngle();
			if( angle>270 )
				angle = angle - 360;
			m_fp.m_value_xi = p.x;
			m_fp.m_value_yi = p.y;
			m_fp.m_value_angle = angle;
			m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
			SetCursorMode( CUR_FP_VALUE_SELECTED );
			m_fp.m_value_text.Highlight();
			FootprintModified( TRUE );
		}
		else if( m_cursor_mode == CUR_FP_DRAG_POLY_MOVE )
		{
			// move corner of polyline
			PushUndo();
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			BOOL bEnforceCircularArcs = FALSE;
			if( m_fp.m_outline_poly[m_sel_id.I2()].Layer() >= LAY_FP_TOP_COPPER
				&& m_fp.m_outline_poly[m_sel_id.I2()].Layer() <= LAY_FP_BOTTOM_COPPER )
			{
				bEnforceCircularArcs = TRUE;
			}
			BOOL bMod = m_fp.m_outline_poly[m_sel_id.I2()].MoveCorner( m_sel_id.I3(), p.x, p.y, bEnforceCircularArcs );
			m_fp.m_outline_poly[m_sel_id.I2()].HighlightCorner( m_sel_id.I3() );
			if( bMod )
			{
				CString s ((LPCSTR) IDS_ArcsWithEndpointsNotAt45DegreeAngles);
				AfxMessageBox( s );
			}
			SetCursorMode( CUR_FP_POLY_CORNER_SELECTED );
			FootprintModified( TRUE );
		}
		else if( m_cursor_mode == CUR_FP_DRAG_POLY_INSERT )
		{
			// insert new corner into polyline
			PushUndo();
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			m_fp.m_outline_poly[m_sel_id.I2()].InsertCorner( m_sel_id.I3()+1, p.x, p.y );
			// now select new corner
			m_fp.m_outline_poly[m_sel_id.I2()].HighlightCorner( m_sel_id.I3()+1 );
			m_sel_id.Set( ID_PART, -1, ID_POLYLINE, -1, m_sel_id.I2(), ID_SEL_CORNER, -1, m_sel_id.I3()+1 );
			SetCursorMode( CUR_FP_POLY_CORNER_SELECTED );
			FootprintModified( TRUE );
		}
		else if( m_cursor_mode == CUR_FP_ADD_POLY )
		{
			// place first corner of polyline
			PushUndo();
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			// make new polyline
			int ip = m_fp.m_outline_poly.GetSize();
			m_sel_id.Set( ID_PART, -1, ID_POLYLINE, -1, ip, ID_SEL_CORNER, -1, 0 );
			m_fp.m_outline_poly.SetSize( ip+1 );
			m_fp.m_outline_poly[ip].Start( m_polyline_layer, m_polyline_width, 
				20*NM_PER_MIL, p.x, p.y, 0, &m_sel_id, NULL );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_FP_SELECTION, 1, 1 );
			SetCursorMode( CUR_FP_DRAG_POLY_1 );
			FootprintModified( TRUE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_FP_DRAG_POLY_1 )
		{
			// place second corner of polyline
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_fp.m_outline_poly[m_sel_id.I2()].AppendCorner( p.x, p.y, m_polyline_style );
			m_fp.m_outline_poly[m_sel_id.I2()].Draw( m_dlist );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_FP_SELECTION, 1, 1 );
			m_sel_id.SetI3( m_sel_id.I3() + 1 );
			SetCursorMode( CUR_FP_DRAG_POLY );
			FootprintModified( TRUE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_FP_DRAG_POLY )
		{
			// place subsequent corners of board outline
			PushUndo();
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			if( p.x == m_fp.m_outline_poly[m_sel_id.I2()].X(0)
				&& p.y == m_fp.m_outline_poly[m_sel_id.I2()].Y(0) )
			{
				// this point is the start point, close the polyline and quit
				m_fp.m_outline_poly[m_sel_id.I2()].Close( m_polyline_style );
				SetCursorMode( CUR_FP_NONE_SELECTED );
				m_dlist->StopDragging();
			}
			else
			{
				// add corner to polyline
				m_fp.m_outline_poly[m_sel_id.I2()].AppendCorner( p.x, p.y, m_polyline_style );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_FP_SELECTION, 1, 1 );
				m_sel_id.SetI3( m_sel_id.I3() + 1 );
				m_snap_angle_ref = m_last_cursor_point;
			}
			FootprintModified( TRUE );
		}
		else if( m_cursor_mode == CUR_FP_DRAG_TEXT )
		{
			if( !m_dragging_new_item )
				PushUndo();	// if new item, PushUndo() has already been called
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			int old_angle = m_sel_text->m_angle;
			int angle = old_angle + m_dlist->GetDragAngle();
			if( angle>270 )
				angle = angle - 360;
			int old_mirror = m_sel_text->m_mirror;
			BOOL negative = m_sel_text->m_bNegative;
			int mirror = (old_mirror + m_dlist->GetDragSide())%2;
			int layer = m_sel_text->m_layer;
			m_fp.m_tl->MoveText( m_sel_text, p.x, p.y, 
									angle, mirror, negative, layer );
			m_dragging_new_item = FALSE;
			SetCursorMode( CUR_FP_TEXT_SELECTED );
			m_fp.m_tl->HighlightText( m_sel_text );
			FootprintModified( TRUE );
		}
		else if( m_cursor_mode == CUR_FP_MOVE_ORIGIN )
		{
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			PushUndo();
			MoveOrigin( p.x, p.y );
			SetCursorMode( CUR_FP_NONE_SELECTED );
			FootprintModified( TRUE );
			OnViewEntireFootprint();
		}
		else if( m_cursor_mode == CUR_FP_DRAG_CENTROID )
		{
			CPoint p;
			p = m_last_cursor_point;
			m_fp.CancelDraggingCentroid();
			PushUndo();
			m_fp.Undraw();
			m_fp.m_centroid_x = p.x;
			m_fp.m_centroid_y = p.y;
			m_fp.m_centroid_type = CENTROID_DEFINED;
			for( int idot=0; idot<m_fp.m_glue.GetSize(); idot++ )
			{
				glue * g = &m_fp.m_glue[idot];
				if( g->type == GLUE_POS_CENTROID )
				{
					g->x_rel = p.x;
					g->y_rel = p.y;
				}
			}
			m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
			m_fp.SelectCentroid();
			SetCursorMode( CUR_FP_CENTROID_SELECTED );
			FootprintModified( TRUE );
		}
		else if( m_cursor_mode == CUR_FP_DRAG_ADHESIVE )
		{
			int idot = m_sel_id.I2();
			CPoint p;
			p = m_last_cursor_point;
			m_fp.CancelDraggingAdhesive( idot );
			m_fp.Undraw();
			m_fp.m_glue[idot].x_rel = p.x;
			m_fp.m_glue[idot].y_rel = p.y;
			m_fp.m_glue[idot].type = GLUE_POS_DEFINED;
			m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
			m_fp.SelectAdhesive( idot );
			SetCursorMode( CUR_FP_ADHESIVE_SELECTED );
			FootprintModified( TRUE );
			m_dragging_new_item = FALSE;	// default
		}
		ShowSelectStatus();
	}
	Invalidate( FALSE );
	if( pDC )
		ReleaseDC( pDC );
	CView::OnLButtonDown(nFlags, point);
}

// left double-click
//
void CFootprintView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
#if 0
	if( m_cursor_mode == CUR_PART_SELECTED )
	{
		SetCursorMode( CUR_DRAG_PART );
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		CPoint p = m_last_mouse_point;
		m_dlist->StartDraggingSelection( pDC, p.x, p.y );
	}
	if( m_cursor_mode == CUR_REF_SELECTED )
	{
		SetCursorMode( CUR_DRAG_REF );
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		CPoint p = m_last_mouse_point;
		m_dlist->StartDraggingSelection( pDC, p.x, p.y );
	}
#endif
	CView::OnLButtonDblClk(nFlags, point);
}

// right mouse button
//
void CFootprintView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	m_disable_context_menu = 1;
	if( m_cursor_mode == CUR_FP_DRAG_PAD )	
	{
		m_fp.CancelDraggingPad( m_sel_id.I2() );
		if( m_dragging_new_item )
		{
			UndoNoRedo();
			CancelSelection();
		}
		else
		{
			m_fp.CancelDraggingPad( m_sel_id.I2() );
			m_fp.HighlightPad( m_sel_id.I2() );
			SetCursorMode( CUR_FP_PAD_SELECTED );
		}
	}
	else if( m_cursor_mode == CUR_FP_DRAG_REF )
	{
		m_fp.m_ref_text.CancelDragging();
		m_fp.m_ref_text.Highlight();
		SetCursorMode( CUR_FP_REF_SELECTED );
	}
	else if( m_cursor_mode == CUR_FP_DRAG_VALUE )
	{
		m_fp.m_value_text.CancelDragging();
		m_fp.m_value_text.Highlight();
		SetCursorMode( CUR_FP_VALUE_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_FP_ADD_POLY )
	{
		m_dlist->StopDragging();
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_FP_DRAG_POLY_1 )
	{
		m_dlist->StopDragging();
		OnPolylineDelete();
	}
	else if( ( m_cursor_mode == CUR_FP_DRAG_POLY 
				&& m_fp.m_outline_poly[m_sel_id.I2()].NumCorners()<3 
				&& m_polyline_closed_flag )
		  || ( m_cursor_mode == CUR_FP_DRAG_POLY 
				&& m_fp.m_outline_poly[m_sel_id.I2()].NumCorners()<2 
				&& !m_polyline_closed_flag ) )
	{
		m_dlist->StopDragging();
		OnPolylineDelete();
	}
	else if( m_cursor_mode == CUR_FP_DRAG_POLY )
	{
		m_dlist->StopDragging();
		if( m_polyline_closed_flag )
		{
			PushUndo();
			m_fp.m_outline_poly[m_sel_id.I2()].Close( m_polyline_style );
		}
		CancelSelection();
		FootprintModified( TRUE );
	}
	else if( m_cursor_mode == CUR_FP_DRAG_POLY_INSERT )
	{
		m_dlist->StopDragging();
		m_fp.m_outline_poly[m_sel_id.I2()].MakeVisible();
		m_fp.m_outline_poly[m_sel_id.I2()].HighlightSide( m_sel_id.I3() );
		SetCursorMode( CUR_FP_POLY_SIDE_SELECTED );
	}
	else if( m_cursor_mode == CUR_FP_DRAG_POLY_MOVE )
	{
		m_dlist->StopDragging();
		m_fp.m_outline_poly[m_sel_id.I2()].MakeVisible();
		SetCursorMode( CUR_FP_POLY_CORNER_SELECTED );
		m_fp.m_outline_poly[m_sel_id.I2()].HighlightCorner( m_sel_id.I3() );
	}
	else if( m_cursor_mode == CUR_FP_DRAG_TEXT )
	{
		m_fp.m_tl->CancelDraggingText( m_sel_text );
		if( m_dragging_new_item )
		{
			m_fp.m_tl->RemoveText( m_sel_text );
			CancelSelection();
		}
		else
		{
			SetCursorMode( CUR_FP_TEXT_SELECTED );
		}
	}
	else if( m_cursor_mode == CUR_FP_MOVE_ORIGIN )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_FP_NONE_SELECTED );
	}
	else if( m_cursor_mode == CUR_FP_DRAG_CENTROID )
	{
		m_fp.CancelDraggingCentroid();
		m_fp.SelectCentroid();
		SetCursorMode( CUR_FP_CENTROID_SELECTED );
	}
	else if( m_cursor_mode == CUR_FP_DRAG_ADHESIVE )
	{
		m_fp.CancelDraggingAdhesive( m_sel_id.I2() );
		UndoNoRedo();	// restore state before dragging
		if( m_dragging_new_item )
		{
			// cancel new item
			CancelSelection();
		}
		else
		{
			// reselect item and change mode
			m_fp.SelectAdhesive( m_sel_id.I2() );
			SetCursorMode( CUR_FP_ADHESIVE_SELECTED );
		}
	}
	else
	{
		m_disable_context_menu = 0;
	}
	m_dragging_new_item = FALSE;
	Invalidate( FALSE );
	ShowSelectStatus();
	CView::OnRButtonDown(nFlags, point);
}

// System Key on keyboard pressed down
//
void CFootprintView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if( nChar == 121 )
		OnKeyDown( nChar, nRepCnt, nFlags);
	else
		CView::OnSysKeyDown(nChar, nRepCnt, nFlags);
}

// System Key on keyboard pressed down
//
void CFootprintView::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if( nChar != 121 )
		CView::OnSysKeyUp(nChar, nRepCnt, nFlags);
}

// Key on keyboard pressed down
//
void CFootprintView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	HandleKeyPress( nChar, nRepCnt, nFlags );

	// don't pass through SysKey F10
	if( nChar != 121 )
		CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

// Key on keyboard pressed down
//
void CFootprintView::HandleKeyPress(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	int fk = FK_FP_NONE;
	if( nChar >= 112 && nChar <= 123 )		// Function key 
	{
		fk = m_fkey_option[nChar-112];
	}
	if( nChar == '1' || nChar == '2' || nChar == '3' )
	{
		// change visibility of layers
		if( nChar == '1' )
		{
			m_active_layer = LAY_FP_TOP_COPPER;
			ShowActiveLayer();
		}
		else if( nChar == '2' )
		{
			m_active_layer = LAY_FP_INNER_COPPER;
			ShowActiveLayer();
		}
		else if( nChar == '3' )
		{
			m_active_layer = LAY_FP_BOTTOM_COPPER;
			ShowActiveLayer();
		}
	}

	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );

	// get cursor position and convert to PCB coords
	CPoint p;
	GetCursorPos( &p );		// cursor pos in screen coords
	p = ScreenToPCB( p );	// convert to PCB coords

	// now handle key-press
	switch( m_cursor_mode )
	{
	case  CUR_FP_NONE_SELECTED:
		if( fk == FK_FP_ADD_PAD )
			OnAddPin();
		else if( fk == FK_FP_ADD_TEXT )
			OnAddText();
		else if( fk == FK_FP_ADD_POLYLINE )
			OnAddPolyline();
		break;

	case CUR_FP_PAD_SELECTED:
		if( fk == FK_FP_DELETE_PAD || nChar == 46 )
			OnPadDelete( m_sel_id.I2() );
		else if( fk == FK_FP_EDIT_PAD )
			OnPadEdit( m_sel_id.I2() );
		else if( fk == FK_FP_MOVE_PAD )
			OnPadMove( m_sel_id.I2() );
		break;

	case CUR_FP_REF_SELECTED:
		if( fk == FK_FP_EDIT_PROPERTIES )
			OnRefProperties();
		else if( fk == FK_FP_MOVE_REF )
			OnRefMove();
		break;

	case CUR_FP_VALUE_SELECTED:
		if( fk == FK_FP_EDIT_PROPERTIES )
			OnValueEdit();
		else if( fk == FK_FP_MOVE_VALUE )
			OnValueMove();
		break;

	case CUR_FP_POLY_CORNER_SELECTED:
		if( fk == FK_FP_SET_POSITION )
			OnPolylineCornerEdit();
		else if( fk == FK_FP_MOVE_CORNER )
			OnPolylineCornerMove();
		else if( fk == FK_FP_DELETE_CORNER || nChar == 46 )
		{
			OnPolylineCornerDelete();
			FootprintModified( TRUE );
		}
		else if( fk == FK_FP_DELETE_POLYLINE )
		{
			OnPolylineDelete();
			FootprintModified( TRUE );
		}
		break;

	case CUR_FP_POLY_SIDE_SELECTED:
		if( fk == FK_FP_POLY_STRAIGHT )
			OnPolylineSideConvertToStraightLine();
		else if( fk == FK_FP_POLY_ARC_CW )
			OnPolylineSideConvertToArcCw();
		else if( fk == FK_FP_POLY_ARC_CCW )
			OnPolylineSideConvertToArcCcw();
		else if( fk == FK_FP_ADD_CORNER )
			OnPolylineSideAddCorner();
		else if( fk == FK_FP_DELETE_POLYLINE || nChar == 46 )
			OnPolylineDelete();
		FootprintModified( TRUE );
		break;

	case CUR_FP_TEXT_SELECTED:
		if( fk == FK_FP_EDIT_TEXT )
			OnFpTextEdit();
		else if( fk == FK_FP_MOVE_TEXT )
			OnFpTextMove();
		else if( fk == FK_FP_DELETE_TEXT || nChar == 46 )
			OnFpTextDelete();
		break;

	case CUR_FP_CENTROID_SELECTED:
		if( fk == FK_FP_EDIT_CENTROID )
			OnCentroidEdit();
		else if( fk == FK_FP_ROTATE_CENTROID )
			OnCentroidRotateAxis();
		else if( fk == FK_FP_MOVE_CENTROID )
			OnCentroidMove();
		break;

	case CUR_FP_ADHESIVE_SELECTED:
		if( fk == FK_FP_EDIT_ADHESIVE )
			OnAdhesiveEdit();
		else if( fk == FK_FP_MOVE_ADHESIVE )
			OnAdhesiveMove();
		else if( fk == FK_FP_DELETE_ADHESIVE || nChar == 46 )
			OnAdhesiveDelete();
		break;

	case  CUR_FP_DRAG_PAD:
		if( fk == FK_FP_ROTATE_PAD )
			m_dlist->IncrementDragAngle( pDC );
		break;

	case  CUR_FP_DRAG_VALUE: 
		if( fk == FK_FP_ROTATE_VALUE )
			m_dlist->IncrementDragAngle( pDC );
		break;

	case  CUR_FP_DRAG_REF: 
		if( fk == FK_FP_ROTATE_REF )
			m_dlist->IncrementDragAngle( pDC );
		break;

	case  CUR_FP_DRAG_POLY_1:
	case  CUR_FP_DRAG_POLY:
		if( fk == FK_FP_POLY_STRAIGHT )
		{
			m_polyline_style = CPolyLine::STRAIGHT;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_FP_POLY_ARC_CW )
		{
			m_polyline_style = CPolyLine::ARC_CW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_FP_POLY_ARC_CCW )
		{
			m_polyline_style = CPolyLine::ARC_CCW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		break;

	case  CUR_FP_DRAG_TEXT:
		if( fk == FK_FP_ROTATE_TEXT )
			m_dlist->IncrementDragAngle( pDC );
		break;

	default: 
		break;
	}	// end switch

	if( nChar == VK_HOME )
	{
		// Home key pressed
		OnViewEntireFootprint();
	}
	if( nChar == ' ' )
	{
		// space bar pressed, center window on cursor then center cursor
		m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
		m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
		CRect screen_r;
		GetWindowRect( &screen_r );
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, 
			m_org_x, m_org_y );
		Invalidate( FALSE );
		p = PCBToScreen( p );
		SetCursorPos( p.x, p.y - 4 );
	}
	else if( nChar == 33 )
	{
		// PgUp pressed, zoom in
		if( m_pcbu_per_pixel > 254 )
		{
			m_pcbu_per_pixel = m_pcbu_per_pixel/ZOOM_RATIO;
			m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
			m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
			CRect screen_r;
			GetWindowRect( &screen_r );
			m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, 
				m_org_x, m_org_y );
			Invalidate( FALSE );
			p = PCBToScreen( p );
			SetCursorPos( p.x, p.y - 4 );
		}
	}
	else if( nChar == 34 )
	{
		// PgDn pressed, zoom out
		// first, make sure that window boundaries will be OK
		int org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel*ZOOM_RATIO)/2;
		int org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel*ZOOM_RATIO)/2;
		int max_x = org_x + (m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel*ZOOM_RATIO;
		int max_y = org_y + (m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel*ZOOM_RATIO;
		if( org_x > -PCB_BOUND && org_x < PCB_BOUND && max_x > -PCB_BOUND && max_x < PCB_BOUND
			&& org_y > -PCB_BOUND && org_y < PCB_BOUND && max_y > -PCB_BOUND && max_y < PCB_BOUND )
		{
			// OK, do it
			m_org_x = org_x;
			m_org_y = org_y;
			m_pcbu_per_pixel = m_pcbu_per_pixel*ZOOM_RATIO;
			CRect screen_r;
			GetWindowRect( &screen_r );
			m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, 
				m_org_x, m_org_y );
			Invalidate( FALSE );
			p = PCBToScreen( p );
			SetCursorPos( p.x, p.y - 4 );
		}
	}
	else if( nChar == 27 )
	{
		// ESC key, simulate a right-click
		OnRButtonDown( 0, NULL );
	}
	ReleaseDC( pDC );
	ShowSelectStatus();
}

// Mouse moved
//
void CFootprintView::OnMouseMove(UINT nFlags, CPoint point) 
{
	m_last_mouse_point = WindowToPCB( point );
	SnapCursorPoint( m_last_mouse_point );
}

/////////////////////////////////////////////////////////////////////////
// Utility functions
//

// Set the device context to world coords
//
int CFootprintView::SetDCToWorldCoords( CDC * pDC )
{
	m_dlist->SetDCToWorldCoords( pDC, &m_memDC, m_org_x, m_org_y );

	return 0;
}


// Convert point in window coords to PCB units (i.e. nanometers)
//
CPoint CFootprintView::WindowToPCB( CPoint point )
{
	CPoint p;
	p.x = (point.x-m_left_pane_w)*m_pcbu_per_pixel + m_org_x;
	p.y = (m_client_r.bottom-m_bottom_pane_h-point.y)*m_pcbu_per_pixel + m_org_y;
	return p;
}

// Convert point in screen coords to PCB units
//
CPoint CFootprintView::ScreenToPCB( CPoint point )
{
	CPoint p;
	CRect wr;
	GetWindowRect( &wr );		// client rect in screen coords
	p.x = point.x - wr.left;
	p.y = point.y - wr.top;
	p = WindowToPCB( p );
	return p;
}

// Convert point in PCB units to screen coords
//
CPoint CFootprintView::PCBToScreen( CPoint point )
{
	CPoint p;
	CRect wr;
	GetWindowRect( &wr );		// client rect in screen coords
	p.x = (point.x - m_org_x)/m_pcbu_per_pixel+m_left_pane_w+wr.left;
	p.y = (m_org_y - point.y)/m_pcbu_per_pixel-m_bottom_pane_h+wr.bottom;
	return p;
}

// Set cursor mode, update function key menu if necessary
//
void CFootprintView::SetCursorMode( int mode )
{
	if( mode != m_cursor_mode )
	{
		SetFKText( mode );
		m_cursor_mode = mode;
		ShowSelectStatus();
	}
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
		m_fkey_option[2] = FK_FP_ADD_POLYLINE;
		m_fkey_option[3] = FK_FP_ADD_PAD;
		break;

	case CUR_FP_PAD_SELECTED:
		m_fkey_option[0] = FK_FP_EDIT_PAD;
		m_fkey_option[3] = FK_FP_MOVE_PAD;
		m_fkey_option[6] = FK_FP_DELETE_PAD;
		break;

	case CUR_FP_REF_SELECTED:
		m_fkey_option[0] = FK_FP_EDIT_PROPERTIES;
		m_fkey_option[3] = FK_FP_MOVE_REF;
		break;

	case CUR_FP_VALUE_SELECTED:
		m_fkey_option[0] = FK_FP_EDIT_PROPERTIES;
		m_fkey_option[3] = FK_FP_MOVE_VALUE;
		break;

	case CUR_FP_POLY_CORNER_SELECTED:
		m_fkey_option[0] = FK_FP_SET_POSITION;
		m_fkey_option[3] = FK_FP_MOVE_CORNER;
		m_fkey_option[4] = FK_FP_DELETE_CORNER;
		m_fkey_option[6] = FK_FP_DELETE_POLYLINE;
		break;

	case CUR_FP_POLY_SIDE_SELECTED:
		m_fkey_option[0] = FK_FP_POLY_STRAIGHT;
		m_fkey_option[1] = FK_FP_POLY_ARC_CW;
		m_fkey_option[2] = FK_FP_POLY_ARC_CCW;
		{
			int style = m_fp.m_outline_poly[m_sel_id.I2()].SideStyle( m_sel_id.I3() );
			if( style == CPolyLine::STRAIGHT )
				m_fkey_option[3] = FK_FP_ADD_CORNER;
		}
		m_fkey_option[6] = FK_FP_DELETE_POLYLINE;
		break;

	case CUR_FP_TEXT_SELECTED:
		m_fkey_option[0] = FK_FP_EDIT_TEXT;
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
		if( m_drag_num_pads == 1 )
			m_fkey_option[2] = FK_FP_ROTATE_PAD;
		break;

	case CUR_FP_DRAG_REF:
		m_fkey_option[2] = FK_FP_ROTATE_REF;
		break;

	case CUR_FP_DRAG_VALUE:
		m_fkey_option[2] = FK_FP_ROTATE_VALUE;
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
	if (m_Doc->m_bLefthanded) 
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

	InvalidateLeftPane();
	Invalidate( FALSE );
}

// Draw bottom pane
//
void CFootprintView::DrawBottomPane()
{
	CDC * pDC = GetDC();
	CFont * old_font = pDC->SelectObject( &m_small_font );

	// get client rectangle
	GetClientRect( &m_client_r );

	// draw labels for function keys at bottom of client area
	// draw labels for function keys at bottom of client area.  CPT:  adjusted the positions of the gaps in lefthanded mode
	for (int ifn=0; ifn<9; ifn++)
	{
		int left = FKEY_OFFSET_X + ifn*FKEY_STEP;
		if (!m_Doc->m_bLefthanded)
			left += ifn/4 * FKEY_GAP;
		else
			left += (ifn+3)/4 * FKEY_GAP;
		CRect r( left, 
			m_client_r.bottom-FKEY_OFFSET_Y-FKEY_R_H,
			left + FKEY_R_W,
			m_client_r.bottom-FKEY_OFFSET_Y );
		pDC->Rectangle( &r );
		pDC->MoveTo( r.left+FKEY_SEP_W, r.top );
		pDC->LineTo( r.left+FKEY_SEP_W, r.top + FKEY_R_H/2 + 1 );
		pDC->MoveTo( r.left, r.top + FKEY_R_H/2 );
		pDC->LineTo( r.left+FKEY_SEP_W, r.top + FKEY_R_H/2 );
		r.top += 1;
		r.left += 2;
		char fkstr[3] = "F1";
		fkstr[1] = '1' + ifn;
		pDC->DrawText( fkstr, -1, &r, 0 );
		r.left += FKEY_SEP_W;
		CString str1 ((LPCSTR) m_fkey_rsrc[2*ifn]);
		CString str2 ((LPCSTR) m_fkey_rsrc[2*ifn+1]);
		pDC->DrawText( str1, -1, &r, 0 );
		r.top += FKEY_R_H/2 - 2;
		pDC->DrawText( str2, -1, &r, 0 );
	}
	// end CPT

	pDC->SelectObject( old_font );
	ReleaseDC( pDC );
}

// display selected item in status bar 
//
int CFootprintView::ShowSelectStatus()
{
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	CString str, s, x_str, y_str, w_str, type_str, style_str;
	int i, j, x, y;

	switch( m_cursor_mode )
	{
	case CUR_FP_NONE_SELECTED: 
		str.LoadStringA(IDS_NoSelection);
		break;

	case CUR_FP_PAD_SELECTED: 
		i = m_sel_id.I2();	// pin number (zero-based)
		x = m_fp.m_padstack[i].x_rel;
		y = m_fp.m_padstack[i].y_rel;
		::MakeCStringFromDimension( &x_str, x, m_units, TRUE, TRUE, TRUE, 3 );
		::MakeCStringFromDimension( &y_str, y, m_units, TRUE, TRUE, TRUE, 3 );
		s.LoadStringA(IDS_PinXY);
		str.Format( s, m_fp.GetPinNameByIndex( i ), x_str, y_str );
		break;

	case CUR_FP_DRAG_PAD:
		s.LoadStringA(IDS_MovingPin);
		str.Format( s, m_fp.GetPinNameByIndex( m_sel_id.I2() ) );
		break;

	case CUR_FP_POLY_CORNER_SELECTED: 
		i = m_sel_id.I2();
		x = m_fp.m_outline_poly[i].X(m_sel_id.I3());
		y = m_fp.m_outline_poly[i].Y(m_sel_id.I3());
		::MakeCStringFromDimension( &x_str, x, m_units, TRUE, TRUE, TRUE, 3 );
		::MakeCStringFromDimension( &y_str, y, m_units, TRUE, TRUE, TRUE, 3 );
		s.LoadStringA(IDS_PolylineCornerXY);
		str.Format( s, m_sel_id.I2()+1, m_sel_id.I3()+1, x_str, y_str );
		break;


	case CUR_FP_POLY_SIDE_SELECTED: 
		i = m_sel_id.I2(); 
		j = m_sel_id.I3();
		if( m_fp.m_outline_poly[i].SideStyle( j ) == CPolyLine::STRAIGHT )
			style_str.LoadStringA(IDS_straight);
		else if( m_fp.m_outline_poly[i].SideStyle( j ) == CPolyLine::ARC_CW )
			style_str.LoadStringA(IDS_ArcCw);
		else if( m_fp.m_outline_poly[i].SideStyle( j ) == CPolyLine::ARC_CCW )
			style_str.LoadStringA(IDS_ArcCcw);
		s.LoadStringA(IDS_PolylineSideStyle);
		str.Format( s, i+1, j+1, style_str );
		break;

	case CUR_FP_CENTROID_SELECTED:
		if( m_fp.m_centroid_type == CENTROID_DEFAULT )
			type_str.LoadStringA(IDS_DefaultPosition); 
		else
			type_str.LoadStringA(IDS_Defined);
		::MakeCStringFromDimension( &x_str, m_fp.m_centroid_x, m_units, TRUE, TRUE, TRUE, 3 );
		::MakeCStringFromDimension( &y_str, m_fp.m_centroid_y, m_units, TRUE, TRUE, TRUE, 3 );
		s.LoadStringA(IDS_CentroidXYAngle);
		str.Format( s, type_str, x_str, y_str, m_fp.m_centroid_angle );
		break;

	case CUR_FP_ADHESIVE_SELECTED:
		{
			int idot = m_sel_id.I2();
			int w = m_fp.m_glue[idot].w;
			if( w > 0 )
				::MakeCStringFromDimension( &w_str, m_fp.m_glue[idot].w, m_units, TRUE, TRUE, TRUE, 3 );
			else
			{
				w_str.LoadStringA(IDS_ProjectDefault);
				w = 15*NM_PER_MIL;
			}
			::MakeCStringFromDimension( &x_str, m_fp.m_glue[idot].x_rel, m_units, TRUE, TRUE, TRUE, 3 );
			::MakeCStringFromDimension( &y_str, m_fp.m_glue[idot].y_rel, m_units, TRUE, TRUE, TRUE, 3 );
			if( m_fp.m_glue[idot].type == GLUE_POS_DEFINED )
				s.LoadStringA(IDS_AdhesiveSpotWXY),
				str.Format( s, idot+1, w_str, x_str, y_str );
			else
				s.LoadStringA(IDS_AdhesiveSpotWAtCentroid),
				str.Format( s, idot+1, w_str );
		}
		break;

	case CUR_FP_DRAG_POLY_MOVE:
		s.LoadStringA(IDS_MovingCornerOfPolyline);
		str.Format( s, m_sel_id.I3()+1, m_sel_id.I2()+1 );
		break;


	}
	pMain->DrawStatus( 3, &str );
	return 0;
}

// display cursor coords in status bar 
//
int CFootprintView::ShowCursor()
{
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	CString str;
	CPoint p;
	p = m_last_cursor_point;
	if( m_units == MIL )
	{
		str.Format( "X: %d", m_last_cursor_point.x/PCBU_PER_MIL );
		pMain->DrawStatus( 1, &str );
		str.Format( "Y: %d", m_last_cursor_point.y/PCBU_PER_MIL );
		pMain->DrawStatus( 2, &str );
	}
	else
	{
		str.Format( "X: %8.3f", m_last_cursor_point.x/1000000.0 );
		pMain->DrawStatus( 1, &str );
		str.Format( "Y: %8.3f", m_last_cursor_point.y/1000000.0 );
		pMain->DrawStatus( 2, &str );
	}

	return 0;
}

// handle mouse scroll wheel
//
BOOL CFootprintView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
#define MIN_WHEEL_DELAY 1.0

	static struct _timeb current_time;
	static struct _timeb last_time;
	static int first_time = 1;
	double diff;

	// get current time
	_ftime( &current_time );
	
	if( first_time )
	{
		diff = 999.0;
		first_time = 0;
	}
	else
	{
		// get elapsed time since last wheel event
		diff = difftime( current_time.time, last_time.time );
		double diff_mil = (double)(current_time.millitm - last_time.millitm)*0.001;
		diff = diff + diff_mil;
	}

	if( diff > MIN_WHEEL_DELAY )
	{
		// first wheel movement in a while
		// center window on cursor then center cursor
		CPoint p;
		GetCursorPos( &p );		// cursor pos in screen coords
		p = ScreenToPCB( p );
		m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
		m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
		CRect screen_r;
		GetWindowRect( &screen_r );
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, m_org_x, m_org_y );
		Invalidate( FALSE );
		p = PCBToScreen( p );
		SetCursorPos( p.x, p.y - 4 );
	}
	else
	{
		// serial movements, zoom in or out
		if( zDelta > 0 && m_pcbu_per_pixel > NM_PER_MIL/1000 )
		{
			// wheel pushed, zoom in then center world coords and cursor
			CPoint p;
			GetCursorPos( &p );		// cursor pos in screen coords
			p = ScreenToPCB( p );	// convert to PCB coords
			m_pcbu_per_pixel = m_pcbu_per_pixel/ZOOM_RATIO;
			m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2.0;
			m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2.0;
			CRect screen_r;
			GetWindowRect( &screen_r );
			m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, m_org_x, m_org_y );
			Invalidate( FALSE );
			p = PCBToScreen( p );
			SetCursorPos( p.x, p.y - 4 );
		}
		else if( zDelta < 0 )
		{
			// wheel pulled, zoom out then center
			// first, make sure that window boundaries will be OK
			CPoint p;
			GetCursorPos( &p );		// cursor pos in screen coords
			p = ScreenToPCB( p );
			int org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel*ZOOM_RATIO)/2.0;
			int org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel*ZOOM_RATIO)/2.0;
			int max_x = org_x + (m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel*ZOOM_RATIO;
			int max_y = org_y + (m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel*ZOOM_RATIO;
			if( org_x > -PCB_BOUND && org_x < PCB_BOUND && max_x > -PCB_BOUND && max_x < PCB_BOUND
				&& org_y > -PCB_BOUND && org_y < PCB_BOUND && max_y > -PCB_BOUND && max_y < PCB_BOUND )
			{
				// OK, do it
				m_org_x = org_x;
				m_org_y = org_y;
				m_pcbu_per_pixel = m_pcbu_per_pixel*ZOOM_RATIO;
				CRect screen_r;
				GetWindowRect( &screen_r );
				m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, m_org_x, m_org_y );
				Invalidate( FALSE );
				p = PCBToScreen( p );
				SetCursorPos( p.x, p.y - 4 );
			}
		}
	}
	last_time = current_time;

	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

// cancel selection
//
void CFootprintView::CancelSelection()
{
	m_dlist->CancelHighLight();
	m_sel_id.Clear();
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
	// OK, pop-up context menu
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_FP_CONTEXT));
	CMenu* pPopup;
	int style;
	switch( m_cursor_mode )
	{
	case CUR_FP_NONE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_NONE);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_FP_PAD_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_PAD);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_FP_POLY_SIDE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_SIDE);
		ASSERT(pPopup != NULL);
		style = m_fp.m_outline_poly[m_sel_id.I2()].SideStyle( m_sel_id.I3() );
		if( style == CPolyLine::STRAIGHT )
		{
			int xi = m_fp.m_outline_poly[m_sel_id.I2()].X( m_sel_id.I3() );
			int yi = m_fp.m_outline_poly[m_sel_id.I2()].Y( m_sel_id.I3() );
			int xf, yf;
			if( m_sel_id.I3() != (m_fp.m_outline_poly[m_sel_id.I2()].NumCorners()-1) )
			{
				xf = m_fp.m_outline_poly[m_sel_id.I2()].X( m_sel_id.I3()+1 );
				yf = m_fp.m_outline_poly[m_sel_id.I2()].Y( m_sel_id.I3()+1 );
			}
			else
			{
				xf = m_fp.m_outline_poly[m_sel_id.I2()].X( 0 );
				yf = m_fp.m_outline_poly[m_sel_id.I2()].Y( 0 );
			}
			if( xi == xf || yi == yf )
			{
				pPopup->EnableMenuItem( ID_FP_CONVERTTOARC, MF_GRAYED );
				pPopup->EnableMenuItem( ID_FP_CONVERTTOARC32778, MF_GRAYED );
			}
			pPopup->EnableMenuItem( ID_FP_CONVERTTOSTRAIGHT, MF_GRAYED );
		}
		else if( style == CPolyLine::ARC_CW )
		{
			pPopup->EnableMenuItem( ID_FP_CONVERTTOARC, MF_GRAYED );
			pPopup->EnableMenuItem( ID_FP_INSERTCORNER, MF_GRAYED );
		}
		else if( style == CPolyLine::ARC_CCW )
		{
			pPopup->EnableMenuItem( ID_FP_CONVERTTOARC32778, MF_GRAYED );
			pPopup->EnableMenuItem( ID_FP_INSERTCORNER, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_FP_POLY_CORNER_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_CORNER);
		ASSERT(pPopup != NULL);
		{
			if( m_fp.m_outline_poly[m_sel_id.I2()].NumCorners() < 4 )
				pPopup->EnableMenuItem( ID_FP_DELETECORNER, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;


	case CUR_FP_REF_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_REF);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_FP_VALUE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_VALUE);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_FP_TEXT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_TEXT);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_FP_CENTROID_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_CENTROID);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_FP_ADHESIVE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_FP_ADHESIVE);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	}
}

// Delete pad
//
void CFootprintView::OnPadDelete( int i )
{
	PushUndo();
	CancelSelection();
	m_fp.m_padstack.RemoveAt( i );
	m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
	FootprintModified( TRUE );
}

// edit pad
//
void CFootprintView::OnPadEdit( int i )
{
	// save original position and angle of pad, in case we decide
	// to drag the pad, and then cancel dragging
	int m_orig_x = m_fp.m_padstack[i].x_rel;
	int m_orig_y = m_fp.m_padstack[i].y_rel;
	int m_orig_angle = m_fp.m_padstack[i].angle;
	// save undo info, since dialog may make lots of changes
	PushUndo();
	// now launch dialog
	CDlgAddPin dlg;
	dlg.InitDialog( &m_fp, CDlgAddPin::EDIT, i, m_units );
	m_dlist->CancelHighLight();
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// if OK, footprint has already been undrawn by dlg
		if( dlg.m_drag_flag )
		{
			// if dragging, move pad back to original position and start
			m_fp.m_padstack[i].x_rel = m_orig_x;
			m_fp.m_padstack[i].y_rel = m_orig_y;
			m_fp.m_padstack[i].angle = m_orig_angle;
			m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
			OnPadMove( i );
			return;
		}
		else
		{
			// not dragging, just redraw
			m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
			FootprintModified( TRUE );
		}
	}
	else
	{
		Undo();	// restore to original state
	}
	m_fp.HighlightPad( i );
	Invalidate( FALSE );
}

// move pad, don't push undo, this will be done when move completed
//
void CFootprintView::OnPadMove( int i, int num )
{
	// drag pad
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to pad
	CPoint p;
	p.x = m_fp.m_padstack[i].x_rel;
	p.y = m_fp.m_padstack[i].y_rel;
	CPoint cur_p = PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start dragging
	m_drag_num_pads = num;
	m_fp.StartDraggingPadRow( pDC, i, num );
	SetCursorMode( CUR_FP_DRAG_PAD );
	Invalidate( FALSE );
	ReleaseDC( pDC );
}


// move ref. designator text for part
//
void CFootprintView::OnRefMove()
{
	// move reference ID
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to ref
	CPoint p;
	p.x = m_fp.m_ref_xi;
	p.y = m_fp.m_ref_yi;
	CPoint cur_p = PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start dragging
	m_dragging_new_item = 0;
	m_fp.m_ref_text.StartDragging( pDC );
	SetCursorMode( CUR_FP_DRAG_REF );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// start adding board outline by dragging line for first side
//
void CFootprintView::OnAddBoardOutline()
{
}

void CFootprintView::OnPolylineDelete()
{
	PushUndo();
	m_fp.m_outline_poly.RemoveAt( m_sel_id.I2() );
	CancelSelection();
	m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
	FootprintModified( TRUE );
}

// move an outline polyline corner
//
void CFootprintView::OnPolylineCornerMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_fp.m_outline_poly[m_sel_id.I2()].StartDraggingToMoveCorner( pDC, m_sel_id.I3(), p.x, p.y );
	SetCursorMode( CUR_FP_DRAG_POLY_MOVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// edit an outline polyline corner
//
void CFootprintView::OnPolylineCornerEdit()
{
	DlgEditBoardCorner dlg;
	CString str ((LPCSTR) IDS_CornerPosition);
	int x = m_fp.m_outline_poly[m_sel_id.I2()].X(m_sel_id.I3());
	int y = m_fp.m_outline_poly[m_sel_id.I2()].Y(m_sel_id.I3());
	dlg.Init( &str, m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		PushUndo();
		m_fp.m_outline_poly[m_sel_id.I2()].MoveCorner( m_sel_id.I3(), 
			dlg.X(), dlg.Y() );
		CancelSelection();
		Invalidate( FALSE );
		FootprintModified( TRUE );
	}
}

// delete an outline polyline board corner
//
void CFootprintView::OnPolylineCornerDelete()
{
	PushUndo();
	CPolyLine * poly = &m_fp.m_outline_poly[m_sel_id.I2()];
	if( poly->Closed() && poly->NumCorners() < 4
		|| !poly->Closed() && poly->NumCorners() < 3 )
	{
		CString s ((LPCSTR) IDS_PolylineHasTooFewCorners);
		AfxMessageBox( s );
		return;
	}
	m_fp.m_outline_poly[m_sel_id.I2()].DeleteCorner( m_sel_id.I3() );
	CancelSelection();
	FootprintModified( TRUE );
	Invalidate( FALSE );
}

// insert a new corner in a side of a polyline
//
void CFootprintView::OnPolylineSideAddCorner()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_fp.m_outline_poly[m_sel_id.I2()].StartDraggingToInsertCorner( pDC, m_sel_id.I3(), p.x, p.y );
	SetCursorMode( CUR_FP_DRAG_POLY_INSERT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}



// detect state where nothing is selected or being dragged
//
BOOL CFootprintView::CurNone()
{
	return( m_cursor_mode == CUR_FP_NONE_SELECTED );
}

// detect any selected state
//
BOOL CFootprintView::CurSelected()
{	
	return( m_cursor_mode > CUR_FP_NONE_SELECTED && m_cursor_mode < CUR_FP_NUM_SELECTED_MODES );
}

// detect any dragging state
//
BOOL CFootprintView::CurDragging()
{
	return( m_cursor_mode > CUR_FP_NUM_SELECTED_MODES );	
}

// detect states using placement grid
//
BOOL CFootprintView::CurDraggingPlacement()
{
	return( m_cursor_mode == CUR_FP_DRAG_PAD
		|| m_cursor_mode == CUR_FP_DRAG_REF 
		|| m_cursor_mode == CUR_FP_DRAG_POLY_1 
		|| m_cursor_mode == CUR_FP_DRAG_POLY 
		|| m_cursor_mode == CUR_FP_DRAG_POLY_MOVE 
		|| m_cursor_mode == CUR_FP_DRAG_POLY_INSERT 
		);
}

// snap cursor if required and set m_last_cursor_point
//
void CFootprintView::SnapCursorPoint( CPoint wp )
{
	if( CurDragging() )
	{	
		int grid_spacing;
		grid_spacing = m_Doc->m_fp_part_grid_spacing;

		// snap angle if needed
		if( m_Doc->m_fp_snap_angle && (wp != m_snap_angle_ref) 
			&& ( m_cursor_mode == CUR_FP_DRAG_POLY_1 
			|| m_cursor_mode == CUR_FP_DRAG_POLY ) )
		{
			// snap to angle only if the starting point is on-grid
			double ddx = fmod( (double)(m_snap_angle_ref.x), grid_spacing );
			double ddy = fmod( (double)(m_snap_angle_ref.y), grid_spacing );
			if( fabs(ddx) < 0.5 && fabs(ddy) < 0.5 )
			{
				// starting point is on-grid, snap to angle
				// snap to n*45 degree angle
				const double pi = 3.14159265359;		
				double dx = wp.x - m_snap_angle_ref.x;
				double dy = wp.y - m_snap_angle_ref.y;
				double dist = sqrt( dx*dx + dy*dy );
				double dist45 = dist/sqrt(2.0);
				{
					int d;
					d = (int)(dist/grid_spacing+0.5);
					dist = d*grid_spacing;
					d = (int)(dist45/grid_spacing+0.5);
					dist45 = d*grid_spacing;
				}
				if( m_Doc->m_fp_snap_angle == 45 )
				{
					// snap angle = 45 degrees, divide circle into 8 octants
					double angle = atan2( dy, dx );
					if( angle < 0.0 )
						angle = 2.0*pi + angle;
					angle += pi/8.0;
					double d_quad = angle/(pi/4.0);
					int oct = d_quad;
					switch( oct )
					{
					case 0:
						wp.x = m_snap_angle_ref.x + dist;
						wp.y = m_snap_angle_ref.y;
						break;
					case 1:
						wp.x = m_snap_angle_ref.x + dist45;
						wp.y = m_snap_angle_ref.y + dist45;
						break;
					case 2:
						wp.x = m_snap_angle_ref.x;
						wp.y = m_snap_angle_ref.y + dist;
						break;
					case 3:
						wp.x = m_snap_angle_ref.x - dist45;
						wp.y = m_snap_angle_ref.y + dist45;
						break;
					case 4:
						wp.x = m_snap_angle_ref.x - dist;
						wp.y = m_snap_angle_ref.y;
						break;
					case 5:
						wp.x = m_snap_angle_ref.x - dist45;
						wp.y = m_snap_angle_ref.y - dist45;
						break;
					case 6:
						wp.x = m_snap_angle_ref.x;
						wp.y = m_snap_angle_ref.y - dist;
						break;
					case 7:
						wp.x = m_snap_angle_ref.x + dist45;
						wp.y = m_snap_angle_ref.y - dist45;
						break;
					case 8:
						wp.x = m_snap_angle_ref.x + dist;
						wp.y = m_snap_angle_ref.y;
						break;
					default:
						ASSERT(0);
						break;
					}
				}
				else
				{
					// snap angle is 90 degrees, divide into 4 quadrants
					double angle = atan2( dy, dx );
					if( angle < 0.0 )
						angle = 2.0*pi + angle;
					angle += pi/4.0;
					double d_quad = angle/(pi/2.0);
					int quad = d_quad;
					switch( quad )
					{
					case 0:
						wp.x = m_snap_angle_ref.x + dist;
						wp.y = m_snap_angle_ref.y;
						break;
					case 1:
						wp.x = m_snap_angle_ref.x;
						wp.y = m_snap_angle_ref.y + dist;
						break;
					case 2:
						wp.x = m_snap_angle_ref.x - dist;
						wp.y = m_snap_angle_ref.y;
						break;
					case 3:
						wp.x = m_snap_angle_ref.x;
						wp.y = m_snap_angle_ref.y - dist;
						break;
					case 4:
						wp.x = m_snap_angle_ref.x + dist;
						wp.y = m_snap_angle_ref.y;
						break;
					default:
						ASSERT(0);
						break;
					}
				}
			}
		}
		// snap to grid
		{
			// get position in integral units of grid_spacing
			if( wp.x > 0 )
				wp.x = (wp.x + grid_spacing/2)/grid_spacing;
			else
				wp.x = (wp.x - grid_spacing/2)/grid_spacing;
			if( wp.y > 0 )
				wp.y = (wp.y + grid_spacing/2)/grid_spacing;
			else
				wp.y = (wp.y - grid_spacing/2)/grid_spacing;
			// thrn multiply by grid spacing, adding or subracting 0.5 to prevent round-off
			// when using a fractional grid
			double test = wp.x * grid_spacing;
			if( test > 0.0 )
				test += 0.5;
			else
				test -= 0.5;
			wp.x = test;
			test = wp.y * grid_spacing;
			if( test > 0.0 )
				test += 0.5;
			else
				test -= 0.5;
			wp.y = test;
		}
	}
	if( CurDragging() )
	{
		// update drag operation
		if( wp != m_last_cursor_point )
		{
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			m_dlist->Drag( pDC, wp.x, wp.y );
			ReleaseDC( pDC );
		}
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
		m_Doc->m_fp_visual_grid_spacing = fabs( m_Doc->m_fp_visible_grid[lp] );
	else
		ASSERT(0);
	m_dlist->SetVisibleGrid( TRUE, m_Doc->m_fp_visual_grid_spacing );
	Invalidate( FALSE );
	SetFocus();
	return 0;
}

LONG CFootprintView::OnChangePlacementGrid( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
		m_Doc->m_fp_part_grid_spacing = fabs( m_Doc->m_fp_part_grid[lp] );
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
			m_Doc->m_fp_snap_angle = 45;
		else if( lp == 1 )
			m_Doc->m_fp_snap_angle = 90;
		else
			m_Doc->m_fp_snap_angle = 0;
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
	FootprintModified(TRUE);
	SetFocus();
	return 0;
}


void CFootprintView::OnRefProperties()
{

	CString str = "";
	CDlgFpText dlg;
	CString ref_str = "REF";
	dlg.Initialize( FALSE, TRUE, &ref_str, m_fp.m_ref->m_layer, m_units, 
		m_fp.m_ref_angle, m_fp.m_ref_size, m_fp.m_ref_w, 
		m_fp.m_ref_xi, m_fp.m_ref_yi );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CancelSelection();
		if( dlg.m_bDrag )
		{
			OnRefMove();
		}
		else
		{
			PushUndo();
			m_fp.Undraw();
			m_fp.m_ref->m_layer = dlg.m_layer;
			m_fp.m_ref_xi = dlg.m_x;
			m_fp.m_ref_yi = dlg.m_y;
			m_fp.m_ref_angle = dlg.m_angle;
			m_fp.m_ref_size = dlg.m_height;
			m_fp.m_ref_w = dlg.m_width;
			m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
			if( m_fp.m_ref_size )
			{
				m_fp.m_ref_text.Highlight();
				SetCursorMode( CUR_FP_REF_SELECTED );
			}
			else
				CancelSelection();
		}
		Invalidate( FALSE );		
	}
}

BOOL CFootprintView::OnEraseBkgnd(CDC* pDC)
{
	// Erase the left and bottom panes, the PCB area is always redrawn
	m_left_pane_invalid = TRUE;
	return FALSE;
}

void CFootprintView::OnPolylineSideConvertToStraightLine()
{
	PushUndo();
	m_dlist->CancelHighLight();
	m_fp.m_outline_poly[m_sel_id.I2()].SetSideStyle( m_sel_id.I3(), CPolyLine::STRAIGHT );
	m_fp.m_outline_poly[m_sel_id.I2()].HighlightSide( m_sel_id.I3() );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	Invalidate( FALSE );
}

void CFootprintView::OnPolylineSideConvertToArcCw()
{
	PushUndo();
	m_dlist->CancelHighLight();
	m_fp.m_outline_poly[m_sel_id.I2()].SetSideStyle( m_sel_id.I3(), CPolyLine::ARC_CW );
	m_fp.m_outline_poly[m_sel_id.I2()].HighlightSide( m_sel_id.I3() );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	Invalidate( FALSE );
}

void CFootprintView::OnPolylineSideConvertToArcCcw()
{
	PushUndo(); 
	m_dlist->CancelHighLight();
	m_fp.m_outline_poly[m_sel_id.I2()].SetSideStyle( m_sel_id.I3(), CPolyLine::ARC_CCW );
	m_fp.m_outline_poly[m_sel_id.I2()].HighlightSide( m_sel_id.I3() );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	Invalidate( FALSE );
}

void CFootprintView::OnAddPin()
{
	PushUndo();
	CDlgAddPin dlg;
	dlg.InitDialog( &m_fp, CDlgAddPin::ADD, m_fp.GetNumPins() + 1, m_units );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// if OK, footprint has been undrawn by dialog
		// and new pin added to footprint
		if( dlg.m_drag_flag )
		{
			// if dragging, move new pad(s) to cursor position
			int ip = dlg.m_pin_num;
			int num = dlg.m_num_pins;
			CPoint p;
			GetCursorPos( &p );		// cursor pos in screen coords
			p = ScreenToPCB( p );	// convert to PCB coords
			int dx = p.x - m_fp.m_padstack[ip].x_rel;
			int dy = p.y - m_fp.m_padstack[ip].y_rel;
			for( int i=ip; i<(ip+num); i++ )
			{
				m_fp.m_padstack[i].x_rel += dx;
				m_fp.m_padstack[i].y_rel += dy;
			}
			m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
			// now start dragging
			m_sel_id.SetT1( ID_PART );
			m_sel_id.SetT2( ID_SEL_PAD );
			m_sel_id.SetI2( ip );
			m_dragging_new_item = TRUE;
			OnPadMove( ip, num );
			return;
		}
		else
		{
			m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
		}
	}
	Invalidate( FALSE );
}

void CFootprintView::OnFootprintFileSaveAs()
{
	CString str_name = m_fp.m_name;

	CRect r;
	BOOL bOK = m_fp.GenerateSelectionRectangle( &r );
	if( !bOK )
	{
		CString s ((LPCSTR) IDS_UnableToSaveEmptyFootprint);
		AfxMessageBox( s, MB_OK );
		return;
	}
	m_fp.Draw( m_dlist, m_Doc->m_smfontutil );

	// now save it
	CDlgSaveFootprint dlg;
	dlg.Initialize( &str_name, &m_fp, m_units, "",
		&m_Doc->m_footprint_cache_map, &m_Doc->m_footlibfoldermap, m_Doc->m_dlg_log );	
	int ret = dlg.DoModal();
	if( ret == IDOK )	
	{
		FootprintModified( FALSE );
		ClearUndo();
		ClearRedo();
		FootprintNameChanged( &m_fp.m_name );
	}
}

void CFootprintView::OnAddPolyline()
{
	CDlgAddPoly dlg;
	dlg.Initialize( TRUE, -1, m_units, -1, TRUE, &m_fp.m_padstack );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// start new outline by dragging first point
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		CPoint p = m_last_mouse_point;
		m_dlist->CancelHighLight();
		m_sel_id.Set( ID_PART, -1, 
					ID_POLYLINE, -1, m_fp.m_outline_poly.GetSize(), 
					ID_SEL_CORNER, -1, 0 );
		m_polyline_closed_flag = dlg.GetClosedFlag();
		m_polyline_style = CPolyLine::STRAIGHT;
		m_polyline_width = dlg.GetWidth();
		if( dlg.GetLayerIndex() < 2 )
		{
			m_polyline_layer = LAY_FP_SILK_TOP + dlg.GetLayerIndex();
		}
		else
		{
			m_polyline_layer = LAY_FP_TOP_COPPER + dlg.GetLayerIndex() - 2;
		}
		m_dlist->StartDraggingArray( pDC, p.x, p.y, 0, LAY_FP_SELECTION );
		SetCursorMode( CUR_FP_ADD_POLY );
		ReleaseDC( pDC );
		Invalidate( FALSE );
	}
}

void CFootprintView::OnEditPolyline()
{
	CPolyLine * poly = &m_fp.m_outline_poly[m_sel_id.I2()];
	int layer_index = 0;
	if( poly->Layer() == LAY_FP_SILK_BOTTOM )
		layer_index = 1;
	CDlgAddPoly dlg;
	dlg.Initialize( FALSE, layer_index, m_units, poly->W(), poly->Closed(), &m_fp.m_padstack );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// change polyline properties
		PushUndo();
		int layer = LAY_FP_SILK_TOP;
		if( dlg.GetLayerIndex() == 1 )
			layer = LAY_FP_SILK_BOTTOM;
		poly->Undraw();
		poly->SetW( dlg.GetWidth() );
		poly->SetLayer( layer );
		poly->SetClosed( dlg.GetClosedFlag() );
		poly->Draw();
		FootprintModified( TRUE );
		CancelSelection();
		Invalidate( FALSE );
	}
}

void CFootprintView::OnFootprintFileImport()
{
	CDlgImportFootprint dlg;

	dlg.InitInstance( &m_Doc->m_footprint_cache_map, &m_Doc->m_footlibfoldermap, m_Doc->m_dlg_log );
	int ret = dlg.DoModal();

	// now import if OK
	if( ret == IDOK && dlg.m_footprint_name != "" && dlg.m_shape.m_name != "" )
	{
		m_fp.Copy( &dlg.m_shape );
		m_fp.Draw( m_dlist, m_Doc->m_smfontutil );

		// update window title and units
		SetWindowTitle( &m_fp.m_name );
		m_Doc->m_footprint_name_changed = TRUE;
		m_Doc->m_footprint_modified = FALSE;
		CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
		m_units = m_fp.m_units;
		frm->m_wndMyToolBar.SetUnits( m_units );
		ClearUndo();
		ClearRedo();
		OnViewEntireFootprint();
	}
	Invalidate( FALSE );
}

void CFootprintView::OnFootprintFileClose()
{
	// set units
	m_fp.m_units = m_units;

	// reset selection rectangle
	CRect br;
	br.left = br.bottom = INT_MAX;
	br.right = br.top = INT_MIN;
	for( int ip=0; ip<m_fp.GetNumPins(); ip++ )
	{
		CRect padr = m_fp.GetPadBounds( ip );
		br.left = min( br.left, padr.left ); 
		br.bottom = min( br.bottom, padr.bottom ); 
		br.right = max( br.right, padr.right ); 
		br.top = max( br.top, padr.top ); 
	}
	for( int ip=0; ip<m_fp.m_outline_poly.GetSize(); ip++ )
	{
		CRect polyr = m_fp.m_outline_poly[ip].GetBounds();
		br.left = min( br.left, polyr.left ); 
		br.bottom = min( br.bottom, polyr.bottom ); 
		br.right = max( br.right, polyr.right ); 
		br.top = max( br.top, polyr.top ); 
	}
	m_fp.m_sel_xi = br.left - 10*NM_PER_MIL;
	m_fp.m_sel_xf = br.right + 10*NM_PER_MIL;
	m_fp.m_sel_yi = br.bottom - 10*NM_PER_MIL;
	m_fp.m_sel_yf = br.top + 10*NM_PER_MIL;
	m_fp.Draw( m_dlist, m_Doc->m_smfontutil );

	if( m_Doc->m_footprint_modified )
	{
		CString s ((LPCSTR) IDS_SaveFootprintBeforeExiting);
		int ret = AfxMessageBox( s, MB_YESNOCANCEL );
		m_Doc->m_file_close_ret = ret;
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
	if( m_Doc->m_footprint_modified ) 
	{
		CString s ((LPCSTR) IDS_SaveFootprint);
		int ret = AfxMessageBox( s, MB_YESNOCANCEL );
		if( ret == IDCANCEL )
			return;
		else if( ret == IDYES )
			OnFootprintFileSaveAs();
	}
	m_dlist->CancelHighLight();
	m_fp.Clear();
	m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
	SetWindowTitle( &m_fp.m_name );
	FootprintModified( FALSE, TRUE );
	ClearUndo();
	ClearRedo();
	Invalidate( FALSE );
}

void CFootprintView::FootprintModified( BOOL flag, BOOL force, BOOL clear_redo )
{
	// if requested, clear redo stack (this is the default)
	if( clear_redo )
		ClearRedo();

	// see if we need to do anything
	if( flag == m_Doc->m_footprint_modified && !force )
		return;	// no!

	// OK, set state and window title
	m_Doc->m_footprint_modified = flag;
	if( flag == TRUE )
	{
		// add "*" to end of window title
		if( m_Doc->m_fp_window_title.Right(1) != "*" )
			m_Doc->m_fp_window_title = m_Doc->m_fp_window_title + "*";
	}
	else if( flag == FALSE )
	{
		// remove "*" from end of window title
		if( m_Doc->m_fp_window_title.Right(1) == "*" )
			m_Doc->m_fp_window_title = m_Doc->m_fp_window_title.Left( m_Doc->m_fp_window_title.GetLength()-1 );
	}
	CMainFrame * pMain = (CMainFrame*)AfxGetMainWnd();
	pMain->SetWindowText( m_Doc->m_fp_window_title );
}

void CFootprintView::FootprintNameChanged( CString * str )
{
	m_Doc->m_footprint_name_changed = TRUE;
	SetWindowTitle( &m_fp.m_name );
}


void CFootprintView::OnViewEntireFootprint()
{
	CRect r;
	r = m_fp.GetBounds();

	int max_x = (3*r.right - r.left)/2;
	int min_x = (3*r.left - r.right)/2;
	int max_y = (3*r.top - r.bottom)/2;
	int min_y = (3*r.bottom - r.top)/2;
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
	int n = undo_stack.GetSize();
	for( int i=0; i<n; i++ )
		delete undo_stack[i];
	undo_stack.RemoveAll();
	EnableUndo( FALSE );
}

void CFootprintView::ClearRedo()
{
	int n = redo_stack.GetSize();
	for( int i=0; i<n; i++ )
		delete redo_stack[i];
	redo_stack.RemoveAll();
	EnableRedo( FALSE );
}

void CFootprintView::PushUndo()
{
	if( undo_stack.GetSize() > 100 )
	{
		delete undo_stack[0];
		undo_stack.RemoveAt( 0 );
	}
	CEditShape * sh = new CEditShape;
	sh->Copy( &m_fp );
	undo_stack.Add( sh );
	EnableUndo( TRUE );
}

void CFootprintView::PushRedo()
{
	if( redo_stack.GetSize() > 100 )
	{
		delete redo_stack[0];
		redo_stack.RemoveAt( 0 );
	}
	CEditShape * sh = new CEditShape;
	sh->Copy( &m_fp );
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
		m_fp.Clear();
		CEditShape * sh = undo_stack[n-1];
		m_fp.Copy( sh );
		m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
		delete sh;
		undo_stack.SetSize( n-1 );
	}
	EnableUndo( undo_stack.GetSize() );
	FootprintModified( TRUE, 0, 0 );	// don't clear redo stack
	Invalidate( FALSE );
}

void CFootprintView::Redo()
{
	PushUndo();
	int n = redo_stack.GetSize();
	if( n )
	{
		CancelSelection();
		m_fp.Clear();
		CEditShape * sh = redo_stack[n-1];
		m_fp.Copy( sh );
		m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
		delete sh;
		redo_stack.SetSize( n-1 );
	}
	EnableRedo( redo_stack.GetSize() );
	FootprintModified( TRUE, 0, 0 ); 	// don't clear redo stack
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

void CFootprintView::OnFpMove()
{
	OnPadMove( m_sel_id.I2(), 1 );
}

void CFootprintView::OnFpEditproperties()
{
	OnPadEdit( m_sel_id.I2() );
}

void CFootprintView::OnFpDelete()
{
	OnPadDelete( m_sel_id.I2() );
}

void CFootprintView::OnFpToolsFootprintwizard()
{
	// ask about saving
	if( m_Doc->m_footprint_modified )
	{
		CString s ((LPCSTR) IDS_SaveFootprintBeforeLaunchingWizard);
		int ret = AfxMessageBox( s, MB_YESNOCANCEL );
		m_Doc->m_file_close_ret = ret;
		if( ret == IDCANCEL )
			return;
		else if( ret == IDYES )
			OnFootprintFileSaveAs();
	}

	// OK, launch wizard
	CDlgWizQuad dlg;
	dlg.Initialize( &m_Doc->m_footprint_cache_map, &m_Doc->m_footlibfoldermap, 
		FALSE, m_Doc->m_dlg_log );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// import wizard-created footprint
		m_fp.Clear();
		m_fp.Copy( &dlg.m_footprint );
		m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
		SetWindowTitle( &m_fp.m_name );
		FootprintModified( TRUE, TRUE );
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
	m_Doc->m_fp_window_title = s + *str;
	CMainFrame * pMain = (CMainFrame*)AfxGetMainWnd();
	pMain->SetWindowText( m_Doc->m_fp_window_title );
}

void CFootprintView::OnToolsFootprintLibraryManager()
{
	CDlgLibraryManager dlg;
	dlg.Initialize( &m_Doc->m_footlibfoldermap, m_Doc->m_dlg_log );
	dlg.DoModal();
}

void CFootprintView::OnAddText()
{
	CString str = "";
	CDlgFpText dlg;
	dlg.Initialize( TRUE, FALSE, NULL, LAY_FP_SILK_TOP, m_units, 0, 0, 0, 0, 0 );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		int x = dlg.m_x;
		int y = dlg.m_y;
		int angle = dlg.m_angle;
		int font_size = dlg.m_height;
		int stroke_width = dlg.m_width;
		int layer = dlg.m_layer;
		BOOL mirror = (layer == LAY_FP_SILK_BOTTOM || layer == LAY_FP_BOTTOM_COPPER);
		CString str = dlg.m_str;

		// get cursor position and convert to PCB coords
		PushUndo();
		CPoint p;
		GetCursorPos( &p );		// cursor pos in screen coords
		p = ScreenToPCB( p );	// convert to PCB coords
		// set pDC to PCB coords
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		if( dlg.m_bDrag )
		{
			m_sel_text = m_fp.m_tl->AddText( p.x, p.y, angle, mirror, FALSE, 
				layer, font_size, stroke_width, &str );
			m_dragging_new_item = 1;
			m_fp.m_tl->StartDraggingText( pDC, m_sel_text );
			SetCursorMode( CUR_FP_DRAG_TEXT );
		}
		else
		{
			m_sel_text = m_fp.m_tl->AddText( x, y, angle, mirror, FALSE, 
				layer, font_size, stroke_width, &str ); 
			m_fp.m_tl->HighlightText( m_sel_text );
		}
	}
}

void CFootprintView::OnFpTextEdit()
{
	// create dialog and pass parameters
	CDlgFpText dlg;
	CString test_str = m_sel_text->m_str;
	dlg.Initialize( FALSE, FALSE, &test_str, m_sel_text->m_layer, m_units,
		m_sel_text->m_angle, m_sel_text->m_font_size, 
		m_sel_text->m_stroke_width, m_sel_text->m_x, m_sel_text->m_y );
	int ret = dlg.DoModal();
	if( ret == IDCANCEL )
		return;

	// replace old text with new one
	PushUndo();
	int x = dlg.m_x;
	int y = dlg.m_y;
	int angle = dlg.m_angle;
	int font_size = dlg.m_height;
	int stroke_width = dlg.m_width;
	int layer = dlg.m_layer;
	BOOL mirror = (layer == LAY_FP_SILK_BOTTOM || layer == LAY_FP_BOTTOM_COPPER);
	CString str = dlg.m_str;
	m_dlist->CancelHighLight();
	m_fp.m_tl->RemoveText( m_sel_text );
	CText * new_text = m_fp.m_tl->AddText( x, y, angle, mirror, FALSE,
		layer, font_size, stroke_width, &str );
	m_sel_text = new_text;
	m_fp.m_tl->HighlightText( m_sel_text );

	// start dragging if requested in dialog
	if( dlg.m_bDrag )
		OnFpTextMove();
	else
		Invalidate( FALSE );
	FootprintModified( TRUE );
}

// move text
void CFootprintView::OnFpTextMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to ref
	CPoint p;
	p.x = m_sel_text->m_x;
	p.y = m_sel_text->m_y;
	CPoint cur_p = PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start dragging
	m_dragging_new_item = 0;
	m_fp.m_tl->StartDraggingText( pDC, m_sel_text );
	SetCursorMode( CUR_FP_DRAG_TEXT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFootprintView::OnFpTextDelete()
{
	PushUndo(); 
	m_fp.m_tl->RemoveText( m_sel_text );
	m_dlist->CancelHighLight();
	SetCursorMode( CUR_FP_NONE_SELECTED );
	FootprintModified( TRUE );
	Invalidate( FALSE );
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
	dlg.Initialize( m_Doc->m_units );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		if( dlg.m_drag )
		{
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			m_dlist->CancelHighLight();
			SetCursorMode( CUR_FP_MOVE_ORIGIN );
			m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x, 
				m_last_cursor_point.y, 0, LAY_SELECTION, 2 );
			Invalidate( FALSE );
			ReleaseDC( pDC );
		}
		else
		{
			PushUndo();
			CancelSelection();
			MoveOrigin( dlg.m_x, dlg.m_y );
			OnViewEntireFootprint();
		}
	}
}

void CFootprintView::MoveOrigin( int x, int y )
{
	m_fp.Undraw(); 
	m_fp.m_sel_xi -= x;
	m_fp.m_sel_xf -= x;
	m_fp.m_sel_yi -= y;
	m_fp.m_sel_yf -= y;
	m_fp.m_ref_xi -= x;
	m_fp.m_ref_yi -= y;
	m_fp.m_value_xi -= x;
	m_fp.m_value_yi -= y;
	m_fp.m_centroid_x -= x; 
	m_fp.m_centroid_y -= y;
	for( int ip=0; ip<m_fp.m_padstack.GetSize(); ip++ )
	{
		padstack * ps = &m_fp.m_padstack[ip];
		ps->x_rel -= x;
		ps->y_rel -= y;
	}
	for( int ip=0; ip<m_fp.m_outline_poly.GetSize(); ip++ ) 
	{
		CPolyLine * poly = &m_fp.m_outline_poly[ip];
		poly->MoveOrigin( -x, -y );
	}
	m_fp.m_tl->MoveOrigin( -x, -y );
	m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
	FootprintModified( TRUE );
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

void CFootprintView::OnCentroidEdit()
{
	CDlgCentroid dlg;
	dlg.Initialize( m_fp.m_centroid_type, m_units, 
		m_fp.m_centroid_x, m_fp.m_centroid_y, m_fp.m_centroid_angle );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		PushUndo();
		m_dlist->CancelHighLight();
		m_fp.Undraw();
		m_fp.m_centroid_type = dlg.m_type; 
		if( m_fp.m_centroid_type == CENTROID_DEFAULT )
		{
			CPoint c = m_fp.GetDefaultCentroid();
			m_fp.m_centroid_x = c.x; 
			m_fp.m_centroid_y = c.y;
		}
		else
		{
			m_fp.m_centroid_x = dlg.m_x; 
			m_fp.m_centroid_y = dlg.m_y;
		}
		m_fp.m_centroid_angle = dlg.m_angle;
		m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
		m_fp.SelectCentroid();
		FootprintModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFootprintView::OnCentroidMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to centroid
	CPoint p;
	p.x = m_fp.m_centroid_x;
	p.y = m_fp.m_centroid_y;
	CPoint cur_p = PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start dragging
	m_dragging_new_item = 0;
	m_fp.StartDraggingCentroid( pDC );
	SetCursorMode( CUR_FP_DRAG_CENTROID );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFootprintView::OnAddSlot()
{
	CDlgSlot dlg;
	dlg.Initialize( m_units, 0, 0, 0, 0, 0 );
	dlg.DoModal();
}

void CFootprintView::OnAddValueText()
{
	CancelSelection();
	CString str = "";
	CDlgFpText dlg;
	CString value_str = "VALUE";
	dlg.Initialize( TRUE, TRUE, &value_str, 0, m_units, 0, 0, 0, 0, 0 );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_fp.Undraw();
		m_fp.m_value_xi = dlg.m_x;
		m_fp.m_value_yi = dlg.m_y;
		m_fp.m_value_angle = dlg.m_angle;
		m_fp.m_value_size = dlg.m_height;
		m_fp.m_value_w = dlg.m_width;
		m_fp.m_value->m_layer = dlg.m_layer;
		m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
		if( dlg.m_bDrag )
		{
			m_dragging_new_item = TRUE;
			OnValueMove();
		}
		Invalidate( FALSE );		
	}
}

void CFootprintView::OnAddHole()
{
	CDlgHole dlg;
	dlg.Initialize( m_units, 0, 0, 0 );
	dlg.DoModal();
}

void CFootprintView::OnValueEdit()
{
	CString str = "";
	CDlgFpText dlg;
	CString value_str ((LPCSTR) IDS_Value);
	dlg.Initialize( FALSE, TRUE, &value_str, m_fp.m_value->m_layer, m_units, 
		m_fp.m_value_angle, m_fp.m_value_size, m_fp.m_value_w, 
		m_fp.m_value_xi, m_fp.m_value_yi );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CancelSelection();
		if( dlg.m_bDrag )
		{
			OnValueMove();
		}
		else
		{
			PushUndo();
			m_fp.Undraw();
			m_fp.m_value->m_layer = dlg.m_layer;
			m_fp.m_value_xi = dlg.m_x;
			m_fp.m_value_yi = dlg.m_y;
			m_fp.m_value_angle = dlg.m_angle;
			m_fp.m_value_size = dlg.m_height;
			m_fp.m_value_w = dlg.m_width;
			m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
			if( m_fp.m_value_size )
			{
				m_fp.m_value_text.Highlight();
				SetCursorMode( CUR_FP_VALUE_SELECTED );
			}
			else
				CancelSelection();
		}
		Invalidate( FALSE );		
	}
}

void CFootprintView::OnValueMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to ref
	CPoint p;
	p.x = m_fp.m_value_xi;
	p.y = m_fp.m_value_yi;
	CPoint cur_p = PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start dragging
	CancelSelection();
	m_dragging_new_item = 0;
	m_fp.m_value_text.StartDragging( pDC );
	SetCursorMode( CUR_FP_DRAG_VALUE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFootprintView::OnAddAdhesive()
{
	CDlgGlue dlg;
	dlg.Initialize( GLUE_POS_CENTROID, m_units, 0, 0, 0 );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		PushUndo();		// save state before creation of dot
		m_fp.Undraw();
		int i_spot = m_fp.m_glue.GetSize();
		m_fp.m_glue.SetSize( i_spot + 1 );
		m_fp.m_glue[i_spot].w = dlg.m_w;
		m_fp.m_glue[i_spot].type = dlg.m_pos_type;
		if( dlg.m_pos_type == GLUE_POS_DEFINED )
		{
			m_fp.m_glue[i_spot].x_rel = dlg.m_x;
			m_fp.m_glue[i_spot].y_rel = dlg.m_y;
		}
		else
		{
			m_fp.m_glue[i_spot].x_rel = m_fp.m_centroid_x;
			m_fp.m_glue[i_spot].y_rel = m_fp.m_centroid_y;
		}
		m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
		m_sel_id.Set( ID_GLUE, -1, ID_SEL_SPOT, -1, i_spot );
		if( dlg.m_bDrag )
		{
			m_dragging_new_item = TRUE;
			OnAdhesiveDrag();
		}
		else
			FootprintModified( TRUE );
		Invalidate( FALSE );		
	}
}


void CFootprintView::OnAdhesiveEdit()
{
	CDlgGlue dlg;
	int idot = m_sel_id.I2();
	glue * g = &m_fp.m_glue[idot];
	dlg.Initialize( g->type, m_units, g->w, g->x_rel, g->y_rel );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		PushUndo();
		g->w = dlg.m_w;		// 0 to use project default
		g->type = dlg.m_pos_type;	// position flag 
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
		if ( dlg.m_bDrag )
		{
			// start dragging
			m_dragging_new_item = FALSE;
			OnAdhesiveDrag();
		}
		else
		{
			m_dlist->CancelHighLight();
			m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
			m_fp.SelectAdhesive( m_sel_id.I2() );
			FootprintModified( TRUE );
			Invalidate( FALSE );
		}
	}
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
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to dot
	int idot = m_sel_id.I2();
	CPoint p;
	p.x = m_fp.m_glue[idot].x_rel;
	p.y = m_fp.m_glue[idot].y_rel;
	CPoint cur_p = PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start dragging
	m_fp.StartDraggingAdhesive( pDC, idot );
	SetCursorMode( CUR_FP_DRAG_ADHESIVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFootprintView::OnAdhesiveDelete()
{
	PushUndo();
	m_fp.Undraw();
	m_fp.m_glue.RemoveAt( m_sel_id.I2() );
	m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
	CancelSelection();
	FootprintModified( TRUE );
	Invalidate( FALSE );
}

void CFootprintView::OnCentroidRotateAxis()
{
	PushUndo();
	m_fp.Undraw();
	m_fp.m_centroid_angle += 90;
	if( m_fp.m_centroid_angle > 270 )
		m_fp.m_centroid_angle = 0;
	m_fp.Draw( m_dlist, m_Doc->m_smfontutil );
	FootprintModified( TRUE );
	Invalidate( FALSE );
}
