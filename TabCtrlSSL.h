#if !defined(AFX_TABCTRLSSL_H__75BE48A7_864C_11D5_9F04_000102FB9990__INCLUDED_)
#define AFX_TABCTRLSSL_H__75BE48A7_864C_11D5_9F04_000102FB9990__INCLUDED_

// Written by Derek Lakin.  See his comment about usage (from code project.com)
// http://www.codeproject.com/KB/tabs/ctabctrlssl.aspx?msg=913105
//
//    Apologies for not making this clear. You can use and/or modify the source code 
//    in any application you like for any purpose you like. It would be nice if I got
//    a credit in an About box or documentation.  I'd also appreciate it if you let me
//    know what you're building, just so I can get an idea of what people are writing 
//    with my code..

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TabCtrlEx.h : header file
//

#include <afxtempl.h>

class CTabPageSSL;

#ifdef _DEBUG
#pragma pack (push, 1)

typedef struct {
	WORD dlgVer;
	WORD signature;
	DWORD helpID;
	DWORD exStyle;
	DWORD style;
	WORD cDlgItems;
	short x;
	short y;
	short cx;
	short cy;
} DLGTEMPLATEEX;

#pragma pack (pop)
#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTabCtrlSSL window

class CTabCtrlSSL : public CTabCtrl {
public:
// Construction
	CTabCtrlSSL ();
// Destruction
	virtual ~CTabCtrlSSL (void);
// Page Functions
	int		AddSSLPage (LPCTSTR pszTitle, int nPageID, CTabPageSSL* pTabPage);
	int		AddSSLPage (LPCTSTR pszTitle, int nPageID,	LPCTSTR pszTemplateName);
	int		AddSSLPage (LPCTSTR pszTitle, int nPageID, int nTemplateID);
	BOOL	RemoveSSLPage (int nIndex);
	int		GetSSLPageCount (void);
	BOOL	GetSSLPageTitle (int nIndex, CString& strTitle);
	BOOL	SetSSLPageTitle (int nIndex, LPCTSTR pszTitle);
	int		GetSSLPageID (int nIndex);
	int		SetSSLPageID (int nIndex, int nPageID);
	BOOL	ActivateSSLPage (int nIndex);
	int		GetSSLActivePage (void);
	CWnd*	GetSSLPage (int nIndex);
	int		GetSSLPageIndex (int nPageID);

protected:
    struct TabDelete {
        CTabPageSSL*   pTabPage;
        BOOL        bDelete;
    };
    CArray<TabDelete, TabDelete> m_tabs;
	CArray<HWND, HWND> m_hFocusWnd;
	CArray<int, int> m_nPageIDs;

	int AddPage (LPCTSTR pszTitle, int nPageID, TabDelete tabDelete);

    virtual void OnInitPage (int nIndex, int nPageID);
	virtual void OnActivatePage (int nIndex, int nPageID);
	virtual void OnDeactivatePage (int nIndex, int nPageID);
	virtual void OnDestroyPage (int nIndex, int nPageID);

public:
	virtual BOOL OnCommand (WPARAM wParam, LPARAM lParam);
	virtual BOOL OnNotify (WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual BOOL OnCmdMsg (UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

#ifdef _DEBUG
	BOOL CheckDialogTemplate (LPCTSTR pszTemplateName);
#endif // _DEBUG
	void ResizeDialog (int nIndex, int cx, int cy);
	// Generated message map functions
public:
	//{{AFX_MSG(CTabCtrlSSL)
	afx_msg void OnDestroy (void);
	afx_msg void OnSetFocus (CWnd* pOldWnd);
	afx_msg void OnKillFocus (CWnd* pNewWnd);
	afx_msg void OnSelChanging (NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelChange (NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	BOOL OnDDXOut(void);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#include "TabPageSSL.h"

#endif // !defined(AFX_TABCTRLSSL_H__75BE48A7_864C_11D5_9F04_000102FB9990__INCLUDED_)
