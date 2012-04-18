// DlgProjectOptions.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgProjectOptions.h"
#include "DlgAddWidth.h"
#include "PathDialog.h"
#include ".\dlgprojectoptions.h"

// global callback function for sorting
//		
int CALLBACK WidthCompare( LPARAM lp1, LPARAM lp2, LPARAM type )
{
	if( lp1 == lp2 )
		return 0;
	else if( lp1 > lp2 )
		return 1;
	else
		return -1;
}

// CDlgProjectOptions dialog

IMPLEMENT_DYNAMIC(CDlgProjectOptions, CDialog)
CDlgProjectOptions::CDlgProjectOptions(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgProjectOptions::IDD, pParent)
	, m_trace_w(0)
	, m_via_w(0)
	, m_hole_w(0)
	, m_layers(0)
{
	m_folder_changed = FALSE;
	m_folder_has_focus = FALSE;
}

CDlgProjectOptions::~CDlgProjectOptions()
{
}

void CDlgProjectOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	if( !pDX->m_bSaveAndValidate )
	{
		// convert NM to MILS
		m_glue_w = m_glue_w/NM_PER_MIL;
		m_trace_w = m_trace_w/NM_PER_MIL;
		m_via_w = m_via_w/NM_PER_MIL;
		m_hole_w = m_hole_w/NM_PER_MIL;
		// convert seconds to minutes
		m_auto_interval = m_auto_interval/60;
	}
	DDX_Control(pDX, IDC_EDIT_NAME, m_edit_name);
	DDX_Text(pDX, IDC_EDIT_NAME, m_name );
	DDX_Control(pDX, IDC_EDIT_FOLDER, m_edit_folder);
	DDX_Text(pDX, IDC_EDIT_FOLDER, m_path_to_folder );
	DDX_Control(pDX, IDC_EDIT_LIBRARY_FOLDER, m_edit_lib_folder);
	DDX_Text(pDX, IDC_EDIT_LIBRARY_FOLDER, m_lib_folder );
	DDX_Control(pDX, IDC_EDIT_NUM_LAYERS, m_edit_layers );
	DDX_Text(pDX, IDC_EDIT_NUM_LAYERS, m_layers );
	DDV_MinMaxInt(pDX, m_layers, 1, 16 );
	DDX_Text(pDX, IDC_EDIT_GLUE_W, m_glue_w ); 
	DDV_MinMaxInt(pDX, m_glue_w, 1, 1000 );
	DDX_Text(pDX, IDC_EDIT_DEF_TRACE_W, m_trace_w ); 
	DDV_MinMaxInt(pDX, m_trace_w, 1, 1000 );
	DDX_Text(pDX, IDC_EDIT_DEF_VIA_W, m_via_w );
	DDV_MinMaxInt(pDX, m_via_w, 1, 1000 );
	DDX_Text(pDX, IDC_EDIT_DEF_VIA_HOLE, m_hole_w );
	DDV_MinMaxInt(pDX, m_hole_w, 1, 1000 );
	DDX_Control(pDX, IDC_LIST_WIDTH_MENU, m_list_menu);
	DDX_Control(pDX, IDC_CHECK_AUTOSAVE, m_check_autosave);
	DDX_Control(pDX, IDC_EDIT_AUTO_INTERVAL, m_edit_auto_interval);
	DDX_Text(pDX, IDC_EDIT_AUTO_INTERVAL, m_auto_interval );
	DDX_Control(pDX, IDC_CHECK1, m_check_SMT_connect);
	DDX_Control(pDX, IDC_CHECK_AUTORAT_DISABLE, m_check_disable_auto_rats);
	DDX_Control(pDX, IDC_EDIT_MIN_PINS, m_edit_min_pins);
	DDX_Text(pDX, IDC_EDIT_MIN_PINS, m_auto_ratline_min_pins );
	DDV_MinMaxInt(pDX, m_auto_ratline_min_pins, 0, 10000 );

	if( pDX->m_bSaveAndValidate )
	{
		// leaving dialog
		if( m_name.GetLength() == 0 )
		{
			pDX->PrepareEditCtrl( IDC_EDIT_NAME );
			AfxMessageBox( "Please enter name for project" );
			pDX->Fail();
		}
		else if( m_path_to_folder.GetLength() == 0 )
		{
			pDX->PrepareEditCtrl( IDC_EDIT_FOLDER );
			AfxMessageBox( "Please enter project folder" );
			pDX->Fail();
		}
		else if( m_lib_folder.GetLength() == 0 )
		{
			pDX->PrepareEditCtrl( IDC_EDIT_LIBRARY_FOLDER );
			AfxMessageBox( "Please enter library folder" );
			pDX->Fail();
		}
		else
		{
			// save options
			m_bSMT_connect_copper = m_check_SMT_connect.GetCheck();
			m_bAuto_Ratline_Disable = m_check_disable_auto_rats.GetCheck();

			// convert minutes to seconds
			m_auto_interval *= 60;

			// convert NM to MILS
			m_trace_w = m_trace_w*NM_PER_MIL;
			m_via_w = m_via_w*NM_PER_MIL;
			m_hole_w = m_hole_w*NM_PER_MIL;
			m_glue_w = m_glue_w*NM_PER_MIL;

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
	}
}


