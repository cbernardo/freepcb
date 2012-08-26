// DlgAddPoly.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddPoly.h"

static int gLastLayerIndex = -1;
static int gLastWidth = -1;																// CPT2, in order to implement Initialize() in the way the comments describe
static int gDlgAddPolyLayers[] =														// CPT2, improved the behavior of layers in this dlg.
	{ LAY_FP_SILK_TOP, LAY_FP_SILK_BOTTOM, LAY_FP_TOP_COPPER, LAY_FP_BOTTOM_COPPER };

// CDlgAddPoly dialog

IMPLEMENT_DYNAMIC(CDlgAddPoly, CDialog)
CDlgAddPoly::CDlgAddPoly(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddPoly::IDD, pParent)
{
	m_width = 10*NM_PER_MIL;	// default
	m_layer_index = 0;
}

CDlgAddPoly::~CDlgAddPoly()
{
}

void CDlgAddPoly::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO_OPEN, m_radio_open);
	DDX_Control(pDX, IDC_RADIO_CLOSED, m_radio_closed);
	DDX_Control(pDX, IDC_COMBO_ADD_POLY_UNITS, m_combo_units);
	DDX_Control( pDX, IDC_EDIT_WIDTH, m_edit_width );
	DDX_Control(pDX, IDC_COMBO_LAYER, m_combo_layer);
	DDX_Control(pDX, IDCANCEL, m_radio_closed_and_filled);
	DDX_Control(pDX, IDC_COMBO_PIN_NAME, m_combo_pin_name);
	if( !pDX->m_bSaveAndValidate )
	{
		// entry
		m_radio_open.SetCheck( 1 );
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );
		if( m_units == MIL )
			m_combo_units.SetCurSel( 0 );
		else
			m_combo_units.SetCurSel( 1 );
		CString s;
		for (int i=0; i<4; i++)
			s.LoadStringA(IDS_TopSilk+i),
			m_combo_layer.InsertString( i, s );
		CIter<CPadstack> ips (m_padstacks);
		int i = 0;
		for (CPadstack *ps = ips.First(); ps; ps = ips.Next())
			m_combo_pin_name.InsertString( i++, ps->name );
		SetFields();
	}
	else
	{
		// exit
		GetFields();
		m_closed_flag = m_radio_closed.GetCheck();
		if( m_width < 1*NM_PER_MIL || m_width > 999*NM_PER_MIL )
		{
			pDX->PrepareEditCtrl( IDC_EDIT_WIDTH );
			CString s ((LPCSTR) IDS_WidthOutOfRange);
			AfxMessageBox( s );
			pDX->Fail();
		}
		gLastLayerIndex = m_layer_index;
		gLastWidth = m_width;
		m_layer = gDlgAddPolyLayers[m_layer_index];
	}
}

// initialize parameters
// if bNewPoly, this is a new polyline
// if layer == -1, use layer used last time, or 0 (LAY_FP_TOP_SILK)
// if width == -1, use width used last time, or 10 mils
// if bNewPoly, assume closed
void CDlgAddPoly::Initialize( BOOL bNewPoly, int layer, int units, 
							 int width, BOOL bClosed, CHeap<CPadstack> * padstack )
{
	m_bNewPoly = bNewPoly;
	m_units = units;
	// Get layer index value (within gDlgAddPolyLayers)
	if( layer == -1 )
		m_layer_index = gLastLayerIndex == -1? 0: gLastLayerIndex;
	else
	{
		for (m_layer_index=0; m_layer_index<4; m_layer_index++)
			if (gDlgAddPolyLayers[m_layer_index] == layer)
				break;
		if (m_layer_index==4) m_layer_index = 0;
	}
	if (width==-1)
		m_width = gLastWidth==-1? 10*NM_PER_MIL: gLastWidth;
	else
		m_width = width;
	if( bNewPoly )
		m_closed_flag = 1;
	else
		m_closed_flag = bClosed;
	m_padstacks = padstack;
}


BEGIN_MESSAGE_MAP(CDlgAddPoly, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_ADD_POLY_UNITS, OnCbnSelchangeComboAddPolyUnits)
	ON_CBN_SELCHANGE(IDC_COMBO_LAYER, &CDlgAddPoly::OnCbnSelchangeComboLayer)
END_MESSAGE_MAP()


void CDlgAddPoly::GetFields()
{
	CString str;
	if( m_units == MIL )
	{
		m_edit_width.GetWindowText( str );
		m_width = atof( str ) * NM_PER_MIL;
	}
	else
	{
		m_edit_width.GetWindowText( str );
		m_width = atof( str ) * 1000000.0;
	}
	m_layer_index = m_combo_layer.GetCurSel();
	m_closed_flag = m_radio_closed.GetCheck();
	int pin_name_index = m_combo_pin_name.GetCurSel();
	if( pin_name_index == CB_ERR )
		m_pin_name = "";
	else
		m_combo_pin_name.GetWindowTextA( m_pin_name );				// CPT2 TODO.  Does not appear to be doing anything useful at this point
}

void CDlgAddPoly::SetFields()
{
	CString str;
	if( m_units == MIL )
	{
		MakeCStringFromDouble( &str, m_width/NM_PER_MIL );
		m_edit_width.SetWindowText( str );
	}
	else
	{
		MakeCStringFromDouble( &str, m_width/1000000.0 );
		m_edit_width.SetWindowText( str );
	}
	m_combo_layer.SetCurSel( m_layer_index );
	m_radio_open.SetCheck( !m_closed_flag );
	m_radio_closed.SetCheck( m_closed_flag );
	m_combo_pin_name.EnableWindow( m_layer_index > 1 );
}

void CDlgAddPoly::OnCbnSelchangeComboAddPolyUnits()
{
	GetFields();
	if( m_combo_units.GetCurSel() == 0 )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}


void CDlgAddPoly::OnCbnSelchangeComboLayer()
{
	m_layer_index = m_combo_layer.GetCurSel();
	m_combo_pin_name.EnableWindow( m_layer_index > 1 );
}
