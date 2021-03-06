#pragma once
#include "layers.h"
#include "rgb.h"

#define NUM_DLG_LAYERS (LAY_TOP_COPPER + 16)

// CDlgLayers dialog

class CDlgLayers : public CDialog
{
	DECLARE_DYNAMIC(CDlgLayers)

public:
	CDlgLayers(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgLayers();

// Dialog Data
	enum { IDD = IDD_LAYERS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

private:
	int m_nlayers;
	int * m_vis;
	C_RGB m_rgb[MAX_LAYERS];
	C_RGB * m_rgb_ptr;
	CBrush m_brush;
	CColorDialog * m_cdlg;

	DECLARE_MESSAGE_MAP()

public:
	int m_check[NUM_DLG_LAYERS];
	int m_ratline_w;

	void Initialize( int nlayers, int ratlineWidth, int vis[], C_RGB rgb[] );

protected:
	void EditColor( int layer );
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedButtonLayer1();
	afx_msg void OnBnClickedButtonLayer2();
	afx_msg void OnBnClickedButtonLayer3();
	afx_msg void OnBnClickedButtonLayer4();
	afx_msg void OnBnClickedButtonLayer5();
	afx_msg void OnBnClickedButtonLayer6();
	afx_msg void OnBnClickedButtonLayer7();
	afx_msg void OnBnClickedButtonLayer8();
	afx_msg void OnBnClickedButtonLayer9();
	afx_msg void OnBnClickedButtonLayer10();
	afx_msg void OnBnClickedButtonLayer11();
	afx_msg void OnBnClickedButtonLayer12();
	afx_msg void OnBnClickedButtonLayer13();
	afx_msg void OnBnClickedButtonLayer14();
	afx_msg void OnBnClickedButtonLayer15();
	afx_msg void OnBnClickedButtonLayer16();
	afx_msg void OnBnClickedButtonLayer17();
	afx_msg void OnBnClickedButtonLayer18();
	afx_msg void OnBnClickedButtonLayer19();
	afx_msg void OnBnClickedButtonLayer20();
	afx_msg void OnBnClickedButtonLayer21();
	afx_msg void OnBnClickedButtonLayer22();
	afx_msg void OnBnClickedButtonLayer23();
	afx_msg void OnBnClickedButtonLayer24();
	afx_msg void OnBnClickedButtonLayer25();
	afx_msg void OnBnClickedButtonLayer26();
	afx_msg void OnBnClickedButtonLayer27();
	afx_msg void OnBnClickedButtonLayer28();
	afx_msg void OnBnClickedButtonLayer7W();
};
