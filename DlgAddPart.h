#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "FreePcbDoc.h"

// CDlgAddPart dialog

enum {
	MSK_FOOTPRINT = 1,
	MSK_PACKAGE = 2,
	MSK_VALUE = 4
};

class CDlgAddPart : public CDialog
{
	DECLARE_DYNAMIC(CDlgAddPart)

public:
	CDlgAddPart(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAddPart();

// Dialog Data
	enum { IDD = IDD_ADD_PART };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	CFreePcbDoc *m_doc;			// CPT2 added.
	CString m_footprint_name;
	int m_ilib;
	int m_ihead;
	int m_ifoot;
	BOOL m_in_cache;			// true if new footprint selected from cache
	CShape *m_shape;			// CPT2 was CShape, now CShape*.  Set on entry if we're editing, null if we're adding;  
								// set when "OK" is hit, either way
	CString m_shape_file_name;	// CPT2 new.  Informs the caller about the selected shape's library file ("" => local cache)
	BOOL m_drag_flag;
	BOOL m_offboard_flag;
	partlist_info * m_pl;
	int m_ip;					// The index of the single pre-existing item being edited, if appropriate
	BOOL m_standalone;			// False if we're being called from DlgPartlist, true otherwise
	BOOL m_new_part;
	BOOL m_multiple;
	CComboBox m_combo_shape;
	CShapeList * m_cache_shapes;
	CFootLibFolderMap * m_footlibfoldermap;
	CFootLibFolder * m_folder;
	CTreeCtrl part_tree;
	CEdit m_edit_ref_des;
	CEdit m_edit_package;
	CEdit m_edit_footprint;
	CEdit m_edit_value;
	CButton m_check_value_visible;
	CButton m_check_ref_visible;
	CButton m_radio_drag;
	CButton m_radio_offboard;
	CButton m_radio_set;
	CEdit m_edit_x;
	CEdit m_edit_y;
	CListBox m_list_side;
	CStatic m_preview;
	CComboBox m_combo_units;
	CEdit m_edit_author;
	CEdit m_edit_source;
	CEdit m_edit_desc;
	CButton m_button_browse;
	CEdit m_edit_lib;
	CDlgLog * m_dlg_log;
	CComboBox m_combo_angle;
	CButton m_button_compare_with_netlist;		// CPT2 new
	int m_x_initial;							// CPT2.  Stores the initial value of m_x (we must take note if it changes)
	int m_y_initial;							// CPT2.  Similar
	int m_units_initial;						// CPT2.  Similar

public:
	void Initialize( partlist_info * pli,
		int i, 
		BOOL standalone,
		BOOL new_part,
		BOOL multiple,
		int multiple_mask,
		CFreePcbDoc *doc,
		int units );
	void InitPartLibTree();
	BOOL GetDragFlag();
	void SetFields();
	void GetFields();

	int m_units;
	int m_x, m_y;
	int m_angle;
	int m_multiple_mask;
	CString m_ref_des;													// CPT2, used when a single part is being created/edited

private:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedRadioDrag();
	afx_msg void OnBnClickedRadioSet();
	afx_msg void OnTvnSelchangedPartLibTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedRadioOffBoard();
	afx_msg void OnBnClickedButtonBrowse();
	afx_msg void OnCbnSelchangeComboAddPartUnits();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
public:
	afx_msg void OnBnClickedButtonCompareWithNetlist();
};
