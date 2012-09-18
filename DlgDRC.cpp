// DlgDRC.cpp : implementation file
//  CPT2 reworked slightly so that it coordinates better with the new logging feature.  Both the Check and OK buttons cause us to exit the dlg;
//  but the former ensures that an actual DRC occurs after returning


#include "stdafx.h"
#include "FreePcb.h"
#include "DlgDRC.h"
#include "Part.h"

#define nm_per_mil 25400.0 

// DlgDRC dialog

IMPLEMENT_DYNAMIC(DlgDRC, CDialog)
DlgDRC::DlgDRC(CWnd* pParent /*=NULL*/)
	: CDialog(DlgDRC::IDD, pParent)
{
}

DlgDRC::~DlgDRC()
{
}

void DlgDRC::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_combo_units);
	DDX_Control(pDX, IDC_EDIT1, m_edit_pad_pad);
	DDX_Control(pDX, IDC_EDIT7, m_edit_pad_trace);
	DDX_Control(pDX, IDC_EDIT6, m_edit_trace_trace);
	DDX_Control(pDX, IDC_EDIT5, m_edit_hole_copper);
	DDX_Control(pDX, IDC_EDIT4, m_edit_annular_ring_pins);
	DDX_Control(pDX, IDC_EDIT2, m_edit_board_edge_copper);
	DDX_Control(pDX, IDC_EDIT8, m_edit_hole_hole);
	DDX_Control(pDX, IDC_EDIT9, m_edit_annular_ring_vias);
	DDX_Control(pDX, IDC_EDIT10, m_edit_copper_copper);
	DDX_Control(pDX, IDC_EDIT14, m_edit_trace_width);
	DDX_Control(pDX, IDC_EDIT11, m_edit_board_edge_hole);
	DDX_Control(pDX, IDC_CHECK_SHOW_UNROUTED, m_check_show_unrouted);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );
		if( m_units == MIL )
			m_combo_units.SetCurSel( 0 );
		else
			m_combo_units.SetCurSel( 1 );
		SetFields();
	}
}


BEGIN_MESSAGE_MAP(DlgDRC, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnChangeUnits)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedCheck)
END_MESSAGE_MAP()


// Initialize
//
void DlgDRC::Initialize( CFreePcbDoc *doc )
{
	m_doc = doc;
	m_units = doc->m_view->m_units; 
	m_dr = &doc->m_dr;
	m_dr_local = doc->m_dr;
	m_CAM_annular_ring_pins = doc->m_annular_ring_pins;
	m_CAM_annular_ring_vias = doc->m_annular_ring_vias;
}

void DlgDRC::GetFields()
{
	CString str;
	m_dr_local.bCheckUnrouted = m_check_show_unrouted.GetCheck();
	double mult = m_units==MIL? nm_per_mil: 1000000.;
	m_edit_trace_width.GetWindowText( str );
	m_dr_local.trace_width = atof( str ) * mult;
	m_edit_pad_pad.GetWindowText( str );
	m_dr_local.pad_pad = atof( str ) * mult;
	m_edit_pad_trace.GetWindowText( str );
	m_dr_local.pad_trace = atof( str ) * mult;
	m_edit_trace_trace.GetWindowText( str );
	m_dr_local.trace_trace = atof( str ) * mult;
	m_edit_hole_copper.GetWindowText( str );
	m_dr_local.hole_copper = atof( str ) * mult;
	m_edit_annular_ring_pins.GetWindowText( str );
	m_dr_local.annular_ring_pins = atof( str ) * mult;
	m_edit_annular_ring_vias.GetWindowText( str );
	m_dr_local.annular_ring_vias = atof( str ) * mult;
	m_edit_board_edge_copper.GetWindowText( str );
	m_dr_local.board_edge_copper = atof( str ) * mult;
	m_edit_board_edge_hole.GetWindowText( str );
	m_dr_local.board_edge_hole = atof( str ) * mult;
	m_edit_hole_hole.GetWindowText( str );
	m_dr_local.hole_hole = atof( str ) * mult;
	m_edit_copper_copper.GetWindowText( str );
	m_dr_local.copper_copper = atof( str ) * mult;
}

