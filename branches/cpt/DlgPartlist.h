#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "Partlist.h"

// CDlgPartlist dialog

class CDlgPartlist : public CDialog
{
	DECLARE_DYNAMIC(CDlgPartlist)

public:
	CDlgPartlist(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgPartlist();

// Dialog Data
	enum { IDD = IDD_PARTLIST };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	void DrawListCtrl();
	void Initialize( cpartlist * plist,
			CMapStringToPtr * shape_cache_map,
			CFootLibFolderMap * footlibfoldermap,
			int units, CDlgLog * log );
	void SaveSelections();
	void RestoreSelections();
	cpartlist * m_plist;
	CMapStringToPtr * m_footprint_cache_map;
	CFootLibFolderMap * m_footlibfoldermap;
	int m_units;
	int m_sort_type;
	CListCtrl m_list_ctrl;
	CButton m_button_add;
	CButton m_button_edit;
	CButton m_button_delete;
	CDlgLog * m_dlg_log;
	CArray<BOOL> bSelected;
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonEdit();
	afx_msg void OnBnClickedButtonDelete();
	afx_msg void OnLvnColumnClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedValueVisible();
	afx_msg void OnBnClickedValueInvisible();
	CButton m_check_footprint;
	CButton m_check_package;
	CButton m_check_value;
	afx_msg void OnNMClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	static partlist_info pli;												// CPT this was in the global namespace, but it seemed better in here
																			// (e.g. CFreePcbDoc::OnProjectPartlist() now accesses it directly)
};
