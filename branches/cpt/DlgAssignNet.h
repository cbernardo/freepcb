#pragma once
#include "NetListNew.h"

// DlgAssignNet dialog

class DlgAssignNet : public CDialog
{
	DECLARE_DYNAMIC(DlgAssignNet)

public:
	DlgAssignNet(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgAssignNet();
	CString m_net_str;
	cnetlist *m_nlist;
	CComboBox m_combo_net;

// Dialog Data
	enum { IDD = IDD_ASSIGN_NET };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	CString created_name;
	BOOL m_bCreated;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonNewNet();
	afx_msg void OnBnClickedCancel();
};
