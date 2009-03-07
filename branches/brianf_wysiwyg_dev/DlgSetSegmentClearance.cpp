// DlgSetSegmentClearance.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgSetSegmentClearance.h"


// DlgSetSegmentClearance dialog

IMPLEMENT_DYNAMIC(DlgSetSegmentClearance, CDialog)

DlgSetSegmentClearance::DlgSetSegmentClearance(CWnd* pParent /*=NULL*/)
	: CDialog(DlgSetSegmentClearance::IDD, pParent)
{

}

DlgSetSegmentClearance::~DlgSetSegmentClearance()
{
}

void DlgSetSegmentClearance::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_RADIO_USE_NET_CLEARANCE, m_radio1_use_net_default);
	DDX_Control(pDX, IDC_RADIO_SET_TRACE_CLEARANCE, m_radio1_set_trace_clearance);

	DDX_Control(pDX, IDC_EDIT_CLEARANCE, m_edit_clearance);
	DDX_Control(pDX, IDC_DEF_NET, m_check_def_net);

	DDX_Control(pDX, IDC_APPLY_SEG, m_radio2_apply_seg);
	DDX_Control(pDX, IDC_APPLY_CON, m_radio2_apply_con);
	DDX_Control(pDX, IDC_APPLY_NET, m_radio2_apply_net);

	DDX_Control(pDX, IDC_APPLY_PINVIA_NO,     m_radio3_no);
	DDX_Control(pDX, IDC_APPLY_PINVIA_AUTO,   m_radio3_auto);
	DDX_Control(pDX, IDC_APPLY_PINVIA_AS_SEL, m_radio3_as_sel);

	if (pDX->m_bSaveAndValidate)
	{
		int val;
		if( m_radio1_use_net_default.GetCheck() )
		{
			val = CClearanceInfo::E_USE_PARENT;
		}
		else
		{
			CString str;
			m_edit_clearance.GetWindowText(str);
			if ( (sscanf(str, "%d", &val) != 1) || (val < 0) || (val > MAX_CLEARANCE_MIL) )
			{
				str.Format("Invalid clearance value (0-%d)", MAX_CLEARANCE_MIL);
				AfxMessageBox( str );
				pDX->Fail();
				return;
			}

			val *= NM_PER_MIL;
		}

		// on exit
		m_clearance.m_ca_clearance = val;

		if( m_check_def_net.GetCheck() )
		{
			m_def = 2;
		}
		else
		{
			m_def = 0;
		}

		if(      m_radio2_apply_seg.GetCheck() ) m_apply = 1;
		else if( m_radio2_apply_con.GetCheck() ) m_apply = 2;
		else if( m_radio2_apply_net.GetCheck() ) m_apply = 3;
		else                                     m_apply = 1;

		if(      m_radio3_no    .GetCheck() ) { /* Do nothing - leave m_clearance_pinvia undefined */ }
		else if( m_radio3_as_sel.GetCheck() ) { m_clearance_pinvia = m_clearance; }
		else                                  { m_clearance_pinvia.m_ca_clearance = CClearanceInfo::E_AUTO_CALC; }
	}
}

BOOL DlgSetSegmentClearance::OnInitDialog()
{
	CDialog::OnInitDialog();

	if( m_mode == 0 )
	{
		// called from segment
		m_radio2_apply_seg.SetCheck( 1 );
	}
	else if( m_mode == 1 )
	{
		// called from trace, or ratline trace segment
		m_radio2_apply_seg.EnableWindow( 0 );
		m_radio2_apply_con.SetCheck( 1 );
		m_check_def_net.SetCheck( 0 );
	}
	else if( m_mode == 2 )
	{
		// called from net, or ratline connection
		m_radio2_apply_seg.EnableWindow( 0 );
		m_radio2_apply_con.EnableWindow( 0 );
		m_radio2_apply_net.SetCheck( 1 );
		m_check_def_net.SetCheck( 1 );
	}

	// Must come after mode selection
	if (m_clearance.m_ca_clearance.m_status < 0)
	{
		// Just to make sure
		m_clearance.m_ca_clearance.m_status = CInheritableInfo::E_USE_PARENT;

		// Using net default
		m_radio1_use_net_default.SetCheck(1);
		m_edit_clearance.EnableWindow(0);
		m_check_def_net.EnableWindow(0);
		m_check_def_net.SetCheck( 0 );
	}
	else
	{
		// Using segment value
		m_radio1_set_trace_clearance.SetCheck(1);
		m_edit_clearance.EnableWindow(1);
		m_check_def_net.EnableWindow(1);
	}

	CString str;

	m_clearance.Update_ca_clearance();
	str.Format("%d", m_clearance.m_ca_clearance.m_val / NM_PER_MIL);
	m_edit_clearance.SetWindowText(str);

	m_radio3_auto.SetCheck( 1 );

	return TRUE;
}


BEGIN_MESSAGE_MAP(DlgSetSegmentClearance, CDialog)
	ON_BN_CLICKED(IDC_RADIO_USE_NET_CLEARANCE, &DlgSetSegmentClearance::OnBnClickedRadioUseNetClearance)
	ON_BN_CLICKED(IDC_RADIO_SET_TRACE_CLEARANCE, &DlgSetSegmentClearance::OnBnClickedRadioSetTraceClearance)
	ON_BN_CLICKED(IDC_APPLY_SEG, &DlgSetSegmentClearance::OnBnClickedApplySeg)
	ON_BN_CLICKED(IDC_APPLY_CON, &DlgSetSegmentClearance::OnBnClickedApplyCon)
	ON_BN_CLICKED(IDC_APPLY_NET, &DlgSetSegmentClearance::OnBnClickedApplyNet)
END_MESSAGE_MAP()


// DlgSetSegmentClearance message handlers

void DlgSetSegmentClearance::OnBnClickedApplySeg()
{
	m_apply = 1;
}

void DlgSetSegmentClearance::OnBnClickedApplyCon()
{
	m_apply = 2;
}

void DlgSetSegmentClearance::OnBnClickedApplyNet()
{
	m_apply = 3;
}

void DlgSetSegmentClearance::OnBnClickedRadioUseNetClearance()
{
	m_edit_clearance.EnableWindow(0);
	m_check_def_net.EnableWindow(0);
	m_check_def_net.SetCheck( 0 );
}

void DlgSetSegmentClearance::OnBnClickedRadioSetTraceClearance()
{
	m_edit_clearance.EnableWindow(1);
	m_check_def_net.EnableWindow(1);
}
