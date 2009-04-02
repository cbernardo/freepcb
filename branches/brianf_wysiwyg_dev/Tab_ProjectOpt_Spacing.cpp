// Tab_ProjectOpt_Spacing.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddWidth.h"
#include "DlgMyMessageBox2.h"
#include "Tab_ProjectOpt_Spacing.h"


// global callback function for sorting
//
static int CALLBACK WidthCompare( LPARAM lp1, LPARAM lp2, LPARAM type )
{
	if( lp1 == lp2 )
		return 0;
	else if( lp1 > lp2 )
		return 1;
	else
		return -1;
}


// CTab_ProjectOpt_Spacing dialog
int CTab_ProjectOpt_Spacing::m_bShowMessageForClearance = 1;

IMPLEMENT_DYNAMIC(CTab_ProjectOpt_Spacing, CDialog)

BEGIN_MESSAGE_MAP(CTab_ProjectOpt_Spacing, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, &CTab_ProjectOpt_Spacing::OnBnClickedButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, &CTab_ProjectOpt_Spacing::OnBnClickedButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CTab_ProjectOpt_Spacing::OnBnClickedButtonAdd)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_WIDTH_MENU, &CTab_ProjectOpt_Spacing::OnNMDblclkListWidthMenu)
END_MESSAGE_MAP()


CTab_ProjectOpt_Spacing::CTab_ProjectOpt_Spacing(CWnd* pParent /*=NULL*/)
	: CTabPageSSL(CTab_ProjectOpt_Spacing::IDD, pParent)
	, m_trace_w(0)
	, m_ca_clearance_trace(0)
	, m_ca_clearance_hole(0)
	, m_via_w(0)
	, m_hole_w(0)
{
}

CTab_ProjectOpt_Spacing::~CTab_ProjectOpt_Spacing()
{
}

void CTab_ProjectOpt_Spacing::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_DEF_TRACE_W, m_edit_def_tracew);
	DDX_Control(pDX, IDC_EDIT_DEF_VIA_W, m_edit_def_viapad);
	DDX_Control(pDX, IDC_EDIT_DEF_VIA_HOLE, m_edit_def_viahole);
	DDX_Control(pDX, IDC_EDIT_DEF_CA_CLEARANCE, m_edit_def_cac_trace);
	DDX_Control(pDX, IDC_EDIT_CAD_HOLE_CLEARANCE, m_edit_def_cac_holeedge);
	DDX_Control(pDX, IDC_LIST_WIDTH_MENU, m_list_menu);
}

BOOL CTab_ProjectOpt_Spacing::OnInitPage(int nPageID)
{
	CString str;

	str.Format("%d", m_trace_w / NM_PER_MIL);
	m_edit_def_tracew.SetWindowText(str);

	str.Format("%d", m_via_w / NM_PER_MIL);
	m_edit_def_viapad.SetWindowText(str);

	str.Format("%d", m_hole_w / NM_PER_MIL);
	m_edit_def_viahole.SetWindowText(str);

	str.Format("%d", m_ca_clearance_trace / NM_PER_MIL);
	m_edit_def_cac_trace.SetWindowText(str);

	str.Format("%d", m_ca_clearance_hole / NM_PER_MIL);
	m_edit_def_cac_holeedge.SetWindowText(str);

	// set up list control
	DWORD old_style = m_list_menu.GetExtendedStyle();

	m_list_menu.SetExtendedStyle( LVS_EX_FULLROWSELECT | LVS_EX_FLATSB | old_style );
	m_list_menu.InsertColumn( 0, "Trace width", LVCFMT_LEFT, 77 );
	m_list_menu.InsertColumn( 1, "Via pad width", LVCFMT_LEFT, 77 );
	m_list_menu.InsertColumn( 2, "Via hole width", LVCFMT_LEFT, 78 );

	int n = m_w->GetSize();
	for( int i=0; i<n; i++ )
	{
		int nItem = m_list_menu.InsertItem( i, "" );

		m_list_menu.SetItemData( i, (LPARAM)m_w->GetAt(i)/NM_PER_MIL );

		str.Format( "%d", m_w->GetAt(i)/NM_PER_MIL );
		m_list_menu.SetItem( i, 0, LVIF_TEXT, str, 0, 0, 0, 0 );
		str.Format( "%d", m_v_w->GetAt(i)/NM_PER_MIL );
		m_list_menu.SetItem( i, 1, LVIF_TEXT, str, 0, 0, 0, 0 );
		str.Format( "%d", m_v_h_w->GetAt(i)/NM_PER_MIL );
		m_list_menu.SetItem( i, 2, LVIF_TEXT, str, 0, 0, 0, 0 );
	}

	m_list_menu.SortItems( ::WidthCompare, 0 );

	return TRUE;
}

