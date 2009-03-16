#pragma once
#include "afxwin.h"
#include "NetList.h"


// CDlgEditNet dialog

class CDlgEditNet : public CDialog
{
	DECLARE_DYNAMIC(CDlgEditNet)

public:
	CDlgEditNet(CWnd* pParent = NULL);
	virtual ~CDlgEditNet();
	void Initialize(
				CNetList const * netlist, // For parent info for widths/clearance
				netlist_info * nl,	 	  // netlist_info struct
				int i,					  // index into nl (ignored if new net)
				CPartList * plist,		  // partlist
				BOOL new_net,			  // flag for new net
				BOOL visible,			  // visibility flag
				int units,				  // MIL or MM
				CArray<int> * w,          // array of default trace widths
				CArray<int> * v_w,        // array of default via widths
				CArray<int> * v_h_w );    // array of default via hole widths
	void SetFields();
	void GetFields();
// Dialog Data
	enum { IDD = IDD_EDIT_NET };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
	CString m_name;
	BOOL m_new_net;
	CArray<net_info> *m_nl;
	CPartList * m_plist;
	int m_in;
	BOOL m_visible;

	CNetWidthInfo m_width_attrib;

	// pointers to arrays of default trace and via widths
	CArray<int> *m_w;
	CArray<int> *m_v_w;
	CArray<int> *m_v_h_w;

protected:
	int m_units;
	BOOL m_pins_edited;
	afx_msg void OnEnChangeEditWidth();
	virtual BOOL OnInitDialog();

	CEdit m_edit_name;
	CButton m_check_visible;

	CButton m_radio1_default;
	CButton m_radio1_set;
	CComboBox m_combo_width;
	CButton m_check_width_apply;
	afx_msg void OnCbnSelchangeComboWidth();
	afx_msg void OnCbnEditchangeComboWidth();
	afx_msg void OnBnClickedRadio1ProjDef();
	afx_msg void OnBnClickedRadio1SetTo();
	void ChangeTraceWidth( int new_w );

	CButton m_radio2_default;
	CButton m_radio2_def_for_width;
	CButton m_radio2_set;
	CEdit m_edit_pad_w;
	CEdit m_edit_hole_w;
	CButton m_check_via_apply;
	afx_msg void OnBnClickedRadio2DefWidth();
	afx_msg void OnBnClickedRadio2DefForTrace();
	afx_msg void OnBnClickedRadio2SetWidth();

	CButton m_radio3_default;
	CButton m_radio3_set;
	CEdit m_edit_clearance;
	CButton m_check_clearance_apply;
	afx_msg void OnBnClickedRadioDefClearance();
	afx_msg void OnBnClickedRadioSetClearance();

	CListBox m_list_pins;
	CButton m_button_add_pin;
	CEdit m_edit_addpin;
	afx_msg void OnEnUpdateEditAddPin();
	afx_msg void OnBnClickedButtonDelete();
	afx_msg void OnBnClickedButtonAdd();

	CButton m_button_OK;
	afx_msg void OnBnClickedOk();
};
