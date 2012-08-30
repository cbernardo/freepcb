// MyToolBar.cpp : implementation file
//

#include "stdafx.h"
#include "MyToolBar.h"
#include "math.h"

//#define IDC_COMBO_VISIBLE_GRID 2000
#define COMBO_W 100
#define COMBO_H 100

#define POS_COMBO_VISIBLE_GRID 11

extern CFreePcbApp theApp;

// CMyToolBar

IMPLEMENT_DYNAMIC(CMyToolBar, CToolBar)
CMyToolBar::CMyToolBar()
{
	m_last_vg = -1;
}

CMyToolBar::~CMyToolBar()
{
}


BEGIN_MESSAGE_MAP(CMyToolBar, CToolBar)
	ON_WM_CREATE()
	ON_CBN_SELENDOK( IDC_COMBO_VISIBLE_GRID, OnSelectVisibleGrid )
	ON_CBN_SELENDOK( IDC_COMBO_PLACEMENT_GRID, OnSelectPlacementGrid )
	ON_CBN_SELENDOK( IDC_COMBO_ROUTING_GRID, OnSelectRoutingGrid )
	ON_CBN_SELENDOK( IDC_COMBO_SNAP_ANGLE, OnSelectSnapAngle )
	ON_CBN_SELENDOK( IDC_COMBO_UNITS, OnSelectUnits )
END_MESSAGE_MAP()



// CMyToolBar message handlers

int CMyToolBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CToolBar::OnCreate(lpCreateStruct) == -1)
		return -1;

	if( !LoadToolBar( IDR_MYTOOLBAR ) )
		return -1;

	// CPT:  make widths of static text adjustable via changing string resource values
	CString s;
	s.LoadStringA(IDS_UnitsWidth);
	int wUnits = atoi(s);
	if (wUnits<=0) wUnits = 40;
	s.LoadStringA(IDS_GridsVisibleWidth);
	int wGridsVisible = atoi(s);
	if (wGridsVisible<=0) wGridsVisible = 80;
	s.LoadStringA(IDS_PlacementWidth);
	int wPlacement = atoi(s);
	if (wPlacement<=0) wPlacement = 64;
	s.LoadStringA(IDS_RoutingWidth);
	int wRouting = atoi(s);
	if (wRouting<=0) wRouting = 50;
	s.LoadStringA(IDS_AngleWidth);
	int wAngle = atoi(s);
	if (wAngle<=0) wAngle = 50;

	SetButtonInfo( 12, IDC_STATIC_VISIBLE_GRID, TBBS_SEPARATOR, wUnits );
	SetButtonInfo( 13, IDC_STATIC_VISIBLE_GRID, TBBS_SEPARATOR, 50 );
	SetButtonInfo( 14, IDC_STATIC_VISIBLE_GRID, TBBS_SEPARATOR, wGridsVisible );
	SetButtonInfo( 15, IDC_COMBO_VISIBLE_GRID, TBBS_SEPARATOR, 70 );
	SetButtonInfo( 16, IDC_STATIC_PLACEMENT_GRID, TBBS_SEPARATOR, wPlacement );
	SetButtonInfo( 17, IDC_COMBO_PLACEMENT_GRID, TBBS_SEPARATOR, 70 );
	SetButtonInfo( 18, IDC_STATIC_ROUTING_GRID, TBBS_SEPARATOR, wRouting );
	SetButtonInfo( 19, IDC_COMBO_ROUTING_GRID, TBBS_SEPARATOR, 70 );
	SetButtonInfo( 20, IDC_STATIC_SNAP_ANGLE, TBBS_SEPARATOR, wAngle );
	SetButtonInfo( 21, IDC_COMBO_SNAP_ANGLE, TBBS_SEPARATOR, 50 );
	// end CPT

	m_font.CreateFont( 14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, 
		DEFAULT_PITCH | FF_DONTCARE, "Arial" );
	SetFont( &m_font );

	CRect rect;
	GetItemRect( 12, &rect );
	rect.top += 5;
	rect.left += 5;
	s.LoadStringA(IDS_ToolbarUnits);
	m_ctlStaticUnits.Create( s, WS_VISIBLE, rect, this, IDC_STATIC_VISIBLE_GRID );
	m_ctlStaticUnits.SetFont( &m_font );

	GetItemRect( 13, &rect );
	rect.bottom = rect.top + 300;
	m_ctlComboUnits.Create( WS_CHILD | WS_VISIBLE | WS_VSCROLL |
		CBS_DROPDOWNLIST, rect, this, IDC_COMBO_UNITS );
	m_ctlComboUnits.SetFont( &m_font );

	m_ctlComboUnits.InsertString( 0, "mil" );
	m_ctlComboUnits.InsertString( 1, "mm" );
	m_ctlComboUnits.SetCurSel( 0 );

	GetItemRect( 14, &rect );
	rect.top += 5;
	rect.left += 5;
	s.LoadStringA(IDS_ToolbarGridsVisible);
	m_ctlStaticVisibleGrid.Create( s, WS_VISIBLE, rect, this, IDC_STATIC_VISIBLE_GRID );
	m_ctlStaticVisibleGrid.SetFont( &m_font );

	GetItemRect( 15, &rect );
	rect.bottom = rect.top + 300;
	m_ctlComboVisibleGrid.Create( WS_CHILD | WS_VISIBLE | WS_VSCROLL |
		CBS_DROPDOWNLIST, rect, this, IDC_COMBO_VISIBLE_GRID );
	m_ctlComboVisibleGrid.SetFont( &m_font );

	GetItemRect( 16, &rect );
	rect.top += 5;
	rect.left += 5;
	s.LoadStringA(IDS_ToolbarPlacement);
	m_ctlStaticPlacementGrid.Create( s, WS_VISIBLE, rect, this, IDC_STATIC_PLACEMENT_GRID );
	m_ctlStaticPlacementGrid.SetFont( &m_font );

	GetItemRect( 17, &rect );
	rect.bottom = rect.top + 300;
	m_ctlComboPlacementGrid.Create( WS_CHILD | WS_VISIBLE | WS_VSCROLL |
		CBS_DROPDOWNLIST, rect, this, IDC_COMBO_PLACEMENT_GRID );
	m_ctlComboPlacementGrid.SetFont( &m_font );

	GetItemRect( 18, &rect );
	rect.top += 5;
	rect.left += 5;
	s.LoadStringA(IDS_ToolbarRouting);
	m_ctlStaticRoutingGrid.Create( s, WS_VISIBLE, rect, this, IDC_STATIC_ROUTING_GRID );
	m_ctlStaticRoutingGrid.SetFont( &m_font );

	GetItemRect( 19, &rect );
	rect.bottom = rect.top + 300;
	m_ctlComboRoutingGrid.Create( WS_CHILD | WS_VISIBLE | WS_VSCROLL |
		CBS_DROPDOWNLIST, rect, this, IDC_COMBO_ROUTING_GRID );
	m_ctlComboRoutingGrid.SetFont( &m_font );

	GetItemRect( 20, &rect );
	rect.top += 5;
	rect.left += 5;
	s.LoadStringA(IDS_ToolbarAngle);
	m_ctlStaticSnapAngle.Create( s, WS_VISIBLE, rect, this, IDC_STATIC_SNAP_ANGLE );
	m_ctlStaticSnapAngle.SetFont( &m_font );

	GetItemRect( 21, &rect );
	rect.bottom = rect.top + 300;
	m_ctlComboSnapAngle.Create( WS_CHILD | WS_VISIBLE | WS_VSCROLL |
		CBS_DROPDOWNLIST, rect, this, IDC_COMBO_SNAP_ANGLE );
	m_ctlComboSnapAngle.SetFont( &m_font );

	return 0;
}

