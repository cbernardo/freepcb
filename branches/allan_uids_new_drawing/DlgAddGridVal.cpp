// DlgAddGridVal.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddGridVal.h"
#include "afxdialogex.h"


// CDlgAddGridVal dialog.  ALL CPT

IMPLEMENT_DYNAMIC(CDlgAddGridVal, CDialogEx)

CDlgAddGridVal::CDlgAddGridVal(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDlgAddGridVal::IDD, pParent)
{ }

CDlgAddGridVal::~CDlgAddGridVal()
{ }

void CDlgAddGridVal::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VALUE, m_edit_value);
	if( pDX->m_bSaveAndValidate ) {
		CString text;
		m_edit_value.GetWindowText( text );
		result = GetDimensionFromString(&text, MIL, true, true);		// Returns 0 on error
		}
	}


BEGIN_MESSAGE_MAP(CDlgAddGridVal, CDialogEx)
END_MESSAGE_MAP()


// CDlgAddGridVal message handlers
