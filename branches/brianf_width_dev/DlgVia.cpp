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

void CDlgVia::Initialize( CConnectionWidthInfo const &via_width )
{
	m_via_width = via_width;
}

BOOL CDlgVia::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_via_width.Update();

	CString str;

	if( m_via_width.m_via_width.m_status < 0 )
	{
		m_via_width.m_via_width.m_status = CInheritableInfo::E_USE_PARENT;
		m_via_width.m_via_hole.m_status = CInheritableInfo::E_USE_PARENT;

		m_radio_proj_def.SetCheck( 1 );

		m_edit_via_w.EnableWindow( 0 );
		m_edit_hole_w.EnableWindow( 0 );
	}
	else
	{
		m_radio_set_to.SetCheck( 1 );

		m_edit_via_w.EnableWindow( 1 );
		m_edit_hole_w.EnableWindow( 1 );
	}

	str.Format( "%d", m_via_width.m_via_width.m_val / NM_PER_MIL );
	m_edit_via_w.SetWindowText( str );

	str.Format( "%d", m_via_width.m_via_hole.m_val / NM_PER_MIL );
	m_edit_hole_w.SetWindowText( str );

	return TRUE;
}

BEGIN_MESSAGE_MAP(CDlgVia, CDialog)
	ON_BN_CLICKED(IDC_RADIO1_PROJ_DEF, &CDlgVia::OnBnClickedRadio1ProjDef)
	ON_BN_CLICKED(IDC_RADIO1_SET_TO, &CDlgVia::OnBnClickedRadio1SetTo)
END_MESSAGE_MAP()

void CDlgVia::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_VIA_WIDTH, m_edit_via_w);
	DDX_Control(pDX, IDC_VIA_HOLE_WIDTH, m_edit_hole_w);
	DDX_Control(pDX, IDC_RADIO1_PROJ_DEF, m_radio_proj_def);
	DDX_Control(pDX, IDC_RADIO1_SET_TO, m_radio_set_to);

	if( pDX->m_bSaveAndValidate )
	{
		// outgoing
		CString str;
		int i;

		if( m_radio_proj_def.GetCheck() )
		{
			m_via_width.m_via_width.m_status = CInheritableInfo::E_USE_PARENT;
			m_via_width.m_via_hole.m_status = CInheritableInfo::E_USE_PARENT;
		}
		else
		{
			m_edit_via_w.GetWindowText( str );
			i = GetDimensionFromString( &str );
			if( i <= 0 )
			{
				AfxMessageBox( "Illegal via width" );
				pDX->Fail();
			}
			m_via_width.m_via_width = i;

			m_edit_hole_w.GetWindowText( str );
			i = GetDimensionFromString( &str );
			if( i <= 0 )
			{
				AfxMessageBox( "Illegal via width" );
				pDX->Fail();
			}
			m_via_width.m_via_hole = i;
		}
	}
}

// CDlgVia message handlers

void CDlgVia::OnBnClickedRadio1ProjDef()
{
	m_edit_via_w.EnableWindow( 0 );
	m_edit_hole_w.EnableWindow( 0 );

	m_via_width.m_via_width.m_status = CInheritableInfo::E_USE_PARENT;
	m_via_width.m_via_hole.m_status = CInheritableInfo::E_USE_PARENT;
	m_via_width.Update();

	CString str;

	str.Format( "%d", m_via_width.m_via_width.m_val / NM_PER_MIL );
	m_edit_via_w.SetWindowText( str );

	str.Format( "%d", m_via_width.m_via_hole.m_val / NM_PER_MIL );
	m_edit_hole_w.SetWindowText( str );
}

void CDlgVia::OnBnClickedRadio1SetTo()
{
	m_edit_via_w.EnableWindow( 1 );
	m_edit_hole_w.EnableWindow( 1 );
}
