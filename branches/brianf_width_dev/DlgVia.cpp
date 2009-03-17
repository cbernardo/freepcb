// DlgVia.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgVia.h"


// CDlgVia dialog

IMPLEMENT_DYNAMIC(CDlgVia, CDialog)
CDlgVia::CDlgVia(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgVia::IDD, pParent)
	, CSubDlg_ViaWidth()
{
}

CDlgVia::~CDlgVia()
{
}

void CDlgVia::Initialize( CConnectionWidthInfo const &via_width )
{
	m_via_width = via_width;
}

BOOL CDlgVia::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_via_width.Update();

	// Enable Modification of Vias
	m_check_v_modify.SetCheck(1);

	CSubDlg_ViaWidth::OnInitDialog(m_via_width);

	return TRUE;
}

BEGIN_MESSAGE_MAP(CDlgVia, CDialog)
	ON_BN_CLICKED(IDC_RADIO1_PROJ_DEF, OnBnClicked_v_Default)
	ON_BN_CLICKED(IDC_RADIO1_SET_TO,   OnBnClicked_v_Set)
END_MESSAGE_MAP()

void CDlgVia::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_RADIO1_PROJ_DEF,      m_rb_v_default);
	DDX_Control(pDX, IDC_RADIO1_SET_TO,        m_rb_v_set);
	DDX_Control(pDX, IDC_TEXT_PAD,             m_text_v_pad_w);
	DDX_Control(pDX, IDC_VIA_WIDTH,            m_edit_v_pad_w);
	DDX_Control(pDX, IDC_TEXT_HOLE,            m_text_v_hole_w);
	DDX_Control(pDX, IDC_VIA_HOLE_WIDTH,       m_edit_v_hole_w);
	DDX_Control(pDX, IDC_CHECK_VIA_MOD,        m_check_v_modify);

	if( pDX->m_bSaveAndValidate )
	{
		// outgoing
		if( !CSubDlg_ViaWidth  ::OnDDXOut() )
		{
			pDX->Fail();
			return;
		}
		else
		{
			m_via_width.Undef();

			m_via_width = CSubDlg_ViaWidth::m_attrib;
		}
	}
}
