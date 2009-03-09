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
{
}

CDlgSetTraceWidths::~CDlgSetTraceWidths()
{
}

void CDlgSetTraceWidths::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_COMBO_WIDTH, m_combo_width);
	DDX_Control(pDX, IDC_RADIO_DEF, m_radio_default_via_for_trace);
	DDX_Control(pDX, IDC_RADIO_SET, m_radio_set_via_width);
	DDX_Control(pDX, IDC_EDIT_VIA_W, m_edit_via_pad);
	DDX_Control(pDX, IDC_EDIT_HOLE_W, m_edit_via_hole);
	DDX_Control(pDX, IDC_CHECK1, m_check_apply);
	DDX_Control(pDX, IDC_CHECK2, m_check_trace);
	DDX_Control(pDX, IDC_CHECK3, m_check_vias);
	DDX_Control(pDX, IDC_RADIO_REVERT_TRACES, m_radio_revert_traces);
	DDX_Control(pDX, IDC_RADIO_REVERT_VIAS, m_radio_revert_vias);
	DDX_Control(pDX, IDC_RADIO_SET_TRACE_WIDTH, m_radio_set_trace_width);

	DDX_Control(pDX, IDC_RADIO_DEF_CLEARANCE, m_radio3_def_clearance);
	DDX_Control(pDX, IDC_RADIO_SET_CLEARANCE, m_radio3_set_clearance);
	DDX_Control(pDX, IDC_EDIT_CLEARANCE, m_edit_clearance);
	DDX_Control(pDX, IDC_MOD_CLEARANCE, m_check_clearance);

	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		CString str;
		for( int i=0; i<m_w->GetSize(); i++ )
		{
			str.Format( "%d", (*m_w)[i]/NM_PER_MIL );
			m_combo_width.InsertString( i, str );
		}
		if( m_width > 0 )
		{
			str.Format( "%d", m_width/NM_PER_MIL );
			m_combo_width.SetWindowText( str );
		}
		if( m_via_width > 0 && m_hole_width > 0 )
		{
			str.Format( "%d", m_via_width/NM_PER_MIL );
			m_edit_via_pad.SetWindowText( str );
			str.Format( "%d", m_hole_width/NM_PER_MIL );
			m_edit_via_hole.SetWindowText( str );
		}
		m_check_trace.SetCheck(1);
		m_radio_set_trace_width.SetCheck(1);
		m_check_vias.SetCheck(1);

		if( m_width > 0 )
			m_radio_set_via_width.SetCheck( 1 );
		else
			m_radio_default_via_for_trace.SetCheck( 1 );

		m_check_apply.SetCheck(1);

		// Clearance section
		if( m_clearance.m_ca_clearance.m_status < 0 )
		{
			// Just to make sure
			m_clearance.m_ca_clearance.m_status = CInheritableInfo::E_USE_PARENT;

			m_radio3_def_clearance.SetCheck(1);
			m_edit_clearance.EnableWindow(0);
		}
		else
		{
			m_radio3_set_clearance.SetCheck(0);
			m_edit_clearance.EnableWindow(1);
		}

		str.Format( "%d", m_clearance.m_ca_clearance.m_val / NM_PER_MIL );
		m_edit_clearance.SetWindowText(str);

		m_check_clearance.SetCheck(1);

		SetFields();
	}
	else
	{
		// outgoing
		SetFields();
		if( bTraces && bRevertTraces )
		{
			m_width = 0;
		}
		else if( bTraces )
		{
			DDX_Text( pDX, IDC_COMBO_WIDTH, m_width );
			m_width *= NM_PER_MIL;
		}
		else
			m_width = -1;
		if( bVias && bRevertVias )
		{
			m_via_width = 0;
			m_hole_width = 0;
		}
		else if( bVias )
		{
			DDX_Text( pDX, IDC_EDIT_VIA_W, m_via_width );
			DDX_Text( pDX, IDC_EDIT_HOLE_W, m_hole_width );
			m_via_width *= NM_PER_MIL;
			m_hole_width *= NM_PER_MIL;
		}
		else
		{
			m_via_width = -1;
		}

		if( m_apply_clearance )
		{
			int val;
			if( m_radio3_def_clearance.GetCheck() )
			{
				m_clearance.m_ca_clearance.m_status = CClearanceInfo::E_USE_PARENT;
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
				m_clearance.m_ca_clearance = val;
			}
		}
	}
}


