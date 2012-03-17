#pragma once
#include "afxwin.h"


// CDlgImportOptions dialog

class CDlgImportOptions : public CDialog
{
	DECLARE_DYNAMIC(CDlgImportOptions)

public:
	CDlgImportOptions(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgImportOptions();
	void Initialize( int flags );

// Dialog Data
	enum { IDD = IDD_IMPORT_OPTIONS };
	enum { FREEPCB, PADSPCB }; 

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	int m_flags;
	int m_format;
	CButton m_radio_remove_parts;
	CButton m_radio_keep_parts_no_connections;
	CButton m_radio_keep_parts_and_connections;
	CButton m_radio_change_fp;
	CButton m_radio_keep_fp;
	CButton m_radio_remove_nets;
	CButton m_radio_keep_nets;
	CButton m_check_keep_traces;
	CButton m_check_keep_stubs;
	CButton m_check_keep_areas;
	CButton m_button_save_and_import;
	// CPT added (used to be in file-open dialog):
	CButton m_radio_parts;
	CButton m_radio_nets;
	CButton m_radio_parts_and_nets;
	CButton m_radio_padspcb;
	CButton m_radio_freepcb;

	afx_msg void OnBnClickedSaveAndImport();
	afx_msg void OnBnClickedOk();
	void SetPartsNetsFlags();
	void EnableDisableButtons();
	afx_msg void OnBnClickedPartsAndNets();
};
