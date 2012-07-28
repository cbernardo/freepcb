// CommonView.cpp:  New common base class for CFreePcbView and CFootprintView

#include "stdafx.h"
#include "DlgFpText.h"
#include "DlgAssignNet.h"
#include "DlgSetSegmentWidth.h"
#include "DlgEditBoardCorner.h"
#include "DlgAddArea.h"
#include "DlgFpRefText.h"
#include "MyToolBar.h"
#include <Mmsystem.h>
#include <sys/timeb.h>
#include <time.h>
#include <math.h>
#include "CommonView.h" 
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
#include "DlgGridVals.h"
#include "DlgMyMessageBox.h"

#define ZOOM_RATIO 1.4

// constants for function key menu
#define FKEY_OFFSET_X 4
#define FKEY_OFFSET_Y 4
#define	FKEY_R_W m_fkey_w				// CPT:	now a variable, controlled by a string resource (for the sake of foreign language translators)
#define FKEY_R_H 30
#define FKEY_STEP (FKEY_R_W+5)
#define FKEY_GAP 20
#define FKEY_SEP_W 16
// constants for layer list
#define VSTEP 14

extern CFreePcbApp theApp;
BOOL g_bShow_Ratline_Warning = TRUE;


CCommonView::CCommonView()
{
	// GetDocument() is not available at this point
	m_small_font.CreateFont( 14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, 
		DEFAULT_PITCH | FF_DONTCARE, "Arial" );
	m_Doc = NULL;
	m_dlist = NULL; 
	m_last_mouse_point.x = 0;
	m_last_mouse_point.y = 0;
	m_last_cursor_point.x = 0;
	m_last_cursor_point.y = 0;
	m_lastKeyWasArrow = m_lastKeyWasGroupRotate = FALSE;
	m_totalArrowMoveX = 0;
	m_totalArrowMoveY = 0;

	// CPT: left pane width customizable by changing resource string 
	CString s ((LPCSTR) IDS_LeftPaneWidth);
	m_left_pane_w = atoi(s);
	if (m_left_pane_w<=0) m_left_pane_w = 125;
	// CPT: Likewise f-key box width 
	s.LoadStringA(IDS_FKeyWidth);
	m_fkey_w = atoi(s);
	if (m_fkey_w<=0) m_fkey_w = 70;
	// end CPT

	m_bottom_pane_h = 40;	// the bottom pane on screen is this high (pixels)
	m_memDC_created = FALSE;
	m_dragging_new_item = FALSE;
	m_units = MIL;
	m_cursor_mode = -1;												// CPT.  Ensures that SetFKText() will get called by InitInstance(),  no matter what.
	m_sel_offset = -1;
	}

#ifdef _DEBUG
void CCommonView::AssertValid() const
{
	CView::AssertValid();
}

void CCommonView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CFreePcbDoc* CCommonView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFreePcbDoc)));
	return (CFreePcbDoc*)m_pDocument;
}
#endif //_DEBUG

int CCommonView::SetDCToWorldCoords( CDC * pDC )
{
	m_dlist->SetDCToWorldCoords( pDC, &m_memDC, &m_memDC2, m_org_x, m_org_y );
	return 0;
}


void CCommonView::OnSize(UINT nType, int cx, int cy)
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
		m_old_bitmap = (HBITMAP)::SelectObject( m_memDC.m_hDC, m_bitmap.m_hObject );		
		m_bitmap_rect = m_client_r;
		// CPT experimental
		m_memDC2.CreateCompatibleDC( pDC );
		m_bitmap2.CreateCompatibleBitmap( pDC, m_client_r.right, m_client_r.bottom );
		m_old_bitmap2 = (HBITMAP)::SelectObject( m_memDC2.m_hDC, m_bitmap2.m_hObject );
		// end CPT
		ReleaseDC( pDC );
	}
	else if( m_memDC_created && (m_bitmap_rect != m_client_r) )
	{
		CDC * pDC = GetDC();
		::SelectObject(m_memDC.m_hDC, m_old_bitmap );
		m_bitmap.DeleteObject();
		m_bitmap.CreateCompatibleBitmap( pDC, m_client_r.right, m_client_r.bottom );
		m_old_bitmap = (HBITMAP)::SelectObject( m_memDC.m_hDC, m_bitmap.m_hObject );		
		m_bitmap_rect = m_client_r;
		// CPT experimental
		::SelectObject(m_memDC2.m_hDC, m_old_bitmap2 );
		m_bitmap2.DeleteObject();
		m_bitmap2.CreateCompatibleBitmap( pDC, m_client_r.right, m_client_r.bottom );
		m_old_bitmap2 = (HBITMAP)::SelectObject( m_memDC2.m_hDC, m_bitmap2.m_hObject );		
		// end CPT
		ReleaseDC( pDC );
	}
}

