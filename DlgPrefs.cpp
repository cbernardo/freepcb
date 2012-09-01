// DlgPrefs.cpp : implementation file.  ALL CPT.

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgPrefs.h"
#include "afxdialogex.h"
#include "PathDialog.h"


// CDlgPrefs dialog

IMPLEMENT_DYNAMIC(CDlgPrefs, CDialogEx)

CDlgPrefs::CDlgPrefs(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDlgPrefs::IDD, pParent)
	{ }

CDlgPrefs::~CDlgPrefs()
	{ }

void CDlgPrefs::Init(bool bReverse, bool bLefthanded, bool bHighlightNet, bool bErrorSound,
	int auto_interval, bool bAuto_Ratline_Disable, int auto_ratline_min_pins) 
{
	extern CFreePcbApp theApp;
	doc = theApp.m_doc;
	m_bReverse = bReverse;
	m_bLefthanded = bLefthanded;
	m_bHighlightNet = bHighlightNet;
	m_bErrorSound = bErrorSound;
	m_auto_interval = auto_interval;
	m_bAuto_Ratline_Disable = bAuto_Ratline_Disable;
	m_auto_ratline_min_pins = auto_ratline_min_pins;
}

void CDlgPrefs::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REVERSE_PGUP_PGDN, m_check_reverse);
	DDX_Control(pDX, IDC_LEFTHAND_MODE, m_check_lefthanded);
	DDX_Control(pDX, IDC_CHECK_AUTO_HIGHLIGHT_NET, m_check_highlight_net);
	DDX_Control(pDX, IDC_ERROR_SOUND_ENABLED, m_check_error_sound);
	DDX_Control(pDX, IDC_CHECK_AUTOSAVE, m_check_autosave);
	DDX_Control(pDX, IDC_EDIT_AUTO_INTERVAL, m_edit_auto_interval);
	DDX_Control(pDX, IDC_CHECK_AUTORAT_DISABLE, m_check_disable_auto_rats);
	DDX_Control(pDX, IDC_EDIT_MIN_PINS, m_edit_min_pins);
	DDX_Control(pDX, IDC_EDIT_PREFS_DEFAULTCFG, m_edit_defaultcfg);
	DDX_Control(pDX, IDC_STATIC_WARNINGS, m_static_warnings);
	DDX_Control(pDX, IDC_REINSTATE_WARNINGS, m_check_warnings);

	if (!pDX->m_bSaveAndValidate) 
	{
		// incoming:  convert seconds to minutes
		m_auto_interval /= 60;
		DDX_Text(pDX, IDC_EDIT_AUTO_INTERVAL, m_auto_interval );
		// Set the caption for m_static_warnings, to give user a sense of which warnings have been turned off in the past
		CString shortWarning1 (""), shortWarning2 (""), caption ("");
		CString messagesAbout ((LPCSTR) IDS_MessagesAbout);
		bool bMore = false;
		for (int i=0; i<NUM_WARNINGS; i++)
		{
			if (!doc->m_bWarningDisable[i])
				continue;
			if (shortWarning1=="")
				shortWarning1.LoadStringA(IDS_ShortWarning0 + i);
			else if (shortWarning2=="")
			{
				shortWarning2.LoadStringA(IDS_ShortWarning0 + i);
				if (shortWarning2 == shortWarning1)
					shortWarning2 = "";
			}
			else
				{ bMore = true; break; }
		}
		if (shortWarning1=="")
			m_check_warnings.EnableWindow(false);
		else if (shortWarning2=="")
			caption = messagesAbout + shortWarning1 + ")";
		else if (!bMore)
			caption = messagesAbout + shortWarning1 + "; " + shortWarning2 + ")";
		else
			caption = messagesAbout + shortWarning1 + "; " + shortWarning2 + "; ...)";
		m_static_warnings.SetWindowTextA( caption );
	}

	else 
	{ 
		// outgoing
		if (m_check_disable_auto_rats.GetCheck()) 
			DDX_Text(pDX, IDC_EDIT_MIN_PINS, m_auto_ratline_min_pins ),
			DDV_MinMaxInt(pDX, m_auto_ratline_min_pins, 0, 10000 );
		m_bReverse = m_check_reverse.GetCheck();
		m_bLefthanded = m_check_lefthanded.GetCheck();
		m_bAuto_Ratline_Disable = m_check_disable_auto_rats.GetCheck();
		m_bErrorSound = m_check_error_sound.GetCheck();
		m_bHighlightNet = m_check_highlight_net.GetCheck();
		m_bReinstateWarnings = m_check_warnings.GetCheck();
		DDX_Text(pDX, IDC_EDIT_AUTO_INTERVAL, m_auto_interval );
		m_auto_interval *= 60;															// convert minutes to seconds
		m_edit_defaultcfg.GetWindowTextA(m_defaultcfg_dir);
		if (m_defaultcfg_dir.Right(1)=="\\")
			m_defaultcfg_dir.Left( m_defaultcfg_dir.GetLength()-1 );
		HANDLE h = CreateFile(m_defaultcfg_dir + "\\default.cfg", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (h == INVALID_HANDLE_VALUE)
		{
			CString str, str0 ((LPCSTR) IDS_CantWriteToFolder);
			str.Format(str0, m_defaultcfg_dir);
			AfxMessageBox(str);
			pDX->Fail();
		}
		else
			CloseHandle(h);
	}
}


