#if !defined(AFX_TABPAGESSL_H__619331B3_7DE7_4DB1_A039_2103E87E8E71__INCLUDED_)
#define AFX_TABPAGESSL_H__619331B3_7DE7_4DB1_A039_2103E87E8E71__INCLUDED_

// Written by Derek Lakin.  See his comment about usage (from code project.com)
// http://www.codeproject.com/KB/tabs/ctabctrlssl.aspx?msg=913105
//
//    Apologies for not making this clear. You can use and/or modify the source code 
//    in any application you like for any purpose you like. It would be nice if I got
//    a credit in an About box or documentation.  I'd also appreciate it if you let me
//    know what you're building, just so I can get an idea of what people are writing 
//    with my code..
//
// In the tab page dialogs, you will need to ensure that the following attributes are set:
//    style = Child 
//    border = None

#include "TabCtrlSSL.h"

/////////////////////////////////////////////////////////////////////////////
// CTabPageSSL declaration

class CTabPageSSL : public CDialog
{
public:
// Construction
	CTabPageSSL ();	// Default Constructor
	CTabPageSSL (UINT nIDTemplate, CWnd* pParent = NULL);	// Standard Constructor
// Destruction
	~CTabPageSSL ();

	// Enable/disable command routing to the parent.
	void EnableRouteCommand(bool bRoute = true) { m_bRouteCommand = bRoute; };
	bool IsRouteCommand() { return m_bRouteCommand; };
	// Enable CmdMsg routing to the parent.
	void EnableRouteCmdMsg(bool bRoute = true) { m_bRouteCmdMsg = bRoute; };
	bool IsRouteCmdMsg() { return m_bRouteCmdMsg; };
	// Enable/Disable Notify routing to the parent.
	void EnableRouteNotify(bool bRoute = true) { m_bRouteNotify = bRoute; };
	bool IsRouteNotify() { return m_bRouteNotify; };

	CTabCtrlSSL *GetParent() const { return my_TabCtrl; }

protected:
// Message Handlers
	virtual BOOL OnCommand (WPARAM wParam, LPARAM lParam);
	virtual BOOL OnNotify (WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual void OnOK (void);
	virtual void OnCancel (void);
	virtual BOOL OnCmdMsg (UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

// Routing flags
	bool m_bRouteCommand;
	bool m_bRouteCmdMsg;
	bool m_bRouteNotify;

// Indexing
	friend class CTabCtrlSSL;
	CTabCtrlSSL *my_TabCtrl;
    int m_PageIndex;

public:
	BOOL MakePageActive(void);

public:
    virtual BOOL OnInitPage (int nPageID);
	virtual void OnActivatePage (int nPageID);
	virtual void OnDeactivatePage (int nPageID);
	virtual void OnDestroyPage (int nPageID);
};

#endif // !defined(AFX_TABPAGE_H__619331B3_7DE7_4DB1_A039_2103E87E8E71__INCLUDED_)
