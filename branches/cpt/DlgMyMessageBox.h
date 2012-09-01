#pragma once
#include "afxwin.h"


// CDlgMyMessageBox dialog

class CDlgMyMessageBox : public CDialog
{
	DECLARE_DYNAMIC(CDlgMyMessageBox)

public:
	CDlgMyMessageBox(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgMyMessageBox();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
public:
	bool Initialize( int warningId, bool bShowCancel=false, int arg1=0, int arg2=0 );


	enum { IDD = IDD_MY_MESSAGE };
	DECLARE_MESSAGE_MAP()
	CStatic m_message;
	int m_warningId;
	bool m_bShowCancel;
	CButton m_check_dont_show;
	BOOL bDontShowBoxState;
	CString m_mess;
	BOOL m_bHideCursor;
	CButton m_button_cancel, m_button_ok;
};
