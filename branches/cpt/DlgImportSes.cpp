// DlgImportSes.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgImportSes.h"
#include ".\dlgimportses.h"

// globals to remember options
BOOL g_bSesVerbose = FALSE;

// CDlgImportSes dialog

IMPLEMENT_DYNAMIC(CDlgImportSes, CDialog)
CDlgImportSes::CDlgImportSes(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgImportSes::IDD, pParent)
	, m_bVerbose(FALSE)
{
}

CDlgImportSes::~CDlgImportSes()
{
}

void CDlgImportSes::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_SES, m_edit_ses_filepath);
	DDX_Check(pDX, IDC_CHECK_VERBOSE, m_bVerbose);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		int dot_pos = m_pcb_filepath.ReverseFind( '.' );
		if( dot_pos > 0 )
		{
			m_default_ses_filepath = m_pcb_filepath.Left( dot_pos );
			m_routed_pcb_filepath = m_default_ses_filepath + "_routed.fpc";
			m_default_ses_filepath.Append( ".ses" );
		}
		if( m_ses_filepath == "" )
			m_ses_filepath = m_default_ses_filepath;
		m_edit_ses_filepath.SetWindowText( m_ses_filepath );
	}
	else
	{
		// outgoing
		g_bSesVerbose = m_bVerbose;
		m_edit_ses_filepath.GetWindowText( m_ses_filepath );
	}
}


BEGIN_MESSAGE_MAP(CDlgImportSes, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_DEF, OnBnClickedButtonDef)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnBnClickedButtonBrowse)
END_MESSAGE_MAP()


void CDlgImportSes::Initialize( CString * ses_filepath, 
								CString * pcb_filepath )
{
	m_ses_filepath = *ses_filepath;
	m_pcb_filepath = *pcb_filepath;
	m_bVerbose = g_bSesVerbose;
}

// CDlgImportSes message handlers

void CDlgImportSes::OnBnClickedButtonDef()
{
	m_ses_filepath = m_default_ses_filepath;
	m_edit_ses_filepath.SetWindowText( m_ses_filepath );
}

void CDlgImportSes::OnBnClickedButtonBrowse()
{
	// force old-style file dialog by setting size of OPENFILENAME struct (for Win98)
	CString s ((LPCSTR) IDS_SessionFiles);
	CFileDialog dlg( 1, "ses", LPCTSTR(m_ses_filepath), 0, 
		s, NULL, OPENFILENAME_SIZE_VERSION_400 );
	dlg.AssertValid();

	// get folder of last ses file
	CString ses_folder;
	if( ses_folder != "" )
	{
		ses_folder = m_ses_filepath.Left( m_ses_filepath.ReverseFind( '\\' ) ) + "\\";
		dlg.m_ofn.lpstrInitialDir = ses_folder;
	}
	// now show dialog
	int err = dlg.DoModal(); 
	if( err == IDOK )
	{
		m_ses_filepath = dlg.GetPathName();
		m_edit_ses_filepath.SetWindowText( m_ses_filepath );
	}
}
