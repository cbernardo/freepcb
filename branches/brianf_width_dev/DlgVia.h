#pragma once
#include "afxwin.h"
#include "SubdlgViaWidth.h"


// CDlgVia dialog

class CDlgVia 
	: public CDialog
	, public CSubDlg_ViaWidth
{
	DECLARE_DYNAMIC(CDlgVia)

public:
	CDlgVia(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgVia();
	void Initialize( CConnectionWidthInfo const &via_width );

// Dialog Data
	enum { IDD = IDD_VIA };

	CConnectionWidthInfo m_via_width;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClicked_v_Default()	 { CSubDlg_ViaWidth::OnBnClicked_v_Default();	  }
	afx_msg void OnBnClicked_v_Set()		 { CSubDlg_ViaWidth::OnBnClicked_v_Set();		  }
};