void DlgDRC::SetFields()
{
	CString str;
	m_check_show_unrouted.SetCheck( m_dr_local.bCheckUnrouted );
	double div = m_units==MIL? nm_per_mil: 1000000.0;
	MakeCStringFromDouble( &str, m_dr_local.trace_width/div );
	m_edit_trace_width.SetWindowText( str );
	MakeCStringFromDouble( &str, m_dr_local.pad_pad/div );
	m_edit_pad_pad.SetWindowText( str );
	MakeCStringFromDouble( &str, m_dr_local.pad_trace/div );
	m_edit_pad_trace.SetWindowText( str );
	MakeCStringFromDouble( &str, m_dr_local.trace_trace/div );
	m_edit_trace_trace.SetWindowText( str );
	MakeCStringFromDouble( &str, m_dr_local.hole_copper/div );
	m_edit_hole_copper.SetWindowText( str );
	MakeCStringFromDouble( &str, m_dr_local.annular_ring_pins/div );
	m_edit_annular_ring_pins.SetWindowText( str );
	MakeCStringFromDouble( &str, m_dr_local.annular_ring_vias/div );
	m_edit_annular_ring_vias.SetWindowText( str );
	MakeCStringFromDouble( &str, m_dr_local.board_edge_copper/div );
	m_edit_board_edge_copper.SetWindowText( str );
	MakeCStringFromDouble( &str, m_dr_local.board_edge_hole/div );
	m_edit_board_edge_hole.SetWindowText( str );
	MakeCStringFromDouble( &str, m_dr_local.hole_hole/div );
	m_edit_hole_hole.SetWindowText( str );
	MakeCStringFromDouble( &str, m_dr_local.copper_copper/div );
	m_edit_copper_copper.SetWindowText( str );
}

// DlgDRC message handlers

void DlgDRC::OnCbnChangeUnits()
{
	CString str;
	m_combo_units.GetWindowText( str );
	GetFields();
	if( str == "MIL" )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}

void DlgDRC::OnBnClickedCancel()
{
	OnCancel();
}

int DlgDRC::CamWarnings()
{
	if( m_dr_local.annular_ring_pins > m_CAM_annular_ring_pins )  
	{
		CString s ((LPCSTR) IDS_WarningYourDesignRule);
		int ret = AfxMessageBox( s, MB_YESNOCANCEL );
		if( ret == IDCANCEL )
			return IDCANCEL;
		else if( ret == IDYES )
			m_CAM_annular_ring_pins = m_dr_local.annular_ring_pins;
	}
	if( m_dr_local.annular_ring_vias > m_CAM_annular_ring_vias )
	{
		CString s ((LPCSTR) IDS_WarningYourDesignRuleVias);
		int ret = AfxMessageBox( s, MB_YESNOCANCEL );
		if( ret == IDCANCEL )
			return IDCANCEL;
		else if( ret == IDYES )
			m_CAM_annular_ring_vias = m_dr_local.annular_ring_vias;
	}
	return IDOK;
}

void DlgDRC::OnBnClickedOk()
{
	GetFields();
	if (CamWarnings() == IDCANCEL)
		return;
	*m_dr = m_dr_local;
	m_doc->m_annular_ring_pins = m_CAM_annular_ring_pins;
	m_doc->m_annular_ring_vias = m_CAM_annular_ring_vias;
	bCheckOnExit = false;
	OnOK();
}

void DlgDRC::OnBnClickedCheck()
{
	GetFields();
	if (CamWarnings() == IDCANCEL)
		return;
	*m_dr = m_dr_local;
	m_doc->m_annular_ring_pins = m_CAM_annular_ring_pins;
	m_doc->m_annular_ring_vias = m_CAM_annular_ring_vias;
	bCheckOnExit = true;
	OnOK();
}
