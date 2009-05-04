#pragma once

#include "TabPageSSL.h"
#include "afxwin.h"

// CTab_ProjectOpt_Thermal dialog

class CTab_ProjectOpt_Thermal : public CTabPageSSL
{
	DECLARE_DYNAMIC(CTab_ProjectOpt_Thermal)

public:
	CTab_ProjectOpt_Thermal(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTab_ProjectOpt_Thermal();

	// Dialog Data
	enum { IDD = IDD_TAB_PROJ_OPT_THERMAL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitPage(int nPageID);

	DECLARE_MESSAGE_MAP()

	BOOL Verify();
	void OnDDXOut();

protected:
	friend class CTab_ProjectOpt_Main;

	CButton m_check_TR_for_SMT_pads;
	CButton m_check_TR_for_TH_pins;
	CButton m_check_TR_for_vias;
	CButton m_check_90degree_thermals;
	CEdit m_edit_TR_line_w;
	CEdit m_edit_TR_clearance;

public:
	BOOL m_b_TR_for_SMT_pads;
	BOOL m_b_TR_for_TH_pins;
	BOOL m_b_TR_for_vias;
	BOOL m_b_90degree_thermals;

	int m_TR_line_w;
	int m_TR_clearance;
};
