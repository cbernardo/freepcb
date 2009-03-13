#pragma once
#include "afxwin.h"
#include "resource.h"


// CDlgSetTraceWidths dialog

class CDlgSetTraceWidths : public CDialog
{
	DECLARE_DYNAMIC(CDlgSetTraceWidths)

public:
	CDlgSetTraceWidths(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSetTraceWidths();

// Dialog Data
	enum { IDD = IDD_SET_TRACE_WIDTHS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	CNetWidthInfo m_width_attrib;

	BOOL m_apply_trace;
	BOOL m_apply_via;
	BOOL m_apply_clearance;
	BOOL m_apply_to_routed;

	CArray<int> *m_w;
	CArray<int> *m_v_w;
	CArray<int> *m_v_h_w;

private:
	BOOL bTraces;
	BOOL bRevertTraces;
	BOOL bVias;
	BOOL bDefaultVias;
	BOOL bRevertVias;

protected:
	CComboBox m_combo_width;
	CButton m_radio_default_via_for_trace;
	CEdit m_edit_via_pad;
	CEdit m_edit_via_hole;
	afx_msg void OnBnClickedRadioDef();
	afx_msg void OnBnClickedRadioSet();
	afx_msg void OnCbnSelchangeComboWidth();
	afx_msg void OnCbnEditchangeComboWidth();
	CButton m_check_apply;
	CButton m_check_trace;
	CButton m_check_vias;
	afx_msg void OnBnClickedSetTrace();
	afx_msg void OnBnClickedSetVias();
	void SetFields();
	CButton m_radio_set_via_width;
	CButton m_radio_revert_traces;
	afx_msg void OnBnClickedRadioRevertTraces();
	CButton m_radio_revert_vias;
	afx_msg void OnBnClickedRadioRevertVias();
	afx_msg void OnBnClickedRadioSetTraceWidth();
	CButton m_radio_set_trace_width;

	// Clearance
	CButton m_check_clearance;
	CButton m_radio3_def_clearance;
	CButton m_radio3_set_clearance;
	CEdit m_edit_clearance;
	afx_msg void OnBnClickedRadioRevertTraceClearance();
	afx_msg void OnBnClickedRadioSetTraceClearance();
	afx_msg void OnBnClickedModClearance();
};
