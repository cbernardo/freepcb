#pragma once


// CDlgAddGridVal dialog

class CDlgAddGridVal : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgAddGridVal)

public:
	CDlgAddGridVal(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAddGridVal();

// Dialog Data
	enum { IDD = IDD_ADD_GRIDVAL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CEdit m_edit_value;
public:
	double result;

	DECLARE_MESSAGE_MAP()
};