void CMyToolBar::SetLists( CArray<double> * visible, 
			  CArray<double> * placement, 
			  CArray<double> * routing,
			  double vg, double pg, double rg, int ag,
			  int units )
{
	if( units == NM || units == MM )
		m_ctlComboUnits.SetCurSel(1);
	else
		m_ctlComboUnits.SetCurSel(0);
	m_v = visible;
	m_p = placement;
	m_r = routing;
	m_ctlComboVisibleGrid.ResetContent();
	m_ctlComboPlacementGrid.ResetContent();
	m_ctlComboRoutingGrid.ResetContent();
	m_ctlComboSnapAngle.ResetContent();
	int sel = 0;
	for( int i=0; i<visible->GetSize(); i++ )
	{
		CString str;
		double val = (*visible)[i];
		MakeCStringFromGridVal(&str, val);
		m_ctlComboVisibleGrid.AddString( str );
		if (fabs(val) == vg) sel = i;									// CPT
	}
	m_ctlComboVisibleGrid.SetCurSel( sel );

	sel = 0;
	for( int i=0; i<placement->GetSize(); i++ )
	{
		CString str;
		double val = (*placement)[i];
		MakeCStringFromGridVal(&str, val); // CPT streamlined
		m_ctlComboPlacementGrid.AddString(str); 
		if (fabs(val) == pg) sel = i;
	}
	m_ctlComboPlacementGrid.SetCurSel( sel );

	if( routing != NULL )
	{
		sel = 0;
		for( int i=0; i<routing->GetSize(); i++ )
		{
			CString str;
			double val = (*routing)[i];
			MakeCStringFromGridVal(&str, val); // CPT streamlined
			m_ctlComboRoutingGrid.AddString(str); 
			if (fabs(val) == rg) sel = i;
		}
		m_ctlComboRoutingGrid.SetCurSel( sel );
	}
	else
	{
		m_ctlComboRoutingGrid.AddString( "---" );
		m_ctlComboRoutingGrid.SetCurSel(0);
	}
	// end CPT

	m_ctlComboSnapAngle.AddString( "45" );
	m_ctlComboSnapAngle.AddString( "90" );
	CString s ((LPCSTR) IDS_Off);
	m_ctlComboSnapAngle.AddString( s );
	m_ctlComboSnapAngle.SetCurSel( 0 );
}

void CMyToolBar::OnSelectVisibleGrid()
{
	CString str;

	int cur_sel = m_ctlComboVisibleGrid.GetCurSel();
	AfxGetMainWnd()->SendMessage( WM_USER_VISIBLE_GRID, WM_BY_INDEX, cur_sel );
	return;
}

void CMyToolBar::OnSelectPlacementGrid()
{
	CString str;

	int cur_sel = m_ctlComboPlacementGrid.GetCurSel();
	AfxGetMainWnd()->SendMessage( WM_USER_PLACEMENT_GRID, WM_BY_INDEX, cur_sel );
	return;
}

void CMyToolBar::OnSelectRoutingGrid()
{
	CString str;

	int cur_sel = m_ctlComboRoutingGrid.GetCurSel();
	AfxGetMainWnd()->SendMessage( WM_USER_ROUTING_GRID, WM_BY_INDEX, cur_sel );
	return;
}

void CMyToolBar::OnSelectSnapAngle()
{
	CString str;

	int cur_sel = m_ctlComboSnapAngle.GetCurSel();
	AfxGetMainWnd()->SendMessage( WM_USER_SNAP_ANGLE, WM_BY_INDEX, cur_sel );
	return;
}

void CMyToolBar::OnSelectUnits()
{
	CString str;

	int cur_sel = m_ctlComboUnits.GetCurSel();
	AfxGetMainWnd()->SendMessage( WM_USER_UNITS, WM_BY_INDEX, cur_sel );
	return;
}

void CMyToolBar::SetUnits( int units )
{
	CString str;
	int cur_sel;

	if( units == MIL )
		cur_sel = 0;
	else
		cur_sel = 1;
	m_ctlComboUnits.SetCurSel( cur_sel );	
	AfxGetMainWnd()->SendMessage( WM_USER_UNITS, WM_BY_INDEX, cur_sel );
	return;
}


// CPT:  all that follows

