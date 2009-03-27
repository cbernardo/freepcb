// DlgVia.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgVia.h"


// CDlgViaPinSize dialog

IMPLEMENT_DYNAMIC(CDlgViaPinSize, CDialog)
CDlgViaPinSize::CDlgViaPinSize(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgViaPinSize::IDD, pParent)
	, CSubDlg_ViaWidth()
	, CSubDlg_Clearance( E_NO_AUTO_MODE )
{
	m_via_width.Undef();
}

CDlgViaPinSize::~CDlgViaPinSize()
{
}

void CDlgViaPinSize::Initialize( CInheritableInfo const &via_width )
{
	m_via_width = via_width;
}

BOOL CDlgViaPinSize::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_via_width.Update();

	// Enable Modification of Vias
	m_check_v_modify.SetCheck( 1 );
	m_check_c_modify.SetCheck( 1 );

	CSubDlg_ViaWidth ::OnInitDialog( m_via_width );
	CSubDlg_Clearance::OnInitDialog( m_via_width );

	return TRUE;
}

BEGIN_MESSAGE_MAP(CDlgViaPinSize, CDialog)
	ON_BN_CLICKED(IDC_CHECK_VIA,       OnBnClicked_v_modify)
	ON_BN_CLICKED(IDC_RADIO2_PROJ_DEF, OnBnClicked_v_Default)
	ON_BN_CLICKED(IDC_RADIO2_SET_TO,   OnBnClicked_v_Set)

	ON_BN_CLICKED(IDC_CHECK_CLEARANCE,           OnBnClicked_c_modify)
	ON_BN_CLICKED(IDC_RADIO_USE_NET_CLEARANCE,   OnBnClicked_c_Default)
	ON_BN_CLICKED(IDC_RADIO_SET_TRACE_CLEARANCE, OnBnClicked_c_Set)
END_MESSAGE_MAP()

void CDlgViaPinSize::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_STATIC_GROUP_WIDTH,   m_text_v_group);
	DDX_Control(pDX, IDC_CHECK_VIA,            m_check_v_modify);
	DDX_Control(pDX, IDC_RADIO2_PROJ_DEF,      m_rb_v_default);
	DDX_Control(pDX, IDC_RADIO2_SET_TO,        m_rb_v_set);
	DDX_Control(pDX, IDC_TEXT_PAD,             m_text_v_pad_w);
	DDX_Control(pDX, IDC_EDIT_VIA_PAD_W,       m_edit_v_pad_w);
	DDX_Control(pDX, IDC_TEXT_HOLE,            m_text_v_hole_w);
	DDX_Control(pDX, IDC_EDIT_VIA_HOLE_W,      m_edit_v_hole_w);

	DDX_Control(pDX, IDC_CHECK_CLEARANCE,           m_check_c_modify);
	DDX_Control(pDX, IDC_RADIO_USE_NET_CLEARANCE,   m_rb_c_default);
	DDX_Control(pDX, IDC_RADIO_SET_TRACE_CLEARANCE, m_rb_c_set);
	DDX_Control(pDX, IDC_EDIT_CLEARANCE,            m_edit_c_clearance);

	if( pDX->m_bSaveAndValidate )
	{
		// outgoing
		if( !CSubDlg_ViaWidth ::OnDDXOut() ||
			!CSubDlg_Clearance::OnDDXOut() )
		{
			pDX->Fail();
			return;
		}
		else
		{
			m_via_width.Undef();

			m_via_width = CSubDlg_ViaWidth ::m_attrib;
			m_via_width = CSubDlg_Clearance::m_attrib;
		}
	}
}
