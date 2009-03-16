#pragma once
#include "afxwin.h"
#include "NetList.h"
#include "SubdlgTraceWidth.h"
#include "SubdlgViaWidth.h"
#include "SubdlgClearance.h"


// CDlgEditNet dialog

class CDlgEditNet
	: public CDialog
	, public CSubDlg_TraceWidth
	, public CSubDlg_ViaWidth
	, public CSubDlg_Clearance
{
	DECLARE_DYNAMIC(CDlgEditNet)

public:
	CDlgEditNet(CWnd* pParent = NULL);
	virtual ~CDlgEditNet();
	void Initialize(
				CNetList const * netlist, // For parent info for widths/clearance
				netlist_info * nl,	 	  // netlist_info struct
				int i,					  // index into nl (ignored if new net)
				CPartList * plist,		  // partlist
				BOOL new_net,			  // flag for new net
				BOOL visible,			  // visibility flag
				int units,				  // MIL or MM
				CArray<int> * w,          // array of default trace widths
				CArray<int> * v_w,        // array of default via widths
				CArray<int> * v_h_w );    // array of default via hole widths

// Dialog Data
	enum { IDD = IDD_EDIT_NET };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	// Mapping functions to mix-in classes CSubDlg...
	afx_msg void OnBnClicked_t_Default()     { CSubDlg_TraceWidth::OnBnClicked_t_Default();	  }
	afx_msg void OnBnClicked_t_Set()	     { CSubDlg_TraceWidth::OnBnClicked_t_Set();		  }
	afx_msg void OnCbnSelchange_t_width()    { CSubDlg_TraceWidth::OnCbnSelchange_t_width();  }
	afx_msg void OnCbnEditchange_t_width()   { CSubDlg_TraceWidth::OnCbnEditchange_t_width(); }

	afx_msg void OnBnClicked_v_Default()	 { CSubDlg_ViaWidth::OnBnClicked_v_Default();	  }
	afx_msg void OnBnClicked_v_DefForTrace() { CSubDlg_ViaWidth::OnBnClicked_v_DefForTrace(); }
	afx_msg void OnBnClicked_v_Set()		 { CSubDlg_ViaWidth::OnBnClicked_v_Set();		  }

	afx_msg void OnBnClicked_c_Default()     { CSubDlg_Clearance::OnBnClicked_c_Default();    }
	afx_msg void OnBnClicked_c_Set()         { CSubDlg_Clearance::OnBnClicked_c_Set();        }

protected:
	CString m_name;
	BOOL m_new_net;
	CArray<net_info> *m_nl;
	CPartList * m_plist;
	int m_in;
	BOOL m_visible;

	CNetWidthInfo m_width_attrib;

protected:
	int m_units;
	BOOL m_pins_edited;

	CEdit m_edit_name;
	CButton m_check_visible;

	CListBox m_list_pins;
	CButton m_button_add_pin;
	CEdit m_edit_addpin;

	afx_msg void OnEnUpdateEditAddPin();
	afx_msg void OnBnClickedButtonDelete();
	afx_msg void OnBnClickedButtonAdd();

	CButton m_button_OK;
	afx_msg void OnBnClickedOk();
};
