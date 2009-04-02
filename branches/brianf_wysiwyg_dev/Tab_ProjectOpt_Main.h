#pragma once

#include "TabPageSSL.h"
#include "afxwin.h"

// CTab_ProjectOpt_Main dialog

class CTab_ProjectOpt_Main : public CTabPageSSL
{
	DECLARE_DYNAMIC(CTab_ProjectOpt_Main)

	friend class CTab_ProjectOpt_Thermal;

private:
	BOOL m_folder_changed;
	BOOL m_folder_has_focus;

    void UpdateProjectFolder();

public:
	CTab_ProjectOpt_Main(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTab_ProjectOpt_Main();

	// Dialog Data
	enum { IDD = IDD_TAB_PROJ_OPT_MAIN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitPage(int nPageID);

	DECLARE_MESSAGE_MAP()

	BOOL Verify();
	void OnDDXOut();

	afx_msg void OnEnChangeEditName();
	afx_msg void OnEnChangeEditFolder();
	afx_msg void OnEnKillfocusEditFolder();
	afx_msg void OnEnSetfocusEditFolder();
	afx_msg void OnBnClickedButtonLib();
	afx_msg void OnBnClickedConnectSMTPad();
	afx_msg void OnBnClickedCheckAutosave();
	afx_msg void OnBnClickedCheck1();

protected:
	CEdit m_edit_name;
	CEdit m_edit_folder;
	CEdit m_edit_lib_folder;

	CEdit m_edit_layers;
	CEdit m_edit_ratline_w;
	CButton m_check_SMT_connect_copper;
	CEdit m_edit_glue_w;
	CButton m_check_autosave;
	CEdit m_edit_autosave_interval;

public:
	CString m_name;
	CString m_path_to_folder;
	CString m_lib_folder;

    int m_layers;
	int m_ratline_w;
	int m_glue_w;
	BOOL m_bSMT_connect_copper;
	int m_auto_interval;
};
