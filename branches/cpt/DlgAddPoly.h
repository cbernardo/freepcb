#pragma once
#include "afxwin.h"


// CDlgAddPoly dialog

class CDlgAddPoly : public CDialog
{
	DECLARE_DYNAMIC(CDlgAddPoly)

public:
	CDlgAddPoly(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAddPoly();
	void Initialize( BOOL bNewPoly, int layer_index, int units,
		int width, BOOL bClosed, CArray<padstack> * padstack );
	int GetWidth(){ return m_width; };
	int GetLayerIndex(){ return m_layer_index; };
	int GetClosedFlag(){ return m_closed_flag; };
	CString GetPinName(){ return m_pin_name; };

// Dialog Data
	enum { IDD = IDD_ADD_POLY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	CArray<padstack> * m_padstack;
	void SetFields();
	void GetFields();
	BOOL m_bNewPoly;
	CButton m_radio_open;
	CButton m_radio_closed;
	CEdit m_edit_width;
	BOOL m_closed_flag;
	int m_units;
	int m_width;
	int m_layer_index;
	BOOL m_bClosed;
	CComboBox m_combo_units;
	afx_msg void OnCbnSelchangeComboAddPolyUnits();
	CComboBox m_combo_layer;
	CButton m_radio_closed_and_filled;
	CComboBox m_combo_pin_name;
	CString m_pin_name;
	afx_msg void OnCbnSelchangeComboLayer();
};
