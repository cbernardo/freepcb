// DlgVia.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgVia.h"


// CDlgVia dialog

IMPLEMENT_DYNAMIC(CDlgVia, CDialog)
CDlgVia::CDlgVia(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgVia::IDD, pParent)
{
}

CDlgVia::~CDlgVia()
{
}

void CDlgVia::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VIA_WIDTH, m_edit_via_w);
	DDX_Control(pDX, IDC_VIA_HOLE_WIDTH, m_edit_hole_w);
	DDX_Control(pDX, IDC_COMBO_VIA_UNITS, m_combo_units);
	DDX_Control(pDX, IDC_VIA_X, m_edit_x);
	DDX_Control(pDX, IDC_VIA_Y, m_edit_y);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		CString str;
		str.Format( "%d", m_via_w/NM_PER_MIL );
		m_edit_via_w.SetWindowText( str );
		str.Format( "%d", m_via_hole_w/NM_PER_MIL );
		m_edit_hole_w.SetWindowText( str );
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );
		if( m_units == MIL )
			m_combo_units.SetCurSel(0);
		else
			m_combo_units.SetCurSel(1);
		SetFields();
	}
	else
	{
		// outgoing
		GetFields();
		CString str;
		m_edit_via_w.GetWindowText( str );
		m_via_w = GetDimensionFromString( &str );
		if( m_via_w <= 0 )
		{
			CString s ((LPCSTR) IDS_IllegalViaWidth);
			AfxMessageBox( s );
			pDX->Fail();
		}
		m_edit_hole_w.GetWindowText( str );
		m_via_hole_w = GetDimensionFromString( &str );
		if( m_via_hole_w <= 0 )
		{
			CString s ((LPCSTR) IDS_IllegalViaHoleWidth);
			AfxMessageBox( s );
			pDX->Fail();
		}
	}
}

void CDlgVia::Initialize( int via_w, int via_hole_w, CPoint pt, int units )
{
	m_via_w = via_w;
	m_via_hole_w = via_hole_w;
	m_pt = pt;
	m_units = units;
}

BEGIN_MESSAGE_MAP(CDlgVia, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_VIA_UNITS, &CDlgVia::OnCbnSelChangeComboViaUnits)
END_MESSAGE_MAP()


// CDlgVia message handlers

void CDlgVia::OnCbnSelChangeComboViaUnits()
{
	GetFields();
	if( m_combo_units.GetCurSel() == 0 )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}

void CDlgVia::GetFields()
{
	// get x and y values
	CString xstr;
	m_edit_x.GetWindowText( xstr );
	CString ystr;
	m_edit_y.GetWindowText( ystr );
	if( m_units == MIL )
	{
		m_pt.x = my_atof( &xstr )*NM_PER_MIL;
		m_pt.y = my_atof( &ystr )*NM_PER_MIL;
	}
	else
	{
		m_pt.x = my_atof( &xstr )*1000000.0;
		m_pt.y = my_atof( &ystr )*1000000.0;
	}
}

void CDlgVia::SetFields()
{
	CString str;
	if( m_units == MIL )
	{
		::MakeCStringFromDouble( &str, (double)m_pt.x/NM_PER_MIL );
		m_edit_x.SetWindowText( str );
		::MakeCStringFromDouble( &str, (double)m_pt.y/NM_PER_MIL );
		m_edit_y.SetWindowText( str );
	}
	else
	{
		::MakeCStringFromDouble( &str, (double)m_pt.x/1000000.0 );
		m_edit_x.SetWindowText( str );
		::MakeCStringFromDouble( &str, (double)m_pt.y/1000000.0 );
		m_edit_y.SetWindowText( str );
	}

}
