// DlgRefText.cpp : implementation file
// CPT2:  THIS DIALOG NOW COVERS NOT ONLY SITUATIONS WHERE USER HITS F1 WITH A REFERENCE-TEXT SELECTED, BUT ALSO
// F1 FOR VALUE-TEXTS

#include "stdafx.h"
#include "FreePcb.h"
#include "Part.h"
#include "DlgRefText.h"


// CDlgRefText dialog

IMPLEMENT_DYNAMIC(CDlgRefText, CDialog)
CDlgRefText::CDlgRefText(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgRefText::IDD, pParent)
{
}

CDlgRefText::~CDlgRefText()
{
}

void CDlgRefText::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_REF_ID, m_edit_ref_des);
	DDX_Control(pDX, IDC_EDIT_CHAR_HEIGHT, m_edit_height);
	DDX_Control(pDX, IDC_COMBO_REF_TEXT_UNITS, m_combo_units);
	DDX_Control(pDX, IDC_RADIO_SET, m_radio_set);
	DDX_Control(pDX, IDC_RADIO_DEF, m_radio_def);
	DDX_Control(pDX, IDC_EDIT_WIDTH, m_edit_width);
	DDX_Control(pDX, IDC_EDIT_DEF_WIDTH, m_edit_def_width);
	DDX_Control(pDX, IDC_CHECK_VISIBLE, m_check_visible);
	DDX_Control(pDX, IDC_COMBO_LAYER_REF, m_combo_layer);
	if( !pDX->m_bSaveAndValidate )
	{
		// entry
		// CPT2.  First set dialog's title based on whether it's a reference or a value being edited
		CString title;
		if (m_text->IsRefText())
			title.LoadStringA(IDS_ReferenceTextProperties);
		else
			title.LoadStringA(IDS_ValueTextProperties);
		SetWindowTextA(title);
		m_edit_ref_des.EnableWindow( true );					// CPT2.  Now enabled, both for value texts and ref texts
		m_edit_ref_des.SetWindowText( m_text->m_str );
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );

		CString s;
		for (int i=0; i<4; i++)
			s.LoadStringA(IDS_TopSilk+i),
			m_combo_layer.InsertString( i, s );

		if( m_width == m_height/10 )
		{
			m_radio_set.SetCheck( FALSE );
			m_radio_def.SetCheck( TRUE );
			m_edit_def_width.EnableWindow( FALSE );
			m_edit_width.EnableWindow( FALSE );
		}
		else
		{
			m_radio_set.SetCheck( TRUE );
			m_radio_def.SetCheck( FALSE );
			m_edit_def_width.EnableWindow( FALSE );
			m_edit_width.EnableWindow( TRUE );
		}
		SetFields();
	}
	else
	{
		// exit
		GetFields();
		if (m_height==0)
		{
			// CPT2 new restriction.  Let 'em hide it if they don't want to see it.
			CString s ((LPCSTR) IDS_TextHeightZeroIsNotPermitted);
			AfxMessageBox( s );
			pDX->Fail();
		}
		m_edit_ref_des.GetWindowText( m_str );
		if (m_str != m_text->m_str && m_text->IsRefText())
		{
			// CPT2. User wants to change text for a reference designator.  Certain checks must occur first (cf. the checks in DlgAddPart.cpp)
			m_str.Trim();
			CString ird ((LPCSTR) IDS_IllegalReferenceDesignator);
			bool bOK = true;
			if( m_str == "" )
				bOK = false;
			else if( m_str.FindOneOf( ". " ) != -1 )
				ird += " " + m_str,
				bOK = false;
			else if ( m_text->doc->m_plist->GetPartByName(&m_str) )
			{
				CString duplicateRef ((LPCSTR) IDS_DuplicateReference);
				ird.Format( duplicateRef, m_str );
				bOK = false;
			}
			if (!bOK)
			{
				AfxMessageBox( ird );
				pDX->PrepareEditCtrl( IDC_EDIT_REF_ID );
				pDX->Fail();
				return;
			}
		}

		if( m_radio_def.GetCheck() )
			m_width = m_def_width;
	}
}

// the calling program should call this to set up dialog
// and provide pointers to variables which will be modified
//
void CDlgRefText::Initialize( CText *text, CShapeList * cache_shapes )
{
	m_text = text;
	m_cache_shapes = cache_shapes;
	CPart *part = text->GetPart();
	m_vis = text->m_bShown;
	m_units = part->shape->m_units;
	m_width = text->m_stroke_width;
	m_height = text->m_font_size;
	m_def_width = m_height/10;
	m_layer = FlipLayer( part->side, text->m_layer );
}

