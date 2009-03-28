// DlgSetSegmentWidth.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgSetSegmentWidth.h"

// DlgSetSegmentWidth dialog

IMPLEMENT_DYNAMIC(DlgSetSegmentWidth, CDialog)

DlgSetSegmentWidth::DlgSetSegmentWidth(CWnd* pParent /*=NULL*/)
	: CDialog(DlgSetSegmentWidth::IDD, pParent)
	, CSubDlg_ViaWidth(static_cast<CSubDlg_TraceWidth*>(this))
{
	m_mode = 0;
	m_def = 0;
	m_apply = 0;
}

DlgSetSegmentWidth::~DlgSetSegmentWidth()
{
}


BOOL DlgSetSegmentWidth::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_width_attrib.Update();

	if( m_mode == 0 )
	{
		// called from segment
		m_apply_seg.SetCheck( 1 );
	}
	else if( m_mode == 1 )
	{
		// called from trace, or ratline trace segment
		m_apply_con.SetCheck( 1 );

		m_apply_seg.EnableWindow( 0 );

		m_set_net_default.SetCheck( 0 );
	}
	else if( m_mode == 2 )
	{
		// called from net, or ratline connection
		m_apply_net.SetCheck( 1 );

		m_apply_seg.EnableWindow( 0 );
		m_apply_con.EnableWindow( 0 );

		m_set_net_default.SetCheck( 1 );
	}

	m_check_t_modify.SetCheck( 1 );
	m_check_v_modify.SetCheck( 1 );

	// Do these last after other dialog items are setup
	CSubDlg_TraceWidth::OnInitDialog(m_width_attrib);
	CSubDlg_ViaWidth  ::OnInitDialog(m_width_attrib);

	return TRUE;
}


void DlgSetSegmentWidth::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CHECK_TRACE,     m_check_t_modify);
	DDX_Control(pDX, IDC_RADIO1_PROJ_DEF, m_rb_t_default);
	DDX_Control(pDX, IDC_RADIO1_SET_TO,   m_rb_t_set);
	DDX_Control(pDX, IDC_COMBO_WIDTH,     m_combo_t_width);

	DDX_Control(pDX, IDC_CHECK_VIA,            m_check_v_modify);
	DDX_Control(pDX, IDC_RADIO2_PROJ_DEF,      m_rb_v_default);
	DDX_Control(pDX, IDC_RADIO2_DEF_FOR_TRACE, m_rb_v_def_for_width);
	DDX_Control(pDX, IDC_RADIO2_SET_TO,        m_rb_v_set);
	DDX_Control(pDX, IDC_TEXT_PAD,             m_text_v_pad_w);
	DDX_Control(pDX, IDC_EDIT_VIA_PAD_W,       m_edit_v_pad_w);
	DDX_Control(pDX, IDC_TEXT_HOLE,            m_text_v_hole_w);
	DDX_Control(pDX, IDC_EDIT_VIA_HOLE_W,      m_edit_v_hole_w);

	DDX_Control(pDX, IDC_DEF_NET, m_set_net_default);

	DDX_Control(pDX, IDC_APPLY_NET, m_apply_net);
	DDX_Control(pDX, IDC_APPLY_CON, m_apply_con);
	DDX_Control(pDX, IDC_APPLY_SEG, m_apply_seg);

	if( pDX->m_bSaveAndValidate )
	{
		// exiting dialog
		if( !CSubDlg_TraceWidth::OnDDXOut() || !CSubDlg_ViaWidth::OnDDXOut() )
		{
			pDX->Fail();
			return;
		}
		else
		{
			m_width_attrib.Undef();

			m_width_attrib = CSubDlg_TraceWidth::m_attrib;
			m_width_attrib = CSubDlg_ViaWidth::m_attrib;
		}

		// decode buttons
		if( m_set_net_default.GetCheck() )
		{
			m_def = 2;
		}
		else
		{
			m_def = 0;
		}

		if     ( m_apply_net.GetCheck() ) m_apply = 3;
		else if( m_apply_con.GetCheck() ) m_apply = 2;
		else if( m_apply_seg.GetCheck() ) m_apply = 1;
		else                              m_apply = 0;
	}
}


BEGIN_MESSAGE_MAP(DlgSetSegmentWidth, CDialog)
	ON_BN_CLICKED(IDC_CHECK_TRACE,     OnBnClicked_t_modify)
	ON_BN_CLICKED(IDC_RADIO1_PROJ_DEF, OnBnClicked_t_Default)
	ON_BN_CLICKED(IDC_RADIO1_SET_TO,   OnBnClicked_t_Set)
	ON_CBN_SELCHANGE(IDC_COMBO_WIDTH,  OnCbnSelchange_t_width)
	ON_CBN_EDITCHANGE(IDC_COMBO_WIDTH, OnCbnEditchange_t_width)

	ON_BN_CLICKED(IDC_CHECK_VIA,            OnBnClicked_v_modify)
	ON_BN_CLICKED(IDC_RADIO2_PROJ_DEF,      OnBnClicked_v_Default)
	ON_BN_CLICKED(IDC_RADIO2_DEF_FOR_TRACE, OnBnClicked_v_DefForTrace)
	ON_BN_CLICKED(IDC_RADIO2_SET_TO,        OnBnClicked_v_Set)
END_MESSAGE_MAP()
