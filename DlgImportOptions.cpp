// DlgImportOptions.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgImportOptions.h"
#include ".\dlgimportoptions.h"


// CDlgImportOptions dialog

IMPLEMENT_DYNAMIC(CDlgImportOptions, CDialog)
CDlgImportOptions::CDlgImportOptions(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgImportOptions::IDD, pParent)
{
}

CDlgImportOptions::~CDlgImportOptions()
{
}

void CDlgImportOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO_REMOVE_PARTS, m_radio_remove_parts);
	DDX_Control(pDX, IDC_RADIO_KEEP_PARTS_NO_CONNECTIONS, m_radio_keep_parts_no_connections);
	DDX_Control(pDX, IDC_RADIO_KEEP_PARTS_AND_CONNECTIONS, m_radio_keep_parts_and_connections);
	DDX_Control(pDX, IDC_RADIO4, m_radio_change_fp);
	DDX_Control(pDX, IDC_RADIO3, m_radio_keep_fp);
	DDX_Control(pDX, IDC_RADIO6, m_radio_remove_nets);
	DDX_Control(pDX, IDC_RADIO5, m_radio_keep_nets);
	DDX_Control(pDX, ID_SAVE_AND_IMPORT, m_button_save_and_import);
	// CPT:
	DDX_Control( pDX, IDC_RADIO_PARTS, m_radio_parts );
	DDX_Control( pDX, IDC_RADIO_NETS, m_radio_nets );
	DDX_Control( pDX, IDC_RADIO_PARTSANDNETS, m_radio_parts_and_nets );
	DDX_Control( pDX, IDC_RADIO_PADSPCB, m_radio_padspcb );
	DDX_Control( pDX, IDC_RADIO_FREEPCB, m_radio_freepcb );
	// end CPT

	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_radio_padspcb.SetCheck(TRUE);
		EnableDisableButtons();

		if( m_flags & KEEP_PARTS_AND_CON )
			m_radio_keep_parts_and_connections.SetCheck( TRUE );
		else if( m_flags & KEEP_PARTS_NO_CON )
			m_radio_keep_parts_no_connections.SetCheck( TRUE );
		else
			m_radio_remove_parts.SetCheck( TRUE );
		if( m_flags & KEEP_FP )
			m_radio_keep_fp.SetCheck( TRUE );
		else
		m_radio_change_fp.SetCheck( TRUE );
		if( m_flags & KEEP_NETS )
			m_radio_keep_nets.SetCheck( TRUE );
		else
			m_radio_remove_nets.SetCheck( TRUE );
	}

	else
	{
		// outgoing
		SetPartsNetsFlags();
		// NOT IMPLEMENTED: if( m_radio_padspcb.GetCheck() )
		m_format = PADSPCB;

		if( m_flags & IMPORT_PARTS )
		{
			m_flags &= ~(KEEP_PARTS_NO_CON | KEEP_PARTS_AND_CON | KEEP_FP);
			if( m_radio_keep_parts_no_connections.GetCheck() )
				m_flags |= KEEP_PARTS_NO_CON;
			else if( m_radio_keep_parts_and_connections.GetCheck() )
				m_flags |= KEEP_PARTS_AND_CON;
			if( m_radio_keep_fp.GetCheck() )
				m_flags |= KEEP_FP;
		}
		if( m_flags & IMPORT_NETS )
		{
			m_flags &= ~KEEP_NETS;
			if( m_radio_keep_nets.GetCheck() )
				m_flags |= KEEP_NETS;
		}
	}
}
// end CPT

BEGIN_MESSAGE_MAP(CDlgImportOptions, CDialog)
	ON_BN_CLICKED(ID_SAVE_AND_IMPORT, OnBnClickedSaveAndImport)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_RADIO_PARTS, OnBnClickedPartsAndNets)					// CPT
	ON_BN_CLICKED(IDC_RADIO_NETS, OnBnClickedPartsAndNets)					// CPT
	ON_BN_CLICKED(IDC_RADIO_PARTSANDNETS, OnBnClickedPartsAndNets)			// CPT
END_MESSAGE_MAP()

void CDlgImportOptions::Initialize( int flags )
{
	m_flags = flags;
	// CPT2 deal with possibility that neither IMPORT_NETS flag nor IMPORT_PARTS is set:
	if (!(m_flags & (IMPORT_NETS|IMPORT_PARTS)))
		m_flags |= IMPORT_PARTS|IMPORT_NETS;
}

// CDlgImportOptions message handlers

void CDlgImportOptions::OnBnClickedSaveAndImport()
{
	m_flags |= SAVE_BEFORE_IMPORT;
	OnOK();
}

void CDlgImportOptions::OnBnClickedOk()
{
	m_flags &= ~SAVE_BEFORE_IMPORT;
	OnOK();
}

// CPT:  All that follows:
void CDlgImportOptions::SetPartsNetsFlags() {
	m_flags &= ~IMPORT_PARTS;
	m_flags &= ~IMPORT_NETS;
	if( m_radio_parts.GetCheck() || m_radio_parts_and_nets.GetCheck() )
		m_flags |= IMPORT_PARTS;
	if( m_radio_nets.GetCheck() || m_radio_parts_and_nets.GetCheck() )
		m_flags |= IMPORT_NETS;
	if (!(m_flags & IMPORT_PARTS) && !(m_flags & IMPORT_NETS))
		m_flags |= IMPORT_PARTS | IMPORT_NETS;
	}

void CDlgImportOptions::EnableDisableButtons() {
	// Enable and disable buttons depending on the states of the IMPORT_PARTS and IMPORT_NETS flags.  Assumes at least one of those is set.
	bool fPartsOnly = !(m_flags & IMPORT_NETS);
	bool fNetsOnly = !(m_flags & IMPORT_PARTS);
	bool fPartsAndNets = !fPartsOnly && !fNetsOnly;

	m_radio_parts.SetCheck(fPartsOnly);
	m_radio_nets.SetCheck(fNetsOnly);
	m_radio_parts_and_nets.SetCheck(fPartsAndNets);

	if( m_flags & IMPORT_PARTS )	{
		m_radio_remove_parts.EnableWindow( 1 );
		m_radio_keep_parts_no_connections.EnableWindow( 1 );
		m_radio_keep_parts_and_connections.EnableWindow( 1 );
		m_radio_keep_fp.EnableWindow( 1 );
		m_radio_change_fp.EnableWindow( 1 );
		}
	else	{
		m_radio_remove_parts.EnableWindow( 0 );
		m_radio_keep_parts_no_connections.EnableWindow( 0 );
		m_radio_keep_parts_and_connections.EnableWindow( 0 );
		m_radio_keep_fp.EnableWindow( 0 );
		m_radio_change_fp.EnableWindow( 0 );
		}
	if( m_flags & IMPORT_NETS ) {
		m_radio_keep_nets.EnableWindow( 1 );
		m_radio_remove_nets.EnableWindow( 1 );
		}
	else {
		m_radio_keep_nets.EnableWindow( 0 );
		m_radio_remove_nets.EnableWindow( 0 );
		}
	}

void CDlgImportOptions::OnBnClickedPartsAndNets() {
	SetPartsNetsFlags();
	EnableDisableButtons();
	}

// end CPT