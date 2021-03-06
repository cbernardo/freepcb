#pragma once
#include "afxwin.h"
#include "DlgLog.h"


// CDlgCAD dialog

class CDlgCAD : public CDialog
{
	DECLARE_DYNAMIC(CDlgCAD)

public:
	CDlgCAD(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgCAD();
	void Initialize( double version, CString * folder, CString * project_folder,
		CString * app_folder,
		int num_copper_layers, int units,
		int mask_clearance,
		int pilot_diameter, int min_silkscreen_wid,
		int outline_width, 
		int annular_ring_pins, int annular_ring_vias, int shrink_paste,
		int n_x, int n_y, int space_x, int space_y,
		int flags, int layers, int drill_file,
		CArray<CPolyLine> * bd, CArray<CPolyLine> * sm,
		CPartList * pl, CNetList * nl, CTextList * tl, CDisplayList * dl,
		CDlgLog * log );
	void SetFields();
	void GetFields();
// Dialog Data
	enum { IDD = IDD_CAD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	double m_version;
	double m_file_version;
	CEdit m_edit_folder;
	CEdit m_edit_mask;
	CButton m_check_drill;
	CButton m_check_top_silk;
	CButton m_check_bottom_silk;
	CButton m_check_top_solder;
	CButton m_check_bottom_solder;
	CButton m_check_top_copper;
	CButton m_check_bottom_copper;
	CButton m_check_inner1;
	CButton m_check_inner2;
	CButton m_check_inner3;
	CButton m_check_inner4;
	CButton m_check_inner5;
	CButton m_check_inner6;
	CButton m_check_inner7;
	CButton m_check_inner8;
	CButton m_check_inner9;
	CButton m_check_inner10;
	CButton m_check_inner11;
	CButton m_check_inner12;
	CButton m_check_inner13;
	CButton m_check_inner14;
	CButton m_check_outline;
	CButton m_check_moires;
	CButton m_check_layer_text;
	int m_num_copper_layers;
	int m_mask_clearance;
	int m_pilot_diameter;
	int m_min_silkscreen_width;
	int m_outline_width;
	int m_annular_ring_pins;
	int m_annular_ring_vias;
	int m_units;
	int m_flags;
	int m_layers;
	int m_drill_file;
	CArray<CPolyLine> * m_bd;
	CArray<CPolyLine> * m_sm;
	CPartList * m_pl;
	CNetList * m_nl;
	CTextList * m_tl;
	CDisplayList * m_dl;
	CDlgLog * m_dlg_log;
	CString m_folder;
	CString m_project_folder;
	CString m_app_folder;
	afx_msg void OnBnClickedGo();
	CComboBox m_combo_units;
	afx_msg void OnCbnSelchangeComboCadUnits();
	CEdit m_edit_pilot_diam;
	CButton m_check_pilot;
	afx_msg void OnBnClickedCheckCadPilot();
	afx_msg void OnBnClickedCancel();
	CEdit m_edit_min_ss_w;
	CEdit m_edit_outline_width;
	CEdit m_edit_ann_pins;
	CEdit m_edit_ann_vias;
	CButton m_check_mask_vias;
	afx_msg void OnBnClickedButtonDef();
	afx_msg void OnBnClickedButtonFolder();
	CButton m_check_board;
	CButton m_check_top_paste;
	CButton m_check_bottom_paste;
	int m_n_x;
	int m_n_y;
	CEdit m_edit_n_x;
	CEdit m_edit_n_y;
	CEdit m_edit_space_x;
	CEdit m_edit_space_y;
	int m_space_x, m_space_y;
	CEdit m_edit_shrink_paste;
	int m_paste_shrink;
	CButton m_check_render_all;
	CButton m_check_mirror_bottom;
	afx_msg void OnBnClickedRenderAllGerbers();
	CButton m_check_sm_clearance_for_cutouts;
	afx_msg void OnBnClickedButtonDone();
};