BOOL CTab_ProjectOpt_Spacing::Verify()
{
	CString str;

	int trace_w;
	m_edit_def_tracew.GetWindowText(str);
	if( (sscanf(str, "%d", &trace_w) != 1) || (trace_w < 1) || (trace_w > MAX_TRACE_W_MIL) )
	{
		MakePageActive();
		str.Format( "Invalid default trace width (1-%d)", MAX_TRACE_W_MIL );
		AfxMessageBox( str );
		return 0;
	}

	int via_w;
	m_edit_def_viapad.GetWindowText(str);
	if( (sscanf(str, "%d", &via_w) != 1) || (via_w < 1) || (via_w > MAX_VIA_W_MIL) )
	{
		MakePageActive();
		str.Format( "Invalid default via width (1-%d)", MAX_VIA_W_MIL );
		AfxMessageBox( str );
		return 0;
	}

	int hole_w;
	m_edit_def_viahole.GetWindowText(str);
	if( (sscanf(str, "%d", &hole_w) != 1) || (hole_w < 1) || (hole_w > MAX_VIA_HOLE_MIL) )
	{
		MakePageActive();
		str.Format( "Invalid default via hole width (1-%d)", MAX_VIA_HOLE_MIL );
		AfxMessageBox( str );
		return 0;
	}

	int ca_clearance_trace;
	m_edit_def_cac_trace.GetWindowText(str);
	if( (sscanf(str, "%d", &ca_clearance_trace) != 1) || (ca_clearance_trace < 0) || (ca_clearance_trace > MAX_CLEARANCE_MIL))
	{
		MakePageActive();
		str.Format( "Invalid default copper area to trace clearance (0-%d)", MAX_CLEARANCE_MIL );
		AfxMessageBox( str );
		return 0;
	}

	if( ca_clearance_trace == 0 && m_bShowMessageForClearance )
	{
		// warn about copper-copper clearance
		CDlgMyMessageBox2 dlg;

		MakePageActive();

		CString msg = "WARNING: You have set the trace to copper-fill clearance to 0.";
		msg += "\nThis will disable automatic generation of clearances for pads and vias in copper areas.";
		msg += "\nAre you SURE that you don't need these clearances ?";
		dlg.Initialize( &msg );
		int ret = dlg.DoModal();
		if( ret == IDCANCEL )
		{
			return 0;
		}
		else
		{
			m_bShowMessageForClearance = !dlg.bDontShowBoxState;
		}
	}

	int ca_clearance_hole;
	m_edit_def_cac_holeedge.GetWindowText(str);
	if( (sscanf(str, "%d", &ca_clearance_hole) != 1) || (ca_clearance_hole < 0) || (ca_clearance_hole > MAX_CLEARANCE_MIL))
	{
		MakePageActive();
		str.Format( "Invalid default copper area to hole clearance (0-%d)", MAX_CLEARANCE_MIL );
		AfxMessageBox( str );
		return 0;
	}

	m_trace_w = trace_w;
	m_via_w   = via_w;
	m_hole_w  = hole_w;
	m_ca_clearance_trace = ca_clearance_trace;
	m_ca_clearance_hole  = ca_clearance_hole;

	return 1;
}

