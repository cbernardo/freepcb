#pragma once

#include "TabCtrlSSL.h"
#include "Tab_ProjectOpt_Main.h"
#include "Tab_ProjectOpt_Spacing.h"
#include "Tab_ProjectOpt_Thermal.h"


// CDlgProjectOptionsTabbed dialog

class CDlgProjectOptionsTabbed : public CDialog
{
	DECLARE_DYNAMIC(CDlgProjectOptionsTabbed)

public:
	CDlgProjectOptionsTabbed(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgProjectOptionsTabbed();

// Dialog Data
	enum { IDD = IDD_PROJECT_OPTIONS_TABBED };

	class CTabs : public CTabCtrlSSL
	{
	public:
		CTab_ProjectOpt_Main	m_Tab_Main;
		CTab_ProjectOpt_Spacing m_Tab_Spacing;
		CTab_ProjectOpt_Thermal m_Tab_Thermal;

		BOOL m_new_project;
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	CTabs m_tabs;

    friend CTab_ProjectOpt_Main;
    friend CTab_ProjectOpt_Spacing;
    friend CTab_ProjectOpt_Thermal;

	DECLARE_MESSAGE_MAP()

public:
	void Init( BOOL new_flag,
		CString * name,
		CString * path_to_folder,
		CString * lib_folder,
		int num_layers,
		int ratline_w,
		BOOL bSMT_connect_copper,
		int glue_w,
		CConnectionWidthInfo const &width,
		int ca_clearance,
		int hole_clearance,
		int auto_interval,
		int thermal_width,
		int thermal_clearance,
		CArray<int> * w,
		CArray<int> * v_w,
		CArray<int> * v_h_w );

	CString GetName() const              { return m_tabs.m_Tab_Main.m_name; };
	CString GetPathToFolder() const      { return m_tabs.m_Tab_Main.m_path_to_folder; };
	CString GetLibFolder() const         { return m_tabs.m_Tab_Main.m_lib_folder; };
	int GetNumCopperLayers() const       { return m_tabs.m_Tab_Main.m_layers; };
	int GetRatlineWidth() const          { return m_tabs.m_Tab_Main.m_ratline_w; };
	int GetAutoInterval() const          { return m_tabs.m_Tab_Main.m_auto_interval; };
	int GetGlueWidth() const             { return m_tabs.m_Tab_Main.m_glue_w; };
    BOOL Get_bSMT_connect_copper() const { return m_tabs.m_Tab_Main.m_bSMT_connect_copper; }

	int GetTraceWidth() const            { return m_tabs.m_Tab_Spacing.m_trace_w; };
	int GetCopperAreaClearance() const   { return m_tabs.m_Tab_Spacing.m_ca_clearance_trace; };
	int GetHoleClearance() const         { return m_tabs.m_Tab_Spacing.m_ca_clearance_hole; }
	int GetViaWidth() const              { return m_tabs.m_Tab_Spacing.m_via_w; };
	int GetViaHoleWidth() const          { return m_tabs.m_Tab_Spacing.m_hole_w; };

	int GetThermalWidth() const          { return m_tabs.m_Tab_Thermal.m_TR_line_w; }
	int GetThermalClearance() const      { return m_tabs.m_Tab_Thermal.m_TR_clearance; }
};