BEGIN_MESSAGE_MAP(CDlgProjectOptions, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnBnClickedButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnBnClickedButtonDelete)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnEnChangeEditName)
	ON_EN_CHANGE(IDC_EDIT_FOLDER, OnEnChangeEditFolder)
	ON_EN_SETFOCUS(IDC_EDIT_FOLDER, OnEnSetfocusEditFolder)
	ON_EN_KILLFOCUS(IDC_EDIT_FOLDER, OnEnKillfocusEditFolder)
	ON_BN_CLICKED(IDC_CHECK_AUTOSAVE, OnBnClickedCheckAutosave)
	ON_BN_CLICKED(IDC_BUTTON_LIB, OnBnClickedButtonLib)
	ON_BN_CLICKED(IDC_CHECK_AUTORAT_DISABLE, OnBnClickedCheckAutoRatDisable)
END_MESSAGE_MAP()

// initialize data
//
void CDlgProjectOptions::Init( BOOL new_project,
							  CString * name,
							  CString * path_to_folder,
							  CString * lib_folder,
							  int num_layers,
							  BOOL bSMT_connect_copper,
							  int glue_w,
							  int trace_w,
							  int via_w,
							  int hole_w,
							  int auto_interval,
							  BOOL bAuto_Ratline_Disable,
							  int auto_ratline_min_pins,
							  CArray<int> * w,
							  CArray<int> * v_w,
							  CArray<int> * v_h_w )
{
	m_new_project = new_project;
	m_name = *name;
	m_path_to_folder = *path_to_folder;
	m_lib_folder = *lib_folder;
	m_layers = num_layers;
	m_bSMT_connect_copper = bSMT_connect_copper;
	m_glue_w = glue_w;
	m_trace_w = trace_w;
	m_via_w = via_w;
	m_hole_w = hole_w;
	m_auto_interval = auto_interval;
	m_bAuto_Ratline_Disable = bAuto_Ratline_Disable;
	m_auto_ratline_min_pins = auto_ratline_min_pins;
	m_w = w;
	m_v_w = v_w;
	m_v_h_w = v_h_w;
}

