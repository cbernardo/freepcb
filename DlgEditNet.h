#pragma once
#include "afxwin.h"
#include "NetListNew.h"


// CDlgEditNet dialog

class CDlgEditNet : public CDialog
{
	DECLARE_DYNAMIC(CDlgEditNet)

public:
	CDlgEditNet(CWnd* pParent = NULL); 
	virtual ~CDlgEditNet();
	void Initialize( netlist_info * nli,	// netlist_info struct
				int i,					// index into nli (ignored if new net)
				cpartlist * plist,		// partlist
				BOOL new_net,			// flag for new net
				BOOL visible,			// visibility flag
				int units,				// MIL or MM
				CArray<int> * w,		// array of default trace widths
				CArray<int> * v_w,		// array of default via widths
				CArray<int> * v_h_w );	// array of default via hole widths
	void SetFields();
	void GetFields();
// Dialog Data
	enum { IDD = IDD_EDIT_NET };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
	CString m_name;
	BOOL m_new_net;
	CArray<net_info> *m_nli;
	cpartlist * m_plist;
	int m_in;
	BOOL m_visible;
	int m_def_w;
	int m_def_v_w;
	int m_def_v_h_w;
	// pointers to arrays of default trace and via widths
	CArray<int> *m_w;
	CArray<int> *m_v_w;	
	CArray<int> *m_v_h_w;
protected:
	int m_units;
	BOOL m_pins_edited;
	CButton m_button_def_width;
	CButton m_button_set_width;
	CEdit m_edit_pad_w;
	CEdit m_edit_hole_w;
	CListBox m_list_pins;
	afx_msg void OnBnClickedButtonDelete();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedRadioDefWidth();
	afx_msg void OnBnClickedRadioSetWidth();
	afx_msg void OnEnChangeEditWidth();
	void ChangeTraceWidth( CString str );
	virtual BOOL OnInitDialog();
	CEdit m_edit_name;
	CButton m_check_visible;
	afx_msg void OnCbnSelchangeComboWidth();
	afx_msg void OnCbnEditchangeComboWidth();
	CComboBox m_combo_width;
	CEdit m_edit_addpin;
public:
	CButton m_check_apply;
	afx_msg void OnEnUpdateEditAddPin();
	CButton m_button_add_pin;
	CButton m_button_OK;
	afx_msg void OnBnClickedOk();
};