BOOL CCommonView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	return CView::PreCreateWindow(cs);
}

void CCommonView::BaseInit() 
{
	// All CPT.
	// Initialization that occurs after GetDocument() is ready to run
	m_Doc = (CFreePcbDoc*) GetDocument();
	SetDList();
	if( m_Doc == NULL || m_dlist == NULL )
		ASSERT(0);

	// Set default values
	m_dragging_new_item = 0;
	m_pcbu_per_pixel = 5.0*PCBU_PER_MIL;	// 5 mils per pixel
	m_org_x = -100.0*PCBU_PER_MIL;			// lower left corner of window
	m_org_y = -100.0*PCBU_PER_MIL;
	m_Doc->m_fp_snap_angle = 45;
	m_left_pane_invalid = TRUE;
	CancelSelection();
	m_sel_mask = 0xffff;
	SetSelMaskArray( m_sel_mask );

	CRect screen_r;
	GetWindowRect( &screen_r );
	m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, 
		m_pcbu_per_pixel, m_org_x, m_org_y );
	for(int i=0; i<GetNLayers(); i++ )
		m_dlist->SetLayerRGB( i, GetLayerRGB(i) ), 
		m_dlist->SetLayerVisible( i, 1 );
	Invalidate( FALSE );
}

// Set array of selection mask ids
//
void CCommonView::SetSelMaskArray( int mask )
{
	for( int i=0; i<NUM_SEL_MASKS; i++ )
	{
		if( mask & (1<<i) )
		{
			m_mask_id[i] = m_mask_default_id[i];
		}
		else
		{
			m_mask_id[i].SetT1( ID_NONE );	// guaranteed not to exist
		}
	}
}


// DISPLAY-RELATED