BEGIN_MESSAGE_MAP(CDlgSetTraceWidths, CDialog)
	ON_BN_CLICKED(IDC_RADIO_DEF, OnBnClickedRadioDef)
	ON_BN_CLICKED(IDC_RADIO_SET, OnBnClickedRadioSet)
	ON_CBN_SELCHANGE(IDC_COMBO_WIDTH, OnCbnSelchangeComboWidth)
	ON_CBN_EDITCHANGE(IDC_COMBO_WIDTH, OnCbnEditchangeComboWidth)
	ON_BN_CLICKED(IDC_CHECK2, OnBnClickedSetTrace)
	ON_BN_CLICKED(IDC_CHECK3, OnBnClickedSetVias)
	ON_BN_CLICKED(IDC_RADIO_REVERT_TRACES, OnBnClickedRadioRevertTraces)
	ON_BN_CLICKED(IDC_RADIO_REVERT_VIAS, OnBnClickedRadioRevertVias)
	ON_BN_CLICKED(IDC_RADIO_SET_TRACE_WIDTH, OnBnClickedRadioSetTraceWidth)
	ON_BN_CLICKED(IDC_RADIO_DEF_CLEARANCE, &CDlgSetTraceWidths::OnBnClickedRadioRevertTraceClearance)
	ON_BN_CLICKED(IDC_RADIO_SET_CLEARANCE, &CDlgSetTraceWidths::OnBnClickedRadioSetTraceClearance)
	ON_BN_CLICKED(IDC_MOD_CLEARANCE, &CDlgSetTraceWidths::OnBnClickedModClearance)
END_MESSAGE_MAP()


// CDlgSetTraceWidths message handlers

void CDlgSetTraceWidths::OnBnClickedRadioDef()
{
	SetFields();
	OnCbnEditchangeComboWidth();
}

void CDlgSetTraceWidths::OnBnClickedRadioSet()
{
	SetFields();
}

void CDlgSetTraceWidths::OnCbnSelchangeComboWidth()
{
	CString test;
	int i = m_combo_width.GetCurSel();
	m_combo_width.GetLBText( i, test );
	int n = m_w->GetSize();
	if( m_radio_default_via_for_trace.GetCheck() )
	{
		int new_w = atoi( (LPCSTR)test )*NM_PER_MIL;
		int new_v_w = 0;
		int new_v_h_w = 0;
		if( new_w >= 0 )
		{
			if( new_w == 0 )
			{
				new_v_w = 0;
				new_v_h_w = 0;
			}
			else if( new_w <= (*m_w)[0] )
			{
				new_v_w = (*m_v_w)[0];
				new_v_h_w = (*m_v_h_w)[0];
			}
			else if( new_w >= (*m_w)[n-1] )
			{
				new_v_w = (*m_v_w)[n-1];
				new_v_h_w = (*m_v_h_w)[n-1];
			}
			else
			{
				for( int i=1; i<n; i++ )
				{
					if( new_w > (*m_w)[i-1] && new_w <= (*m_w)[i] )
					{
						new_v_w = (*m_v_w)[i];
						new_v_h_w = (*m_v_h_w)[i];
						break;
					}
				}
			}
			test.Format( "%d", new_v_w/NM_PER_MIL );
			m_edit_via_pad.SetWindowText( test );
			test.Format( "%d", new_v_h_w/NM_PER_MIL );
			m_edit_via_hole.SetWindowText( test );
		}
	}
}

