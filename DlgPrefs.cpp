// DlgPrefs.cpp : implementation file.  ALL CPT.

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgPrefs.h"
#include "afxdialogex.h"


// CDlgPrefs dialog
//** AMW2: wasn't working properly, fixed it

IMPLEMENT_DYNAMIC(CDlgPrefs, CDialogEx)

CDlgPrefs::CDlgPrefs(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDlgPrefs::IDD, pParent)
	{ }

CDlgPrefs::~CDlgPrefs()
	{ }

void CDlgPrefs::Init(bool bReverse, bool bLefthanded, BOOL bHighlightNet,
	int auto_interval, BOOL bAuto_Ratline_Disable, int auto_ratline_min_pins) 
{
	m_bReverse = bReverse;
	m_bLefthanded = bLefthanded;
	m_bHighlightNet = bHighlightNet;
	m_auto_interval = auto_interval;
	m_bAuto_Ratline_Disable = auto_ratline_min_pins;
	m_auto_ratline_min_pins = auto_ratline_min_pins;
}

void CDlgPrefs::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	if (!pDX->m_bSaveAndValidate) 
	{
		// incoming:  convert seconds to minutes
		m_auto_interval /= 60;
	}
	DDX_Control(pDX, IDC_REVERSE_PGUP_PGDN, m_check_reverse);
	DDX_Control(pDX, IDC_LEFTHAND_MODE, m_check_lefthanded);
	DDX_Control(pDX, IDC_CHECK_AUTO_HIGHLIGHT_NET, m_check_highlight_net);
	DDX_Control(pDX, IDC_CHECK_AUTOSAVE, m_check_autosave);
	DDX_Control(pDX, IDC_EDIT_AUTO_INTERVAL, m_edit_auto_interval);
	DDX_Text(pDX, IDC_EDIT_AUTO_INTERVAL, m_auto_interval );
	DDX_Control(pDX, IDC_CHECK_AUTORAT_DISABLE, m_check_disable_auto_rats);
	DDX_Control(pDX, IDC_EDIT_MIN_PINS, m_edit_min_pins);
	DDX_Text(pDX, IDC_EDIT_MIN_PINS, m_auto_ratline_min_pins ),
	DDV_MinMaxInt(pDX, m_auto_ratline_min_pins, 0, 10000 );

	if(pDX->m_bSaveAndValidate)  
	{ 
		// outgoing
		m_bReverse = m_check_reverse.GetCheck();
		m_bLefthanded = m_check_lefthanded.GetCheck();
		m_bAuto_Ratline_Disable = m_check_disable_auto_rats.GetCheck();
		m_bHighlightNet = m_check_highlight_net.GetCheck();
		// convert minutes to seconds
		m_auto_interval *= 60;
	}
}


BEGIN_MESSAGE_MAP(CDlgPrefs, CDialogEx)
	ON_BN_CLICKED(IDC_CHECK_AUTOSAVE, OnBnClickedCheckAutosave)
	ON_BN_CLICKED(IDC_CHECK_AUTORAT_DISABLE, OnBnClickedCheckAutoRatDisable)
END_MESSAGE_MAP()


// CDlgPrefs message handlers

BOOL CDlgPrefs::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_check_reverse.SetCheck(m_bReverse);
	m_check_lefthanded.SetCheck(m_bLefthanded);
	m_check_highlight_net.SetCheck(m_bHighlightNet);
	m_edit_auto_interval.EnableWindow( m_auto_interval );
	m_check_autosave.SetCheck( m_auto_interval );
	m_edit_min_pins.EnableWindow( m_auto_ratline_min_pins );
	m_check_disable_auto_rats.SetCheck( m_auto_ratline_min_pins );
	return TRUE;
}

void CDlgPrefs::OnBnClickedCheckAutosave()
{
	if( m_check_autosave.GetCheck() )
	{
		m_edit_auto_interval.EnableWindow( TRUE );
		m_edit_auto_interval.SetWindowText( "10" );
	}
	else
	{
		m_edit_auto_interval.EnableWindow( FALSE );
		m_edit_auto_interval.SetWindowText( "0" );
	}
}

void CDlgPrefs::OnBnClickedCheckAutoRatDisable()
{
	if( m_check_disable_auto_rats.GetCheck() )
	{
		m_edit_min_pins.EnableWindow( TRUE );
		m_edit_min_pins.SetWindowText( "100" );
	}
	else
	{
		m_edit_min_pins.EnableWindow( FALSE );
		m_edit_min_pins.SetWindowText( "0" );
	}
}

