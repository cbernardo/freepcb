#pragma once

#include "TabPageSSL.h"
#include "afxwin.h"
#include "afxcmn.h"

// CTab_ProjectOpt_Spacing dialog

class CTab_ProjectOpt_Spacing : public CTabPageSSL
{
	DECLARE_DYNAMIC(CTab_ProjectOpt_Spacing)

private:
	static BOOL m_bShowMessageForClearance;

public:
	CTab_ProjectOpt_Spacing(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTab_ProjectOpt_Spacing();

	// Dialog Data
	enum { IDD = IDD_TAB_PROJ_OPT_SPACING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitPage(int nPageID);

	DECLARE_MESSAGE_MAP()

	BOOL Verify();
	void OnDDXOut();

	afx_msg void OnBnClickedButtonDelete();
	afx_msg void OnBnClickedButtonEdit();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnNMDblclkListWidthMenu(NMHDR *pNMHDR, LRESULT *pResult);

	CEdit m_edit_def_tracew;
	CEdit m_edit_def_viapad;
	CEdit m_edit_def_viahole;

	CEdit m_edit_def_cac_trace;
	CEdit m_edit_def_cac_holeedge;

	CListCtrl m_list_menu;

public:
	int m_trace_w;
	int m_ca_clearance_trace;
	int m_ca_clearance_hole;
	int m_via_w;
	int m_hole_w;

	CArray<int> *m_w;
	CArray<int> *m_v_w;
	CArray<int> *m_v_h_w;
};
