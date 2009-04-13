#include "stdafx.h"
#include "TabPageSSL.h"
#include "TabCtrlSSL.h"

// Written by Derek Lakin.  See his comment about usage (from code project.com)
// http://www.codeproject.com/KB/tabs/ctabctrlssl.aspx?msg=913105
//
//    Apologies for not making this clear. You can use and/or modify the source code 
//    in any application you like for any purpose you like. It would be nice if I got
//    a credit in an About box or documentation.  I'd also appreciate it if you let me
//    know what you're building, just so I can get an idea of what people are writing 
//    with my code..

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Construction

CTabPageSSL::CTabPageSSL () {
#ifndef _AFX_NO_OCC_SUPPORT
	AfxEnableControlContainer ();
#endif // !_AFX_NO_OCC_SUPPORT
	m_bRouteCommand = false;
	m_bRouteCmdMsg = false;
	m_bRouteNotify = false;

	my_TabCtrl = NULL;
	m_PageIndex = -1;
}

CTabPageSSL::CTabPageSSL (UINT nIDTemplate, CWnd* pParent /*=NULL*/)
	: CDialog(nIDTemplate, pParent)
{
#ifndef _AFX_NO_OCC_SUPPORT
	AfxEnableControlContainer ();
#endif // !_AFX_NO_OCC_SUPPORT
	m_bRouteCommand = false;
	m_bRouteCmdMsg = false;
	m_bRouteNotify = false;

	my_TabCtrl = NULL;
	m_PageIndex = -1;
}

/////////////////////////////////////////////////////////////////////////////
// Destruction

CTabPageSSL::~CTabPageSSL () {
}


/////////////////////////////////////////////////////////////////////////////
// Overridables

BOOL CTabPageSSL::OnInitPage (int nPageID)
{
	// TODO: Override in derived class to initialise pages.
	return TRUE;
}

void CTabPageSSL::OnActivatePage (int nPageID)
{
	// TODO: Override in derived class to respond to page activations.
}

void CTabPageSSL::OnDeactivatePage (int nPageID)
{
	// TODO: Override in derived class to respond to page deactivations.
}

void CTabPageSSL::OnDestroyPage (int nPageID)
{
	// TODO: Override in derived class to free resources.
}


/////////////////////////////////////////////////////////////////////////////
// Visibility
BOOL CTabPageSSL::MakePageActive(void)
{
	return my_TabCtrl->ActivateSSLPage(m_PageIndex);
}


/////////////////////////////////////////////////////////////////////////////
// Message Handlers

void CTabPageSSL::OnOK (void) {
	//
	// Prevent CDialog::OnOK from calling EndDialog.
	//
}

void CTabPageSSL::OnCancel (void) {
	//
	// Prevent CDialog::OnCancel from calling EndDialog.
	//
}

BOOL CTabPageSSL::OnCommand (WPARAM wParam, LPARAM lParam) {
	// Call base class OnCommand to allow message map processing
	BOOL bReturn = CDialog::OnCommand (wParam, lParam);

	if (true == m_bRouteCommand)
	{
		//
		// Forward WM_COMMAND messages to the dialog's parent.
		//
		return GetParent ()->SendMessage (WM_COMMAND, wParam, lParam);
	}

	return bReturn;
}

BOOL CTabPageSSL::OnNotify (WPARAM wParam, LPARAM lParam, LRESULT* pResult) {
	BOOL bReturn = CDialog::OnNotify(wParam, lParam, pResult);

	if (true == m_bRouteNotify)
	{
		//
		// Forward WM_NOTIFY messages to the dialog's parent.
		//
		return GetParent ()->SendMessage (WM_NOTIFY, wParam, lParam);
	}

	return bReturn;
}

BOOL CTabPageSSL::OnCmdMsg (UINT nID, int nCode, void* pExtra,
	AFX_CMDHANDLERINFO* pHandlerInfo) {
	BOOL bReturn = CDialog::OnCmdMsg (nID, nCode, pExtra, pHandlerInfo);

#ifndef _AFX_NO_OCC_SUPPORT
	if (true == m_bRouteCmdMsg)
	{
		//
		// Forward ActiveX control events to the dialog's parent.
		//
		if (nCode == CN_EVENT)
			return GetParent ()->OnCmdMsg (nID, nCode, pExtra, pHandlerInfo);
	}
#endif // !_AFX_NO_OCC_SUPPORT

	return bReturn;
}
