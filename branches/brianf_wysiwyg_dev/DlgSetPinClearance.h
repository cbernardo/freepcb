#pragma once
#include "afxwin.h"
#include "SubdlgClearance.h"

// CDlgSetPinClearance dialog

class CDlgSetPinClearance
	: public CDialog
	, public CSubDlg_Clearance
{
	DECLARE_DYNAMIC(CDlgSetPinClearance)

public:
	CDlgSetPinClearance(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSetPinClearance();

// Dialog Data
	enum { IDD = IDD_PIN_CLEARANCE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	// Mapping functions to mix-in classes CSubDlg...
	afx_msg void OnBnClicked_c_Default() { CSubDlg_Clearance::OnBnClicked_c_Default(); }
	afx_msg void OnBnClicked_c_Auto()    { CSubDlg_Clearance::OnBnClicked_c_Auto();    }
	afx_msg void OnBnClicked_c_Set()     { CSubDlg_Clearance::OnBnClicked_c_Set();     }

	CButton m_apply_this_only;
	CButton m_apply_con;
	CButton m_apply_net;

public:
	// these variables should be set on entry
	CClearanceInfo m_clearance; // clearance

	// Set the following on entry
	int m_allow_automode;

	// these variables will be set on exit
	int m_apply;	 // apply clearance to (1=pin/via, 2=con, 3=net)
};
