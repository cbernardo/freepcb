#pragma once


// CDlgSetPinClearance dialog

class CDlgSetPinClearance : public CDialog
{
	DECLARE_DYNAMIC(CDlgSetPinClearance)

public:
	CDlgSetPinClearance(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSetPinClearance();

// Dialog Data
	enum { IDD = IDD_PIN_CLEARANCE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
