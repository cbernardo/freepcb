// DlgAddWidth.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddWidth.h"


// CDlgAddWidth dialog

IMPLEMENT_DYNAMIC(CDlgAddWidth, CDialog)
CDlgAddWidth::CDlgAddWidth(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddWidth::IDD, pParent)
	, m_width(0)
	, m_via_w(0)
	, m_via_hole_w(0)
{
}

CDlgAddWidth::~CDlgAddWidth()
{
}

void CDlgAddWidth::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text( pDX, IDC_EDIT_WIDTH, m_width );
	DDX_Text( pDX, IDC_EDIT_VIA_W, m_via_w );
	DDX_Text( pDX, IDC_EDIT_HOLE_W, m_via_hole_w );
}


BEGIN_MESSAGE_MAP(CDlgAddWidth, CDialog)
END_MESSAGE_MAP()


// CDlgAddWidth message handlers
