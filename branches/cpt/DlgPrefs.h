#pragma once
#include "afxwin.h"
#include "afxdialogex.h"


// CDlgPrefs dialog.  ALL CPT

class CDlgPrefs : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgPrefs)

protected:
	CButton m_check_autosave;
	CEdit m_edit_auto_interval;
	CButton m_check_reverse;
	CButton m_check_lefthanded;
	CButton m_check_error_sound;		// CPT2 new
	CButton m_check_drag_no_sides;		// CPT2 new
	CButton m_check_disable_auto_rats;
	CEdit m_edit_min_pins;
	CButton m_check_highlight_net;
	CEdit m_edit_defaultcfg;
	CStatic m_static_warnings;
	CButton m_check_warnings;
public:
	bool m_bReverse, m_bLefthanded, m_bHighlightNet, m_bErrorSound, m_bDragNoSides;
	int m_auto_interval;
	bool m_bAuto_Ratline_Disable;
	int m_auto_ratline_min_pins;
	CString m_defaultcfg_dir;			// CPT2 new
	bool m_bReinstateWarnings;
	CFreePcbDoc *doc;

public:
	CDlgPrefs(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgPrefs();
	void Init(bool bReverse, bool bLefthanded, bool bHighlightNet, bool bDragNoSides, bool bErrorSound, int auto_interval, 
		bool bAuto_Ratline_Disable, int auto_ratline_min_pins);

// Dialog Data
	enum { IDD = IDD_PREFS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEditAutoInterval();
	afx_msg void OnBnClickedCheckAutosave();
	afx_msg void OnBnClickedCheckAutoRatDisable();
	afx_msg void OnBnClickedBrowse();
};
