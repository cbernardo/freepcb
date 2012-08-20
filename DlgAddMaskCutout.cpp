// DlgAddMaskCutout.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddMaskCutout.h"


// CDlgAddMaskCutout dialog

IMPLEMENT_DYNAMIC(CDlgAddMaskCutout, CDialog)
CDlgAddMaskCutout::CDlgAddMaskCutout(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddMaskCutout::IDD, pParent)
{
}

CDlgAddMaskCutout::~CDlgAddMaskCutout()
{
}

void CDlgAddMaskCutout::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_combo_layer);
	DDX_Control(pDX, IDC_RADIO1, m_radio_none);
	DDX_Control(pDX, IDC_RADIO2, m_radio_edge);
	DDX_Control(pDX, IDC_RADIO3, m_radio_full);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		CString str1 ((LPCSTR) IDS_TopSolderMask), str2 ((LPCSTR) IDS_BottomSolderMask);
		m_combo_layer.InsertString( 0, str1 );
		m_combo_layer.InsertString( 1, str2 );
		m_combo_layer.SetCurSel( m_layer==LAY_SM_TOP? 0: 1 );
		m_radio_none.SetCheck( m_hatch == cpolyline::NO_HATCH );
		m_radio_edge.SetCheck( m_hatch == cpolyline::DIAGONAL_EDGE );
		m_radio_full.SetCheck( m_hatch == cpolyline::DIAGONAL_FULL );
	}
	else
	{
		// outgoing
		if( m_combo_layer.GetCurSel() == 0 )
			m_layer = LAY_SM_TOP;
		else if( m_combo_layer.GetCurSel() == 1 )
			m_layer = LAY_SM_BOTTOM;
		else
			ASSERT(0);
		if( m_radio_none.GetCheck() )
			m_hatch = cpolyline::NO_HATCH;
		else if( m_radio_edge.GetCheck() )
			m_hatch = cpolyline::DIAGONAL_EDGE;
		else if( m_radio_full.GetCheck() )
			m_hatch = cpolyline::DIAGONAL_FULL;
		else
			ASSERT(0);
	}
}

void CDlgAddMaskCutout::Initialize( int layer, int hatch )
{
	m_layer = layer;
	m_hatch = hatch;
}

BEGIN_MESSAGE_MAP(CDlgAddMaskCutout, CDialog)
END_MESSAGE_MAP()


// CDlgAddMaskCutout message handlers
