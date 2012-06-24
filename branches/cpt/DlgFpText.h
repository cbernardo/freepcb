#pragma once
#include "afxwin.h"


// CDlgFpText dialog

class CDlgFpText : public CDialog
{
	DECLARE_DYNAMIC(CDlgFpText)

public:
	CDlgFpText(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgFpText();
	void Initialize( BOOL bDrag, BOOL bFixedString,
		CString * str, int layer, int units, 
		int angle, int height, int width, int x, int y, BOOL bForbidZeroHeight = false );	// CPT added last param
	int Layer2LayerIndex( int layer);
	int LayerIndex2Layer( int layer_index );

// Dialog Data
	enum { IDD = IDD_FP_TEXT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	void SetFields();
	void GetFields();
	BOOL m_bNewText;
	BOOL m_bFixedString;		// if TRUE, can't edit string
	BOOL m_bForbidZeroHeight;	// CPT added
	BOOL m_bDrag;		// 1 if dragging to position
	int m_x, m_y;		// set on entry if editing
	int m_width;
	int m_height;
	int m_angle;
	int m_units;
	int m_unit_mult;
	int m_layer;
	CString m_str;
	virtual BOOL OnInitDialog();
	CEdit m_edit_height;
	CButton m_button_set_width;
	CButton m_button_def_width;
	CEdit m_edit_width;
	CEdit m_text;
	CEdit m_edit_y;
	CEdit m_edit_x;
	CButton m_button_set_position;
	CListBox m_list_angle;
	CButton m_button_drag;
	afx_msg void OnBnClickedSetPosition();
	afx_msg void OnBnClickedDrag();
	afx_msg void OnEnChangeEditHeight();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedSetWidth();
	afx_msg void OnBnClickedDefWidth();
	CComboBox m_combo_units;
	afx_msg void OnCbnSelchangeComboAddTextUnits();
	CComboBox m_combo_layer;
};
