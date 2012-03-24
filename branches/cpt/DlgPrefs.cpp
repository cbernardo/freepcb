// DlgPrefs.cpp : implementation file.  ALL CPT.

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgPrefs.h"
#include "afxdialogex.h"


// CDlgPrefs dialog

IMPLEMENT_DYNAMIC(CDlgPrefs, CDialogEx)

CDlgPrefs::CDlgPrefs(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDlgPrefs::IDD, pParent)
{

}

CDlgPrefs::~CDlgPrefs()
{
}

void CDlgPrefs::Init(bool bReverse, int auto_interval,	BOOL bAuto_Ratline_Disable,	int auto_ratline_min_pins) {
	m_bReverse = bReverse;
	m_auto_interval = auto_interval;
	m_bAuto_Ratline_Disable = bAuto_Ratline_Disable;
	m_auto_ratline_min_pins = auto_ratline_min_pins;
	}

void CDlgPrefs::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	if (!pDX->m_bSaveAndValidate) 
		// incoming:  convert seconds to minutes
		m_auto_interval /= 60;
	DDX_Control(pDX, IDC_REVERSE_PGUP_PGDN, m_check_reverse);
	DDX_Control(pDX, IDC_CHECK_AUTOSAVE, m_check_autosave);
	DDX_Control(pDX, IDC_EDIT_AUTO_INTERVAL, m_edit_auto_interval);
	DDX_Text(pDX, IDC_EDIT_AUTO_INTERVAL, m_auto_interval );
	DDX_Control(pDX, IDC_CHECK_AUTORAT_DISABLE, m_check_disable_auto_rats);
	DDX_Control(pDX, IDC_EDIT_MIN_PINS, m_edit_min_pins);
	if (m_check_disable_auto_rats.GetCheck()) 
		DDX_Text(pDX, IDC_EDIT_MIN_PINS, m_auto_ratline_min_pins ),
		DDV_MinMaxInt(pDX, m_auto_ratline_min_pins, 0, 10000 );

	if(pDX->m_bSaveAndValidate)  { 
		// outgoing
		m_bReverse = m_check_reverse.GetCheck();
		m_bAuto_Ratline_Disable = m_check_disable_auto_rats.GetCheck();
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
	if( !m_auto_interval )
	{
		m_edit_auto_interval.EnableWindow( FALSE );
		m_check_autosave.SetCheck(0);
	}
	else
		m_check_autosave.SetCheck(1);

	m_check_disable_auto_rats.SetCheck( m_bAuto_Ratline_Disable );
	m_edit_min_pins.EnableWindow( m_bAuto_Ratline_Disable );
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

