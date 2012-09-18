#pragma once
#include "stdafx.h"
#include "DesignRules.h"
#include "afxwin.h"
#include "PcbItem.h"

// DlgDRC dialog

class DlgDRC : public CDialog
{
	DECLARE_DYNAMIC(DlgDRC)

public:
	DlgDRC(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgDRC(); 
	void Initialize( CFreePcbDoc *doc );
	void GetFields();
	void SetFields();
	int CamWarnings();
	afx_msg void OnCbnChangeUnits();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCheck();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()

public:
	enum { IDD = IDD_DRC };
	CFreePcbDoc *m_doc;				// CPT2 Including this member, and deriving other members like m_pl from it.
	int m_units; 
	DesignRules m_dr_local;
	DesignRules * m_dr;
	int m_CAM_annular_ring_pins;
	int m_CAM_annular_ring_vias;
	CComboBox m_combo_units;
	CEdit m_edit_pad_pad;
	CEdit m_edit_pad_trace;
	CEdit m_edit_trace_trace;
	CEdit m_edit_hole_copper;
	CEdit m_edit_annular_ring_pins;
	CEdit m_edit_board_edge_copper;
	CEdit m_edit_hole_hole;
	CEdit m_edit_annular_ring_vias;
	CEdit m_edit_copper_copper;
	CEdit m_edit_trace_width;
	CEdit m_edit_board_edge_hole;
	CButton m_check_show_unrouted;
	bool bCheckOnExit;						// CPT2 new
};