void CMyToolBar::RoutingGridDown() {
	int cur_sel = m_ctlComboRoutingGrid.GetCurSel();
	cur_sel++;
	m_ctlComboRoutingGrid.SetCurSel(cur_sel);
	cur_sel = m_ctlComboRoutingGrid.GetCurSel();
	AfxGetMainWnd()->SendMessage( WM_USER_ROUTING_GRID, WM_BY_INDEX, cur_sel );
	}

void CMyToolBar::RoutingGridUp() {
	int cur_sel = m_ctlComboRoutingGrid.GetCurSel();
	if (cur_sel==0) return;
	cur_sel--;
	m_ctlComboRoutingGrid.SetCurSel(cur_sel);
	cur_sel = m_ctlComboRoutingGrid.GetCurSel();
	AfxGetMainWnd()->SendMessage( WM_USER_ROUTING_GRID, WM_BY_INDEX, cur_sel );
	}

void CMyToolBar::PlacementGridDown() {
	int cur_sel = m_ctlComboPlacementGrid.GetCurSel();
	cur_sel++;
	m_ctlComboPlacementGrid.SetCurSel(cur_sel);
	cur_sel = m_ctlComboPlacementGrid.GetCurSel();
	AfxGetMainWnd()->SendMessage( WM_USER_PLACEMENT_GRID, WM_BY_INDEX, cur_sel );
	}

void CMyToolBar::PlacementGridUp() {
	int cur_sel = m_ctlComboPlacementGrid.GetCurSel();
	if (cur_sel==0) return;
	cur_sel--;
	m_ctlComboPlacementGrid.SetCurSel(cur_sel);
	cur_sel = m_ctlComboPlacementGrid.GetCurSel();
	AfxGetMainWnd()->SendMessage( WM_USER_PLACEMENT_GRID, WM_BY_INDEX, cur_sel );
	}

void CMyToolBar::VisibleGridDown() {
	int cur_sel = m_ctlComboVisibleGrid.GetCurSel();
	cur_sel++;
	m_ctlComboVisibleGrid.SetCurSel(cur_sel);
	cur_sel = m_ctlComboVisibleGrid.GetCurSel();
	AfxGetMainWnd()->SendMessage( WM_USER_VISIBLE_GRID, WM_BY_INDEX, cur_sel );
	}
 
void CMyToolBar::VisibleGridUp() {
	int cur_sel = m_ctlComboVisibleGrid.GetCurSel();
	if (cur_sel==0) return;
	cur_sel--;
	m_ctlComboVisibleGrid.SetCurSel(cur_sel);
	cur_sel = m_ctlComboVisibleGrid.GetCurSel();
	AfxGetMainWnd()->SendMessage( WM_USER_VISIBLE_GRID, WM_BY_INDEX, cur_sel );
	}

void CMyToolBar::AngleUp() {
	int cur_sel = m_ctlComboSnapAngle.GetCurSel();
	if (cur_sel==0) return;
	cur_sel--;
	m_ctlComboSnapAngle.SetCurSel(cur_sel);
	cur_sel = m_ctlComboSnapAngle.GetCurSel();
	AfxGetMainWnd()->SendMessage( WM_USER_SNAP_ANGLE, WM_BY_INDEX, cur_sel );
	}

void CMyToolBar::AngleDown() {
	int cur_sel = m_ctlComboSnapAngle.GetCurSel();
	cur_sel++;
	m_ctlComboSnapAngle.SetCurSel(cur_sel);
	cur_sel = m_ctlComboSnapAngle.GetCurSel();
	AfxGetMainWnd()->SendMessage( WM_USER_SNAP_ANGLE, WM_BY_INDEX, cur_sel );
	}

