// Tab_ProjectOpt_Main.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "PathDialog.h"
#include "Tab_ProjectOpt_Main.h"
#include "DlgProjectOptionsTabbed.h"


// CTab_ProjectOpt_Main dialog

IMPLEMENT_DYNAMIC(CTab_ProjectOpt_Main, CDialog)

BEGIN_MESSAGE_MAP(CTab_ProjectOpt_Main, CDialog)
	ON_EN_CHANGE(IDC_EDIT_NAME, &CTab_ProjectOpt_Main::OnEnChangeEditName)
	ON_EN_CHANGE(IDC_EDIT_FOLDER, &CTab_ProjectOpt_Main::OnEnChangeEditFolder)
	ON_EN_KILLFOCUS(IDC_EDIT_FOLDER, &CTab_ProjectOpt_Main::OnEnKillfocusEditFolder)
	ON_EN_SETFOCUS(IDC_EDIT_FOLDER, &CTab_ProjectOpt_Main::OnEnSetfocusEditFolder)
	ON_BN_CLICKED(IDC_BUTTON_LIB, &CTab_ProjectOpt_Main::OnBnClickedButtonLib)
	ON_BN_CLICKED(IDC_CHECK_AUTOSAVE, &CTab_ProjectOpt_Main::OnBnClickedCheckAutosave)
	ON_BN_CLICKED(IDC_CHECK1, &CTab_ProjectOpt_Main::OnBnClickedCheck1)
END_MESSAGE_MAP()


CTab_ProjectOpt_Main::CTab_ProjectOpt_Main(CWnd* pParent /*=NULL*/)
	: CTabPageSSL(CTab_ProjectOpt_Main::IDD, pParent)
	, m_layers(10)
{
	m_folder_changed = FALSE;
	m_folder_has_focus = FALSE;
}

CTab_ProjectOpt_Main::~CTab_ProjectOpt_Main()
{
}


void CTab_ProjectOpt_Main::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT_NAME, m_edit_name);
	DDX_Control(pDX, IDC_EDIT_FOLDER, m_edit_folder);
	DDX_Control(pDX, IDC_EDIT_LIBRARY_FOLDER, m_edit_lib_folder);
	DDX_Control(pDX, IDC_CHECK1, m_check_SMT_connect_copper);
	DDX_Control(pDX, IDC_EDIT_NUM_LAYERS, m_edit_layers);
	DDX_Control(pDX, IDC_EDIT_GLUE_W, m_edit_glue_w);
	DDX_Control(pDX, IDC_CHECK_AUTOSAVE, m_check_autosave);
	DDX_Control(pDX, IDC_EDIT_AUTO_INTERVAL, m_edit_autosave_interval);
	DDX_Control(pDX, IDC_EDIT_RATLINE_WIDTH, m_edit_ratline_w);
}


BOOL CTab_ProjectOpt_Main::OnInitPage(int nPageID)
{
	m_edit_name.SetWindowText(m_name);
	m_edit_folder.SetWindowText(m_path_to_folder);
    m_edit_lib_folder.SetWindowText(m_lib_folder);

	CString str;

	// Layers
	str.Format("%d", m_layers);
	m_edit_layers.SetWindowText(str);

	// Layers
	m_ratline_w /= NM_PER_MIL;
	str.Format("%d", m_ratline_w);
	m_edit_ratline_w.SetWindowText(str);

	// Adhesive Dot
	// convert NM to MILS
	m_glue_w /= NM_PER_MIL;
	str.Format("%d", m_glue_w);
	m_edit_glue_w.SetWindowText(str);

	// SMT connect to copper
	m_check_SMT_connect_copper.SetCheck( m_bSMT_connect_copper );

	// convert seconds to minutes
	m_auto_interval = m_auto_interval/60;
	str.Format("%d", m_auto_interval);
	m_edit_autosave_interval.SetWindowText(str);

	if( m_auto_interval == 0)
	{
		m_edit_autosave_interval.EnableWindow( FALSE );
		m_check_autosave.SetCheck(0);
	}
	else
	{
		m_check_autosave.SetCheck(1);
	}

	CDlgProjectOptionsTabbed::CTabs *pParent = static_cast<CDlgProjectOptionsTabbed::CTabs*>(GetParent());
	if( !pParent->m_new_project )
	{
		// disable some fields for existing project
		m_edit_folder.EnableWindow( FALSE );
	}

    UpdateProjectFolder();
	OnBnClickedCheck1();

	return TRUE;
}


