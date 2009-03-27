#pragma once
#include "afxwin.h"
#include "SubdlgViaWidth.h"
#include "SubdlgClearance.h"


// CDlgViaPinSize dialog

class CDlgViaPinSize 
	: public CDialog
	, public CSubDlg_ViaWidth
	, public CSubDlg_Clearance
{
	DECLARE_DYNAMIC(CDlgViaPinSize)

public:
	CDlgViaPinSize(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgViaPinSize();
	void Initialize( CInheritableInfo const &via_width );

// Dialog Data
	enum { IDD = IDD_VIA_PIN_SIZE };

	CViaWidthInfo m_via_width;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClicked_v_modify()		 { CSubDlg_ViaWidth::OnBnClicked_v_modify();   }
	afx_msg void OnBnClicked_v_Default()	 { CSubDlg_ViaWidth::OnBnClicked_v_Default();  }
	afx_msg void OnBnClicked_v_Set()		 { CSubDlg_ViaWidth::OnBnClicked_v_Set();      }

	afx_msg void OnBnClicked_c_modify()		 { CSubDlg_Clearance::OnBnClicked_c_modify();  }
	afx_msg void OnBnClicked_c_Default()     { CSubDlg_Clearance::OnBnClicked_c_Default(); }
	afx_msg void OnBnClicked_c_Set()         { CSubDlg_Clearance::OnBnClicked_c_Set();     }
};
