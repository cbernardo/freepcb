// DlgAssignLayers.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAssignLayers.h"


// CDlgAssignLayers dialog

IMPLEMENT_DYNAMIC(CDlgAssignLayers, CDialog)
CDlgAssignLayers::CDlgAssignLayers(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAssignLayers::IDD, pParent)
{
}

CDlgAssignLayers::~CDlgAssignLayers()
{
}

void CDlgAssignLayers::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgAssignLayers, CDialog)
END_MESSAGE_MAP()


// CDlgAssignLayers message handlers
