#pragma once


// CDlgAddWidth dialog

class CDlgAddWidth : public CDialog
{
	DECLARE_DYNAMIC(CDlgAddWidth)

public:
	CDlgAddWidth(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAddWidth();

// Dialog Data
	enum { IDD = IDD_ADD_WIDTH_MENU_ITEM };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	int m_width;
	int m_via_w;
	int m_via_hole_w;
};
