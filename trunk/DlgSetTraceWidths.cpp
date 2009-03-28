// DlgSetTraceWidths.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgSetTraceWidths.h"
#include ".\dlgsettracewidths.h"


// CDlgSetTraceWidths dialog

IMPLEMENT_DYNAMIC(CDlgSetTraceWidths, CDialog)
CDlgSetTraceWidths::CDlgSetTraceWidths(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSetTraceWidths::IDD, pParent)
	, CSubDlg_ViaWidth(static_cast<CSubDlg_TraceWidth*>(this))
	, CSubDlg_Clearance( E_NO_AUTO_MODE )
{
}

CDlgSetTraceWidths::~CDlgSetTraceWidths()
{
}

BOOL CDlgSetTraceWidths::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_width_attrib.Update();

	// Enable Modification of Traces, Vias, and Clearance
	m_check_t_modify.SetCheck(1);
	m_check_v_modify.SetCheck(1);
	m_check_c_modify.SetCheck(1);

	// default is to apply new trace width & clearances
	m_check_t_apply.SetCheck(1);
	m_check_v_apply.SetCheck(1);
	m_check_c_apply.SetCheck(1);

	// Do these last after other dialog items are setup
	CSubDlg_TraceWidth::OnInitDialog(m_width_attrib);
	CSubDlg_ViaWidth  ::OnInitDialog(m_width_attrib);
	CSubDlg_Clearance ::OnInitDialog(m_width_attrib);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgSetTraceWidths::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_RADIO1_PROJ_DEF, m_rb_t_default);
	DDX_Control(pDX, IDC_RADIO1_SET_TO,   m_rb_t_set);
	DDX_Control(pDX, IDC_COMBO_WIDTH,     m_combo_t_width);
	DDX_Control(pDX, IDC_CHECK_TRACE,     m_check_t_apply);
	DDX_Control(pDX, IDC_CHECK_TRACE_MOD, m_check_t_modify);

	DDX_Control(pDX, IDC_RADIO2_PROJ_DEF,      m_rb_v_default);
	DDX_Control(pDX, IDC_RADIO2_DEF_FOR_TRACE, m_rb_v_def_for_width);
	DDX_Control(pDX, IDC_RADIO2_SET_TO,        m_rb_v_set);
	DDX_Control(pDX, IDC_TEXT_PAD,             m_text_v_pad_w);
	DDX_Control(pDX, IDC_EDIT_VIA_PAD_W,       m_edit_v_pad_w);
	DDX_Control(pDX, IDC_TEXT_HOLE,            m_text_v_hole_w);
	DDX_Control(pDX, IDC_EDIT_VIA_HOLE_W,      m_edit_v_hole_w);
	DDX_Control(pDX, IDC_CHECK_VIA,            m_check_v_apply);
	DDX_Control(pDX, IDC_CHECK_VIA_MOD,        m_check_v_modify);

	DDX_Control(pDX, IDC_RADIO3_PROJ_DEF,     m_rb_c_default);
	DDX_Control(pDX, IDC_RADIO3_SET_TO,       m_rb_c_set);
	DDX_Control(pDX, IDC_EDIT_CLEARANCE,      m_edit_c_clearance);
	DDX_Control(pDX, IDC_CHECK_CLEARANCE,     m_check_c_apply);
	DDX_Control(pDX, IDC_CHECK_CLEARANCE_MOD, m_check_c_modify);

	if( pDX->m_bSaveAndValidate )
	{
		// outgoing
		if( !CSubDlg_TraceWidth::OnDDXOut() ||
		    !CSubDlg_ViaWidth  ::OnDDXOut() ||
		    !CSubDlg_Clearance ::OnDDXOut() )
		{
			pDX->Fail();
			return;
		}
		else
		{
			m_width_attrib.Undef();

			m_width_attrib = CSubDlg_TraceWidth::m_attrib.get_data();
			m_width_attrib = CSubDlg_ViaWidth::m_attrib;
			m_width_attrib = CSubDlg_Clearance::m_attrib;
		}

		m_apply_trace     = m_check_t_apply.GetCheck();
		m_apply_via		  = m_check_v_apply.GetCheck();
		m_apply_clearance = m_check_c_apply.GetCheck();
	}
}


BEGIN_MESSAGE_MAP(CDlgSetTraceWidths, CDialog)
	ON_BN_CLICKED(IDC_CHECK_TRACE_MOD, OnBnClicked_t_modify)
	ON_BN_CLICKED(IDC_RADIO1_PROJ_DEF, OnBnClicked_t_Default)
	ON_BN_CLICKED(IDC_RADIO1_SET_TO,   OnBnClicked_t_Set)
	ON_CBN_SELCHANGE(IDC_COMBO_WIDTH,  OnCbnSelchange_t_width)
	ON_CBN_EDITCHANGE(IDC_COMBO_WIDTH, OnCbnEditchange_t_width)

	ON_BN_CLICKED(IDC_CHECK_VIA_MOD,        OnBnClicked_v_modify)
	ON_BN_CLICKED(IDC_RADIO2_PROJ_DEF,      OnBnClicked_v_Default)
	ON_BN_CLICKED(IDC_RADIO2_DEF_FOR_TRACE, OnBnClicked_v_DefForTrace)
	ON_BN_CLICKED(IDC_RADIO2_SET_TO,        OnBnClicked_v_Set)

	ON_BN_CLICKED(IDC_CHECK_CLEARANCE_MOD, OnBnClicked_c_modify)
	ON_BN_CLICKED(IDC_RADIO3_PROJ_DEF,     OnBnClicked_c_Default)
	ON_BN_CLICKED(IDC_RADIO3_SET_TO,       OnBnClicked_c_Set)
END_MESSAGE_MAP()
