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
		str.Format( "%d", m_via_w/NM_PER_MIL );
		m_edit_via_w.SetWindowText( str );
		str.Format( "%d", m_via_hole_w/NM_PER_MIL );
		m_edit_hole_w.SetWindowText( str );
	}
	else
	{
		// outgoing
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

void CDlgVia::Initialize( int via_w, int via_hole_w )
{
	m_via_w = via_w;
	m_via_hole_w = via_hole_w;
}

BEGIN_MESSAGE_MAP(CDlgVia, CDialog)
END_MESSAGE_MAP()


// CDlgVia message handlers
