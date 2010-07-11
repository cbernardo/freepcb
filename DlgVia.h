#pragma once
#include "afxwin.h"


// CDlgVia dialog

class CDlgVia : public CDialog
{
	DECLARE_DYNAMIC(CDlgVia)

public:
	CDlgVia(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgVia();
	void Initialize( int via_w, int via_hole_w );

// Dialog Data
	enum { IDD = IDD_VIA };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_edit_via_w;
	CEdit m_edit_hole_w;
	int m_via_w;
	int m_via_hole_w;
};
