// DlgMyMessageBox.cpp : implementation file
// CPT2 updated this class to be a little smarter:  Initialize() now gets passed a warning message id, and returns false if the warning has
// been disabled, so that caller won't bother with DoModal().  DoDataExchange() on exit will set the disabled flag for the warning message id.

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgMyMessageBox.h"


// CDlgMyMessageBox dialog

IMPLEMENT_DYNAMIC(CDlgMyMessageBox, CDialog)
CDlgMyMessageBox::CDlgMyMessageBox(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgMyMessageBox::IDD, pParent)
{
}

CDlgMyMessageBox::~CDlgMyMessageBox()
{
}

void CDlgMyMessageBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_MYMESSAGE, m_message);
	DDX_Control(pDX, IDC_CHECK1, m_check_dont_show);
	DDX_Control(pDX, IDOK, m_button_ok);
	DDX_Control(pDX, IDCANCEL, m_button_cancel);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_message.SetWindowText( m_mess );
		if (m_bShowCancel)
		{
			WINDOWPLACEMENT wp;
			m_button_ok.GetWindowPlacement(&wp);
			wp.rcNormalPosition.left -= 100, wp.rcNormalPosition.right -= 100;
			m_button_ok.SetWindowPlacement(&wp);
			m_button_cancel.GetWindowPlacement(&wp);
			wp.rcNormalPosition.left += 100, wp.rcNormalPosition.right += 100;
			m_button_cancel.SetWindowPlacement(&wp);
		}
		else
			m_button_cancel.ShowWindow( SW_HIDE );
		// show cursor
		::ShowCursor( TRUE );
	}
	else
	{
		// outgoing
		if (m_check_dont_show.GetCheck())
		{
			// Save change to default.cfg right away.
			extern CFreePcbApp theApp;
			extern void ReadFileLines(CString &fname, CArray<CString> &lines);							// In FreePcbDoc.cpp
			extern void WriteFileLines(CString &fname, CArray<CString> &lines);							// Ditto
			extern void ReplaceLines(CArray<CString> &oldLines, CArray<CString> &newLines, char *key);	// Ditto
			CFreePcbDoc *doc = theApp.m_doc;
			doc->m_bWarningDisable[m_warningId] = true;
			CString fn = doc->m_defaultcfg_dir + "\\default.cfg";
			CString line;
			CArray<CString> oldLines, newLines;
			for (int i=0; i<NUM_WARNINGS; i++)
				line.Format( "warning_disable: %d %d\n", i, doc->m_bWarningDisable[i]),
				newLines.Add(line);
			ReadFileLines(fn, oldLines);
			ReplaceLines(oldLines, newLines, "warning_disable");
			WriteFileLines(fn, oldLines);
		}
		::ShowCursor( FALSE );
	}
}

BEGIN_MESSAGE_MAP(CDlgMyMessageBox, CDialog)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


bool CDlgMyMessageBox::Initialize( int warningId, bool bShowCancel, int arg1, int arg2 )
{
	m_warningId = warningId;
	m_bShowCancel = bShowCancel;
	extern CFreePcbApp theApp;
	if (theApp.m_doc->m_bWarningDisable[warningId]) return false;
	CString str0 ((LPCSTR) (IDS_WarningMessage0 + warningId));
	m_mess.Format( str0, arg1, arg2 );
	return true;
}


