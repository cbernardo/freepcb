#pragma once
#include "afxwin.h"
#include "afxdialogex.h"


// CDlgPrefs dialog.  ALL CPT

class CDlgPrefs : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgPrefs)

public:
	CDlgPrefs(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgPrefs();
	void Init(bool bReverse, bool bLefthanded, BOOL bHighlightNet, int auto_interval, 
		BOOL bAuto_Ratline_Disable, int auto_ratline_min_pins);

// Dialog Data
	enum { IDD = IDD_PREFS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedCheckAutosave();
	CButton m_check_autosave;
	CEdit m_edit_auto_interval;
	CButton m_check_reverse;
	CButton m_check_lefthanded;
	CButton m_check_disable_auto_rats;
	CEdit m_edit_min_pins;
	CButton m_check_highlight_net;
	afx_msg void OnBnClickedCheckAutoRatDisable();
public:
	bool m_bReverse, m_bLefthanded, m_bHighlightNet;
	int m_auto_interval;
	BOOL m_bAuto_Ratline_Disable;
	int m_auto_ratline_min_pins;
	CFreePcbDoc *doc;
};
