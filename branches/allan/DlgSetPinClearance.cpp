// DlgSetPinClearance.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgSetPinClearance.h"


// CDlgSetPinClearance dialog

IMPLEMENT_DYNAMIC(CDlgSetPinClearance, CDialog)

CDlgSetPinClearance::CDlgSetPinClearance(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSetPinClearance::IDD, pParent)
	, CSubDlg_Clearance( E_USE_AUTO_MODE )
{

}

CDlgSetPinClearance::~CDlgSetPinClearance()
{
}


BOOL CDlgSetPinClearance::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_clearance.Update();

	m_apply_this_only.SetCheck( 1 );

	m_check_c_modify.SetCheck( 1 );

	// Do this last after other dialog items are setup
	CSubDlg_Clearance::OnInitDialog(m_clearance);

	return TRUE;
}


void CDlgSetPinClearance::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CHECK_CLEARANCE_MOD,      m_check_c_modify);
	DDX_Control(pDX, IDC_RADIO_PIN_NET_CLEARANCE,  m_rb_c_default);
	DDX_Control(pDX, IDC_RADIO_PIN_AUTO_CLEARANCE, m_rb_c_auto);
	DDX_Control(pDX, IDC_RADIO_PIN_SET_CLEARANCE,  m_rb_c_set);
	DDX_Control(pDX, IDC_EDIT_CLEARANCE,           m_edit_c_clearance);

	DDX_Control(pDX, IDC_APPLY_PIN, m_apply_this_only);
	DDX_Control(pDX, IDC_APPLY_CON, m_apply_con);
	DDX_Control(pDX, IDC_APPLY_NET, m_apply_net);

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

		if(      m_apply_this_only.GetCheck() ) m_apply = 1;
		else if( m_apply_con      .GetCheck() ) m_apply = 2;
		else if( m_apply_net      .GetCheck() ) m_apply = 3;
		else                                    m_apply = 1;
	}
}


BEGIN_MESSAGE_MAP(CDlgSetPinClearance, CDialog)
	ON_BN_CLICKED(IDC_RADIO_PIN_AUTO_CLEARANCE, OnBnClicked_c_Auto)
	ON_BN_CLICKED(IDC_RADIO_PIN_NET_CLEARANCE,  OnBnClicked_c_Default)
	ON_BN_CLICKED(IDC_RADIO_PIN_SET_CLEARANCE,  OnBnClicked_c_Set)
END_MESSAGE_MAP()