BEGIN_MESSAGE_MAP(CDlgPrefs, CDialogEx)
	ON_BN_CLICKED(IDC_CHECK_AUTOSAVE, OnBnClickedCheckAutosave)
	ON_BN_CLICKED(IDC_CHECK_AUTORAT_DISABLE, OnBnClickedCheckAutoRatDisable)
	ON_EN_CHANGE(IDC_EDIT_AUTO_INTERVAL, OnEnChangeEditAutoInterval)
	ON_BN_CLICKED(IDC_BUTTON_PREFS_BROWSE, OnBnClickedBrowse)
END_MESSAGE_MAP()


// CDlgPrefs message handlers

BOOL CDlgPrefs::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_check_reverse.SetCheck(m_bReverse);
	m_check_lefthanded.SetCheck(m_bLefthanded);
	m_check_highlight_net.SetCheck(m_bHighlightNet);
	m_check_error_sound.SetCheck(m_bErrorSound);
	if( !m_auto_interval )
	{
		m_edit_auto_interval.EnableWindow( FALSE );
		m_check_autosave.SetCheck(0);
	}
	else
		m_check_autosave.SetCheck(1);

	m_check_disable_auto_rats.SetCheck( m_bAuto_Ratline_Disable );
	m_edit_min_pins.EnableWindow( m_bAuto_Ratline_Disable );
	extern CFreePcbApp theApp;
	CString dir = theApp.GetProfileString(_T("Settings"),_T("DefaultCfgDir"));
	if (dir == "")
		dir = doc->m_app_dir;
	m_edit_defaultcfg.SetWindowTextA( dir );
	return TRUE;
}

void CDlgPrefs::OnBnClickedCheckAutosave()
{
	if( m_check_autosave.GetCheck() )
		m_edit_auto_interval.EnableWindow( TRUE );
	else
	{
		m_edit_auto_interval.EnableWindow( FALSE );
		m_edit_auto_interval.SetWindowText( "0" );
	}
}

void CDlgPrefs::OnBnClickedCheckAutoRatDisable()
{
	m_edit_min_pins.EnableWindow( m_check_disable_auto_rats.GetCheck() );
}



void CDlgPrefs::OnEnChangeEditAutoInterval()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}

void CDlgPrefs::OnBnClickedBrowse()
{
	CString open ((LPCSTR) IDS_OpenFolder), select ((LPCSTR) IDS_SelectFolderForDefaultCfg);
	CString path_str;
	m_edit_defaultcfg.GetWindowTextA( path_str );
	CPathDialog dlg( open, select, path_str );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		path_str = dlg.GetPathName();
		m_edit_defaultcfg.SetWindowText( path_str );
	}
}
