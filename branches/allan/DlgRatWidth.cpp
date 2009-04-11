// DlgRatWidth.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgRatWidth.h"


// CRatWidth dialog

IMPLEMENT_DYNAMIC(CRatWidth, CDialog)

CRatWidth::CRatWidth(CWnd* pParent /*=NULL*/)
	: CDialog(CRatWidth::IDD, pParent)
	, m_ratline_w(0)
{

}

CRatWidth::~CRatWidth()
{
}

void CRatWidth::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_RAT_W, m_ratline_w);
	DDV_MinMaxInt(pDX, m_ratline_w, 0, MAX_RATLINE_W_MIL);
}


BEGIN_MESSAGE_MAP(CRatWidth, CDialog)
END_MESSAGE_MAP()


// CRatWidth message handlers