int CCommonView::ShowCursor()
{
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	CString str;

	// AMW2 enforce limits of +/- 32 inches
	if( m_last_cursor_point.x > 32000*PCBU_PER_MIL )
		m_last_cursor_point.x = 32000*PCBU_PER_MIL;
	else if( m_last_cursor_point.x < -32000*PCBU_PER_MIL )
		m_last_cursor_point.x = -32000*PCBU_PER_MIL;
	if( m_last_cursor_point.y > 32000*PCBU_PER_MIL )
		m_last_cursor_point.y = 32000*PCBU_PER_MIL;
	else if( m_last_cursor_point.y < -32000*PCBU_PER_MIL )
		m_last_cursor_point.y = -32000*PCBU_PER_MIL;

	if( m_units == MIL )  
	{
		str.Format( "X: %8.1f", (double)m_last_cursor_point.x/PCBU_PER_MIL );
		pMain->DrawStatus( 1, &str );
		str.Format( "Y: %8.1f", (double)m_last_cursor_point.y/PCBU_PER_MIL );
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

void CCommonView::ShowRelativeDistance( int dx, int dy )
{
	CString str;
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	double d = sqrt( (double)dx*dx + (double)dy*dy );  
	if( m_units == MIL )
		str.Format( "dx = %.1f, dy = %.1f, d = %.2f", 
		(double)dx/NM_PER_MIL, (double)dy/NM_PER_MIL, d/NM_PER_MIL );
	else
		str.Format( "dx = %.3f, dy = %.3f, d = %.3f", dx/1000000.0, dy/1000000.0, d/1000000.0 );
	pMain->DrawStatus( 3, &str );
}

void CCommonView::ShowRelativeDistance( int x, int y, int dx, int dy )
{
	CString str;
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	double d = sqrt( (double)dx*dx + (double)dy*dy );  
	if( m_units == MIL )
		str.Format( "x = %.1f, y = %.1f, dx = %.1f, dy = %.1f, d = %.2f",
		(double)x/NM_PER_MIL, (double)y/NM_PER_MIL,
		(double)dx/NM_PER_MIL, (double)dy/NM_PER_MIL, d/NM_PER_MIL );
	else
		str.Format( "x = %.3f, y = %.3f, dx = %.3f, dy = %.3f, d = %.3f", 
		x/1000000.0, y/1000000.0,
		dx/1000000.0, dy/1000000.0, d/1000000.0 );
	pMain->DrawStatus( 3, &str );
}

void CCommonView::DrawLeftPane(CDC *pDC) {
	#define VSTEP 14
	CRect r = m_client_r;
	int y_off = 10;
	int x_off = 10;
	if( m_left_pane_invalid )
	{
		m_left_pane_invalid = FALSE;
		// erase previous contents
		CBrush brush( RGB(255, 255, 255) );
		CPen pen( PS_SOLID, 1, RGB(255, 255, 255) );
		CBrush * old_brush = pDC->SelectObject( &brush );
		CPen * old_pen = pDC->SelectObject( &pen );
		// erase left pane
		r.right = m_left_pane_w;
		r.bottom -= m_bottom_pane_h;
		pDC->Rectangle( &r );
		pDC->SelectObject( old_brush );
		pDC->SelectObject( old_pen );
		CFont * old_font = pDC->SelectObject( &m_small_font );
		// CPT: modified so that "Selection" is no longer one of the displayed layers
		for( int i=1; i<GetNLayers(); i++ )
		{
			// i = position index
			r.SetRect( x_off, (i-1)*VSTEP+y_off, x_off+12, (i-1)*VSTEP+12+y_off );
			// il = true layer num since copper layers are displayed out of order
			int il = GetLayerNum(i);
			CBrush brush( RGB(GetLayerRGB(il,0), GetLayerRGB(il,1), GetLayerRGB(il,2)));
			if( GetLayerVis(il) )
			{
				// if layer is visible, draw colored rectangle
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
			// CPT
			CString label;
			GetLayerLabel(i, label);
			pDC->DrawText( label, -1, &r, 0 );
			CRect ar = r;
			ar.left = 2;
			ar.right = 8;
			ar.bottom -= 5;
			if( il == m_active_layer )
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
		r.left = x_off;
		r.bottom += VSTEP*2;
		r.top += VSTEP*2;
		CString s ((LPCSTR) IDS_SelectionMask);
		pDC->DrawText( s, -1, &r, DT_TOP );
		y_off = r.bottom;
		for( int i=0; i<GetNMasks(); i++ )
		{
			// i = position index
			r.left = x_off;
			r.right = x_off+12;
			r.top = i*VSTEP+y_off;
			r.bottom = i*VSTEP+12+y_off;
			CBrush green_brush( RGB(0, 255, 0) );
			CBrush red_brush( RGB(255, 0, 0) );
			if( m_sel_mask & (1<<i) )
			{
				// if mask is selected is visible, draw green rectangle
				CBrush * old_brush = pDC->SelectObject( &green_brush );
				pDC->Rectangle( &r );
				pDC->SelectObject( old_brush );
			}
			else
			{
				// if mask not selected, draw red
				CBrush * old_brush = pDC->SelectObject( &red_brush );
				pDC->Rectangle( &r );
				pDC->SelectObject( old_brush );
			}
			r.left += 20;
			r.right += 120;
			r.bottom += 5;
			// CPT
			int id = GetMaskNamesID();
			CString label, s ((LPCSTR) (id+i));
			label.Format("%s. A%c", s, i<9? '1'+i: i==9? '0': '-');
			pDC->DrawText( label, -1, &r, DT_TOP );
		}
		// CPT
		r.left = x_off;
		r.bottom += VSTEP*2;
		r.top += VSTEP*2;
		int id = GetLeftPaneKeyID();
		for (int i=0; i<9; i++) 
		{
			s.LoadStringA(id+i);
			pDC->DrawText( s, -1, &r, DT_TOP );
			int step = i%3==2? VSTEP*3/2: VSTEP;
			r.bottom += step;
			r.top += step;
		}
	}
}

void CCommonView::DrawBottomPane(CDC *pDC)
{
	CFont * old_font = pDC->SelectObject( &m_small_font );

	// get client rectangle
	GetClientRect( &m_client_r );

	// Erase bottom pane (in case left pane overflowed)
	CBrush brush( RGB(255, 255, 255) );
	CPen pen( PS_SOLID, 1, RGB(255, 255, 255) );
	CBrush * old_brush = pDC->SelectObject( &brush );
	CPen * old_pen = pDC->SelectObject( &pen );
	CRect r (m_client_r);
	r.top = r.bottom-m_bottom_pane_h;
	pDC->Rectangle( &r );
	pDC->SelectObject(old_brush);
	pDC->SelectObject(old_pen);

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
}


// USER INPUT RESPONSE

bool CCommonView::CheckBottomPaneClick(CPoint &point) {
	if( point.y <= (m_client_r.bottom-m_bottom_pane_h) ) return false;
	// clicked in bottom pane, test for hit on function key rectangle
	// CPT: added left-handed mode support
	for( int i=0; i<9; i++ )
	{
		int left = FKEY_OFFSET_X + i*FKEY_STEP;
		if (!m_Doc->m_bLefthanded)
			left += i/4 * FKEY_GAP;
		else
			left += (i+3)/4 * FKEY_GAP;

		CRect r( left,
			m_client_r.bottom-FKEY_OFFSET_Y-FKEY_R_H,
			left+FKEY_R_W,
			m_client_r.bottom-FKEY_OFFSET_Y );
		if( r.PtInRect( point ) )
		{
			// fake function key pressed
			int nChar = i + 112;
			HandleKeyPress( nChar, 0, 0 );
			return true;
		}
	}
	return false;
}

bool CCommonView::CheckLeftPaneClick(CPoint &point) {
	if( point.x >= m_left_pane_w ) return false;
	// clicked in left pane
	InvalidateLeftPane();
	CRect r = m_client_r;
	int y_off = 10;
	int x_off = 10;
	// CPT modified: "Selection" is no longer one of the displayed layers, so click handling changes slightly
	for( int i=1; i<GetNLayers(); i++ )
	{
		// i = position index
		// il = true layer number, since copper layers are displayed out of order
		int il = GetLayerNum(i);
		// get color square
		r.left = x_off;
		r.right = x_off+12;
		r.top = (i-1)*VSTEP+y_off;
		r.bottom = (i-1)*VSTEP+12+y_off;
		if( r.PtInRect( point ) && il > LAY_BACKGND )
		{
			// clicked in color square
			int vis = ToggleLayerVis(il);
			m_dlist->SetLayerVisible( il, vis );
			if( IsFreePcbView() && il == LAY_RAT_LINE && m_Doc->m_vis[il] && g_bShow_Ratline_Warning )
			{
				CDlgMyMessageBox dlg;
				CString s ((LPCSTR) IDS_RatlinesTurnedBackOn);
				dlg.Initialize( s );
				dlg.DoModal();
				g_bShow_Ratline_Warning = !dlg.bDontShowBoxState;
			}
			Invalidate( FALSE );
			return true;
		}
		else
		{
			// get layer name rect
			r.left += 20;
			r.right += 120;
			r.bottom += 5;
			if( r.PtInRect( point ) )
			{
				int nChar = layer_char[i - GetTopCopperLayer()];
				HandleKeyPress( nChar, 0, 0 );
				Invalidate( FALSE );
				return true;
			}
		}
	}

	y_off = r.bottom + 2*VSTEP;
	for( int i=0; i<GetNMasks(); i++ )
	{
		// get color square
		r.left = x_off;
		r.right = x_off+12+120;
		r.top = i*VSTEP+y_off;
		r.bottom = i*VSTEP+12+y_off;
		if( r.PtInRect( point ) )
		{
			// clicked in color square or name
			m_sel_mask = m_sel_mask ^ (1<<i);
			SetSelMaskArray( m_sel_mask );
			Invalidate( FALSE );
			return true;
		}
	}

	return true;
}


BOOL CCommonView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
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
		p = m_dlist->ScreenToPCB( p );
		m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
		m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
		CRect screen_r;
		GetWindowRect( &screen_r );
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, m_org_x, m_org_y );
		Invalidate( FALSE );
		p = m_dlist->PCBToScreen( p );
		SetCursorPos( p.x, p.y - 4 );
	}
	else
	{
		// serial movements, zoom in or out
		if( zDelta > 0 && m_pcbu_per_pixel > NM_PER_MIL/1000 )
		{
			// wheel pushed, zoom in then center world coords and cursor
			CPoint p;
			GetCursorPos( &p );     		// cursor pos in screen coords
			p = m_dlist->ScreenToPCB( p );	// convert to PCB coords
			m_pcbu_per_pixel = m_pcbu_per_pixel/ZOOM_RATIO;
			m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2.0;
			m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2.0;
			CRect screen_r;
			GetWindowRect( &screen_r );
			m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, m_org_x, m_org_y );
			Invalidate( FALSE );
			p = m_dlist->PCBToScreen( p );
			SetCursorPos( p.x, p.y - 4 );
		}
		else if( zDelta < 0 )
		{
			// wheel pulled, zoom out then center
			// first, make sure that window boundaries will be OK
			CPoint p;
			GetCursorPos( &p );		// cursor pos in screen coords
			p = m_dlist->ScreenToPCB( p );
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
				p = m_dlist->PCBToScreen( p );
				SetCursorPos( p.x, p.y - 4 );
			}
		}
	}
	last_time = current_time;

	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

