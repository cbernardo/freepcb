#pragma once
#include "afxwin.h"


// CDlgAddPoly dialog

class CDlgAddPoly : public CDialog
{
	DECLARE_DYNAMIC(CDlgAddPoly)

public:
	CDlgAddPoly(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAddPoly();
	void Initialize( int units );
	int GetWidth(){ return m_width; };
	int GetClosedFlag(){ return m_closed_flag; };

// Dialog Data
	enum { IDD = IDD_ADD_POLY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	void SetFields();
	void GetFields();
	CButton m_radio_open;
	CButton m_radio_closed;
	CEdit m_edit_width;
	BOOL m_closed_flag;
	int m_units;
	int m_width;
	CComboBox m_combo_units;
	afx_msg void OnCbnSelchangeComboAddPolyUnits();
public:
};