void CDlgSetTraceWidths::OnCbnEditchangeComboWidth()
{
	CString test;
	int n = m_w->GetSize();
	if( m_radio_default_via_for_trace.GetCheck() )
	{
		m_combo_width.GetWindowText( test );
		int new_w = atoi( (LPCSTR)test )*NM_PER_MIL;
		int new_v_w = 0;
		int new_v_h_w = 0;
		if( new_w >= 0 )
		{
			if( new_w == 0 )
			{
				new_v_w = 0;
				new_v_h_w = 0;
			}
			else if( new_w <= (*m_w)[0] )
			{
				new_v_w = (*m_v_w)[0];
				new_v_h_w = (*m_v_h_w)[0];
			}
			else if( new_w >= (*m_w)[n-1] )
			{
				new_v_w = (*m_v_w)[n-1];
				new_v_h_w = (*m_v_h_w)[n-1];
			}
			else
			{
				for( int i=1; i<n; i++ )
				{
					if( new_w > (*m_w)[i-1] && new_w <= (*m_w)[i] )
					{
						new_v_w = (*m_v_w)[i];
						new_v_h_w = (*m_v_h_w)[i];
						break;
					}
				}
			}
			test.Format( "%d", new_v_w/NM_PER_MIL );
			m_edit_via_pad.SetWindowText( test );
			test.Format( "%d", new_v_h_w/NM_PER_MIL );
			m_edit_via_hole.SetWindowText( test );
		}
	}
}

void CDlgSetTraceWidths::OnBnClickedSetTrace()
{
	SetFields();
}

void CDlgSetTraceWidths::OnBnClickedSetVias()
{
	SetFields();
}

void CDlgSetTraceWidths::SetFields()
{
	bTraces = m_check_trace.GetCheck();
	bRevertTraces = m_radio_revert_traces.GetCheck();

	m_radio_revert_traces.EnableWindow( bTraces );
	m_radio_set_trace_width.EnableWindow( bTraces );
	m_combo_width.EnableWindow( bTraces && !bRevertTraces );

	bVias = m_check_vias.GetCheck();
	if( m_radio_default_via_for_trace.GetCheck() && !bTraces )
	{
		m_radio_revert_vias.SetCheck(1);
		m_radio_default_via_for_trace.SetCheck(0);
	}
	bDefaultVias = m_radio_default_via_for_trace.GetCheck();
	bRevertVias = m_radio_revert_vias.GetCheck();

	m_radio_revert_vias.EnableWindow( bVias );
	m_radio_default_via_for_trace.EnableWindow( bVias && bTraces );
	m_radio_set_via_width.EnableWindow( bVias );
	m_edit_via_pad.EnableWindow( bVias && !bDefaultVias && !bRevertVias );
	m_edit_via_hole.EnableWindow( bVias && !bDefaultVias && !bRevertVias  );

	m_apply_trace = bTraces;
	m_apply_via = bVias;

	// Section: Clearance
	m_apply_clearance = m_check_clearance.GetCheck();

	m_radio3_def_clearance.EnableWindow(m_apply_clearance);
	m_radio3_set_clearance.EnableWindow(m_apply_clearance);

	if( m_apply_clearance && m_radio3_set_clearance.GetCheck() )
	{
		m_edit_clearance.EnableWindow(1);
	}
	else
	{
		m_edit_clearance.EnableWindow(0);
	}

	// Section: Apply to already routed traces
	m_apply_to_routed = m_check_apply.GetCheck();
}

void CDlgSetTraceWidths::OnBnClickedRadioRevertTraces()
{
	SetFields();
}

void CDlgSetTraceWidths::OnBnClickedRadioRevertVias()
{
	SetFields();
}

void CDlgSetTraceWidths::OnBnClickedRadioSetTraceWidth()
{
	SetFields();
}

void CDlgSetTraceWidths::OnBnClickedRadioRevertTraceClearance()
{
	SetFields();
}

void CDlgSetTraceWidths::OnBnClickedRadioSetTraceClearance()
{
	SetFields();
}

void CDlgSetTraceWidths::OnBnClickedModClearance()
{
	SetFields();
}
