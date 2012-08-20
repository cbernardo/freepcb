#pragma once
#include "afxcmn.h"
#include "Net.h"
#include "afxwin.h"


// CDlgNetlist dialog

class CDlgNetlist : public CDialog
{
	DECLARE_DYNAMIC(CDlgNetlist)

public:
	CDlgNetlist(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgNetlist();

// Dialog Data
	enum { IDD = IDD_NET_LIST };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	int m_item_selected;
	CArray<int> *m_w;		// array of default widths
	CArray<int> *m_v_w;		// array of via widths (matching m_w[] entries)
	CArray<int> *m_v_h_w;	// array of via hole widths
	cnetlist * m_nlist;
	cpartlist * m_plist;
	netlist_info * m_nli;

	void Initialize( cnetlist * nlist, cpartlist * plist,
		CArray<int> * w, CArray<int> * v_w, CArray<int> * v_h_w ); 
private:
	int m_sort_type;
	CListCtrl m_list_ctrl;
	virtual BOOL OnInitDialog();
	void CDlgNetlist::DrawListCtrl();
	afx_msg void OnLvnColumnclickListNet(NMHDR *pNMHDR, LRESULT *pResult);
	CButton m_button_visible;
	CButton m_button_delete_single;
	CButton m_button_delete;
	CButton m_button_edit;
	CButton m_button_add;
	CButton m_button_nl_width;
	CButton m_button_combine;					// CPT2 added
	CButton m_button_select_all;
	CButton m_OK;
	CButton m_cancel;
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonDelete();
	afx_msg void OnBnClickedButtonVisible();
	afx_msg void OnBnClickedButtonEdit();
	afx_msg void OnBnClickedButtonSelectAll();
	afx_msg void OnBnClickedButtonNLWidth();
	afx_msg void OnBnClickedButtonCombine();	// CPT2 added
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedDeleteNetsWithNoPins();
	afx_msg void OnNMClickListNet(NMHDR *pNMHDR, LRESULT *pResult);
};
