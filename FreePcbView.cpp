// FreePcbView.cpp : implementation of the CFreePcbView class
//

#include "stdafx.h"
#include "DlgAddText.h"
#include "DlgAssignNet.h"
#include "DlgSetSegmentWidth.h"
#include "DlgEditBoardCorner.h"
#include "DlgAddArea.h"
#include "DlgRefText.h"
#include "MyToolBar.h"
#include <Mmsystem.h>
#include <sys/timeb.h>
#include <time.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include "freepcbview.h"
#include "DlgAddPart.h"
#include "DlgSetAreaHatch.h"
#include "DlgDupFootprintName.h" 
#include "DlgFindPart.h"
#include "DlgAddMaskCutout.h"
#include "DlgChangeLayer.h"
#include "DlgEditNet.h"
#include "DlgMoveOrigin.h"
#include "DlgMyMessageBox.h"
#include "DlgVia.h"
#include "DlgAreaLayer.h"
#include "DlgGroupPaste.h"
#include "DlgSideStyle.h"
#include "DlgValueText.h"
#include "layers.h"

// globals
BOOL g_bShow_Ratline_Warning = TRUE;	
extern CFreePcbApp theApp;
BOOL t_pressed = FALSE;
BOOL n_pressed = FALSE;
BOOL gLastKeyWasArrow = FALSE;
int gTotalArrowMoveX = 0;
int gTotalArrowMoveY = 0;
BOOL gShiftKeyDown = FALSE;

BOOL gLastKeyWasGroupRotate = FALSE;
long long groupAverageX=0, groupAverageY=0;
int groupNumberItems=0;

HCURSOR my_cursor = LoadCursor( NULL, IDC_CROSS );

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ZOOM_RATIO 1.4

#define MAX_HITS   500		// max number of items selected

// constants for function key menu
#define FKEY_OFFSET_X 4
#define FKEY_OFFSET_Y 4
#define	FKEY_R_W 70
#define FKEY_R_H 30
#define FKEY_STEP (FKEY_R_W+5)
#define FKEY_GAP 20
#define FKEY_SEP_W 16

// constants for layer list
#define VSTEP 14

// macro for approximating angles to 1 degree accuracy
#define APPROX(angle,ref) ((angle > ref-M_PI/360) && (angle < ref+M_PI/360))

//** these must be changed if context menu is edited
enum {
	CONTEXT_NONE = 0,
	CONTEXT_PART,
	CONTEXT_REF_TEXT,
	CONTEXT_PAD,
	CONTEXT_SEGMENT,
	CONTEXT_RATLINE,
	CONTEXT_VERTEX,
	CONTEXT_TEXT,
	CONTEXT_AREA_CORNER,
	CONTEXT_AREA_EDGE,
	CONTEXT_BOARD_CORNER,
	CONTEXT_BOARD_SIDE,
	CONTEXT_END_VERTEX,
	CONTEXT_FP_PAD,
	CONTEXT_SM_CORNER,
	CONTEXT_SM_SIDE,
	CONTEXT_CONNECT,
	CONTEXT_NET,
	CONTEXT_GROUP,
	CONTEXT_VALUE_TEXT
};

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView

IMPLEMENT_DYNCREATE(CFreePcbView, CView)

BEGIN_MESSAGE_MAP(CFreePcbView, CView)
	//{{AFX_MSG_MAP(CFreePcbView)
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
//	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
//	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
//	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
//	ON_WM_SYSCHAR()
//  ON_WM_SYSCOMMAND()
ON_WM_CONTEXTMENU()
ON_COMMAND(ID_PART_MOVE, OnPartMove)
ON_COMMAND(ID_NONE_ADDTEXT, OnTextAdd)
ON_COMMAND(ID_TEXT_DELETE, OnTextDelete)
ON_COMMAND(ID_TEXT_MOVE, OnTextMove)
ON_COMMAND(ID_PART_GLUE, OnPartGlue)
ON_COMMAND(ID_PART_UNGLUE, OnPartUnglue)
ON_COMMAND(ID_PART_DELETE, OnPartDelete)
ON_COMMAND(ID_PART_OPTIMIZE, OnPartOptimize)
ON_COMMAND(ID_REF_MOVE, OnRefMove)
ON_COMMAND(ID_PAD_OPTIMIZERATLINES, OnPadOptimize)
ON_COMMAND(ID_PAD_ADDTONET, OnPadAddToNet)
ON_COMMAND(ID_PAD_DETACHFROMNET, OnPadDetachFromNet)
ON_COMMAND(ID_PAD_CONNECTTOPIN, OnPadConnectToPin)
ON_COMMAND(ID_SEGMENT_SETWIDTH, OnSegmentSetWidth)
ON_COMMAND(ID_SEGMENT_UNROUTE, OnSegmentUnroute)
ON_COMMAND(ID_RATLINE_ROUTE, OnRatlineRoute)
ON_COMMAND(ID_RATLINE_OPTIMIZE, OnRatlineOptimize)
ON_COMMAND(ID_VERTEX_MOVE, OnVertexMove)
ON_COMMAND(ID_VERTEX_DELETE, OnVertexDelete)
ON_COMMAND(ID_VERTEX_SETSIZE, OnVertexProperties)
ON_COMMAND(ID_RATLINE_COMPLETE, OnRatlineComplete)
ON_COMMAND(ID_RATLINE_SETWIDTH, OnRatlineSetWidth)
ON_COMMAND(ID_RATLINE_DELETECONNECTION, OnRatlineDeleteConnection)
ON_COMMAND(ID_RATLINE_LOCKCONNECTION, OnRatlineLockConnection)
ON_COMMAND(ID_RATLINE_UNLOCKCONNECTION, OnRatlineUnlockConnection)
ON_COMMAND(ID_TEXT_EDIT, OnTextEdit)
ON_COMMAND(ID_ADD_BOARDOUTLINE, OnAddBoardOutline)
ON_COMMAND(ID_NONE_ADDBOARDOUTLINE, OnAddBoardOutline)
ON_COMMAND(ID_BOARDCORNER_MOVE, OnBoardCornerMove)
ON_COMMAND(ID_BOARDCORNER_EDIT, OnBoardCornerEdit)
ON_COMMAND(ID_BOARDCORNER_DELETECORNER, OnBoardCornerDelete)
ON_COMMAND(ID_BOARDCORNER_DELETEOUTLINE, OnBoardDeleteOutline)
ON_COMMAND(ID_BOARDSIDE_INSERTCORNER, OnBoardSideAddCorner)
ON_COMMAND(ID_BOARDSIDE_DELETEOUTLINE, OnBoardDeleteOutline)
ON_COMMAND(ID_PAD_STARTSTUBTRACE, OnPadStartStubTrace)
ON_COMMAND(ID_SEGMENT_DELETE, OnSegmentDelete)
ON_COMMAND(ID_ENDVERTEX_MOVE, OnEndVertexMove)
ON_COMMAND(ID_ENDVERTEX_ADDSEGMENTS, OnEndVertexAddSegments)
ON_COMMAND(ID_ENDVERTEX_ADDCONNECTION, OnEndVertexAddConnection)
ON_COMMAND(ID_ENDVERTEX_DELETE, OnEndVertexDelete)
ON_COMMAND(ID_ENDVERTEX_EDIT, OnEndVertexEdit)
ON_COMMAND(ID_AREACORNER_MOVE, OnAreaCornerMove)
ON_COMMAND(ID_AREACORNER_DELETE, OnAreaCornerDelete)
ON_COMMAND(ID_AREACORNER_DELETEAREA, OnAreaCornerDeleteArea)
ON_COMMAND(ID_AREAEDGE_ADDCORNER, OnAreaSideAddCorner)
ON_COMMAND(ID_AREAEDGE_DELETE, OnAreaSideDeleteArea)
ON_COMMAND(ID_AREAEDGE_DELETECUTOUT, OnAreaDeleteCutout)
ON_COMMAND(ID_AREACORNER_DELETECUTOUT, OnAreaDeleteCutout)
ON_COMMAND(ID_ADD_AREA, OnAddArea)
ON_COMMAND(ID_NONE_ADDCOPPERAREA, OnAddArea)
ON_COMMAND(ID_ENDVERTEX_ADDVIA, OnEndVertexAddVia)
ON_COMMAND(ID_ENDVERTEX_REMOVEVIA, OnEndVertexRemoveVia)
ON_COMMAND(ID_ENDVERTEX_SETSIZE, OnVertexProperties)
ON_MESSAGE( WM_USER_VISIBLE_GRID, OnChangeVisibleGrid )
ON_COMMAND(ID_ADD_TEXT, OnTextAdd)
ON_COMMAND(ID_SEGMENT_DELETETRACE, OnSegmentDeleteTrace)
ON_COMMAND(ID_AREACORNER_PROPERTIES, OnAreaCornerProperties)
ON_COMMAND(ID_REF_PROPERTIES, OnRefProperties)
ON_COMMAND(ID_VERTEX_PROPERITES, OnVertexProperties)
ON_WM_ERASEBKGND()
ON_COMMAND(ID_BOARDSIDE_CONVERTTOSTRAIGHTLINE, OnBoardSideConvertToStraightLine)
ON_COMMAND(ID_BOARDSIDE_CONVERTTOARC_CW, OnBoardSideConvertToArcCw)
ON_COMMAND(ID_BOARDSIDE_CONVERTTOARC_CCW, OnBoardSideConvertToArcCcw)
ON_COMMAND(ID_SEGMENT_UNROUTETRACE, OnUnrouteTrace)
ON_COMMAND(ID_VERTEX_UNROUTETRACE, OnUnrouteTrace)
ON_COMMAND(ID_VIEW_ENTIREBOARD, OnViewEntireBoard)
ON_COMMAND(ID_VIEW_ALLELEMENTS, OnViewAllElements)
ON_COMMAND(ID_AREAEDGE_HATCHSTYLE, OnAreaEdgeHatchStyle)
ON_COMMAND(ID_PART_EDITFOOTPRINT, OnPartEditThisFootprint)
ON_COMMAND(ID_PART_SET_REF, OnRefProperties)
ON_COMMAND(ID_RATLINE_CHANGEPIN, OnRatlineChangeEndPin)
ON_WM_KEYUP()
ON_COMMAND(ID_VIEW_FINDPART, OnViewFindpart)
ON_COMMAND(ID_NONE_FOOTPRINTWIZARD, OnFootprintWizard)
ON_COMMAND(ID_NONE_FOOTPRINTEDITOR, OnFootprintEditor)
ON_COMMAND(ID_NONE_CHECKPARTSANDNETS, OnCheckPartsAndNets)
ON_COMMAND(ID_NONE_DRC, OnDrc)
ON_COMMAND(ID_NONE_CLEARDRCERRORS, OnClearDRC)
ON_COMMAND(ID_NONE_VIEWALL, OnViewAll)
ON_COMMAND(ID_ADD_SOLDERMASKCUTOUT, OnAddSoldermaskCutout)
ON_COMMAND(ID_SMCORNER_MOVE, OnSmCornerMove)
ON_COMMAND(ID_SMCORNER_SETPOSITION, OnSmCornerSetPosition)
ON_COMMAND(ID_SMCORNER_DELETECORNER, OnSmCornerDeleteCorner)
ON_COMMAND(ID_SMCORNER_DELETECUTOUT, OnSmCornerDeleteCutout)
ON_COMMAND(ID_SMSIDE_INSERTCORNER, OnSmSideInsertCorner)
ON_COMMAND(ID_SMSIDE_HATCHSTYLE, OnSmSideHatchStyle)
ON_COMMAND(ID_SMSIDE_DELETECUTOUT, OnSmSideDeleteCutout)
ON_COMMAND(ID_PART_CHANGESIDE, OnPartChangeSide)
ON_COMMAND(ID_PART_ROTATE, OnPartRotate)
ON_COMMAND(ID_AREAEDGE_ADDCUTOUT, OnAreaAddCutout)
ON_COMMAND(ID_AREACORNER_ADDCUTOUT, OnAreaAddCutout)
ON_COMMAND(ID_NET_SETWIDTH, OnNetSetWidth)
ON_COMMAND(ID_CONNECT_SETWIDTH, OnConnectSetWidth)
ON_COMMAND(ID_CONNECT_UNROUTETRACE, OnConnectUnroutetrace)
ON_COMMAND(ID_CONNECT_DELETETRACE, OnConnectDeletetrace)
ON_COMMAND(ID_SEGMENT_CHANGELAYER, OnSegmentChangeLayer)
ON_COMMAND(ID_SEGMENT_ADDVERTEX, OnSegmentAddVertex)
ON_COMMAND(ID_CONNECT_CHANGELAYER, OnConnectChangeLayer)
ON_COMMAND(ID_NET_CHANGELAYER, OnNetChangeLayer)
ON_COMMAND(ID_NET_EDITNET, OnNetEditnet)
ON_COMMAND(ID_TOOLS_MOVEORIGIN, OnToolsMoveOrigin)
ON_WM_LBUTTONUP()
ON_COMMAND(ID_GROUP_MOVE, OnGroupMove)
ON_COMMAND(ID_AREACORNER_ADDNEWAREA, OnAddSimilarArea)
ON_COMMAND(ID_AREAEDGE_ADDNEWAREA, OnAddSimilarArea)
ON_COMMAND(ID_AREAEDGE_CHANGELAYER, OnAreaEdit)
ON_COMMAND(ID_AREACORNER_CHANGELAYER, OnAreaEdit)
ON_COMMAND(ID_AREAEDGE_APPLYCLEARANCES, OnAreaEdgeApplyClearances)
ON_COMMAND(ID_GROUP_SAVETOFILE, OnGroupSaveToFile)
ON_COMMAND(ID_GROUP_COPY, OnGroupCopy)
ON_COMMAND(ID_GROUP_CUT, OnGroupCut)
ON_COMMAND(ID_GROUP_DELETE, OnGroupDelete)
ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
ON_COMMAND(ID_VERTEX_CONNECTTOPIN, OnVertexConnectToPin)
ON_COMMAND(ID_EDIT_CUT, OnEditCut)
ON_COMMAND(ID_EDIT_SAVEGROUPTOFILE, OnGroupSaveToFile)
ON_COMMAND(ID_GROUP_ROTATE, OnGroupRotate)
ON_WM_SETCURSOR()
ON_WM_MOVE()
ON_COMMAND(ID_REF_SHOWPART, OnRefShowPart)
ON_COMMAND(ID_AREA_SIDESTYLE, OnAreaSideStyle)
ON_COMMAND(ID_VALUE_MOVE, OnValueMove)
ON_COMMAND(ID_VALUE_CHANGESIZE, OnValueProperties)
ON_COMMAND(ID_VALUE_SHOWPART, OnValueShowPart)
ON_COMMAND(ID_PART_EDITVALUE, OnPartEditValue)
ON_COMMAND(ID_PART_ROTATECOUNTERCLOCKWISE, OnPartRotateCCW)
ON_COMMAND(ID_REF_ROTATECW, OnRefRotateCW)
ON_COMMAND(ID_REF_ROTATECCW, OnRefRotateCCW)
ON_COMMAND(ID_VALUE_ROTATECW, OnValueRotateCW)
ON_COMMAND(ID_VALUE_ROTATECCW, OnValueRotateCCW)
ON_COMMAND(ID_SEGMENT_MOVE, OnSegmentMove)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView construction/destruction

CFreePcbView::CFreePcbView()
{
	// GetDocument() is not available at this point
	m_small_font.CreateFont( 14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "Arial" );
	m_Doc = NULL;
	m_dlist = 0;
	m_last_mouse_point.x = 0;
	m_last_mouse_point.y = 0;
	m_last_cursor_point.x = 0;
	m_last_cursor_point.y = 0;
	m_left_pane_w = 110;	// the left pane on screen is this wide (pixels)
	m_bottom_pane_h = 40;	// the bottom pane on screen is this high (pixels)
	m_memDC_created = FALSE;
	m_dragging_new_item = FALSE;
	m_bDraggingRect = FALSE;
	m_bLButtonDown = FALSE;
	CalibrateTimer();
 }

// initialize the view object
// this code can't be placed in the constructor, because it depends on document
// don't try to draw window until this function has been called
// need only be called once
//
void CFreePcbView::InitInstance()
{
	// this should be called from InitInstance function of CApp,
	// after the document is created
	m_Doc = GetDocument();
	ASSERT_VALID(m_Doc);
	m_Doc->m_edit_footprint = FALSE;
	m_dlist = m_Doc->m_dlist;
	InitializeView();
	CRect screen_r;
	GetWindowRect( &screen_r );
	m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h,
		m_pcbu_per_pixel, m_org_x, m_org_y );
	for(int i=0; i<m_Doc->m_num_layers; i++ )
		m_dlist->SetLayerRGB( i, C_RGB(m_Doc->m_rgb[i][0], m_Doc->m_rgb[i][1], m_Doc->m_rgb[i][2]) );
	ShowSelectStatus();
	ShowActiveLayer();
	m_Doc->m_view = this;
	// set up array of mask ids
	m_mask_id[SEL_MASK_PARTS].Set( ID_PART, -1, ID_SEL_RECT );
	m_mask_id[SEL_MASK_REF].Set( ID_PART, -1, ID_REF_TXT, -1, -1, ID_SEL_TXT );
	m_mask_id[SEL_MASK_VALUE].Set( ID_PART, -1, ID_VALUE_TXT, -1, -1, ID_SEL_TXT );
	m_mask_id[SEL_MASK_PINS].Set( ID_PART, -1, ID_SEL_PAD );
	m_mask_id[SEL_MASK_CON].Set( ID_NET, -1, ID_CONNECT, -1, -1, ID_SEL_SEG );
	m_mask_id[SEL_MASK_VIA].Set( ID_NET, -1, ID_CONNECT, -1, -1, ID_SEL_VERTEX );
	m_mask_id[SEL_MASK_AREAS].Set( ID_NET, -1, ID_AREA );
	m_mask_id[SEL_MASK_TEXT].Set( ID_TEXT, -1, ID_TEXT, -1, -1, ID_SEL_TXT );
	m_mask_id[SEL_MASK_SM].Set( ID_MASK );
	m_mask_id[SEL_MASK_BOARD].Set( ID_BOARD, -1, ID_OUTLINE );
	m_mask_id[SEL_MASK_DRC].Set( ID_DRC );
}

// initialize view with defaults for a new project
// should be called each time a new project is created
//
void CFreePcbView::InitializeView()
{
	if( !m_dlist )
		ASSERT(0);

	// set defaults
	SetCursorMode( CUR_NONE_SELECTED );
	m_sel_id.Clear();
	m_sel_layer = 0;
	m_dir = 0;
	m_debug_flag = 0;
	m_dragging_new_item = 0;
	m_active_layer = LAY_TOP_COPPER;
	m_bDraggingRect = FALSE;
	m_bLButtonDown = FALSE;
	m_sel_mask = 0xffff;
	SetSelMaskArray( m_sel_mask );
	m_inflection_mode = IM_90_45;
	m_snap_mode = SM_GRID_POINTS;

	// default screen coords in world units (i.e. display units)
	m_pcbu_per_pixel = 5.0*PCBU_PER_MIL;	// 5 mils per pixel
	m_org_x = -100.0*PCBU_PER_MIL;			// lower left corner of window
	m_org_y = -100.0*PCBU_PER_MIL;

	// grid defaults
	m_Doc->m_snap_angle = 45;

	// new selection class
//**	ss.Initialize( m_Doc->m_plist, m_Doc->m_nlist ); 

	m_left_pane_invalid = TRUE;
	Invalidate( FALSE );
}

// destructor
CFreePcbView::~CFreePcbView()
{
}

BOOL CFreePcbView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView drawing

void CFreePcbView::OnDraw(CDC* pDC)
{
	// get client rectangle
	GetClientRect( &m_client_r );

	// clear screen to black if no project open
	if( !m_Doc )
	{
		pDC->FillSolidRect( m_client_r, RGB(0,0,0) );
		return;
	}
	if( !m_Doc->m_project_open )
	{
		pDC->FillSolidRect( m_client_r, RGB(0,0,0) );
		return;
	}

	// draw stuff on left pane
	CRect r = m_client_r;
	int y_off = 10;
	int x_off = 10;
	if( m_left_pane_invalid )
	{
		// erase previous contents if changed
		CBrush brush( RGB(255, 255, 255) );
		CPen pen( PS_SOLID, 1, RGB(255, 255, 255) );
		CBrush * old_brush = pDC->SelectObject( &brush );
		CPen * old_pen = pDC->SelectObject( &pen );
		// erase left pane
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
	int index_to_active_layer;
	for( int i=0; i<m_Doc->m_num_layers; i++ )
	{
		// i = position index
		r.left = x_off;
		r.right = x_off+12;
		r.top = i*VSTEP+y_off;
		r.bottom = i*VSTEP+12+y_off;
		// il = layer index, since copper layers are displayed out of order
		int il = i;
		if( i == m_Doc->m_num_layers-1 && m_Doc->m_num_copper_layers > 1 )
			il = LAY_BOTTOM_COPPER;
		else if( i > LAY_TOP_COPPER )
			il = i+1;
		CBrush brush( RGB(m_Doc->m_rgb[il][0], m_Doc->m_rgb[il][1], m_Doc->m_rgb[il][2]) );
		if( m_Doc->m_vis[il] )
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
		if( il == LAY_TOP_COPPER )
			pDC->DrawText( "top copper", -1, &r, DT_TOP );
		else if( il == LAY_BOTTOM_COPPER )
			pDC->DrawText( "bottom", -1, &r, 0 );
		else if( il == LAY_PAD_THRU )
			pDC->DrawText( "drilled hole", -1, &r, 0 );
		else
			pDC->DrawText( &layer_str[il][0], -1, &r, 0 );
		if( il >= LAY_TOP_COPPER )
		{
			CString num_str;
			num_str.Format( "[%c*]", layer_char[i-LAY_TOP_COPPER] );
			CRect nr = r;
			nr.left = nr.right - 55;
			pDC->DrawText( num_str, -1, &nr, DT_TOP );
		}
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
	pDC->DrawText( "SELECTION MASK", -1, &r, DT_TOP );
	y_off = r.bottom;
	for( int i=0; i<NUM_SEL_MASKS; i++ )
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
		pDC->DrawText( sel_mask_str[i], -1, &r, DT_TOP );
	}
	r.left = x_off;
	r.bottom += VSTEP*2;
	r.top += VSTEP*2;
	pDC->DrawText( "* Use these", -1, &r, DT_TOP );
	r.bottom += VSTEP;
	r.top += VSTEP;
	pDC->DrawText( "keys to change", -1, &r, DT_TOP );
	r.bottom += VSTEP;
	r.top += VSTEP;
	pDC->DrawText( "routing layer", -1, &r, DT_TOP );

	// draw function keys on bottom pane
	DrawBottomPane();

	//** this is for testing only, needs to be converted to PCB coords
#if 0
	if( b_update_rect )
	{
		// clip to update rectangle
		CRgn rgn;
		rgn.CreateRectRgn( m_left_pane_w + update_rect.left,
			update_rect.bottom,
			m_left_pane_w + update_rect.right,
			update_rect.top  );
		pDC->SelectClipRgn( &rgn );
	}
	else
#endif
	{
		// clip to pcb drawing region
		pDC->SelectClipRgn( &m_pcb_rgn );
	}

	// now draw the display list
	SetDCToWorldCoords( pDC );
	m_dlist->Draw( pDC );
}

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView printing

BOOL CFreePcbView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CFreePcbView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CFreePcbView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView diagnostics

#ifdef _DEBUG
void CFreePcbView::AssertValid() const
{
	CView::AssertValid();
}

void CFreePcbView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CFreePcbDoc* CFreePcbView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFreePcbDoc)));
	return (CFreePcbDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView message handlers

// Window was resized
//
void CFreePcbView::OnSize(UINT nType, int cx, int cy)
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
		ReleaseDC( pDC );
	}
}

// select item from id sid
// sets m_sel_id and pointer to top level item 
// returns TRUE if successful
//
BOOL CFreePcbView::SelectItem( id sid )
{
	if( sid.IsDRC() )
	{
#if 0
		CancelSelection();
		DRError * dre = (DRError*)ptr;
		m_sel_id = sid;
		m_sel_dre = dre;
		m_Doc->m_drelist->HighLight( m_sel_dre );
		SetCursorMode( CUR_DRE_SELECTED );
		Invalidate( FALSE );
#endif
	}
	else if( sid.IsBoardCorner() )
	{
		CancelSelection();
		m_Doc->m_board_outline[sid.I2()].HighlightCorner( sid.I3() );
		m_sel_id = sid;
		SetCursorMode( CUR_BOARD_CORNER_SELECTED );
		Invalidate( FALSE );
	}
	else if( sid.IsBoardSide() )
	{
		CancelSelection();
		m_Doc->m_board_outline[sid.I2()].HighlightSide( sid.I3() );
		m_sel_id = sid;
		SetCursorMode( CUR_BOARD_SIDE_SELECTED );
		Invalidate( FALSE );
	}
	else if( sid.IsMaskCorner() )
	{
		CancelSelection();
		m_Doc->m_sm_cutout[sid.I2()].HighlightCorner( sid.I3() );
		m_sel_id = sid;
		SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
		Invalidate( FALSE );
	}
	else if( sid.IsMaskSide() )
	{
		CancelSelection();
		m_Doc->m_sm_cutout[sid.I2()].HighlightSide( sid.I3() );
		m_sel_id = sid;
		SetCursorMode( CUR_SMCUTOUT_SIDE_SELECTED );
		Invalidate( FALSE );
	}
	else if( sid.T1() == ID_PART )
	{
		CancelSelection();
		m_sel_part = sid.Part();
		m_sel_id = sid;
		if( (GetKeyState('N') & 0x8000) && sid.T2()  == ID_SEL_PAD )
		{
			// pad selected and if "n" held down, select net
			cnet * net = m_Doc->m_plist->GetPinNet( m_sel_part, sid.I2() );
			if( net )
			{
				m_sel_net = sid.Net();
				m_sel_id = net->m_id;
				m_sel_id.SetT2( ID_ENTIRE_NET );
				m_Doc->m_nlist->HighlightNetConnections( m_sel_net );
				m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net );
				SetCursorMode( CUR_NET_SELECTED );
			}
			else
			{
				m_Doc->m_plist->SelectPad( m_sel_part, sid.I2() );
				SetCursorMode( CUR_PAD_SELECTED );
				Invalidate( FALSE );
			}
		}
		else if( sid.IsPart() )
		{
			SelectPart( m_sel_part );
			m_Doc->m_plist->SelectRefText( m_sel_part );
			m_Doc->m_plist->SelectValueText( m_sel_part );
		}
		else if( sid.IsRefText() )
		{
			m_Doc->m_plist->SelectRefText( m_sel_part );
			SetCursorMode( CUR_REF_SELECTED );
			Invalidate( FALSE );
		}
		else if( sid.IsValueText() )
		{
			m_Doc->m_plist->SelectValueText( m_sel_part );
			SetCursorMode( CUR_VALUE_SELECTED );
			Invalidate( FALSE );
		}
		else if( sid.IsPin() )
		{
			m_Doc->m_plist->SelectPad( m_sel_part, sid.I2() );
			SetCursorMode( CUR_PAD_SELECTED );
			Invalidate( FALSE );
		}
	}
	else if( sid.T1() == ID_NET )
	{
		CancelSelection();
		m_sel_net = sid.Net();
		m_sel_id = sid;
		if( sid.T2()  == ID_CONNECT && sid.T3() == ID_SEL_SEG )
		{
			// select segment
			m_Doc->m_nlist->HighlightSegment( m_sel_net, sid.I2(), sid.I3() );
			if( m_sel_seg->layer != LAY_RAT_LINE )
				SetCursorMode( CUR_SEG_SELECTED );
			else
				SetCursorMode( CUR_RAT_SELECTED );
			Invalidate( FALSE );
		}
		else if( sid.T2()  == ID_CONNECT && sid.T3() == ID_SEL_VERTEX )
		{
			// select vertex
			cvertex * v = m_sel_id.Vtx();
			if( v->GetType() == cvertex::V_END )
				SetCursorMode( CUR_END_VTX_SELECTED );
			else
				SetCursorMode( CUR_VTX_SELECTED );
			m_Doc->m_nlist->HighlightVertex( m_sel_net, sid.I2(), sid.I3() );
			Invalidate( FALSE );
		}
		else if( sid.T2()  == ID_AREA && sid.T3() == ID_SEL_SIDE )
		{
			// select copper area side
			m_Doc->m_nlist->SelectAreaSide( m_sel_net, sid.I2(), sid.I3() );
			SetCursorMode( CUR_AREA_SIDE_SELECTED );
			Invalidate( FALSE );
		}
		else if( sid.T2()  == ID_AREA && sid.T3() == ID_SEL_CORNER )
		{
			// select copper area corner
			m_Doc->m_nlist->SelectAreaCorner( m_sel_net, sid.I2(), sid.I3() );
			SetCursorMode( CUR_AREA_CORNER_SELECTED );
			Invalidate( FALSE );
		}
		else
			ASSERT(0);
	}
	else if( sid.T1() == ID_TEXT )
	{
		CancelSelection();
		m_sel_text = sid.Text();
		m_sel_id = sid;
		m_Doc->m_tlist->HighlightText( m_sel_text );
		SetCursorMode( CUR_TEXT_SELECTED );
		Invalidate( FALSE );
	}
	else
	{
		// nothing selected
		return FALSE;
		CancelSelection();
		m_sel_id.Clear();
		Invalidate( FALSE );
	}
	return TRUE;
}

// Displays a popup menu for the mouse hits in hit_info
//
// Param:
//	point    - current mouse position (relative to client window)
//	hit_info - Drawing objects hit by mouse (must be sorted - priority order)
//	num_hits - # objects in hit_info array
int CFreePcbView::SelectObjPopup( CPoint const &point, CDL_job::HitInfo hit_info[], int num_hits )
{
	CDC *winDC = GetDC();

	CDC dc;
	dc.CreateCompatibleDC(winDC);
	dc.SetMapMode(MM_TEXT);
	dc.SetWindowExt( 1,1 );
	dc.SetWindowOrg( 0,0 );
	dc.SetViewportExt( 1,1 );
	dc.SetViewportOrg( 0,0 );

	CDL_job::HitInfo *pInfo;

	int idx;

	// Create bitmap array
	CArray<CBitmap> bitmaps;
	bitmaps.SetSize(num_hits);

	int sel = 0;
	{
		CString str;
		CMenu file_menu;
		file_menu.CreatePopupMenu();

		for( idx = 0, pInfo = &hit_info[0]; idx < num_hits; idx++, pInfo++ )
		{
			// Don't display masked items
			if( pInfo->priority < 0 )
			{
				break;
			}

			CRect r(0,0, 139,23);
			CBitmap *pBitmap = &bitmaps[idx];
			str = "";

			pBitmap->CreateCompatibleBitmap(winDC, r.Width()+1, r.Height()+1);

			CBitmap *pOldBitmap = dc.SelectObject(pBitmap);
			{
				COLORREF layer_color = C_RGB( m_Doc->m_rgb[ pInfo->layer ][0],
												m_Doc->m_rgb[ pInfo->layer ][1],
												m_Doc->m_rgb[ pInfo->layer ][2] );
				COLORREF text_color  = C_RGB(m_Doc->m_rgb[ LAY_BACKGND ][0],
												m_Doc->m_rgb[ LAY_BACKGND ][1],
												m_Doc->m_rgb[ LAY_BACKGND ][2] );

				dc.FillSolidRect(r, layer_color);
				dc.SetTextColor(text_color);

				if( pInfo->ID.T1() == ID_BOARD )
				{
					str = "BOARD";

					if( pInfo->ID.T3() == ID_SEL_SIDE )
					{
						str += " SIDE";
					}
					else if( pInfo->ID.T3() == ID_SEL_CORNER )
					{
						str += " CORNER";
					}
				}
				else if( pInfo->ID.T1() == ID_PART )
				{
					cpart *part = (cpart*)pInfo->ptr;

					if( pInfo->ID.T2() == ID_SEL_PAD )
					{
						str.Format( "PIN: %s.%s", part->ref_des, part->shape->GetPinNameByIndex(pInfo->ID.I2()) );
					}
					else if( pInfo->ID.T2() == ID_SEL_RECT )
					{
						CShape *shape = part->shape;

						if( shape )
						{
							CMetaFileDC m_mfDC;

							CRect shape_bounds = shape->GetBounds();
							int dx = -shape_bounds.Height() / NM_PER_MIL;

							// Scale part bitmap height between 40 and 128 for better readability
							r.bottom = 32 + dx / 11;
							if( r.bottom > 128 ) r.bottom = 128;

							// Trade in the default bitmap for the new one
							dc.SelectObject(pOldBitmap);
							pBitmap->DeleteObject();
							pBitmap->CreateCompatibleBitmap(winDC, r.Width()+1, r.Height()+1);
							dc.SelectObject(pBitmap);

							// Draw the shape with actual ref_des & no selection rectangle
							HENHMETAFILE hMF = shape->CreateMetafile( &m_mfDC, winDC, r, part->ref_des, FALSE );
							dc.PlayMetaFile( hMF, r );
							DeleteEnhMetaFile( hMF );
						}
					}
					else if( pInfo->ID.T2() == ID_REF_TXT )
					{
						str.Format("REF: %s", part->ref_des);
					}
					else if( pInfo->ID.T2() == ID_VALUE_TXT )
					{
						str.Format("VALUE: %s", part->value);
					}
				}
				else if( pInfo->ID.T1() == ID_NET )
				{
					cnet *net = (cnet*)pInfo->ptr;

					if( pInfo->ID.T2() == ID_CONNECT )
					{
						if( pInfo->ID.T3() == ID_SEL_SEG )
						{
							str = "SEGMENT";
						}
						else if( pInfo->ID.T3() == ID_SEL_VERTEX )
						{
							if( net->ConByIndex(pInfo->ID.I2())->VtxByIndex(pInfo->ID.I3()).via_w )
							{
								str = "VIA";
							}
							else
							{
								str = "VERTEX";
							}
						}
					}
					else if( pInfo->ID.T2() == ID_AREA )
					{
						str = "COPPER";

						if( pInfo->ID.T3() == ID_SEL_SIDE )
						{
							str += " SIDE";
						}
						else if( pInfo->ID.T3() == ID_SEL_CORNER )
						{
							str += " CORNER";
						}
					}
				}
				else if( pInfo->ID.T1() == ID_TEXT )
				{
					CText *text = (CText*)pInfo->ptr;

					str.Format("TEXT: %s", text->m_str);
				}
				else if( pInfo->ID.T1() == ID_DRC )
				{
					str = "DRC";
				}
				else if( pInfo->ID.T1() == ID_MASK )
				{
					str = "CUTOUT";

					if( pInfo->ID.T3() == ID_SEL_SIDE )
					{
						str += " SIDE";
					}
					else if( pInfo->ID.T3() == ID_SEL_CORNER )
					{
						str += " CORNER";
					}
				}
				else if( pInfo->ID.T1() == ID_CENTROID )
				{
					str = "CENTROID";
				}
				else if( pInfo->ID.T1() == ID_GLUE )
				{
					str = "GLUE SPOT";
				}
				else
				{
					str = "Unknown";
				}

				if( str.GetLength() > 0 )
				{
					dc.TextOut( 10,3, str );
				}

				// Draw bounding box around the bitmap
				dc.MoveTo(r.left,r.top);
				dc.LineTo(r.right,r.top);
				dc.LineTo(r.right,r.bottom);
				dc.LineTo(r.left,r.bottom);
				dc.LineTo(r.left,r.top);
			}
			dc.SelectObject(pOldBitmap);

			file_menu.AppendMenu( MF_STRING, idx + 1, pBitmap );
		}

		if (idx > 0)
		{
			CRect r;
			GetWindowRect(r);
			sel = file_menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x + r.left + 5, point.y + r.top + 5, this);
		}
	}

	// Release GDI objects
	bitmaps.RemoveAll();
	ReleaseDC(&dc);
	ReleaseDC(winDC);

	return (sel - 1);
}


// Left mouse button released, we should probably do something
//
void CFreePcbView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if( !m_bLButtonDown )
	{
		// this avoids problems with opening a project with the button held down
		CView::OnLButtonUp(nFlags, point);
		return;
	}

	CDC * pDC = NULL;
	CPoint tp = m_dlist->WindowToPCB( point );

	m_bLButtonDown = FALSE;
	gLastKeyWasArrow = FALSE;		// cancel series of arrow keys
	gLastKeyWasGroupRotate=false;	// cancel series of group rotations
	if( m_bDraggingRect )
	{
		// we were dragging selection rect, handle it
		m_last_drag_rect.NormalizeRect();
		CPoint tl = m_dlist->WindowToPCB( m_last_drag_rect.TopLeft() );
		CPoint br = m_dlist->WindowToPCB( m_last_drag_rect.BottomRight() );
		m_sel_rect = CRect( tl, br );
		if( nFlags & MK_CONTROL )
		{
			// control key held down
			// if current selection is a single item, convert to group
			if( m_cursor_mode == CUR_PART_SELECTED )
			{
				if( m_sel_id.T1() != ID_PART && m_sel_id.T2()  != ID_SEL_RECT )
					ASSERT(0);
				m_sel_ids.Add( m_sel_id );
				m_sel_ptrs.Add( m_sel_part );
				SetCursorMode( CUR_GROUP_SELECTED );
			}
			else if( m_cursor_mode == CUR_TEXT_SELECTED )
			{
				if( m_sel_id.T1() != ID_TEXT )
					ASSERT(0);
				m_sel_ids.Add( m_sel_id );
				m_sel_ptrs.Add( m_sel_text );
				SetCursorMode( CUR_GROUP_SELECTED );
			}
			else if( m_cursor_mode == CUR_SEG_SELECTED )
			{
				if( m_sel_id.T1() != ID_NET )
					ASSERT(0);
				m_sel_ids.Add( m_sel_id );
				m_sel_ptrs.Add( m_sel_net );
				SetCursorMode( CUR_GROUP_SELECTED );
			}
			if( m_cursor_mode == CUR_GROUP_SELECTED )
			{
				SelectItemsInRect( m_sel_rect, TRUE );
			}
			else
				SelectItemsInRect( m_sel_rect, FALSE );
		}
		else
		{
			SelectItemsInRect( m_sel_rect, FALSE );
		}
		m_bDraggingRect = FALSE;
		Invalidate( FALSE );
		CView::OnLButtonUp(nFlags, point);
		return;
	}

	if( point.y > (m_client_r.bottom-m_bottom_pane_h) )
	{
		// clicked in bottom pane, test for hit on function key rectangle
		for( int i=0; i<9; i++ )
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
			}
		}
	}
	else if( point.x < m_left_pane_w )
	{
		// clicked in left pane
		CRect r = m_client_r;
		int y_off = 10;
		int x_off = 10;
		for( int i=0; i<m_Doc->m_num_layers; i++ )
		{
			// i = position index
			// il = layer index, since copper layers are displayed out of order
			int il = i;
			if( i == m_Doc->m_num_layers-1 && m_Doc->m_num_copper_layers > 1 )
				il = LAY_BOTTOM_COPPER;
			else if( i > LAY_TOP_COPPER )
				il = i+1;
			// get color square
			r.left = x_off;
			r.right = x_off+12;
			r.top = i*VSTEP+y_off;
			r.bottom = i*VSTEP+12+y_off;
			if( r.PtInRect( point ) && il > LAY_BACKGND )
			{
				// clicked in color square
				m_Doc->m_vis[il] = !m_Doc->m_vis[il];
				m_dlist->SetLayerVisible( il, m_Doc->m_vis[il] );
				if( il == LAY_RAT_LINE && m_Doc->m_vis[il] && g_bShow_Ratline_Warning )
				{
					CDlgMyMessageBox dlg;
					dlg.Initialize( "Ratlines turned back on, but may not be up to date.\n\nPress F9 to recalculate." );
					dlg.DoModal();
					g_bShow_Ratline_Warning = !dlg.bDontShowBoxState;
				}
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
					if( i >= LAY_TOP_COPPER && i < LAY_TOP_COPPER + 8 )
					{
						int nChar = '1' + i - LAY_TOP_COPPER;
						HandleKeyPress( nChar, 0, 0 );
						Invalidate( FALSE );
					}
					else
					{
						switch( i )
						{
						case LAY_TOP_COPPER: HandleKeyPress( '1', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+1: HandleKeyPress( '2', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+2: HandleKeyPress( '3', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+3: HandleKeyPress( '4', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+4: HandleKeyPress( '5', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+5: HandleKeyPress( '6', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+6: HandleKeyPress( '7', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+7: HandleKeyPress( '8', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+8: HandleKeyPress( 'Q', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+9: HandleKeyPress( 'W', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+10: HandleKeyPress( 'E', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+11: HandleKeyPress( 'R', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+12: HandleKeyPress( 'T', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+13: HandleKeyPress( 'Y', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+14: HandleKeyPress( 'U', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+15: HandleKeyPress( 'I', 0, 0 ); Invalidate( FALSE ); break;
						}
					}
				}
			}
		}
		y_off = r.bottom + 2*VSTEP;
		for( int i=0; i<NUM_SEL_MASKS; i++ )
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
			}
		}
	}
	else
	{
		// clicked in PCB pane
		if(	CurNone() || CurSelected() )
		{
			// see if new item selected
			CPoint p = m_dlist->WindowToPCB( point );
			id sid;
			void * sel_ptr = NULL;		// pointer to last selected top-level item
			if( m_sel_id.T1() == ID_PART )
				sel_ptr = m_sel_part;
			else if( m_sel_id.T1() == ID_NET )
				sel_ptr = m_sel_net;
			else if( m_sel_id.T1() == ID_TEXT )
				sel_ptr = m_sel_text;
			else if( m_sel_id.T1() == ID_DRC )
				sel_ptr = m_sel_dre;

			// save masks in case they are changed
			id old_mask_pins = m_mask_id[SEL_MASK_PINS];
			id old_mask_ref = m_mask_id[SEL_MASK_REF];
			if( nFlags & MK_CONTROL && m_mask_id[SEL_MASK_PARTS].I3() == 0xfffe )
			{
				// if control key pressed and parts masked, also mask pins and ref
				m_mask_id[SEL_MASK_PINS].SetI3( 0xfffe );
				m_mask_id[SEL_MASK_REF].SetI3( 0xfffe );
			}

			// Default selection mask is to use the global selection mask
			id *pMask_id = m_mask_id;
			int num_incl_masks = NUM_SEL_MASKS;
			id mask_id[4];
#if 0
			if( nFlags & MK_SHIFT )
			{
				// On shift, allow all items to be selected
				pMask_id = NULL;
				num_incl_masks = 0;
			}
#endif

			//************* now see if item was selected *******************
			int idx;
			int num_hits;
			CDL_job::HitInfo hit_info[MAX_HITS];
			void * ptr = NULL;
			sid.Clear();

			if( nFlags & MK_SHIFT )
			{
				idx = m_dlist->TestSelect(
					p.x, p.y,                     // Point
					hit_info, 25, num_hits,       // Hit Information
					NULL, NULL,                   // No exclusions
					NULL, 0						  // Inclusions
				);
			}
			else
			{
				idx = m_dlist->TestSelect(
					p.x, p.y,					  // Point
					hit_info, MAX_HITS, num_hits, // Hit Information
					&m_sel_id, sel_ptr,			  // Exclusions
					pMask_id, num_incl_masks      // Inclusions
				);
			}

			// restore mask
			m_mask_id[SEL_MASK_PINS] = old_mask_pins;
			m_mask_id[SEL_MASK_REF] = old_mask_ref;

			if( idx >= 0 )
			{
				if( nFlags & MK_SHIFT )
				{
					std::sort( &hit_info[0], &hit_info[num_hits] );

					idx = SelectObjPopup( point, hit_info, num_hits );
				}
			}

			if( idx >= 0 )
			{
				void * ptr = hit_info[idx].ptr;
 				id sid = hit_info[idx].ID;
				m_sel_layer = hit_info[idx].layer;

				// check for second pad selected while holding down 's'
				SHORT kc = GetKeyState( 'S' );
				if( kc & 0x8000 && m_cursor_mode == CUR_PAD_SELECTED )
				{
					if( sid.T1() == ID_PART && sid.T2()  == ID_SEL_PAD )
					{
						CString mess;
						// OK, now swap pads
						cpart * part1 = m_sel_part;
						CString pin_name1 = part1->shape->GetPinNameByIndex( m_sel_id.I2() );
						cnet * net1 = m_Doc->m_plist->GetPinNet(part1, &pin_name1);
						CString net_name1 = "unconnected";
						if( net1 )
							net_name1 = net1->name;
						cpart * part2 = (cpart*)ptr;
						CString pin_name2 = part2->shape->GetPinNameByIndex( sid.I2() );
						cnet * net2 = m_Doc->m_plist->GetPinNet(part2, &pin_name2);
						CString net_name2 = "unconnected";
						if( net2 )
							net_name2 = net2->name;
						if( net1 == NULL && net2 == NULL )
						{
							AfxMessageBox( "No connections to swap" );
							return;
						}
						mess.Format( "Swap %s.%s (\"%s\") and %s.%s (\"%s\") ?",
							part1->ref_des, pin_name1, net_name1,
							part2->ref_des, pin_name2, net_name2 );
						int ret = AfxMessageBox( mess, MB_OKCANCEL );
						if( ret == IDOK )
						{
							SaveUndoInfoFor2PartsAndNets( part1, part2, TRUE, m_Doc->m_undo_list );
							m_Doc->m_nlist->SwapPins( part1, &pin_name1, part2, &pin_name2 );
							m_Doc->ProjectModified( TRUE );
							ShowSelectStatus();
							Invalidate( FALSE );
						}
						return;
					}
				}

				// now handle new selection
				if( nFlags & MK_CONTROL )
				{
					// control key held down
					if(    sid.T1() == ID_PART
							&& m_mask_id[SEL_MASK_PARTS].I3() != 0xfffe
						|| sid.T1() == ID_TEXT
							&& m_mask_id[SEL_MASK_TEXT].I3() != 0xfffe
						|| (sid.T1() == ID_NET && sid.T2()  == ID_CONNECT && sid.T3() == ID_SEL_SEG
							&& ((cnet*)ptr)->ConByIndex(sid.I2())->SegByIndex(sid.I3()).layer != LAY_RAT_LINE)
							&& m_mask_id[SEL_MASK_CON].I3() != 0xfffe
						|| sid.T1() == ID_NET && sid.T2()  == ID_CONNECT && sid.T3() == ID_SEL_VERTEX
							&& (((cnet*)ptr)->ConByIndex(sid.I2())->VtxByIndex(sid.I3()).tee_ID
								|| ((cnet*)ptr)->ConByIndex(sid.I2())->VtxByIndex(sid.I3()).force_via_flag )
							&& m_mask_id[SEL_MASK_VIA].I3() != 0xfffe
						|| sid.T1() == ID_NET && sid.T2()  == ID_AREA && sid.T3() == ID_SEL_SIDE
							&& m_mask_id[SEL_MASK_AREAS].I3() != 0xfffe
						|| sid.T1() == ID_MASK && sid.T2()  == ID_MASK && sid.T3() == ID_SEL_SIDE
							&& m_mask_id[SEL_MASK_SM].I3() != 0xfffe
						|| sid.T1() == ID_BOARD && sid.T2()  == ID_BOARD && sid.T3() == ID_SEL_SIDE
							&& m_mask_id[SEL_MASK_BOARD].I3() != 0xfffe
							)
					{
						// legal selection for group
						if( sid.T1() == ID_PART )
						{
							sid.SetT2( ID_SEL_RECT );
							sid.SetI2( 0 );
							sid.SetT3( 0 );
							sid.SetI3( 0 );
						}
						// if previous single selection, convert to group
						if( m_cursor_mode == CUR_PART_SELECTED )
						{
							m_sel_ids.Add( m_sel_id );
							m_sel_ptrs.Add( m_sel_part );
							SetCursorMode( CUR_GROUP_SELECTED );
							m_sel_id.SetT1(ID_MULTI);
						}
						else if( m_cursor_mode == CUR_SEG_SELECTED )
						{
							m_sel_ids.Add( m_sel_id );
							m_sel_ptrs.Add( m_sel_net );
							SetCursorMode( CUR_GROUP_SELECTED );
							m_sel_id.SetT1(ID_MULTI);
						}
						else if( m_cursor_mode == CUR_AREA_SIDE_SELECTED )
						{
							m_sel_ids.Add( m_sel_id );
							m_sel_ptrs.Add( m_sel_net );
							SetCursorMode( CUR_GROUP_SELECTED );
							m_sel_id.SetT1(ID_MULTI);
						}
						else if( m_cursor_mode == CUR_SMCUTOUT_SIDE_SELECTED
							|| m_cursor_mode == CUR_BOARD_SIDE_SELECTED )
						{
							m_sel_ids.Add( m_sel_id );
							m_sel_ptrs.Add( NULL );
							SetCursorMode( CUR_GROUP_SELECTED );
							m_sel_id.SetT1(ID_MULTI);
						}
						else if( m_cursor_mode == CUR_TEXT_SELECTED )
						{
							if( m_sel_ids.GetSize() )
								ASSERT(0);
							m_sel_ids.Add( m_sel_id );
							m_sel_ptrs.Add( m_sel_text );
							SetCursorMode( CUR_GROUP_SELECTED );
							m_sel_id.SetT1(ID_MULTI);
						}
						// now add or remove from group
						if( m_cursor_mode == CUR_GROUP_SELECTED )
						{
							BOOL bFound = FALSE;
							for( int i=0; i<m_sel_ids.GetSize(); i++ )
							{
								id tid = m_sel_ids[i];
								void * tptr = m_sel_ptrs[i];
								if( tid == sid && m_sel_ptrs[i] == ptr )
								{
									bFound = TRUE;
									m_sel_ptrs.RemoveAt(i);
									m_sel_ids.RemoveAt(i);
								}
							}
							if( !bFound )
							{
								m_sel_ids.Add( sid );
								m_sel_ptrs.Add( ptr );
							}
							if( m_sel_ids.GetSize() == 0 )
							{
								CancelSelection();
							}
							else
							{
								HighlightGroup();
							}
							Invalidate( FALSE );
						}
					}
				}
				else
				{
					BOOL bOK = SelectItem( sid );
				}
#if 0
				else if( sid.T1() == ID_DRC && sid.T2()  == ID_SEL_DRE )
				{
					CancelSelection();
					DRError * dre = (DRError*)ptr;
					m_sel_id = sid;
					m_sel_dre = dre;
					m_Doc->m_drelist->HighLight( m_sel_dre );
					SetCursorMode( CUR_DRE_SELECTED );
					Invalidate( FALSE );
				}
				else if( sid.T1() == ID_BOARD && sid.T2()  == ID_OUTLINE
					&& sid.T3() == ID_SEL_CORNER )
				{
					CancelSelection();
					m_Doc->m_board_outline[sid.I2()].HighlightCorner( sid.I3() );
					m_sel_id = sid;
					SetCursorMode( CUR_BOARD_CORNER_SELECTED );
					Invalidate( FALSE );
				}
				else if( sid.T1() == ID_BOARD && sid.T2()  == ID_OUTLINE
					&& sid.T3() == ID_SEL_SIDE )
				{
					CancelSelection();
					m_Doc->m_board_outline[sid.I2()].HighlightSide( sid.I3() );
					m_sel_id = sid;
					SetCursorMode( CUR_BOARD_SIDE_SELECTED );
					Invalidate( FALSE );
				}
				else if( sid.T1() == ID_MASK && sid.T2()  == ID_MASK
					&& sid.T3() == ID_SEL_CORNER )
				{
					CancelSelection();
					m_Doc->m_sm_cutout[sid.I2()].HighlightCorner( sid.I3() );
					m_sel_id = sid;
					SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
					Invalidate( FALSE );
				}
				else if( sid.T1() == ID_MASK && sid.T2()  == ID_MASK
					&& sid.T3() == ID_SEL_SIDE )
				{
					CancelSelection();
					m_Doc->m_sm_cutout[sid.I2()].HighlightSide( sid.I3() );
					m_sel_id = sid;
					SetCursorMode( CUR_SMCUTOUT_SIDE_SELECTED );
					Invalidate( FALSE );
				}
				else if( sid.T1() == ID_PART )
				{
					CancelSelection();
					m_sel_part = (cpart*)ptr;
					m_sel_id = sid;
					if( (GetKeyState('N') & 0x8000) && sid.T2()  == ID_SEL_PAD )
					{
						// pad selected and if "n" held down, select net
						cnet * net = m_Doc->m_plist->GetPinNet( m_sel_part, sid.I2() );
						if( net )
						{
							m_sel_net = net;
							m_sel_id = net->m_id;
							m_sel_id.SetT2( ID_ENTIRE_NET );
							m_Doc->m_nlist->HighlightNetConnections( m_sel_net );
							m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net );
							SetCursorMode( CUR_NET_SELECTED );
						}
						else
						{
							m_Doc->m_plist->SelectPad( m_sel_part, sid.I2() );
							SetCursorMode( CUR_PAD_SELECTED );
							Invalidate( FALSE );
						}
					}
					else if( sid.T2()  == ID_SEL_RECT )
					{
						SelectPart( m_sel_part );
						m_Doc->m_plist->SelectRefText( m_sel_part );
						m_Doc->m_plist->SelectValueText( m_sel_part );
					}
					else if( sid.T2()  == ID_REF_TXT )
					{
						m_Doc->m_plist->SelectRefText( m_sel_part );
						SetCursorMode( CUR_REF_SELECTED );
						Invalidate( FALSE );
					}
					else if( sid.T2()  == ID_VALUE_TXT )
					{
						m_Doc->m_plist->SelectValueText( m_sel_part );
						SetCursorMode( CUR_VALUE_SELECTED );
						Invalidate( FALSE );
					}
					else if( sid.T2()  == ID_SEL_PAD )
					{
						m_Doc->m_plist->SelectPad( m_sel_part, sid.I2() );
						SetCursorMode( CUR_PAD_SELECTED );
						Invalidate( FALSE );
					}
				}
				else if( sid.T1() == ID_NET )
				{
					CancelSelection();
					m_sel_net = (cnet*)ptr;
					m_sel_id = sid;
					if( sid.T2()  == ID_CONNECT && sid.T3() == ID_SEL_SEG )
					{
						// select segment
						m_Doc->m_nlist->HighlightSegment( m_sel_net, sid.I2(), sid.I3() );
						if( m_sel_seg->layer != LAY_RAT_LINE )
							SetCursorMode( CUR_SEG_SELECTED );
						else
							SetCursorMode( CUR_RAT_SELECTED );
						Invalidate( FALSE );
					}
					else if( sid.T2()  == ID_CONNECT && sid.T3() == ID_SEL_VERTEX )
					{
						// select vertex
						cconnect * c = m_sel_con;
						if( c->EndPin() == NULL && sid.I3() == c->NumSegs() )
							SetCursorMode( CUR_END_VTX_SELECTED );
						else
							SetCursorMode( CUR_VTX_SELECTED );
						m_Doc->m_nlist->HighlightVertex( m_sel_net, sid.I2(), sid.I3() );
						Invalidate( FALSE );
					}
					else if( sid.T2()  == ID_AREA && sid.T3() == ID_SEL_SIDE )
					{
						// select copper area side
						m_Doc->m_nlist->SelectAreaSide( m_sel_net, sid.I2(), sid.I3() );
						SetCursorMode( CUR_AREA_SIDE_SELECTED );
						Invalidate( FALSE );
					}
					else if( sid.T2()  == ID_AREA && sid.T3() == ID_SEL_CORNER )
					{
						// select copper area corner
						m_Doc->m_nlist->SelectAreaCorner( m_sel_net, sid.I2(), sid.I3() );
						SetCursorMode( CUR_AREA_CORNER_SELECTED );
						Invalidate( FALSE );
					}
					else
						ASSERT(0);
				}
				else if( sid.T1() == ID_TEXT )
				{
					CancelSelection();
					m_sel_text = (CText*)ptr;
					m_sel_id = sid;
					m_Doc->m_tlist->HighlightText( m_sel_text );
					SetCursorMode( CUR_TEXT_SELECTED );
					Invalidate( FALSE );
				}
#endif
			}
			else
			{
				// nothing selected
				CancelSelection();
				m_sel_id.Clear();
				Invalidate( FALSE );
			}
		}
		else if( m_cursor_mode == CUR_DRAG_PART )
		{
			// complete move
			SetCursorMode( CUR_PART_SELECTED );
			CPoint p = m_dlist->WindowToPCB( point );
			m_Doc->m_plist->StopDragging();
			int old_angle = m_Doc->m_plist->GetAngle( m_sel_part );
			int angle = old_angle + m_dlist->GetDragAngle();
			angle = angle % 360;
			int old_side = m_sel_part->side;
			int side = old_side + m_dlist->GetDragSide();
			if( side > 1 )
				side = side - 2;

			// save undo info for part and attached nets
			if( !m_dragging_new_item )
				SaveUndoInfoForPartAndNets( m_sel_part,
				CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
			m_dragging_new_item = FALSE;

			// now move it
			m_sel_part->glued = 0;
			m_Doc->m_plist->Move( m_sel_part, m_last_cursor_point.x, m_last_cursor_point.y,
				angle, side );
			m_Doc->m_plist->HighlightPart( m_sel_part );
			m_Doc->m_nlist->PartMoved( m_sel_part );
			if( m_Doc->m_vis[LAY_RAT_LINE] )
				m_Doc->m_nlist->OptimizeConnections( m_sel_part, m_Doc->m_auto_ratline_disable, 
										m_Doc->m_auto_ratline_min_pins );
			SetFKText( m_cursor_mode );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_GROUP || m_cursor_mode == CUR_DRAG_GROUP_ADD )
		{
			// complete move
			m_Doc->m_dlist->StopDragging();
			if( m_cursor_mode == CUR_DRAG_GROUP )
				SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, &m_sel_ptrs, &m_sel_ids, m_Doc->m_undo_list );
			MoveGroup( m_last_cursor_point.x - m_from_pt.x, m_last_cursor_point.y - m_from_pt.y );
			m_dlist->SetLayerVisible( LAY_HILITE, TRUE );
			HighlightGroup();
			if(m_cursor_mode == CUR_DRAG_GROUP_ADD)
				FindGroupCenter();
			SetCursorMode( CUR_GROUP_SELECTED );
			m_dlist->SetLayerVisible( LAY_RAT_LINE, m_Doc->m_vis[LAY_RAT_LINE] );
			if( m_Doc->m_vis[LAY_RAT_LINE] )
				m_Doc->m_nlist->OptimizeConnections( m_Doc->m_auto_ratline_disable, 
										m_Doc->m_auto_ratline_min_pins );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_MOVE_ORIGIN )
		{
			// complete move
			SetCursorMode( CUR_NONE_SELECTED );
			CPoint p = m_dlist->WindowToPCB( point );
			m_Doc->m_dlist->StopDragging();
			SaveUndoInfoForMoveOrigin( -m_last_cursor_point.x, -m_last_cursor_point.y, m_Doc->m_undo_list );
			MoveOrigin( -m_last_cursor_point.x, -m_last_cursor_point.y );
			OnViewAllElements();
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_REF )
		{
			// complete move
			SetCursorMode( CUR_REF_SELECTED );
			CPoint p = m_dlist->WindowToPCB( point );
			m_Doc->m_plist->StopDragging();
			int drag_angle = m_dlist->GetDragAngle();
			// if part on bottom of board, drag angle is CCW instead of CW
			if( m_Doc->m_plist->GetSide( m_sel_part ) && drag_angle )
				drag_angle = 360 - drag_angle;
			int angle = m_Doc->m_plist->GetRefAngle( m_sel_part ) + drag_angle;
			if( angle>270 )
				angle = angle - 360;
			// save undo info
			SaveUndoInfoForPart( m_sel_part,
				CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
			// now move it
			m_Doc->m_plist->MoveRefText( m_sel_part, m_last_cursor_point.x, m_last_cursor_point.y,
				angle, m_sel_part->m_ref_size, m_sel_part->m_ref_w );
			m_Doc->m_plist->SelectRefText( m_sel_part );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_VALUE )
		{
			// complete move
			SetCursorMode( CUR_VALUE_SELECTED );
			CPoint p = m_dlist->WindowToPCB( point );
			m_Doc->m_plist->StopDragging();
			int drag_angle = m_dlist->GetDragAngle();
			// if part on bottom of board, drag angle is CCW instead of CW
			if( m_Doc->m_plist->GetSide( m_sel_part ) && drag_angle )
				drag_angle = 360 - drag_angle;
			int angle = m_Doc->m_plist->GetValueAngle( m_sel_part ) + drag_angle;
			if( angle>270 )
				angle = angle - 360;
			// save undo info
			SaveUndoInfoForPart( m_sel_part,
				CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
			// now move it
			m_Doc->m_plist->MoveValueText( m_sel_part, m_last_cursor_point.x, m_last_cursor_point.y,
				angle, m_sel_part->m_value_size, m_sel_part->m_value_w );
			m_Doc->m_plist->SelectValueText( m_sel_part );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_RAT )
		{
			// routing a ratline, add segment(s)
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
//			m_dlist->StopDragging();
			// get trace widths
			int w = m_Doc->m_trace_w;
			int via_w = m_Doc->m_via_w;
			int via_hole_w = m_Doc->m_via_hole_w;
			GetWidthsForSegment( &w, &via_w, &via_hole_w );
			cconnect * c = m_sel_net->ConByIndex(m_sel_ic);
			// test for termination of trace
			if( c->EndPin() == NULL && m_sel_is == c->NumSegs()-1 && m_dir == 0
				&& c->VtxByIndex(c->NumSegs()).tee_ID )
			{
				// routing branch to tee-vertex, test for hit on tee-vertex
				cnet * hit_net;
				int hit_ic, hit_iv;
				BOOL bHit = m_Doc->m_nlist->TestForHitOnVertex( m_sel_net, 0,
					m_last_cursor_point.x, m_last_cursor_point.y,
					&hit_net, &hit_ic, &hit_iv );
				if( bHit && hit_net == m_sel_net )
				{
					int tee_ic, tee_iv;
					BOOL bTeeFound = m_Doc->m_nlist->FindTeeVertexInNet( m_sel_net, c->VtxByIndex(c->NumSegs()).tee_ID,
						&tee_ic, &tee_iv );
					if( bTeeFound && tee_ic == hit_ic && tee_iv == hit_iv )
					{
						// now route to tee-vertex
						SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
						CPoint pi = m_snap_angle_ref;
						CPoint pf = m_last_cursor_point;
						CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
						BOOL insert_flag = FALSE;
						if( pp != pi )
						{
							insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
								pp.x, pp.y, m_active_layer, w, via_w, via_hole_w, m_dir );
							if( !insert_flag )
							{
								// hit end-vertex of segment, terminate routing
								goto cancel_selection_and_goodbye;
							}
							if( m_dir == 0 )
								m_sel_id.SetI3( m_sel_id.I3() + 1 );
						}
						insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
							m_last_cursor_point.x, m_last_cursor_point.y,
							m_active_layer, w, via_w, via_hole_w, m_dir );
						if( !insert_flag )
						{
							// hit end-vertex of segment, terminate routing
							goto cancel_selection_and_goodbye;
						}
						if( m_dir == 0 )
							m_sel_id.SetI3( m_sel_id.I3() + 1 );
						// finish trace if necessary
						m_Doc->m_nlist->RouteSegment( m_sel_net, m_sel_ic, m_sel_is,
							m_active_layer, w );
						m_Doc->m_nlist->ReconcileVia( m_sel_net, tee_ic, tee_iv );
						goto cancel_selection_and_goodbye;
					}
				}
			}
			else if( m_dir == 0 && c->VtxByIndex(m_sel_is+1).tee_ID || m_dir == 1 && c->VtxByIndex(m_sel_is).tee_ID )
			{
				// routing ratline to tee-vertex
				int tee_iv = m_sel_is + 1 - m_dir;
				cnet * hit_net;
				int hit_ic, hit_iv;
				BOOL bHit = m_Doc->m_nlist->TestForHitOnVertex( m_sel_net, 0,
					m_last_cursor_point.x, m_last_cursor_point.y,
					&hit_net, &hit_ic, &hit_iv );
				if( bHit && hit_net == m_sel_net && hit_ic == m_sel_ic && hit_iv == tee_iv )
				{
					// now route to tee-vertex
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					CPoint pi = m_snap_angle_ref;
					CPoint pf = m_last_cursor_point;
					CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
					BOOL insert_flag = FALSE;
					if( pp != pi )
					{
						insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
							pp.x, pp.y, m_active_layer, w, via_w, via_hole_w, m_dir );
						if( !insert_flag )
						{
							// hit end-vertex of segment, terminate routing
							goto cancel_selection_and_goodbye;
						}
						if( m_dir == 0 )
							m_sel_id.SetI3( m_sel_id.I3() + 1 );
					}
					insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
						m_last_cursor_point.x, m_last_cursor_point.y,
						m_active_layer, w, via_w, via_hole_w, m_dir );
					if( !insert_flag )
					{
						// hit end-vertex of segment, terminate routing
						goto cancel_selection_and_goodbye;
					}
					if( m_dir == 0 )
						m_sel_id.SetI3( m_sel_id.I3() + 1 );
					// finish trace if necessary
					m_Doc->m_nlist->RouteSegment( m_sel_net, m_sel_ic, m_sel_is,
						m_active_layer, w );
					goto cancel_selection_and_goodbye;
				}
			}
			else if( m_sel_is == 0 && m_dir == 1 || m_sel_is == c->NumSegs()-1 && m_dir == 0 )
			{
				// routing ratline at end of trace, test for hit on any pad in net
				int ip = m_Doc->m_nlist->TestHitOnAnyPadInNet( m_last_cursor_point.x,
					m_last_cursor_point.y,
					m_active_layer, m_sel_net );
				int ns = m_sel_con->NumSegs();
				if( ip != -1 )
				{
					// hit on pad in net, see if this is our starting pad
					if( ns < 3 && (m_dir == 0 && ip == m_sel_con->start_pin
						|| m_dir == 1 && ip == m_sel_con->end_pin) )
					{
						// starting pin with too few segments, don't route to pin
					}
					else
					{
						// route to pin
						// see if this is our destination
						if( !(m_dir == 0 && ip == m_sel_con->end_pin
							|| m_dir == 1 && ip == m_sel_con->start_pin) )
						{
							// no, change connection to this pin unless it is the starting pin
							cpart * hit_part = m_sel_net->pin[ip].part;
							CString * hit_pin_name = &m_sel_net->pin[ip].pin_name;
							m_Doc->m_nlist->ChangeConnectionPin( m_sel_net, m_sel_ic, 1-m_dir, hit_part, hit_pin_name );
						}
						// now route to destination pin
						SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
						CPoint pi = m_snap_angle_ref;
						CPoint pf = m_last_cursor_point;
						CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
						BOOL insert_flag = FALSE;
						if( pp != pi )
						{
							insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
								pp.x, pp.y, m_active_layer, w, via_w, via_hole_w, m_dir );
							if( !insert_flag )
							{
								// hit end-vertex of segment, terminate routing
								goto cancel_selection_and_goodbye;
							}
							if( m_dir == 0 )
								m_sel_id.SetI3( m_sel_id.I3() + 1 );
						}
						insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
							m_last_cursor_point.x, m_last_cursor_point.y,
							m_active_layer, w, via_w, via_hole_w, m_dir );
						if( !insert_flag )
						{
							// hit end-vertex of segment, terminate routing
							goto cancel_selection_and_goodbye;
						}
						if( m_dir == 0 )
							m_sel_id.SetI3( m_sel_id.I3() + 1 );
						// finish trace to pad if necessary
						m_Doc->m_nlist->RouteSegment( m_sel_net, m_sel_ic, m_sel_is,
							m_active_layer, w );
						goto cancel_selection_and_goodbye;
					}
				}
			}
			// trace was not terminated, insert segment and continue routing
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			CPoint pi = m_snap_angle_ref;
			CPoint pf = m_last_cursor_point;
			CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
			BOOL insert_flag = FALSE;
			if( pp != pi )
			{
				insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
					pp.x, pp.y, m_active_layer, w, via_w, via_hole_w, m_dir );
				if( !insert_flag )
				{
					// hit end-vertex of segment, terminate routing
					goto cancel_selection_and_goodbye;
				}
				if( m_dir == 0 )
					m_sel_id.SetI3( m_sel_id.I3() + 1 );
			}
			insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
				m_last_cursor_point.x, m_last_cursor_point.y,
				m_active_layer, w, via_w, via_hole_w, m_dir );
			if( !insert_flag )
			{
				// hit end-vertex of segment, terminate routing
				goto cancel_selection_and_goodbye;
			}
			if( m_dir == 0 )
				m_sel_id.SetI3( m_sel_id.I3() + 1 );
			m_Doc->m_nlist->StartDraggingSegment( pDC, m_sel_net,
				m_sel_id.I2(), m_sel_is,
				m_last_cursor_point.x, m_last_cursor_point.y, m_active_layer,
				LAY_SELECTION, w,
				m_active_layer, via_w, via_hole_w, m_dir, 2 );
			m_snap_angle_ref = m_last_cursor_point;
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_VTX_INSERT )
		{
			// add trace segment and vertex
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			m_dlist->StopDragging();

			// make undo record
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			int layer = m_sel_seg->layer;
			int w = m_sel_seg->width;
			int insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
				m_last_cursor_point.x, m_last_cursor_point.y,
				layer, w, 0, 0, m_dir );
			CancelSelection();
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_ADD_BOARD )
		{
			// place first corner of board outline
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			// make new board outline
			int ib = m_Doc->m_board_outline.GetSize();
			m_Doc->m_board_outline.SetSize( ib+1 );
			m_Doc->m_board_outline[ib].SetDisplayList( m_dlist );
			m_sel_id.SetI2( ib );
			m_Doc->m_board_outline[ib].Start( LAY_BOARD_OUTLINE, 1, 20*NM_PER_MIL, p.x, p.y,
				0, &m_sel_id, NULL );
			m_sel_id.SetT3( ID_SEL_CORNER );
			m_sel_id.SetI3( 0 );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			SetCursorMode( CUR_DRAG_BOARD_1 );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_BOARD_1 )
		{
			// place second corner of board outline
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_Doc->m_board_outline[m_sel_id.I2()].AppendCorner( p.x, p.y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.SetI3( m_sel_id.I3() + 1 );
			SetCursorMode( CUR_DRAG_BOARD );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_BOARD )
		{
			// place subsequent corners of board outline
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			if( p.x == m_Doc->m_board_outline[m_sel_id.I2()].X(0)
				&& p.y == m_Doc->m_board_outline[m_sel_id.I2()].Y(0) )
			{
				// this point is the start point, close the polyline and quit
				SaveUndoInfoForBoardOutlines( TRUE, m_Doc->m_undo_list );
				m_Doc->m_board_outline[m_sel_id.I2()].Close( m_polyline_style );
				SetCursorMode( CUR_NONE_SELECTED );
				m_Doc->m_dlist->StopDragging();
			}
			else
			{
				// add corner to polyline
				m_Doc->m_board_outline[m_sel_id.I2()].AppendCorner( p.x, p.y, m_polyline_style );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				m_sel_id.SetI3( m_sel_id.I3() + 1 );
				m_snap_angle_ref = m_last_cursor_point;
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_BOARD_INSERT )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			SaveUndoInfoForBoardOutlines( TRUE, m_Doc->m_undo_list );
			m_Doc->m_board_outline[m_sel_id.I2()].InsertCorner( m_sel_id.I3()+1, p.x, p.y );
			m_Doc->m_board_outline[m_sel_id.I2()].HighlightCorner( m_sel_id.I3()+1 );
			SetCursorMode( CUR_BOARD_CORNER_SELECTED );
			m_sel_id.Set( ID_BOARD, -1, ID_OUTLINE, -1, m_sel_id.I2(), ID_SEL_CORNER, -1, m_sel_id.I3()+1 );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_BOARD_MOVE )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			SaveUndoInfoForBoardOutlines( TRUE, m_Doc->m_undo_list );
			m_Doc->m_board_outline[m_sel_id.I2()].MoveCorner( m_sel_id.I3(), p.x, p.y );
			m_Doc->m_board_outline[m_sel_id.I2()].HighlightCorner( m_sel_id.I3() );
			SetCursorMode( CUR_BOARD_CORNER_SELECTED );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_ADD_AREA )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			int iarea = m_Doc->m_nlist->AddArea( m_sel_net, m_active_layer, p.x, p.y, m_polyline_hatch );
			m_sel_id.Set( m_sel_net->m_id.T1(), -1, ID_AREA, -1, iarea, ID_SEL_CORNER, -1, 1 );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			SetCursorMode( CUR_DRAG_AREA_1 );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_AREA_1 )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.SetI3( 2 );
			SetCursorMode( CUR_DRAG_AREA );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_AREA )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			if( p.x == m_sel_net->area[m_sel_id.I2()].X(0)
				&& p.y == m_sel_net->area[m_sel_id.I2()].Y(0) )
			{
				// cursor point is first point, close area
				SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
				m_Doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
				m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, TRUE, TRUE );
				m_Doc->m_dlist->StopDragging();
				if( m_Doc->m_vis[LAY_RAT_LINE] )
					m_Doc->m_nlist->OptimizeConnections( m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE );
				CancelSelection();
			}
			else
			{
				// add cursor point
				m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				m_sel_id.SetI3( m_sel_id.I3() + 1 );
				SetCursorMode( CUR_DRAG_AREA );
				m_snap_angle_ref = m_last_cursor_point;
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_AREA_MOVE )
		{
			SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			m_Doc->m_nlist->MoveAreaCorner( m_sel_net, m_sel_ia, m_sel_is, p.x, p.y );
			int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, TRUE );
			if( ret == -1 )
			{
				// error
				AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
				CancelSelection();
				m_Doc->OnEditUndo();
			}
			else
			{
				TryToReselectAreaCorner( p.x, p.y );
				if( m_Doc->m_vis[LAY_RAT_LINE] )
					m_Doc->m_nlist->OptimizeConnections( m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE );
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_AREA_INSERT )
		{
			SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			m_Doc->m_nlist->InsertAreaCorner( m_sel_net, m_sel_ia, m_sel_is+1, p.x, p.y, CPolyLine::STRAIGHT );
			int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, TRUE );
			if( ret == -1 )
			{
				// error
				AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
				CancelSelection();
				m_Doc->OnEditUndo();
			}
			else
			{
				TryToReselectAreaCorner( p.x, p.y );
				if( m_Doc->m_vis[LAY_RAT_LINE] )
					m_Doc->m_nlist->OptimizeConnections( m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE );
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_ADD_AREA_CUTOUT )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			int ia = m_sel_id.I2();
			carea * a = &m_sel_net->area[ia];
			m_Doc->m_nlist->AppendAreaCorner( m_sel_net, ia, p.x, p.y, m_polyline_style );
			m_sel_id.Set( m_sel_net->m_id.T1(), -1, ID_AREA, -1, ia, ID_SEL_CORNER, -1, a->NumCorners()-1 );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			SetCursorMode( CUR_DRAG_AREA_CUTOUT_1 );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1 )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.SetI3( 2 );
			SetCursorMode( CUR_DRAG_AREA_CUTOUT );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			CPolyLine * poly = &m_sel_net->area[m_sel_id.I2()];
			int icontour = poly->Contour( poly->NumCorners()-1 );
			int istart = poly->ContourStart( icontour );
			if( p.x == poly->X(istart)
				&& p.y == poly->Y(istart) )
			{
				// cursor point is first point, close area
				SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
				m_Doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
				m_Doc->m_dlist->StopDragging();
				int n_old_areas = m_sel_net->area.GetSize();
				int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, FALSE );
				if( ret == -1 )
				{
					// error
					AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
					m_Doc->OnEditUndo();
				}
				else
				{
					if( m_Doc->m_vis[LAY_RAT_LINE] )
						m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
				}
				CancelSelection();
			}
			else
			{
				// add cursor point
				m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				m_sel_id.SetI3( m_sel_id.I3() + 1 );
				SetCursorMode( CUR_DRAG_AREA_CUTOUT );
				m_snap_angle_ref = m_last_cursor_point;
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_ADD_SMCUTOUT )
		{
			// add poly for new cutout
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			int ism = m_Doc->m_sm_cutout.GetSize();
			m_Doc->m_sm_cutout.SetSize( ism + 1 );
			CPolyLine * p_sm = &m_Doc->m_sm_cutout[ism];
			p_sm->SetDisplayList( m_Doc->m_dlist );
			id id_sm( ID_MASK, -1, ID_MASK, -1, ism );
			m_sel_id = id_sm;
			p_sm->Start( m_polyline_layer, 0, 10*NM_PER_MIL, p.x, p.y, m_polyline_hatch, &m_sel_id, NULL );
			m_sel_id.SetT3( ID_SEL_CORNER );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.SetI3( 1 );
			SetCursorMode( CUR_DRAG_SMCUTOUT_1 );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_1 )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			CPolyLine * p_sm = &m_Doc->m_sm_cutout[m_sel_id.I2()];
			p_sm->AppendCorner( p.x, p.y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.SetI3( 2 );
			SetCursorMode( CUR_DRAG_SMCUTOUT );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_SMCUTOUT )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			CPolyLine * p_sm = &m_Doc->m_sm_cutout[m_sel_id.I2()];
			if( p.x == p_sm->X(0)
				&& p.y == p_sm->Y(0) )
			{
				// cursor point is first point, close area
				SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
				p_sm->Close( m_polyline_style );
				SetCursorMode( CUR_NONE_SELECTED );
				m_Doc->m_dlist->StopDragging();
			}
			else
			{
				// add cursor point
				p_sm->AppendCorner( p.x, p.y, m_polyline_style );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				m_sel_id.SetI3( m_sel_id.I3() + 1 );
				SetCursorMode( CUR_DRAG_SMCUTOUT );
				m_snap_angle_ref = m_last_cursor_point;
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_MOVE )
		{
			SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.I2()];
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			poly->MoveCorner( m_sel_id.I3(), p.x, p.y );
			poly->HighlightCorner( m_sel_id.I3() );
			SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_INSERT )
		{
			SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.I2()];
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			poly->InsertCorner( m_sel_id.I3()+1, p.x, p.y );
			poly->HighlightCorner( m_sel_id.I3()+1 );
			m_sel_id.Set( ID_MASK, -1, ID_MASK, -1, m_sel_id.I2(), ID_SEL_CORNER, -1, m_sel_id.I3()+1 );
			SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_ADD_AREA_CUTOUT )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			int ia = m_sel_id.I2();
			carea * a = &m_sel_net->area[ia];
			m_Doc->m_nlist->AppendAreaCorner( m_sel_net, ia, p.x, p.y, m_polyline_style );
			m_sel_id.Set( m_sel_net->m_id.T1(), -1, ID_AREA, -1, ia, ID_SEL_CORNER, -1, a->NumCorners()-1 );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			SetCursorMode( CUR_DRAG_AREA_1 );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_VTX )
		{
			// move vertex by modifying adjacent segments and reconciling via
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			CPoint p;
			p = m_last_cursor_point;
			int ic = m_sel_id.I2();
			int ivtx = m_sel_id.I3();
			m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is, p.x, p.y );
			m_Doc->m_nlist->HighlightVertex( m_sel_net, ic, ivtx );
			if( m_Doc->m_vis[LAY_RAT_LINE] )
			{
				m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
				if( m_sel_id.Resolve() )
					SelectItem( m_sel_id );
				else
					CancelSelection();
			}
			else
				SetCursorMode( CUR_VTX_SELECTED );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_MOVE_SEGMENT )
		{
			// move vertex by modifying adjacent segments and reconciling via
			m_Doc->m_dlist->StopDragging();
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			CPoint cpi;
			CPoint cpf;
			m_Doc->m_dlist->Get_Endpoints(&cpi, &cpf);
			int ic = m_sel_id.I2();
			int ivtx = m_sel_id.I3();
			m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,   cpi.x, cpi.y );
			ASSERT(cpi != cpf);			// Should be at least one grid snap apart.
			m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is+1, cpf.x, cpf.y );
			SetCursorMode( CUR_NONE_SELECTED );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_END_VTX )
		{
			// move end-vertex of stub trace
			m_Doc->m_dlist->StopDragging();
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			CPoint p;
			p = m_last_cursor_point;
			int ic = m_sel_id.I2();
			int ivtx = m_sel_id.I3();
			m_Doc->m_nlist->MoveVertex( m_sel_net, ic, ivtx, p.x, p.y );
			SetCursorMode( CUR_END_VTX_SELECTED );
			if( m_Doc->m_vis[LAY_RAT_LINE] )
			{
				m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
				if( m_sel_id.Resolve() )
					SelectItem( m_sel_id );
				else
					CancelSelection();
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_CONNECT )
		{
			// dragging ratline to make a new connection
			// test for hit on pin
			CPoint p = m_dlist->WindowToPCB( point );
			id sel_id;	// id of selected item
			id pad_id( ID_PART, -1, ID_SEL_PAD );	// force selection of pad
			int idx;
			int num_hits;
			CDL_job::HitInfo hit_info[MAX_HITS];

			idx = m_dlist->TestSelect(
				p.x, p.y,					  // Point
				hit_info, MAX_HITS, num_hits, // Hit Information
				&m_sel_id, NULL,			  // Exclusions
				&pad_id, 1					  // Inclusions
			);

			if( num_hits )
			{
				sel_id = hit_info[idx].ID;		// best hit
				if( sel_id.T1() == ID_PART && sel_id.T2()  == ID_SEL_PAD )
				{
					// hit on pin
					void * ptr = m_Doc->m_plist->GetPartByUID( sel_id.U1() );
					ASSERT( ptr );
					cpart * new_sel_part = (cpart*)ptr;
					cnet * new_sel_net = (cnet*)new_sel_part->pin[sel_id.I2()].net;
					if( m_sel_id.T1() == ID_NET  && m_sel_id.T2()  == ID_CONNECT
						&& m_sel_id.T3() == ID_SEL_VERTEX )
					{
						// dragging ratline from vertex of a trace
						if( new_sel_net && new_sel_net != m_sel_net )
						{
							// pin assigned to different net, can't connect it
							CString mess;
							mess.Format( "You are trying to connect a trace to a pin on a different net\nYou must detach the pin from the net first" );
							AfxMessageBox( mess );
						}
						else if( m_sel_con->end_pin == cconnect::NO_END
							&& m_sel_iv == m_sel_con->NumSegs() )
						{
							// dragging ratline from end of stub trace
							CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.I2() );
							CPoint p = m_Doc->m_plist->GetPinPoint( new_sel_part, pin_name );
							int pin = m_Doc->m_nlist->GetNetPinIndex( m_sel_net, &new_sel_part->ref_des, &pin_name );
							if( pin == m_sel_con->start_pin )
							{
								// trying to connect pin to itself
								goto goodbye;
							}
							// convert to regular pin-pin trace
							if(  m_sel_vtx->tee_ID )
								ASSERT(0);	// error, should not be a branch end or a tee-vertex
							SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
							SaveUndoInfoForPart( new_sel_part,
								CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_Doc->m_undo_list );
							if( new_sel_net == 0 )
							{
								m_sel_net->AddPin( &new_sel_part->ref_des,
									&pin_name );
							}
							m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic, p.x, p.y,
								LAY_RAT_LINE, 0 );
							m_sel_con->Undraw();
							m_sel_vtx->force_via_flag = 0;
							m_sel_con->end_pin = pin;
							cvertex * end_v = &m_sel_con->VtxByIndex(m_sel_con->NumSegs());
							end_v->pad_layer = m_Doc->m_plist->GetPinLayer( new_sel_part, sel_id.I2() );
							m_sel_con->Draw();
							if( m_Doc->m_vis[LAY_RAT_LINE] )
								m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
							CancelSelection();
							m_Doc->m_dlist->StopDragging();
							Invalidate( FALSE );
							m_Doc->ProjectModified( TRUE );
						}
						else
						{
							// dragging ratline from any other vertex to pin
							// add new stub trace from pin to tee
							SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
							SaveUndoInfoForPart( new_sel_part,
								CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_Doc->m_undo_list );
							CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.I2() );
							if( new_sel_net == 0 )
							{
								m_sel_net->AddPin( &new_sel_part->ref_des,
									&pin_name );
							}
							int p1 = m_Doc->m_nlist->GetNetPinIndex( m_sel_net, &new_sel_part->ref_des, &pin_name );
							m_sel_con->Undraw();
							cconnect * new_con = m_sel_net->AddConnectionFromVertexToPin( m_sel_id, p1 );
							m_sel_con->Draw();
							new_con->Draw();
							CancelSelection();
							m_Doc->m_dlist->StopDragging();
							Invalidate( FALSE );
							m_Doc->ProjectModified( TRUE );
						}
					}
					else if( m_sel_id.T1() == ID_PART  && m_sel_id.T2()  == ID_SEL_PAD )
					{
						// connecting pin to pin
						cnet * from_sel_net = (cnet*)m_sel_part->pin[m_sel_id.I2()].net;
						if( new_sel_net && from_sel_net && (new_sel_net != from_sel_net) )
						{
							// pins assigned to different nets, can't connect them
							CString mess;
							mess.Format( "You are trying to connect pins on different nets\nYou must detach one of them first" );
							AfxMessageBox( mess );
							m_Doc->m_dlist->StopDragging();
							SetCursorMode( CUR_PAD_SELECTED );
						}
						else
						{
							// see if we are trying to connect a pin to itself
							if( new_sel_part == m_sel_part && m_sel_id.I2() == sel_id.I2() )
							{
								// yes, forget it
								goto goodbye;
							}
							// we can connect these pins
							SaveUndoInfoForPart( m_sel_part,
								CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
							SaveUndoInfoForPart( new_sel_part,
								CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_Doc->m_undo_list );
							if( new_sel_net != from_sel_net )
							{
								// one pin is unassigned, assign it to net
								if( !new_sel_net )
								{
									// connecting to unassigned pin, assign it
									SaveUndoInfoForNetAndConnections( from_sel_net, CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
									CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.I2() );
									from_sel_net->AddPin( &new_sel_part->ref_des, &pin_name );
									new_sel_net = from_sel_net;
								}
								else if( !from_sel_net )
								{
									// connecting from unassigned pin, assign it
									SaveUndoInfoForNetAndConnections( new_sel_net, CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
									CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.I2() );
									new_sel_net->AddPin( &m_sel_part->ref_des, &pin_name );
									from_sel_net = new_sel_net;
								}
								else
									ASSERT(0);
							}
							else if( !new_sel_net && !m_sel_part->pin[m_sel_id.I2()].net )
							{
								// connecting 2 unassigned pins, select net
								DlgAssignNet assign_net_dlg;
								assign_net_dlg.m_map = &m_Doc->m_nlist->m_map;
								int ret = assign_net_dlg.DoModal();
								if( ret != IDOK )
								{
									m_Doc->m_dlist->StopDragging();
									SetCursorMode( CUR_PAD_SELECTED );
									goto goodbye;
								}
								CString name = assign_net_dlg.m_net_str;
								void * ptr;
								int test = m_Doc->m_nlist->m_map.Lookup( name, ptr );
								if( test )
								{
									// assign pins to existing net
									new_sel_net = (cnet*)ptr;
									SaveUndoInfoForNetAndConnections( new_sel_net,
										CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
									CString pin_name1 = m_sel_part->shape->GetPinNameByIndex( m_sel_id.I2() );
									CString pin_name2 = new_sel_part->shape->GetPinNameByIndex( sel_id.I2() );
									new_sel_net->AddPin( &m_sel_part->ref_des, &pin_name1 );
									new_sel_net->AddPin( &new_sel_part->ref_des, &pin_name2 );
								}
								else
								{
									// make new net
									new_sel_net = m_Doc->m_nlist->AddNet( (char*)(LPCTSTR)name, 10, 0, 0, 0 );
									SaveUndoInfoForNetAndConnections( new_sel_net,
										CNetList::UNDO_NET_ADD, FALSE, m_Doc->m_undo_list );
									CString pin_name1 = m_sel_part->shape->GetPinNameByIndex( m_sel_id.I2() );
									CString pin_name2 = new_sel_part->shape->GetPinNameByIndex( sel_id.I2() );
									new_sel_net->AddPin( &m_sel_part->ref_des, &pin_name1 );
									new_sel_net->AddPin( &new_sel_part->ref_des, &pin_name2 );
								}
							}
							// find pins in net and connect them
							int p1 = -1;
							int p2 = -1;
							for( int ip=0; ip<new_sel_net->NumPins(); ip++ )
							{
								CString pin_name = new_sel_net->pin[ip].pin_name;
								if( new_sel_net->pin[ip].part == m_sel_part )
								{
									int pin_index = m_sel_part->shape->GetPinIndexByName( pin_name );
									if( pin_index == m_sel_id.I2() )
									{
										// found starting pin in net
										p1 = ip;
									}
								}
								if( new_sel_net->pin[ip].part == new_sel_part )
								{
									int pin_index = new_sel_part->shape->GetPinIndexByName( pin_name );
									if( pin_index == sel_id.I2() )
									{
										// found ending pin in net
										p2 = ip;
									}
								}
							}
							if( p1>=0 && p2>=0 )
								m_sel_net->AddConnectFromPinToPin( p1, p2 );
							else
								ASSERT(0);	// couldn't find pins in net
							m_dlist->StopDragging();
							SetCursorMode( CUR_PAD_SELECTED );
						}
						m_Doc->ProjectModified( TRUE );
						Invalidate( FALSE );
					}
				}
			}
		}
		else if( m_cursor_mode == CUR_DRAG_RAT_PIN )
		{
			// see if pad selected
			CPoint p = m_dlist->WindowToPCB( point );
			id sel_id;	// id of selected item
			id pad_id( ID_PART, -1, ID_SEL_PAD );	// force selection of pad
//100			void * ptr = m_dlist->TestSelect( p.x, p.y, &sel_id, &m_sel_layer, NULL, &pad_id );
			void * ptr = NULL;
			sel_id.Clear();
			if( ptr )
			{
				if( sel_id.T1() == ID_PART )
				{
					if( sel_id.T2()  == ID_SEL_PAD )
					{
						// see if we can connect to this pin
						cpart * new_sel_part = (cpart*)ptr;
						cnet * new_sel_net = (cnet*)new_sel_part->pin[sel_id.I2()].net;
						CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.I2() );

						if( new_sel_net && (new_sel_net != m_sel_net) )
						{
							// pin assigned to different net, can't connect it
							CString mess;
							mess.Format( "You are trying to connect to a pin on a different net" );
							AfxMessageBox( mess );
							return;
						}
						else if( new_sel_net == 0 )
						{
							// unassigned pin, assign it
							SaveUndoInfoForPart( new_sel_part,
								CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
							SaveUndoInfoForNetAndConnections( m_sel_net,
								CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
							new_sel_net->AddPin( &new_sel_part->ref_des, &pin_name );
						}
						else
						{
							// pin already assigned to this net
							SaveUndoInfoForNetAndConnections( m_sel_net,
								CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
						}
						m_Doc->m_nlist->ChangeConnectionPin( m_sel_net, m_sel_ic, m_sel_is,
							new_sel_part, &pin_name );
						m_dlist->Set_visible( m_sel_seg->dl_el, TRUE );
						m_dlist->StopDragging();
						m_dlist->CancelHighLight();
						SetCursorMode( CUR_RAT_SELECTED );
						m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
						m_Doc->m_nlist->SetAreaConnections( m_sel_net );
						m_Doc->ProjectModified( TRUE );
						Invalidate( FALSE );
					}
				}
			}
		}
		else if( m_cursor_mode == CUR_DRAG_STUB )
		{
			if( m_sel_is != 0 )
			{
				// if first vertex, we have already saved
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			}
			int w = m_Doc->m_trace_w;
			int via_w = m_Doc->m_via_w;
			int via_hole_w = m_Doc->m_via_hole_w;
			GetWidthsForSegment( &w, &via_w, &via_hole_w );
			// see if cursor on pad
			CPoint p = m_dlist->WindowToPCB( point );
			id sel_id;	// id of selected item
			id pad_id( ID_PART, -1, ID_SEL_PAD );	// test for hit on any pad
//100			void * ptr = m_dlist->TestSelect( p.x, p.y, &sel_id, &m_sel_layer, NULL, &pad_id );
			void * ptr = NULL;
			sel_id.Clear();
//100

			if( ptr && sel_id.T1() == ID_PART && sel_id.T2()  == ID_SEL_PAD )
			{
				// see if we can connect to this pin
				cpart * new_sel_part = (cpart*)ptr;
				cnet * new_sel_net = (cnet*)new_sel_part->pin[sel_id.I2()].net;
				CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.I2() );
				if( m_Doc->m_plist->TestHitOnPad( new_sel_part, &pin_name, p.x, p.y, m_active_layer ) )
				{
					// check for starting pad of stub trace
					cpart * origin_part = m_sel_con_start_pin->part;
					CString * origin_pin_name = &m_sel_con_start_pin->pin_name;
					if( origin_part != new_sel_part || *origin_pin_name != *pin_name )
					{
						// not starting pad
						if( new_sel_net && (new_sel_net != m_sel_net) )
						{
							// pin assigned to different net, can't connect it
							CString mess;
							mess.Format( "You are trying to connect to a pin on a different net" );
							AfxMessageBox( mess );
							return;
						}
						else if( new_sel_net == 0 )
						{
							// unassigned pin, assign it
							SaveUndoInfoForPart( new_sel_part,
								CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_Doc->m_undo_list );
							new_sel_net->AddPin( &new_sel_part->ref_des, &pin_name );
						}
						else
						{
							// pin already assigned to this net
						}
						CPoint pi = m_snap_angle_ref;
						CPoint pf = m_last_cursor_point;
						CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
						if( pp != pi )
						{
							m_sel_id.SetI3( m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
								pp.x, pp.y, m_active_layer, w ) );
						}
						m_sel_id.SetI3( m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
							m_last_cursor_point.x, m_last_cursor_point.y, m_active_layer, w ) );
						CPoint pin_point = m_Doc->m_plist->GetPinPoint( new_sel_part, pin_name );
						if( m_last_cursor_point != pin_point )
						{
							m_sel_id.SetI3( m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
								pin_point.x, pin_point.y, m_active_layer, w ) );
						}
						m_Doc->m_nlist->ChangeConnectionPin( m_sel_net, m_sel_ic, TRUE,
							new_sel_part, &pin_name );
						m_dlist->StopDragging();
						SetCursorMode( CUR_NONE_SELECTED );
						m_Doc->m_nlist->SetAreaConnections( m_sel_net );
						m_Doc->ProjectModified( TRUE );
						Invalidate( FALSE );
						return;
					}
				}
			}
			// test for tee-connection to vertex
			cnet * hit_net = NULL;
			int hit_ic, hit_iv;
			BOOL bHit = m_Doc->m_nlist->TestForHitOnVertex( m_sel_net, m_active_layer,
				m_last_cursor_point.x, m_last_cursor_point.y,
				&hit_net, &hit_ic, &hit_iv );
			if( bHit )
			{
				// hit on vertex
				cconnect * hit_c;
				cvertex * hit_v;
				hit_c = m_sel_net->ConByIndex(hit_ic);
				hit_v = &hit_c->VtxByIndex(hit_iv);
				int ret = AfxMessageBox( "Branch created" );
				// first, see if already a tee
				int id;
				if( hit_v->tee_ID )
				{
					// yes
					id = hit_v->tee_ID;
				}
				else
				{
					// no
					id = m_Doc->m_nlist->GetNewTeeID();
					hit_v->tee_ID = id;
				}
				// now connect it (no via)
				CPoint pi = m_snap_angle_ref;
				CPoint pf = m_last_cursor_point;
				CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
				if( pp != pi )
				{
					m_sel_id.SetI3( m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
						pp.x, pp.y, m_active_layer, w ) );
				}
				m_sel_id.SetI3( m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
					m_last_cursor_point.x, m_last_cursor_point.y, m_active_layer, w ) );
				if( m_last_cursor_point.x != hit_v->x || m_last_cursor_point.y != hit_v->y )
				{
					m_sel_id.SetI3( m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
						hit_v->x, hit_v->y, m_active_layer, w ) );
				}
				// set tee_ID for end-vertex and remove selector
				m_sel_con->VtxByIndex(m_sel_iv+1).tee_ID = id;
				m_Doc->m_nlist->ReconcileVia( m_sel_net, m_sel_ic, m_sel_iv+1 );
				m_dlist->StopDragging();
				if( m_Doc->m_vis[LAY_RAT_LINE] )
					m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
				CancelSelection();
				m_Doc->ProjectModified( TRUE );
				Invalidate( FALSE );
				return;
			}

			// come here if not connecting to anything
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			m_sel_vtx->force_via_flag = 0;
			CPoint pi = m_snap_angle_ref;
			CPoint pf = m_last_cursor_point;
			CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
			if( pp != pi )
			{
				m_sel_id.SetI3( m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
					pp.x, pp.y, m_active_layer, w ) );
			}
			m_sel_id.SetI3( m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic,
				m_last_cursor_point.x, m_last_cursor_point.y, m_active_layer, w ) );
			m_dlist->StopDragging();
			m_sel_id.SetI3( m_sel_id.I3() + 1 );
			m_Doc->m_nlist->StartDraggingStub( pDC, m_sel_net, m_sel_ic, m_sel_is,
				m_last_cursor_point.x, m_last_cursor_point.y, m_active_layer, w, m_active_layer,
				via_w, via_hole_w, 2, m_inflection_mode );
			m_snap_angle_ref = m_last_cursor_point;
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_TEXT )
		{
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			if( !m_dragging_new_item )
				SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_MODIFY, TRUE, m_Doc->m_undo_list );
			int old_angle = m_sel_text->m_angle;
			int angle = old_angle + m_dlist->GetDragAngle();
			if( angle>270 )
				angle = angle - 360;
			int old_mirror = m_sel_text->m_mirror;
			int mirror = (old_mirror + m_dlist->GetDragSide())%2;
			BOOL negative = m_sel_text->m_bNegative;;
			int layer = m_sel_text->m_layer;
			m_Doc->m_tlist->MoveText( m_sel_text, m_last_cursor_point.x, m_last_cursor_point.y,
				angle, mirror, negative, layer );
			if( m_dragging_new_item )
			{
				SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_ADD, TRUE, m_Doc->m_undo_list );
				m_dragging_new_item = FALSE;
			}
			SetCursorMode( CUR_TEXT_SELECTED );
			m_Doc->m_tlist->HighlightText( m_sel_text );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_MEASURE_1 )
		{
			m_from_pt = m_last_cursor_point;
			m_dlist->MakeDragRatlineArray( 1, 1 );
			m_dlist->AddDragRatline( m_from_pt, zero ); 
			SetCursorMode( CUR_DRAG_MEASURE_2 );
		}
		else if( m_cursor_mode == CUR_DRAG_MEASURE_2 )
		{
			m_dlist->StopDragging();
			SetCursorMode( CUR_NONE_SELECTED );
		}
		goto goodbye;

cancel_selection_and_goodbye:
		m_dlist->StopDragging();
		CancelSelection();
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );

goodbye:
		ShowSelectStatus();
	}
	if( pDC )
		ReleaseDC( pDC );
	CView::OnLButtonUp(nFlags, point);
}

// left double-click
//
void CFreePcbView::OnLButtonDblClk(UINT nFlags, CPoint point)
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
void CFreePcbView::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_disable_context_menu = 1;
	if( m_cursor_mode == CUR_DRAG_PART )
	{
		m_Doc->m_plist->CancelDraggingPart( m_sel_part );
		if( m_dragging_new_item )
		{
			CancelSelection();
			m_Doc->OnEditUndo();	// remove the part
		}
		else
		{
			SetCursorMode( CUR_PART_SELECTED );
			m_Doc->m_plist->HighlightPart( m_sel_part );
		}
		m_dragging_new_item = FALSE;
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_REF )
	{
		m_Doc->m_plist->CancelDraggingRefText( m_sel_part );
		SetCursorMode( CUR_REF_SELECTED );
		m_Doc->m_plist->SelectRefText( m_sel_part );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_VALUE )
	{
		m_Doc->m_plist->CancelDraggingValue( m_sel_part );
		SetCursorMode( CUR_VALUE_SELECTED );
		m_Doc->m_plist->SelectValueText( m_sel_part );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_RAT )
	{
		m_Doc->m_nlist->CancelDraggingSegment( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_RAT_SELECTED );
		m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_RAT_PIN )
	{
		m_dlist->StopDragging();
		m_dlist->Set_visible( m_sel_seg->dl_el, TRUE );
		m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_RAT_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_VTX )
	{
		m_Doc->m_nlist->CancelDraggingVertex( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_VTX_SELECTED );
		m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_MOVE_SEGMENT )
	{
		m_Doc->m_nlist->CancelMovingSegment( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_SEG_SELECTED );
		m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_VTX_INSERT )
	{
		m_Doc->m_nlist->CancelDraggingSegmentNewVertex( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_SEG_SELECTED );
		m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_END_VTX )
	{
		m_Doc->m_nlist->CancelDraggingVertex( m_sel_net, m_sel_ic, m_sel_iv );
		SetCursorMode( CUR_END_VTX_SELECTED );
		m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_CONNECT )
	{
		m_Doc->m_dlist->StopDragging();
		if( m_sel_id.T1() == ID_PART )
			SetCursorMode( CUR_PAD_SELECTED );
		else
			SetCursorMode( CUR_VTX_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_TEXT )
	{
		m_Doc->m_tlist->CancelDraggingText( m_sel_text );
		if( m_dragging_new_item )
		{
			m_Doc->m_tlist->RemoveText( m_sel_text );
			CancelSelection();
			m_dragging_new_item = 0;
		}
		else
		{
			SetCursorMode( CUR_TEXT_SELECTED );
		}
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_SMCUTOUT
		  || m_cursor_mode == CUR_DRAG_SMCUTOUT_1
		  || (m_cursor_mode == CUR_DRAG_SMCUTOUT && m_sel_id.I3()<3) )
	{
		// dragging first, second or third corner of solder mask cutout
		// delete it and cancel dragging
		m_dlist->StopDragging();
		if( m_cursor_mode != CUR_ADD_SMCUTOUT )
			m_Doc->m_sm_cutout.RemoveAt( m_sel_id.I2() );
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_SMCUTOUT )
	{
		// dragging fourth or higher corner of solder mask cutout, close it
		m_dlist->StopDragging();
		SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
		m_Doc->m_sm_cutout[m_sel_id.I2()].Close( m_polyline_style );
		m_Doc->ProjectModified( TRUE );
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_INSERT )
	{
		m_dlist->StopDragging();
		CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.I2()];
		poly->MakeVisible();
		poly->HighlightSide( m_sel_id.I3() );
		SetCursorMode( CUR_SMCUTOUT_SIDE_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_MOVE )
	{
		m_dlist->StopDragging();
		CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.I2()];
		poly->MakeVisible();
		SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
		poly->HighlightCorner( m_sel_id.I3() );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_BOARD
		  || m_cursor_mode == CUR_DRAG_BOARD_1
		  || (m_cursor_mode == CUR_DRAG_BOARD && m_Doc->m_board_outline[m_sel_id.I2()].NumCorners()<3) )
	{
		// dragging first, second or third corner of board outline
		// just delete it (if necessary) and cancel
		m_dlist->StopDragging();
		if( m_cursor_mode != CUR_ADD_BOARD )
			m_Doc->m_board_outline.RemoveAt( m_sel_id.I2() );	
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_BOARD )
	{
		// dragging fourth or higher corner of board outline, close it
		m_dlist->StopDragging();
		SaveUndoInfoForBoardOutlines( TRUE, m_Doc->m_undo_list );
		m_Doc->m_board_outline[m_sel_id.I2()].Close( m_polyline_style );
		m_Doc->ProjectModified( TRUE );
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_BOARD_INSERT )
	{
		m_dlist->StopDragging();
		m_Doc->m_board_outline[m_sel_id.I2()].MakeVisible();
		m_Doc->m_board_outline[m_sel_id.I2()].HighlightSide( m_sel_id.I2() );
		SetCursorMode( CUR_BOARD_SIDE_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_BOARD_MOVE )
	{
		// get indexes for preceding and following corners
		m_dlist->StopDragging();
		m_Doc->m_board_outline[m_sel_id.I2()].MakeVisible();
		SetCursorMode( CUR_BOARD_CORNER_SELECTED );
		m_Doc->m_board_outline[m_sel_id.I2()].HighlightCorner( m_sel_id.I2() );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_AREA )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_1
		  || (m_cursor_mode == CUR_DRAG_AREA && m_sel_id.I3()<3) )
	{
		m_dlist->StopDragging();
		m_Doc->m_nlist->RemoveArea( m_sel_net, m_sel_ia );
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA)
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
		SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
		m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, TRUE, TRUE );
		CancelSelection();
		if( m_Doc->m_vis[LAY_RAT_LINE] )
			m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_AREA_CUTOUT )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1
		  || (m_cursor_mode == CUR_DRAG_AREA_CUTOUT && m_sel_id.I3()<3) )
	{
		m_dlist->StopDragging();
		CPolyLine * poly = &m_sel_net->area[m_sel_id.I2()];
		int ncont = poly->NumContours();
		poly->RemoveContour(ncont-1);
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
		SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
		int icont = m_sel_net->area[m_sel_ia].NumContours() - 1;
		int ic = m_sel_net->area[m_sel_ia].ContourStart(icont);
		CPoint p;
		p.x = m_sel_net->area[m_sel_ia].X(ic);
		p.y = m_sel_net->area[m_sel_ia].Y(ic);
		int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, FALSE );
		if( ret == -1 )
		{
			// error
			AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
			m_Doc->OnEditUndo();
		}
		TryToReselectAreaCorner( p.x, p.y );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_INSERT )
	{
		m_Doc->m_nlist->CancelDraggingInsertedAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		m_Doc->m_nlist->SelectAreaSide( m_sel_net, m_sel_ia, m_sel_is );
		SetCursorMode( CUR_AREA_SIDE_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_MOVE )
	{
		m_Doc->m_nlist->CancelDraggingAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		m_Doc->m_nlist->SelectAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		SetCursorMode( CUR_AREA_CORNER_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_STUB )
	{
		m_dlist->StopDragging();
		if( m_sel_id.I3() > 0 )
		{
			m_Doc->m_nlist->CancelDraggingStub( m_sel_net, m_sel_ic, m_sel_is );
			int x = m_sel_vtx->x;
			int y = m_sel_vtx->y;
			m_sel_id = m_sel_vtx->Id();
			m_sel_id.Resolve();
			SetCursorMode( CUR_END_VTX_SELECTED );
			BOOL test = m_Doc->m_nlist->TestPointInArea( m_sel_net, x, y, m_active_layer, NULL );
			if( !test )
			{
				// add a via and optimize
				OnEndVertexAddVia();	// this also optimizes and selects via
			}
			else
			{
				// just optimize
				if( m_Doc->m_vis[LAY_RAT_LINE] )
				{
					m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
					if( m_sel_id.Resolve() )
						SelectItem( m_sel_id );
					else
						CancelSelection();
				}
			}
		}
		else
		{
			m_Doc->m_nlist->RemoveNetConnect( m_sel_net, m_sel_ic );
			CancelSelection();
		}
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_GROUP )
	{
		CancelDraggingGroup();
		m_dlist->SetLayerVisible( LAY_RAT_LINE, m_Doc->m_vis[LAY_RAT_LINE] );
	}
	else if( m_cursor_mode == CUR_DRAG_GROUP_ADD )
	{
		CancelDraggingGroup();
		m_dlist->SetLayerVisible( LAY_RAT_LINE, m_Doc->m_vis[LAY_RAT_LINE] );
		m_Doc->OnEditUndo();
	}
	else if( m_cursor_mode == CUR_DRAG_MEASURE_1 || m_cursor_mode == CUR_DRAG_MEASURE_2 )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
	}
	else
	{
		m_disable_context_menu = 0;
	}
	ShowSelectStatus();
	CView::OnRButtonDown(nFlags, point);
}

// System Key on keyboard pressed down
//
void CFreePcbView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( nChar == 121 )
		OnKeyDown( nChar, nRepCnt, nFlags);
	else
		CView::OnSysKeyDown(nChar, nRepCnt, nFlags);
}

// System Key on keyboard pressed up
//
void CFreePcbView::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( nChar != 121 )
		CView::OnSysKeyUp(nChar, nRepCnt, nFlags);
}

// Key pressed up
//
void CFreePcbView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( nChar == 16 )
	{
		gShiftKeyDown = FALSE;
	}
	if( nChar == 'D' )
	{
		// 'd'
		m_Doc->m_drelist->MakeHollowCircles();
		Invalidate( FALSE );
	}
	else if( nChar == 16 || nChar == 17 )
	{
		if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB )
		{
			// routing a trace segment, set mode
			if( nChar == 17 )
				m_snap_mode = SM_GRID_POINTS;
			if( nChar == 16 && m_Doc->m_snap_angle == 45 )
				m_inflection_mode = IM_90_45;
			m_dlist->SetInflectionMode( m_inflection_mode );
			Invalidate( FALSE );
		}
	}
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

// Key pressed down
//
void CFreePcbView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( nChar == 16 )
		gShiftKeyDown = TRUE;
	if( nChar == 'D' )
	{
		// 'd'
		m_Doc->m_drelist->MakeSolidCircles();
		Invalidate( FALSE );
	}
	else if( nChar == 16 || nChar == 17 )
	{
		if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB )
		{
			// routing a trace segment, set mode
			if( nChar == 17 )	// ctrl
				m_snap_mode = SM_GRID_LINES;
			if( nChar == 16 && m_Doc->m_snap_angle == 45 )	// shift
				m_inflection_mode = IM_45_90;
			m_dlist->SetInflectionMode( m_inflection_mode );
			Invalidate( FALSE );
		}
	}
	else
	{
		HandleKeyPress( nChar, nRepCnt, nFlags );
	}

	// don't pass through SysKey F10
	if( nChar != 121 )
		CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

// Key on keyboard pressed down
//
void CFreePcbView::HandleKeyPress(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( m_bDraggingRect )
		return;

#ifdef ALLOW_CURVED_SEGMENTS
	if( nChar == 'C' && m_cursor_mode == CUR_SEG_SELECTED )
	{
		// toggle segment through straight and curved shapes
		cconnect * c = &m_sel_net->connect[m_sel_ic];
		cseg * s = &c->seg[m_sel_is];
		cvertex * pre_v = &m_sel_net->connect[m_sel_ic].vtx[m_sel_iv];
		cvertex * post_v = &m_sel_net->connect[m_sel_ic].vtx[m_sel_iv+1];
		int dx = post_v->x - pre_v->x;
		int dy = post_v->y - pre_v->y;
		if( dx == 0 || dy == 0 || s->layer == LAY_RAT_LINE )
		{
			// ratline or vertical or horizontal segment, must be straight
			if( s->curve != cseg::STRAIGHT )
				ASSERT(0);
		}
		else
		{
			// toggle through straight or curved options
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			m_Doc->m_nlist->UndrawConnection( m_sel_net, m_sel_ic );
			s->curve++;
			if( s->curve > cseg::CW )
				s->curve = cseg::STRAIGHT;
			m_Doc->m_nlist->DrawConnection( m_sel_net, m_sel_ic );
			ShowSelectStatus();
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		return;
	}
#endif

	if( nChar == 'F' && ( m_cursor_mode == CUR_VTX_SELECTED || m_cursor_mode == CUR_END_VTX_SELECTED ) )
	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->ForceVia( m_sel_net, m_sel_ic, m_sel_iv, FALSE );
		if( m_Doc->m_vis[LAY_RAT_LINE] )
		{
			m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
			if( m_sel_id.Resolve() )
				SelectItem( m_sel_id );
			else
				CancelSelection();
		}
		ShowSelectStatus();
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		return;
	}
	if( nChar == 'U' && ( m_cursor_mode == CUR_VTX_SELECTED || m_cursor_mode == CUR_END_VTX_SELECTED ) )
	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->UnforceVia( m_sel_net, m_sel_ic, m_sel_iv );
		if( m_cursor_mode == CUR_END_VTX_SELECTED
			&& m_sel_con->SegByIndex(m_sel_iv-1).layer == LAY_RAT_LINE
			&& m_sel_vtx->tee_ID == 0 )
		{
			m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_iv-1, TRUE );
			CancelSelection();
		}
		else
		{
			m_Doc->m_nlist->MergeUnroutedSegments( m_sel_net, m_sel_ic );
		}
		if( m_Doc->m_vis[LAY_RAT_LINE] )
		{
			m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
			if( m_sel_id.Resolve() )
				SelectItem( m_sel_id );
			else
				CancelSelection();
		}
		ShowSelectStatus();
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		return;
	}

	if( nChar == 'T' && (m_cursor_mode == CUR_VTX_SELECTED || m_cursor_mode == CUR_SEG_SELECTED ) )
	{
		// "t" pressed, select trace
		m_sel_id.SetT3( ID_ENTIRE_CONNECT );
		m_Doc->m_nlist->HighlightConnection( m_sel_net, m_sel_ic );
		SetCursorMode( CUR_CONNECT_SELECTED );
		Invalidate( FALSE );
	}

	if( nChar == 'N' )
	{
		// "n" pressed, select net
		if( m_cursor_mode == CUR_VTX_SELECTED
			|| m_cursor_mode == CUR_SEG_SELECTED
			|| m_cursor_mode == CUR_CONNECT_SELECTED 
			|| m_cursor_mode == CUR_AREA_CORNER_SELECTED 
			|| m_cursor_mode == CUR_AREA_SIDE_SELECTED )
		{
			m_sel_id.SetT2( ID_ENTIRE_NET );
			m_Doc->m_nlist->HighlightNet( m_sel_net );
			m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net );
			SetCursorMode( CUR_NET_SELECTED );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_PAD_SELECTED )
		{
			// pad selected and if "n" held down, select net
			cnet * net = m_Doc->m_plist->GetPinNet( m_sel_part, m_sel_id.I2() );
			if( net )
			{
				m_sel_net = net;
				m_sel_id = net->m_id;
				m_sel_id.SetT2( ID_ENTIRE_NET );
				m_Doc->m_nlist->HighlightNetConnections( m_sel_net );
				m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net );
				SetCursorMode( CUR_NET_SELECTED );
			}
		}
	}

	if( nChar == 27 )
	{
		// ESC key, if something selected, cancel it
		// otherwise, fake a right-click
		if( CurSelected() )
			CancelSelection();
		else
			OnRButtonDown( nFlags, CPoint(0,0) );
		return;
	}

	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );

	if( nChar == 'M' )
	{
		if( !CurDragging() )
		{
			CancelSelection();
			SetCursorMode( CUR_DRAG_MEASURE_1 );
			m_dlist->StartDraggingArray( pDC, m_last_mouse_point.x, m_last_mouse_point.y, 0, LAY_SELECTION, 1 ); 
		}
		else if( m_cursor_mode == CUR_DRAG_MEASURE_1 || m_cursor_mode == CUR_DRAG_MEASURE_2 )
		{
			m_dlist->StopDragging();
			SetCursorMode( CUR_NONE_SELECTED );
		}
	}

	if( nChar == 8 )
	{
		// backspace, see if we are routing
		if( m_cursor_mode == CUR_DRAG_RAT )
		{
			// backup, if possible, by unrouting preceding segment and changing active layer
			if( m_dir == 0 && m_sel_is > 0 )
			{
				// routing forward
				if( m_sel_vtx->tee_ID )
				{
					AfxMessageBox( "tee-vertex reached" );
				}
				else
				{
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					int new_active_layer = m_sel_con->SegByIndex(m_sel_is-1).layer;
					m_Doc->m_nlist->UnrouteSegment( m_sel_net, m_sel_ic, m_sel_is-1 );
					m_sel_id.SetI3( m_sel_id.I3() - 1 );
					ShowSelectStatus();
					m_last_mouse_point.x = m_sel_vtx->x;
					m_last_mouse_point.y = m_sel_vtx->y;
					CPoint p = m_dlist->PCBToScreen( m_last_mouse_point );
					SetCursorPos( p.x, p.y );
					OnRatlineRoute();
					m_dlist->ChangeRoutingLayer( pDC, new_active_layer, LAY_SELECTION, 0 );
					m_active_layer = new_active_layer;
					ShowActiveLayer();
				}
			}
			else if( m_dir == 1 && m_sel_is < m_sel_con->NumSegs()-1
				&& !(m_sel_is == m_sel_con->NumSegs()-2
				&& m_sel_con->end_pin == cconnect::NO_END ) )
			{
				// routing backward, not at end of stub trace
				if( m_sel_next_vtx->tee_ID )
				{
					AfxMessageBox( "tee-vertex reached" );
				}
				else
				{
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					m_Doc->m_nlist->CancelDraggingSegment( m_sel_net, m_sel_ic, m_sel_is );
					int new_active_layer = m_sel_con->SegByIndex(m_sel_is+1).layer;
					m_Doc->m_nlist->UnrouteSegment( m_sel_net, m_sel_ic, m_sel_is+1 );
					ShowSelectStatus();
					m_last_mouse_point.x = m_sel_next_vtx->x;
					m_last_mouse_point.y = m_sel_next_vtx->y;
					CPoint p = m_dlist->PCBToScreen( m_last_mouse_point );
					SetCursorPos( p.x, p.y );
					OnRatlineRoute();
					m_dlist->ChangeRoutingLayer( pDC, new_active_layer, LAY_SELECTION, 0 );
					m_active_layer = new_active_layer;
					ShowActiveLayer();
				}
			}
		}
		else if( m_cursor_mode == CUR_DRAG_STUB )
		{
			// routing stub trace
			if( m_sel_is > 1 )
			{
				if( m_sel_vtx->tee_ID )
				{
					AfxMessageBox( "tee-vertex reached" );
				}
				else
				{
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_is-1, FALSE );
					int ns = m_sel_con->NumSegs();
					m_sel_id.SetI3( ns );
					ShowSelectStatus();
					m_last_mouse_point.x = m_sel_vtx->x;
					m_last_mouse_point.y = m_sel_vtx->y;
					CPoint p = m_dlist->PCBToScreen( m_last_mouse_point );
					SetCursorPos( p.x, p.y );
					OnEndVertexAddSegments();
					int new_active_layer = m_sel_con->SegByIndex(m_sel_is-1).layer;
					m_dlist->ChangeRoutingLayer( pDC, new_active_layer, LAY_SELECTION, 0 );
					m_active_layer = new_active_layer;
					ShowActiveLayer();
				}
			}
			else
			{
				if( m_sel_vtx->tee_ID )
				{
					AfxMessageBox( "tee-vertex reached" );
				}
				else
				{
					m_Doc->m_nlist->CancelDraggingStub( m_sel_net, m_sel_ic, m_sel_is );
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					cpart * sel_part = m_Doc->m_plist->GetPart( m_sel_con_start_pin->ref_des );
					int i = sel_part->shape->GetPinIndexByName( m_sel_con_start_pin->pin_name );
					m_sel_con->Undraw();
					m_Doc->m_nlist->RemoveNetConnect( m_sel_net, m_sel_ic );
					CancelSelection();
					m_sel_net = NULL;
					m_sel_part = sel_part;
					m_sel_id = sel_part->m_id;
					m_sel_id.SetT2( ID_PAD );
					m_sel_id.SetI2( i );
					m_Doc->m_plist->SelectPad( sel_part, i );
					OnPadStartStubTrace();
				}
			}
		}
		return;
	}

	int fk = FK_NONE;
	int dx = 0;
	int dy = 0;

	// get cursor position and convert to PCB coords
	CPoint p;
	GetCursorPos( &p );		// cursor pos in screen coords
	p = m_dlist->ScreenToPCB( p );	// convert to PCB coords

	// test for pressing key to change layer
	char test_char = nChar;
	if( test_char >= 97 )
		test_char = '1' + nChar - 97;
	char * ch = strchr( layer_char, test_char );
	if( ch && m_Doc->m_num_copper_layers > 1 )
	{
		int ilayer = ch - layer_char;
		if( ilayer < m_Doc->m_num_copper_layers )
		{
			// OK, shortcut key to a copper layer
			int new_active_layer = ilayer + LAY_TOP_COPPER;
			if( ilayer == m_Doc->m_num_copper_layers-1 )
				new_active_layer = LAY_BOTTOM_COPPER;
			else if( new_active_layer > LAY_TOP_COPPER )
				new_active_layer++;
			if( !gShiftKeyDown )
			{
				// shift key not held down, change active layer for routing
				if( !m_Doc->m_vis[new_active_layer] )
				{
					PlaySound( TEXT("CriticalStop"), 0, 0 );
					AfxMessageBox( "Can't route on invisible layer" );
					ReleaseDC( pDC );
					return;
				}
				if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB)
				{
					// if we are routing, change layer
					pDC->SelectClipRgn( &m_pcb_rgn );
					SetDCToWorldCoords( pDC );
					if( m_sel_id.I3() == 0 && m_dir == 0 )
					{
						// we are trying to change first segment from pad
						int p1 = m_sel_con->start_pin;
						CString pin_name = m_sel_net->pin[p1].pin_name;
						int pin_index = m_sel_net->pin[p1].part->shape->GetPinIndexByName( pin_name );
						if( m_sel_net->pin[p1].part->shape->m_padstack[pin_index].hole_size == 0)
						{
							// SMT pad, this is illegal;
							new_active_layer = -1;
							PlaySound( TEXT("CriticalStop"), 0, 0 );
						}
					}
					else if( m_sel_id.I3() == (m_sel_con->NumSegs()-1) && m_dir == 1 )
					{
						// we are trying to change last segment to pad
						int p2 = m_sel_con->end_pin;
						if( p2 != -1 )
						{
							CString pin_name = m_sel_net->pin[p2].pin_name;
							int pin_index = m_sel_net->pin[p2].part->shape->GetPinIndexByName( pin_name );
							if( m_sel_net->pin[p2].part->shape->m_padstack[pin_index].hole_size == 0)
							{
								// SMT pad
								new_active_layer = -1;
								PlaySound( TEXT("CriticalStop"), 0, 0 );
							}
						}
					}
					if( new_active_layer != -1 )
					{
						m_dlist->ChangeRoutingLayer( pDC, new_active_layer, LAY_SELECTION, 0 );
						m_active_layer = new_active_layer;
						ShowActiveLayer();
					}
				}
				else
				{
					m_active_layer = new_active_layer;
					ShowActiveLayer();
				}
				return;
			}
			else
			{
				// shift key held down, change layer if item selected 
				if( m_cursor_mode == CUR_SEG_SELECTED )
				{
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					m_sel_con->Undraw();
					cconnect * c = m_sel_con;
					cseg * seg = m_sel_seg;
					seg->layer = new_active_layer;
					m_sel_con->Draw();
					m_Doc->ProjectModified( TRUE );
					Invalidate( FALSE );
				}
				else if( m_cursor_mode == CUR_CONNECT_SELECTED )
				{
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					cconnect * c = m_sel_con;
					c->Undraw();
					for( int is=0; is<c->NumSegs(); is++ )
					{
						cseg * seg = &c->SegByIndex(is);
						seg->layer = new_active_layer;
					}
					m_sel_con->Draw();
					m_Doc->ProjectModified( TRUE );
					Invalidate( FALSE );
				}
				else if( m_cursor_mode == CUR_AREA_CORNER_SELECTED 
					|| m_cursor_mode == CUR_AREA_SIDE_SELECTED )
				{
					SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
					carea * a = &m_sel_net->area[m_sel_ia];
					a->Undraw();
					a->SetLayer( new_active_layer );
					a->Draw( m_dlist );
					int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, TRUE, TRUE );
					if( ret == -1 )
					{
						// error
						AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
						m_Doc->OnEditUndo();
					}
					else
					{
						if( m_Doc->m_vis[LAY_RAT_LINE] )
							m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
					}
					CancelSelection();
					m_Doc->ProjectModified( TRUE );
					Invalidate( FALSE );
				}
				return;
			}
		}
	}

	// continue
	if( nChar >= 112 && nChar <= 123 )
	{
		// function key pressed
		fk = m_fkey_option[nChar-112];
	}
	if( nChar >= 37 && nChar <= 40 )
	{
		// arrow key
		BOOL bShift;
		SHORT kc = GetKeyState( VK_SHIFT );
		if( kc < 0 )
			bShift = TRUE;
		else
			bShift = FALSE;
		fk = FK_ARROW;
		int d;
		if( bShift && m_Doc->m_units == MM )
			d = 10000;		// 0.01 mm
		else if( bShift && m_Doc->m_units == MIL )
			d = 25400;		// 1 mil
		else if( m_sel_id.T1() == ID_NET )
			d = m_Doc->m_routing_grid_spacing;
		else
			d = m_Doc->m_part_grid_spacing;
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
		gLastKeyWasArrow = FALSE;

	switch( m_cursor_mode )
	{
	case  CUR_NONE_SELECTED:
		if( fk == FK_ADD_AREA )
			OnAddArea();
		else if( fk == FK_ADD_TEXT )
			OnTextAdd();
		else if( fk == FK_ADD_PART )
			m_Doc->OnAddPart();
		else if( fk == FK_REDO_RATLINES )
		{
			SaveUndoInfoForAllNets( TRUE, m_Doc->m_undo_list );
			//			StartTimer();
			m_Doc->m_nlist->OptimizeConnections();
			//			double time = GetElapsedTime();
			Invalidate( FALSE );
		}
		break;

	case CUR_PART_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				if( m_sel_part->glued )
				{
					int ret = AfxMessageBox( "This part is glued, do you want to unglue it ?  ", MB_YESNO );
					if( ret == IDYES )
						m_sel_part->glued = 0;
					else
						return;
				}
				SaveUndoInfoForPartAndNets( m_sel_part,
					CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			m_Doc->m_plist->Move( m_sel_part,
				m_sel_part->x+dx,
				m_sel_part->y+dy,
				m_sel_part->angle,
				m_sel_part->side );
			m_Doc->m_nlist->PartMoved( m_sel_part );
			if( m_Doc->m_vis[LAY_RAT_LINE] )
				m_Doc->m_nlist->OptimizeConnections( m_sel_part, m_Doc->m_auto_ratline_disable, 
										m_Doc->m_auto_ratline_min_pins );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			m_Doc->m_plist->HighlightPart( m_sel_part );
			ShowRelativeDistance( m_sel_part->x, m_sel_part->y, 
				gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_DELETE_PART || nChar == 46 )
			OnPartDelete();
		else if( fk == FK_EDIT_PART )
			m_Doc->OnPartProperties();
		else if( fk == FK_EDIT_FOOTPRINT )
		{
			m_Doc->m_edit_footprint = TRUE;
			OnPartEditFootprint();
		}
		else if( fk == FK_GLUE_PART )
			OnPartGlue();
		else if( fk == FK_UNGLUE_PART )
			OnPartUnglue();
		else if( fk == FK_MOVE_PART )
			OnPartMove();
		else if( fk == FK_REDO_RATLINES )
			OnPartOptimize();
		break;

	case CUR_REF_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForPart( m_sel_part,
					CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			CPoint ref_pt = m_Doc->m_plist->GetRefPoint( m_sel_part );
			m_Doc->m_plist->MoveRefText( m_sel_part,
										ref_pt.x + dx,
										ref_pt.y + dy,
										m_sel_part->m_ref_angle,
										m_sel_part->m_ref_size,
										m_sel_part->m_ref_w );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			m_Doc->m_plist->SelectRefText( m_sel_part );
			ShowRelativeDistance( m_Doc->m_plist->GetRefPoint(m_sel_part).x,
				m_Doc->m_plist->GetRefPoint(m_sel_part).y,
				gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_SET_PARAMS )
			OnRefProperties();
		else if( fk == FK_MOVE_REF )
			OnRefMove();
		else if( fk == FK_ROTATE_REF )
			OnRefRotateCW();
		else if( fk == FK_ROTATE_REF_CCW )
			OnRefRotateCCW();
		break;

	case CUR_VALUE_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForPart( m_sel_part,
					CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			CPoint val_pt = m_Doc->m_plist->GetValuePoint( m_sel_part );
			m_Doc->m_plist->MoveValueText( m_sel_part,
										val_pt.x + dx,
										val_pt.y + dy,
										m_sel_part->m_value_angle,
										m_sel_part->m_value_size,
										m_sel_part->m_value_w );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			m_Doc->m_plist->SelectValueText( m_sel_part );
			ShowRelativeDistance( m_Doc->m_plist->GetValuePoint(m_sel_part).x,
				m_Doc->m_plist->GetValuePoint(m_sel_part).y,
				gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_SET_PARAMS )
			OnValueProperties();
		else if( fk == FK_MOVE_VALUE )
			OnValueMove();
		else if( fk == FK_ROTATE_VALUE )
			OnValueRotateCW();
		else if( fk == FK_ROTATE_VALUE_CCW )
			OnValueRotateCCW();
		break;

	case CUR_RAT_SELECTED:
		if( fk == FK_SET_WIDTH )
			OnRatlineSetWidth();
		else if( fk == FK_LOCK_CONNECT )
			OnRatlineLockConnection();
		else if( fk == FK_UNLOCK_CONNECT )
			OnRatlineUnlockConnection();
		else if( fk == FK_ROUTE )
			OnRatlineRoute();
		else if( fk == FK_CHANGE_PIN )
			OnRatlineChangeEndPin();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_DELETE_SEGMENT )
			OnSegmentDelete();
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnSegmentDeleteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();
		break;

	case  CUR_SEG_SELECTED:
		if( fk == FK_ARROW )
		{
			if(!SegmentMovable())
			{
				PlaySound( TEXT("CriticalStop"), 0, 0 );
				break;
			}

			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();

			// 1. Move the line defined by the segment
			m_last_pt.x = m_sel_prev_vtx->x;
			m_last_pt.y = m_sel_prev_vtx->y;

			m_from_pt.x = m_sel_vtx->x;
			m_from_pt.y = m_sel_vtx->y;

			m_to_pt.x = m_sel_next_vtx->x;
			m_to_pt.y = m_sel_next_vtx->y;

			int nsegs = m_sel_con->NumSegs();
			int use_third_segment = m_sel_is < nsegs - 1;
			if(use_third_segment)
			{
				m_next_pt.x = m_sel_next_next_vtx->x;	// Shouldn't really do this if we're off the edge?
				m_next_pt.y = m_sel_next_next_vtx->y;
			} else {
				m_next_pt.x = 0;
				m_next_pt.y = 0;
			}

			// 1. Move the endpoints of (xi, yi), (xf, yf) of the line by the mouse movement. This
			//		is just temporary, since the final ending position is determined by the intercept
			//		points with the leading and trailing segments:
			int new_from_x = m_from_pt.x + dx;			
			int new_from_y = m_from_pt.y + dy;

			int new_to_x = m_to_pt.x + dx;			
			int new_to_y = m_to_pt.y + dy;

			int old_x0_dir = sign(m_from_pt.x - m_last_pt.x);
			int old_y0_dir = sign(m_from_pt.y - m_last_pt.y);

			int old_x1_dir = sign(m_to_pt.x - m_from_pt.x);
			int old_y1_dir = sign(m_to_pt.y - m_from_pt.y);

			int old_x2_dir = sign(m_next_pt.x - m_to_pt.x);
			int old_y2_dir = sign(m_next_pt.y - m_to_pt.y);

			// 2. Find the intercept between the extended segment in motion and the leading segment.
			double d_new_from_x;
			double d_new_from_y;
			FindLineIntersection(m_last_pt.x, m_last_pt.y, m_from_pt.x, m_from_pt.y,
									new_from_x,    new_from_y,	   new_to_x,    new_to_y,
									&d_new_from_x, &d_new_from_y);
			int i_nudge_from_x = floor(d_new_from_x + .5);
			int i_nudge_from_y = floor(d_new_from_y + .5);

			// 3. Find the intercept between the extended segment in motion and the trailing segment:
			int i_nudge_to_x, i_nudge_to_y;
			if(use_third_segment)
			{
				double d_new_to_x;
				double d_new_to_y;
				FindLineIntersection(new_from_x,    new_from_y,	   new_to_x,    new_to_y,
									 m_to_pt.x,		m_to_pt.y,	m_next_pt.x, m_next_pt.y,
										&d_new_to_x, &d_new_to_y);

				i_nudge_to_x = floor(d_new_to_x + .5);
				i_nudge_to_y = floor(d_new_to_y + .5);
			} else {
				i_nudge_to_x = new_to_x;
				i_nudge_to_y = new_to_y;
			}
			
			// If we drag too far, the line segment can reverse itself causing a little triangle to form.
			//   That's a bad thing.
			if(    sign(i_nudge_to_x - i_nudge_from_x) == old_x1_dir 
				&& sign(i_nudge_to_y - i_nudge_from_y) == old_y1_dir
				&& sign(i_nudge_from_x - m_last_pt.x) == old_x0_dir
				&& sign(i_nudge_from_y - m_last_pt.y) == old_y0_dir
				&& (!use_third_segment || (sign(m_next_pt.x - i_nudge_to_x) == old_x2_dir 
										&& sign(m_next_pt.y - i_nudge_to_y) == old_y2_dir)))
			{
			//	Move both vetices to the new position:
				m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,
											i_nudge_from_x, i_nudge_from_y );
				m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is+1,
											i_nudge_to_x, i_nudge_to_y );
			} else {
				break;
			}

			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( m_sel_vtx->x, m_sel_vtx->y, gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		if( fk == FK_SET_WIDTH )
			OnSegmentSetWidth();
		else if( fk == FK_CHANGE_LAYER )
			OnSegmentChangeLayer();
		else if( fk == FK_ADD_VERTEX )
			OnSegmentAddVertex();
		else if( fk == FK_MOVE_SEGMENT)
			OnSegmentMove();
		else if( fk == FK_UNROUTE )
			OnSegmentUnroute();
		else if( fk == FK_DELETE_SEGMENT )
			OnSegmentDelete();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnSegmentDeleteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();	//**
		break;

	case  CUR_VTX_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,
										m_sel_vtx->x + dx, m_sel_vtx->y + dy );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( m_sel_vtx->x, m_sel_vtx->y, gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_SET_POSITION )
			OnVertexProperties();
		else if( fk == FK_START_STUB )
			OnVertexStartStub();
		else if( fk == FK_VERTEX_PROPERTIES )
			OnVertexProperties();
		else if( fk == FK_MOVE_VERTEX )
			OnVertexMove();
		else if( fk == FK_ADD_CONNECT )
			OnVertexConnectToPin();
		else if( fk == FK_DELETE_VERTEX )
			OnVertexDelete();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnSegmentDeleteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();
		break;

	case  CUR_END_VTX_SELECTED:
		if( fk == FK_SET_POSITION )
			OnVertexProperties();
		else if( fk == FK_ADD_CONNECT )
			OnVertexConnectToPin();
		else if( fk == FK_MOVE_VERTEX )
			OnEndVertexMove();
		else if( fk == FK_DELETE_VERTEX )
			OnVertexDelete();
		else if( fk == FK_ADD_SEGMENT )
			OnEndVertexAddSegments();
		else if( fk == FK_ADD_VIA )
			OnEndVertexAddVia();
		else if( fk == FK_DELETE_VIA )
			OnEndVertexRemoveVia();
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnSegmentDeleteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();
		break;

	case  CUR_CONNECT_SELECTED:
		if( fk == FK_SET_WIDTH )
			OnConnectSetWidth();
		else if( fk == FK_CHANGE_LAYER )
			OnConnectChangeLayer();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();	//**
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnSegmentDeleteTrace();
		break;

	case  CUR_NET_SELECTED:
		if( fk == FK_SET_WIDTH )
			OnNetSetWidth();
		else if( fk == FK_CHANGE_LAYER )
			OnNetChangeLayer();
		else if( fk == FK_EDIT_NET )
			OnNetEditnet();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();
		break;

	case  CUR_PAD_SELECTED:
		if( fk == FK_ATTACH_NET )
			OnPadAddToNet();
		else if( fk == FK_START_STUB )
			OnPadStartStubTrace();
		else if( fk == FK_ADD_CONNECT )
			OnPadConnectToPin();
		else if( fk == FK_DETACH_NET )
			OnPadDetachFromNet();
		else if( fk == FK_REDO_RATLINES )
			OnPadOptimize();
		break;

	case CUR_TEXT_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_MODIFY, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			m_Doc->m_tlist->MoveText( m_sel_text,
						m_sel_text->m_x + dx, m_sel_text->m_y + dy,
						m_sel_text->m_angle, m_sel_text->m_mirror,
						m_sel_text->m_bNegative, m_sel_text->m_layer );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( m_sel_text->m_x, m_sel_text->m_y, 
				gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->m_tlist->HighlightText( m_sel_text );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_EDIT_TEXT )
			OnTextEdit();
		else if( fk == FK_MOVE_TEXT )
			OnTextMove();
		else if( fk == FK_DELETE_TEXT || nChar == 46 )
			OnTextDelete();
		break;

	case CUR_BOARD_CORNER_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForBoardOutlines( TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			CPolyLine * poly = &m_Doc->m_board_outline[m_sel_id.I2()];
			poly->MoveCorner( m_sel_is,
				poly->X( m_sel_is ) + dx,
				poly->Y( m_sel_is ) + dy );
			m_dlist->CancelHighLight();
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( poly->X( m_sel_is ), poly->Y( m_sel_is ),
				gTotalArrowMoveX, gTotalArrowMoveY );
			poly->HighlightCorner( m_sel_is );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_SET_POSITION )
			OnBoardCornerEdit();
		else if( fk == FK_MOVE_CORNER )
			OnBoardCornerMove();
		else if( fk == FK_DELETE_CORNER )
			OnBoardCornerDelete();
		else if( fk == FK_DELETE_OUTLINE || nChar == 46 )
			OnBoardDeleteOutline();
		break;

	case CUR_BOARD_SIDE_SELECTED:
		if( fk == FK_POLY_STRAIGHT )
			OnBoardSideConvertToStraightLine();
		else if( fk == FK_POLY_ARC_CW )
			OnBoardSideConvertToArcCw();
		else if( fk == FK_POLY_ARC_CCW )
			OnBoardSideConvertToArcCcw();
		else if( fk == FK_ADD_CORNER )
			OnBoardSideAddCorner();
		else if( fk == FK_DELETE_OUTLINE || nChar == 46 )
			OnBoardDeleteOutline();
		break;

	case CUR_SMCUTOUT_CORNER_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_ic];
			poly->MoveCorner( m_sel_is,
				poly->X( m_sel_is ) + dx,
				poly->Y( m_sel_is ) + dy );
			m_dlist->CancelHighLight();
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( poly->X( m_sel_is ), poly->Y( m_sel_is ),
				gTotalArrowMoveX, gTotalArrowMoveY );
			poly->HighlightCorner( m_sel_is );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_SET_POSITION )
			OnSmCornerSetPosition();
		else if( fk == FK_MOVE_CORNER )
			OnSmCornerMove();
		else if( fk == FK_DELETE_CORNER )
			OnSmCornerDeleteCorner();
		else if( fk == FK_DELETE_CUTOUT || nChar == 46 )
			OnSmCornerDeleteCutout();
		break;

	case CUR_SMCUTOUT_SIDE_SELECTED:
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.I2()];
			if( fk == FK_POLY_STRAIGHT )
			{
				m_dlist->CancelHighLight();
				m_polyline_style = CPolyLine::STRAIGHT;
				poly->SetSideStyle( m_sel_id.I3(), m_polyline_style );
				SetFKText( m_cursor_mode );
				poly->HighlightSide( m_sel_id.I3() );
				Invalidate( FALSE );
				m_Doc->ProjectModified( TRUE );
			}
			else if( fk == FK_POLY_ARC_CW )
			{
				m_dlist->CancelHighLight();
				m_polyline_style = CPolyLine::ARC_CW;
				poly->SetSideStyle( m_sel_id.I3(), m_polyline_style );
				SetFKText( m_cursor_mode );
				poly->HighlightSide( m_sel_id.I3() );
				Invalidate( FALSE );
				m_Doc->ProjectModified( TRUE );
			}
			else if( fk == FK_POLY_ARC_CCW )
			{
				m_dlist->CancelHighLight();
				m_polyline_style = CPolyLine::ARC_CCW;
				poly->SetSideStyle( m_sel_id.I3(), m_polyline_style );
				SetFKText( m_cursor_mode );
				poly->HighlightSide( m_sel_id.I3() );
				Invalidate( FALSE );
				m_Doc->ProjectModified( TRUE );
			}
			else if( fk == FK_ADD_CORNER )
				OnSmSideInsertCorner();
			else if( fk == FK_DELETE_CUTOUT || nChar == 46 )
				OnSmSideDeleteCutout();
		}
		break;

	case CUR_AREA_CORNER_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			CPolyLine * poly = &m_sel_net->area[m_sel_ic];
			CPoint p;
			p.x = poly->X(m_sel_is)+dx;
			p.y = poly->Y(m_sel_is)+dy;
			poly->MoveCorner( m_sel_is, p.x, p.y );
			int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, TRUE );
			if( ret == -1 )
			{
				// error
				AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
				CancelSelection();
				m_Doc->OnEditUndo();
			}
			else
			{
				gTotalArrowMoveX += dx;
				gTotalArrowMoveY += dy;
				ShowRelativeDistance( p.x, p.y, gTotalArrowMoveX, gTotalArrowMoveY );
				TryToReselectAreaCorner( p.x, p.y );
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_EDIT_AREA )
			OnAreaEdit();
		else if( fk == FK_SET_POSITION )
			OnAreaCornerProperties();
		else if( fk == FK_MOVE_CORNER )
			OnAreaCornerMove();
		else if( fk == FK_DELETE_CORNER )
			OnAreaCornerDelete();
		else if( fk == FK_DELETE_AREA )
			OnAreaCornerDeleteArea();
		else if( fk == FK_AREA_CUTOUT )
			OnAreaAddCutout();
		else if( fk == FK_DELETE_CUTOUT )
			OnAreaDeleteCutout();
		else if( nChar == 46 )
			OnAreaCornerDelete();
		break;

	case CUR_AREA_SIDE_SELECTED:
		if( fk == FK_SIDE_STYLE )
			OnAreaSideStyle();
		else if( fk == FK_EDIT_AREA )
			OnAreaEdit();
		else if( fk == FK_POLY_ARC_CCW )
		{
			SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
//			SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
			m_polyline_style = CPolyLine::ARC_CCW;
			m_Doc->m_nlist->SetAreaSideStyle( m_sel_net, m_sel_ia, m_sel_is, m_polyline_style );
			m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
			if( m_Doc->m_vis[LAY_RAT_LINE] )
				m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
			SetFKText( m_cursor_mode );
			Invalidate( FALSE );
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_ADD_CORNER )
			OnAreaSideAddCorner();
		else if( fk == FK_DELETE_AREA )
			OnAreaSideDeleteArea();
		else if( fk == FK_AREA_CUTOUT )
			OnAreaAddCutout();
		else if( fk == FK_DELETE_CUTOUT )
			OnAreaDeleteCutout();
		else if( nChar == 46 )
		{
			CPolyLine * poly = &m_sel_net->area[m_sel_ia];
			if( poly->Contour( m_sel_id.I3() ) > 0 )
				OnAreaDeleteCutout();
			else
				OnAreaSideDeleteArea();
		}
		break;

	case CUR_DRE_SELECTED:
		if( nChar == 46 )
		{
			CancelSelection();
			m_Doc->m_drelist->Remove( m_sel_dre );
			Invalidate( FALSE );
		}
		break;

	case CUR_GROUP_SELECTED:
		if( fk == FK_ARROW )
		{
			m_dlist->CancelHighLight();
			if( !gLastKeyWasArrow && !gLastKeyWasGroupRotate)
			{
				if( GluedPartsInGroup() )
				{
					int ret = AfxMessageBox( "This group contains glued parts, do you want to unglue them ?  ", MB_YESNO );
					if( ret != IDYES )
						return;
				}
				SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, &m_sel_ptrs, &m_sel_ids, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			MoveGroup( dx, dy );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			HighlightGroup();
			ShowRelativeDistance( gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_MOVE_GROUP )
		{
			OnGroupMove();
		}
		else if( fk == FK_DELETE_GROUP || nChar == 46 )
		{
			OnGroupDelete();
		}
		else if(fk == FK_ROTATE_GROUP)
		{
			OnGroupRotate();
		}
		break;

	case CUR_DRAG_RAT:
		if( fk == FK_COMPLETE )
		{
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			int w, v_w, v_h_w;
			GetWidthsForSegment( &w, &v_w, &v_h_w );
			int test = m_Doc->m_nlist->RouteSegment( m_sel_net, m_sel_ic,
				m_sel_is, m_active_layer, w );
			if( !test )
			{
				m_Doc->m_nlist->CancelDraggingSegment( m_sel_net, m_sel_ic, m_sel_is );
				CancelSelection();
			}
			else
				PlaySound( TEXT("CriticalStop"), 0, 0 );
			Invalidate( FALSE );
			m_Doc->ProjectModified( TRUE );
		}
		break;

	case CUR_DRAG_STUB:
		break;

	case  CUR_DRAG_PART:
		if( fk == FK_ROTATE_PART )
			m_dlist->IncrementDragAngle( pDC );
		if( fk == FK_ROTATE_PART_CCW )
		{
			m_dlist->IncrementDragAngle( pDC );
			m_dlist->IncrementDragAngle( pDC );
			m_dlist->IncrementDragAngle( pDC );
		}
		else if( fk == FK_SIDE )
			m_dlist->FlipDragSide( pDC );
		break;

	case  CUR_DRAG_REF:
		if( fk == FK_ROTATE_REF )
			m_dlist->IncrementDragAngle( pDC );
		break;

	case  CUR_DRAG_VALUE:
		if( fk == FK_ROTATE_VALUE )
			m_dlist->IncrementDragAngle( pDC );
		break;

	case  CUR_DRAG_TEXT:
		if( fk == FK_ROTATE_TEXT )
			m_dlist->IncrementDragAngle( pDC );
		break;

	case  CUR_DRAG_BOARD:
	case  CUR_DRAG_BOARD_1:
		if( fk == FK_POLY_STRAIGHT )
		{
			m_polyline_style = CPolyLine::STRAIGHT;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CW )
		{
			m_polyline_style = CPolyLine::ARC_CW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CCW )
		{
			m_polyline_style = CPolyLine::ARC_CCW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		break;

	case  CUR_DRAG_AREA:
	case  CUR_DRAG_AREA_1:
	case  CUR_DRAG_AREA_CUTOUT:
	case  CUR_DRAG_AREA_CUTOUT_1:
		if( fk == FK_POLY_STRAIGHT )
		{
			m_polyline_style = CPolyLine::STRAIGHT;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CW )
		{
			m_polyline_style = CPolyLine::ARC_CW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CCW )
		{
			m_polyline_style = CPolyLine::ARC_CCW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		break;

	case  CUR_DRAG_SMCUTOUT:
	case  CUR_DRAG_SMCUTOUT_1:
		if( fk == FK_POLY_STRAIGHT )
		{
			m_polyline_style = CPolyLine::STRAIGHT;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CW )
		{
			m_polyline_style = CPolyLine::ARC_CW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CCW )
		{
			m_polyline_style = CPolyLine::ARC_CCW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		break;

	default:
		break;
	}	// end switch

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
		SetCursorPos( p.x, p.y );
	}
	else if( nChar == VK_HOME )
	{
		// home key pressed, ViewAllElements
		OnViewAllElements();
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
			SetCursorPos( p.x, p.y );
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
	ReleaseDC( pDC );
	if( gLastKeyWasArrow ==FALSE && gLastKeyWasGroupRotate==false )
		ShowSelectStatus();
}

// Mouse moved
//
void CFreePcbView::OnMouseMove(UINT nFlags, CPoint point)
{
	static BOOL bCursorOn = TRUE;

	if( (nFlags & MK_LBUTTON) && m_bLButtonDown )
	{
		double d = abs(point.x-m_start_pt.x) + abs(point.y-m_start_pt.y);
		if( m_bDraggingRect
			|| (d > 10 && !CurDragging() ) )
		{
			// we are dragging a selection rect
			SIZE s1;
			s1.cx = s1.cy = 1;
			m_drag_rect.TopLeft() = m_start_pt;
			m_drag_rect.BottomRight() = point;
			m_drag_rect.NormalizeRect();
			CDC * pDC = GetDC();
			if( !m_bDraggingRect )
			{
				//start dragging rect
				pDC->DrawDragRect( &m_drag_rect, s1, NULL, s1 );
			}
			else
			{
				// continue dragging rect
				pDC->DrawDragRect( &m_drag_rect, s1, &m_last_drag_rect, s1 );
			}
			m_bDraggingRect  = TRUE;
			m_last_drag_rect = m_drag_rect;
			ReleaseDC( pDC );
		}
	}
	m_last_mouse_point = m_dlist->WindowToPCB( point );
	SnapCursorPoint( m_last_mouse_point, nFlags );
	// check for cursor hiding
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	if( !CurDragging() )
		frm->m_bHideCursor = FALSE;		// disable cursor hiding
	else if( !frm->m_bHideCursor )
	{
		// enable cursor hiding and set rect
		CRect r = frm->m_client_rect;
		r.left += m_left_pane_w;
		r.bottom -= m_bottom_pane_h;
		frm->SetHideCursor( TRUE, &r );
	}
}


/////////////////////////////////////////////////////////////////////////
// Utility functions
//

// Set the device context to world coords
//
int CFreePcbView::SetDCToWorldCoords( CDC * pDC )
{
	m_dlist->SetDCToWorldCoords( pDC, &m_memDC, m_org_x, m_org_y );

	return 0;
}


// Set cursor mode, update function key menu if necessary
//
void CFreePcbView::SetCursorMode( int mode )
{
	if( mode != m_cursor_mode )
	{
		SetFKText( mode );
		m_cursor_mode = mode;
		ShowSelectStatus();
		if( mode == CUR_GROUP_SELECTED )
		{
			CWnd* pMain = AfxGetMainWnd();
			if (pMain != NULL)
			{
				CMenu* pMenu = pMain->GetMenu();
				CMenu* submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
				submenu->EnableMenuItem( ID_EDIT_COPY, MF_BYCOMMAND | MF_ENABLED );
				submenu->EnableMenuItem( ID_EDIT_CUT, MF_BYCOMMAND | MF_ENABLED );
				submenu->EnableMenuItem( ID_EDIT_SAVEGROUPTOFILE, MF_BYCOMMAND | MF_ENABLED );
				pMain->DrawMenuBar();
			}
		}
		else
		{
			CWnd* pMain = AfxGetMainWnd();
			if (pMain != NULL)
			{
				CMenu* pMenu = pMain->GetMenu();
				CMenu* submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
				submenu->EnableMenuItem( ID_EDIT_COPY, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
				submenu->EnableMenuItem( ID_EDIT_CUT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
				submenu->EnableMenuItem( ID_EDIT_SAVEGROUPTOFILE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
				pMain->DrawMenuBar();
			}
		}
		if( CurDragging() )
			SetMainMenu( FALSE );
		else if( m_Doc->m_project_open )
			SetMainMenu( TRUE );
	}
}

// Set function key shortcut text
//
void CFreePcbView::SetFKText( int mode )
{
	for( int i=0; i<12; i++ )
	{
		m_fkey_option[i] = 0;
		m_fkey_command[i] = 0;
	}

	switch( mode )
	{
	case CUR_NONE_SELECTED:
		if( m_Doc->m_project_open )
		{
			m_fkey_option[1] = FK_ADD_AREA;
			m_fkey_option[2] = FK_ADD_TEXT;
			m_fkey_option[3] = FK_ADD_PART;
			m_fkey_option[8] = FK_REDO_RATLINES;
		}
		break;

	case CUR_PART_SELECTED:
		m_fkey_option[0] = FK_EDIT_PART;
		m_fkey_option[1] = FK_EDIT_FOOTPRINT;
		if( m_sel_part->glued )
			m_fkey_option[2] = FK_UNGLUE_PART;
		else
			m_fkey_option[2] = FK_GLUE_PART;
		m_fkey_option[3] = FK_MOVE_PART;
		m_fkey_option[7] = FK_DELETE_PART;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_REF_SELECTED:
		m_fkey_option[0] = FK_SET_PARAMS;
		m_fkey_option[1] = FK_ROTATE_REF;
		m_fkey_option[2] = FK_ROTATE_REF_CCW;
		m_fkey_option[3] = FK_MOVE_REF;
		break;

	case CUR_VALUE_SELECTED:
		m_fkey_option[0] = FK_SET_PARAMS;
		m_fkey_option[1] = FK_ROTATE_VALUE;
		m_fkey_option[2] = FK_ROTATE_VALUE_CCW;
		m_fkey_option[3] = FK_MOVE_VALUE;
		break;

	case CUR_PAD_SELECTED:
		if( m_sel_part->pin[m_sel_id.I2()].net )
			m_fkey_option[0] = FK_DETACH_NET;
		else
			m_fkey_option[0] = FK_ATTACH_NET;
		m_fkey_option[2] = FK_START_STUB;
		m_fkey_option[3] = FK_ADD_CONNECT;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_TEXT_SELECTED:
		m_fkey_option[0] = FK_EDIT_TEXT;
		m_fkey_option[3] = FK_MOVE_TEXT;
		m_fkey_option[7] = FK_DELETE_TEXT;
		break;

	case CUR_SMCUTOUT_CORNER_SELECTED:
		m_fkey_option[0] = FK_SET_POSITION;
		m_fkey_option[3] = FK_MOVE_CORNER;
		m_fkey_option[4] = FK_DELETE_CORNER;
		m_fkey_option[7] = FK_DELETE_CUTOUT;
		break;

	case CUR_SMCUTOUT_SIDE_SELECTED:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		{
			int style = m_Doc->m_sm_cutout[m_sel_id.I2()].SideStyle( m_sel_id.I3() );
			if( style == CPolyLine::STRAIGHT )
				m_fkey_option[3] = FK_ADD_CORNER;
		}
		m_fkey_option[7] = FK_DELETE_CUTOUT;
		break;

	case CUR_BOARD_CORNER_SELECTED:
		m_fkey_option[0] = FK_SET_POSITION;
		m_fkey_option[3] = FK_MOVE_CORNER;
		m_fkey_option[4] = FK_DELETE_CORNER;
		m_fkey_option[7] = FK_DELETE_OUTLINE;
		break;

	case CUR_BOARD_SIDE_SELECTED:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		{
			int style = m_Doc->m_board_outline[m_sel_id.I2()].SideStyle( m_sel_id.I3() );
			if( style == CPolyLine::STRAIGHT )
				m_fkey_option[3] = FK_ADD_CORNER;
		}
		m_fkey_option[7] = FK_DELETE_OUTLINE;
		break;

	case CUR_AREA_CORNER_SELECTED:
		m_fkey_option[0] = FK_SET_POSITION;
		m_fkey_option[1] = FK_EDIT_AREA;
		m_fkey_option[3] = FK_MOVE_CORNER;
		m_fkey_option[4] = FK_DELETE_CORNER;
		{
			CPolyLine * poly = &m_sel_net->area[m_sel_ia];
			if( poly->Contour( m_sel_id.I3() ) > 0 )
				m_fkey_option[6] = FK_DELETE_CUTOUT;
			else
				m_fkey_option[6] = FK_AREA_CUTOUT;
		}
		m_fkey_option[7] = FK_DELETE_AREA;
		break;

	case CUR_AREA_SIDE_SELECTED:
		m_fkey_option[0] = FK_SIDE_STYLE;
		m_fkey_option[1] = FK_EDIT_AREA;
		{
			int style = m_sel_net->area[m_sel_id.I2()].SideStyle(m_sel_id.I3());
			if( style == CPolyLine::STRAIGHT )
				m_fkey_option[3] = FK_ADD_CORNER;
		}
		{
			CPolyLine * poly = &m_sel_net->area[m_sel_ia];
			if( poly->Contour( m_sel_id.I3() ) > 0 )
				m_fkey_option[6] = FK_DELETE_CUTOUT;
			else
				m_fkey_option[6] = FK_AREA_CUTOUT;
		}
		m_fkey_option[7] = FK_DELETE_AREA;
		break;

	case CUR_SEG_SELECTED:
		m_fkey_option[0] = FK_SET_WIDTH;
		m_fkey_option[1] = FK_CHANGE_LAYER;
		if( m_sel_con->end_pin == cconnect::NO_END )
		{
			// stub trace
			if( m_sel_con->VtxByIndex(m_sel_con->NumSegs()).force_via_flag
				|| m_sel_con->VtxByIndex(m_sel_con->NumSegs()).tee_ID )
			{
				m_fkey_option[5] = FK_UNROUTE_TRACE;
			}
			if( m_sel_con->NumSegs() == (m_sel_id.I3()+1) )
			{
				// end segment of stub trace
				if( m_sel_con->VtxByIndex(m_sel_con->NumSegs()).force_via_flag
					|| m_sel_con->VtxByIndex(m_sel_con->NumSegs()).tee_ID )
					m_fkey_option[4] = FK_UNROUTE;
				m_fkey_option[6] = FK_DELETE_SEGMENT;
			}
			else
			{
				// other segment of stub trace
				m_fkey_option[4] = FK_UNROUTE;
			}
		}
		else
		{
			// normal trace
			m_fkey_option[4] = FK_UNROUTE;
			m_fkey_option[5] = FK_UNROUTE_TRACE;
		}
		m_fkey_option[2] = FK_ADD_VERTEX;
		if(SegmentMovable())
			m_fkey_option[3] = FK_MOVE_SEGMENT;
		m_fkey_option[7] = FK_DELETE_CONNECT;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_RAT_SELECTED:
		m_fkey_option[0] = FK_SET_WIDTH;
		if( m_sel_con->locked )
			m_fkey_option[2] = FK_UNLOCK_CONNECT;
		else
			m_fkey_option[2] = FK_LOCK_CONNECT;
		m_fkey_option[3] = FK_ROUTE;
		m_fkey_option[5] = FK_UNROUTE_TRACE;
		if( m_sel_con->end_pin == cconnect::NO_END )
		{
			// stub trace
			if( m_sel_con->NumSegs() == (m_sel_id.I3()+1) )
			{
				// end segment of stub trace
				m_fkey_option[6] = FK_DELETE_SEGMENT;
			}
		}
		else if( m_sel_con->NumSegs() > 1
			&& ( m_sel_id.I3() == 0 || m_sel_id.I3() == (m_sel_con->NumSegs()-1) ) )
		{
			// pin-pin connection
			m_fkey_option[4] = FK_CHANGE_PIN;
		}
		m_fkey_option[7] = FK_DELETE_CONNECT;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_VTX_SELECTED:
		m_fkey_option[0] = FK_VERTEX_PROPERTIES;
		m_fkey_option[1] = FK_START_STUB;
		m_fkey_option[2] = FK_ADD_CONNECT;
		m_fkey_option[3] = FK_MOVE_VERTEX;
		if( m_sel_con->end_pin != cconnect::NO_END 
			|| m_sel_con->VtxByIndex(m_sel_con->NumSegs()).via_w
			|| m_sel_con->VtxByIndex(m_sel_con->NumSegs()).tee_ID )
			m_fkey_option[5] = FK_UNROUTE_TRACE;
		m_fkey_option[6] = FK_DELETE_VERTEX;
		m_fkey_option[7] = FK_DELETE_CONNECT;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_END_VTX_SELECTED:
		m_fkey_option[0] = FK_VERTEX_PROPERTIES;
		m_fkey_option[1] = FK_ADD_SEGMENT;
		if( m_sel_vtx->via_w )
			m_fkey_option[4] = FK_DELETE_VIA;
		else
			m_fkey_option[4] = FK_ADD_VIA;
		m_fkey_option[2] = FK_ADD_CONNECT;
		m_fkey_option[3] = FK_MOVE_VERTEX;
		if( m_sel_vtx->via_w )
			m_fkey_option[5] = FK_UNROUTE_TRACE;
		m_fkey_option[6] = FK_DELETE_VERTEX;
		m_fkey_option[7] = FK_DELETE_CONNECT;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_CONNECT_SELECTED:
		m_fkey_option[0] = FK_SET_WIDTH;
		m_fkey_option[1] = FK_CHANGE_LAYER;
		m_fkey_option[5] = FK_UNROUTE_TRACE;
		m_fkey_option[7] = FK_DELETE_CONNECT;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_NET_SELECTED:
		m_fkey_option[0] = FK_SET_WIDTH;
		m_fkey_option[1] = FK_CHANGE_LAYER;
		m_fkey_option[2] = FK_EDIT_NET;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_GROUP_SELECTED:
		m_fkey_option[2] = FK_ROTATE_GROUP;
		m_fkey_option[3] = FK_MOVE_GROUP;
		m_fkey_option[7] = FK_DELETE_GROUP;
		break;

	case CUR_DRAG_PART:
		m_fkey_option[1] = FK_SIDE;
		m_fkey_option[2] = FK_ROTATE_PART;
		m_fkey_option[3] = FK_ROTATE_PART_CCW;
		break;

	case CUR_DRAG_REF:
		m_fkey_option[2] = FK_ROTATE_REF;
		break;

	case CUR_DRAG_VALUE:
		m_fkey_option[2] = FK_ROTATE_VALUE;
		break;

	case CUR_DRAG_TEXT:
		m_fkey_option[2] = FK_ROTATE_TEXT;
		break;

	case CUR_DRAG_VTX:
		break;

	case CUR_DRAG_RAT:
		m_fkey_option[3] = FK_COMPLETE;
		break;

	case CUR_DRAG_STUB:
		break;

	case CUR_DRAG_SMCUTOUT_1:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_SMCUTOUT:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_AREA_1:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_AREA_CUTOUT:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_AREA_CUTOUT_1:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_AREA:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_BOARD:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_BOARD_1:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;
	}

	for( int i=0; i<12; i++ )
	{
		strcpy( m_fkey_str[2*i],   fk_str[2*m_fkey_option[i]] );
		strcpy( m_fkey_str[2*i+1], fk_str[2*m_fkey_option[i]+1] );
	}

	InvalidateLeftPane();
	Invalidate( FALSE );
}

int CFreePcbView::SegmentMovable(void)
{
	// see if this is the end of the road, if so, can't move it:
	{
		int x = m_sel_vtx->x;
		int y = m_sel_vtx->y;
		int layer = m_sel_seg->layer;
		BOOL test = m_Doc->m_nlist->TestHitOnConnectionEndPad( x, y, m_sel_net,
			m_sel_id.I2(), layer, 1 );
		if( test || m_sel_vtx->tee_ID )
		{
			return FALSE;
		}
	}

	// see if end vertex of this segment is in end pad of connection
	{
		int x = m_sel_next_vtx->x;
		int y = m_sel_next_vtx->y;
		int layer = m_sel_seg->layer;
		BOOL test = m_Doc->m_nlist->TestHitOnConnectionEndPad( x, y, m_sel_net,
			m_sel_id.I2(), layer, 0 );
		if( test || m_sel_next_vtx->tee_ID)
		{
			return FALSE;
		}
	}
	return TRUE;
}

// Draw bottom pane
//
void CFreePcbView::DrawBottomPane()
{
	CDC * pDC = GetDC();
	CFont * old_font = pDC->SelectObject( &m_small_font );

	// get client rectangle
	GetClientRect( &m_client_r );

	// draw labels for function keys at bottom of client area
	for( int j=0; j<3; j++ )
	{
		for( int i=0; i<4; i++ )
		{
			int ifn = j*4+i;
			if( ifn < 9 )
			{
				CRect r( FKEY_OFFSET_X+ifn*FKEY_STEP+j*FKEY_GAP,
					m_client_r.bottom-FKEY_OFFSET_Y-FKEY_R_H,
					FKEY_OFFSET_X+ifn*FKEY_STEP+j*FKEY_GAP+FKEY_R_W,
					m_client_r.bottom-FKEY_OFFSET_Y );
				pDC->Rectangle( &r );
				pDC->MoveTo( r.left+FKEY_SEP_W, r.top );
				pDC->LineTo( r.left+FKEY_SEP_W, r.top + FKEY_R_H/2 + 1 );
				pDC->MoveTo( r.left, r.top + FKEY_R_H/2 );
				pDC->LineTo( r.left+FKEY_SEP_W, r.top + FKEY_R_H/2 );
				r.top += 1;
				r.left += 2;
				char fkstr[3] = "F1";
				fkstr[1] = '1' + j*4+i;
				pDC->DrawText( fkstr, -1, &r, 0 );
				r.left += FKEY_SEP_W;
				char * str1 = &m_fkey_str[2*ifn][0];
				char * str2 = &m_fkey_str[2*ifn+1][0];
				pDC->DrawText( str1, -1, &r, 0 );
				r.top += FKEY_R_H/2 - 2;
				pDC->DrawText( str2, -1, &r, 0 );
			}
		}
	}
	pDC->SelectObject( old_font );
	ReleaseDC( pDC );
}

void CFreePcbView::ShowRelativeDistance( int dx, int dy )
{
	CString str;
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	double d = sqrt( (double)dx*dx + (double)dy*dy );  
	if( m_Doc->m_units == MIL )
		str.Format( "dx = %.1f, dy = %.1f, d = %.2f", 
		(double)dx/NM_PER_MIL, (double)dy/NM_PER_MIL, d/NM_PER_MIL );
	else
		str.Format( "dx = %.3f, dy = %.3f, d = %.3f", dx/1000000.0, dy/1000000.0, d/1000000.0 );
	pMain->DrawStatus( 3, &str );
}

void CFreePcbView::ShowRelativeDistance( int x, int y, int dx, int dy )
{
	CString str;
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	double d = sqrt( (double)dx*dx + (double)dy*dy );  
	if( m_Doc->m_units == MIL )
		str.Format( "x = %.1f, y = %.1f, dx = %.1f, dy = %.1f, d = %.2f",
		(double)x/NM_PER_MIL, (double)y/NM_PER_MIL,
		(double)dx/NM_PER_MIL, (double)dy/NM_PER_MIL, d/NM_PER_MIL );
	else
		str.Format( "x = %.3f, y = %.3f, dx = %.3f, dy = %.3f, d = %.3f", 
		x/1000000.0, y/1000000.0,
		dx/1000000.0, dy/1000000.0, d/1000000.0 );
	pMain->DrawStatus( 3, &str );
}

// display selected item in status bar
//
int CFreePcbView::ShowSelectStatus()
{
#define SHOW_UIDS	// show UIDs for selected element
	CString uid_str;
#ifdef SHOW_UIDS
	if( m_sel_id.U3() != -1 )
		uid_str.Format( ",uid %d %d %d", m_sel_id.U1() , m_sel_id.U2(), m_sel_id.U3() );
	else if( m_sel_id.U2() != -1 && m_sel_id.U2() != m_sel_id.U1() )
		uid_str.Format( ",uid %d %d", m_sel_id.U1() , m_sel_id.U2() );
	else
		uid_str.Format( ",uid %d", m_sel_id.U1() );
#endif

	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	int u = m_Doc->m_units;
	CString x_str, y_str, w_str, hole_str, via_w_str, via_hole_str;
	CString str;

	switch( m_cursor_mode )
	{
	case CUR_NONE_SELECTED:
		str.Format( "No selection" );
		break;

	case CUR_DRE_SELECTED:
		str.Format( "DRE %s", m_sel_dre->str );
		break;

	case CUR_SMCUTOUT_CORNER_SELECTED:
		{
			CString lay_str;
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.I2()];
			if( poly->Layer() == LAY_SM_TOP )
				lay_str = "Top";
			else
				lay_str = "Bottom";
			::MakeCStringFromDimension( &x_str, poly->X(m_sel_id.I3()), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, poly->Y(m_sel_id.I3()), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			str.Format( "Solder mask cutout %d: %s, corner %d, x %s, y %s%s",
				m_sel_id.I2()+1, lay_str, m_sel_id.I3()+1, x_str, y_str, uid_str );
		}
		break;

	case CUR_SMCUTOUT_SIDE_SELECTED:
		{
			CString style_str;
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.I2()];
			if( poly->SideStyle( m_sel_id.I3() ) == CPolyLine::STRAIGHT )
				style_str = "straight";
			else if( poly->SideStyle( m_sel_id.I3() ) == CPolyLine::ARC_CW )
				style_str = "arc(cw)";
			else if( poly->SideStyle( m_sel_id.I3() ) == CPolyLine::ARC_CCW )
				style_str = "arc(ccw)";
			CString lay_str;
			if( poly->Layer() == LAY_SM_TOP )
				lay_str = "Top";
			else
				lay_str = "Bottom";
			str.Format( "Solder mask cutout %d: %s, side %d of %d, %s%s",
				m_sel_id.I2()+1, lay_str, m_sel_id.I3()+1,
				poly->NumCorners(), style_str, uid_str );
		}
		break;

	case CUR_BOARD_CORNER_SELECTED:
		::MakeCStringFromDimension( &x_str, m_Doc->m_board_outline[m_sel_id.I2()].X(m_sel_id.I3()), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		::MakeCStringFromDimension( &y_str, m_Doc->m_board_outline[m_sel_id.I2()].Y(m_sel_id.I3()), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		str.Format( "board outline %d, corner %d, x %s, y %s%s",
			m_sel_id.I2()+1, m_sel_id.I3()+1, x_str, y_str, uid_str );
		break;

	case CUR_BOARD_SIDE_SELECTED:
		{
			CString style_str;
			if( m_Doc->m_board_outline[m_sel_id.I2()].SideStyle( m_sel_id.I3() ) == CPolyLine::STRAIGHT )
				style_str = "straight";
			else if( m_Doc->m_board_outline[m_sel_id.I2()].SideStyle( m_sel_id.I3() ) == CPolyLine::ARC_CW )
				style_str = "arc(cw)";
			else if( m_Doc->m_board_outline[m_sel_id.I2()].SideStyle( m_sel_id.I3() ) == CPolyLine::ARC_CCW )
				style_str = "arc(ccw)";
			str.Format( "board outline %d, side %d of %d, %s%s", m_sel_id.I2()+1, m_sel_id.I3()+1,
				m_Doc->m_board_outline[m_sel_id.I2()].NumCorners(), style_str, uid_str );
		}
		break;

	case CUR_PART_SELECTED:
		{
			CString side = "top"; 
			if( m_sel_part->side ) 
				side = "bottom";
			::MakeCStringFromDimension( &x_str, m_sel_part->x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, m_sel_part->y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			int rep_angle = ::GetReportedAngleForPart( m_sel_part->angle, 
				m_sel_part->shape->m_centroid_angle, m_sel_part->side );
			str.Format( "part %s \"%s\", x %s, y %s, angle %d, %s%s",
				m_sel_part->ref_des, m_sel_part->shape->m_name, 
				x_str, y_str, 
				rep_angle,
				side,
				uid_str );
		}
		break;

	case CUR_REF_SELECTED:
		str.Format( "ref text: %s%s", m_sel_part->ref_des, uid_str ); 
		break;

	case CUR_VALUE_SELECTED:
		str.Format( "value: %s%s", m_sel_part->value, uid_str );
		break;

	case CUR_PAD_SELECTED:
		{
			cnet * pin_net = (cnet*)m_sel_part->pin[m_sel_id.I2()].net;
			::MakeCStringFromDimension( &x_str, m_sel_part->pin[m_sel_id.I2()].x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, m_sel_part->pin[m_sel_id.I2()].y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			if( pin_net )
			{
				// pad attached to net
				str.Format( "pin %s.%s on net \"%s\", x %s, y %s%s",
					m_sel_part->ref_des,
					m_sel_part->shape->GetPinNameByIndex(m_sel_id.I2()),
					pin_net->name, x_str, y_str, uid_str );
			}
			else
			{
				// pad not attached to a net
				str.Format( "pin %s.%s unconnected, x %s, y %s%s",
					m_sel_part->ref_des,
					m_sel_part->shape->GetPinNameByIndex(m_sel_id.I2()),
					x_str, y_str, uid_str );
			}
		}
		break;

	case CUR_SEG_SELECTED:
	case CUR_RAT_SELECTED:
	case CUR_DRAG_STUB:
	case CUR_DRAG_RAT:
		{
			CString con_str, seg_str;
			m_sel_id.Con()->GetStatusStr( &con_str );
			cseg * s = m_sel_id.Seg();
			if( s )
			{
				m_sel_id.Seg()->GetStatusStr( &seg_str );
				str = con_str + ", " + seg_str;
			}
			else
			{
				str = con_str;
			}
		}
		break;

	case CUR_VTX_SELECTED:
	case CUR_END_VTX_SELECTED:
		{
			CString con_str;
			m_sel_id.Con()->GetStatusStr( &con_str );
			CString vtx_str;
			cvertex * v = m_sel_id.Vtx();
			if( v )
			{
				m_sel_id.Vtx()->GetStatusStr( &vtx_str );
				str = con_str + ", " + vtx_str;
			}
			else
			{
				str = con_str;
			}
		}
		break;

	case CUR_CONNECT_SELECTED:
		{
			CString con_str;
			m_sel_id.Con()->GetStatusStr( &con_str );
			// get length of trace
			CString len_str;
			double len = 0;
			double last_x = m_sel_con->VtxByIndex(0).x;
			double last_y = m_sel_con->VtxByIndex(0).y;
			for( int iv=1; iv<=m_sel_con->NumSegs(); iv++ )
			{
				double x = m_sel_id.Con()->VtxByIndex(iv).x;
				double y = m_sel_id.Con()->VtxByIndex(iv).y;
				len += sqrt( (x-last_x)*(x-last_x) + (y-last_y)*(y-last_y) );
				last_x = x;
				last_y = y;
			}
			::MakeCStringFromDimension( &len_str, (int)len, u, TRUE, TRUE, FALSE, u==MIL?1:3 );
			str = con_str + ", length " + len_str;
		}
		break;

	case CUR_NET_SELECTED:
		str.Format( "net \"%s\"", m_sel_net->name );
		break;

	case CUR_TEXT_SELECTED:
		{
			CString neg_str = "";
			if( m_sel_text->m_bNegative )
				neg_str = "(NEG)";
			str.Format( "Text \"%s\" %s,%s", m_sel_text->m_str, neg_str, uid_str ); 
			break;
		}

	case CUR_AREA_CORNER_SELECTED:
		{
			CPoint p = m_Doc->m_nlist->GetAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
			::MakeCStringFromDimension( &x_str, p.x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, p.y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			str.Format( "\"%s\" copper area %d corner %d, x %s, y %s%s",
				m_sel_net->name, m_sel_id.I2()+1, m_sel_id.I3()+1,
				x_str, y_str, uid_str );
		}
		break;

	case CUR_AREA_SIDE_SELECTED:
		{
			int ic = m_sel_id.I3();
			int ia = m_sel_id.I2();
			CPolyLine * p = &m_sel_net->area[ia];
			int ncont = p->Contour(ic);
			if( ncont == 0 )
				str.Format( "\"%s\" copper area %d edge %d%s", 
				m_sel_net->name, ia+1, ic+1, uid_str );
			else
			{
				str.Format( "\"%s\" copper area %d cutout %d edge %d%s",
					m_sel_net->name, ia+1, ncont, ic+1-p->ContourStart(ncont),
					uid_str );
			}
		}
		break;

	case CUR_GROUP_SELECTED:
		str.Format( "Group selected" );
		break;

	case CUR_ADD_BOARD:
		str.Format( "Placing first corner of board outline" );
		break;

	case CUR_DRAG_BOARD_1:
		str.Format( "Placing second corner of board outline" );
		break;

	case CUR_DRAG_BOARD:
		str.Format( "Placing corner %d of board outline", m_sel_id.I3()+2 );
		break;

	case CUR_DRAG_BOARD_INSERT:
		str.Format( "Inserting corner %d of board outline", m_sel_id.I3()+2 );
		break;

	case CUR_DRAG_BOARD_MOVE:
		str.Format( "Moving corner %d of board outline", m_sel_id.I3()+1 );
		break;

	case CUR_DRAG_PART:
		str.Format( "Moving part %s", m_sel_part->ref_des );
		break;

	case CUR_DRAG_REF:
		str.Format( "Moving ref text for part %s", m_sel_part->ref_des );
		break;

	case CUR_DRAG_VTX:
		str.Format( "Routing net \"%s\"", m_sel_net->name );
		break;

	case CUR_DRAG_END_VTX:
		str.Format( "Routing net \"%s\"", m_sel_net->name );
		break;

	case CUR_DRAG_TEXT:
		str.Format( "Moving text \"%s\"", m_sel_text->m_str );
		break;

	case CUR_ADD_AREA:
		str.Format( "Placing first corner of copper area" );
		break;

	case CUR_DRAG_AREA_1:
		str.Format( "Placing second corner of copper area" );
		break;

	case CUR_DRAG_AREA:
		str.Format( "Placing corner %d of copper area", m_sel_id.I3()+1 );
		break;

	case CUR_DRAG_AREA_INSERT:
		str.Format( "Inserting corner %d of copper area", m_sel_id.I3()+2 );
		break;

	case CUR_DRAG_AREA_MOVE:
		str.Format( "Moving corner %d of copper area", m_sel_id.I3()+1 );
		break;

	case CUR_DRAG_CONNECT:
		if( m_sel_id.T1() == ID_PART )
			str.Format( "Adding connection to pin \"%s.%s",
			m_sel_part->ref_des,
			m_sel_part->shape->GetPinNameByIndex(m_sel_id.I2()) );
		else if( m_sel_id.T1() == ID_NET )
			str.Format( "Adding branch to trace \"%s.%d",
			m_sel_net->name,
			m_sel_id.I2() );
		break;

	case CUR_DRAG_MEASURE_1:
		str = "Measurement mode: left-click to start";
		break;

	}
	pMain->DrawStatus( 3, &str );
	return 0;
}

// display cursor coords in status bar
//
int CFreePcbView::ShowCursor()
{
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	CString str;
	CPoint p;
	p = m_last_cursor_point;
	if( m_Doc->m_units == MIL )  
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

// display active layer in status bar and change layer order for DisplayList
//
int CFreePcbView::ShowActiveLayer()
{
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	CString str;
	if( m_active_layer == LAY_TOP_COPPER )
		str.Format( "Top" );
	else if( m_active_layer == LAY_BOTTOM_COPPER )
		str.Format( "Bottom" );
	else if( m_active_layer > LAY_BOTTOM_COPPER )
		str.Format( "Inner %d", m_active_layer - LAY_BOTTOM_COPPER );
	pMain->DrawStatus( 4, &str );
	for( int order=LAY_TOP_COPPER; order<LAY_TOP_COPPER+m_Doc->m_num_copper_layers; order++ )
	{
		if( order == LAY_TOP_COPPER )
			m_dlist->SetLayerDrawOrder( m_active_layer, order );
		else if( order <= m_active_layer )
			m_dlist->SetLayerDrawOrder( order-1, order );
		else
			m_dlist->SetLayerDrawOrder( order, order );
	}
	Invalidate( FALSE );
	return 0;
}

// handle mouse scroll wheel
//
BOOL CFreePcbView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
#define MIN_WHEEL_DELAY 1.0

	static struct _timeb current_time;
	static struct _timeb last_time;
	static int first_time = 1;
	double diff;

	// ignore if cursor not in window
	CRect wr;
	GetWindowRect( wr );
	if( pt.x < wr.left || pt.x > wr.right || pt.y < wr.top || pt.y > wr.bottom )
		return CView::OnMouseWheel(nFlags, zDelta, pt);

	// ignore if we are dragging a selection rect
	if( m_bDraggingRect )
		return CView::OnMouseWheel(nFlags, zDelta, pt);

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
			GetCursorPos( &p );		// cursor pos in screen coords
			p = m_dlist->ScreenToPCB( p );	// convert to PCB coords
			m_pcbu_per_pixel = m_pcbu_per_pixel/ZOOM_RATIO;
			m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
			m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
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

// SelectPart...this is called from FreePcbDoc when a new part is added
// selects the new part as long as the cursor is not dragging something
//
int CFreePcbView::SelectPart( cpart * part )
{
	if(	!CurDragging() )
	{
		// deselect previously selected item
		if( m_sel_id.T1() )
			CancelSelection();

		// select part
		m_sel_part = part;
		m_sel_id = part->m_id;
		m_sel_id.SetT2( ID_SEL_RECT );
		m_Doc->m_plist->SelectPart( m_sel_part );
		SetCursorMode( CUR_PART_SELECTED );
	}
	gLastKeyWasArrow = FALSE;
	gLastKeyWasGroupRotate=false;
	Invalidate( FALSE );
	return 0;
}

// cancel selection
//
void CFreePcbView::CancelSelection()
{
	m_Doc->m_dlist->CancelHighLight();
	m_sel_ids.RemoveAll();
	m_sel_ptrs.RemoveAll();
	m_sel_id.Clear();
	SetCursorMode( CUR_NONE_SELECTED );
}

// attempt to reselect area corner based on position
// should be used after areas are modified
void CFreePcbView::TryToReselectAreaCorner( int x, int y )
{
	m_dlist->CancelHighLight();
	for( int ia=0; ia<m_sel_net->NumAreas(); ia++ )
	{
		for( int ic=0; ic<m_sel_net->area[ia].NumCorners(); ic++ )
		{
			carea * sel_a = & m_sel_net->area[ia];
			if( x == sel_a->X(ic)
				&& y == sel_a->Y(ic) )
			{
				// found matching corner
				m_sel_id = id(ID_NET, m_sel_net->UID(), ID_AREA, sel_a->UID(), ia, 
					ID_SEL_CORNER, sel_a->CornerUID(ic), ic);
				SetCursorMode( CUR_AREA_CORNER_SELECTED );
				m_Doc->m_nlist->HighlightAreaCorner( m_sel_net, ia, ic );
				return;
			}
		}
	}
	CancelSelection();
}


// set trace width using dialog
// enter with:
//	mode = 0 if called with segment selected
//	mode = 1 if called with connection selected
//	mode = 2 if called with net selected
//
int CFreePcbView::SetWidth( int mode )
{
	// set parameters for dialog
	DlgSetSegmentWidth dlg;
	dlg.m_w = &m_Doc->m_w;
	dlg.m_v_w = &m_Doc->m_v_w;
	dlg.m_v_h_w = &m_Doc->m_v_h_w;
	dlg.m_init_w = m_Doc->m_trace_w;
	dlg.m_init_via_w = m_Doc->m_via_w;
	dlg.m_init_via_hole_w = m_Doc->m_via_hole_w;
	if( mode == 0 )
	{
		cseg * seg = m_sel_seg;
		cconnect * con = m_sel_con;
		int seg_w = seg->width;
		if( seg_w )
			dlg.m_init_w = seg_w;
		else if( m_sel_net->def_w )
			dlg.m_init_w = m_sel_net->def_w;
	}
	else
	{
		if( m_sel_net->def_w )
			dlg.m_init_w = m_sel_net->def_w;
	}

	// launch dialog
	dlg.m_mode = mode;
	int ret = dlg.DoModal();
	int w = 0;
	int via_w = 0;
	int via_hole_w = 0;
	if( ret == IDOK )
	{
		// returned with "OK"
		w = dlg.m_width;
		via_w = dlg.m_via_width;
		via_hole_w = dlg.m_hole_width;
		if( dlg.m_tv == 3 )
			w = 0;
		else if( dlg.m_tv == 2 )
			via_w = 0;
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );

		// set default values for net or connection
		if( dlg.m_def == 2 )
		{
			// set default for net
			if( w )
				m_sel_net->def_w = w;
			if( via_w )
			{
				m_sel_net->def_via_w = via_w;
				m_sel_net->def_via_hole_w = via_hole_w;
			}
		}
		// apply new widths to net, connection or segment
		if( dlg.m_apply == 3 )
		{
			// apply width to net
			m_Doc->m_nlist->SetNetWidth( m_sel_net, w, via_w, via_hole_w );
		}
		else if( dlg.m_apply == 2 )
		{
			// apply width to connection
			m_Doc->m_nlist->SetConnectionWidth( m_sel_net, m_sel_ic, w, via_w, via_hole_w );
		}
		else if( dlg.m_apply == 1 )
		{
			// apply width to segment
			m_Doc->m_nlist->SetSegmentWidth( m_sel_net, m_sel_ic,
				m_sel_id.I3(), w, via_w, via_hole_w );
		}
	}
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
	return 0;
}

// Get trace and via widths
// tries default widths for net, then board
//
int CFreePcbView::GetWidthsForSegment( int * w, int * via_w, int * via_hole_w )
{
	*w = m_Doc->m_trace_w;
	if( m_sel_net->def_w )
		*w = m_sel_net->def_w;

	*via_w = m_Doc->m_via_w;
	if( m_sel_net->def_via_w )
		*via_w = m_sel_net->def_via_w;

	*via_hole_w = m_Doc->m_via_hole_w;
	if( m_sel_net->def_via_hole_w )
		*via_hole_w = m_sel_net->def_via_hole_w;

	return 0;
}

// context-sensitive menu invoked by right-click
//
void CFreePcbView::OnContextMenu(CWnd* pWnd, CPoint point )
{
	if( m_disable_context_menu )
	{
		// right-click already handled, don't pop up menu
		m_disable_context_menu = 0;
		return;
	}
	if( !m_Doc->m_project_open )	// no project open
		return;

	// OK, pop-up context menu
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_CONTEXT));
	CMenu* pPopup;
	int style;
	switch( m_cursor_mode )
	{
	case CUR_NONE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_NONE);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_BOARD_CORNER_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_BOARD_CORNER);
		ASSERT(pPopup != NULL);
		if( m_Doc->m_board_outline[m_sel_id.I2()].NumCorners() < 4 )
				pPopup->EnableMenuItem( ID_BOARDCORNER_DELETECORNER, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_BOARD_SIDE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_BOARD_SIDE);
		ASSERT(pPopup != NULL);
		style = m_Doc->m_board_outline[m_sel_id.I2()].SideStyle( m_sel_id.I3() );
		if( style == CPolyLine::STRAIGHT )
		{
			int xi = m_Doc->m_board_outline[m_sel_id.I2()].X( m_sel_id.I3() );
			int yi = m_Doc->m_board_outline[m_sel_id.I2()].Y( m_sel_id.I3() );
			int xf, yf;
			if( m_sel_id.I3() != (m_Doc->m_board_outline[m_sel_id.I2()].NumCorners()-1) )
			{
				xf = m_Doc->m_board_outline[m_sel_id.I2()].X( m_sel_id.I3()+1 );
				yf = m_Doc->m_board_outline[m_sel_id.I2()].Y( m_sel_id.I3()+1 );
			}
			else
			{
				xf = m_Doc->m_board_outline[m_sel_id.I2()].X( 0 );
				yf = m_Doc->m_board_outline[m_sel_id.I2()].Y( 0 );
			}
			if( xi == xf || yi == yf )
			{
				pPopup->EnableMenuItem( ID_BOARDSIDE_CONVERTTOARC_CW, MF_GRAYED );
				pPopup->EnableMenuItem( ID_BOARDSIDE_CONVERTTOARC_CCW, MF_GRAYED );
			}
			pPopup->EnableMenuItem( ID_BOARDSIDE_CONVERTTOSTRAIGHTLINE, MF_GRAYED );
		}
		else if( style == CPolyLine::ARC_CW )
		{
			pPopup->EnableMenuItem( ID_BOARDSIDE_CONVERTTOARC_CW, MF_GRAYED );
			pPopup->EnableMenuItem( ID_BOARDSIDE_INSERTCORNER, MF_GRAYED );
		}
		else if( style == CPolyLine::ARC_CCW )
		{
			pPopup->EnableMenuItem( ID_BOARDSIDE_CONVERTTOARC_CCW, MF_GRAYED );
			pPopup->EnableMenuItem( ID_BOARDSIDE_INSERTCORNER, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_PART_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_PART);
		ASSERT(pPopup != NULL);
		if( m_sel_part->glued )
			pPopup->EnableMenuItem( ID_PART_GLUE, MF_GRAYED );
		else
			pPopup->EnableMenuItem( ID_PART_UNGLUE, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_REF_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_REF_TEXT);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_VALUE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_VALUE_TEXT);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_PAD_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_PAD);
		ASSERT(pPopup != NULL);
		if( m_sel_part->pin[m_sel_id.I2()].net )
			pPopup->EnableMenuItem( ID_PAD_ADDTONET, MF_GRAYED );
		else
			pPopup->EnableMenuItem( ID_PAD_DETACHFROMNET, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_SEG_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_SEGMENT);
		ASSERT(pPopup != NULL);

		if(!SegmentMovable())
				pPopup->EnableMenuItem( ID_SEGMENT_MOVE, MF_GRAYED );

		if( m_sel_con->end_pin == cconnect::NO_END
			&& m_sel_con->VtxByIndex(m_sel_con->NumSegs()).tee_ID == 0
			&& m_sel_con->VtxByIndex(m_sel_con->NumSegs()).force_via_flag == 0
			)
		{
			pPopup->EnableMenuItem( ID_SEGMENT_UNROUTETRACE, MF_GRAYED );
		}
		if( m_sel_con->end_pin == cconnect::NO_END
			&& m_sel_con->NumSegs() == (m_sel_id.I3()+1)
			&& m_sel_con_last_vtx->tee_ID == 0
			&& m_sel_con_last_vtx->force_via_flag == 0
			)
		{
			// last segment of stub trace unless a tee or via
			pPopup->EnableMenuItem( ID_SEGMENT_UNROUTE, MF_GRAYED );
		}
		if( m_sel_con->end_pin != cconnect::NO_END
			|| m_sel_con->NumSegs() > (m_sel_id.I3()+1)
			)
		{
			pPopup->EnableMenuItem( ID_SEGMENT_DELETE, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_RAT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_RATLINE);
		ASSERT(pPopup != NULL);
		if( m_sel_con->locked )
			pPopup->EnableMenuItem( ID_RATLINE_LOCKCONNECTION, MF_GRAYED );
		else
			pPopup->EnableMenuItem( ID_RATLINE_UNLOCKCONNECTION, MF_GRAYED );
		if( m_sel_con->end_pin == cconnect::NO_END )
			pPopup->EnableMenuItem( ID_SEGMENT_UNROUTETRACE, MF_GRAYED );
		if( m_sel_con->NumSegs() == 1
			|| !(m_sel_id.I3() == 0 || m_sel_id.I3() == (m_sel_con->NumSegs()-1) ) )
			pPopup->EnableMenuItem( ID_RATLINE_CHANGEPIN, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_VTX_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_VERTEX);
		ASSERT(pPopup != NULL);
		if( m_sel_vtx->via_w == 0 )
			pPopup->EnableMenuItem( ID_VERTEX_SETSIZE, MF_GRAYED );
		if( m_sel_con->end_pin == cconnect::NO_END
			&& m_sel_con_last_vtx->tee_ID == 0
			&& m_sel_con_last_vtx->force_via_flag == 0
			)
		{
			pPopup->EnableMenuItem( ID_VERTEX_UNROUTETRACE, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_END_VTX_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_END_VERTEX);
		ASSERT(pPopup != NULL);
		if( m_sel_vtx->via_w )
			pPopup->EnableMenuItem( ID_ENDVERTEX_ADDVIA, MF_GRAYED );
		else
		{
			pPopup->EnableMenuItem( ID_ENDVERTEX_SETSIZE, MF_GRAYED );
			pPopup->EnableMenuItem( ID_ENDVERTEX_REMOVEVIA, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_CONNECT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_CONNECT);
		ASSERT(pPopup != NULL);
		if( m_sel_con->end_pin == cconnect::NO_END )
			pPopup->EnableMenuItem( ID_CONNECT_UNROUTETRACE, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_NET_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_NET);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_TEXT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_TEXT);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_AREA_CORNER_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_AREA_CORNER);
		ASSERT(pPopup != NULL);
		{
			carea * area = &m_sel_net->area[m_sel_ia];
			if( area->Contour( m_sel_id.I3() ) == 0 )
				pPopup->EnableMenuItem( ID_AREACORNER_DELETECUTOUT, MF_GRAYED );
			else
				pPopup->EnableMenuItem( ID_AREACORNER_ADDCUTOUT, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_AREA_SIDE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_AREA_EDGE);
		ASSERT(pPopup != NULL);
		{
			carea * area = &m_sel_net->area[m_sel_ia];
			if( area->Contour( m_sel_id.I3() ) == 0 )
				pPopup->EnableMenuItem( ID_AREAEDGE_DELETECUTOUT, MF_GRAYED );
			else
				pPopup->EnableMenuItem( ID_AREAEDGE_ADDCUTOUT, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_SMCUTOUT_SIDE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_SM_SIDE);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_SMCUTOUT_CORNER_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_SM_CORNER);
		ASSERT(pPopup != NULL);
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.I2()];
			if( poly->NumCorners() < 4 )
				pPopup->EnableMenuItem( ID_SMCORNER_DELETECORNER, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_GROUP_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_GROUP);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;
	}
}

// add copper area
//
void CFreePcbView::OnAddArea()
{
	CDlgAddArea dlg;
	dlg.Initialize( m_Doc->m_nlist, m_Doc->m_num_layers, NULL, m_active_layer, -1 );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		if( !dlg.m_net )
		{
			CString str;
			str.Format( "Net \"%s\" not found", dlg.m_net_name );
			AfxMessageBox( str, MB_OK );
		}
		else
		{
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			m_dlist->CancelHighLight();
			SetCursorMode( CUR_ADD_AREA );
			// make layer visible
			m_active_layer = dlg.m_layer;
			m_Doc->m_vis[m_active_layer] = TRUE;
			m_dlist->SetLayerVisible( m_active_layer, TRUE );
			ShowActiveLayer();
			m_sel_net = dlg.m_net;
			m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
				m_last_cursor_point.y, 0, m_active_layer, 2 );
			m_polyline_style = CPolyLine::STRAIGHT;
			m_polyline_hatch = dlg.m_hatch;
			Invalidate( FALSE );
			ReleaseDC( pDC );
		}
	}
}

// add copper area cutout
//
void CFreePcbView::OnAreaAddCutout()
{
	// check if any non-straight sides
	BOOL bArcs = FALSE;
	CPolyLine * poly = &m_sel_net->area[m_sel_ia];
	int ns = poly->NumCorners();
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dlist->CancelHighLight();
	SetCursorMode( CUR_ADD_AREA_CUTOUT );
	// make layer visible
	m_active_layer = m_sel_net->area[m_sel_ia].Layer();
	m_Doc->m_vis[m_active_layer] = TRUE;
	m_dlist->SetLayerVisible( m_active_layer, TRUE );
	ShowActiveLayer();
	m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
		m_last_cursor_point.y, 0, m_active_layer, 2 );
	m_polyline_style = CPolyLine::STRAIGHT;
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

void CFreePcbView::OnAreaDeleteCutout()
{
	CPolyLine * poly = &m_sel_net->area[m_sel_ia];
	int icont = poly->Contour( m_sel_id.I3() );
	if( icont < 1 )
		ASSERT(0);
	SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
	poly->RemoveContour( icont );
	CancelSelection();
	m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
	if( m_Doc->m_vis[LAY_RAT_LINE] )
		m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
	Invalidate( FALSE );
}

// move part
//
void CFreePcbView::OnPartMove()
{
	// check for glue
	if( m_sel_part->glued )
	{
		int ret = AfxMessageBox( "This part is glued, do you want to unglue it ?  ", MB_YESNO );
		if( ret != IDYES )
			return;
	}
	// drag part
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to part origin
	CPoint p;
	p.x  = m_sel_part->x;
	p.y  = m_sel_part->y;
	m_from_pt = p;
	CPoint cur_p = m_dlist->PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start dragging
	BOOL bRatlines = m_Doc->m_vis[LAY_RAT_LINE];
	m_Doc->m_plist->StartDraggingPart( pDC, m_sel_part, bRatlines, 
					m_Doc->m_auto_ratline_disable, m_Doc->m_auto_ratline_min_pins );
	SetCursorMode( CUR_DRAG_PART );
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

// add text string
//
void CFreePcbView::OnTextAdd()
{
	// create, initialize and show dialog
	CDlgAddText add_text_dlg;
	CString str = "";
	add_text_dlg.Initialize( 0, m_Doc->m_num_layers, 1, &str, m_Doc->m_units,
			LAY_SILK_TOP, 0, 0, 0, 0, 0, 0, 0 );
	add_text_dlg.m_num_layers = m_Doc->m_num_layers;
	add_text_dlg.m_bDrag = 1;
	// defaults for dialog
	int ret = add_text_dlg.DoModal();
	if( ret == IDCANCEL )
		return;
	else
	{
		int x = add_text_dlg.m_x;
		int y = add_text_dlg.m_y;
		int mirror = add_text_dlg.m_bMirror;
		BOOL bNegative = add_text_dlg.m_bNegative;
		int angle = add_text_dlg.m_angle;
		int font_size = add_text_dlg.m_height;
		int stroke_width = add_text_dlg.m_width;
		int layer = add_text_dlg.m_layer;
		CString str = add_text_dlg.m_str;
		m_Doc->m_vis[layer] = TRUE;
		m_dlist->SetLayerVisible( layer, TRUE );

		// get cursor position and convert to PCB coords
		CPoint p;
		GetCursorPos( &p );		// cursor pos in screen coords
		p = m_dlist->ScreenToPCB( p );	// convert to PCB coords
		// set pDC to PCB coords
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		if( add_text_dlg.m_bDrag )
		{
			m_sel_text = m_Doc->m_tlist->AddText( p.x, p.y, angle, mirror, bNegative,
				layer, font_size, stroke_width, &str );
			m_dragging_new_item = 1;
			m_Doc->m_tlist->StartDraggingText( pDC, m_sel_text );
			SetCursorMode( CUR_DRAG_TEXT );
		}
		else
		{
			m_sel_text = m_Doc->m_tlist->AddText( x, y, angle, mirror, bNegative,
				layer, font_size,  stroke_width, &str );
			SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_ADD, TRUE, m_Doc->m_undo_list );
			m_Doc->m_tlist->HighlightText( m_sel_text );
		}
		ReleaseDC( pDC );
		Invalidate( FALSE );
	}
}

// delete text ... enter with text selected
//
void CFreePcbView::OnTextDelete()
{
	SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_DELETE, TRUE, m_Doc->m_undo_list );
	m_Doc->m_tlist->RemoveText( m_sel_text );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// move text, enter with text selected
//
void CFreePcbView::OnTextMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to text origin
	CPoint p;
	p.x  = m_sel_text->m_x;
	p.y  = m_sel_text->m_y;
	CPoint cur_p = m_dlist->PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start moving
	m_dlist->CancelHighLight();
	m_dragging_new_item = 0;
	m_Doc->m_tlist->StartDraggingText( pDC, m_sel_text );
	SetCursorMode( CUR_DRAG_TEXT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// glue part
//
void CFreePcbView::OnPartGlue()
{
	SaveUndoInfoForPart( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_sel_part->glued = 1;
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
}

// unglue part
//
void CFreePcbView::OnPartUnglue()
{
	SaveUndoInfoForPart( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_sel_part->glued = 0;
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
}

// delete part
//
void CFreePcbView::OnPartDelete()
{
	// delete part
	CString mess;
	mess.Format( "Deleting part %s\nDo you wish to remove all references\nto this part from netlist ?",
		m_sel_part->ref_des );
	int ret = AfxMessageBox( mess, MB_YESNOCANCEL );
	if( ret == IDCANCEL )
		return;
	// save undo info
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_DELETE, NULL, TRUE, m_Doc->m_undo_list );
	// now do it
	if( ret == IDYES )
		m_Doc->m_nlist->PartDeleted( m_sel_part, TRUE );
	else if( ret == IDNO )
		m_Doc->m_nlist->PartDisconnected( m_sel_part, TRUE );
	m_Doc->m_plist->Remove( m_sel_part );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// optimize all nets to part
//
void CFreePcbView::OnPartOptimize()
{
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->OptimizeConnections( m_sel_part, FALSE, -1 );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// move ref. designator text for part
//
void CFreePcbView::OnRefMove()
{
	// move reference ID
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to part origin
	CPoint cur_p = m_dlist->PCBToScreen( m_last_cursor_point );
	SetCursorPos( cur_p.x, cur_p.y );
	m_dragging_new_item = 0;
	m_Doc->m_plist->StartDraggingRefText( pDC, m_sel_part );
	SetCursorMode( CUR_DRAG_REF );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// optimize net for this pad
//
void CFreePcbView::OnPadOptimize()
{
	cnet * pin_net = (cnet*)m_sel_part->pin[m_sel_id.I2()].net;
	if( pin_net )
	{
		m_Doc->m_nlist->OptimizeConnections( pin_net, -1, FALSE, -1, FALSE );
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

// start stub trace from this pad
//
void CFreePcbView::OnPadStartStubTrace()
{
	cnet * net = (cnet*)m_sel_part->pin[m_sel_id.I2()].net;
	if( net == NULL )
	{
		AfxMessageBox( "Pad must be assigned to a net before adding trace", MB_OK );
		return;
	}
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint pi = m_last_cursor_point;
	CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.I2() );
	CPoint p = m_Doc->m_plist->GetPinPoint( m_sel_part, pin_name );

	// force to layer of pad if SMT
	if( m_sel_part->shape->m_padstack[m_sel_id.I2()].hole_size == 0 )
	{
		m_active_layer = m_Doc->m_plist->GetPinLayer( m_sel_part, &pin_name );
		ShowActiveLayer();
	}

	// find starting pin in net
	int p1 = -1;
	for( int ip=0; ip<net->NumPins(); ip++ )
	{
		if( net->pin[ip].part == m_sel_part )
		{
			if( net->pin[ip].pin_name == m_sel_part->shape->GetPinNameByIndex( m_sel_id.I2() ) )
			{
				// found starting pin in net
				p1 = ip;
			}
		}
	}
	if( p1 == -1 )
		ASSERT(0);		// starting pin not found in net

	// add connection for stub trace
	SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_sel_net = net;
	m_sel_id.Set( ID_NET, net->UID(), ID_CONNECT, -1, 0, ID_SEL_SEG, -1, 0 );
	m_sel_id.SetI2( net->AddConnectFromPin( p1 ) );
	cconnect * c = net->ConByIndex( m_sel_id.I2() );
	m_sel_id.SetU2( c->UID() );

	// start dragging line
	int w = m_Doc->m_trace_w;
	if( net->def_w )
		w = net->def_w;
	int via_w = m_Doc->m_via_w;
	if( net->def_via_w )
		via_w = net->def_via_w;
	int via_hole_w = m_Doc->m_via_hole_w;
	if( net->def_via_hole_w )
		via_hole_w = net->def_via_hole_w;
	m_Doc->m_nlist->StartDraggingStub( pDC, net, m_sel_id.I2(), m_sel_id.I3(),
		pi.x, pi.y, m_active_layer, w, m_active_layer, via_w, via_hole_w,
		2, m_inflection_mode );
	m_snap_angle_ref = p;
	SetCursorMode( CUR_DRAG_STUB );
	m_dlist->CancelHighLight();
	ShowSelectStatus();
	m_Doc->ProjectModified( TRUE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// attach this pad to a net
//
void CFreePcbView::OnPadAddToNet()
{
	DlgAssignNet assign_net_dlg;
	assign_net_dlg.m_map = &m_Doc->m_nlist->m_map;
	int ret = assign_net_dlg.DoModal();
	if( ret == IDOK )
	{
		CString name = assign_net_dlg.m_net_str;
		void * ptr;
		cnet * new_net = 0;
		int test = m_Doc->m_nlist->m_map.Lookup( name, ptr );
		if( !test )
		{
			// create new net if legal string
			name.Trim();
			if( name.GetLength() )
			{
				new_net = m_Doc->m_nlist->AddNet( (char*)(LPCTSTR)name, 10, 0, 0, 0 );
				SaveUndoInfoForNetAndConnections( new_net, CNetList::UNDO_NET_ADD, TRUE, m_Doc->m_undo_list );
			}
			else
			{
				// blank net name
				AfxMessageBox( "Illegal net name" );
				return;
			}
		}
		else
		{
			// use selected net
			new_net = (cnet*)ptr;
			SaveUndoInfoForNetAndConnections( new_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		}
		// assign pin to net
		if( new_net )
		{
			SaveUndoInfoForPart( m_sel_part,
				CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_Doc->m_undo_list );
			CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.I2() );
			new_net->AddPin( &m_sel_part->ref_des, &pin_name );
			if( m_Doc->m_vis[LAY_RAT_LINE] )
				m_Doc->m_nlist->OptimizeConnections(  new_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
			SetFKText( m_cursor_mode );
		}
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

// remove this pad from net
//
void CFreePcbView::OnPadDetachFromNet()
{
	cnet * pin_net = (cnet*)m_sel_part->pin[m_sel_id.I2()].net;
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	CString pin_name = m_sel_part->shape->GetPinNameByIndex(m_sel_id.I2());
	cnet * net = m_Doc->m_plist->GetPinNet( m_sel_part, m_sel_id.I2() );
	m_Doc->m_nlist->RemoveNetPin( m_sel_part, &pin_name );
	m_Doc->m_nlist->RemoveOrphanBranches( net, 0 );
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// connect this pad to another pad
//
void CFreePcbView::OnPadConnectToPin()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.I2() );
	CPoint p = m_Doc->m_plist->GetPinPoint( m_sel_part, pin_name );
	m_dragging_new_item = 0;
	m_dlist->StartDraggingRatLine( pDC, 0, 0, p.x, p.y, LAY_RAT_LINE, 1, 1 );
	SetCursorMode( CUR_DRAG_CONNECT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// connect this vertex to another pad with a tee connection
//
void CFreePcbView::OnVertexConnectToPin()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dragging_new_item = 0;
	m_dlist->StartDraggingRatLine( pDC, 0, 0, m_sel_vtx->x, m_sel_vtx->y, LAY_RAT_LINE, 1, 1 );
	SetCursorMode( CUR_DRAG_CONNECT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

//100 start stub trace from vertex
//
void CFreePcbView::OnVertexStartStub()
{
	CDC * pDC = GetDC();
	SetDCToWorldCoords( pDC );
	pDC->SelectClipRgn( &m_pcb_rgn );
	CPoint p;
	p = m_last_cursor_point;
	m_sel_id.SetT3( ID_SEL_SEG );
	int w, via_w, via_hole_w;
	m_snap_angle_ref.x = m_sel_vtx->x;
	m_snap_angle_ref.y = m_sel_vtx->y;
	// now add new connection and select first vertex
	m_sel_id = m_sel_net->AddConnectFromVtx( m_sel_id );
	m_sel_id = m_sel_id.Con()->FirstVtx()->Id();
	GetWidthsForSegment( &w, &via_w, &via_hole_w );
	m_Doc->m_nlist->StartDraggingStub( pDC, m_sel_net, m_sel_ic, m_sel_iv,
		p.x, p.y, m_active_layer, w, m_active_layer, via_w, via_hole_w,
		2, m_inflection_mode );
	SetCursorMode( CUR_DRAG_STUB );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// set width for this segment (not a ratline)
//
void CFreePcbView::OnSegmentSetWidth()
{
	SetWidth( 0 );
	m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
	Invalidate( FALSE );
}

// unroute this segment, convert to a ratline
//
void CFreePcbView::OnSegmentUnroute()
{
	// save undo info for connection
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );

	// edit connection segment
//	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	// see if segments to pin also need to be unrouted
	// see if start vertex of this segment is in start pad of connection
	int x = m_sel_vtx->x;
	int y = m_sel_vtx->y;
	int layer = m_sel_seg->layer;
	BOOL test = m_Doc->m_nlist->TestHitOnConnectionEndPad( x, y, m_sel_net,
		m_sel_id.I2(), layer, 1 );
	if( test )
	{
		// unroute preceding segments
		for( int is=m_sel_id.I3()-1; is>=0; is-- )
			m_Doc->m_nlist->UnrouteSegmentWithoutMerge( m_sel_net, m_sel_ic, is );	}
	// see if end vertex of this segment is in end pad of connection
	x = m_sel_next_vtx->x;
	y = m_sel_next_vtx->y;
	test = m_Doc->m_nlist->TestHitOnConnectionEndPad( x, y, m_sel_net,
		m_sel_id.I2(), layer, 0 );
	if( test )
	{
		// unroute following segments
		for( int is=m_sel_con->NumSegs()-1; is>m_sel_id.I3(); is-- )
			m_Doc->m_nlist->UnrouteSegmentWithoutMerge( m_sel_net, m_sel_ic, is );
	}

	// unroute segment
	id id = m_Doc->m_nlist->UnrouteSegment( m_sel_net, m_sel_ic, m_sel_is );
	m_Doc->m_nlist->SetAreaConnections( m_sel_net );

	// reselect it if possible
	if( m_sel_id.Resolve() )
	{
		SelectItem( m_sel_id );
		SetCursorMode( CUR_RAT_SELECTED );
	}
	else
	{
		CancelSelection();
	}
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// delete this segment, only used for the last segment of a stub trace
//
void CFreePcbView::OnSegmentDelete()
{
	int id = m_sel_con_last_vtx->tee_ID;
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	if( id )
		m_Doc->m_nlist->DisconnectBranch( m_sel_net, m_sel_ic );
	m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_is, TRUE );
	m_Doc->m_nlist->SetAreaConnections( m_sel_net );
	CancelSelection();
	ShowSelectStatus();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// route this ratline
//
void CFreePcbView::OnRatlineRoute()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	int last_seg_layer = 0;
	int n_segs = m_sel_con->NumSegs();
	// get direction for routing, based on closest end of selected segment to cursor
	double d1x = p.x - m_sel_vtx->x;
	double d1y = p.y - m_sel_vtx->y;
	double d2x = p.x - m_sel_next_vtx->x;
	double d2y = p.y - m_sel_next_vtx->y;
	double d1 = d1x*d1x + d1y*d1y;
	double d2 = d2x*d2x + d2y*d2y;
	if( d1<d2 )
	{
		// route forward
		m_dir = 0;
		if( m_sel_id.I3() > 0 )
			last_seg_layer = m_sel_con->SegByIndex(m_sel_is-1).layer;
		m_snap_angle_ref.x = m_sel_vtx->x;
		m_snap_angle_ref.y = m_sel_vtx->y;
	}
	else
	{
		// route backward
		m_dir = 1;
		if( m_sel_id.I3() < (m_sel_con->NumSegs()-1) )
			last_seg_layer = m_sel_con->SegByIndex(m_sel_is+1).layer;
		m_snap_angle_ref.x = m_sel_next_vtx->x;
		m_snap_angle_ref.y = m_sel_next_vtx->y;
	}
	if( m_sel_id.I3() == 0 && m_dir == 0)
	{
		// first segment, force to layer of starting pad if SMT
		int p1 = m_sel_con->start_pin;
		cpart * p = m_sel_net->pin[p1].part;
		CString pin_name = m_sel_net->pin[p1].pin_name;
		int pin_index = p->shape->GetPinIndexByName( pin_name );
		if( p->shape->m_padstack[pin_index].hole_size == 0)
		{
			m_active_layer = m_Doc->m_plist->GetPinLayer( p, &pin_name );
			ShowActiveLayer();
		}
	}
	else if( m_sel_id.I3() == (n_segs-1) && m_dir == 1 )
	{
		// last segment, force to layer of starting pad if SMT
		int p1 = m_sel_con->end_pin;
		if( p1 != cconnect::NO_END )
		{
			cpart * p = m_sel_net->pin[p1].part;
			CString pin_name = m_sel_net->pin[p1].pin_name;
			int pin_index = p->shape->GetPinIndexByName( pin_name );
			if( p1 != cconnect::NO_END )
			{
				if( p->shape->m_padstack[pin_index].hole_size == 0)
				{
					m_active_layer = m_Doc->m_plist->GetPinLayer( p, &pin_name );
					ShowActiveLayer();
				}
			}
		}
	}
	// now start dragging new segment
	int w = m_Doc->m_trace_w;
	int via_w = m_Doc->m_via_w;
	int via_hole_w = m_Doc->m_via_hole_w;
	GetWidthsForSegment( &w, &via_w, &via_hole_w );
	m_dragging_new_item = 0;
	m_Doc->m_nlist->StartDraggingSegment( pDC, m_sel_net, m_sel_ic, m_sel_is,
		p.x, p.y, m_active_layer,
		LAY_SELECTION, w,
		last_seg_layer, via_w, via_hole_w, m_dir, 2 );
	SetCursorMode( CUR_DRAG_RAT );
	ReleaseDC( pDC );
}

// optimize this connection
//
void CFreePcbView::OnRatlineOptimize()
{
	int new_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, FALSE, -1, FALSE  );
	ReselectNetItemIfConnectionsChanged( new_ic );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// optimize this connection
//
void CFreePcbView::OnRatlineChangeEndPin()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dlist->CancelHighLight();
	cconnect * c = m_sel_con;
	m_dlist->Set_visible( m_sel_seg->dl_el, FALSE );
	int x, y;
	if( m_sel_id.I3() == 0 )
	{
		// ratline is first segment of connection
		x = c->VtxByIndex(1).x;
		y = c->VtxByIndex(1).y;
	}
	else
	{
		// ratline is last segment of connection
		x = m_sel_vtx->x;
		y = m_sel_vtx->y;
	}
	m_dlist->StartDraggingRatLine( pDC, 0, 0, x, y, LAY_RAT_LINE, 1, 1 );
	SetCursorMode( CUR_DRAG_RAT_PIN );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// change via properties
//
void CFreePcbView::OnVertexProperties()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	CPoint v_pt( m_sel_vtx->x, m_sel_vtx->y );
	CDlgVia dlg = new CDlgVia;
	dlg.Initialize( m_sel_vtx->via_w, m_sel_vtx->via_hole_w, v_pt, m_Doc->m_units );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_sel_vtx->via_w = dlg.m_via_w;
		m_sel_vtx->via_hole_w = dlg.m_via_hole_w;
		m_dlist->CancelHighLight();
		m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,
			dlg.pt().x, dlg.pt().y );
		m_Doc->ProjectModified( TRUE );
		m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
	}
	Invalidate( FALSE );
}

// move this vertex
//
void CFreePcbView::OnVertexMove()
{
//	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	id id = m_sel_id;
	int ic = m_sel_id.I2();
	int ivtx = m_sel_id.I3();
	m_dragging_new_item = 0;
	m_from_pt.x = m_sel_vtx->x;
	m_from_pt.y = m_sel_vtx->y;
	m_Doc->m_nlist->StartDraggingVertex( pDC, m_sel_net, ic, ivtx, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_VTX );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// delete this vertex
// i.e. unroute adjacent segments and reroute if on same layer
// stub trace needs some special handling
//
void CFreePcbView::OnVertexDelete()
{
	int end_via_w, end_via_hole_w;
	int ic = m_sel_id.I2();
	int iv = m_sel_id.I3();
	cconnect * c = m_sel_net->ConByIndex(ic);
	cvertex * v = &c->VtxByIndex(iv);

	// deal with tee-connections
	int tee_id = v->tee_ID;
	if( tee_id )
	{
		// this is a tee-connection
		int ret = AfxMessageBox( "You are deleting a tee-vertex\nContinue ?",
									MB_OKCANCEL );
		if( ret == IDCANCEL )
			return;
	}
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	v->tee_ID = 0;
	v->force_via_flag = 0;
//	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );

	if( c->end_pin == cconnect::NO_END && iv == c->NumSegs() )
	{
		// last vertex of stub trace, just delete last segment
		m_Doc->m_nlist->RemoveSegment( m_sel_net, ic, iv-1, TRUE );
		m_Doc->m_nlist->SetAreaConnections( m_sel_net );
	}
	else if( c->SegByIndex(iv-1).layer != c->SegByIndex(iv).layer
				|| c->SegByIndex(iv-1).layer == LAY_RAT_LINE )
	{
		// deleting vertex between two disimilar segments
		v->force_via_flag = 0;
		v->tee_ID = 0;
		m_Doc->m_nlist->UnrouteSegmentWithoutMerge( m_sel_net, m_sel_ic, iv-1 );
		m_Doc->m_nlist->UnrouteSegmentWithoutMerge( m_sel_net, m_sel_ic, iv );
		m_Doc->m_nlist->MergeUnroutedSegments( m_sel_net, m_sel_ic );
		m_Doc->m_nlist->SetAreaConnections( m_sel_net );
	}
	else
	{
		// deleting vertex between two similar segments, combine them
		// start by unrouting both adjacent segments
		// NOTE: this creates a problem if the segments pre or post
		// the adjacent segments are unrouted, since they will be combined
		// with the adjacent segments when unrouted
		// the solution is to temporarily put them on a different layer,
		// then restore them
		int pre_pre_layer = -1;
		int pre_pre_width = -1;
		int post_post_layer = -1;
		int post_post_width = -1;
		if( iv >= 2 )
		{
			// temporarily put pre_pre_segment on top copper layer
			pre_pre_layer = c->SegByIndex(iv-2).layer;
			pre_pre_width = c->SegByIndex(iv-2).width;
			c->SegByIndex(iv-2).layer = LAY_TOP_COPPER-1;
			c->SegByIndex(iv-2).width = 1;
		}
		if( iv < c->NumSegs()-1 )
		{
			// temporarily put post_post_segment on top copper layer
			post_post_layer = c->SegByIndex(iv+1).layer;
			post_post_width = c->SegByIndex(iv+1).width;
			c->SegByIndex(iv+1).layer = LAY_TOP_COPPER-1;
			c->SegByIndex(iv+1).width = 1;
		}

		// save preceding vertex parameters
		int pre_via_w = c->VtxByIndex(iv-1).via_w;
		int pre_via_hole_w = c->VtxByIndex(iv-1).via_hole_w;
		int pre_force_via_flag = c->VtxByIndex(iv-1).force_via_flag;
		int pre_tee_ID = c->VtxByIndex(iv-1).tee_ID;
		// save following vertex parameters
		int post_via_w = c->VtxByIndex(iv+1).via_w;
		int post_via_hole_w = c->VtxByIndex(iv+1).via_hole_w;
		int post_force_via_flag = c->VtxByIndex(iv+1).force_via_flag;
		int post_tee_ID = c->VtxByIndex(iv+1).tee_ID;
		// get adjacent segment width and layers
		int w = max( c->SegByIndex(iv-1).width, c->SegByIndex(iv-1).width );
		int pre_layer = c->SegByIndex(iv-1).layer;
		int post_layer = c->SegByIndex(iv).layer;
		m_Doc->m_nlist->UnrouteSegmentWithoutMerge( m_sel_net, ic, iv );
		m_Doc->m_nlist->UnrouteSegment( m_sel_net, ic, iv-1 );
		m_dlist->CancelHighLight();
		// now reroute if they were on the same layer
		BOOL bReroute = !m_Doc->m_nlist->RouteSegment( m_sel_net, ic, iv-1, pre_layer, w );
		ASSERT( bReroute );		// failed
		// reconstruct adjacent vias
		c->VtxByIndex(iv-1).via_w = pre_via_w;
		c->VtxByIndex(iv-1).via_hole_w = pre_via_hole_w;
		c->VtxByIndex(iv-1).force_via_flag = pre_force_via_flag;
		c->VtxByIndex(iv-1).tee_ID = pre_tee_ID;
		c->VtxByIndex(iv).via_w = post_via_w;
		c->VtxByIndex(iv).via_hole_w = post_via_hole_w;
		c->VtxByIndex(iv).force_via_flag = post_force_via_flag;
		c->VtxByIndex(iv).tee_ID = post_tee_ID;
		m_Doc->m_nlist->ReconcileVia( m_sel_net, ic, iv-1 );
		m_Doc->m_nlist->ReconcileVia( m_sel_net, ic, iv );

		// reconstruct segments next to adjacent segments
		if( pre_pre_layer != -1 )
		{
			c->SegByIndex(iv-2).layer = pre_pre_layer;
			c->SegByIndex(iv-2).width = pre_pre_width;
		}
		if( post_post_layer != -1 )
		{
			c->SegByIndex(iv).layer = post_post_layer;
			c->SegByIndex(iv).width = post_post_width;
		}
		m_Doc->m_nlist->MergeUnroutedSegments( m_sel_net, ic );
	}
	if( tee_id )
	{
		// if tee, remove it and any stubs
		m_Doc->m_nlist->RemoveTee( m_sel_net, tee_id );
		m_Doc->m_nlist->RemoveOrphanBranches( m_sel_net, tee_id, TRUE );
	}
	if( m_Doc->m_vis[LAY_RAT_LINE] )
		m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// move the end vertex of a stub trace
//
void CFreePcbView::OnEndVertexMove()
{
//	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	CDC * pDC = GetDC();
	SetDCToWorldCoords( pDC );
	pDC->SelectClipRgn( &m_pcb_rgn );
	CPoint p;
	p = m_last_cursor_point;
	SetCursorMode( CUR_DRAG_END_VTX );
	m_Doc->m_nlist->StartDraggingVertex( pDC, m_sel_net, m_sel_ic, m_sel_iv, p.x, p.y, 2 );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}


// force a via on end vertex
//
void CFreePcbView::OnEndVertexAddVia()
{
//	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->ForceVia( m_sel_net, m_sel_ic, m_sel_is );
	SetFKText( m_cursor_mode );
	if( m_Doc->m_vis[LAY_RAT_LINE] ) 
	{
		m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
		if( m_sel_id.Resolve() )
			SelectItem( m_sel_id );
		else
			CancelSelection();
	}
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// remove forced via on end vertex
//
void CFreePcbView::OnEndVertexRemoveVia()
{
//	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->UnforceVia( m_sel_net, m_sel_ic, m_sel_is, FALSE );
	if( m_sel_prev_seg->layer == LAY_RAT_LINE )
	{
		m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_is-1, TRUE );
		CancelSelection();
	}
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
	if( m_Doc->m_vis[LAY_RAT_LINE] )
	{
		m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
		if( m_sel_id.Resolve() )
			SelectItem( m_sel_id );
		else
			CancelSelection();
	}
	Invalidate( FALSE );
}

// append more segments to this stub trace
//
void CFreePcbView::OnEndVertexAddSegments()
{
//	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	CDC * pDC = GetDC();
	SetDCToWorldCoords( pDC );
	pDC->SelectClipRgn( &m_pcb_rgn );
	CPoint p;
	p = m_last_cursor_point;
	m_sel_id.SetT3( ID_SEL_SEG );
	int w, via_w, via_hole_w;
	m_snap_angle_ref.x = m_sel_vtx->x;
	m_snap_angle_ref.y = m_sel_vtx->y;
	GetWidthsForSegment( &w, &via_w, &via_hole_w );
	m_Doc->m_nlist->StartDraggingStub( pDC, m_sel_net, m_sel_ic, m_sel_is,
		p.x, p.y, m_active_layer, w, m_active_layer, via_w, via_hole_w,
		2, m_inflection_mode );
	SetCursorMode( CUR_DRAG_STUB );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// convert stub trace to regular connection by adding ratline to a pad
//
void CFreePcbView::OnEndVertexAddConnection()
{
	OnVertexConnectToPin();
}

// end vertex selected, delete it and the adjacent segment
//
void CFreePcbView::OnEndVertexDelete()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
//	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_is-1, TRUE );
	if( m_Doc->m_vis[LAY_RAT_LINE] )
		m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// edit the position of an end vertex
//
void CFreePcbView::OnEndVertexEdit()
{
	DlgEditBoardCorner dlg;
	CString str = "Edit End Vertex Position";
	int x = m_sel_vtx->x;
	int y = m_sel_vtx->y;
	dlg.Init( &str, m_Doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,
			dlg.X(), dlg.Y() );
		m_Doc->ProjectModified( TRUE );
		m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
}

// finish routing a connection by making a segment to the destination pad
//
void CFreePcbView::OnRatlineComplete()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );

	// complete routing to pin
	int w = m_Doc->m_trace_w;
	int via_w = m_Doc->m_via_w;
	int via_hole_w = m_Doc->m_via_hole_w;
	GetWidthsForSegment( &w, &via_w, &via_hole_w );
	int test = m_Doc->m_nlist->RouteSegment( m_sel_net, m_sel_ic, m_sel_is, m_active_layer, w );
	if( !test )
	{
		CancelSelection();
		Invalidate( FALSE );
	}
	else
	{
		// didn't work
		PlaySound( TEXT("CriticalStop"), 0, 0 );
		// TODO: eliminate undo
	}
	m_Doc->ProjectModified( TRUE );
}

// set width of a connection
//
void CFreePcbView::OnRatlineSetWidth()
{
	if( m_sel_con->NumSegs() == 1 )
		SetWidth( 2 );
	else
		SetWidth( 1 );
	Invalidate( FALSE );
}

// delete a connection
//
void CFreePcbView::OnRatlineDeleteConnection()
{
	if( m_sel_con->locked )
	{
		int ret = AfxMessageBox( "You are trying to delete a locked connection.\nAre you sure ? ",
			MB_YESNO );
		if( ret == IDNO )
			return;
	}
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->RemoveNetConnect( m_sel_net, m_sel_ic, FALSE );
	m_Doc->m_nlist->RemoveOrphanBranches( m_sel_net, 0, TRUE );
	m_Doc->m_nlist->SetAreaConnections( m_sel_net );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// lock a connection
//
void CFreePcbView::OnRatlineLockConnection()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_sel_con->locked = 1;
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
}

// unlock a connection
//
void CFreePcbView::OnRatlineUnlockConnection()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_sel_con->locked = 0;
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	Invalidate( FALSE );
	m_Doc->ProjectModified( TRUE );
}

// edit a text string
//
void CFreePcbView::OnTextEdit()
{
	// create dialog and pass parameters
	CDlgAddText add_text_dlg;
	add_text_dlg.Initialize( 0, m_Doc->m_num_layers, 0, &m_sel_text->m_str,
		m_Doc->m_units, m_sel_text->m_layer, m_sel_text->m_mirror,
			m_sel_text->m_bNegative, m_sel_text->m_angle, m_sel_text->m_font_size,
			m_sel_text->m_stroke_width, m_sel_text->m_x, m_sel_text->m_y );
	int ret = add_text_dlg.DoModal();
	if( ret == IDCANCEL )
		return;

	// now replace old text with new one
	SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_MODIFY, TRUE, m_Doc->m_undo_list );
	int x = add_text_dlg.m_x;
	int y = add_text_dlg.m_y;
	int mirror = add_text_dlg.m_bMirror;
	BOOL bNegative = add_text_dlg.m_bNegative;
	int angle = add_text_dlg.m_angle;
	int font_size = add_text_dlg.m_height;
	int stroke_width = add_text_dlg.m_width;
	int layer = add_text_dlg.m_layer;
	CString test_str = add_text_dlg.m_str;
	m_dlist->CancelHighLight();
	CText * new_text = m_Doc->m_tlist->AddText( x, y, angle, mirror, bNegative,
		layer, font_size, stroke_width, &test_str );
	new_text->m_uid = m_sel_text->m_uid;
	m_Doc->m_tlist->RemoveText( m_sel_text );
	m_sel_text = new_text;
	m_Doc->m_tlist->HighlightText( m_sel_text );

	// start dragging if requested in dialog
	if( add_text_dlg.m_bDrag )
		OnTextMove();
	else
		Invalidate( FALSE );
	m_Doc->ProjectModified( TRUE );
}

// start adding board outline by dragging line for first side
//
void CFreePcbView::OnAddBoardOutline()
{
	m_Doc->m_vis[LAY_BOARD_OUTLINE] = TRUE;
	m_dlist->SetLayerVisible( LAY_BOARD_OUTLINE, TRUE );
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_dlist->CancelHighLight();
	int ib = m_Doc->m_board_outline.GetSize() - 1;
	m_sel_id.Set( ID_BOARD, -1, ID_OUTLINE, -1, ib, ID_SEL_CORNER, -1, 0 );
	m_polyline_style = CPolyLine::STRAIGHT;
	m_dlist->StartDraggingArray( pDC, p.x, p.y, 0, LAY_BOARD_OUTLINE, 2 );
	SetCursorMode( CUR_ADD_BOARD );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// move a board outline corner
//
void CFreePcbView::OnBoardCornerMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_from_pt.x = m_Doc->m_board_outline[m_sel_id.I2()].X( m_sel_id.I3() );
	m_from_pt.y = m_Doc->m_board_outline[m_sel_id.I2()].Y( m_sel_id.I3() );
	m_Doc->m_board_outline[m_sel_id.I2()].StartDraggingToMoveCorner( pDC, m_sel_id.I3(), p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_BOARD_MOVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// edit a board outline corner
//
void CFreePcbView::OnBoardCornerEdit()
{
	DlgEditBoardCorner dlg;
	CString str = "Corner Position";
	int x = m_Doc->m_board_outline[m_sel_id.I2()].X(m_sel_id.I3());
	int y = m_Doc->m_board_outline[m_sel_id.I2()].Y(m_sel_id.I3());
	dlg.Init( &str, m_Doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForBoardOutlines( TRUE, m_Doc->m_undo_list );
		m_Doc->m_board_outline[m_sel_id.I2()].MoveCorner( m_sel_id.I3(),
			dlg.X(), dlg.Y() );
		CancelSelection();
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

// delete a board corner
//
void CFreePcbView::OnBoardCornerDelete()
{
	if( m_Doc->m_board_outline[m_sel_id.I2()].NumCorners() < 4 )
	{
		AfxMessageBox( "Board outline has too few corners" );
		return;
	}
	SaveUndoInfoForBoardOutlines( TRUE, m_Doc->m_undo_list );
	m_Doc->m_board_outline[m_sel_id.I2()].DeleteCorner( m_sel_id.I3() );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// insert a new corner in a side of a board outline
//
void CFreePcbView::OnBoardSideAddCorner()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_Doc->m_board_outline[m_sel_id.I2()].StartDraggingToInsertCorner( pDC, m_sel_id.I3(), p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_BOARD_INSERT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// delete entire board outline
//
void CFreePcbView::OnBoardDeleteOutline()
{
	SaveUndoInfoForBoardOutlines( TRUE, m_Doc->m_undo_list );
	m_Doc->m_board_outline.RemoveAt( m_sel_id.I2() );
	id new_id = m_sel_id;
	for( int i=m_sel_id.I2(); i<m_Doc->m_board_outline.GetSize(); i++ )
	{
		CPolyLine * poly = &m_Doc->m_board_outline[i];
		new_id.SetI2( i );
		poly->SetId( &new_id );
	}
	m_Doc->ProjectModified( TRUE );
	CancelSelection();
	Invalidate( FALSE );
}

// move a copper area corner
//
void CFreePcbView::OnAreaCornerMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_from_pt.x = m_sel_net->area[m_sel_id.I2()].X( m_sel_id.I3() );
	m_from_pt.y = m_sel_net->area[m_sel_id.I2()].Y( m_sel_id.I3() );
	m_Doc->m_nlist->StartDraggingAreaCorner( pDC, m_sel_net, m_sel_ia, m_sel_is, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_AREA_MOVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// delete a copper area corner
//
void CFreePcbView::OnAreaCornerDelete()
{
	carea * area;
	area = &m_sel_net->area[m_sel_id.I2()];
	if( area->NumCorners() > 3 )
	{
		SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
//		SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
		area->DeleteCorner( m_sel_id.I3() );
		m_dlist->CancelHighLight();
		m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
		if( m_Doc->m_vis[LAY_RAT_LINE] )
			m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
		m_Doc->ProjectModified( TRUE );
		SetCursorMode( CUR_NONE_SELECTED );
		Invalidate( FALSE );
	}
	else
		OnAreaCornerDeleteArea();
}

// delete entire area
//
void CFreePcbView::OnAreaCornerDeleteArea()
{
	OnAreaSideDeleteArea();
	m_Doc->ProjectModified( TRUE );
}

//insert a new corner in a side of a copper area
//
void CFreePcbView::OnAreaSideAddCorner()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_Doc->m_nlist->StartDraggingInsertedAreaCorner( pDC, m_sel_net, m_sel_ia, m_sel_is, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_AREA_INSERT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// delete entire area
//
void CFreePcbView::OnAreaSideDeleteArea()
{
	SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_DELETE, TRUE, m_Doc->m_undo_list );
//	SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_DELETE, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->RemoveArea( m_sel_net, m_sel_ia );
	if( m_Doc->m_vis[LAY_RAT_LINE] )
		m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// detect state where nothing is selected or being dragged
//
BOOL CFreePcbView::CurNone()
{
	return( m_cursor_mode == CUR_NONE_SELECTED );
}

// detect any selected state
//
BOOL CFreePcbView::CurSelected()
{
	return( m_cursor_mode > CUR_NONE_SELECTED && m_cursor_mode < CUR_NUM_SELECTED_MODES );
}

// detect any dragging state
//
BOOL CFreePcbView::CurDragging()
{
	return( m_cursor_mode > CUR_NUM_SELECTED_MODES && m_cursor_mode < CUR_NUM_MODES );
}

// detect states using routing grid
//
BOOL CFreePcbView::CurDraggingRouting()
{
	return( m_cursor_mode == CUR_DRAG_RAT
		|| m_cursor_mode == CUR_DRAG_VTX
		|| m_cursor_mode == CUR_DRAG_VTX_INSERT
		|| m_cursor_mode == CUR_DRAG_END_VTX
		|| m_cursor_mode == CUR_ADD_AREA
		|| m_cursor_mode == CUR_DRAG_AREA_1
		|| m_cursor_mode == CUR_DRAG_AREA
		|| m_cursor_mode == CUR_DRAG_AREA_INSERT
		|| m_cursor_mode == CUR_DRAG_AREA_MOVE
		|| m_cursor_mode == CUR_ADD_AREA_CUTOUT
		|| m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1
		|| m_cursor_mode == CUR_DRAG_AREA_CUTOUT
		|| m_cursor_mode == CUR_ADD_SMCUTOUT
		|| m_cursor_mode == CUR_DRAG_SMCUTOUT_1
		|| m_cursor_mode == CUR_DRAG_SMCUTOUT
		|| m_cursor_mode == CUR_DRAG_SMCUTOUT_INSERT
		|| m_cursor_mode == CUR_DRAG_SMCUTOUT_MOVE
		|| m_cursor_mode == CUR_DRAG_STUB
		|| m_cursor_mode == CUR_MOVE_SEGMENT
		|| m_cursor_mode == CUR_DRAG_VTX_INSERT
		);
}

// detect states using placement grid
//
BOOL CFreePcbView::CurDraggingPlacement()
{
	return( m_cursor_mode == CUR_ADD_BOARD
		|| m_cursor_mode == CUR_DRAG_BOARD_1
		|| m_cursor_mode == CUR_DRAG_BOARD
		|| m_cursor_mode == CUR_DRAG_BOARD_INSERT
		|| m_cursor_mode == CUR_DRAG_BOARD_MOVE
		|| m_cursor_mode == CUR_DRAG_PART
		|| m_cursor_mode == CUR_DRAG_REF
		|| m_cursor_mode == CUR_DRAG_TEXT
		|| m_cursor_mode == CUR_DRAG_GROUP
		|| m_cursor_mode == CUR_DRAG_GROUP_ADD
		|| m_cursor_mode == CUR_MOVE_ORIGIN );
}

// snap cursor if required and set m_last_cursor_point
//
void CFreePcbView::SnapCursorPoint( CPoint wp, UINT nFlags )
{
	// see if we need to snap at all
	if( CurDragging() )
	{
		// yes, set snap modes based on cursor mode and SHIFT and CTRL keys
		if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB )
		{
			// routing a trace segment, set modes
			if( nFlags != -1 )
			{
				if( nFlags & MK_CONTROL )
				{
					// control key held down
					m_snap_mode = SM_GRID_LINES;
					m_inflection_mode = IM_NONE;
				}
				else
				{
					m_snap_mode = SM_GRID_POINTS;
					if( m_Doc->m_snap_angle == 45 )
					{
						if( nFlags & MK_SHIFT )
							m_inflection_mode = IM_45_90;
						else
							m_inflection_mode = IM_90_45;
					}
					else if( m_Doc->m_snap_angle == 90 )
						m_inflection_mode = IM_90;

				}
				m_dlist->SetInflectionMode( m_inflection_mode );
			}
		}
		else
		{
			// for other dragging modes, always use grid points with no inflection
			m_snap_mode = SM_GRID_POINTS;
			m_inflection_mode = IM_NONE;
			m_dlist->SetInflectionMode( m_inflection_mode );
		}
		// set grid spacing
		int grid_spacing;
		if( CurDraggingPlacement() )
		{
			grid_spacing = m_Doc->m_part_grid_spacing;
		}
		else if( CurDraggingRouting() )
		{
			grid_spacing = m_Doc->m_routing_grid_spacing;
		}
		else if( m_Doc->m_units == MIL )
		{
			grid_spacing = m_Doc->m_pcbu_per_wu;
		}
		else if( m_Doc->m_units == MM )
		{
			grid_spacing = m_Doc->m_pcbu_per_wu;               
		}
		else 
			ASSERT(0);
		// see if we need to snap to angle
		if( m_Doc->m_snap_angle && (wp != m_snap_angle_ref)
			&& ( m_cursor_mode == CUR_DRAG_RAT
			|| m_cursor_mode == CUR_DRAG_STUB
			|| m_cursor_mode == CUR_DRAG_AREA_1
			|| m_cursor_mode == CUR_DRAG_AREA
			|| m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1
			|| m_cursor_mode == CUR_DRAG_AREA_CUTOUT
			|| m_cursor_mode == CUR_DRAG_BOARD_1
			|| m_cursor_mode == CUR_DRAG_BOARD ) )
		{
			// yes, check snap mode
			if( m_snap_mode == SM_GRID_LINES )
			{
				// patch to snap to grid lines, contributed by ???
				// modified by AMW to work when cursor x,y are < 0
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
				CString test, test_grid;
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
			else
			{
				// old code
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
					if( m_Doc->m_snap_angle == 45 )
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
		}
		else
			m_snap_mode = SM_GRID_POINTS;

		// snap to grid points if needed
		if( m_snap_mode == SM_GRID_POINTS )
		{
			// snap to grid
			// get position in integral units of grid_spacing
			if( wp.x > 0 )
				wp.x = (wp.x + grid_spacing/2)/grid_spacing;
			else
				wp.x = (wp.x - grid_spacing/2)/grid_spacing;
			if( wp.y > 0 )
				wp.y = (wp.y + grid_spacing/2)/grid_spacing;
			else
				wp.y = (wp.y - grid_spacing/2)/grid_spacing;
			// multiply by grid spacing, adding or subracting 0.5 to prevent round-off
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
			// show relative distance
			if( m_cursor_mode == CUR_DRAG_GROUP
				|| m_cursor_mode == CUR_DRAG_GROUP_ADD
				|| m_cursor_mode == CUR_DRAG_PART
				|| m_cursor_mode == CUR_DRAG_VTX
				|| m_cursor_mode ==  CUR_DRAG_BOARD_MOVE
				|| m_cursor_mode == CUR_DRAG_AREA_MOVE
				|| m_cursor_mode ==  CUR_DRAG_SMCUTOUT_MOVE
				|| m_cursor_mode ==  CUR_DRAG_MEASURE_2
				|| m_cursor_mode == CUR_MOVE_SEGMENT
				)
			{
				ShowRelativeDistance( wp.x - m_from_pt.x, wp.y - m_from_pt.y );
			}
		}
	}
	// update cursor position
	m_last_cursor_point = wp;
	ShowCursor();
}

LONG CFreePcbView::OnChangeVisibleGrid( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
		m_Doc->m_visual_grid_spacing = fabs( m_Doc->m_visible_grid[lp] );
	else
		ASSERT(0);
	m_dlist->SetVisibleGrid( TRUE, m_Doc->m_visual_grid_spacing );
	Invalidate( FALSE );
	m_Doc->ProjectModified( TRUE );
	SetFocus();
	return 0;
}

LONG CFreePcbView::OnChangePlacementGrid( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
		m_Doc->m_part_grid_spacing = fabs( m_Doc->m_part_grid[lp] );
	else
		ASSERT(0);
	m_Doc->ProjectModified( TRUE );
	SetFocus();
	return 0;
}

LONG CFreePcbView::OnChangeRoutingGrid( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
		m_Doc->m_routing_grid_spacing = fabs( m_Doc->m_routing_grid[lp] );
	else
		ASSERT(0);
	SetFocus();
	m_Doc->ProjectModified( TRUE );
	return 0;
}

LONG CFreePcbView::OnChangeSnapAngle( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
	{
		if( lp == 0 )
		{
			m_Doc->m_snap_angle = 45;
			m_inflection_mode = IM_90_45;
		}
		else if( lp == 1 )
		{
			m_Doc->m_snap_angle = 90;
			m_inflection_mode = IM_90;
		}
		else
		{
			m_Doc->m_snap_angle = 0;
			m_inflection_mode = IM_NONE;
		}
	}
	else
		ASSERT(0);
	m_Doc->ProjectModified( TRUE );
	SetFocus();
	return 0;
}

LONG CFreePcbView::OnChangeUnits( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
	{
		if( lp == 0 )
			m_Doc->m_units = MIL;
		else
			m_Doc->m_units = MM;
	}
	else
		ASSERT(0);
	m_Doc->ProjectModified( TRUE );
	SetFocus();
	ShowSelectStatus();
	return 0;
}


void CFreePcbView::OnSegmentDeleteTrace()
{
	OnRatlineDeleteConnection();
}

void CFreePcbView::OnAreaCornerProperties()
{
	// reuse board corner dialog
	DlgEditBoardCorner dlg;
	CString str = "Corner Position";
	CPoint pt = m_Doc->m_nlist->GetAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
	dlg.Init( &str, m_Doc->m_units, pt.x, pt.y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
//		SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->MoveAreaCorner( m_sel_net, m_sel_ia, m_sel_is,
			dlg.X(), dlg.Y() );
		m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
		if( m_Doc->m_vis[LAY_RAT_LINE] )
			m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
		m_Doc->m_nlist->HighlightAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		SetCursorMode( CUR_AREA_CORNER_SELECTED );
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnRefProperties()
{
	CDlgRefText dlg;
	dlg.Initialize( m_Doc->m_plist, m_sel_part, &m_Doc->m_footprint_cache_map );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// edit this part
		SaveUndoInfoForPart( m_sel_part,
			CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
		m_sel_part->m_ref_layer = FlipLayer( m_sel_part->side, dlg.m_layer );
		m_Doc->m_plist->ResizeRefText( m_sel_part, dlg.m_height, dlg.m_width, dlg.m_vis );
		m_Doc->ProjectModified( TRUE );
		m_dlist->CancelHighLight();
		if( m_cursor_mode == CUR_PART_SELECTED )
			m_Doc->m_plist->SelectPart( m_sel_part );
		else if( m_cursor_mode == CUR_REF_SELECTED 
			&& m_sel_part->m_ref_size && m_sel_part->m_ref_vis )
			m_Doc->m_plist->SelectRefText( m_sel_part );
		else
			CancelSelection();
		Invalidate( FALSE );
	}
}

#if 0
void CFreePcbView::OnVertexProperties()
{
	DlgEditBoardCorner dlg;
	CString str = "Vertex Position";
	int x = m_sel_vtx->x;
	int y = m_sel_vtx->y;
	dlg.Init( &str, m_Doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY,
			TRUE, m_Doc->m_undo_list );
		m_dlist->CancelHighLight();
		m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,
			dlg.X(), dlg.Y() );
		m_Doc->ProjectModified( TRUE );
		m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
}
#endif

BOOL CFreePcbView::OnEraseBkgnd(CDC* pDC)
{
	// Erase the left and bottom panes, the PCB area is always redrawn
	m_left_pane_invalid = TRUE;
	return FALSE;
}

void CFreePcbView::OnBoardSideConvertToStraightLine()
{
	m_Doc->m_board_outline[m_sel_id.I2()].SetSideStyle( m_sel_id.I3(), CPolyLine::STRAIGHT );
	m_Doc->m_board_outline[m_sel_id.I2()].HighlightSide( m_sel_id.I3() );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnBoardSideConvertToArcCw()
{
	m_Doc->m_board_outline[m_sel_id.I2()].SetSideStyle( m_sel_id.I3(), CPolyLine::ARC_CW );
	m_Doc->m_board_outline[m_sel_id.I2()].HighlightSide( m_sel_id.I3() );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnBoardSideConvertToArcCcw()
{
	m_Doc->m_board_outline[m_sel_id.I2()].SetSideStyle( m_sel_id.I3(), CPolyLine::ARC_CCW );
	m_Doc->m_board_outline[m_sel_id.I2()].HighlightSide( m_sel_id.I3() );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// unroute entire connection
//
void CFreePcbView::OnUnrouteTrace()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	cconnect * c = m_sel_con;
	for( int is=0; is<c->NumSegs(); is++ )
		m_Doc->m_nlist->UnrouteSegmentWithoutMerge( m_sel_net, m_sel_ic, is );
	m_Doc->m_nlist->MergeUnroutedSegments( m_sel_net, m_sel_ic );
	m_Doc->m_nlist->SetAreaConnections( m_sel_net );
	// try to reselect
	m_sel_id = m_sel_id.Con()->SegByIndex(0).Id();
	if( m_sel_id.Resolve() )
	{
		SelectItem( m_sel_id );
		SetCursorMode( CUR_RAT_SELECTED );
	}
	else
	{
		CancelSelection();
	}
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// save undo info for a group, for UNDO_GROUP_MODIFY or UNDO_GROUP_DELETE
//
void CFreePcbView::SaveUndoInfoForGroup( int type, CArray<void*> * ptrs, CArray<id> * ids, CUndoList * list )
{
	if( type != UNDO_GROUP_MODIFY && type != UNDO_GROUP_DELETE )
		ASSERT(0);

	void * ptr;

	// first, mark all nets as unselected and set new_event flag
	m_Doc->m_nlist->MarkAllNets(0);
	list->NewEvent();

	// save info for all relevant nets
	for( int i=0; i<ids->GetSize(); i++ )
	{
		id sid = (*ids)[i];
		if( sid.T1() == ID_PART )
		{
			cpart * part = (cpart*)(*ptrs)[i];
			for( int ip=0; ip<part->pin.GetSize(); ip++ )
			{
				cnet * net = (cnet*)part->pin[ip].net;
				if( net )
				{
					if( net->utility == FALSE )
					{
						// unsaved
						SaveUndoInfoForNetAndConnectionsAndAreas( net, FALSE, list );
						net->utility = TRUE;
					}
				}
			}
		}
		else if( sid.T1() == ID_NET )
		{
			cnet * net = (cnet*)(*ptrs)[i];
			if( net )
			{
				if( net->utility == FALSE )
				{
					// unsaved
					SaveUndoInfoForNetAndConnectionsAndAreas( net, FALSE, list );
					net->utility = TRUE;
				}
			}
		}
	}
	// save undo info for all parts and texts in group
	for( int i=0; i<ids->GetSize(); i++ )
	{
		id sid = (*ids)[i];
		if( sid.T1() == ID_PART )
		{
			cpart * part = (cpart*)(*ptrs)[i];
			SaveUndoInfoForPart( part,
				CPartList::UNDO_PART_MODIFY, NULL, FALSE, list );
		}
		else if( sid.T1() == ID_TEXT )
		{
			CText * text = (CText*)(*ptrs)[i];
			if( type == UNDO_GROUP_MODIFY )
				SaveUndoInfoForText( text, CTextList::UNDO_TEXT_MODIFY, FALSE, list );
			else if( type == UNDO_GROUP_DELETE )
				SaveUndoInfoForText( text, CTextList::UNDO_TEXT_DELETE, FALSE, list );
		}
	}
	// save undo info for all sm cutouts
	SaveUndoInfoForSMCutouts( FALSE, list );
	// save undo info for all board outlines
	SaveUndoInfoForBoardOutlines( FALSE, list );

	// now save undo descriptor );
	ptr = (void*)CreateGroupDescriptor( list, ptrs, ids, type );
	list->Push( UNDO_GROUP, ptr, &UndoGroupCallback );
}

// save undo info for two existing parts and all nets connected to them,
// assuming that the parts will be modified (not deleted or added)
//
void CFreePcbView::SaveUndoInfoFor2PartsAndNets( cpart * part1, cpart * part2, BOOL new_event, CUndoList * list )
{
	void * ptr;
	cpart * part;

	if( new_event )
		list->NewEvent();
	for( int i=0; i<2; i++ )
	{
		if( i==0 )
			part = part1;
		else
			part = part2;
		for( int ip=0; ip<part->pin.GetSize(); ip++ )
		{
			cnet * net = (cnet*)part->pin[ip].net;
			if( net )
				net->utility = 0;
		}
		for( int ip=0; ip<part->pin.GetSize(); ip++ )
		{
			cnet * net = (cnet*)part->pin[ip].net;
			if( net )
			{
				if( net->utility == 0 )
				{
					for( int ic=0; ic<net->NumCons(); ic++ )
					{
						undo_con * u_con = m_Doc->m_nlist->CreateConnectUndoRecord( net, ic );
						list->Push( CNetList::UNDO_CONNECT_MODIFY, u_con,
							&m_Doc->m_nlist->ConnectUndoCallback, u_con->size );
					}
					undo_net * u_net = m_Doc->m_nlist->CreateNetUndoRecord( net );
					list->Push( CNetList::UNDO_NET_MODIFY, u_net,
						&m_Doc->m_nlist->NetUndoCallback, u_net->size );
					net->utility = 1;
				}
			}
		}
	}
	// now save undo info for parts
	undo_part * u_part1 = m_Doc->m_plist->CreatePartUndoRecord( part1, NULL );
	list->Push( CPartList::UNDO_PART_MODIFY, u_part1,
		&m_Doc->m_plist->PartUndoCallback, u_part1->size );
	undo_part * u_part2 = m_Doc->m_plist->CreatePartUndoRecord( part2, NULL );
	list->Push( CPartList::UNDO_PART_MODIFY, u_part2,
		&m_Doc->m_plist->PartUndoCallback, u_part2->size );
	// now save undo descriptor
	if( new_event )
	{
		ptr = CreateUndoDescriptor( list, 0, &part1->ref_des, &part2->ref_des, 0, 0, NULL, NULL );
		list->Push( UNDO_2_PARTS_AND_NETS, ptr, &UndoCallback );
	}
}

// save undo info for net, all connections and all areas
//
void CFreePcbView::SaveUndoInfoForNetAndConnectionsAndAreas( cnet * net, BOOL new_event, CUndoList * list )
{
	if( new_event )
		ASSERT( 0 );
	SaveUndoInfoForNetAndConnections( net,
		CNetList::UNDO_NET_MODIFY, new_event, list );
	for( int ia=0; ia<net->NumAreas(); ia++ )
		SaveUndoInfoForArea( net, ia, CNetList::UNDO_AREA_MODIFY, FALSE, list );
}

// save undo info for net, all connections and
// a single copper area
// type may be:
//	CNetList::UNDO_AREA_MODIFY	if area will be modified
//	CNetList::UNDO_AREA_DELETE	if area will be deleted
//
void CFreePcbView::SaveUndoInfoForNetAndConnectionsAndArea( cnet * net, int iarea,
														   int type, BOOL new_event, CUndoList * list )
{
	if( new_event )
	{
		list->NewEvent();
	}
	SaveUndoInfoForArea( net, iarea, type, FALSE, m_Doc->m_undo_list );
	SaveUndoInfoForNetAndConnections( net,
		CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
	// now save undo descriptor
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, type, &net->name, NULL, iarea, 0, NULL, NULL );
		list->Push( UNDO_NET_AND_CONNECTIONS_AND_AREA, ptr, &UndoCallback );
	}
}

// save undo info for a copper area to be modified or deleted
// type may be:
//	CNetList::UNDO_AREA_ADD	if area will be added
//	CNetList::UNDO_AREA_MODIFY	if area will be modified
//	CNetList::UNDO_AREA_DELETE	if area will be deleted
//
void CFreePcbView::SaveUndoInfoForArea( cnet * net, int iarea, int type, BOOL new_event, CUndoList * list )
{
	void *ptr;
	if( new_event )
	{
		list->NewEvent();
		SaveUndoInfoForNet( net, CNetList::UNDO_NET_OPTIMIZE, FALSE, list );
	}
	int nc = 1;
	if( type != CNetList::UNDO_AREA_ADD )
	{
		nc = net->area[iarea].NumContours();
		if( !net->area[iarea].Closed() )
			nc--;
	}
	if( nc > 0 )
	{
		undo_area * undo = m_Doc->m_nlist->CreateAreaUndoRecord( net, iarea, type );
		list->Push( type, (void*)undo, &m_Doc->m_nlist->AreaUndoCallback, undo->size );
	}
	// now save undo descriptor
	if( new_event )
	{
		ptr = CreateUndoDescriptor( list, type, &net->name, NULL, iarea, 0, NULL, NULL );
		list->Push( UNDO_AREA, ptr, &UndoCallback );
	}
}

// save undo info for all of the areas in a net
//
void CFreePcbView::SaveUndoInfoForAllAreasInNet( cnet * net, BOOL new_event, CUndoList * list )
{
	if( new_event )
	{
		list->NewEvent();		// flag new undo event
		SaveUndoInfoForNet( net, CNetList::UNDO_NET_OPTIMIZE, FALSE, list );
	}
	for( int ia=net->area.GetSize()-1; ia>=0; ia-- )
		SaveUndoInfoForArea( net, ia, CNetList::UNDO_AREA_DELETE, FALSE, list );
	undo_area * u_area = m_Doc->m_nlist->CreateAreaUndoRecord( net, 0, CNetList::UNDO_AREA_CLEAR_ALL );
	list->Push( CNetList::UNDO_AREA_CLEAR_ALL, u_area, &m_Doc->m_nlist->AreaUndoCallback, u_area->size );
	// now save undo descriptor
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, 0, &net->name, NULL, 0, 0, NULL, NULL );
		list->Push( UNDO_ALL_AREAS_IN_NET, ptr, &UndoCallback );
	}
}

// save undo info for all of the areas in two nets
//
void CFreePcbView::SaveUndoInfoForAllAreasIn2Nets( cnet * net1, cnet * net2, BOOL new_event, CUndoList * list )
{
	if( new_event )
	{
		list->NewEvent();		// flag new undo event
		SaveUndoInfoForNet( net1, CNetList::UNDO_NET_OPTIMIZE, FALSE, list );
		SaveUndoInfoForNet( net2, CNetList::UNDO_NET_OPTIMIZE, FALSE, list );
	}
	SaveUndoInfoForAllAreasInNet( net1, FALSE, list );
	SaveUndoInfoForAllAreasInNet( net2, FALSE, list );
	// now save undo descriptor
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, 0, &net1->name, &net2->name, 0, 0, NULL, NULL );
		list->Push( UNDO_ALL_AREAS_IN_2_NETS, ptr, &UndoCallback );
	}
}

// save undo info for all nets (but not areas)
//
void CFreePcbView::SaveUndoInfoForAllNets( BOOL new_event, CUndoList * list )
{
	POSITION pos;
	CString name;
	CMapStringToPtr * m_map = &m_Doc->m_nlist->m_map;
	void * net_ptr;
	if( new_event )
		list->NewEvent();		// flag new undo event
	// traverse map of nets
	for( pos = m_map->GetStartPosition(); pos != NULL; )
	{
		// next net
		m_map->GetNextAssoc( pos, name, net_ptr );
		cnet * net = (cnet*)net_ptr;
		void * ptr;
		// loop through all connections in net
		for( int ic=0; ic<net->NumCons(); ic++ )
		{
			undo_con * u_con = m_Doc->m_nlist->CreateConnectUndoRecord( net, ic );
			list->Push( CNetList::UNDO_CONNECT_MODIFY, u_con,
				&m_Doc->m_nlist->ConnectUndoCallback, u_con->size );
		}
		undo_net * u_net = m_Doc->m_nlist->CreateNetUndoRecord( net );
		list->Push( CNetList::UNDO_NET_MODIFY, u_net,
			&m_Doc->m_nlist->NetUndoCallback, u_net->size );
	}
}

// save undo info for all nets including areas
//
void CFreePcbView::SaveUndoInfoForAllNetsAndConnectionsAndAreas( BOOL new_event, CUndoList * list )
{
	POSITION pos;
	CString name;
	CMapStringToPtr * m_map = &m_Doc->m_nlist->m_map;
	void * net_ptr;
	if( new_event )
		list->NewEvent();		// flag new undo event
	// traverse map of nets
	for( pos = m_map->GetStartPosition(); pos != NULL; )
	{
		// next net
		m_map->GetNextAssoc( pos, name, net_ptr );
		cnet * net = (cnet*)net_ptr;
		SaveUndoInfoForNetAndConnectionsAndAreas( net, FALSE, list );
	}
}

void CFreePcbView::SaveUndoInfoForMoveOrigin( int x_off, int y_off, CUndoList * list )
{
	// now push onto undo list
	undo_move_origin * undo = m_Doc->CreateMoveOriginUndoRecord( x_off, y_off );
	list->NewEvent();
	list->Push( 0, (void*)undo, &m_Doc->MoveOriginUndoCallback );
	// save top-level descriptor
	void * ptr = CreateUndoDescriptor( list, 0, NULL, NULL, x_off, y_off, NULL, NULL );
	list->Push( UNDO_MOVE_ORIGIN, ptr, &UndoCallback );
}

void CFreePcbView::SaveUndoInfoForBoardOutlines( BOOL new_event, CUndoList * list )
{
	// now push onto undo list
	if( new_event )
		list->NewEvent();		// flag new undo event

	// get number of closed board outlines
	int n_closed = 0;
	for( int i=0; i<m_Doc->m_board_outline.GetSize(); i++ )
	{
		CPolyLine * poly = &m_Doc->m_board_outline[i];
		if( poly->Closed() )
			n_closed = i+1;
	}
	// push all board outlines onto undo list
	for( int i=0; i<n_closed; i++ )
	{
		CPolyLine * poly = &m_Doc->m_board_outline[i];
		undo_board_outline * undo = m_Doc->CreateBoardOutlineUndoRecord( poly );
		list->Push( UNDO_BOARD, (void*)undo, &m_Doc->BoardOutlineUndoCallback );
	}
	list->Push( UNDO_BOARD_OUTLINE_CLEAR_ALL, NULL, &m_Doc->BoardOutlineUndoCallback );
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, 0, NULL, NULL, 0, 0, NULL, NULL );
		list->Push( UNDO_ALL_BOARD_OUTLINES, ptr, &UndoCallback );
	}
}

void CFreePcbView::SaveUndoInfoForSMCutouts( BOOL new_event, CUndoList * list )
{
	// push undo info onto list
	if( new_event )
		list->NewEvent();		// flag new undo event
	// get last closed cutout
	int i;
	int n_closed = 0;
	for( i=0; i<m_Doc->m_sm_cutout.GetSize(); i++ )
	{
		CPolyLine * poly = &m_Doc->m_sm_cutout[i];
		if( poly->Closed() )
			n_closed = i+1;
		else
			break;
	}
	// push all closed cutouts onto undo list
	for( i=0; i<n_closed; i++ )
	{
		CPolyLine * poly = &m_Doc->m_sm_cutout[i];
		undo_sm_cutout * undo = m_Doc->CreateSMCutoutUndoRecord( poly );
		list->Push( UNDO_SM_CUTOUT, (void*)undo, &m_Doc->SMCutoutUndoCallback );
	}
	// create UNDO_SM_CUTOUT_CLEAR_ALL record and push it
	list->Push( UNDO_SM_CUTOUT_CLEAR_ALL, NULL, &m_Doc->SMCutoutUndoCallback );
	// now push top-level callback for redoing
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, 0, NULL, NULL, 0, 0, NULL, NULL );
		list->Push( UNDO_ALL_SM_CUTOUTS, ptr, &UndoCallback );
	}
}

void CFreePcbView::SaveUndoInfoForText( CText * text, int type, BOOL new_event, CUndoList * list )
{
	// create new undo record and push onto undo list
	undo_text * undo = m_Doc->m_tlist->CreateUndoRecord( text );
	if( new_event )
		list->NewEvent();		// flag new undo event
	list->Push( type, (void*)undo, &m_Doc->m_tlist->TextUndoCallback );
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, type, NULL, NULL, 0, 0, NULL, (void *)undo );
		list->Push( UNDO_TEXT, ptr, &UndoCallback );
	}
}

void CFreePcbView::SaveUndoInfoForText( undo_text * u_text, int type, BOOL new_event, CUndoList * list )
{
	// copy undo record and push onto undo list
	undo_text * undo = new undo_text;
	*undo = *u_text;
	if( new_event )
		list->NewEvent();		// flag new undo event
	list->Push( type, (void*)undo, &m_Doc->m_tlist->TextUndoCallback );
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, type, NULL, NULL, 0, 0, NULL, (void*)undo );
		list->Push( UNDO_TEXT, ptr, &UndoCallback );
	}
}


void CFreePcbView::OnViewEntireBoard()
{
	if( m_Doc->m_board_outline.GetSize() )
	{
		// get boundaries of board outline
		int max_x = INT_MIN;
		int min_x = INT_MAX;
		int max_y = INT_MIN;
		int min_y = INT_MAX;
		for( int ib=0; ib<m_Doc->m_board_outline.GetSize(); ib++ )
		{
			for( int ic=0; ic<m_Doc->m_board_outline[0].NumCorners(); ic++ )
			{
				if( m_Doc->m_board_outline[ib].X( ic ) > max_x )
					max_x = m_Doc->m_board_outline[ib].X( ic );
				if( m_Doc->m_board_outline[ib].X( ic ) < min_x )
					min_x = m_Doc->m_board_outline[ib].X( ic );
				if( m_Doc->m_board_outline[ib].Y( ic ) > max_y )
					max_y = m_Doc->m_board_outline[ib].Y( ic );
				if( m_Doc->m_board_outline[ib].Y( ic ) < min_y )
					min_y = m_Doc->m_board_outline[ib].Y( ic );
			}
		}
		// reset window to enclose board outline
//		m_org_x = min_x - (max_x - min_x)/20;	// in pcbu
//		m_org_y = min_y - (max_y - min_y)/20;	// in pcbu
		double x_pcbu_per_pixel = 1.1 * (double)(max_x - min_x)/(m_client_r.right - m_left_pane_w);
		double y_pcbu_per_pixel = 1.1 * (double)(max_y - min_y)/(m_client_r.bottom - m_bottom_pane_h);
		m_pcbu_per_pixel = max( x_pcbu_per_pixel, y_pcbu_per_pixel );
		m_org_x = (max_x + min_x)/2 - (m_client_r.right - m_left_pane_w)/2 * m_pcbu_per_pixel;
		m_org_y = (max_y + min_y)/2 - (m_client_r.bottom - m_bottom_pane_h)/2 * m_pcbu_per_pixel;
		CRect screen_r;
		GetWindowRect( &screen_r );		// in pixels
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
			m_org_x, m_org_y );
		Invalidate( FALSE );
	}
	else
	{
		AfxMessageBox( "Board outline does not exist" );
	}
}

void CFreePcbView::OnViewAllElements()
{
	// reset window to enclose all elements
	BOOL bOK = FALSE;
	CRect r;
	// parts
	int test = m_Doc->m_plist->GetPartBoundaries( &r );
	if( test != 0 )
		bOK = TRUE;
	int max_x = r.right;
	int min_x = r.left;
	int max_y = r.top;
	int min_y = r.bottom;
	// board outline
	for( int ib=0; ib<m_Doc->m_board_outline.GetSize(); ib++ )
	{
		r = m_Doc->m_board_outline[ib].GetBounds();
		max_x = max( max_x, r.right );
		min_x = min( min_x, r.left );
		max_y = max( max_y, r.top );
		min_y = min( min_y, r.bottom );
		bOK = TRUE;
	}
	// nets
	if( m_Doc->m_nlist->GetNetBoundaries( &r ) )
	{
		max_x = max( max_x, r.right );
		min_x = min( min_x, r.left );
		max_y = max( max_y, r.top );
		min_y = min( min_y, r.bottom );
		bOK = TRUE;
	}
	// texts
	if( m_Doc->m_tlist->GetTextBoundaries( &r ) )
	{
		max_x = max( max_x, r.right );
		min_x = min( min_x, r.left );
		max_y = max( max_y, r.top );
		min_y = min( min_y, r.bottom );
		bOK = TRUE;
	}
	if( bOK )
	{
		// reset window
//		m_org_x = min_x - (max_x - min_x)/20;	// NM
//		m_org_y = min_y - (max_y - min_y)/20;	// NM
		double x_pcbu_per_pixel = 1.1 * (double)(max_x - min_x)/(m_client_r.right - m_left_pane_w);
		double y_pcbu_per_pixel = 1.1 * (double)(max_y - min_y)/(m_client_r.bottom - m_bottom_pane_h);
		m_pcbu_per_pixel = max( x_pcbu_per_pixel, y_pcbu_per_pixel );
		m_org_x = (max_x + min_x)/2 - (m_client_r.right - m_left_pane_w)/2 * m_pcbu_per_pixel;
		m_org_y = (max_y + min_y)/2 - (m_client_r.bottom - m_bottom_pane_h)/2 * m_pcbu_per_pixel;
		CRect screen_r;
		GetWindowRect( &screen_r );
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
			m_org_x, m_org_y );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnAreaEdgeHatchStyle()
{
	CDlgSetAreaHatch dlg;
	dlg.Init( m_sel_net->area[m_sel_id.I2()].GetHatch() );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		int hatch = dlg.GetHatch();
		m_sel_net->area[m_sel_id.I2()].SetHatch( hatch );
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnPartEditFootprint()
{
	theApp.OnViewFootprint();
}

void CFreePcbView::OnPartEditThisFootprint()
{
	m_Doc->m_edit_footprint = TRUE;
	theApp.OnViewFootprint();
}

// Offer new footprint from the Footprint Editor
//
void CFreePcbView::OnExternalChangeFootprint( CShape * fp )
{
	CString str;
	str.Format( "Do you wish to replace the footprint of part \"%s\"\nwith the new footprint \"%s\" ?",
		m_sel_part->ref_des, fp->m_name );
	int ret = AfxMessageBox( str, MB_YESNO );
	if( ret == IDYES )
	{
		// OK, see if a footprint of the same name is already in the cache
		void * ptr;
		BOOL found = m_Doc->m_footprint_cache_map.Lookup( fp->m_name, ptr );
		if( found )
		{
			// see how many parts are using it, not counting the current one
			CShape * old_fp = (CShape*)ptr;
			int num = m_Doc->m_plist->GetNumFootprintInstances( old_fp );
			if( m_sel_part->shape == old_fp )
				num--;
			if( num <= 0 )
			{
				// go ahead and replace it
				m_Doc->m_plist->UndrawPart( m_sel_part );
				old_fp->Copy( fp );
				m_Doc->m_plist->PartFootprintChanged( m_sel_part, old_fp );
				m_Doc->ResetUndoState();
			}
			else
			{
				// offer to overwrite or rename it
				CDlgDupFootprintName dlg;
				CString mess;
				mess.Format( "Warning: A footprint named \"%s\"\r\nis already in use by other parts.\r\n", fp->m_name );
				mess += "Loading this new footprint will overwrite the old one\r\nunless you change its name\r\n";
				dlg.Initialize( &mess, &m_Doc->m_footprint_cache_map );
				int ret = dlg.DoModal();
				if( ret == IDOK )
				{
					// clicked "OK"
					if( dlg.m_replace_all_flag )
					{
						// replace all instances of footprint
						old_fp->Copy( fp );
						m_Doc->m_plist->FootprintChanged( old_fp );
						m_Doc->ResetUndoState();
					}
					else
					{
						// assign new name to footprint and put in cache
						CShape * shape = new CShape;
						shape->Copy( fp );
						shape->m_name = *dlg.GetNewName();
						m_Doc->m_footprint_cache_map.SetAt( shape->m_name, shape );
						m_Doc->m_plist->PartFootprintChanged( m_sel_part, shape );
						m_Doc->ResetUndoState();
					}
				}
			}
		}
		else
		{
			// footprint name not found in cache, add the new footprint
			CShape * shape = new CShape;
			shape->Copy( fp );
			m_Doc->m_footprint_cache_map.SetAt( shape->m_name, shape );
			m_Doc->m_plist->PartFootprintChanged( m_sel_part, shape );
			m_Doc->ResetUndoState();
		}
		m_Doc->ProjectModified( TRUE );
		m_dlist->CancelHighLight();
		m_Doc->m_plist->SelectRefText( m_sel_part );
		m_Doc->m_plist->HighlightPart( m_sel_part );
		Invalidate( FALSE );
	}
}

// find a part in the layout, center window on it and select it
//
void CFreePcbView::OnViewFindpart()
{
	CDlgFindPart dlg;
	dlg.Initialize( m_Doc->m_plist );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString * ref_des = &dlg.sel_ref_des;
		cpart * part = m_Doc->m_plist->GetPart( *ref_des );
		if( part )
		{
			if( part->shape )
			{
				dl_element * dl_sel = part->dl_sel;
				int xc = (m_dlist->Get_x( dl_sel ) + m_dlist->Get_xf( dl_sel ))/2;
				int yc = (m_dlist->Get_y( dl_sel ) + m_dlist->Get_yf( dl_sel ))/2;
				m_org_x = xc - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
				m_org_y = yc - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
				CRect screen_r;
				GetWindowRect( &screen_r );
				m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
					m_org_x, m_org_y );
				CPoint p(xc, yc);
				p = m_dlist->PCBToScreen( p );
				SetCursorPos( p.x, p.y - 4 );
				SelectPart( part );
				Invalidate( FALSE );
			}
			else
			{
				AfxMessageBox( "Sorry, this part doesn't have a footprint" );
			}
		}
		else
		{
			AfxMessageBox( "Sorry, this part doesn't exist" );
		}
	}
}

void CFreePcbView::OnFootprintWizard()
{
	m_Doc->OnToolsFootprintwizard();
}

void CFreePcbView::OnFootprintEditor()
{
	theApp.OnViewFootprint();
}

void CFreePcbView::OnCheckPartsAndNets()
{
	m_Doc->OnToolsCheckPartsAndNets();
}

void CFreePcbView::OnDrc()
{
	m_Doc->OnToolsDrc();
}

void CFreePcbView::OnClearDRC()
{
	m_Doc->OnToolsClearDrc();
}

void CFreePcbView::OnViewAll()
{
	OnViewAllElements();
}

void CFreePcbView::OnAddSoldermaskCutout()
{
	CDlgAddMaskCutout dlg;
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// force layer visible
		int il = dlg.m_layer;
		m_Doc->m_vis[il] = TRUE;
		m_dlist->SetLayerVisible( il, TRUE );
		// start dragging rectangle
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		m_dlist->CancelHighLight();
		SetCursorMode( CUR_ADD_SMCUTOUT );
		m_polyline_layer = il;
		m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
			m_last_cursor_point.y, 0, il, 2 );
		m_polyline_style = CPolyLine::STRAIGHT;
		m_polyline_hatch = dlg.m_hatch;
		Invalidate( FALSE );
		ReleaseDC( pDC );
	}
}

void CFreePcbView::OnSmCornerMove()
{
	CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.I2()];
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_from_pt.x = poly->X( m_sel_id.I3() );
	m_from_pt.y = poly->Y( m_sel_id.I3() );
	poly->StartDraggingToMoveCorner( pDC, m_sel_id.I3(), p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_SMCUTOUT_MOVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFreePcbView::OnSmCornerSetPosition()
{
	CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.I2()];
	DlgEditBoardCorner dlg;
	CString str = "Corner Position";
	int x = poly->X(m_sel_id.I3());
	int y = poly->Y(m_sel_id.I3());
	dlg.Init( &str, m_Doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
		poly->MoveCorner( m_sel_id.I3(),
			dlg.X(), dlg.Y() );
		CancelSelection();
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnSmCornerDeleteCorner()
{
	CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.I2()];
	if( poly->NumCorners() < 4 )
	{
		AfxMessageBox( "Solder mask cutout has too few corners" );
		return;
	}
	SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
	poly->DeleteCorner( m_sel_id.I3() );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnSmCornerDeleteCutout()
{
	SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
	m_Doc->m_sm_cutout.RemoveAt( m_sel_id.I2() );
	id new_id = m_sel_id;
	for( int i=m_sel_id.I2(); i<m_Doc->m_sm_cutout.GetSize(); i++ )
	{
		CPolyLine * poly = &m_Doc->m_sm_cutout[i];
		new_id.SetI2( i );
		poly->SetId( &new_id );
	}
	m_Doc->ProjectModified( TRUE );
	CancelSelection();
	Invalidate( FALSE );
}

// insert corner into solder mask cutout side and start dragging
void CFreePcbView::OnSmSideInsertCorner()
{
	CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.I2()];
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	poly->StartDraggingToInsertCorner( pDC, m_sel_id.I3(), p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_SMCUTOUT_INSERT );
	ReleaseDC( pDC );
	Invalidate( FALSE );

}

// change hatch style for solder mask cutout
void CFreePcbView::OnSmSideHatchStyle()
{
	CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.I2()];
	CDlgSetAreaHatch dlg;
	dlg.Init( poly->GetHatch() );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
		int hatch = dlg.GetHatch();
		poly->SetHatch( hatch );
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnSmSideDeleteCutout()
{
	OnSmCornerDeleteCutout();
}

// change side of part
void CFreePcbView::OnPartChangeSide()
{
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->side = 1 - m_sel_part->side;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_nlist->PartMoved( m_sel_part );
	if( m_Doc->m_vis[LAY_RAT_LINE] )
		m_Doc->m_nlist->OptimizeConnections( m_sel_part, m_Doc->m_auto_ratline_disable, 
										m_Doc->m_auto_ratline_min_pins );
	m_Doc->m_plist->HighlightPart( m_sel_part );
	ShowSelectStatus();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// rotate part clockwise 90 degrees clockwise
//
void CFreePcbView::OnPartRotate()
{
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->angle = (m_sel_part->angle + 90)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_nlist->PartMoved( m_sel_part );
	if( m_Doc->m_vis[LAY_RAT_LINE] )
		m_Doc->m_nlist->OptimizeConnections( m_sel_part, m_Doc->m_auto_ratline_disable, 
										m_Doc->m_auto_ratline_min_pins );
	m_Doc->m_plist->HighlightPart( m_sel_part );
	ShowSelectStatus();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnPartRotateCCW()
{
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->angle = (m_sel_part->angle + 270)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_nlist->PartMoved( m_sel_part );
	if( m_Doc->m_vis[LAY_RAT_LINE] )
		m_Doc->m_nlist->OptimizeConnections( m_sel_part, m_Doc->m_auto_ratline_disable, 
										m_Doc->m_auto_ratline_min_pins );
	m_Doc->m_plist->HighlightPart( m_sel_part );
	ShowSelectStatus();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}


void CFreePcbView::OnNetSetWidth()
{
	SetWidth( 2 );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightNetConnections( m_sel_net );
}

void CFreePcbView::OnConnectSetWidth()
{
	SetWidth( 1 );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightConnection( m_sel_net, m_sel_ic );
}

void CFreePcbView::OnSegmentAddVertex()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	SetCursorMode( CUR_DRAG_VTX_INSERT );
	m_Doc->m_nlist->StartDraggingSegmentNewVertex( pDC, m_sel_net, m_sel_ic, m_sel_is,
		p.x, p.y, m_sel_seg->layer,
		m_sel_seg->width, 2 );
}

void CFreePcbView::OnConnectUnroutetrace()
{
	OnUnrouteTrace();
}

void CFreePcbView::OnConnectDeletetrace()
{
	OnSegmentDeleteTrace();
}

void CFreePcbView::OnSegmentChangeLayer()
{
	ChangeTraceLayer( 0, m_sel_seg->layer );
}

void CFreePcbView::OnConnectChangeLayer()
{
	ChangeTraceLayer( 1 );
}

void CFreePcbView::OnNetChangeLayer()
{
	ChangeTraceLayer( 2 );
}

// change layer of routed trace segments
// if mode = 0, current segment
// if mode = 1, current connection
// if mode = 2, current net
//
void CFreePcbView::ChangeTraceLayer( int mode, int old_layer )
{
	CDlgChangeLayer dlg;
	dlg.Initialize( mode, old_layer, m_Doc->m_num_copper_layers );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		int err = 0;
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		cconnect * c = m_sel_con;
		if( dlg.m_apply_to == 0 )
		{
			err = m_Doc->m_nlist->ChangeSegmentLayer( m_sel_net,
						m_sel_id.I2(), m_sel_id.I3(), dlg.m_new_layer );
			if( err )
			{
				AfxMessageBox( "Unable to change layer for this segment" );
			}
		}
		else if( dlg.m_apply_to == 1 )
		{
			for( int is=0; is<c->NumSegs(); is++ )
			{
				if( c->SegByIndex(is).layer >= LAY_TOP_COPPER )
				{
					err += m_Doc->m_nlist->ChangeSegmentLayer( m_sel_net,
						m_sel_id.I2(), is, dlg.m_new_layer );
				}
			}
			if( err )
			{
				AfxMessageBox( "Unable to change layer for all segments" );
			}
		}
		else if( dlg.m_apply_to == 2 )
		{
			for( int ic=0; ic<m_sel_net->NumCons(); ic++ )
			{
				cconnect * c = m_sel_net->ConByIndex(ic);
				for( int is=0; is<c->NumSegs(); is++ )
				{
					if( c->SegByIndex(is).layer >= LAY_TOP_COPPER )
					{
						err += m_Doc->m_nlist->ChangeSegmentLayer( m_sel_net,
							ic, is, dlg.m_new_layer );
					}
				}
			}
			if( err )
			{
				AfxMessageBox( "Unable to change layer for all segments" );
			}
		}
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnNetEditnet()
{
	CDlgEditNet dlg;
	netlist_info nl;
	m_Doc->m_nlist->ExportNetListInfo( &nl );
	int inet = -1;
	for( int i=0; i<nl.GetSize(); i++ )
	{
		if( nl[i].net == m_sel_net )
		{
			inet = i;
			break;
		}
	}
	if( inet == -1 )
		ASSERT(0);
	dlg.Initialize( &nl, inet, m_Doc->m_plist, FALSE, TRUE, m_Doc->m_units,
		&m_Doc->m_w, &m_Doc->m_v_w, &m_Doc->m_v_h_w );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_Doc->ResetUndoState();
		CancelSelection();
		m_Doc->m_nlist->ImportNetListInfo( &nl, 0, NULL,
			m_Doc->m_trace_w, m_Doc->m_via_w, m_Doc->m_via_hole_w );
		Invalidate( FALSE );
	}
}
void CFreePcbView::OnToolsMoveOrigin()
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
			SetCursorMode( CUR_MOVE_ORIGIN );
			m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
				m_last_cursor_point.y, 0, LAY_SELECTION, 2 );
			Invalidate( FALSE );
			ReleaseDC( pDC );
		}
		else
		{
			SaveUndoInfoForMoveOrigin( -dlg.m_x, -dlg.m_x, m_Doc->m_undo_list );
			MoveOrigin( -dlg.m_x, -dlg.m_y );
			OnViewAllElements();
			Invalidate( FALSE );
		}
	}
}

// move origin of coord system by moving everything
// by (x_off, y_off)
//
void CFreePcbView::MoveOrigin( int x_off, int y_off )
{
	for( int ib=0; ib<m_Doc->m_board_outline.GetSize(); ib++ )
		m_Doc->m_board_outline[ib].MoveOrigin( x_off, y_off );
	m_Doc->m_plist->MoveOrigin( x_off, y_off );
	m_Doc->m_nlist->MoveOrigin( x_off, y_off );
	m_Doc->m_tlist->MoveOrigin( x_off, y_off );
	for( int ism=0; ism<m_Doc->m_sm_cutout.GetSize(); ism++ )
		m_Doc->m_sm_cutout[ism].MoveOrigin( x_off, y_off );
}

void CFreePcbView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// save starting position in pixels
	m_bLButtonDown = TRUE;
	m_bDraggingRect = FALSE;
	m_start_pt = point;
	CView::OnLButtonDown(nFlags, point);
}

// Select all items in rectangle
// Fill arrays m_sel_ids[] and m_sel_ptrs[]
// Set utility flags for selected parts and segments
//
void CFreePcbView::SelectItemsInRect( CRect r, BOOL bAddToGroup )
{
	if( !bAddToGroup )
		CancelSelection();
	r.NormalizeRect();

	// find parts in rect
	if( m_sel_mask & (1<<SEL_MASK_PARTS ) )
	{
		cpart * part = m_Doc->m_plist->GetFirstPart();
		while( part )
		{
			CRect p_r;
			if( m_Doc->m_plist->GetPartBoundingRect( part, &p_r ) )
			{
				p_r.NormalizeRect();
				if( InRange( p_r.top, r.top, r.bottom )
					&& InRange( p_r.bottom, r.top, r.bottom )
					&& InRange( p_r.left, r.left, r.right )
					&& InRange( p_r.right, r.left, r.right ) )
				{
					// add part to selection list and highlight it
					id pid = part->m_id;
					pid.SetLevel2( ID_SEL_RECT );
					if( FindItemInGroup( part, &pid ) == -1 )
					{
						m_sel_ids.Add( pid );
						m_sel_ptrs.Add( part );
					}
				}
			}
			part = m_Doc->m_plist->GetNextPart( part );
		}
	}

	// find trace segments and vertices contained in rect
	if( m_sel_mask & (1<<SEL_MASK_CON ) )
	{
		CIterator_cnet iter_net(m_Doc->m_nlist);
		cnet * net = iter_net.GetFirst();
		while( net )
		{
			CIterator_cconnect iter_con( net );
			for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
			{
				int ic = iter_con.GetIndex();
				CIterator_cseg iter_seg( c );
				for( cseg * s=iter_seg.GetFirst(); s; s=iter_seg.GetNext() )
				{
					int is = iter_seg.GetIndex();
					cvertex * pre_v = &s->GetPreVtx();
					cvertex * post_v = &s->GetPostVtx();
					BOOL bPreV = InRange( pre_v->x, r.left, r.right )
						&& InRange( pre_v->y, r.top, r.bottom );
					BOOL bPostV = InRange( post_v->x, r.left, r.right )
						&& InRange( post_v->y, r.top, r.bottom );
					if( bPreV && bPostV
						&& m_Doc->m_vis[s->layer] )
					{
						// add segment to selection list and highlight it
						id sid = s->Id();
						if( FindItemInGroup( net, &sid ) == -1 )
						{
							m_sel_ids.Add( sid );
							m_sel_ptrs.Add( net );
						}
					}
					if( bPreV && (pre_v->tee_ID || pre_v->via_w) )
					{
						id vid = pre_v->Id();
						if( FindItemInGroup( net, &vid ) == -1 )
						{
							m_sel_ids.Add( vid );
							m_sel_ptrs.Add( net );
						}
					}
					if( bPostV && (post_v->tee_ID || post_v->via_w) )
					{
						id vid = post_v->Id();;
						if( FindItemInGroup( net, &vid ) == -1 )
						{
							m_sel_ids.Add( vid );
							m_sel_ptrs.Add( net );
						}
					}
				}
			}
			net = iter_net.GetNext();
		}
	}

	// find texts in rect
	if( m_sel_mask & (1<<SEL_MASK_TEXT ) )
	{
		CIterator_CText iter_t( m_Doc->m_tlist );
		for( CText * t = iter_t.GetFirst(); t != NULL; t = iter_t.GetNext() )		
		{
			if( InRange( m_dlist->Get_x( t->dl_sel ), r.left, r.right )
				&& InRange( m_dlist->Get_xf( t->dl_sel ), r.left, r.right )
				&& InRange( m_dlist->Get_y( t->dl_sel ), r.top, r.bottom )
				&& InRange( m_dlist->Get_yf( t->dl_sel ), r.top, r.bottom )
				&& m_Doc->m_vis[t->m_layer] )
			{
				// add text to selection list and highlight it
				id sid( ID_TEXT, t->m_uid, ID_TEXT, t->m_uid, -1, ID_SEL_TXT );
				if( FindItemInGroup( t, &sid ) == -1 )
				{
					m_sel_ids.Add( sid );
					m_sel_ptrs.Add( t );
				}
			}
		}
	}

	// find copper area sides in rect
	if( m_sel_mask & (1<<SEL_MASK_AREAS ) )
	{
		CIterator_cnet iter_net(m_Doc->m_nlist);
		cnet * net = iter_net.GetFirst();
		while( net )
		{
			if( net->NumAreas() )
			{
				for( int ia=0; ia<net->NumAreas(); ia++ )
				{
					carea * a = &net->area[ia];
					CPolyLine * poly = a;
					for( int ic=0; ic<poly->NumContours(); ic++ )
					{
						int istart = poly->ContourStart(ic);
						int iend = poly->ContourEnd(ic);
						for( int is=istart; is<=iend; is++ )
						{
							int ic1, ic2;
							ic1 = is;
							if( is < iend )
								ic2 = is+1;
							else
								ic2 = istart;
							int x1 = poly->X(ic1);
							int y1 = poly->Y(ic1);
							int x2 = poly->X(ic2);
							int y2 = poly->Y(ic2);
							if( InRange( x1, r.left, r.right )
								&& InRange( x2, r.left, r.right )
								&& InRange( y1, r.top, r.bottom )
								&& InRange( y2, r.top, r.bottom )
								&& m_Doc->m_vis[poly->Layer()] )
							{
								id aid( ID_NET, net->UID(), ID_AREA, a->UID(), ia, 
									ID_SEL_SIDE, poly->SideUID(is), is );
								if( FindItemInGroup( net, &aid ) == -1 )
								{
									m_sel_ids.Add( aid );
									m_sel_ptrs.Add( net );
								}
							}
						}
					}
				}
			}
			net = iter_net.GetNext();
		}
	}

	// find solder mask cutout sides in rect
	if( m_sel_mask & (1<<SEL_MASK_SM ) )
	{
		for( int im=0; im<m_Doc->m_sm_cutout.GetSize(); im++ )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[im];
			for( int ic=0; ic<poly->NumContours(); ic++ )
			{
				int istart = poly->ContourStart(ic);
				int iend = poly->ContourEnd(ic);
				for( int is=istart; is<=iend; is++ )
				{
					int ic1, ic2;
					ic1 = is;
					if( is < iend )
						ic2 = is+1;
					else
						ic2 = istart;
					int x1 = poly->X(ic1);
					int y1 = poly->Y(ic1);
					int x2 = poly->X(ic2);
					int y2 = poly->Y(ic2);
					if( InRange( x1, r.left, r.right )
						&& InRange( x2, r.left, r.right )
						&& InRange( y1, r.top, r.bottom )
						&& InRange( y2, r.top, r.bottom )
						&& m_Doc->m_vis[poly->Layer()] )
					{
						id sm_id = poly->Id();
						sm_id.SetLevel3( ID_SEL_SIDE, poly->SideUID(is) );
						if( FindItemInGroup( poly, &sm_id ) == -1 )
						{
							m_sel_ids.Add( sm_id );
							m_sel_ptrs.Add( NULL );
						}
					}
				}
			}
		}
	}

	// find board outline sides in rect
	if( m_sel_mask & (1<<SEL_MASK_BOARD ) )
	{
		for( int im=0; im<m_Doc->m_board_outline.GetSize(); im++ )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[im];
			for( int ic=0; ic<poly->NumContours(); ic++ )
			{
				int istart = poly->ContourStart(ic);
				int iend = poly->ContourEnd(ic);
				for( int is=istart; is<=iend; is++ )
				{
					int ic1, ic2;
					ic1 = is;
					if( is < iend )
						ic2 = is+1;
					else
						ic2 = istart;
					int x1 = poly->X(ic1);
					int y1 = poly->Y(ic1);
					int x2 = poly->X(ic2);
					int y2 = poly->Y(ic2);
					if( InRange( x1, r.left, r.right )
						&& InRange( x2, r.left, r.right )
						&& InRange( y1, r.top, r.bottom )
						&& InRange( y2, r.top, r.bottom )
						&& m_Doc->m_vis[poly->Layer()] )
					{
						id bd_id = poly->Id();
						bd_id.SetLevel3( ID_SEL_SIDE, poly->SideUID(is) );
						if( FindItemInGroup( poly, &bd_id ) == -1 )
						{
							m_sel_ids.Add( bd_id );
							m_sel_ptrs.Add( NULL );
						}
					}
				}
			}
		}
	}

	// resolve all item ids
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		BOOL bOK = m_sel_ids[i].Resolve();
		if( !bOK )
		{
			ASSERT(0);	
		}
	}
	// now check size of group and select nothing, one item or group
	if( m_sel_ids.GetSize() == 0 )
	{
		CancelSelection();
	}
	else if( m_sel_ids.GetSize() == 1 )
	{
		// convert to single item selection
		m_sel_id = m_sel_ids[0];
		if( SelectItem( m_sel_id ) == FALSE )
		{
			// this shouldn't happen since everything resolved
			CancelSelection();  
		}
	}
	else
	{
		// valid group
		HighlightGroup();
		SetCursorMode( CUR_GROUP_SELECTED );
		FindGroupCenter();
	}
	gLastKeyWasArrow = FALSE;
	gLastKeyWasGroupRotate = false;
}

// Start dragging group being moved or added
// If group is being added (i.e. pasted):
//	bAdd = TRUE;
//	x, y = coords for cursor point for dragging
//
void CFreePcbView::StartDraggingGroup( BOOL bAdd, int x, int y )
{
	if( !bAdd )
	{
		SetCursorMode( CUR_DRAG_GROUP );
	}
	else
	{
		SetCursorMode( CUR_DRAG_GROUP_ADD );
		m_last_mouse_point.x = x;
		m_last_mouse_point.y = y;
	}

	// snap dragging point to placement grid
	SnapCursorPoint( m_last_mouse_point, -1 );
	m_from_pt = m_last_cursor_point;

	// make texts, parts and segments invisible
	m_dlist->SetLayerVisible( LAY_HILITE, FALSE );
	int n_parts = 0;
	int n_segs = 0;
	int n_texts = 0;
	int n_area_sides = 0;
	int n_sm_sides = 0;
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.IsPart() )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			m_Doc->m_plist->MakePartVisible( part, FALSE );
			n_parts++;
		}
		else if( sid.IsSeg() )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			dl_element * dl = net->ConByIndex(sid.I2())->SegByIndex(sid.I3()).dl_el;
			m_dlist->Set_visible( dl, FALSE );
			m_Doc->m_nlist->SetViaVisible( net, sid.I2(), sid.I3(), FALSE );
			m_Doc->m_nlist->SetViaVisible( net, sid.I2(), sid.I3()+1, FALSE );
			n_segs++;
		}
		else if( sid.IsAreaSide() )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			carea * a = &net->area[sid.I2()];
//			a->SetSideVisible( sid.I3(), FALSE );
			a->MakeVisible( FALSE );
			n_area_sides++;
		}
		else if( sid.IsMaskSide() )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.I2()];
//			poly->SetSideVisible( sid.I3(), FALSE );
			poly->MakeVisible( FALSE );
			n_sm_sides++;
		}
		else if( sid.IsBoardSide() )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[sid.I2()];
//			poly->SetSideVisible( sid.I3(), FALSE );
			poly->MakeVisible( FALSE );
			n_sm_sides++;
		}
		else if( sid.IsText() )
		{
			// make text strokes invisible
			CText * text = (CText*)m_sel_ptrs[i];
			for( int is=0; is<text->m_stroke.GetSize(); is++ )
				((dl_element*)text->m_stroke[is].dl_el)->visible = 0;
			n_texts++;
		}
	}

	// set up dragline array
	m_dlist->MakeDragLineArray( n_parts*4 + n_segs + n_texts*4 + n_area_sides + n_sm_sides );
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.IsPart() )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			int xi = part->shape->m_sel_xi;
			int xf = part->shape->m_sel_xf;
			if( part->side )
			{
				xi = -xi;
				xf = -xf;
			}
			int yi = part->shape->m_sel_yi;
			int yf = part->shape->m_sel_yf;
			CPoint p1( xi, yi );
			CPoint p2( xf, yi );
			CPoint p3( xf, yf );
			CPoint p4( xi, yf );
			RotatePoint( &p1, part->angle, zero );
			RotatePoint( &p2, part->angle, zero );
			RotatePoint( &p3, part->angle, zero );
			RotatePoint( &p4, part->angle, zero );
			p1.x += part->x - m_from_pt.x;
			p2.x += part->x - m_from_pt.x;
			p3.x += part->x - m_from_pt.x;
			p4.x += part->x - m_from_pt.x;
			p1.y += part->y - m_from_pt.y;
			p2.y += part->y - m_from_pt.y;
			p3.y += part->y - m_from_pt.y;
			p4.y += part->y - m_from_pt.y;
			m_dlist->AddDragLine( p1, p2 );
			m_dlist->AddDragLine( p2, p3 );
			m_dlist->AddDragLine( p3, p4 );
			m_dlist->AddDragLine( p4, p1 );
		}
		else if( sid.IsSeg() )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			cconnect * c = net->ConByIndex(sid.I2());
			cseg * s = &c->SegByIndex(sid.I3());
			cvertex * v1 = &s->GetPreVtx();
			cvertex * v2 = &s->GetPostVtx();
			CPoint p1( v1->x - m_from_pt.x, v1->y - m_from_pt.y );
			CPoint p2( v2->x - m_from_pt.x, v2->y - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 );
		}
		else if( sid.IsAreaSide() )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			carea * a = &net->area[sid.I2()];
			CPolyLine * poly = a;
			int icontour = poly->Contour(sid.I3());
			int ic1 = sid.I3();
			int ic2 = sid.I3()+1;
			if( ic2 > poly->ContourEnd(icontour) )
				ic2 = poly->ContourStart(icontour);
			CPoint p1( poly->X(ic1) - m_from_pt.x, poly->Y(ic1) - m_from_pt.y );
			CPoint p2( poly->X(ic2) - m_from_pt.x, poly->Y(ic2) - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 );
		}
		else if( sid.IsMaskSide() )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.I2()];
			int icontour = poly->Contour(sid.I3());
			int ic1 = sid.I3();
			int ic2 = sid.I3()+1;
			if( ic2 > poly->ContourEnd(icontour) )
				ic2 = poly->ContourStart(icontour);
			CPoint p1( poly->X(ic1) - m_from_pt.x, poly->Y(ic1) - m_from_pt.y );
			CPoint p2( poly->X(ic2) - m_from_pt.x, poly->Y(ic2) - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 );
		}
		else if( sid.IsBoardSide() )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[sid.I2()];
			int icontour = poly->Contour(sid.I3());
			int ic1 = sid.I3();
			int ic2 = sid.I3()+1;
			if( ic2 > poly->ContourEnd(icontour) )
				ic2 = poly->ContourStart(icontour);
			CPoint p1( poly->X(ic1) - m_from_pt.x, poly->Y(ic1) - m_from_pt.y );
			CPoint p2( poly->X(ic2) - m_from_pt.x, poly->Y(ic2) - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 );
		}
		else if( sid.IsText() )
		{
			CText * text = (CText*)m_sel_ptrs[i];
			CPoint p1( m_dlist->Get_x( text->dl_sel ), m_dlist->Get_y( text->dl_sel ) );
			CPoint p2( m_dlist->Get_xf( text->dl_sel ), m_dlist->Get_y( text->dl_sel ) );
			CPoint p3( m_dlist->Get_xf( text->dl_sel ), m_dlist->Get_yf( text->dl_sel ) );
			CPoint p4( m_dlist->Get_x( text->dl_sel ), m_dlist->Get_yf( text->dl_sel ) );
			p1 -= m_from_pt;
			p2 -= m_from_pt;
			p3 -= m_from_pt;
			p4 -= m_from_pt;
			m_dlist->AddDragLine( p1, p2 );
			m_dlist->AddDragLine( p2, p3 );
			m_dlist->AddDragLine( p3, p4 );
			m_dlist->AddDragLine( p4, p1 );
		}

	}
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p;
	p.x  = m_from_pt.x;
	p.y  = m_from_pt.y;
	CPoint cur_p = m_dlist->PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	m_dlist->StartDraggingArray( pDC, m_from_pt.x, m_from_pt.y, 0, LAY_SELECTION, TRUE );
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

void CFreePcbView::CancelDraggingGroup()
{
	m_dlist->StopDragging();
	// make elements visible again
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_PART )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			m_Doc->m_plist->MakePartVisible( part, TRUE );
		}
		else if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT
			&& sid.T3() == ID_SEL_SEG )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			dl_element * dl = net->ConByIndex(sid.I2())->SegByIndex(sid.I3()).dl_el;
			m_dlist->Set_visible( dl, TRUE );
			m_Doc->m_nlist->SetViaVisible( net, sid.I2(), sid.I3(), TRUE );
			m_Doc->m_nlist->SetViaVisible( net, sid.I2(), sid.I3()+1, TRUE );
		}
		else if( sid.T1() == ID_NET && sid.T2() == ID_AREA
			&& sid.T3() == ID_SEL_SIDE )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			carea * a = &net->area[sid.I2()];
//			a->SetSideVisible( sid.I3(), TRUE );
			a->MakeVisible( TRUE );
		}
		else if( sid.T1() == ID_MASK && sid.T2() == ID_MASK
			&& sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.I2()];
//			poly->SetSideVisible( sid.I3(), TRUE );
			poly->MakeVisible( TRUE );
		}
		else if( sid.T1() == ID_BOARD && sid.T2() == ID_BOARD
			&& sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[sid.I2()];
//			poly->SetSideVisible( sid.I3(), TRUE );
			poly->MakeVisible( TRUE );
		}
		else if( sid.T1() == ID_TEXT )
		{
			// make text strokes invisible
			CText * text = (CText*)m_sel_ptrs[i];
			for( int is=0; is<text->m_stroke.GetSize(); is++ )
				((dl_element*)text->m_stroke[is].dl_el)->visible = TRUE;
		}
	}
	m_dlist->SetLayerVisible( LAY_HILITE, TRUE );
	SetCursorMode( CUR_GROUP_SELECTED );
	Invalidate( FALSE );
}

void CFreePcbView::OnGroupMove()
{
	if( GluedPartsInGroup() )
	{
		int ret = AfxMessageBox( "This group contains glued parts, unglue and move them ?", MB_OKCANCEL );
		if( ret != IDOK )
			return;
	}
	m_dlist->SetLayerVisible( LAY_RAT_LINE, FALSE );
	StartDraggingGroup();
}


// Move group of parts and trace segments
//
void CFreePcbView::MoveGroup( int dx, int dy )
{
	UngluePartsInGroup();

	// mark all parts and nets as unselected
	m_Doc->m_nlist->MarkAllNets(0);
	m_Doc->m_plist->MarkAllParts(0);

	// mark all corners of solder mask cutouts as unmoved
	for( int im=0; im<m_Doc->m_sm_cutout.GetSize(); im++ )
	{
		CPolyLine * poly = &m_Doc->m_sm_cutout[im];
		poly->SetUtility(0);
		for( int ic=0; ic<poly->NumCorners(); ic++ )
			poly->SetUtility( ic, 0 );	// unmoved
	}
	// mark all corners of board outlines as unmoved
	for( int im=0; im<m_Doc->m_board_outline.GetSize(); im++ )
	{
		CPolyLine * poly = &m_Doc->m_board_outline[im];
		poly->SetUtility(0);
		for( int ic=0; ic<poly->NumCorners(); ic++ )
			poly->SetUtility( ic, 0 );	// unmoved
	}

	// mark nets of ID_NET items as selected
	// mark areas as selected and undraw them
	// mark solder mask cutouts as selected and undraw them
	// mark board outlines as selected and undraw them
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			net->utility = TRUE;
			if( sid.T2() == ID_AREA )
			{
				cnet * net = (cnet*)m_sel_ptrs[i];
				carea * a = &net->area[sid.I2()];
				a->utility = TRUE;
				CPolyLine * poly = a;
				poly->SetUtility(1);
				poly->Undraw();
			}
		}
		else if( sid.T1() == ID_MASK && sid.T2() == ID_MASK && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.I2()];
			poly->SetUtility(1);
			poly->Undraw();
		}
		else if( sid.T1() == ID_BOARD && sid.T2() == ID_OUTLINE && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[sid.I2()];
			poly->SetUtility(1);
			poly->Undraw();
		}
	}

	// mark all relevant parts, nets, connections, segments, vertices 
	// and areas as selected
	// and move text and copper area corners
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT && sid.T3() == ID_SEL_SEG )
		{
			// segment
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.I2();
			int is = sid.I3();
			cconnect * c = net->ConByIndex(ic);	// this connection
			cseg * s = &c->SegByIndex(is);				// this segment
			cvertex * pre_v = &s->GetPreVtx();
			cvertex * post_v = &s->GetPostVtx();
			c->utility = TRUE;					// mark connection selected
			s->utility = TRUE;					// mark segment selected
			pre_v->utility =  TRUE;				// mark adjacent vertices as selected
			post_v->utility =  TRUE;
		}
		else if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT && sid.T3() == ID_SEL_VERTEX )
		{
			// vertex
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.I2();
			int iv = sid.I3();
			cconnect * c = net->ConByIndex(ic);	// this connection
			cvertex * v = &c->VtxByIndex(iv);
			c->utility = TRUE;					// mark connection selected
			v->utility = TRUE;					// mark vertex selected
		}
		else if( sid.T1() == ID_PART && sid.T2() == ID_SEL_RECT )
		{
			// part
			cpart * part = (cpart*)m_sel_ptrs[i];
			part->utility = TRUE;	// mark part selected
		}
		else if( sid.T1() == ID_TEXT && sid.T2() == ID_TEXT && sid.T3() == ID_SEL_TXT )
		{
			// text
			CText * t = (CText*)m_sel_ptrs[i];
			m_Doc->m_tlist->MoveText( t, t->m_x+dx, t->m_y+dy, t->m_angle,
				t->m_mirror, t->m_bNegative, t->m_layer );
		}
		else if( sid.T1() == ID_NET && sid.T2() == ID_AREA && sid.T3() == ID_SEL_SIDE )
		{
			// area side
			cnet * net = (cnet*)m_sel_ptrs[i];
			CPolyLine * poly = &net->area[sid.I2()];
			int icontour = poly->Contour(sid.I3());
			int istart = poly->ContourStart(icontour);
			int iend = poly->ContourEnd(icontour);
			int ic1 = sid.I3();
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			if( !poly->Utility(ic1) )
			{
				// unmoved, move it
				poly->SetX( ic1, poly->X(ic1) + dx );
				poly->SetY( ic1, poly->Y(ic1) + dy );
				poly->SetUtility(ic1,1);
			}
			if( !poly->Utility(ic2) )
			{
				// unmoved, move it
				poly->SetX( ic2, poly->X(ic2) + dx );
				poly->SetY( ic2, poly->Y(ic2) + dy );
				poly->SetUtility(ic2,1);
			}
		}
		// sm_cutout side
		else if( sid.T1() == ID_MASK && sid.T2() == ID_MASK && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.I2()];
			int icontour = poly->Contour(0);
			int istart = poly->ContourStart(icontour);
			int iend = poly->ContourEnd(icontour);
			int ic1 = sid.I3();
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			if( !poly->Utility(ic1) )
			{
				// unmoved, move it
				poly->SetX( ic1, poly->X(ic1) + dx );
				poly->SetY( ic1, poly->Y(ic1) + dy );
				poly->SetUtility(ic1,1);
			}
			if( !poly->Utility(ic2) )
			{
				// unmoved, move it
				poly->SetX( ic2, poly->X(ic2) + dx );
				poly->SetY( ic2, poly->Y(ic2) + dy );
				poly->SetUtility(ic2,1);
			}
		}
		// board outline side
		else if( sid.T1() == ID_BOARD && sid.T2() == ID_BOARD && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[sid.I2()];
			int icontour = poly->Contour(0);
			int istart = poly->ContourStart(icontour);
			int iend = poly->ContourEnd(icontour);
			int ic1 = sid.I3();
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			if( !poly->Utility(ic1) )
			{
				// unmoved, move it
				poly->SetX( ic1, poly->X(ic1) + dx );
				poly->SetY( ic1, poly->Y(ic1) + dy );
				poly->SetUtility(ic1,1);
			}
			if( !poly->Utility(ic2) )
			{
				// unmoved, move it
				poly->SetX( ic2, poly->X(ic2) + dx );
				poly->SetY( ic2, poly->Y(ic2) + dy );
				poly->SetUtility(ic2,1);
			}
		}
		else
			ASSERT(0);
	}

	// now redraw areas, solder mask cutouts and board outlines
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			net->utility = TRUE;
			if( sid.T2() == ID_AREA )
			{
				cnet * net = (cnet*)m_sel_ptrs[i];
				carea * a = &net->area[sid.I2()];
				CPolyLine * poly = a;
				if( poly->Utility() )
				{
					poly->Draw();
					poly->SetUtility(0);	// clear flag so only redraw once
				}
			}
		}
		else if( sid.T1() == ID_MASK && sid.T2() == ID_MASK && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.I2()];
			if( poly->Utility() )
			{
				poly->Draw();
				poly->SetUtility(0);	// clear flag so only redraw once
			}
		}
		else if( sid.T1() == ID_BOARD && sid.T2() == ID_OUTLINE && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[sid.I2()];
			if( poly->Utility() )
			{
				poly->Draw();
				poly->SetUtility(0);	// clear flag so only redraw once
			}
		}
	}

	// assume utility flags have been set on selected parts,
	// nets, connections, segments and vertices

	// move parts in group
	cpart * part = m_Doc->m_plist->GetFirstPart();
	while( part != NULL )
	{
		if( part->utility )
		{
			// move part
			m_Doc->m_plist->Move( part, part->x+dx, part->y+dy, part->angle, part->side );
			// find segments which connect to this part and move them
			// use net->utility2 to avoid repeats
			CIterator_cnet iter_net(m_Doc->m_nlist);
			cnet * net;
			for( net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
				net->utility2 = 0;
			for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
			{
				net = (cnet*)part->pin[ip].net; 
				if( net && net->utility2 == 0 )
				{
					net->utility2 = 1;
					for( int ic=0; ic<net->NumCons(); ic++ )
					{
						cconnect * c = net->ConByIndex(ic);
						int nsegs = c->NumSegs();
						if( nsegs )
						{
							int p1 = c->start_pin;
							CString pin_name1 = net->pin[p1].pin_name;
							int pin_index1 = part->shape->GetPinIndexByName( pin_name1 );
							int p2 = c->end_pin;
							if( net->pin[p1].part == part )
							{
								// starting pin is on part
								if( !c->SegByIndex(0).utility && c->SegByIndex(0).layer != LAY_RAT_LINE )
								{
									// first segment is not selected, unroute it
									if( !c->SegByIndex(0).utility )
										m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, 0 );
								}
								// move vertex if not selected
								if( !c->VtxByIndex(0).utility )
								{
									m_Doc->m_nlist->MoveVertex( net, ic, 0,
										part->pin[pin_index1].x, part->pin[pin_index1].y );
									c->VtxByIndex(0).utility2 = 1; // moved
								}
							}
							if( p2 != cconnect::NO_END )
							{
								if( net->pin[p2].part == part )
								{
									// ending pin is on part
									if( c->SegByIndex(nsegs-1).layer != LAY_RAT_LINE )
									{
										// unroute it if not selected
										if( !c->SegByIndex(nsegs-1).utility )
											m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, nsegs-1 );
									}
									// modify vertex position if necessary
									if( !c->VtxByIndex(nsegs).utility )
									{
										// move vertex if unselected
										CString pin_name2 = net->pin[p2].pin_name;
										int pin_index2 = part->shape->GetPinIndexByName( pin_name2 );
										m_Doc->m_nlist->MoveVertex( net, ic, nsegs,
											part->pin[pin_index2].x, part->pin[pin_index2].y );
										c->VtxByIndex(nsegs).utility2 =  1;	// moved

									}
								}
							}
						}
					}
				}
			}
		}
		part = m_Doc->m_plist->GetNextPart( part );
	}

	// get selected segments
	CIterator_cnet iter_net(m_Doc->m_nlist);
	cnet * net = iter_net.GetFirst();
	while( net != NULL )
	{
		if( net->utility )
		{
			CIterator_cconnect iter_con( net );
			for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
			{
				int ic = iter_con.GetIndex();
				if( c->utility )
				{
					// undraw entire trace
					c->Undraw();
					CIterator_cseg iter_seg( c );
					for( cseg * s=iter_seg.GetFirst(); s; s=iter_seg.GetNext() )
					{
						int is = iter_seg.GetIndex();
						if( s->utility )
						{
							// move trace segment by flagging adjacent vertices
							cvertex * pre_v = &s->GetPreVtx();		// pre vertex
							cvertex * post_v = &s->GetPostVtx();	// post vertex
							CPoint old_pre_v_pt( pre_v->x, pre_v->y );		// pre vertex coords
							CPoint old_post_v_pt( post_v->x, post_v->y );	// post vertex coords
							cpart * part1 = net->pin[c->start_pin].part;	// connection starting part
							cpart * part2 = NULL;				// connection ending part or NULL
							if( c->end_pin != cconnect::NO_END )
								part2 = net->pin[c->end_pin].part;

							// flag adjacent vertices as selected
							pre_v->utility = TRUE;
							post_v->utility = TRUE;

							// unroute adjacent segments unless they are also being moved
							if( is>0 )
							{
								// test for preceding segment
								if( !c->SegByIndex(is-1).utility )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, is-1 );
							}
							if( is < c->NumSegs()-1 )
							{
								// test for following segment and not end of stub trace
								if( !c->SegByIndex(is+1).utility && (part2 || is < c->NumSegs()-2) )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, is+1 );
							}
						}
					}
					// now move vertices
					CIterator_cvertex iter_vtx( c );
					for( cvertex * v=iter_vtx.GetFirst(); v; v=iter_vtx.GetNext() )
					{
						int iv = iter_vtx.GetIndex();
						if( v->utility && !v->utility2 )
						{
							// selected and not already moved
							v->x += dx;
							v->y += dy;
							v->utility2 = TRUE;	// moved
							// if adjacent segments were not selected, unroute them
							if( iv>0 )
							{
								cseg * pre_s = &c->SegByIndex(iv-1);
								if( pre_s->utility == 0 )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, iv-1 );
							}
							if( iv<c->NumSegs() )
							{
								cseg * post_s = &c->SegByIndex(iv);
								if( post_s->utility == 0 )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, iv );
							}
						}
					}

					// now some special cases
					for( int is=0; is<c->NumSegs(); is++ )
					{
						if( c->SegByIndex(is).utility )
						{
							// move trace segment
							cseg * s = &c->SegByIndex(is);				// this segment
							cvertex * pre_v = &s->GetPreVtx();		// pre vertex
							cvertex * post_v = &s->GetPostVtx();	// post vertex
							cpart * part1 = net->pin[c->start_pin].part;	// connection starting part
							cpart * part2 = NULL;				// connection ending part or NULL
							if( c->end_pin != cconnect::NO_END )
								part2 = net->pin[c->end_pin].part;

							// special case, first segment of trace selected but part not selected
							if( part1->utility == FALSE && is == 0 )
							{
								// insert ratline as new first segment, unselected
								CPoint new_v_pt( pre_v->x, pre_v->y );
								CPoint old_v_pt = m_Doc->m_plist->GetPinPoint( part1, net->pin[c->start_pin].pin_name );		// pre vertex coords
								m_Doc->m_nlist->MoveVertex( net, ic, 0, old_v_pt.x, old_v_pt.y );
								m_Doc->m_nlist->InsertSegment( net, ic, 0, new_v_pt.x, new_v_pt.y, LAY_RAT_LINE, 1, 0, 0, 0 );
								c->SegByIndex(0).utility = 0;
								c->VtxByIndex(0).utility = 0;
								is++;
							}

							// special case, last segment of trace selected but part not selected
							if( part2 )
							{
								if( part2->utility == FALSE && is == c->NumSegs()-1 )
								{
									// insert ratline as new last segment
									int old_w = c->SegByIndex(c->NumSegs()-1).width;
									int old_v_w = c->VtxByIndex(c->NumSegs()-1).via_w;
									int old_v_h_w = c->VtxByIndex(c->NumSegs()-1).via_hole_w;
									int old_layer = c->SegByIndex(c->NumSegs()-1).layer;
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, c->NumSegs()-1 );
									CPoint new_v_pt( c->VtxByIndex(c->NumSegs()).x, c->VtxByIndex(c->NumSegs()).y );
									CPoint old_v_pt = m_Doc->m_plist->GetPinPoint( part2, net->pin[c->end_pin].pin_name );
									m_Doc->m_nlist->MoveVertex( net, ic, c->NumSegs(), old_v_pt.x, old_v_pt.y );
									BOOL bInserted = m_Doc->m_nlist->InsertSegment( net, ic, c->NumSegs()-1,
										new_v_pt.x, new_v_pt.y, old_layer, old_w, old_v_w, old_v_h_w, 0 );
									c->SegByIndex(c->NumSegs()-2).utility = 1;
									c->SegByIndex(c->NumSegs()-1).utility = 0;
								}
							}
						}
					}
					m_Doc->m_nlist->MergeUnroutedSegments( net, ic );	// this also redraws connection
				}
			}

			// now deal with tees that have been moved
			// requiring that stubs attached to tees have to move as well
			// if attached segments have not been selected, they must be unrouted
			for( int ic=0; ic<net->NumCons(); ic++ )
			{
				cconnect * c = net->ConByIndex(ic);
				if( c->end_pin == cconnect::NO_END )
				{
					cvertex * end_vtx = &c->VtxByIndex(c->NumSegs());
					cseg * end_seg = &c->SegByIndex(c->NumSegs()-1);
					if( int id = end_vtx->tee_ID )
					{
						// stub tee
						int tee_ic;
						int tee_iv;
						BOOL bFound = m_Doc->m_nlist->FindTeeVertexInNet( net, id, &tee_ic, &tee_iv );
						if ( !bFound )
						{
							end_vtx->tee_ID = 0;
						}
						else
						{
							cvertex * tee_vtx;
							tee_vtx = &net->ConByIndex(tee_ic)->VtxByIndex(tee_iv);
							if( tee_vtx->utility2 )
							{
								// tee-vertex was moved
								end_vtx->x = tee_vtx->x;
								end_vtx->y = tee_vtx->y;
								if( !end_seg->utility )
								{
									// attached segment not selected
									c->Undraw();
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, c->NumSegs()-1 );
									c->Draw();
								}
							}
						}
					}
				}
			}
		}
		net = iter_net.GetNext();
	}

	// merge unrouted segments for all traces
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT
			&& sid.T3() == ID_SEL_SEG )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.I2();
			m_Doc->m_nlist->MergeUnroutedSegments( net, ic );
		}
	}

	//** this shouldn't be necessary
	CNetList * nl = m_Doc->m_nlist;
	for( cnet * n=iter_net.GetFirst(); n; n=iter_net.GetNext() )
		nl->RehookPartsToNet( n );
	//**
	m_Doc->m_nlist->SetAreaConnections();

	// regenerate selection list from utility flags
	// first, remove all segments and vertices
	for( int i=m_sel_ids.GetSize()-1; i>=0; i-- )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT && sid.T3() == ID_SEL_SEG )
		{
			m_sel_ids.RemoveAt(i);
			m_sel_ptrs.RemoveAt(i);
		}
		else if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT && sid.T3() == ID_SEL_VERTEX )
		{
			m_sel_ids.RemoveAt(i);
			m_sel_ptrs.RemoveAt(i);
		}
	}
	// add segments and vertices back into group
	net = iter_net.GetFirst();
	while( net )
	{
		if( net->utility )
		{
			// selected net
			for( int ic=0; ic<net->NumCons(); ic++ )
			{
				cconnect * c = net->ConByIndex(ic);
				if( c->utility )
				{
					// selected connection
					for( int is=0; is<c->NumSegs(); is++ )
					{
						if( c->SegByIndex(is).utility )
						{
							m_sel_ptrs.Add( net );
							id sid( ID_NET, net->UID(), ID_CONNECT, c->UID(), ic, 
								ID_SEL_SEG, c->SegByIndex(is).UID(), is );
							m_sel_ids.Add( sid );
							c->SegByIndex(is).dl_el->visible = 1;	// restore visibility
						}
					}
					for( int iv=0; iv<c->NumSegs()+1; iv++ )
					{
						cvertex * v = &c->VtxByIndex(iv);
						if( v->utility )
						{
							if( v->via_w || v->tee_ID )
							{
								m_sel_ptrs.Add( net );
								id vid( ID_NET, net->UID(), ID_CONNECT, c->UID(), ic, 
									ID_SEL_VERTEX, v->UID(), iv );
								m_sel_ids.Add( vid );
								if( v->via_w )
								{
									int n_el = v->dl_el.GetSize();
									for( int il=0; il<n_el; il++ )
										v->dl_el[il]->visible = 1;
								}
							}
						}
					}
				}
			}
		}
		net = iter_net.GetNext();
	}
	groupAverageX+=dx;
	groupAverageY+=dy;
}

// Highlight group selection
// the only legal members are parts, texts, trace segments and
// copper area, solder mask cutout and board outline sides
//
void CFreePcbView::HighlightGroup()
{
	m_dlist->CancelHighLight();
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_PART && sid.T2() == ID_SEL_RECT )
			m_Doc->m_plist->HighlightPart( (cpart*)m_sel_ptrs[i] );
		else if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT && sid.T3() == ID_SEL_SEG )
			m_Doc->m_nlist->HighlightSegment( (cnet*)m_sel_ptrs[i], sid.I2(), sid.I3() );
		else if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT && sid.T3() == ID_SEL_VERTEX )
		{
			cvertex * v = &((cnet*)m_sel_ptrs[i])->ConByIndex(sid.I2())->VtxByIndex(sid.I3());
			if( v->tee_ID || v->force_via_flag )
				m_Doc->m_nlist->HighlightVertex( (cnet*)m_sel_ptrs[i], sid.I2(), sid.I3() );
		}
		else if( sid.T1() == ID_TEXT && sid.T2() == ID_TEXT && sid.T3() == ID_SEL_TXT )
			m_Doc->m_tlist->HighlightText( (CText*)m_sel_ptrs[i] );
		else if( sid.T1() == ID_NET && sid.T2() == ID_AREA && sid.T3() == ID_SEL_SIDE )
			((cnet*)m_sel_ptrs[i])->area[sid.I2()].HighlightSide(sid.I3());
		else if( sid.T1() == ID_MASK && sid.T2() == ID_MASK && sid.T3() == ID_SEL_SIDE )
			m_Doc->m_sm_cutout[sid.I2()].HighlightSide(sid.I3());
		else if( sid.T1() == ID_BOARD && sid.T2() == ID_BOARD && sid.T3() == ID_SEL_SIDE )
			m_Doc->m_board_outline[sid.I2()].HighlightSide(sid.I3());
		else
			ASSERT(0);
	}
}

// Find item in group by id
// returns index of item if found, otherwise -1
//
int CFreePcbView::FindItemInGroup( void * ptr, id * tid )
{
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		if( m_sel_ptrs[i] == ptr && m_sel_ids[i] == *tid )
			return i;
	}
	return -1;
}

// Test for glued parts in group
//
BOOL CFreePcbView::GluedPartsInGroup()
{
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		if( m_sel_ids[i].T1() == ID_PART )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			if( part->glued )
				return TRUE;
		}
	}
	return FALSE;
}

// Unglue parts in group
// returns index of item if found, otherwise -1
//
void CFreePcbView::UngluePartsInGroup()
{
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		if( m_sel_ids[i].T1() == ID_PART )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			part->glued = FALSE;
		}
	}
}

// Set array of selection mask ids
//
void CFreePcbView::SetSelMaskArray( int mask )
{
	for( int i=0; i<NUM_SEL_MASKS; i++ )
	{
		if( mask & (1<<i) )
			m_mask_id[i].SetI3( -1 );
		else
			m_mask_id[i].SetI3( 0xfffe );	// guaranteed not to exist
	}
}


void CFreePcbView::OnAddSimilarArea()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dlist->CancelHighLight();
	SetCursorMode( CUR_ADD_AREA );
	m_active_layer = m_sel_net->area[m_sel_ia].Layer();
	m_Doc->m_vis[m_active_layer] = TRUE;
	m_dlist->SetLayerVisible( m_active_layer, TRUE );
	ShowActiveLayer();
	m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
		m_last_cursor_point.y, 0, m_active_layer, 2 );
	m_polyline_style = CPolyLine::STRAIGHT;
	m_polyline_hatch = m_sel_net->area[m_sel_ia].GetHatch();
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

void CFreePcbView::OnAreaEdit()
{
	CDlgAddArea dlg;
	int layer = m_sel_net->area[m_sel_id.I2()].Layer();
	int hatch = m_sel_net->area[m_sel_id.I2()].GetHatch();
	dlg.Initialize( m_Doc->m_nlist, m_Doc->m_num_layers, m_sel_net, layer, hatch );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		cnet * net = dlg.m_net;
		if( m_sel_net == net )
		{
			SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
		}
		else
		{
			// move area to new net
			SaveUndoInfoForAllAreasIn2Nets( m_sel_net, net, TRUE, m_Doc->m_undo_list );
			int ia = m_Doc->m_nlist->AddArea( net, dlg.m_layer, 0, 0, 0 );
			net->area[ia].Copy( &m_sel_net->area[m_sel_ia] );
			net->area[ia].SetPtr( net );
			id new_id = net->area[ia].Id();
			new_id.SetI2( ia );
			net->area[ia].SetId( &new_id );
			m_Doc->m_nlist->RemoveArea( m_sel_net, m_sel_ia ); 
			m_Doc->m_nlist->SetAreaConnections( net, ia );
			if( m_Doc->m_vis[LAY_RAT_LINE] )
				m_Doc->m_nlist->OptimizeConnections(  net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
			CancelSelection();
			m_sel_net = net;
			m_sel_id.SetI2( ia );
		}
		m_sel_net->area[m_sel_ia].Undraw();
		m_sel_net->area[m_sel_ia].SetLayer( dlg.m_layer );
		m_sel_net->area[m_sel_ia].SetHatch( dlg.m_hatch );
		m_sel_net->area[m_sel_ia].Draw();
		int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, TRUE );
		if( ret == -1 )
		{
			// error
			AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
			CancelSelection();
			m_Doc->OnEditUndo();
		}
		if( m_Doc->m_vis[LAY_RAT_LINE] )
			m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE  );
		CancelSelection();
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnAreaEdgeApplyClearances()
{
#if 0	//** this is for testing only
	SaveUndoInfoForAllNetsAndConnectionsAndAreas( TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->ApplyClearancesToArea( m_sel_net, m_sel_ia, m_Doc->m_cam_flags,
		m_Doc->m_fill_clearance, m_Doc->m_min_silkscreen_stroke_wid,
		m_Doc->m_thermal_width, m_Doc->m_hole_clearance );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

void CFreePcbView::ReselectNetItemIfConnectionsChanged( int new_ic )
{
	if( new_ic >= 0 && new_ic < m_sel_net->NumCons()
		&& (m_cursor_mode == CUR_SEG_SELECTED
		|| m_cursor_mode == CUR_RAT_SELECTED
		|| m_cursor_mode == CUR_VTX_SELECTED
		|| m_cursor_mode == CUR_END_VTX_SELECTED
		|| m_cursor_mode == CUR_CONNECT_SELECTED
		|| m_cursor_mode == CUR_NET_SELECTED ) )
	{
		m_Doc->m_dlist->CancelHighLight();
		m_sel_id.SetI2( new_ic );
		if( m_cursor_mode == CUR_SEG_SELECTED )
			m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		else if( m_cursor_mode == CUR_RAT_SELECTED )
			m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		else if( m_cursor_mode == CUR_VTX_SELECTED )
			m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_iv );
		else if( m_cursor_mode == CUR_END_VTX_SELECTED )
			m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_iv );
		else if( m_cursor_mode == CUR_CONNECT_SELECTED )
			m_Doc->m_nlist->HighlightConnection( m_sel_net, m_sel_ic );
		else if( m_cursor_mode == CUR_NET_SELECTED )
			m_Doc->m_nlist->HighlightNetConnections( m_sel_net );
	}
	else
		CancelSelection();
}

void CFreePcbView::OnGroupCopy()
{
	// clear clipboard
	m_Doc->clip_sm_cutout.SetSize(0);
	m_Doc->clip_board_outline.SetSize(0);
	m_Doc->clip_tlist->RemoveAllTexts();
	m_Doc->clip_nlist->RemoveAllNets();
	m_Doc->clip_plist->RemoveAllParts();

	// set pointers
	CArray<CPolyLine> * g_sm = &m_Doc->clip_sm_cutout;
	CArray<CPolyLine> * g_bd = &m_Doc->clip_board_outline;
	CPartList * g_pl = m_Doc->clip_plist;
	CNetList * g_nl = m_Doc->clip_nlist;
	CTextList * g_tl = m_Doc->clip_tlist;

	// add all parts and text from group
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_PART && sid.T2() == ID_SEL_RECT )
		{
			// add part to group partlist
			cpart * part = (cpart*)m_sel_ptrs[i];
			CShape * shape = part->shape;
			cpart * g_part = g_pl->Add( part->shape, &part->ref_des, &part->package, part->x, part->y,
				part->side, part->angle, 1, 0 );
			// set ref text parameters
			g_part->m_ref_angle = part->m_ref_angle;
			g_part->m_ref_size = part->m_ref_size;
			g_part->m_ref_w = part->m_ref_w;
			g_part->m_ref_xi = part->m_ref_xi;
			g_part->m_ref_yi = part->m_ref_yi;
			g_part->m_ref_layer = part->m_ref_layer;
			g_part->m_ref_vis = part->m_ref_vis;
			// set value parameters
			g_part->value = part->value;
			g_part->m_value_angle = part->m_value_angle;
			g_part->m_value_size = part->m_value_size;
			g_part->m_value_w = part->m_value_w;
			g_part->m_value_xi = part->m_value_xi;
			g_part->m_value_yi = part->m_value_yi;
			g_part->m_value_layer = part->m_value_layer;
			g_part->m_value_vis = part->m_value_vis;
			// add pin nets to group netlist
			for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
			{
				part_pin * pin = &part->pin[ip];
				CShape * shape = part->shape;
				cnet * net = pin->net;
				if( net )
				{
					// add net to group netlist if not already added
					cnet * g_net = g_nl->GetNetPtrByName( &net->name );
					if( g_net == NULL )
					{
						g_net = g_nl->AddNet( net->name, net->NumPins(), net->def_w, net->def_via_w, net->def_via_hole_w );
					}
					// add pin to net
					CString pin_name = shape->GetPinNameByIndex( ip );
					g_net->AddPin( &part->ref_des, &pin_name, FALSE );
				}
			}
		}
		else if( sid.T1() == ID_TEXT && sid.T2() == ID_TEXT && sid.T3() == ID_SEL_TXT )
		{
			// add text string to group textlist
			CText * t = (CText*)m_sel_ptrs[i];
			g_tl->AddText( t->m_x, t->m_y, t->m_angle, t->m_mirror,  t->m_bNegative,
				t->m_layer, t->m_font_size, t->m_stroke_width, &t->m_str, FALSE );
		}
	}

	// mark all connections and areas as unchecked
	CIterator_cnet iter_net(m_Doc->m_nlist);
	cnet * net = iter_net.GetFirst();
	while( net )
	{
		for( int ic=0; ic<net->NumCons(); ic++ )
			net->ConByIndex(ic)->utility = FALSE;
		for( int ia=0; ia<net->NumAreas(); ia++ )
			net->area[ia].utility = FALSE;
		net = iter_net.GetNext();
	}

	// check all selected areas and connections
	g_nl->ClearTeeIDs();
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT )
		{
			// connection, only add if between parts in group
			cnet * net = (cnet*)m_sel_ptrs[i];
			cconnect * c = net->ConByIndex(sid.I2());
			if( c->utility == FALSE )
			{
				cnet * g_net = g_nl->GetNetPtrByName( &net->name );
				if( g_net == NULL )
				{
					g_net = g_nl->AddNet( net->name, net->NumPins(), net->def_w, net->def_via_w, net->def_via_hole_w );
				}
				// test start and end pins
				BOOL bStartPinInGroup = FALSE;
				BOOL bEndPinInGroup = FALSE;
				BOOL bStubTrace = FALSE;
				if( c->end_pin == cconnect::NO_END )
					bStubTrace = TRUE;
				cpin * pin1 = &net->pin[c->start_pin];
				cpart * part1 = pin1->part;
				cpin * pin2 = NULL;
				cpart * part2 = NULL;
				if( !bStubTrace )
				{
					pin2 = &net->pin[c->end_pin];
					part2 = pin2->part;
				}
				// loop through all group parts
				cpart * g_part = g_pl->GetFirstPart();
				while( g_part )
				{
					if( part1->ref_des == g_part->ref_des )
						bStartPinInGroup = TRUE;
					if( !bStubTrace )
						if( part2->ref_des == g_part->ref_des )
							bEndPinInGroup = TRUE;
					g_part = g_pl->GetNextPart( g_part );
				}
				if( bStartPinInGroup && (bEndPinInGroup || bStubTrace) )
				{
					// add connection to group net, and copy all segments and vertices
					int p1 = g_nl->GetNetPinIndex( g_net, &pin1->ref_des, &pin1->pin_name );
					int g_ic;
					if( !bStubTrace )
					{
						int p2 = g_nl->GetNetPinIndex( g_net, &pin2->ref_des, &pin2->pin_name );
						g_ic = g_net->AddConnectFromPinToPin( p1, p2 );
					}
					else
					{
						g_ic = g_net->AddConnectFromPin( p1 );
					}
					cconnect * g_c = g_net->ConByIndex(g_ic);
					g_c->SetNumSegs( c->NumSegs() );
					for( int is=0; is<c->NumSegs(); is++ )
					{
						g_c->SegByIndex(is) = c->SegByIndex(is);
						g_c->SegByIndex(is).m_dlist = NULL;
						g_c->SegByIndex(is).dl_el = NULL;
						g_c->SegByIndex(is).dl_sel = NULL;
						g_c->VtxByIndex(is) = c->VtxByIndex(is);	// this zeros graphics elements
						c->VtxByIndex(is) = g_c->VtxByIndex(is);	// this restores them
//						g_c->vtx[is].m_bDrawingEnabled = FALSE;
						g_nl->AddTeeID( g_c->VtxByIndex(is).tee_ID );
					}
//					g_c->vtx[c->NumSegs()].m_bDrawingEnabled = FALSE;
					g_nl->AddTeeID( g_c->VtxByIndex(c->NumSegs()).tee_ID );
					// remove any routed segments that are not in group
					for( int is=0; is<c->NumSegs(); is++ )
					{
						if( c->SegByIndex(is).layer != LAY_RAT_LINE )
						{
							// routed segment, is this in group ?
							id search_id = sid;
							search_id.SetT3( ID_SEL_SEG );
							search_id.SetI3( is );
							BOOL bInGroup = FALSE;
							for( int i=0; i<m_sel_ids.GetSize(); i++ )
							{
								id t_id = m_sel_ids[i];
								if( t_id == search_id )
								{
									// this segment is in group
									bInGroup = TRUE;
									break;
								}
							}
							if( !bInGroup )
							{
								// not in group, unroute it
								g_nl->UnrouteSegmentWithoutMerge( g_net, g_ic, is );
							}
						}
					}
					// remove any vias or tees that are not in group
					for( int iv=1; iv<c->NumSegs(); iv++ )
					{
						cvertex * v = &c->VtxByIndex(iv);
						if( v->tee_ID || v->via_w )
						{
							// is this in group?
							id search_id = sid;
							search_id.SetT3( ID_SEL_VERTEX );
							search_id.SetI3( iv );
							BOOL bInGroup = FALSE;
							for( int i=0; i<m_sel_ids.GetSize(); i++ )
							{
								id t_id = m_sel_ids[i];
								if( t_id == search_id )
								{
									// this vertex is in group
									bInGroup = TRUE;
									break;
								}
							}
							if( !bInGroup )
							{
								// delete the vertex from group
								cvertex * g_v = &g_c->VtxByIndex(iv);
								g_v->tee_ID = 0;
								g_v->force_via_flag = 0;
								g_v->via_w = 0;
								if( c->end_pin == cconnect::NO_END && iv == c->NumSegs() )
								{
									// last vertex of stub trace, just delete last segment
									//** TODO should actually remove segment, but this could change
									// the connection array
									g_nl->UnrouteSegmentWithoutMerge( g_net, g_ic, iv-1 );
								}
								else
								{
									// deleting vertex between two segments
									g_nl->UnrouteSegmentWithoutMerge( g_net, g_ic, iv-1 );
									g_nl->UnrouteSegmentWithoutMerge( g_net, g_ic, iv );
								}
								//**
							}
						}
					}
					// merge unrouted segments
					g_nl->MergeUnroutedSegments( g_net, g_ic );
				}
			}
			c->utility = TRUE;	// mark as checked
		}
		else if( sid.T1() == ID_NET && sid.T2() == ID_AREA
			&& sid.T3() == ID_SEL_SIDE )
		{
			// area side selected
			cnet * net = (cnet*)m_sel_ptrs[i];
			carea * a = &net->area[sid.I2()];
			CPolyLine * p = a;
			if( a->utility == 0 )
			{
				// first area side found, mark area as selected and
				// all other sides as unselected
				for( int is=0; is<p->NumSides(); is++ )
					p->SetUtility( is, 0 );
				a->utility = 1;
			}
			p->SetUtility( sid.I3(), 1 );	// mark this side as selected
		}
	}

	g_nl->CleanUpAllConnections();

	// now check areas, only copy areas if all sides are selected
	net = iter_net.GetFirst();
	while( net )
	{
		for( int ia=0; ia<net->NumAreas(); ia++ )
		{
			carea * a = &net->area[ia];
			if( a->utility )
			{
				BOOL bAllSides = TRUE;
				CPolyLine * p = a;
				for( int is=0; is<p->NumSides(); is++ )
				{
					if( p->Utility( is ) == 0 )
					{
						bAllSides = FALSE;
						break;
					}
				}
				if( bAllSides )
				{
					// add area to group
					cnet * g_net = g_nl->GetNetPtrByName( &net->name );
					if( g_net == NULL )
					{
						g_net = g_nl->AddNet( net->name, net->NumPins(), net->def_w, net->def_via_w, net->def_via_hole_w );
					}
					int g_ia = g_nl->AddArea( g_net, p->Layer(), p->X(0), p->Y(0),
						p->GetHatch() );
					CPolyLine * g_p = &g_net->area[g_ia];
					g_p->Copy( p );
					id g_id;
					g_id = g_p->Id();
					g_id.SetI2( g_ia );
					g_p->SetId( &g_id );
				}
			}
		}
		net = iter_net.GetNext();
	}

	// now remove any nets with zero pins, connections and areas
	CIterator_cnet iter_net_g(g_nl);
	net = iter_net_g.GetFirst();
	while( net )
	{
		if( net->NumPins() == 0 && net->NumCons() == 0 && net->NumAreas() == 0 )
			g_nl->RemoveNet( net );
		net = iter_net_g.GetNext();
	}

	// mark all sm_cutouts as unselected
	for( int ism=0; ism<m_Doc->m_sm_cutout.GetSize(); ism++ )
	{
		for( int is=0; is<m_Doc->m_sm_cutout[ism].NumSides(); is++ )
			m_Doc->m_sm_cutout[ism].SetUtility( is, 0 );
	}
	// find selected sides
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_MASK && sid.T2() == ID_MASK && sid.T3() == ID_SEL_SIDE )
		{
			m_Doc->m_sm_cutout[sid.I2()].SetUtility( sid.I3(), 1 );
		}
	}
	// copy to group
	for( int ism=0; ism<m_Doc->m_sm_cutout.GetSize(); ism++ )
	{
		CPolyLine * p = &m_Doc->m_sm_cutout[ism];
		BOOL bAllSides = TRUE;
		for( int is=0; is<p->NumSides(); is++ )
		{
			if( p->Utility( is ) == 0 )
			{
				bAllSides = FALSE;
				break;
			}
		}
		if( bAllSides )
		{
			// add to group
			int g_ism = g_sm->GetSize();
			g_sm->SetSize(g_ism+1);
			CPolyLine * g_p = &(*g_sm)[g_ism];
			g_p->Copy( p );
			id sid = g_p->Id();
			sid.SetI2( g_ism );
			g_p->SetId( &sid );
		}
	}

	// mark all board outlines as unselected
	for( int ibd=0; ibd<m_Doc->m_board_outline.GetSize(); ibd++ )
	{
		for( int is=0; is<m_Doc->m_board_outline[ibd].NumSides(); is++ )
			m_Doc->m_board_outline[ibd].SetUtility( is, 0 );
	}
	// find selected sides
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_BOARD && sid.T2() == ID_BOARD && sid.T3() == ID_SEL_SIDE )
		{
			m_Doc->m_board_outline[sid.I2()].SetUtility( sid.I3(), 1 );
		}
	}
	// copy to group
	for( int ibd=0; ibd<m_Doc->m_board_outline.GetSize(); ibd++ )
	{
		CPolyLine * p = &m_Doc->m_board_outline[ibd];
		BOOL bAllSides = TRUE;
		for( int is=0; is<p->NumSides(); is++ )
		{
			if( p->Utility( is ) == 0 )
			{
				bAllSides = FALSE;
				break;
			}
		}
		if( bAllSides )
		{
			// add to group
			int g_ibd = g_bd->GetSize();
			g_bd->SetSize(g_ibd+1);
			CPolyLine * g_p = &(*g_bd)[g_ibd];
			g_p->Copy( p );
			id sid = g_p->Id();
			sid.SetI2( g_ibd );
			g_p->SetId( &sid );
		}
	}

	// see if anything copied
	if( !iter_net_g.GetFirst() && !g_pl->GetFirstPart() && !g_sm->GetSize() 
		&& !g_bd->GetSize() && !g_tl->GetNumTexts() )
	{
		AfxMessageBox( "Nothing copied !\nRemember that traces must be connected\nto a part in the group to be copied" );
		CWnd* pMain = AfxGetMainWnd();
		if (pMain != NULL)
		{
			CMenu* pMenu = pMain->GetMenu();
			CMenu* submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
			submenu->EnableMenuItem( ID_EDIT_PASTE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
			pMain->DrawMenuBar();
		}
	}
	else
	{
		CWnd* pMain = AfxGetMainWnd();
		if (pMain != NULL)
		{
			CMenu* pMenu = pMain->GetMenu();
			CMenu* submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
			submenu->EnableMenuItem( ID_EDIT_PASTE, MF_BYCOMMAND | MF_ENABLED );
			pMain->DrawMenuBar();
		}
	}
}

// function to find all stub traces ending on tee and mark them for removal,
// then looks for any tees on that stub and recurses
//
void MarkStubsForRemoval( cnet * net, int tee_ID )
{
	CIterator_cconnect iter_con(net);
	for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
	{
		cvertex * end_v = &c->VtxByIndex(c->NumSegs());
		if( c->end_pin == cconnect::NO_END && end_v->tee_ID == tee_ID )
		{
			// if already marked for removal, ignore
			if( c->utility != 2 || net->utility != 1 )
			{
				// else flag this stub for removal, and search for other tees in stub
				c->utility = 2;
				net->utility = 1;
				for( int iv=1; iv<c->NumSegs(); iv++ )
				{
					cvertex * v = &c->VtxByIndex(iv);
					if( v->tee_ID )
					{
						MarkStubsForRemoval( net, v->tee_ID );
					}
				}
			}
		}
	}
}

void CFreePcbView::OnGroupCut()
{
	OnGroupCopy();
	OnGroupDelete();
}

// Remove all elements in group from project
//
void CFreePcbView::OnGroupDelete()
{
	DeleteGroup( &m_sel_ptrs, &m_sel_ids );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
}

void CFreePcbView::DeleteGroup( CArray<void*> * grp_ptr, CArray<id> * grp_id )
{
	CPartList * pl =  m_Doc->m_plist;
	CNetList * nl = m_Doc->m_nlist;
	cpart * part;
	cnet * net;

	// create undo descriptor before deletion
	undo_group_descriptor * undo = (undo_group_descriptor*)CreateGroupDescriptor( m_Doc->m_undo_list,
		grp_ptr, grp_id, UNDO_GROUP_DELETE );

	// mark all parts and nets as unmodified
	nl->MarkAllNets( 0 );
	for( part=pl->GetFirstPart(); part; part=pl->GetNextPart(part) )
		part->utility = 0;

	// loop through selected items and mark parts and nets that need to be saved
	// for undoing
	for( int i=0; i<grp_id->GetSize(); i++ )
	{
		id this_id = (*grp_id)[i];
		void * ptr = (*grp_ptr)[i];
		if( this_id.T1() == ID_PART && this_id.T2() == ID_SEL_RECT )
		{
			cpart * part = (cpart *) (*grp_ptr)[i];
			part->utility = 1;		// this part will be deleted
			if( part->shape )
			{
				for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
				{
					cnet * pin_net = part->pin[ip].net;
					if( pin_net )
						pin_net->utility = 1;	// this net will be modified
				}
			}
		}
		if( this_id.T1() == ID_NET && this_id.T2() == ID_CONNECT )
		{
			cnet * net = (cnet *) (*grp_ptr)[i];
			net->utility = 1;		// this net will be modified
		}
	}
	// save undo info
	m_Doc->m_undo_list->NewEvent();
	CIterator_cnet iter_net(nl);
	for( net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
		if( net->utility )
			SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
	for( part=pl->GetFirstPart(); part; part=pl->GetNextPart(part) )
		if( part->utility )
			SaveUndoInfoForPart( part,
			CPartList::UNDO_PART_DELETE, NULL, FALSE, m_Doc->m_undo_list );

	// mark all nets as unmodified (again)
	nl->MarkAllNets( 0 );
	// mark all sm_cutout sides as unselected
	for( int ism=0; ism<m_Doc->m_sm_cutout.GetSize(); ism++ )
		for( int is=0; is<m_Doc->m_sm_cutout[ism].NumSides(); is++ )
			m_Doc->m_sm_cutout[ism].SetUtility( is, 0 );
	// mark all board outline sides as unselected
	for( int ibd=0; ibd<m_Doc->m_board_outline.GetSize(); ibd++ )
		for( int is=0; is<m_Doc->m_board_outline[ibd].NumSides(); is++ )
			m_Doc->m_board_outline[ibd].SetUtility( is, 0 );

	// identify selected tees and mark dependent stub traces for removal
	for( int i=0; i<grp_id->GetSize(); i++ )
	{
		id this_id = (*grp_id)[i];
		void * ptr = (*grp_ptr)[i];
		if( this_id.T1() == ID_NET )
		{
			cnet * net = (cnet*)ptr;
			if( this_id.T2() == ID_CONNECT )
			{
				int ic = this_id.I2();
				cconnect * c = net->ConByIndex(ic);
				if( this_id.T3() == ID_SEL_VERTEX )
				{
					int iv = this_id.I3();
					cvertex * v = &c->VtxByIndex(iv);
					if( v->tee_ID && iv != c->NumSegs() )
					{
						// tee, not at end of stub trace
						MarkStubsForRemoval( net, v->tee_ID );
					}
				}
			}
		}
	}
	// unroute selected trace segments
	for( int i=0; i<(*grp_id).GetSize(); i++ )
	{
		id this_id = (*grp_id)[i];
		void * ptr = (*grp_ptr)[i];
		if( this_id.T1() == ID_NET )
		{
			cnet * net = (cnet*)ptr;
			if( this_id.T2() == ID_CONNECT )
			{
				// don't actually delete connections, just unroute
				int ic = this_id.I2();
				cconnect * c = net->ConByIndex(ic);
				if( this_id.T3() == ID_SEL_SEG )
				{
					// unroute segment
					int is = this_id.I3();
					nl->UnrouteSegmentWithoutMerge( net, ic, is );
					net->utility = 1;	// flag net as modified
					if( c->utility == 0 )
						c->utility = 1;		// flag connection as modified
				}
				else if( this_id.T3() == ID_SEL_VERTEX )
				{
					// unforce via
					int iv = this_id.I3();
					cvertex * v = &c->VtxByIndex(iv);
					if( v->force_via_flag )
					{
						v->force_via_flag = 0;
						net->utility = 1;
						if( c->utility == 0 )
							c->utility = 1;		// flag connection as modified
					}
				}
				else
					ASSERT(0);
			}
		}
	}

	// find non-branch stub traces with no end via and remove trailing unrouted segments
	net = iter_net.GetFirst();
	while( net )
	{
		if( net->utility )
		{
			for( int ic=net->NumCons()-1; ic>=0; ic-- )
			{
				cconnect * c = net->ConByIndex(ic);
				if( c->utility == 1 )
				{
					cvertex * end_v = &c->VtxByIndex(c->NumSegs());
					if( c->end_pin == cconnect::NO_END )
					{
						int is=c->NumSegs()-1;
						cseg * s = &c->SegByIndex(is);
						cvertex * next_v = &c->VtxByIndex(is+1);
						if( s->layer == LAY_RAT_LINE && next_v->force_via_flag == 0 && next_v->tee_ID == 0 )
						{
							if( c->NumSegs() == 1 )
								c->utility = 2;		// mark connection for deletion
							else
								nl->RemoveSegment( net, ic, is, FALSE, FALSE );	// remove segment
						}
					}
				}
			}
		}
		net = iter_net.GetNext();
	}

	// remove connections marked for deletion and merge unrouted segments
	net = iter_net.GetFirst();
	while( net )
	{
		if( net->utility )
		{
			for( int ic=net->NumCons()-1; ic>=0; ic-- )
			{
				cconnect * c = net->ConByIndex(ic);
				if( c->utility == 2 )
					nl->RemoveNetConnect( net, ic, FALSE );
				else if( c->utility == 1 )
					nl->MergeUnroutedSegments( net, ic );
			}
		}
		net = iter_net.GetNext();
	}
	// remove board outlines, copper areas, parts, texts and sm_cutouts
	for( int i=0; i<(*grp_id).GetSize(); i++ )
	{
		id this_id = (*grp_id)[i];
		if( this_id.T1() == ID_NET && this_id.T2() == ID_AREA && this_id.T3() == ID_SEL_SIDE )
		{
			cnet * net = (cnet*)(*grp_ptr)[i];
			carea * a = &net->area[this_id.I2()];
			a->SetUtility( this_id.I3(), 1 );	// mark for deletion
			a->utility = 1;
			net->utility = 1;						// mark as modified
		}
		if( this_id.T1() == ID_PART && this_id.T2() == ID_SEL_RECT )
		{
			cpart * part = (cpart *) (*grp_ptr)[i];
			nl->PartDeleted( part, FALSE );
			pl->Remove( part );
		}
		else if( this_id.T1() == ID_TEXT )
		{
			CText * text = (CText *)(*grp_ptr)[i];
			SaveUndoInfoForText( text, CTextList::UNDO_TEXT_DELETE, FALSE, m_Doc->m_undo_list );
			m_Doc->m_tlist->RemoveText( text );
		}
		else if( this_id.T1() == ID_MASK && this_id.T3() == ID_SEL_SIDE )
			m_Doc->m_sm_cutout[this_id.I2()].SetUtility( this_id.I3(), 1 );	// mark for deletion
		else if( this_id.T1() == ID_BOARD && this_id.T3() == ID_SEL_SIDE )
			m_Doc->m_board_outline[this_id.I2()].SetUtility( this_id.I3(), 1 );	// mark for deletion
	}
	// delete sm_cutouts and renumber them
	BOOL bUndoSaved = FALSE;
	for( int ism=m_Doc->m_sm_cutout.GetSize()-1; ism>=0; ism-- )
	{
		BOOL bDelete = TRUE;
		for( int is=0; is<m_Doc->m_sm_cutout[ism].NumSides(); is++ )
			if( m_Doc->m_sm_cutout[ism].Utility( is ) == 0 )
				bDelete = FALSE;
		if( bDelete )
		{
			if( !bUndoSaved )
				SaveUndoInfoForSMCutouts( FALSE, m_Doc->m_undo_list );
			m_Doc->m_sm_cutout.RemoveAt( ism );
			bUndoSaved = TRUE;
		}
	}
	for( int ism=0; ism<m_Doc->m_sm_cutout.GetSize(); ism++ )
	{
			id new_id = m_Doc->m_sm_cutout[ism].Id();
			new_id.SetI2( ism );
			m_Doc->m_sm_cutout[ism].SetId( &new_id );
	}
	// delete board outlines and renumber them
	bUndoSaved = FALSE;
	for( int ibd=m_Doc->m_board_outline.GetSize()-1; ibd>=0; ibd-- )
	{
		BOOL bDelete = TRUE;
		for( int is=0; is<m_Doc->m_board_outline[ibd].NumSides(); is++ )
			if( m_Doc->m_board_outline[ibd].Utility( is ) == 0 )
				bDelete = FALSE;
		if( bDelete )
		{
			if( !bUndoSaved )
				SaveUndoInfoForBoardOutlines( FALSE, m_Doc->m_undo_list );
			m_Doc->m_board_outline.RemoveAt( ibd );
			bUndoSaved = TRUE;
		}
	}
	for( int ibd=0; ibd<m_Doc->m_board_outline.GetSize(); ibd++ )
	{
			id new_id = m_Doc->m_board_outline[ibd].Id();
			new_id.SetI2( ibd );
			m_Doc->m_board_outline[ibd].SetId( &new_id );
	}
	// delete copper areas or cutouts if all sides are in group
	net = iter_net.GetFirst();
	while( net )
	{
		for( int ia=net->NumAreas()-1; ia>=0; ia-- )
		{
			carea * a = &net->area[ia];
			if( a->utility )
			{
				// see if entire area can be deleted
				BOOL bDelete = TRUE;
				for( int is=0; is<a->ContourEnd(0); is++ )
					if( a->Utility( is ) == 0 )
						bDelete = FALSE;
				if( bDelete )
				{
					SaveUndoInfoForArea( net, ia, CNetList::UNDO_AREA_DELETE, FALSE, m_Doc->m_undo_list );
					nl->RemoveArea( net, ia );
				}
				else if( a->NumContours() > 1 )
				{
					// see if cutouts can be deleted
					BOOL bCutoutsDeleted = FALSE;
					for( int icont=a->NumContours()-1; icont>0; icont-- )
					{
						int istart = a->ContourStart( icont );
						int iend = a->ContourEnd( icont );
						bDelete = TRUE;
						for( int is=istart; is<=iend; is++ )
							if( a->Utility( is ) == 0 )
								bDelete = FALSE;
						if( bDelete )
						{
							a->RemoveContour( icont );
							bCutoutsDeleted = TRUE;
						}
					}
					if( bCutoutsDeleted )
						SaveUndoInfoForArea( net, ia, CNetList::UNDO_AREA_MODIFY, FALSE, m_Doc->m_undo_list );
				}
			}
		}
		nl->SetAreaConnections( net );
		net = iter_net.GetNext();
	}
	// clean up
	m_Doc->m_undo_list->Push( UNDO_GROUP, (void*)undo, &UndoGroupCallback );
}

void CFreePcbView::OnGroupPaste()
{
	void * vp;
	// pointers to group lists
	CPartList * g_pl = m_Doc->clip_plist;
	CTextList * g_tl = m_Doc->clip_tlist;
	CNetList * g_nl = new CNetList( NULL, g_pl );	// make copy to modify
	g_nl->Copy( m_Doc->clip_nlist );
	g_nl->MarkAllNets( 0 );
	CArray<CPolyLine> * g_sm = &m_Doc->clip_sm_cutout;
	CArray<CPolyLine> * g_bd = &m_Doc->clip_board_outline;
	// pointers to project lists
	CPartList * pl = m_Doc->m_plist;
	CNetList * nl = m_Doc->m_nlist;
	CTextList * tl = m_Doc->m_tlist;
	CArray<CPolyLine> * sm = &m_Doc->m_sm_cutout;
	CArray<CPolyLine> * bd = &m_Doc->m_board_outline;

	// get paste options
	CDlgGroupPaste dlg;
	dlg.Initialize( g_nl );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// start pasting
		CancelSelection();
		SetCursorMode( CUR_GROUP_SELECTED );
		m_sel_id.SetT1(ID_MULTI);
		m_Doc->m_undo_list->NewEvent();
		nl->MarkAllNets( 0 );	// mark all nets as unsaved
		BOOL bDragGroup = !dlg.m_position_option;
		double min_d = (double)INT_MAX*(double)INT_MAX;
		int min_x = INT_MAX;	// lowest-left point for dragging group
		int min_y = INT_MAX;

		// make a map of all reference designators in project, including
		// refs in the netlist that don't exist in the partlist
		CMapStringToPtr ref_des_map;
		cpart * part = pl->GetFirstPart();
		while( part )
		{
			ref_des_map.SetAt( part->ref_des, NULL );
			part = pl->GetNextPart( part );
		}
		CIterator_cnet iter_net(nl);
		cnet * net = iter_net.GetFirst();
		while( net )
		{
			for( int ip=0; ip<net->NumPins(); ip++ )
			{
				cpin * p = &net->pin[ip];
				if( !ref_des_map.Lookup( p->ref_des, vp ) )
				{
					ref_des_map.SetAt( p->ref_des, NULL );
				}
			}
			net = iter_net.GetNext();
		}

		// add parts from group, renaming if necessary
		cpart * g_part = g_pl->GetFirstPart();
		while( g_part )
		{
			CString conflicted_ref;
			CString g_prefix;
			int g_num = ParseRef( &g_part->ref_des, &g_prefix );
			BOOL bConflict = FALSE;
			// make new ref
			CString new_ref = g_part->ref_des;
			if( dlg.m_ref_option == 2 )
			{
				// add offset to ref
				new_ref.Format( "%s%d", g_prefix, g_num + dlg.m_ref_offset );
			}
			if( dlg.m_ref_option != 1 && ref_des_map.Lookup( new_ref, vp ) )
			{
				// new ref conflicts with existing ref in project
				conflicted_ref = new_ref;
				bConflict = TRUE;
			}
			if( dlg.m_ref_option == 1 || bConflict )
			{
				// use next available ref
				int max_num = 0;
				POSITION pos;
				CString key;
				void * ptr;
				for( pos = ref_des_map.GetStartPosition(); pos != NULL; )
				{
					ref_des_map.GetNextAssoc( pos, key, ptr );
					CString prefix;
					int i = ParseRef( &key, &prefix );
					if( prefix == g_prefix && i > max_num )
						max_num = i;
				}
				new_ref.Format( "%s%d", g_prefix, max_num+1 );
			}
			if( bConflict )
			{
				// ref in group conflicts with ref in project
				CString mess = "Part \"";
				mess += conflicted_ref;
				mess += "\" already exists in project.\nIt will be renamed \" ";
				mess += new_ref;
				mess += " \"";
				AfxMessageBox( mess );
				bConflict = TRUE;
			}
			// now change part refs in group netlist
			CIterator_cnet iter_net_g(g_nl);
			net = iter_net_g.GetFirst();
			while( net )
			{
				for( int ip=0; ip<net->NumPins(); ip++ )
				{
					cpin * pin = &net->pin[ip];
					if( pin->utility == 0 && pin->ref_des == g_part->ref_des )
					{
						pin->ref_des = new_ref;
						pin->part = NULL;
						pin->utility = 1;	// only change it once
					}
				}
				net = iter_net_g.GetNext();
			}
			// add new part
			cpart * prj_part = pl->Add( g_part->shape, &new_ref, &g_part->package,
				g_part->x + dlg.m_dx, g_part->y + dlg.m_dy,
				g_part->side, g_part->angle, 1, 0 );
			ref_des_map.SetAt( new_ref, NULL );
			SaveUndoInfoForPart( prj_part,
				CPartList::UNDO_PART_ADD, &prj_part->ref_des, FALSE, m_Doc->m_undo_list );
			pl->UndrawPart( prj_part );

			// set ref text parameters
			prj_part->m_ref_angle = g_part->m_ref_angle;
			prj_part->m_ref_size = g_part->m_ref_size;
			prj_part->m_ref_w = g_part->m_ref_w;
			prj_part->m_ref_xi = g_part->m_ref_xi;
			prj_part->m_ref_yi = g_part->m_ref_yi;
			prj_part->m_ref_vis = g_part->m_ref_vis;
			prj_part->m_ref_layer = g_part->m_ref_layer;
			// set value parameters
			prj_part->value = g_part->value;
			prj_part->m_value_angle = g_part->m_value_angle;
			prj_part->m_value_size = g_part->m_value_size;
			prj_part->m_value_w = g_part->m_value_w;
			prj_part->m_value_xi = g_part->m_value_xi;
			prj_part->m_value_yi = g_part->m_value_yi;
			prj_part->m_value_vis = g_part->m_value_vis;
			prj_part->m_value_layer = g_part->m_value_layer;
			pl->DrawPart( prj_part );
			// find closest part to lower left corner
			double d = prj_part->x + prj_part->y;
			if( d < min_d )
			{
				min_d = d;
				min_x = prj_part->x;
				min_y = prj_part->y;
			}
			// add pointer and id to group selector array
			m_sel_ptrs.Add( prj_part );
			INT_PTR i = m_sel_ids.Add( prj_part->m_id );
			m_sel_ids[i].SetT2( ID_SEL_RECT );
			// end of loop, get next group part
			g_part = g_pl->GetNextPart( g_part );
		}

		// add nets from group
		// rename net if necessary
		CString g_suffix;
		if( dlg.m_net_rename_option == 0 )
		{
			// get highest group suffix already in project
			int max_g_num = 0;
			cnet * net = iter_net.GetFirst();
			while( net )
			{
				int n = net->name.ReverseFind( '_' );
				if( n > 0 )
				{
					CString prefix;
					CString test_suffix = net->name.Right( net->name.GetLength() - n - 1 );
					int g_num = ParseRef( &test_suffix, &prefix );
					if( prefix == "$G" )
						max_g_num = max( g_num, max_g_num );
				}
				net = iter_net.GetNext();
			}
			g_suffix.Format( "_$G%d", max_g_num + 1 );
		}
		// now loop through all nets in group and add or merge with project
		cnet * prj_net = NULL;	// project net
		CIterator_cnet iter_net_g(g_nl);
		cnet * g_net = iter_net_g.GetFirst();	// group net
		while( g_net )
		{
			// see if there are routed segments in this net
			BOOL bRouted = FALSE;
			if( dlg.m_pin_net_option == 1 )
			{
				for( int ic=0; ic<g_net->NumCons(); ic++ )
				{
					cconnect * c = g_net->ConByIndex(ic);
					for( int is=0; is<c->NumSegs(); is++ )
					{
						if( c->SegByIndex(is).width > 0 )
						{
							bRouted = TRUE;
							break;
						}
					}
					if( bRouted )
						break;
				}
			}
			// only add if there are areas, or routed segments if requested
			if( (dlg.m_pin_net_option == 0 || bRouted) || g_net->NumAreas() > 0 )
			{
				// OK, add this net to project
				// utility flag is set in the Group Paste dialog for nets which
				// should be merged (i.e. not renamed)
				if( dlg.m_net_name_option == 1 && g_net->utility == 0 )
				{
					// rename net
					CString new_name;
					if( dlg.m_net_rename_option == 1 )
					{
						// get next "Nnnnnn" net name
						cnet * net = iter_net.GetFirst();
						int max_num = 0;
						CString prefix;
						while( net )
						{
							int num = ParseRef( &net->name, &prefix );
							if( prefix == "N" && num > max_num )
								max_num = num;
							net = iter_net.GetNext();
						}
						new_name.Format( "N%05d", max_num+1 );
					}
					else
					{
						// add group suffix
						new_name = g_net->name + g_suffix;
					}
					// add new net
					prj_net = nl->AddNet( new_name, g_net->NumPins(),
						g_net->def_w, g_net->def_via_w, g_net->def_via_hole_w );
					SaveUndoInfoForNet( prj_net, CNetList::UNDO_NET_ADD, FALSE, m_Doc->m_undo_list );
					prj_net->utility = 1;	// mark as saved
				}
				else
				{
					// merge group net with project net of same name
					prj_net = nl->GetNetPtrByName( &g_net->name );
					if( !prj_net )
					{
						// no project net with the same name
						prj_net = nl->AddNet( g_net->name, g_net->NumPins(),
							g_net->def_w, g_net->def_via_w, g_net->def_via_hole_w );
						SaveUndoInfoForNet( prj_net, CNetList::UNDO_NET_ADD, FALSE, m_Doc->m_undo_list );
						prj_net->utility = 1;	// mark as saved
					}
					else if( prj_net->utility == 0 )
					{
						SaveUndoInfoForNetAndConnectionsAndAreas( prj_net, FALSE, m_Doc->m_undo_list );
						prj_net->utility = 1;	// mark as saved
					}
				}
				if( !prj_net )
					ASSERT(0);
				// now create map for renaming tees
				CMap<int,int,int,int> tee_map;
				// connect group part pins to project net
				for( int ip=0; ip<g_net->NumPins(); ip++ )
				{
					cpin * pin = &g_net->pin[ip];
					BOOL bAdd = TRUE;
					if( dlg.m_pin_net_option == 1 )
					{
						// only add pin if connected to a routed trace
						bAdd = FALSE;
						for( int ic=0; ic<g_net->NumCons(); ic++ )
						{
							cconnect * c = g_net->ConByIndex(ic);
							if( c->start_pin == ip || c->end_pin == ip )
							{
								for( int is=0; is<c->NumSegs(); is++ )
								{
									if( c->SegByIndex(is).width > 0 )
									{
										bAdd = TRUE;
										break;
									}
								}
							}
							if( bAdd )
								break;
						}
					}
					if( bAdd )
						prj_net->AddPin( &pin->ref_des, &pin->pin_name, FALSE );
				}
				// create new traces
				for( int g_ic=0; g_ic<g_net->NumCons(); g_ic++ )
				{
					cconnect * g_c = g_net->ConByIndex(g_ic);
					// get start pin of connection in new net
					CString g_start_ref_des = g_net->pin[g_c->start_pin].ref_des;
					CString g_start_pin_name = g_net->pin[g_c->start_pin].pin_name;
					int new_start_pin = nl->GetNetPinIndex( prj_net, &g_start_ref_des, &g_start_pin_name );
					// get end pin of connection in new net
					CString g_end_ref_des;
					CString g_end_pin_name;
					int new_end_pin = cconnect::NO_END;
					if( g_c->end_pin != cconnect::NO_END )
					{
						g_end_ref_des = g_net->pin[g_c->end_pin].ref_des;
						g_end_pin_name = g_net->pin[g_c->end_pin].pin_name;
						new_end_pin = nl->GetNetPinIndex( prj_net, &g_end_ref_des, &g_end_pin_name );
					}
					if( new_start_pin != -1 && (new_end_pin != -1 || g_c->end_pin == cconnect::NO_END) )
					{
						// add connection to new net
						int ic;
						if( new_end_pin != cconnect::NO_END )
						{
							ic = prj_net->AddConnectFromPinToPin( new_start_pin, new_end_pin );
						}
						else
						{
							ic = prj_net->AddConnectFromPin( new_start_pin );
						}
						// copy it and draw it
						if( ic < 0 )
							ASSERT(0);
						else
						{
							// copy connection
							cconnect * c = prj_net->ConByIndex(ic);
							c->Undraw();
							c->SetNumSegs( g_c->NumSegs() );
							for( int is=0; is<c->NumSegs(); is++ )
							{
								cseg * s = &c->SegByIndex(is);
								cvertex * v = &c->VtxByIndex(is);
								*s = g_c->SegByIndex(is);
								s->m_dlist = m_dlist;
								s->dl_el = NULL;
								s->dl_sel = NULL;
								*v = g_c->VtxByIndex(is);
								v->dl_sel = NULL;
								v->dl_hole = NULL;
								v->dl_el.SetSize(0);
								id seg_id( ID_NET, prj_net->UID(), ID_CONNECT, c->UID(), ic, 
									ID_SEL_SEG, s->UID(), is );
								m_sel_ptrs.Add( prj_net );
								m_sel_ids.Add( seg_id );
								id vtx_id( ID_NET, net->UID(), ID_CONNECT, c->UID(), ic, 
									ID_SEL_VERTEX, v->UID(), is );
								m_sel_ptrs.Add( prj_net );
								m_sel_ids.Add( vtx_id );
							}
							cvertex * v = &c->VtxByIndex(c->NumSegs());
							*v = g_c->VtxByIndex(g_c->NumSegs());
							v->dl_sel = NULL;
							v->dl_hole = NULL;
							v->dl_el.SetSize(0);
							if( c->end_pin != cconnect::NO_END )
							{
								id vtx_id( ID_NET, net->UID(), ID_CONNECT, c->UID(), ic,
									ID_SEL_VERTEX, v->UID(), c->NumSegs() );
								m_sel_ptrs.Add( prj_net );
								m_sel_ids.Add( vtx_id );
							}
							for( int iv=0; iv<c->NumSegs()+1; iv++ )
							{
								c->VtxByIndex(iv).x += dlg.m_dx;
								c->VtxByIndex(iv).y += dlg.m_dy;
								if( int g_id = c->VtxByIndex(iv).tee_ID )
								{
									// assign new tee_ID
									int new_id;
									BOOL bFound = tee_map.Lookup( g_id, new_id );
									if( !bFound )
									{
										new_id = nl->GetNewTeeID();
										tee_map.SetAt( g_id, new_id );
									}
									c->VtxByIndex(iv).tee_ID = new_id;
								}
								// update lower-left corner
								double d = c->VtxByIndex(iv).x + c->VtxByIndex(iv).y;
								if( d < min_d )
								{
									min_d = d;
									min_x = c->VtxByIndex(iv).x;
									min_y = c->VtxByIndex(iv).y;
								}
							}
							c->Draw();
						}
					}
				}
				// add copper areas
				for( int g_ia=0; g_ia<g_net->NumAreas(); g_ia++ )
				{
					carea * ga = &g_net->area[g_ia];
					CPolyLine * gp = ga;
					int ia = nl->AddArea( prj_net, gp->Layer(),
						gp->X(0), gp->Y(0), gp->GetHatch() );
					CPolyLine * p = &prj_net->area[ia];
					id p_id = p->Id();
					p->Copy( gp );
					p->SetId( &p_id );
					p->SetPtr( prj_net );
					for( int is=0; is<p->NumSides(); is++ )
					{
						int x = p->X(is);
						int y = p->Y(is);
						p->SetX( is, x + dlg.m_dx );
						p->SetY( is, y + dlg.m_dy );
						p_id.SetI2( ia );
						p_id.SetT3( ID_SEL_SIDE );
						p_id.SetI3( is );
						m_sel_ids.Add( p_id );
						m_sel_ptrs.Add( prj_net );
						// update lower-left corner
						double d = x + y;
						if( d < min_d )
						{
							min_d = d;
							min_x = x;
							min_y = y;
						}
					}
					p->Draw( prj_net->m_dlist );
				}
			}
			g_net = iter_net_g.GetNext();
		}
		// now destroy modified g_nl and restore links in g_pl
		delete g_nl;

		// add sm_cutouts
		int grp_size = g_sm->GetSize();
		int old_size = sm->GetSize();
		if( grp_size > 0 )
		{
			SaveUndoInfoForSMCutouts( FALSE, m_Doc->m_undo_list );
			sm->SetSize( old_size + grp_size );
			for( int g_ism=0; g_ism<grp_size; g_ism++ )
			{
				int ism = g_ism + old_size;
				CPolyLine * g_p = &(*g_sm)[g_ism];
				CPolyLine * p = &(*sm)[ism];
				p->Copy( g_p );
				id p_id = p->Id();
				p_id.SetI2( ism );
				p->SetId( &p_id );
				for( int is=0; is<p->NumSides(); is++ )
				{
					int x = p->X(is);
					int y = p->Y(is);
					p->SetX( is, x + dlg.m_dx );
					p->SetY( is, y + dlg.m_dy );
					p_id.SetI2( ism );
					p_id.SetT3( ID_SEL_SIDE );
					p_id.SetI3( is );
					m_sel_ids.Add( p_id );
					m_sel_ptrs.Add( NULL );
					// update lower-left corner
					double d = x + y;
					if( d < min_d )
					{
						min_d = d;
						min_x = x;
						min_y = y;
					}
				}
				p->Draw( m_dlist );
			}
		}

		// add board outlines
		grp_size = g_bd->GetSize();
		old_size = bd->GetSize();
		if( grp_size > 0 )
		{
			SaveUndoInfoForBoardOutlines( FALSE, m_Doc->m_undo_list );
			bd->SetSize( old_size + grp_size );
			for( int g_ibd=0; g_ibd<grp_size; g_ibd++ )
			{
				int ibd = g_ibd + old_size;
				CPolyLine * g_p = &(*g_bd)[g_ibd];	// group poly
				CPolyLine * p = &(*bd)[ibd];		// project poly
				p->Copy( g_p );
				id p_id = p->Id();
				p_id.SetI2( ibd );
				p->SetId( &p_id );
				for( int is=0; is<p->NumSides(); is++ )
				{
					int x = p->X(is);
					int y = p->Y(is);
					p->SetX( is, x + dlg.m_dx );
					p->SetY( is, y + dlg.m_dy );
					p_id.SetT3( ID_SEL_SIDE );
					p_id.SetI3( is );
					m_sel_ids.Add( p_id );
					m_sel_ptrs.Add( NULL );
					// update lower-left corner
					double d = x + y;
					if( d < min_d )
					{
						min_d = d;
						min_x = x;
						min_y = y;
					}
				}
				p->Draw( m_dlist );
			}
		}

		// add text
		CIterator_CText iter_t( g_tl );
		for( CText * t = iter_t.GetFirst(); t != NULL; t = iter_t.GetNext() )		
		{
			CText * new_text = m_Doc->m_tlist->AddText( t->m_x+dlg.m_dx, t->m_y+dlg.m_dy, t->m_angle,
				t->m_mirror, t->m_bNegative, t->m_layer, t->m_font_size, t->m_stroke_width,
				&t->m_str, TRUE );
			SaveUndoInfoForText( new_text, CTextList::UNDO_TEXT_ADD, FALSE, m_Doc->m_undo_list );
			id t_id( ID_TEXT, -1, ID_TEXT, -1, -1, ID_SEL_TXT );
			m_sel_ids.Add( t_id );
			m_sel_ptrs.Add( new_text );
			CRect text_bounds;
			m_Doc->m_tlist->GetTextRectOnPCB( new_text, &text_bounds );
			double d = text_bounds.left + text_bounds.bottom;
			if( d < min_d )
			{
				min_d = d;
				min_x = text_bounds.left;
				min_y = text_bounds.bottom;
			}
		}

		HighlightGroup();
		if( bDragGroup )
		{
			if( min_x == INT_MAX || min_y == INT_MAX )
				AfxMessageBox( "No items to drag" );
			else
				StartDraggingGroup( TRUE, min_x, min_y );
		}
		else
		{
			FindGroupCenter();
			if( m_Doc->m_vis[LAY_RAT_LINE] )
			{
				for( net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
				{
					m_Doc->m_nlist->OptimizeConnections( net, -1, m_Doc->m_auto_ratline_disable,
						m_Doc->m_auto_ratline_min_pins, TRUE ); 
				}
			}
		}
		m_Doc->ProjectModified( TRUE );
	}
}

void CFreePcbView::OnGroupSaveToFile()
{
	// Copy group to pseudo-clipboard
	OnGroupCopy();

	// force old-style file dialog by setting size of OPENFILENAME struct
	CFileDialog dlg( 0, "fpc", NULL, 0,
		"PCB files (*.fpc)|*.fpc|All Files (*.*)|*.*||",
		NULL, OPENFILENAME_SIZE_VERSION_400 );
	// get folder of most-recent file or project folder
	CString MRFile = theApp.GetMRUFile();
	CString MRFolder;
	if( MRFile != "" )
	{
		MRFolder = MRFile.Left( MRFile.ReverseFind( '\\' ) ) + "\\";
		dlg.m_ofn.lpstrInitialDir = MRFolder;
	}
	else
		dlg.m_ofn.lpstrInitialDir = m_Doc->m_parent_folder;
	int err = dlg.DoModal();
	if( err == IDOK )
	{
		CString pathname = dlg.GetPathName();
		// write project file
		CStdioFile pcb_file;
		int err = pcb_file.Open( pathname, CFile::modeCreate | CFile::modeWrite, NULL );
		if( !err )
		{
			// error opening partlist file
			CString mess;
			mess.Format( "Unable to open file %s", pathname );
			AfxMessageBox( mess );
		}
		else
		{
			// write clipboard to file
			try
			{
				// make map of all footprints used by group
				CMapStringToPtr clip_cache_map;
				cpart * part = m_Doc->clip_plist->GetFirstPart();
				while( part )
				{
					void * vp;
					if( part->shape )
						if( !clip_cache_map.Lookup( part->shape->m_name, vp ) )
							clip_cache_map.SetAt( part->shape->m_name, part->shape );
					part = m_Doc->clip_plist->GetNextPart( part );
				}
				m_Doc->WriteOptions( &pcb_file );
				m_Doc->WriteFootprints( &pcb_file, &clip_cache_map );
				m_Doc->WriteBoardOutline( &pcb_file, &m_Doc->clip_board_outline );
				m_Doc->WriteSolderMaskCutouts( &pcb_file, &m_Doc->clip_sm_cutout );
				m_Doc->clip_plist->WriteParts( &pcb_file );
				m_Doc->clip_nlist->WriteNets( &pcb_file );
				m_Doc->clip_tlist->WriteTexts( &pcb_file );
				pcb_file.WriteString( "[end]\n" );
				pcb_file.Close();
			}
			catch( CString * err_str )
			{
				// error
				AfxMessageBox( *err_str );
				delete err_str;
				CDC * pDC = GetDC();
				OnDraw( pDC );
				ReleaseDC( pDC );
				return;
			}
		}
	}
}

void CFreePcbView::OnEditCopy()
{
	if( !m_Doc->m_project_open )
		return;
	if( m_cursor_mode == CUR_GROUP_SELECTED )
		OnGroupCopy();
}

void CFreePcbView::OnEditPaste()
{
	if( !m_Doc->m_project_open )
		return;
	OnGroupPaste();
}


void CFreePcbView::OnEditCut()
{
	if( !m_Doc->m_project_open )
		return;
	if( m_cursor_mode == CUR_GROUP_SELECTED )
	{
		OnGroupCopy();
		OnGroupDelete();
	}
}

void CFreePcbView::RotateGroup()
{
	UngluePartsInGroup();

	// mark all parts and nets as unselected
	m_Doc->m_nlist->MarkAllNets(0);
	m_Doc->m_plist->MarkAllParts(0);

	// mark all corners of solder mask cutouts as unmoved
	for( int im=0; im<m_Doc->m_sm_cutout.GetSize(); im++ )
	{
		CPolyLine * poly = &m_Doc->m_sm_cutout[im];
		poly->SetUtility(0);
		for( int ic=0; ic<poly->NumCorners(); ic++ )
			poly->SetUtility( ic, 0 );	// unmoved
	}

	// mark all corners of board outlines as unmoved
	for( int im=0; im<m_Doc->m_board_outline.GetSize(); im++ )
	{
		CPolyLine * poly = &m_Doc->m_board_outline[im];
		poly->SetUtility(0);
		for( int ic=0; ic<poly->NumCorners(); ic++ )
			poly->SetUtility( ic, 0 );	// unmoved
	}

	// mark selected nets, mark and undraw areas, mask cutouts and board outlines
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			net->utility = TRUE;	// mark as selected
			if( sid.T2() == ID_AREA )
			{
				carea * a = &net->area[sid.I2()];
				a->utility = TRUE;
				CPolyLine * poly = a;
				poly->SetUtility(1);
				poly->Undraw();
			}
		}
		else if( sid.T1() == ID_MASK && sid.T2() == ID_MASK && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.I2()];
			poly->SetUtility(1);
			poly->Undraw();
		}
		else if( sid.T1() == ID_BOARD && sid.T2() == ID_OUTLINE && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[sid.I2()];
			poly->SetUtility(1);
			poly->Undraw();
		}
	}

	int tempx;
	// mark all relevant parts, nets, connections and segments as selected
	// and move text and copper area corners
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT
			&& sid.T3() == ID_SEL_SEG )
		{
			// segment
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.I2();
			int is = sid.I3();
			cconnect * c = net->ConByIndex(ic);	// this connection
			cseg * s = &c->SegByIndex(is);				// this segment
			cvertex * pre_v = &s->GetPreVtx();
			cvertex * post_v = &s->GetPostVtx();
			c->utility = TRUE;					// mark connection selected
			s->utility = TRUE;					// mark segment selected
			pre_v->utility =  TRUE;				// mark adjacent vertices as selected
			post_v->utility =  TRUE;
		}
		else if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT
			&& sid.T3() == ID_SEL_VERTEX )
		{
			// vertex
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.I2();
			int iv = sid.I3();
			cconnect * c = net->ConByIndex(ic);	// this connection
			cvertex * v = &c->VtxByIndex(iv);
			c->utility = TRUE;					// mark connection selected
			v->utility = TRUE;					// mark vertex selected
		}
		else if( sid.T1() == ID_PART && sid.T2() == ID_SEL_RECT )
		{
			// part
			cpart * part = (cpart*)m_sel_ptrs[i];
			part->utility = TRUE;	// mark part selected
		}
		else if( sid.T1() == ID_TEXT && sid.T2() == ID_TEXT && sid.T3() == ID_SEL_TXT )
		{
			// text
			CText * t = (CText*)m_sel_ptrs[i];
			m_Doc->m_tlist->MoveText( t, groupAverageX + t->m_y - groupAverageY,
				groupAverageY - t->m_x + groupAverageX, (t->m_angle+90)%360,
				t->m_mirror, t->m_bNegative, t->m_layer );
		}
		else if( sid.T1() == ID_NET && sid.T2() == ID_AREA && sid.T3() == ID_SEL_SIDE )
		{
			// area side
			cnet * net = (cnet*)m_sel_ptrs[i];
			CPolyLine * poly = &net->area[sid.I2()];
			int icontour = poly->Contour(sid.I3());
			int istart = poly->ContourStart(icontour);
			int iend = poly->ContourEnd(icontour);
			int ic1 = sid.I3();
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			if( !poly->Utility(ic1) )
			{
				// unmoved, move it
				tempx=poly->X(ic1);
				poly->SetX( ic1, groupAverageX + poly->Y(ic1)- groupAverageY );
				poly->SetY( ic1, groupAverageY -tempx + groupAverageX );
				poly->SetUtility(ic1,1);
			}
			if( !poly->Utility(ic2) )
			{
				// unmoved, move it
				tempx=poly->X(ic2);
				poly->SetX( ic2, groupAverageX + poly->Y(ic2)- groupAverageY );
				poly->SetY( ic2, groupAverageY - tempx + groupAverageX );
				poly->SetUtility(ic2,1);
			}
		}
		// sm_cutout side
		else if( sid.T1() == ID_MASK && sid.T2() == ID_MASK && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.I2()];
			int icontour = poly->Contour(0);
			int istart = poly->ContourStart(icontour);
			int iend = poly->ContourEnd(icontour);
			int ic1 = sid.I3();
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			if( !poly->Utility(ic1) )
			{
				// unmoved, move it
				tempx=poly->X(ic1);
				poly->SetX( ic1, groupAverageX + poly->Y(ic1)- groupAverageY );
				poly->SetY( ic1, groupAverageY - tempx + groupAverageX );
				poly->SetUtility(ic1,1);
			}
			if( !poly->Utility(ic2) )
			{
				// unmoved, move it
				tempx=poly->X(ic2);
				poly->SetX( ic2, groupAverageX + poly->Y(ic2)- groupAverageY );
				poly->SetY( ic2, groupAverageY - tempx + groupAverageX );
				poly->SetUtility(ic2,1);
			}
		}
		// board outline side
		else if( sid.T1() == ID_BOARD && sid.T2() == ID_BOARD && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[sid.I2()];
			int icontour = poly->Contour(0);
			int istart = poly->ContourStart(icontour);
			int iend = poly->ContourEnd(icontour);
			int ic1 = sid.I3();
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			if( !poly->Utility(ic1) )
			{
				// unmoved, move it
				tempx=poly->X(ic1);
				tempx=poly->X(ic1);
				poly->SetX( ic1, groupAverageX + poly->Y(ic1)- groupAverageY );
				poly->SetY( ic1, groupAverageY - tempx + groupAverageX );
				poly->SetUtility(ic1,1);
			}
			if( !poly->Utility(ic2) )
			{
				// unmoved, move it
				tempx=poly->X(ic2);
				poly->SetX( ic2, groupAverageX + poly->Y(ic2)- groupAverageY );
				poly->SetY( ic2, groupAverageY - tempx + groupAverageX );
				poly->SetUtility(ic2,1);
			}
		}
		else
			ASSERT(0);
	}

	// now redraw areas, solder mask cutouts and board outlines
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			net->utility = TRUE;
			if( sid.T2() == ID_AREA )
			{
				cnet * net = (cnet*)m_sel_ptrs[i];
				carea * a = &net->area[sid.I2()];
				CPolyLine * poly = a;
				if( poly->Utility() )
				{
					poly->Draw();
					poly->SetUtility(0);	// clear flag so only redraw once
				}
			}
		}
		else if( sid.T1() == ID_MASK && sid.T2() == ID_MASK && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.I2()];
			if( poly->Utility() )
			{
				poly->Draw();
				poly->SetUtility(0);	// clear flag so only redraw once
			}
		}
		else if( sid.T1() == ID_BOARD && sid.T2() == ID_OUTLINE && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[sid.I2()];
			if( poly->Utility() )
			{
				poly->Draw();
				poly->SetUtility(0);	// clear flag so only redraw once
			}
		}
	}

	// move parts in group
	cpart * part = m_Doc->m_plist->GetFirstPart();
	while( part != NULL )
	{
		if( part->utility )
		{
			// move part
			m_Doc->m_plist->Move( part, groupAverageX + part->y - groupAverageY,
				groupAverageY - part->x + groupAverageX, (part->angle+90)%360, part->side );
			// find segments which connect to this part and move them
			cnet * net;
			for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
			{
				net = (cnet*)part->pin[ip].net;
				if( net )
				{
					for( int ic=0; ic<net->NumCons(); ic++ )
					{
						cconnect * c = net->ConByIndex(ic);
						int nsegs = c->NumSegs();
						if( nsegs )
						{
							int p1 = c->start_pin;
							CString pin_name1 = net->pin[p1].pin_name;
							int pin_index1 = part->shape->GetPinIndexByName( pin_name1 );
							int p2 = c->end_pin;
							if( net->pin[p1].part == part )
							{
								// starting pin is on part
								if( !c->SegByIndex(0).utility && c->SegByIndex(0).layer != LAY_RAT_LINE )
								{
									// first segment is not selected, unroute it
									if( !c->SegByIndex(0).utility )
										m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, 0 );
								}
								// move vertex if not selected
								if( !c->VtxByIndex(0).utility )
								{
									m_Doc->m_nlist->MoveVertex( net, ic, 0,
										part->pin[pin_index1].x, part->pin[pin_index1].y );
									c->VtxByIndex(0).utility2 = 1; // moved
								}
							}
							if( p2 != cconnect::NO_END )
							{
								if( net->pin[p2].part == part )
								{
									// ending pin is on part
									if( c->SegByIndex(nsegs-1).layer != LAY_RAT_LINE )
									{
										// unroute it if not selected
										if( !c->SegByIndex(nsegs-1).utility )
											m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, nsegs-1 );
									}
									// modify vertex position if necessary
									if( !c->VtxByIndex(nsegs).utility )
									{
										// move vertex if unselected
										CString pin_name2 = net->pin[p2].pin_name;
										int pin_index2 = part->shape->GetPinIndexByName( pin_name2 );
										m_Doc->m_nlist->MoveVertex( net, ic, nsegs,
											part->pin[pin_index2].x, part->pin[pin_index2].y );
										c->VtxByIndex(nsegs).utility2 =  1;	// moved

									}
								}
							}
						}
					}
				}
			}
		}
		part = m_Doc->m_plist->GetNextPart( part );
	}

	// get selected segments
	CIterator_cnet iter_net(m_Doc->m_nlist);
	cnet * net = iter_net.GetFirst();
	while( net != NULL )
	{
		if( net->utility )
		{
			for( int ic=0; ic<net->NumCons(); ic++ )
			{
				cconnect * c = net->ConByIndex(ic);
				if( c->utility )
				{
					// undraw entire trace
					c->Undraw();
					for( int is=0; is<c->NumSegs(); is++ )
					{
						if( c->SegByIndex(is).utility )
						{
							// move trace segment by flagging adjacent vertices
							cseg * s = &c->SegByIndex(is);				// this segment
							cvertex * pre_v = &s->GetPreVtx();		// pre vertex
							cvertex * post_v = &s->GetPostVtx();	// post vertex
							CPoint old_pre_v_pt( pre_v->x, pre_v->y );		// pre vertex coords
							CPoint old_post_v_pt( post_v->x, post_v->y );	// post vertex coords
							cpart * part1 = net->pin[c->start_pin].part;	// connection starting part
							cpart * part2 = NULL;				// connection ending part or NULL
							if( c->end_pin != cconnect::NO_END )
								part2 = net->pin[c->end_pin].part;

							// flag adjacent vertices as selected
							pre_v->utility = TRUE;
							post_v->utility = TRUE;

							// unroute adjacent segments unless they are also being moved
							if( is>0 )
							{
								// test for preceding segment
								if( !c->SegByIndex(is-1).utility )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, is-1 );
							}
							if( is < c->NumSegs()-1 )
							{
								// test for following segment and not end of stub trace
								if( !c->SegByIndex(is+1).utility && (part2 || is < c->NumSegs()-2) )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, is+1 );
							}
						}
					}
					// now move vertices
					for( int iv=0; iv<=c->NumSegs(); iv++ )
					{
						cvertex * v = &c->VtxByIndex(iv);
						if( v->utility && !v->utility2 )
						{
							// selected and not already moved
							tempx=v->x;
							v->x =groupAverageX + v->y -groupAverageY;
							v->y =groupAverageY -tempx + groupAverageX;
							v->utility2 = TRUE;	// moved
							// if adjacent segments were not selected, unroute them
							if( iv>0 )
							{
								cseg * pre_s = &c->SegByIndex(iv-1);
								if( pre_s->utility == 0 )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, iv-1 );
							}
							if( iv<c->NumSegs() )
							{
								cseg * post_s = &c->SegByIndex(iv);
								if( post_s->utility == 0 )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, iv );
							}
						}
					}

					// now some special cases
					for( int is=0; is<c->NumSegs(); is++ )
					{
						if( c->SegByIndex(is).utility )
						{
							// move trace segment
							cseg * s = &c->SegByIndex(is);				// this segment
							cvertex * pre_v = &s->GetPreVtx();		// pre vertex
							cvertex * post_v = &s->GetPostVtx();	// post vertex
							cpart * part1 = net->pin[c->start_pin].part;	// connection starting part
							cpart * part2 = NULL;				// connection ending part or NULL
							if( c->end_pin != cconnect::NO_END )
								part2 = net->pin[c->end_pin].part;

							// special case, first segment of trace selected but part not selected
							if( part1->utility == FALSE && is == 0 )
							{
								// insert ratline as new first segment, unselected
								CPoint new_v_pt( pre_v->x, pre_v->y );
								CPoint old_v_pt = m_Doc->m_plist->GetPinPoint( part1, net->pin[c->start_pin].pin_name );		// pre vertex coords
								m_Doc->m_nlist->MoveVertex( net, ic, 0, old_v_pt.x, old_v_pt.y );
								m_Doc->m_nlist->InsertSegment( net, ic, 0, new_v_pt.x, new_v_pt.y, LAY_RAT_LINE, 1, 0, 0, 0 );
								c->SegByIndex(0).utility = 0;
								c->VtxByIndex(0).utility = 0;
								is++;
							}

							// special case, last segment of trace selected but part not selected
							if( part2 )
							{
								if( part2->utility == FALSE && is == c->NumSegs()-1 )
								{
									// insert ratline as new last segment
									int old_w = c->SegByIndex(c->NumSegs()-1).width;
									int old_v_w = c->VtxByIndex(c->NumSegs()-1).via_w;
									int old_v_h_w = c->VtxByIndex(c->NumSegs()-1).via_hole_w;
									int old_layer = c->SegByIndex(c->NumSegs()-1).layer;
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, c->NumSegs()-1 );
									CPoint new_v_pt( c->VtxByIndex(c->NumSegs()).x, c->VtxByIndex(c->NumSegs()).y );
									CPoint old_v_pt = m_Doc->m_plist->GetPinPoint( part2, net->pin[c->end_pin].pin_name );
									m_Doc->m_nlist->MoveVertex( net, ic, c->NumSegs(), old_v_pt.x, old_v_pt.y );
									BOOL bInserted = m_Doc->m_nlist->InsertSegment( net, ic, c->NumSegs()-1,
										new_v_pt.x, new_v_pt.y, old_layer, old_w, old_v_w, old_v_h_w, 0 );
									c->SegByIndex(c->NumSegs()-2).utility = 1;
									c->SegByIndex(c->NumSegs()-1).utility = 0;
								}
							}
						}
					}
					m_Doc->m_nlist->MergeUnroutedSegments( net, ic );	// this also redraws connection
				}
			}

			// now deal with tees that have been moved
			// requiring that stubs attached to tees have to move as well
			// if attached segments have not been selected, they must be unrouted
			for( int ic=0; ic<net->NumCons(); ic++ )
			{
				cconnect * c = net->ConByIndex(ic);
				if( c->end_pin == cconnect::NO_END )
				{
					cvertex * end_vtx = &c->VtxByIndex(c->NumSegs());
					cseg * end_seg = &c->SegByIndex(c->NumSegs()-1);
					if( int id = end_vtx->tee_ID )
					{
						// stub tee
						int tee_ic;
						int tee_iv;
						BOOL bFound = m_Doc->m_nlist->FindTeeVertexInNet( net, id, &tee_ic, &tee_iv );
						if ( !bFound )
						{
							end_vtx->tee_ID = 0;
						}
						else
						{
							cvertex * tee_vtx;
							tee_vtx = &net->ConByIndex(tee_ic)->VtxByIndex(tee_iv);
							if( tee_vtx->utility2 )
							{
								// tee-vertex was moved
								end_vtx->x = tee_vtx->x;
								end_vtx->y = tee_vtx->y;
								if( !end_seg->utility )
								{
									// attached segment not selected
									c->Undraw();
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, c->NumSegs()-1 );
									c->Draw();
								}
							}
						}
					}
				}
			}
		}
		net = iter_net.GetNext();
	}

	// merge unrouted segments for all traces
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT
			&& sid.T3() == ID_SEL_SEG )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.I2();
			m_Doc->m_nlist->MergeUnroutedSegments( net, ic );
		}
	}

	//** this shouldn't be necessary
	CNetList * nl = m_Doc->m_nlist;
	for( cnet * n=iter_net.GetFirst(); n; n=iter_net.GetNext() )
		nl->RehookPartsToNet( n );
	//**
	m_Doc->m_nlist->SetAreaConnections();

	// regenerate selection list from utility flags
	// first, remove all segments and vertices
	for( int i=m_sel_ids.GetSize()-1; i>=0; i-- )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT && sid.T3() == ID_SEL_SEG )
		{
			m_sel_ids.RemoveAt(i);
			m_sel_ptrs.RemoveAt(i);
		}
		else if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT && sid.T3() == ID_SEL_VERTEX )
		{
			m_sel_ids.RemoveAt(i);
			m_sel_ptrs.RemoveAt(i);
		}
	}
	// add segments and vertices back into group
	net = iter_net.GetFirst();
	while( net )
	{
		if( net->utility )
		{
			// selected net
			for( int ic=0; ic<net->NumCons(); ic++ )
			{
				cconnect * c = net->ConByIndex(ic);
				if( c->utility )
				{
					// selected connection
					for( int is=0; is<c->NumSegs(); is++ )
					{
						if( c->SegByIndex(is).utility )
						{
							cseg * s = &c->SegByIndex(is);
							m_sel_ptrs.Add( net );
							id sid( ID_NET, net->UID(), ID_CONNECT, c->UID(), ic, 
								ID_SEL_SEG, s->UID(), is );
							m_sel_ids.Add( sid );
							c->SegByIndex(is).dl_el->visible = 1;	// restore visibility
						}
					}
					for( int iv=0; iv<c->NumSegs()+1; iv++ )
					{
						cvertex * v = &c->VtxByIndex(iv);
						if( v->utility )
						{
							if( v->via_w || v->tee_ID )
							{
								m_sel_ptrs.Add( net );
								id vid( ID_NET, net->UID(), ID_CONNECT, c->UID(), ic, 
									ID_SEL_VERTEX, v->UID(), iv );
								m_sel_ids.Add( vid );
								if( v->via_w )
								{
									int n_el = v->dl_el.GetSize();
									for( int il=0; il<n_el; il++ )
										v->dl_el[il]->visible = 1;
								}
							}
						}
					}
				}
			}
		}
		net = iter_net.GetNext();
	}
}

void CFreePcbView::FindGroupCenter()
{
	int groupNumberItems = 0;
	groupAverageX = groupAverageY = 0;

	// find parts
	if( m_sel_mask & (1<<SEL_MASK_PARTS ) )  // may not be necessary??
	{
		cpart * part = m_Doc->m_plist->GetFirstPart();
		while( part )
		{
			id pid( ID_PART, -1, ID_SEL_RECT );
			if( FindItemInGroup( part, &pid ) != -1 )
			{
				groupAverageX+=part->x;
				groupAverageY+=part->y;
				groupNumberItems++;
			}
			part = m_Doc->m_plist->GetNextPart( part );
		}
	}

	// find trace segments and vertices contained in rect
	CIterator_cnet iter_net(m_Doc->m_nlist);
	if( m_sel_mask & (1<<SEL_MASK_CON ) )
	{
		cnet * net = iter_net.GetFirst();
		while( net )
		{
			for( int ic=0; ic<net->NumCons(); ic++ )
			{
				cconnect * c = net->ConByIndex(ic);
				for( int is=0; is<c->NumSegs(); is++ )
				{
					cseg * s = &c->SegByIndex(is);
					cvertex * pre_v = &s->GetPreVtx();
					cvertex * post_v = &s->GetPostVtx();

					if( m_Doc->m_vis[s->layer] )
					{
						// add segment to selection list and highlight it
						id sid( ID_NET, net->UID(), ID_CONNECT, c->UID(), ic, 
							ID_SEL_SEG, s->UID(), is );
						if( FindItemInGroup( net, &sid ) != -1 )
						{
							groupAverageX+=pre_v->x+post_v->x;
							groupAverageY+=pre_v->y+post_v->y;
							groupNumberItems+=2;
						}
					}
				}
			}
			net = iter_net.GetNext();
		}
	}

	// find texts in group
	if( m_sel_mask & (1<<SEL_MASK_TEXT ) )
	{
		CIterator_CText iter_t( m_Doc->m_tlist );
		for( CText * t = iter_t.GetFirst(); t != NULL; t = iter_t.GetNext() )		
		{
			if( m_Doc->m_vis[t->m_layer] )
			{
				// add text to selection list and highlight it
				id sid( ID_TEXT, -1, ID_TEXT, -1, -1, ID_SEL_TXT );
				if( FindItemInGroup( t, &sid ) != -1 )
				{
					groupAverageX+=m_dlist->Get_x( t->dl_sel );
					groupAverageY+=m_dlist->Get_y( t->dl_sel );
					groupNumberItems++;
				}
			}
		}
	}

	// find copper area sides in rect
	if( m_sel_mask & (1<<SEL_MASK_AREAS ) )
	{
		cnet * net = iter_net.GetFirst();
		while( net )
		{
			if( net->NumAreas() )
			{
				for( int ia=0; ia<net->NumAreas(); ia++ )
				{
					carea * a = &net->area[ia];
					CPolyLine * poly = a;
					for( int ic=0; ic<poly->NumContours(); ic++ )
					{
						int istart = poly->ContourStart(ic);
						int iend = poly->ContourEnd(ic);
						for( int is=istart; is<=iend; is++ )
						{
							int ic1, ic2;
							ic1 = is;
							if( is < iend )
								ic2 = is+1;
							else
								ic2 = istart;
							int x1 = poly->X(ic1);
							int y1 = poly->Y(ic1);
							int x2 = poly->X(ic2);
							int y2 = poly->Y(ic2);
							if( m_Doc->m_vis[poly->Layer()] )
							{
								id aid( ID_NET, net->UID(), ID_AREA, a->UID(), ia, 
									ID_SEL_SIDE, -1, is );
								if( FindItemInGroup( net, &aid ) != -1 )
								{
									groupAverageX+=x1+x2;
									groupAverageY+=y1+y2;
									groupNumberItems+=2;
								}
							}
						}
					}
				}
			}
			net = iter_net.GetNext();
		}
	}

	// find solder mask cutout sides in rect
	if( m_sel_mask & (1<<SEL_MASK_SM ) )
	{
		for( int im=0; im<m_Doc->m_sm_cutout.GetSize(); im++ )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[im];
			for( int ic=0; ic<poly->NumContours(); ic++ )
			{
				int istart = poly->ContourStart(ic);
				int iend = poly->ContourEnd(ic);
				for( int is=istart; is<=iend; is++ )
				{
					int ic1, ic2;
					ic1 = is;
					if( is < iend )
						ic2 = is+1;
					else
						ic2 = istart;
					int x1 = poly->X(ic1);
					int y1 = poly->Y(ic1);
					int x2 = poly->X(ic2);
					int y2 = poly->Y(ic2);
					if( m_Doc->m_vis[poly->Layer()] )
					{
						id smid( ID_MASK, -1, ID_MASK, -1, im, 
							ID_SEL_SIDE, -1, is );
						if( FindItemInGroup( poly, &smid ) != -1 )
						{
							groupAverageX+=x1+x2;
							groupAverageY+=y1+y2;
							groupNumberItems+=2;
						}
					}
				}
			}
		}
	}

	// find board outline sides in rect
	if( m_sel_mask & (1<<SEL_MASK_BOARD ) )
	{
		for( int im=0; im<m_Doc->m_board_outline.GetSize(); im++ )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[im];
			for( int ic=0; ic<poly->NumContours(); ic++ )
			{
				int istart = poly->ContourStart(ic);
				int iend = poly->ContourEnd(ic);
				for( int is=istart; is<=iend; is++ )
				{
					int ic1, ic2;
					ic1 = is;
					if( is < iend )
						ic2 = is+1;
					else
						ic2 = istart;
					int x1 = poly->X(ic1);
					int y1 = poly->Y(ic1);
					int x2 = poly->X(ic2);
					int y2 = poly->Y(ic2);
					if( m_Doc->m_vis[poly->Layer()] )
					{
						id bd_id( ID_BOARD, ID_BOARD, im, ID_SEL_SIDE, is );
						if( FindItemInGroup( poly, &bd_id ) != -1 )
						{
							groupAverageX+=x1+x2;
							groupAverageY+=y1+y2;
							groupNumberItems+=2;
						}
					}
				}
			}
		}
	}

	if(groupNumberItems>0)
	{
		groupAverageX/=groupNumberItems;
		groupAverageY/=groupNumberItems;

		double x=floor(groupAverageX/m_Doc->m_part_grid_spacing +.5);
		groupAverageX=(long long)(m_Doc->m_part_grid_spacing*x);
		x=floor(groupAverageY/m_Doc->m_part_grid_spacing +.5);
		groupAverageY=(long long)(m_Doc->m_part_grid_spacing*x);
	}
}


// save undo info for part, prior to editing operation
// type may be:
//	UNDO_PART_DELETE	if part will be deleted
//	UNDO_PART_MODIFY	if part will be modified (e.g. moved)
//	UNDO_PART_ADD		if part will be added
// for UNDO_PART_ADD, use reference designator to identify part, ignore cpart * part
// on callback, ref_des will be used to find part, then name will be changed to part->ref_des
//
void CFreePcbView::SaveUndoInfoForPart( cpart * part, int type, CString * ref_des, BOOL new_event, CUndoList * list )
{
	undo_part * u_part;
	if( new_event )
		list->NewEvent();
	if( type == CPartList::UNDO_PART_ADD )
		u_part = m_Doc->m_plist->CreatePartUndoRecord( NULL, ref_des );
	else if( ref_des )
		u_part = m_Doc->m_plist->CreatePartUndoRecord( part, ref_des );
	else
		u_part = m_Doc->m_plist->CreatePartUndoRecord( part, &part->ref_des );

	list->Push( type, u_part, &m_Doc->m_plist->PartUndoCallback, u_part->size );

	void * ptr;
	if( new_event )
	{
		if( type == CPartList::UNDO_PART_ADD )
			ptr = CreateUndoDescriptor( list, type, ref_des, NULL, 0, 0, ref_des, NULL );
		else if( ref_des )
			ptr = CreateUndoDescriptor( list, type, &part->ref_des, NULL, 0, 0, ref_des, NULL );
		else
			ptr = CreateUndoDescriptor( list, type, &part->ref_des, NULL, 0, 0, &part->ref_des, NULL );
		list->Push( UNDO_PART, ptr, &UndoCallback );
	}
}

// save undo info for a part and all nets connected to it
// type may be:
//	UNDO_PART_DELETE	if part will be deleted
//	UNDO_PART_MODIFY	if part will be modified (e.g. moved or ref_des changed)
// note that the ref_des may be different than the part->ref_des
// on callback, ref_des will be used to find part, then name will be changed to part->ref_des
//
void CFreePcbView::SaveUndoInfoForPartAndNets( cpart * part, int type, CString * ref_des, BOOL new_event, CUndoList * list )
{
	void * ptr;
	if( new_event )
		list->NewEvent();
	// set utility = 0 for all nets affected
	for( int ip=0; ip<part->pin.GetSize(); ip++ )
	{
		cnet * net = (cnet*)part->pin[ip].net;
		if( net )
			net->utility = 0;
	}
	// save undo info for all nets affected
	for( int ip=0; ip<part->pin.GetSize(); ip++ )
	{
		cnet * net = (cnet*)part->pin[ip].net;
		if( net )
		{
			if( net->utility == 0 )
			{
				SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, FALSE, list );
				net->utility = 1;
			}
		}
	}
	// save undo info for part
	SaveUndoInfoForPart( part, type, ref_des, FALSE, list );

	// save top-level descriptor
	if( new_event )
	{
		if( ref_des )
			ptr = CreateUndoDescriptor( list, type, &part->ref_des, NULL, 0, 0, ref_des, NULL );
		else
			ptr = CreateUndoDescriptor( list, type, &part->ref_des, NULL, 0, 0, &part->ref_des, NULL );
		list->Push( UNDO_PART_AND_NETS, ptr, &UndoCallback );
	}
}

// save undo info for a net (not connections or areas)
//
void CFreePcbView::SaveUndoInfoForNet( cnet * net, int type, BOOL new_event, CUndoList * list )
{
	void * ptr;
	if( new_event )
		list->NewEvent();
	undo_net * u_net = m_Doc->m_nlist->CreateNetUndoRecord( net );
	list->Push( type, u_net, &m_Doc->m_nlist->NetUndoCallback, u_net->size );
}

// save undo info for a net and connections, not areas
//
void CFreePcbView::SaveUndoInfoForNetAndConnections( cnet * net, int type, BOOL new_event, CUndoList * list )
{
	void * ptr;
	if( new_event )
		list->NewEvent();
	if( type != CNetList::UNDO_NET_ADD )
		for( int ic=net->NumCons()-1; ic>=0; ic-- )
		{
			cconnect * c = net->ConByIndex(ic);
			SaveUndoInfoForConnection( net, ic, FALSE, list );
		}
	SaveUndoInfoForNet( net, type, FALSE, list );
	if( new_event )
	{
		ptr = CreateUndoDescriptor( list, type, &net->name, NULL, 0, 0, NULL, NULL );
		list->Push( UNDO_NET_AND_CONNECTIONS, ptr, &UndoCallback );
	}
}

// save undo info for a connection
// Note: this is now ONLY called from other Undo functions, it should never be used on its own
//
void CFreePcbView::SaveUndoInfoForConnection( cnet * net, int ic, BOOL new_event, CUndoList * list )
{
	if( new_event )
		list->NewEvent();
	undo_con * u_con = m_Doc->m_nlist->CreateConnectUndoRecord( net, ic );
	list->Push( CNetList::UNDO_CONNECT_MODIFY, u_con,
		&m_Doc->m_nlist->ConnectUndoCallback, u_con->size );
}

// top-level description of undo operation
// list is the CUndoList that it will be pushed to
//
void * CFreePcbView::CreateUndoDescriptor( CUndoList * list, int type, CString * name1, CString * name2,
										  int int1, int int2, CString * str1, void * ptr )
{
	undo_descriptor * u_d = new undo_descriptor;
	u_d->view = this;
	u_d->list = list;
	u_d->type = type;
	if( name1 )
		u_d->name1 = *name1;
	if( name2 )
		u_d->name2 = *name2;
	u_d->int1 = int1;
	u_d->int2 = int2;
	if( str1 )
		u_d->str1 = *str1;
	u_d->ptr = ptr;
	return (void*)u_d;
}

// initial callback from undo/redo stack
// used to push redo/undo info onto the other stack
// note this is a static function (i.e. global)
//
void CFreePcbView::UndoCallback( int type, void * ptr, BOOL undo )
{
	undo_descriptor * u_d = (undo_descriptor*)ptr;
	if( undo )
	{
		CFreePcbView * view = u_d->view;
		// if callback was from undo_list, push info to redo list, and vice versa
		CUndoList * redo_list;
		if( u_d->list == view->m_Doc->m_undo_list )
			redo_list = view->m_Doc->m_redo_list;
		else
			redo_list = view->m_Doc->m_undo_list;
		undo_text * u_text = (undo_text *)u_d->ptr;
		// save undo/redo info
		if( type == UNDO_PART )
		{
			cpart * part = view->m_Doc->m_plist->GetPart( u_d->str1 );	//use new ref des
			if( u_d->type == CPartList::UNDO_PART_ADD )
			{
				view->SaveUndoInfoForPartAndNets( part, CPartList::UNDO_PART_DELETE, &u_d->str1, TRUE, redo_list );
			}
			else if( u_d->type == CPartList::UNDO_PART_MODIFY )
			{
				view->SaveUndoInfoForPart( part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, redo_list );
			}
		}
		else if( type == UNDO_PART_AND_NETS )
		{
			cpart * part = view->m_Doc->m_plist->GetPart( u_d->str1 );
			if(u_d->type == CPartList::UNDO_PART_DELETE )
				view->SaveUndoInfoForPart( NULL, CPartList::UNDO_PART_ADD, &u_d->name1, TRUE, redo_list );
			else if( u_d->type == CPartList::UNDO_PART_MODIFY )
				view->SaveUndoInfoForPartAndNets( part, CPartList::UNDO_PART_MODIFY, &u_d->name1, TRUE, redo_list );
		}
		else if( type == UNDO_2_PARTS_AND_NETS )
		{
			cpart * part = view->m_Doc->m_plist->GetPart( u_d->name1 );
			cpart * part2 = view->m_Doc->m_plist->GetPart( u_d->name2 );
			view->SaveUndoInfoFor2PartsAndNets( part, part2, TRUE, redo_list );
		}
		else if( type == UNDO_NET_AND_CONNECTIONS )
		{
			cnet * net = view->m_Doc->m_nlist->GetNetPtrByName( &u_d->name1 );
			view->SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, TRUE, redo_list );
		}
		else if( type == UNDO_AREA )
		{
			cnet * net = view->m_Doc->m_nlist->GetNetPtrByName( &u_d->name1 );
			if( u_d->type == CNetList::UNDO_AREA_ADD )
				view->SaveUndoInfoForArea( net, u_d->int1, CNetList::UNDO_AREA_DELETE, TRUE, redo_list );
			else if( u_d->type == CNetList::UNDO_AREA_DELETE )
				view->SaveUndoInfoForArea( net, u_d->int1, CNetList::UNDO_AREA_ADD, TRUE, redo_list );
			else if( type == UNDO_AREA )
				view->SaveUndoInfoForArea( net, u_d->int1, CNetList::UNDO_AREA_MODIFY, TRUE, redo_list );
		}
		else if( type == UNDO_ALL_AREAS_IN_NET )
		{
			cnet * net = view->m_Doc->m_nlist->GetNetPtrByName( &u_d->name1 );
			view->SaveUndoInfoForAllAreasInNet( net, TRUE, redo_list );
		}
		else if( type == UNDO_ALL_AREAS_IN_2_NETS )
		{
			cnet * net1 = view->m_Doc->m_nlist->GetNetPtrByName( &u_d->name1 );
			cnet * net2 = view->m_Doc->m_nlist->GetNetPtrByName( &u_d->name2 );
			view->SaveUndoInfoForAllAreasIn2Nets( net1, net2, TRUE, redo_list );
		}
		else if( type == UNDO_ALL_BOARD_OUTLINES )
		{
			view->SaveUndoInfoForBoardOutlines( TRUE, redo_list );
		}
		else if( type == UNDO_ALL_SM_CUTOUTS )
		{
			view->SaveUndoInfoForSMCutouts( TRUE, redo_list );
		}
		else if( type == UNDO_TEXT )
		{
			if( u_d->type == CTextList::UNDO_TEXT_ADD )
				view->SaveUndoInfoForText( u_text, CTextList::UNDO_TEXT_DELETE, TRUE, redo_list );
			else if( u_d->type == CTextList::UNDO_TEXT_MODIFY )
			{
				int uid = u_text->m_uid;
				CText * text = view->m_Doc->m_tlist->GetText( uid );
				if( !text )
					ASSERT(0);	// uid not found
				view->SaveUndoInfoForText( text, CTextList::UNDO_TEXT_MODIFY, TRUE, redo_list );
			}
			else if( u_d->type == CTextList::UNDO_TEXT_DELETE )
				view->SaveUndoInfoForText( u_text, CTextList::UNDO_TEXT_ADD, TRUE, redo_list );
		}
		else if( type == UNDO_MOVE_ORIGIN )
		{
			view->SaveUndoInfoForMoveOrigin( -u_d->int1, -u_d->int2, redo_list );
		}
		else
			ASSERT(0);
	}
	delete(u_d);	// delete the undo record
}

// callback for undoing group operations
// note this is a static function (i.e. global)
//
void CFreePcbView::UndoGroupCallback( int type, void * ptr, BOOL undo )
{
	undo_group_descriptor * u_d = (undo_group_descriptor*)ptr;
	if( undo )
	{
		CFreePcbView * view = u_d->view;
		CFreePcbDoc * doc = view->m_Doc;
		// if callback was from undo_list, push info to redo list, and vice versa
		CUndoList * redo_list;
		if( u_d->list == view->m_Doc->m_undo_list )
			redo_list = view->m_Doc->m_redo_list;
		else
			redo_list = view->m_Doc->m_undo_list;
		if( u_d->type == UNDO_GROUP_MODIFY || u_d->type == UNDO_GROUP_ADD )
		{
			// reconstruct pointers from names of items (since they may have changed)
			// and save the current status of the group
			int n_items = u_d->m_ids.GetSize();
			CArray<void*> ptrs;
			ptrs.SetSize( n_items );
			for( int i=0; i<n_items; i++ )
			{
				CString * str_ptr = &u_d->str[i];
				id this_id = u_d->m_ids[i];
				if( this_id.T1() == ID_PART )
				{
					cpart * part = doc->m_plist->GetPart( *str_ptr );
					if( part )
						ptrs[i] = (void*)part;
					else
						ASSERT(0);	// couldn't find part
				}
				else if( this_id.T1() == ID_NET )
				{
					cnet * net = doc->m_nlist->GetNetPtrByName( str_ptr );
					if( net )
						ptrs[i] = (void*)net;
					else
						ASSERT(0);	// couldn't find net
				}
				else if( this_id.T1() == ID_TEXT )
				{
					CText * text = doc->m_tlist->GetText( this_id.U1()  );
					if( text )
						ptrs[i] = (void*)text;
					else
						ASSERT(0);	// couldn't find text
				}
			}
			if( u_d->type == UNDO_GROUP_MODIFY )
				view->SaveUndoInfoForGroup( u_d->type, &ptrs, &u_d->m_ids, redo_list );
			else if( u_d->type == UNDO_GROUP_ADD )
			{
				// delete group
				view->DeleteGroup( &ptrs, &u_d->m_ids );
			}
		}
		else if( u_d->type == UNDO_GROUP_DELETE )
		{
			// just copy the undo record with type UNDO_GROUP_ADD
			undo_group_descriptor * new_u_d = new undo_group_descriptor;
			new_u_d->list = redo_list;
			new_u_d->type = UNDO_GROUP_ADD;
			new_u_d->view = u_d->view;
			int n_items = u_d->m_ids.GetSize();
			new_u_d->str.SetSize( n_items );
			new_u_d->m_ids.SetSize( n_items );
			for( int i=0; i<n_items; i++ )
			{
				new_u_d->m_ids[i] = u_d->m_ids[i];
				new_u_d->str[i] = u_d->str[i];
			}
			redo_list->NewEvent();
			redo_list->Push( UNDO_GROUP, (void*)new_u_d, &view->UndoGroupCallback );
		}
	}
	delete(u_d);	// delete the undo record
}

// create descriptor used for undo/redo of groups
// mainly a list of the items in the group
// since pointers cannot be used for undo/redo since they may change,
// net names, reference designators and guids are saved as strings
//
void * CFreePcbView::CreateGroupDescriptor( CUndoList * list, CArray<void*> * ptrs, CArray<id> * ids, int type )
{
	undo_group_descriptor * undo = new undo_group_descriptor;
	int n_items = ids->GetSize();
	undo->view = this;
	undo->list = list;
	undo->type = type;
	undo->str.SetSize( n_items );
	undo->m_ids.SetSize( n_items );
	for( int i=0; i<n_items; i++ )
	{
		id this_id = (*ids)[i];
		undo->m_ids[i] = this_id;
		if( this_id.T1() == ID_PART )
		{
			cpart * part = (cpart*)(*ptrs)[i];
			undo->str[i] = part->ref_des;
		}
		else if( this_id.T1() == ID_NET )
		{
			cnet * net = (cnet*)(*ptrs)[i];
			undo->str[i] = net->name;
		}
	}
	return undo;
}


void CFreePcbView::OnGroupRotate()
{
	m_dlist->CancelHighLight();
	if( !gLastKeyWasArrow && !gLastKeyWasGroupRotate)
	{
		if( GluedPartsInGroup() )
		{
			int ret = AfxMessageBox( "This group contains glued parts, unglue and rotate them ?  ", MB_YESNO );
			if( ret != IDYES )
				return;
		}
		SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, &m_sel_ptrs, &m_sel_ids, m_Doc->m_undo_list );
		gLastKeyWasGroupRotate=true;
	}
	RotateGroup( );
	HighlightGroup();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// enable/disable the main menu
// used when dragging
//
void CFreePcbView::SetMainMenu( BOOL bAll )
{
	CFrameWnd * pMainWnd = (CFrameWnd*)AfxGetMainWnd();
	if( bAll )
	{
		pMainWnd->SetMenu(&theApp.m_main);
		if( m_Doc->m_project_modified )
			m_Doc->ProjectModified( TRUE, FALSE );
	}
	else
		pMainWnd->SetMenu(&theApp.m_main_drag);
	return;
}
void CFreePcbView::OnRefShowPart()
{
	cpart * part = m_sel_part;
	CancelSelection();
	dl_element * dl_sel = part->dl_sel;
	int xc = (m_dlist->Get_x( dl_sel ) + m_dlist->Get_xf( dl_sel ))/2;
	int yc = (m_dlist->Get_y( dl_sel ) + m_dlist->Get_yf( dl_sel ))/2;
	m_org_x = xc - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
	m_org_y = yc - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
	CRect screen_r;
	GetWindowRect( &screen_r );
	m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
		m_org_x, m_org_y );
	CPoint p(xc, yc);
	p = m_dlist->PCBToScreen( p );
	SetCursorPos( p.x, p.y - 4 );
	SelectPart( part );
}

void CFreePcbView::OnAreaSideStyle()
{
	CDlgSideStyle dlg;
	int style = m_sel_net->area[m_sel_ia].SideStyle( m_sel_id.I3() );
	dlg.Initialize( style );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
		m_dlist->CancelHighLight();
		m_sel_net->area[m_sel_ia].SetSideStyle( m_sel_id.I3(), dlg.m_style );
		m_Doc->m_nlist->SelectAreaSide( m_sel_net, m_sel_ia, m_sel_id.I3() );
		m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
		if( m_Doc->m_vis[LAY_RAT_LINE] )
			m_Doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_Doc->m_auto_ratline_disable,
														m_Doc->m_auto_ratline_min_pins, TRUE );
	}
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// move text string for value
//
void CFreePcbView::OnValueMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to part origin
	CPoint cur_p = m_dlist->PCBToScreen( m_last_cursor_point );
	SetCursorPos( cur_p.x, cur_p.y );
	m_dragging_new_item = 0;
	m_Doc->m_plist->StartDraggingValue( pDC, m_sel_part );
	SetCursorMode( CUR_DRAG_VALUE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFreePcbView::OnValueProperties()
{
	CDlgValueText dlg;
	dlg.Initialize( m_Doc->m_plist, m_sel_part );
	int ret =  dlg.DoModal();
	if( ret == IDOK )
	{
		// edit this part
		SaveUndoInfoForPart( m_sel_part,
			CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list ); 
		int value_layer = dlg.m_layer;
		value_layer = FlipLayer( m_sel_part->side, value_layer );
		m_Doc->m_plist->SetValue( m_sel_part, &m_sel_part->value, 
			m_sel_part->m_value_xi, m_sel_part->m_value_yi, 
			m_sel_part->m_value_angle,
			dlg.m_height, dlg.m_width, dlg.m_vis, value_layer );
		m_Doc->ProjectModified( TRUE );
		m_dlist->CancelHighLight();
		if( m_cursor_mode == CUR_PART_SELECTED )
			m_Doc->m_plist->SelectPart( m_sel_part );
		else if( m_cursor_mode == CUR_VALUE_SELECTED 
			&& m_sel_part->m_value_size && m_sel_part->m_value_vis )
			m_Doc->m_plist->SelectValueText( m_sel_part );
		else
			CancelSelection();
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnValueShowPart()
{
	OnRefShowPart();
}

void CFreePcbView::OnPartEditValue()
{
	OnValueProperties();
}


void CFreePcbView::OnRefRotateCW()
{
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list ); 
	m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->m_ref_angle = (m_sel_part->m_ref_angle + 90)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_plist->SelectRefText( m_sel_part );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnRefRotateCCW()
{
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list ); 
	m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->m_ref_angle = (m_sel_part->m_ref_angle + 270)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_plist->SelectRefText( m_sel_part );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnValueRotateCW()
{
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list ); 
	m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->m_value_angle = (m_sel_part->m_value_angle + 90)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_plist->SelectValueText( m_sel_part );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnValueRotateCCW()
{
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list ); 
	m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->m_value_angle = (m_sel_part->m_value_angle + 270)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_plist->SelectValueText( m_sel_part );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}


void CFreePcbView::OnSegmentMove()
{
//	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	id id = m_sel_id;
	int ic = m_sel_id.I2();
	int ivtx = m_sel_id.I3();
	m_dragging_new_item = 0;
	
	m_last_pt.x = m_sel_prev_vtx->x;
	m_last_pt.y = m_sel_prev_vtx->y;

	m_from_pt.x = m_sel_vtx->x;
	m_from_pt.y = m_sel_vtx->y;

	m_to_pt.x = m_sel_next_vtx->x;
	m_to_pt.y = m_sel_next_vtx->y;

	int nsegs = m_sel_con->NumSegs();
	int use_third_segment = ivtx < nsegs - 1;
	if(use_third_segment)
	{
		m_next_pt.x = m_sel_next_next_vtx->x;	// Shouldn't really do this if we're off the edge?
		m_next_pt.y = m_sel_next_next_vtx->y;
	}

	CPoint p;
//	p = m_last_mouse_point;

	p.x = (m_to_pt.x - m_from_pt.x) / 2 + m_from_pt.x;
	p.y = (m_to_pt.y - m_from_pt.y) / 2 + m_from_pt.y;


	m_Doc->m_nlist->StartMovingSegment( pDC, m_sel_net, ic, ivtx, p.x, p.y, 2, use_third_segment );
	SetCursorMode( CUR_MOVE_SEGMENT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}
