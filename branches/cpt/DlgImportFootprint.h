#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// CDlgImportFootprint dialog

class CDlgImportFootprint : public CDialog
{
	DECLARE_DYNAMIC(CDlgImportFootprint)

public:
	CDlgImportFootprint(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgImportFootprint();
	void InitPartLibTree();
	void InitInstance( cshapelist * cache_shapes,
					   CFootLibFolderMap * foldermap,
					   CDlgLog * log );

// Dialog Data
	enum { IDD = IDD_IMPORT_FOOTPRINT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CDlgLog * m_dlg_log;
	cshapelist * m_cache_shapes;
	CFootLibFolderMap * m_foldermap;
	CFootLibFolder * m_footlibfolder;
	CString m_footprint_name;
	CString m_footprint_filename;
	CString m_footprint_folder;
	cshape *m_shape;					// CPT2 was CShape, changed to cshape* (otherwise initialization doesn't work right)
	BOOL m_in_cache;
	int m_ilib;							// indices into libraries
	int m_ihead;
	int m_ifoot;

	CButton m_button_browse;
	CEdit m_edit_library_folder;
	CTreeCtrl part_tree;
	CStatic m_preview;

private:
	virtual BOOL OnInitDialog();
	afx_msg void OnTvnSelchangedPartLibTree(NMHDR *pNMHDR, LRESULT *pResult);
public:
	afx_msg void OnBnClickedButtonBrowseLibFolder();
	CEdit m_edit_author;
	CEdit m_edit_source;
	CEdit m_edit_desc;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
