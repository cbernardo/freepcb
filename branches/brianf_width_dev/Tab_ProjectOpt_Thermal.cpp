// Tab_ProjectOpt_Thermal.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgProjectOptionsTabbed.h"
#include "Tab_ProjectOpt_Thermal.h"


// CTab_ProjectOpt_Thermal dialog

IMPLEMENT_DYNAMIC(CTab_ProjectOpt_Thermal, CDialog)

BEGIN_MESSAGE_MAP(CTab_ProjectOpt_Thermal, CDialog)
END_MESSAGE_MAP()

CTab_ProjectOpt_Thermal::CTab_ProjectOpt_Thermal(CWnd* pParent /*=NULL*/)
	: CTabPageSSL(CTab_ProjectOpt_Thermal::IDD, pParent)
	, m_b_TR_for_SMT_pads(0)
	, m_b_TR_for_TH_pins(0)
	, m_b_TR_for_vias(0)
	, m_b_90degree_thermals(0)
	, m_TR_line_w(10 * NM_PER_MIL)
	, m_TR_clearance(5 * NM_PER_MIL)
{
}

CTab_ProjectOpt_Thermal::~CTab_ProjectOpt_Thermal()
{
}

void CTab_ProjectOpt_Thermal::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CHECK_SMT_THERMALS, m_check_TR_for_SMT_pads);
	DDX_Control(pDX, IDC_THERMAL_PINS, m_check_TR_for_TH_pins);
	DDX_Control(pDX, IDC_THERMAL_VIAS, m_check_TR_for_vias);
	DDX_Control(pDX, IDC_CHECK_90, m_check_90degree_thermals);
	DDX_Control(pDX, IDC_EDIT_CAD_THERMALWID, m_edit_TR_line_w);
	DDX_Control(pDX, IDC_EDIT_CAD_THERMAL_CLEARANCE, m_edit_TR_clearance);
}

BOOL CTab_ProjectOpt_Thermal::OnInitPage(int nPageID)
{
	m_check_TR_for_SMT_pads.SetCheck(m_b_TR_for_SMT_pads);
	m_check_TR_for_TH_pins.SetCheck(m_b_TR_for_TH_pins);
	m_check_TR_for_vias.SetCheck(m_b_TR_for_vias);

	m_check_90degree_thermals.SetCheck(m_b_90degree_thermals);

	CString str;

	str.Format("%d", m_TR_line_w / NM_PER_MIL);
	m_edit_TR_line_w.SetWindowText(str);

	str.Format("%d", m_TR_clearance / NM_PER_MIL);
	m_edit_TR_clearance.SetWindowText(str);

	// Get info from MAIN tab - SMT connect to copper
	CDlgProjectOptionsTabbed::CTabs *pParent = static_cast<CDlgProjectOptionsTabbed::CTabs*>(GetParent());
	pParent->m_Tab_Main.OnBnClickedCheck1();

	return TRUE;
}


int CTab_ProjectOpt_Thermal::Verify()
{
	CString str;
	int val;

	m_edit_TR_line_w.GetWindowText(str);
	if ( (sscanf(str, "%d", &val) != 1) || (val < MIN_THERMAL_LINE_W) || (val > MAX_THERMAL_LINE_W))
	{
		MakePageActive();
		str.Format( "Invalid Thermal Relief Line Width (%d-%d)", MIN_THERMAL_LINE_W, MAX_THERMAL_LINE_W);
		AfxMessageBox( str );
		return 0;
	}
	m_TR_line_w = val;

	m_edit_TR_clearance.GetWindowText(str);
	if ( (sscanf(str, "%d", &val) != 1) || (val < 0) || (val > MAX_THERMAL_CLEARANCE))
	{
		MakePageActive();
		str.Format( "Invalid Thermal Relief Clearance (0-%d)", MAX_THERMAL_CLEARANCE );
		AfxMessageBox( str );
		return 0;
	}
	m_TR_clearance = val;

	return 1;
}

void CTab_ProjectOpt_Thermal::DDX_out()
{
	CDlgProjectOptionsTabbed::CTabs *pParent = static_cast<CDlgProjectOptionsTabbed::CTabs*>(GetParent());
	if( pParent->m_Tab_Main.m_bSMT_connect_copper )
	{
		m_b_TR_for_SMT_pads = m_check_TR_for_SMT_pads.GetCheck();
	}
	else
	{
		m_b_TR_for_SMT_pads = 0;
	}

	m_b_TR_for_TH_pins    = m_check_TR_for_TH_pins.GetCheck();
	m_b_TR_for_vias       = m_check_TR_for_vias.GetCheck();
	m_b_90degree_thermals = m_check_90degree_thermals.GetCheck();

	m_TR_line_w    *= NM_PER_MIL;
	m_TR_clearance *= NM_PER_MIL;
}

