#pragma once
#include "afxwin.h"
#include "resource.h"
#include "SubdlgTraceWidth.h"
#include "SubdlgViaWidth.h"
#include "SubdlgClearance.h"

// CDlgSetTraceWidths dialog

class CDlgSetTraceWidths
	: public CDialog
	, public CSubDlg_TraceWidth
	, public CSubDlg_ViaWidth
	, public CSubDlg_Clearance
{
	DECLARE_DYNAMIC(CDlgSetTraceWidths)

public:
	CDlgSetTraceWidths(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSetTraceWidths();

// Dialog Data
	enum { IDD = IDD_SET_TRACE_WIDTHS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	// Mapping functions to mix-in classes CSubDlg...
	afx_msg void OnBnClicked_t_modify()      { CSubDlg_TraceWidth::OnBnClicked_t_modify();    }
	afx_msg void OnBnClicked_t_Default()     { CSubDlg_TraceWidth::OnBnClicked_t_Default();	  }
	afx_msg void OnBnClicked_t_Set()	     { CSubDlg_TraceWidth::OnBnClicked_t_Set();		  }
	afx_msg void OnCbnSelchange_t_width()    { CSubDlg_TraceWidth::OnCbnSelchange_t_width();  }
	afx_msg void OnCbnEditchange_t_width()   { CSubDlg_TraceWidth::OnCbnEditchange_t_width(); }

	afx_msg void OnBnClicked_v_modify()      { CSubDlg_ViaWidth::OnBnClicked_v_modify();      }
	afx_msg void OnBnClicked_v_Default()	 { CSubDlg_ViaWidth::OnBnClicked_v_Default();	  }
	afx_msg void OnBnClicked_v_DefForTrace() { CSubDlg_ViaWidth::OnBnClicked_v_DefForTrace(); }
	afx_msg void OnBnClicked_v_Set()		 { CSubDlg_ViaWidth::OnBnClicked_v_Set();		  }

	afx_msg void OnBnClicked_c_modify()      { CSubDlg_Clearance::OnBnClicked_c_modify();     }
	afx_msg void OnBnClicked_c_Default()     { CSubDlg_Clearance::OnBnClicked_c_Default();    }
	afx_msg void OnBnClicked_c_Set()         { CSubDlg_Clearance::OnBnClicked_c_Set();        }

public:
	CNetWidthInfo m_width_attrib;

	int m_apply_trace;
	int m_apply_via;
    int m_apply_clearance;
};