int CTab_ProjectOpt_Main::Verify()
{
	// leaving dialog
	if( m_name.GetLength() == 0 )
	{
		MakePageActive();
		AfxMessageBox( "Please enter name for project" );
		return 0;
	}

	CString path_to_folder;
	CString lib_folder;

	m_edit_folder.GetWindowText(path_to_folder);
	m_edit_lib_folder.GetWindowText(lib_folder);

	if( path_to_folder.GetLength() == 0 )
	{
		MakePageActive();
		AfxMessageBox( "Please enter project folder" );
		return 0;
	}
	
	if( lib_folder.GetLength() == 0 )
	{
		MakePageActive();
		AfxMessageBox( "Please enter library folder" );
		return 0;
	}

	CString str;
	int val;

	m_edit_layers.GetWindowText(str);
	if ( (sscanf(str, "%d", &val) != 1) || (val < 1) || (val > 16))
	{
		MakePageActive();
		AfxMessageBox( "Invalid number of layers (1-16)" );
		return 0;
	}
	m_layers = val;

	m_edit_ratline_w.GetWindowText(str);
	if ( (sscanf(str, "%d", &val) != 1) || (val < 0) || (val > MAX_RATLINE_W_MIL))
	{
		MakePageActive();
		str.Format( "Invalid Ratline Width (0-%d)", MAX_RATLINE_W_MIL );
		AfxMessageBox( str );
		return 0;
	}
	m_ratline_w = val;

	m_edit_glue_w.GetWindowText(str);
	if ( (sscanf(str, "%d", &val) != 1) || (val < 1) || (val > 1000))
	{
		MakePageActive();
		AfxMessageBox( "Invalid Adhesive size" );
		return 0;
	}
	m_glue_w = val;

	m_edit_autosave_interval.GetWindowText(str);
	if (sscanf(str, "%d", &val) != 1)
	{
		MakePageActive();
		AfxMessageBox( "Invalid Autosave interval" );
		return 0;
	}
	m_auto_interval = val;

	return 1;
}

void CTab_ProjectOpt_Main::DDX_out()
{
	m_edit_folder.GetWindowText(m_path_to_folder);
	m_edit_lib_folder.GetWindowText(m_lib_folder);

	m_ratline_w *= NM_PER_MIL;
	m_glue_w *= NM_PER_MIL;

	// save options
	m_bSMT_connect_copper = m_check_SMT_connect_copper.GetCheck();

	// convert minutes to seconds
	m_auto_interval *= 60;
}



// CTab_ProjectOpt_Main message handlers
void CTab_ProjectOpt_Main::UpdateProjectFolder()
{
	CDlgProjectOptionsTabbed::CTabs *pParent = static_cast<CDlgProjectOptionsTabbed::CTabs*>(GetParent());

	if( pParent->m_new_project && m_folder_changed == FALSE )
    {
		m_edit_folder.SetWindowText( m_path_to_folder + m_name );
    }
}


void CTab_ProjectOpt_Main::OnEnChangeEditName()
{
	m_edit_name.GetWindowText(m_name);

    UpdateProjectFolder();
}

void CTab_ProjectOpt_Main::OnEnChangeEditFolder()
{
	if( m_folder_has_focus )
		m_folder_changed = TRUE;
}

void CTab_ProjectOpt_Main::OnEnKillfocusEditFolder()
{
	m_folder_has_focus = FALSE;
}

void CTab_ProjectOpt_Main::OnEnSetfocusEditFolder()
{
	m_folder_has_focus = TRUE;
}

void CTab_ProjectOpt_Main::OnBnClickedButtonLib()
{
	CPathDialog dlg( "Library Folder", "Select default library folder", m_lib_folder );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_lib_folder = dlg.GetPathName();
		m_edit_lib_folder.SetWindowText( m_lib_folder );
	}
}


void CTab_ProjectOpt_Main::OnBnClickedCheckAutosave()
{
	if( m_check_autosave.GetCheck() )
		m_edit_autosave_interval.EnableWindow( TRUE );
	else
	{
		m_edit_autosave_interval.EnableWindow( FALSE );
		m_edit_autosave_interval.SetWindowText( "0" );
	}
}

void CTab_ProjectOpt_Main::OnBnClickedCheck1()
{
	CDlgProjectOptionsTabbed::CTabs *pParent = static_cast<CDlgProjectOptionsTabbed::CTabs*>(GetParent());

	pParent->m_Tab_Thermal.m_check_TR_for_SMT_pads.EnableWindow( m_check_SMT_connect_copper.GetCheck() ); //BAF need value from dialog, not value passed in
}
