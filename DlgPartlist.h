#pragma once
#include "afxcmn.h"
#include "afxwin.h"

// CDlgPartlist dialog

class CMyListCtrl: public CListCtrl
{
	// CPT2 new class that permits the custom drawing of CDlgPartlist's main list.  I wanted two checkboxes per row, one for ref visibility
	// and one for value visibility, and this appears to be the only way to do it.
	DECLARE_DYNAMIC(CMyListCtrl)
public:
	CMyListCtrl() { }
	virtual ~CMyListCtrl() { }

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
};



class CDlgPartlist : public CDialog
{
	DECLARE_DYNAMIC(CDlgPartlist)

public:
	// Dialog Data
	enum { IDD = IDD_PARTLIST };
	CPartList * m_plist;
	CShapeList * m_cache_shapes;
	CFootLibFolderMap * m_footlibfoldermap;
	int m_units;
	int m_sort_type;
	CMyListCtrl m_list_ctrl;
	CButton m_button_ok;
	CButton m_button_add;
	CButton m_button_edit;
	CButton m_button_delete;
	CDlgLog * m_dlg_log;
	CArray<BOOL> bSelected;
	CButton m_check_footprint;
	CButton m_check_package;
	CButton m_check_value;
	static partlist_info pli;												// CPT this was in the global namespace, but it seemed better in here
																			// (e.g. CFreePcbDoc::OnProjectPartlist() now accesses it directly)

	CDlgPartlist(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgPartlist();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	void DrawListCtrl(bool bSetup);						// CPT2 added arg
	void Initialize( CPartList * plist,
			CShapeList * cache_shapes,
			CFootLibFolderMap * footlibfoldermap,
			int units, CDlgLog * log );
	void SaveSelections();
	void RestoreSelections();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonEdit();
	afx_msg void OnBnClickedButtonDelete();
	afx_msg void OnLvnColumnClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedValueVisible();
	afx_msg void OnBnClickedValueInvisible();
	afx_msg void OnBnClickedRefVisible();
	afx_msg void OnBnClickedRefInvisible();
	void OnVisibleButton(bool bVis, bool bValue);					// CPT2 The previous 4 are wrappers around this base routine
	afx_msg void OnNMClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblClickList1(NMHDR *pNMHDR, LRESULT *pResult);
};


