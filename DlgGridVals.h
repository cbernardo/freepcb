#pragma once
#include "afxwin.h"


// CDlgGridVals dialog.  ALL CPT

class CDlgGridVals : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgGridVals)

public:
	CDlgGridVals(CArray<double> *arr, CArray<double> *hidden, int titleRsrc, CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgGridVals();

// Dialog Data
	enum { IDD = IDD_GRIDVALS };
	BOOL bSetDefault;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	CButton m_button_add, m_button_delete;
	CListCtrl m_list_ctrl;
	CButton m_check_default;
	CArray<double> *arr, *arrCopy;
	CArray<double> *hidden, *hiddenCopy;
	CString title;

	void DrawListCtrl();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedAdd();
	afx_msg void OnBnClickedDelete();
	afx_msg void OnNMClickListNet(NMHDR *pNMHDR, LRESULT *pResult);
};
