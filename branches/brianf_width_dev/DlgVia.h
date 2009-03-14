#pragma once
#include "afxwin.h"


// CDlgVia dialog

class CDlgVia : public CDialog
{
	DECLARE_DYNAMIC(CDlgVia)

public:
	CDlgVia(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgVia();
	void Initialize( CConnectionWidthInfo const &via_width );

// Dialog Data
	enum { IDD = IDD_VIA };

	CConnectionWidthInfo m_via_width;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	CEdit m_edit_via_w;
	CEdit m_edit_hole_w;

	CButton m_radio_proj_def;
	CButton m_radio_set_to;

	afx_msg void OnBnClickedRadio1ProjDef();
	afx_msg void OnBnClickedRadio1SetTo();
};
