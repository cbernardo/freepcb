// DlgExportOptions.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgExportOptions.h"

// ALL CPT

IMPLEMENT_DYNAMIC(CDlgExportOptions, CDialog)
CDlgExportOptions::CDlgExportOptions(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgExportOptions::IDD, pParent)
{
}

CDlgExportOptions::~CDlgExportOptions()
{
}

void CDlgExportOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control( pDX, IDC_RADIO_PARTS, m_radio_parts );
	DDX_Control( pDX, IDC_RADIO_NETS, m_radio_nets );
	DDX_Control( pDX, IDC_RADIO_PARTSANDNETS, m_radio_parts_and_nets );
	DDX_Control( pDX, IDC_RADIO_PADSPCB, m_radio_padspcb );
	DDX_Control( pDX, IDC_RADIO_FREEPCB, m_radio_freepcb );
	DDX_Control( pDX, IDC_CHECK_AT, m_check_values );
	if( !pDX->m_bSaveAndValidate )
	{
		// on entry, set controls from member variables
		if( m_select & EXPORT_PARTS && m_select & EXPORT_NETS )
			m_radio_parts_and_nets.SetCheck( TRUE );
		else if( m_select & EXPORT_PARTS )
			m_radio_parts.SetCheck( TRUE );
		else if( m_select & EXPORT_NETS )
			m_radio_nets.SetCheck( TRUE );
		m_check_values.SetCheck( m_select &  EXPORT_VALUES );
		m_radio_padspcb.SetCheck( TRUE );
	}
	else {
		// On exit:
		if( m_radio_parts.GetCheck() )
			m_select = EXPORT_PARTS;
		else if( m_radio_nets.GetCheck() )
			m_select = EXPORT_NETS;
		else
			m_select = EXPORT_PARTS | EXPORT_NETS;
		if( m_check_values.GetCheck() )
			m_select |= EXPORT_VALUES;
		if( m_radio_padspcb.GetCheck() )
			m_format = PADSPCB;
	}
}

// Initialize dialog settings:
//	select = 0 for parts, 1 for nets, 2 for both
//	at_flag = TRUE to use "value@footprint" format for parts
//
void CDlgExportOptions::Initialize( int select )
{
	m_select = select;
}


BEGIN_MESSAGE_MAP(CDlgExportOptions, CDialog)
END_MESSAGE_MAP()