bool CCommonView::HandleLayerKey(UINT nChar, bool bShiftKeyDown, bool bCtrlKeyDown, CDC *pDC) {
	if( nChar>=VK_NUMPAD1 && nChar<=VK_NUMPAD9 )	// Translate number-pad numbers to regular numbers...
		nChar = '1' + nChar - VK_NUMPAD1;
	char * ch = strchr( layer_char, nChar );
	if (!ch) return false;
	int l = ch - layer_char;
	int layer = GetLayerNum(l + GetTopCopperLayer());
	if( layer >= GetNLayers() ) return true;
	InvalidateLeftPane();
	Invalidate(FALSE);
	if (bCtrlKeyDown) 
	{
		// New CPT ctrl-hotkeys
		int vis = ToggleLayerVis(layer);
		m_dlist->SetLayerVisible( layer, vis );
	}
	else if (bShiftKeyDown) HandleShiftLayerKey(layer, pDC);
	else                    HandleNoShiftLayerKey(layer, pDC);
	return true;
}

void CCommonView::HandlePanAndZoom(int nChar, CPoint &p) {
	if (m_Doc->m_bReversePgupPgdn)
		if (nChar==33) nChar = 34;
		else if (nChar==34) nChar = 33;

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
		p = m_dlist->PCBToScreen( p );
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
			p = m_dlist->PCBToScreen( p );
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
			p = m_dlist->PCBToScreen( p );
			SetCursorPos( p.x, p.y );
		}
	}
}

