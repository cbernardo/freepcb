// DlgProjectOptionsTabbed.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgProjectOptionsTabbed.h"


// CDlgProjectOptionsTabbed dialog

IMPLEMENT_DYNAMIC(CDlgProjectOptionsTabbed, CDialog)

CDlgProjectOptionsTabbed::CDlgProjectOptionsTabbed(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgProjectOptionsTabbed::IDD, pParent)
{
}

CDlgProjectOptionsTabbed::~CDlgProjectOptionsTabbed()
{
}

// initialize data
//
void CDlgProjectOptionsTabbed::Init( BOOL new_project,
                                     CString * name,
                                     CString * path_to_folder,
                                     CString * lib_folder,
                                     int num_layers,
									 int ratline_w,
                                     BOOL bSMT_connect_copper,
                                     int glue_w,
                                     CNetWidthInfo const &width_attrib,
									 int hole_clearance,
                                     int auto_interval,
									 int thermal_width,
                                     int thermal_clearance,
                                     CArray<int> * w,
                                     CArray<int> * v_w,
                                     CArray<int> * v_h_w )
{
	m_tabs.m_new_project = new_project;

	m_tabs.m_Tab_Main.m_name = *name;
	m_tabs.m_Tab_Main.m_path_to_folder = *path_to_folder;
	m_tabs.m_Tab_Main.m_lib_folder = *lib_folder;

	m_tabs.m_Tab_Main.m_layers = num_layers;
	m_tabs.m_Tab_Main.m_ratline_w = ratline_w;
	m_tabs.m_Tab_Main.m_bSMT_connect_copper = bSMT_connect_copper;
	m_tabs.m_Tab_Main.m_glue_w = glue_w;
	m_tabs.m_Tab_Main.m_auto_interval = auto_interval;

	m_tabs.m_Tab_Spacing.m_trace_w            = width_attrib.m_seg_width.m_val;
	m_tabs.m_Tab_Spacing.m_via_w              = width_attrib.m_via_width.m_val;
	m_tabs.m_Tab_Spacing.m_hole_w             = width_attrib.m_via_hole.m_val;
	m_tabs.m_Tab_Spacing.m_ca_clearance_trace = width_attrib.m_ca_clearance.m_val;

	m_tabs.m_Tab_Spacing.m_ca_clearance_hole = hole_clearance;
	m_tabs.m_Tab_Spacing.m_w = w;
	m_tabs.m_Tab_Spacing.m_v_w = v_w;
	m_tabs.m_Tab_Spacing.m_v_h_w = v_h_w;

	m_tabs.m_Tab_Thermal.m_TR_line_w    = thermal_width;
    m_tabs.m_Tab_Thermal.m_TR_clearance = thermal_clearance;
}


BOOL CDlgProjectOptionsTabbed::OnInitDialog () {
	CDialog::OnInitDialog ();

    // Setup the tab control
	int nPageID = 0;

	m_tabs.m_Tab_Main.Create (IDD_TAB_PROJ_OPT_MAIN, this);
	m_tabs.m_Tab_Main.EnableRouteCommand(); // Enable ESC from within a tab page
	m_tabs.AddSSLPage( _T("Main"), nPageID++, &m_tabs.m_Tab_Main );

	m_tabs.m_Tab_Spacing.Create (IDD_TAB_PROJ_OPT_SPACING, this);
	m_tabs.m_Tab_Spacing.EnableRouteCommand(); // Enable ESC from within a tab page
	m_tabs.AddSSLPage(_T("Spacing"), nPageID++, &m_tabs.m_Tab_Spacing);

	m_tabs.m_Tab_Thermal.Create (IDD_TAB_PROJ_OPT_THERMAL, this);
	m_tabs.m_Tab_Thermal.EnableRouteCommand(); // Enable ESC from within a tab page
	m_tabs.AddSSLPage(_T("Thermals"), nPageID++, &m_tabs.m_Tab_Thermal);

   return TRUE; // return TRUE unless you set the focus to a control
}

void CDlgProjectOptionsTabbed::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_PROJECT_OPTIONS_TAB, m_tabs);

	if( pDX->m_bSaveAndValidate )
	{
		if ( !m_tabs.m_Tab_Main   .Verify() ||
			 !m_tabs.m_Tab_Spacing.Verify() ||
			 !m_tabs.m_Tab_Thermal.Verify() )
		{
			pDX->Fail();
		}
		else
		{
			m_tabs.m_Tab_Main   .DDX_out();
			m_tabs.m_Tab_Spacing.DDX_out();
			m_tabs.m_Tab_Thermal.DDX_out();
		}
	}
}


BEGIN_MESSAGE_MAP(CDlgProjectOptionsTabbed, CDialog)
END_MESSAGE_MAP()


// CDlgProjectOptionsTabbed message handlers
