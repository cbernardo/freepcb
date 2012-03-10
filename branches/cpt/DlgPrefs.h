#pragma once
#include "afxwin.h"


// CDlgPrefs dialog.  ALL CPT

class CDlgPrefs : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgPrefs)

public:
	CDlgPrefs(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgPrefs();

// Dialog Data
	enum { IDD = IDD_PREFS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	CButton m_check_reverse;
	CButton m_check_grd1;
	CButton m_check_grd2;
	CButton m_check_grd3;
	CButton m_check_grd4;
	CButton m_check_grd5;
	CButton m_check_grd6;
	CButton m_check_grd7;
	CButton m_check_grd8;
	CButton m_check_grd9;
	CButton m_check_grd10;
	CButton m_check_grd11;
	CButton m_check_grd12;
	CButton m_check_grd13;
	CButton m_check_grd14;
	CButton m_check_grd15;
	CButton m_check_grd16;
	CButton m_check_grd17;
	CButton m_check_grd18;
	CButton m_check_grd19;
	CButton m_check_grd20;
	CButton m_check_grd21;
	CButton m_check_grd22;
	CButton m_check_grd23;
	CButton m_check_grd24;
	CButton m_check_grd25;
	CButton m_check_grd26;
	CButton m_check_grd27;
	CButton m_check_grd28;
	CButton m_check_grd29;
	CButton m_check_grd30;

public:
	bool fReverse, fGridFlags[30];
	CFreePcbDoc *doc;
};