BOOL CDlgProjectOptions::OnInitDialog()
{
	CDialog::OnInitDialog();

	// initialize strings
	m_edit_folder.SetWindowText( m_path_to_folder );
	// set up list control
	DWORD old_style = m_list_menu.GetExtendedStyle();
	m_list_menu.SetExtendedStyle( LVS_EX_FULLROWSELECT | LVS_EX_FLATSB | old_style );
	m_list_menu.InsertColumn( 0, "Trace width", LVCFMT_LEFT, 77 );
	m_list_menu.InsertColumn( 1, "Via pad width", LVCFMT_LEFT, 77 );
	m_list_menu.InsertColumn( 2, "Via hole width", LVCFMT_LEFT, 78 );
	CString str;
	int n = m_w->GetSize();
	for( int i=0; i<n; i++ )
	{
		int nItem = m_list_menu.InsertItem( i, "" );
		m_list_menu.SetItemData( i, (LPARAM)m_w->GetAt(i) );
		str.Format( "%d", m_w->GetAt(i)/NM_PER_MIL );
		m_list_menu.SetItem( i, 0, LVIF_TEXT, str, 0, 0, 0, 0 );
		str.Format( "%d", m_v_w->GetAt(i)/NM_PER_MIL );
		m_list_menu.SetItem( i, 1, LVIF_TEXT, str, 0, 0, 0, 0 );
		str.Format( "%d", m_v_h_w->GetAt(i)/NM_PER_MIL );
		m_list_menu.SetItem( i, 2, LVIF_TEXT, str, 0, 0, 0, 0 );
	}
	if( !m_new_project )
	{
		// disable some fields for existing project
		m_edit_folder.EnableWindow( FALSE );
	}
	if( !m_auto_interval )
	{
		m_edit_auto_interval.EnableWindow( FALSE );
		m_check_autosave.SetCheck(0);
	}
	else
		m_check_autosave.SetCheck(1);

	m_check_disable_auto_rats.SetCheck( m_bAuto_Ratline_Disable );
	m_edit_min_pins.EnableWindow( m_bAuto_Ratline_Disable );
	m_check_SMT_connect.SetCheck( m_bSMT_connect_copper );
	return TRUE;
}

// CDlgProjectOptions message handlers

void CDlgProjectOptions::OnBnClickedButtonAdd()
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

void CDlgProjectOptions::OnBnClickedButtonEdit()
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

void CDlgProjectOptions::OnBnClickedButtonDelete()
{
	POSITION pos = m_list_menu.GetFirstSelectedItemPosition();
	int i_sel = m_list_menu.GetNextSelectedItem( pos );
	if( i_sel < 0 )
		AfxMessageBox( "no menu item selected" );
	else
		m_list_menu.DeleteItem( i_sel );
}

void CDlgProjectOptions::OnEnChangeEditName()
{
	CString str;
	m_edit_name.GetWindowText( str ); 
	if( m_new_project == TRUE && m_folder_changed == FALSE )
		m_edit_folder.SetWindowText( m_path_to_folder + str );
}

void CDlgProjectOptions::OnEnChangeEditFolder()
{
	if( m_folder_has_focus )
		m_folder_changed = TRUE;
}

void CDlgProjectOptions::OnEnSetfocusEditFolder()
{
	m_folder_has_focus = TRUE;
}

void CDlgProjectOptions::OnEnKillfocusEditFolder()
{
	m_folder_has_focus = FALSE;
}

void CDlgProjectOptions::OnBnClickedCheckAutosave()
{
	if( m_check_autosave.GetCheck() )
		m_edit_auto_interval.EnableWindow( TRUE );
	else
	{
		m_edit_auto_interval.EnableWindow( FALSE );
		m_edit_auto_interval.SetWindowText( "0" );
	}
}

void CDlgProjectOptions::OnBnClickedButtonLib()
{
	CPathDialog dlg( "Library Folder", "Select default library folder", m_lib_folder );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_lib_folder = dlg.GetPathName();
		m_edit_lib_folder.SetWindowText( m_lib_folder );
	}
}

void CDlgProjectOptions::OnBnClickedCheckAutoRatDisable()
{
	m_edit_min_pins.EnableWindow( m_check_disable_auto_rats.GetCheck() );
}
