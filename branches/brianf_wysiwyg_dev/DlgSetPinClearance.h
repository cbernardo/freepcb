#pragma once
#include "afxwin.h"


// CDlgSetPinClearance dialog

class CDlgSetPinClearance : public CDialog
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

	CButton m_clearance_auto;
	CButton m_clearance_net;
	CButton m_clearance_set;
	CEdit m_edit_clearance;

	CButton m_apply_this_only;
	CButton m_apply_con;
	CButton m_apply_net;

	afx_msg void OnBnClickedRadioPinAutoClearance();
	afx_msg void OnBnClickedRadioPinNetClearance();
	afx_msg void OnBnClickedRadioPinSetClearance();

	void OnUpdateClearanceType();

public:
	// these variables should be set on entry
	CClearanceInfo m_clearance; // clearance

	// these variables will be set on exit
	int m_apply;	 // apply clearance (1=pin/via, 2=con, 3=net)
};
