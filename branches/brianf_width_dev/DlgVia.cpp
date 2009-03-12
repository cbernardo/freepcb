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
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		CString str;

		str = CInheritableInfo::GetItemText( m_via_width.m_via_width );
		m_edit_via_w.SetWindowText( str );

		str = CInheritableInfo::GetItemText( m_via_width.m_via_hole );
		m_edit_hole_w.SetWindowText( str );
	}
	else
	{
		// outgoing
		CString str;
		int i;

		m_edit_via_w.GetWindowText( str );
		if( str == "Default" )
		{
			m_via_width.m_via_width = CInheritableInfo::E_USE_PARENT;
		}
		else
		{
			i = GetDimensionFromString( &str );
			if( i <= 0 )
			{
				AfxMessageBox( "Illegal via width" );
				pDX->Fail();
			}
			m_via_width.m_via_width = i;
		}

		m_edit_hole_w.GetWindowText( str );
		if( str == "Default" )
		{
			m_via_width.m_via_width = CInheritableInfo::E_USE_PARENT;
		}
		else
		{
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

void CDlgVia::Initialize( CConnectionWidthInfo const &via_width )
{
	m_via_width = via_width;
}

BEGIN_MESSAGE_MAP(CDlgVia, CDialog)
END_MESSAGE_MAP()


// CDlgVia message handlers
