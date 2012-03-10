#pragma once
#include "afxwin.h"


class CDlgExportOptions : public CDialog
{
	DECLARE_DYNAMIC(CDlgExportOptions)

	enum { PARTS_ONLY, NETS_ONLY, PARTS_AND_NETS }; 
	enum { FREEPCB, PADSPCB }; 
	enum { IDD = IDD_EXPORT };

public:
	CDlgExportOptions(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgExportOptions();
	void Initialize( int select );
	int m_select;
	int m_format;

protected:
//	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	CButton m_radio_parts;
	CButton m_radio_nets;
	CButton m_radio_parts_and_nets;
	CButton m_radio_padspcb;
	CButton m_radio_freepcb;
	CButton m_check_values;
	DECLARE_MESSAGE_MAP()
};