void CCommonView::HandleCtrlFKey(int nChar) {
	int layer = nChar-110;
	int vis = ToggleLayerVis(layer);
	m_dlist->SetLayerVisible( layer, vis );
	InvalidateLeftPane();
	Invalidate( FALSE );
}

// System Key on keyboard pressed down
//
void CCommonView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( nChar == 121 )
		OnKeyDown( nChar, nRepCnt, nFlags);
	else
		CView::OnSysKeyDown(nChar, nRepCnt, nFlags);
	m_sel_offset = -1;									// CPT r294: indicates that a series of mouse-clicks has been interrupted
}

// System Key on keyboard pressed up
//
void CCommonView::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// CPT new hotkeys
	if (nChar>=VK_NUMPAD0 && nChar<=VK_NUMPAD9 )						// Translate number-pad numbers to regular numbers...
		nChar = '0' + nChar - VK_NUMPAD0;
	else if (nChar==VK_SUBTRACT)
		nChar = VK_OEM_MINUS;
	if (nChar>='0' && nChar<='9' || nChar==VK_OEM_MINUS) {
		int sel = nChar-'1';
		if (nChar=='0') sel = 9;
		else if (nChar==VK_OEM_MINUS) sel = 10;
		m_sel_mask = m_sel_mask ^ (1<<sel);
		SetSelMaskArray( m_sel_mask );
		InvalidateLeftPane();
		Invalidate( FALSE );
		return;
		}
	else if (nChar==VK_UP) {
		// Increase visible grid
		CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
		frm->m_wndMyToolBar.VisibleGridUp();
		}
	else if (nChar==VK_DOWN) {
		// Decrease visible grid
		CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
		frm->m_wndMyToolBar.VisibleGridDown();
		}
	// end CPT

	if( nChar != 121 )
		CView::OnSysKeyUp(nChar, nRepCnt, nFlags);
}