BEGIN_MESSAGE_MAP(CDlgRefText, CDialog)
	ON_BN_CLICKED(IDC_RADIO_SET, OnBnClickedRadioSet)
	ON_EN_CHANGE(IDC_EDIT_CHAR_HEIGHT, OnEnChangeEditCharHeight)
	ON_BN_CLICKED(IDC_RADIO_DEF, OnBnClickedRadioDef)
	ON_CBN_SELCHANGE(IDC_COMBO_REF_TEXT_UNITS, OnCbnSelchangeComboRefTextUnits)
END_MESSAGE_MAP()


// CDlgRefText message handlers

void CDlgRefText::OnBnClickedRadioSet()
{
	m_edit_width.EnableWindow( TRUE );
}

void CDlgRefText::OnBnClickedRadioDef()
{
	m_edit_width.EnableWindow( FALSE );

}

void CDlgRefText::OnEnChangeEditCharHeight()
{
	CString str;
	m_edit_height.GetWindowText( str );
	if( m_units == MIL )
		m_height = atof( str ) * NM_PER_MIL;
	else
		m_height = atof( str ) * 1000000.0;
	m_def_width = m_height/10;
	if( m_def_width < 1*NM_PER_MIL )
		m_def_width = 1*NM_PER_MIL;
	if( m_units == MIL )
	{
		MakeCStringFromDouble( &str, m_def_width/NM_PER_MIL );
		m_edit_def_width.SetWindowText( str );
	}
	else
	{
		MakeCStringFromDouble( &str, m_def_width/1000000.0 );
		m_edit_def_width.SetWindowText( str );
	}
}

void CDlgRefText::GetFields()
{
	CString str;
	switch( m_combo_layer.GetCurSel() )
	{
		case 0: m_layer = LAY_SILK_TOP; break;
		case 1: m_layer = LAY_SILK_BOTTOM; break;
		case 2: m_layer = LAY_TOP_COPPER; break;
		case 3: m_layer = LAY_BOTTOM_COPPER; break;
		default: ASSERT(0); break;
	}
	if( m_units == MIL )
	{
		m_edit_height.GetWindowText( str );
		m_height = atof( str ) * NM_PER_MIL;
		m_edit_width.GetWindowText( str );
		m_width = atof( str ) * NM_PER_MIL;
	}
	else
	{
		m_edit_height.GetWindowText( str );
		m_height = atof( str ) * 1000000.0;
		m_edit_width.GetWindowText( str );
		m_width = atof( str ) * 1000000.0;
	}
	m_def_width = m_height/10;
	m_vis = m_check_visible.GetCheck();
}

void CDlgRefText::SetFields()
{
	CString str;
	switch( m_layer )
	{
	case LAY_SILK_TOP: m_combo_layer.SetCurSel(0); break;
	case LAY_SILK_BOTTOM: m_combo_layer.SetCurSel(1); break;
	case LAY_TOP_COPPER: m_combo_layer.SetCurSel(2); break;
	case LAY_BOTTOM_COPPER: m_combo_layer.SetCurSel(3); break;
	default: assert(0); break;
	}
	if( m_units == MIL )
	{
		m_combo_units.SetCurSel( 0 );
		MakeCStringFromDouble( &str, m_height/NM_PER_MIL );
		m_edit_height.SetWindowText( str );
		MakeCStringFromDouble( &str, m_width/NM_PER_MIL );
		m_edit_width.SetWindowText( str );
		MakeCStringFromDouble( &str, m_def_width/NM_PER_MIL );
		m_edit_def_width.SetWindowText( str );
	}
	else
	{
		m_combo_units.SetCurSel( 1 );
		MakeCStringFromDouble( &str, m_height/1000000.0 );
		m_edit_height.SetWindowText( str );
		MakeCStringFromDouble( &str, m_width/1000000.0 );
		m_edit_width.SetWindowText( str );
		MakeCStringFromDouble( &str, m_def_width/1000000.0 );
		m_edit_def_width.SetWindowText( str );
	}
	m_check_visible.SetCheck( m_vis );
}

void CDlgRefText::OnCbnSelchangeComboRefTextUnits()
{
	GetFields();
	if( m_combo_units.GetCurSel() == 0 )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}