void CMyToolBar::UnitToggle(bool bShiftKeyDown, CArray<double> * visible, CArray<double> * placement, CArray<double> * routing) {
	int cur_sel = m_ctlComboUnits.GetCurSel();
	cur_sel = !cur_sel;
	m_ctlComboUnits.SetCurSel(cur_sel);
	AfxGetMainWnd()->SendMessage( WM_USER_UNITS, WM_BY_INDEX, cur_sel );
	if (!bShiftKeyDown) return;
	// In the following code we switch the three grid combo-boxes (provided shift-key was down) 
	int sz = visible->GetSize();
	double vis_sel = (*visible)[m_ctlComboVisibleGrid.GetCurSel()];
	if (cur_sel==0 && vis_sel<0) {
		// User wants mils but visible grid is in mm.  Find the closest grid val in mils if possible, and select that
		int best = -1;
		double dBest = 1000000000.;
		for (int i=0; i<sz; i++)
			if ((*visible)[i]>0 && abs((*visible)[i]+vis_sel)<dBest)
				dBest = abs((*visible)[i]+vis_sel), best = i;
		if (best>=0)
			m_ctlComboVisibleGrid.SetCurSel(best),
			AfxGetMainWnd()->SendMessage( WM_USER_VISIBLE_GRID, WM_BY_INDEX, best );
		}
	else if (cur_sel==1 && vis_sel>0) {
		// User wants mm but visible grid is in mils 
		int best = -1;
		double dBest = 1000000000.;
		for (int i=0; i<sz; i++)
			if ((*visible)[i]<0 && abs((*visible)[i]+vis_sel)<dBest)
				dBest = abs((*visible)[i]+vis_sel), best = i;
		if (best>=0)
			m_ctlComboVisibleGrid.SetCurSel(best),
			AfxGetMainWnd()->SendMessage( WM_USER_VISIBLE_GRID, WM_BY_INDEX, best );
		}

	// Analogous stuff for placement and routing grids.
	sz = placement->GetSize();
	double place_sel = (*placement)[m_ctlComboPlacementGrid.GetCurSel()];
	if (cur_sel==0 && place_sel<0) {
		int best = -1;
		double dBest = 1000000000.;
		for (int i=0; i<sz; i++)
			if ((*placement)[i]>0 && abs((*placement)[i]+place_sel)<dBest)
				dBest = abs((*placement)[i]+place_sel), best = i;
		if (best>=0)
			m_ctlComboPlacementGrid.SetCurSel(best),
			AfxGetMainWnd()->SendMessage( WM_USER_PLACEMENT_GRID, WM_BY_INDEX, best );
		}
	else if (cur_sel==1 && vis_sel>0) {
		int best = -1, sz = placement->GetSize();
		double dBest = 1000000000.;
		for (int i=0; i<sz; i++)
			if ((*placement)[i]<0 && abs((*placement)[i]+place_sel)<dBest)
				dBest = abs((*placement)[i]+place_sel), best = i;
		if (best>=0)
			m_ctlComboPlacementGrid.SetCurSel(best),
			AfxGetMainWnd()->SendMessage( WM_USER_PLACEMENT_GRID, WM_BY_INDEX, best );
		}

	if (!routing) return;
	sz = routing->GetSize();
	double route_sel = (*routing)[m_ctlComboRoutingGrid.GetCurSel()];
	if (cur_sel==0 && route_sel<0) {
		int best = -1;
		double dBest = 1000000000.;
		for (int i=0; i<sz; i++)
			if ((*routing)[i]>0 && abs((*routing)[i]+route_sel)<dBest)
				dBest = abs((*routing)[i]+route_sel), best = i;
		if (best>=0)
			m_ctlComboRoutingGrid.SetCurSel(best),
			AfxGetMainWnd()->SendMessage( WM_USER_ROUTING_GRID, WM_BY_INDEX, best );
		}
	else if (cur_sel==1 && route_sel>0) {
		int best = -1;
		double dBest = 1000000000.;
		for (int i=0; i<sz; i++)
			if ((*routing)[i]<0 && abs((*routing)[i]+route_sel)<dBest)
				dBest = abs((*routing)[i]+route_sel), best = i;
		if (best>=0)
			m_ctlComboRoutingGrid.SetCurSel(best),
			AfxGetMainWnd()->SendMessage( WM_USER_ROUTING_GRID, WM_BY_INDEX, best );
		}

	}