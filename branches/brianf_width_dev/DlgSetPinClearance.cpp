// DlgSetPinClearance.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgSetPinClearance.h"


// CDlgSetPinClearance dialog

IMPLEMENT_DYNAMIC(CDlgSetPinClearance, CDialog)

CDlgSetPinClearance::CDlgSetPinClearance(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSetPinClearance::IDD, pParent)
{

}

CDlgSetPinClearance::~CDlgSetPinClearance()
{
}


BOOL CDlgSetPinClearance::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString str;

	m_clearance.Update();
	str.Format("%d", m_clearance.m_ca_clearance.m_val / NM_PER_MIL);
	m_edit_clearance.SetWindowText(str);

	switch( m_clearance.m_ca_clearance.m_status )
	{
	default:
	case CClearanceInfo::E_AUTO_CALC:
		m_clearance_auto.SetCheck( 1 );
		break;

	case CClearanceInfo::E_USE_PARENT:
		m_clearance_net.SetCheck( 1 );
		break;

	case CClearanceInfo::E_USE_VAL:
		m_clearance_set.SetCheck( 1 );
		break;
	}

	m_apply_this_only.SetCheck( 1 );

	OnUpdateClearanceType();

	return TRUE;
}


void CDlgSetPinClearance::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_RADIO_PIN_AUTO_CLEARANCE, m_clearance_auto);
	DDX_Control(pDX, IDC_RADIO_PIN_NET_CLEARANCE,  m_clearance_net);
	DDX_Control(pDX, IDC_RADIO_PIN_SET_CLEARANCE,  m_clearance_set);
	DDX_Control(pDX, IDC_EDIT_CLEARANCE, m_edit_clearance);

	DDX_Control(pDX, IDC_APPLY_PIN, m_apply_this_only);
	DDX_Control(pDX, IDC_APPLY_CON, m_apply_con);
	DDX_Control(pDX, IDC_APPLY_NET, m_apply_net);

	if( pDX->m_bSaveAndValidate )
	{
		int val;
		if( m_clearance_auto.GetCheck() )
		{
			m_clearance.m_ca_clearance.m_status = CClearanceInfo::E_AUTO_CALC;
		}
		else if( m_clearance_net.GetCheck() )
		{
			m_clearance.m_ca_clearance.m_status = CClearanceInfo::E_USE_PARENT;
		}
		else
		{
			CString str;
			m_edit_clearance.GetWindowText(str);
			if( (sscanf(str, "%d", &val) != 1) || (val < 0) || (val > MAX_CLEARANCE_MIL) )
			{
				str.Format("Invalid clearance value (0-%d)", MAX_CLEARANCE_MIL);
				AfxMessageBox( str );
				pDX->Fail();
				return;
			}

			val *= NM_PER_MIL;
			m_clearance.m_ca_clearance = val;
		}

		if(      m_apply_this_only.GetCheck() ) m_apply = 1;
		else if( m_apply_con      .GetCheck() ) m_apply = 2;
		else if( m_apply_net      .GetCheck() ) m_apply = 3;
		else                                    m_apply = 1;
	}
}


BEGIN_MESSAGE_MAP(CDlgSetPinClearance, CDialog)
	ON_BN_CLICKED(IDC_RADIO_PIN_AUTO_CLEARANCE, &CDlgSetPinClearance::OnBnClickedRadioPinAutoClearance)
	ON_BN_CLICKED(IDC_RADIO_PIN_NET_CLEARANCE, &CDlgSetPinClearance::OnBnClickedRadioPinNetClearance)
	ON_BN_CLICKED(IDC_RADIO_PIN_SET_CLEARANCE, &CDlgSetPinClearance::OnBnClickedRadioPinSetClearance)
END_MESSAGE_MAP()

void CDlgSetPinClearance::OnUpdateClearanceType()
{
	m_edit_clearance.EnableWindow( m_clearance_set.GetCheck() );
}

// CDlgSetPinClearance message handlers
void CDlgSetPinClearance::OnBnClickedRadioPinAutoClearance()
{
	OnUpdateClearanceType();
}

void CDlgSetPinClearance::OnBnClickedRadioPinNetClearance()
{
	OnUpdateClearanceType();
}

void CDlgSetPinClearance::OnBnClickedRadioPinSetClearance()
{
	OnUpdateClearanceType();
}
