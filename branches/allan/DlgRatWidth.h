#pragma once

// CRatWidth dialog

class CRatWidth : public CDialog
{
	DECLARE_DYNAMIC(CRatWidth)

public:
	CRatWidth(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRatWidth();

// Dialog Data
	enum { IDD = IDD_RAT_W };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	int m_ratline_w;
};
