#pragma once
#include "afxwin.h"


// CDlgVia dialog

class CDlgVia : public CDialog
{
	DECLARE_DYNAMIC(CDlgVia)

public:
	CDlgVia(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgVia();
	void Initialize( int via_w, int via_hole_w, CPoint pt, int units );
	void GetFields();
	void SetFields();
	CPoint pt(){ return m_pt; };

// Dialog Data
	enum { IDD = IDD_VIA };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_combo_units;
	CEdit m_edit_via_w;
	CEdit m_edit_hole_w;

	int m_units;
	int m_via_w, m_via_hole_w;
	CPoint m_pt;

public:
	afx_msg void OnCbnSelChangeComboViaUnits();
public:
	CEdit m_edit_x;
public:
	CEdit m_edit_y;
};
