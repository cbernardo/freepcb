#pragma once
#include "afxwin.h"
#include "afxtempl.h"
#include "SubdlgTraceWidth.h"
#include "SubdlgViaWidth.h"

// DlgSetSegmentWidth dialog

class DlgSetSegmentWidth
	: public CDialog
	, public CSubDlg_TraceWidth
	, public CSubDlg_ViaWidth
{
	DECLARE_DYNAMIC(DlgSetSegmentWidth)

public:
	DlgSetSegmentWidth(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgSetSegmentWidth();

// Dialog Data
	enum { IDD = IDD_SEGMENT_WIDTH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	// Mapping functions to mix-in classes CSubDlg...
	afx_msg void OnBnClicked_t_modify()      { CSubDlg_TraceWidth::OnBnClicked_t_modify();    }
	afx_msg void OnBnClicked_t_Default()     { CSubDlg_TraceWidth::OnBnClicked_t_Default();	  }
	afx_msg void OnBnClicked_t_Set()	     { CSubDlg_TraceWidth::OnBnClicked_t_Set();		  }
	afx_msg void OnCbnSelchange_t_width()    { CSubDlg_TraceWidth::OnCbnSelchange_t_width();  }
	afx_msg void OnCbnEditchange_t_width()   { CSubDlg_TraceWidth::OnCbnEditchange_t_width(); }

	afx_msg void OnBnClicked_v_modify()		 { CSubDlg_ViaWidth::OnBnClicked_v_modify();	  }
	afx_msg void OnBnClicked_v_Default()	 { CSubDlg_ViaWidth::OnBnClicked_v_Default();	  }
	afx_msg void OnBnClicked_v_DefForTrace() { CSubDlg_ViaWidth::OnBnClicked_v_DefForTrace(); }
	afx_msg void OnBnClicked_v_Set()		 { CSubDlg_ViaWidth::OnBnClicked_v_Set();		  }


public:
	// these variables should be set on entry
	int m_mode;		// 0=segment, 1=connection, 2=net

	// these variables will be set on exit
	int m_def;		// set default width for (0=none, 2=net)
	int m_apply;	// apply width to (1=seg, 2=con, 3=net)

	CConnectionWidthInfo m_width_attrib; // trace width info

protected:
	virtual BOOL OnInitDialog();

	CButton m_set_net_default;

	CButton m_apply_net;
	CButton m_apply_con;
	CButton m_apply_seg;
};
