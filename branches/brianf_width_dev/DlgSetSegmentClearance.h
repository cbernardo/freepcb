#pragma once
#include "afxwin.h"
#include "SubdlgClearance.h"

// DlgSetSegmentClearance dialog

class DlgSetSegmentClearance :	
	public CDialog
	, public CSubDlg_Clearance
{
	DECLARE_DYNAMIC(DlgSetSegmentClearance)

public:
	DlgSetSegmentClearance(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgSetSegmentClearance();

// Dialog Data
	enum { IDD = IDD_SEGMENT_CLEARANCE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	// Mapping functions to mix-in classes CSubDlg...
	afx_msg void OnBnClicked_c_Default() { CSubDlg_Clearance::OnBnClicked_c_Default(); }
	afx_msg void OnBnClicked_c_Set()     { CSubDlg_Clearance::OnBnClicked_c_Set();     }


public:
	// these variables should be set on entry
	int m_mode;                 // 0=segment, 1=connection, 2=net
	CClearanceInfo m_clearance; // clearance

	// these variables will be set on exit
	int m_def;		    // set default clearance (1=con, 2=net)
	int m_apply;	    // apply clearance (1=seg, 2=con, 3=net)

	CClearanceInfo m_clearance_pinvia; // pin/via clearance

protected:
	CButton m_set_net_default;

	CButton m_radio2_apply_seg;
	CButton m_radio2_apply_con;
	CButton m_radio2_apply_net;

	CButton m_radio3_no;
	CButton m_radio3_auto;
	CButton m_radio3_as_sel;

	afx_msg void OnBnClickedApplySeg();
	afx_msg void OnBnClickedApplyCon();
	afx_msg void OnBnClickedApplyNet();
};
