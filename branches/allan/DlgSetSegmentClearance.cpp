// DlgSetSegmentClearance.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgSetSegmentClearance.h"


// DlgSetSegmentClearance dialog

IMPLEMENT_DYNAMIC(DlgSetSegmentClearance, CDialog)

DlgSetSegmentClearance::DlgSetSegmentClearance(CWnd* pParent /*=NULL*/)
	: CDialog(DlgSetSegmentClearance::IDD, pParent)
	, CSubDlg_Clearance( E_NO_AUTO_MODE )
{

}

DlgSetSegmentClearance::~DlgSetSegmentClearance()
{
}

BOOL DlgSetSegmentClearance::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_clearance.Update();

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

		m_set_net_default.SetCheck( 0 );
	}
	else if( m_mode == 2 )
	{
		// called from net, or ratline connection
		m_radio2_apply_seg.EnableWindow( 0 );
		m_radio2_apply_con.EnableWindow( 0 );
		m_radio2_apply_net.SetCheck( 1 );

		m_set_net_default.SetCheck( 1 );
	}

	m_radio3_auto.SetCheck( 1 );

	m_check_c_modify.SetCheck( 1 );

	// Do this last after other dialog items are setup
	CSubDlg_Clearance::OnInitDialog(m_clearance);

	return TRUE;
}

void DlgSetSegmentClearance::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CHECK_CLEARANCE_MOD, m_check_c_modify);
	DDX_Control(pDX, IDC_RADIO_USE_NET_CLEARANCE, m_rb_c_default);
	DDX_Control(pDX, IDC_RADIO_SET_TRACE_CLEARANCE, m_rb_c_set);
	DDX_Control(pDX, IDC_EDIT_CLEARANCE, m_edit_c_clearance);

	DDX_Control(pDX, IDC_DEF_NET, m_set_net_default);

	DDX_Control(pDX, IDC_APPLY_SEG, m_radio2_apply_seg);
	DDX_Control(pDX, IDC_APPLY_CON, m_radio2_apply_con);
	DDX_Control(pDX, IDC_APPLY_NET, m_radio2_apply_net);

	DDX_Control(pDX, IDC_APPLY_PINVIA_NO,     m_radio3_no);
	DDX_Control(pDX, IDC_APPLY_PINVIA_AUTO,   m_radio3_auto);
	DDX_Control(pDX, IDC_APPLY_PINVIA_AS_SEL, m_radio3_as_sel);

	if( pDX->m_bSaveAndValidate )
	{
		if( !CSubDlg_Clearance::OnDDXOut() )
		{
			pDX->Fail();
			return;
		}
		else
		{
			m_clearance.Undef();
			m_clearance = CSubDlg_Clearance::m_attrib;
		}

		if( m_set_net_default.GetCheck() )
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

		if(      m_radio3_no    .GetCheck() ) { m_clearance_pinvia.m_ca_clearance.Undef(); }
		else if( m_radio3_as_sel.GetCheck() ) { m_clearance_pinvia = m_clearance; }
		else                                  { m_clearance_pinvia.m_ca_clearance = CClearanceInfo::E_AUTO_CALC; }
	}
}


BEGIN_MESSAGE_MAP(DlgSetSegmentClearance, CDialog)
	ON_BN_CLICKED(IDC_RADIO_USE_NET_CLEARANCE, &DlgSetSegmentClearance::OnBnClicked_c_Default)
	ON_BN_CLICKED(IDC_RADIO_SET_TRACE_CLEARANCE, &DlgSetSegmentClearance::OnBnClicked_c_Set)
END_MESSAGE_MAP()
