// DlgAddPoly.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddPoly.h"

static int gLastLayerIndex = -1;

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
#ifndef CPT2
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
		for( int ip=0; ip<m_padstack->GetSize(); ip++ )
		{
			m_combo_pin_name.InsertString( ip, (*m_padstack)[ip].name );
		}
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
	}
#endif
}

// initialize parameters
// if bNewPoly, this is a new polyline
// if layer_index == -1, use layer used last time, or 0
// if width == -1, use width used last time, or 10 mils
// if bNewPoly, assume closed
void CDlgAddPoly::Initialize( BOOL bNewPoly, int layer_index, int units, 
							 int width, BOOL bClosed, carray<cpadstack> * padstack )
{
	m_bNewPoly = bNewPoly;
	m_units = units;
	if( layer_index == -1 )
	{
		if( gLastLayerIndex == -1 )
		{
			m_layer_index = 0;
		}
		else
		{
			m_layer_index = gLastLayerIndex;
		}
	}
	else
	{
		m_layer_index = layer_index;
	}
	if( bNewPoly )
	{
		m_closed_flag = 1;
	}
	else
	{
		m_closed_flag = bClosed;
	}
	m_padstack = padstack;
}


BEGIN_MESSAGE_MAP(CDlgAddPoly, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_ADD_POLY_UNITS, OnCbnSelchangeComboAddPolyUnits)
	ON_CBN_SELCHANGE(IDC_COMBO_LAYER, &CDlgAddPoly::OnCbnSelchangeComboLayer)
END_MESSAGE_MAP()


void CDlgAddPoly::GetFields()
{
#ifndef CPT2
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
	{
		m_pin_name = "";
	}
	else
	{
		m_pin_name = (*m_padstack)[pin_name_index].name;
	}
#endif
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