void CTab_ProjectOpt_Spacing::OnDDXOut()
{
	// convert NM to MILS
	m_trace_w            *= NM_PER_MIL;
	m_ca_clearance_trace *= NM_PER_MIL;
	m_ca_clearance_hole  *= NM_PER_MIL;
	m_via_w              *= NM_PER_MIL;
	m_hole_w             *= NM_PER_MIL;

	// update trace width menu
	int n = m_list_menu.GetItemCount();
	m_w->SetSize( n );
	m_v_w->SetSize( n );
	m_v_h_w->SetSize( n );
	for( int i=0; i<n; i++ )
	{
		char str[10];
		m_list_menu.GetItemText( i, 0, str, 10 );
		m_w->SetAt(i, atoi(str)*NM_PER_MIL);
		m_list_menu.GetItemText( i, 1, str, 10 );
		m_v_w->SetAt(i, atoi(str)*NM_PER_MIL);
		m_list_menu.GetItemText( i, 2, str, 10 );
		m_v_h_w->SetAt(i, atoi(str)*NM_PER_MIL);
	}
}

// CTab_ProjectOpt_Spacing message handlers

void CTab_ProjectOpt_Spacing::OnBnClickedButtonAdd()
{
	CDlgAddWidth dlg;
	dlg.m_width = 0;
	dlg.m_via_w = 0;
	dlg.m_via_hole_w = 0;
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString str;
		m_list_menu.InsertItem( 0, "" );
		m_list_menu.SetItemData( 0, (LPARAM)dlg.m_width );
		str.Format( "%d", dlg.m_width );
		m_list_menu.SetItem( 0, 0, LVIF_TEXT, str, 0, 0, 0, 0 );
		str.Format( "%d", dlg.m_via_w );
		m_list_menu.SetItem( 0, 1, LVIF_TEXT, str, 0, 0, 0, 0 );
		str.Format( "%d", dlg.m_via_hole_w );
		m_list_menu.SetItem( 0, 2, LVIF_TEXT, str, 0, 0, 0, 0 );
		m_list_menu.SortItems( ::WidthCompare, 0 );
	}
}

void CTab_ProjectOpt_Spacing::OnBnClickedButtonEdit()
{
	POSITION pos = m_list_menu.GetFirstSelectedItemPosition();
	int i_sel = m_list_menu.GetNextSelectedItem( pos );
	if( i_sel < 0 )
	{
		AfxMessageBox( "no menu item selected" );
	}
	else
	{
		CDlgAddWidth dlg;
		char str[10];
		m_list_menu.GetItemText( i_sel, 0, str, 10 );
		dlg.m_width = atoi( str );
		m_list_menu.GetItemText( i_sel, 1, str, 10 );
		dlg.m_via_w = atoi( str );
		m_list_menu.GetItemText( i_sel, 2, str, 10 );
		dlg.m_via_hole_w = atoi( str );
		int ret = dlg.DoModal();
		if( ret == IDOK )
		{
			m_list_menu.DeleteItem( i_sel );
			CString str;
			m_list_menu.InsertItem( 0, "" );
			m_list_menu.SetItemData( 0, (LPARAM)dlg.m_width );
			str.Format( "%d", dlg.m_width );
			m_list_menu.SetItem( 0, 0, LVIF_TEXT, str, 0, 0, 0, 0 );
			str.Format( "%d", dlg.m_via_w );
			m_list_menu.SetItem( 0, 1, LVIF_TEXT, str, 0, 0, 0, 0 );
			str.Format( "%d", dlg.m_via_hole_w );
			m_list_menu.SetItem( 0, 2, LVIF_TEXT, str, 0, 0, 0, 0 );
			m_list_menu.SortItems( ::WidthCompare, 0 );
		}
	}
}

void CTab_ProjectOpt_Spacing::OnBnClickedButtonDelete()
{
	POSITION pos = m_list_menu.GetFirstSelectedItemPosition();
	int i_sel = m_list_menu.GetNextSelectedItem( pos );
	if( i_sel < 0 )
		AfxMessageBox( "no menu item selected" );
	else
		m_list_menu.DeleteItem( i_sel );
}

void CTab_ProjectOpt_Spacing::OnNMDblclkListWidthMenu(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnBnClickedButtonEdit();

	*pResult = 0;
}
