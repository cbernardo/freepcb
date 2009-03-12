#pragma once
#include "afxwin.h"


// CDlgVia dialog

class CDlgVia : public CDialog
{
	DECLARE_DYNAMIC(CDlgVia)

public:
	CDlgVia(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgVia();
	void Initialize( CConnectionWidthInfo const &via_width );

// Dialog Data
	enum { IDD = IDD_VIA };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	CEdit m_edit_via_w;
	CEdit m_edit_hole_w;

public:
	CConnectionWidthInfo m_via_width;
};