void CCommonView::PlacementGridUp() {
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	frm->m_wndMyToolBar.PlacementGridUp();
	}
void CCommonView::PlacementGridDown() {
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	frm->m_wndMyToolBar.PlacementGridDown();
	}


void CCommonView::SnapToAngle(CPoint &wp, int grid_spacing) {
	double ddx = fmod( (double)(m_snap_angle_ref.x), grid_spacing );
	double ddy = fmod( (double)(m_snap_angle_ref.y), grid_spacing );
	if (fabs(ddx)>=.5 || fabs(ddy)>=.5) return;
	// starting point is on-grid, snap to angle
	// snap to n*45 degree angle
	const double pi = 3.14159265359;
	double dx = wp.x - m_snap_angle_ref.x;
	double dy = wp.y - m_snap_angle_ref.y;
	double dist = sqrt( dx*dx + dy*dy );
	double dist45 = dist/sqrt(2.0);
	if( m_Doc->m_snap_angle == 45 )
	{
		// snap angle = 45 degrees, divide circle into 8 octants
		int d = (int)(dist45/grid_spacing+0.5);
		dist45 = d*grid_spacing;
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
		int d = (int)(dist/grid_spacing+0.5);
		dist = d*grid_spacing;
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

void CCommonView::SnapToGridPoint(CPoint &wp, int grid_spacing) 
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


// macro for approximating angles to 1 degree accuracy
#define APPROX(angle,ref) ((angle > ref-M_PI/360) && (angle < ref+M_PI/360))

void CCommonView::SnapToGridLine(CPoint &wp, int grid_spacing) {
	// patch to snap to grid lines, contributed by ???
	// modified by AMW to work when cursor x,y are < 0
	// CPT:  Have my doubts whether this works right...
	// offset cursor and ref positions by integral number of grid spaces
	// to make all values positive
	double offset_grid_spaces;
	modf( (double)INT_MAX/grid_spacing, &offset_grid_spaces );
	double offset = offset_grid_spaces*grid_spacing;
	double off_x = wp.x + offset;
	double off_y = wp.y + offset;
	double ref_x = m_snap_angle_ref.x + offset;
	double ref_y = m_snap_angle_ref.y + offset;
	//find nearest snap angle to an integer division of 90
	int snap_angle = m_Doc->m_snap_angle;
	if(90 % snap_angle != 0)
	{
		int snap_pos = snap_angle;
		int snap_neg = snap_angle;
		while(90 % snap_angle != 0)
		{
			snap_pos++;
			snap_neg--;
			if(snap_pos >= 90)
				snap_pos = 90;
			if(snap_neg <= 1)
				snap_neg = 1;
			if(90 % snap_pos == 0)
				snap_angle = snap_pos;
			if(90 % snap_neg == 0)
				snap_angle = snap_neg;
		}
	}

	//snap the x and y coordinates to the appropriate angle
	double angle_grid = snap_angle*M_PI/180.0;
	double dx = off_x - ref_x;
	double dy = off_y - ref_y;
	double angle = atan2(dy,dx) + 2*M_PI; //make it a positive angle
	if(angle > 2*M_PI)
		angle -= 2*M_PI;
	double angle_angle_grid = angle/angle_grid;
	int sel_snap_angle = (int)angle_angle_grid + ((angle_angle_grid - (double)((int)angle_angle_grid)) > 0.5 ? 1 : 0);
	double point_angle = angle_grid*sel_snap_angle; //result of angle calculation
	CString test, test_grid, s;
	test.Format("point_angle: %f\r\n",point_angle);
	test_grid.Format("grid_spacing: %d\r\n",grid_spacing);

	//find the distance along that angle
	//match the distance the actual point is from the start point
	//double dist = sqrt(dx*dx + dy*dy);
	double dist = dx*cos(point_angle)+dy*sin(point_angle);
	double distx = dist*cos(point_angle);
	double disty = dist*sin(point_angle);

	double xpos = ref_x + distx;
	double ypos = ref_y + disty;

	//special case horizontal lines and vertical lines
	// just to make sure floating point error doesn't cause any problems
	if(APPROX(point_angle,0) || APPROX(point_angle,2*M_PI) || APPROX(point_angle,M_PI))
	{
		//horizontal line
		//snap x component to nearest grid
		off_y = (int)(ypos + 0.5);
		double modval = fmod(xpos,(double)grid_spacing);
		if(modval > grid_spacing/2.0)
		{
			//round up to nearest grid space
			off_x = xpos + ((double)grid_spacing - modval);
		}
		else
		{
			//round down to nearest grid space
			off_x = xpos - modval;
		}
	}
	else if(APPROX(point_angle,M_PI/2) || APPROX(point_angle,3*M_PI/2))
	{
		//vertical line
		//snap y component to nearest grid
		off_x = (int)(xpos + 0.5);
		double modval = fabs(fmod(ypos,(double)grid_spacing));
		int test = modval * grid_spacing - offset;
		if(modval > grid_spacing/2.0)
		{
			off_y = ypos + ((double)grid_spacing - modval);
		}
		else
		{
			off_y = ypos - modval;
		}
	}
	else
	{
		//normal case
		//snap x and y components to nearest grid line along the same angle
		//calculate grid lines surrounding point
		int minx = ((int)(xpos/(double)grid_spacing))*grid_spacing - (xpos < 0);
		//int minx = (int)fmod(xpos,(double)grid_spacing);
		int maxx = minx + grid_spacing;
		int miny = ((int)(ypos/(double)grid_spacing))*grid_spacing - (ypos < 0);
		//int miny = (int)fmod(ypos,(double)grid_spacing);
		int maxy = miny + grid_spacing;

		//calculate the relative distances to each of those grid lines from the ref point
		int rminx = minx - ref_x;
		int rmaxx = maxx - ref_x;
		int rminy = miny - ref_y;
		int rmaxy = maxy - ref_y;

		//calculate the length of the hypotenuse of the triangle
		double maxxh = dist*(double)rmaxx/distx;
		double minxh = dist*(double)rminx/distx;
		double maxyh = dist*(double)rmaxy/disty;
		double minyh = dist*(double)rminy/disty;

		//some stupidly large value.  One of the results MUST be smaller than this unless the grid is this large (unlikely)
		double mindist = 1e100;
		int select = 0;

		if(fabs(maxxh - dist) < mindist)
		{
			select = 1;
			mindist = fabs(maxxh - dist);
		}
		if(fabs(minxh - dist) < mindist)
		{
			select = 2;
			mindist = fabs(minxh - dist);
		}
		if(fabs(maxyh - dist) < mindist)
		{
			select = 3;
			mindist = fabs(maxyh - dist);
		}
		if(fabs(minyh - dist) < mindist)
		{
			select = 4;
			mindist = fabs(minyh - dist);
		}

		switch(select)
		{
		case 1:
			//snap to line right of point
			off_x = maxx;
			off_y = (int)(disty*(double)rmaxx/distx + (double)(ref_y) + 0.5);
			break;
		case 2:
			//snap to line left of point
			off_x = minx;
			off_y = (int)(disty*(double)rminx/distx + (double)(ref_y) + 0.5);
			break;
		case 3:
			//snap to line above point
			off_x = (int)(distx*(double)rmaxy/disty + (double)(ref_x) + 0.5);
			off_y = maxy;
			break;
		case 4:
			//snap to line below point
			off_x = (int)(distx*(double)rminy/disty + (double)(ref_x) + 0.5);
			off_y = miny;
			break;
		default:
			ASSERT(0);//error
		}

	}
	// remove offset and correct for round-off
	double ttest = off_x - offset;
	if( ttest >= 0.0 )
		wp.x = ttest + 0.5;
	else
		wp.x = ttest - 0.5;
	ttest = off_y - offset;
	if( ttest >= 0.0 )
		wp.y = ttest + 0.5;
	else
		wp.y = ttest - 0.5;
}



