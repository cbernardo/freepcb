#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include <afxcoll.h>
#include <afxtempl.h>


// CDlgProjectOptions dialog

class CDlgProjectOptions : public CDialog
{
	DECLARE_DYNAMIC(CDlgProjectOptions)

public:
	CDlgProjectOptions(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgProjectOptions();
	void Init( BOOL new_flag,
		CString * name,
		CString * path_to_folder,
		CString * lib_folder,
		int num_layers,
		BOOL bSMT_connect_copper,
		int glue_w,
		int trace_w,
		int via_w,
		int hole_w,
		CArray<int> * w,
		CArray<int> * v_w,
		CArray<int> * v_h_w );
	CString GetName(){ return m_name; };
	CString GetPathToFolder(){ return m_path_to_folder; };
	CString GetLibFolder(){ return m_lib_folder; };
	int GetNumCopperLayers(){ return m_layers; };
	int GetTraceWidth(){ return m_trace_w; };
	int GetViaWidth(){ return m_via_w; };
	int GetViaHoleWidth(){ return m_hole_w; };
	int GetGlueWidth(){ return m_glue_w; };

// Dialog Data
	enum { IDD = IDD_PROJECT_OPTIONS  };

private:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	BOOL m_new_project;
	BOOL m_folder_changed;
	BOOL m_folder_has_focus;
	CString m_name;
	CString m_path_to_folder;
	CString m_lib_folder;
	int m_glue_w;
	int m_trace_w;
	int m_via_w;
	int m_hole_w;
	DECLARE_MESSAGE_MAP()
	CListCtrl m_list_menu;
	CArray<int> *m_w;		
	CArray<int> *m_v_w;	
	CArray<int> *m_v_h_w;	
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonEdit();
	afx_msg void OnBnClickedButtonDelete();
	afx_msg void OnEnChangeEditName();
	afx_msg void OnEnChangeEditFolder();
	int m_layers;
	CEdit m_edit_name;
	CEdit m_edit_folder;
	afx_msg void OnEnSetfocusEditFolder();
	afx_msg void OnEnKillfocusEditFolder();
	CEdit m_edit_lib_folder;
	CEdit m_edit_layers;
	CButton m_button_proj;
	CButton m_check_default;				// CPT;
public:
	BOOL m_bSMT_connect_copper;
	afx_msg void OnBnClickedButtonLib();
	afx_msg void OnBnClickedButtonProj();
	CButton m_check_SMT_connect;
	BOOL m_default;							// CPT
};
