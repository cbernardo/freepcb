// DlgCAD.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgCAD.h"
#include "Gerber.h"
#include "DlgLog.h"
#include "DlgMyMessageBox2.h"
#include "PathDialog.h"
#include "RTcall.h"
#include ".\dlgcad.h"

// CDlgCAD dialog

IMPLEMENT_DYNAMIC(CDlgCAD, CDialog)
CDlgCAD::CDlgCAD(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCAD::IDD, pParent)
	, m_n_x(0)
	, m_n_y(0)
{
	m_dlg_log = NULL;
	m_folder = "";
}

CDlgCAD::~CDlgCAD()
{
}

void CDlgCAD::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_FOLDER, m_edit_folder);
	DDX_Control(pDX, IDC_EDIT_MASK, m_edit_mask);
	DDX_Control(pDX, IDC_CHECK_DRILL, m_check_drill);
	DDX_Control(pDX, IDC_CHECK_TOP_SILK, m_check_top_silk);
	DDX_Control(pDX, IDC_CHECK_BOTTOM_SILK, m_check_bottom_silk);
	DDX_Control(pDX, IDC_CHECK_TOP_SOLDER, m_check_top_solder);
	DDX_Control(pDX, IDC_CHECKBOTTOM_SOLDER_MASK, m_check_bottom_solder);
	DDX_Control(pDX, IDC_CHECK_TOP_COPPER, m_check_top_copper);
	DDX_Control(pDX, IDC_CHECK_BOTTOM_COPPER, m_check_bottom_copper);
	DDX_Control(pDX, IDC_CHECK_INNER1, m_check_inner1);
	DDX_Control(pDX, IDC_CHECK_INNER2, m_check_inner2);
	DDX_Control(pDX, IDC_CHECK_INNER3, m_check_inner3);
	DDX_Control(pDX, IDC_CHECK_INNER4, m_check_inner4);
	DDX_Control(pDX, IDC_CHECK_INNER5, m_check_inner5);
	DDX_Control(pDX, IDC_CHECK_INNER6, m_check_inner6);
	DDX_Control(pDX, IDC_CHECK_INNER7, m_check_inner7);
	DDX_Control(pDX, IDC_CHECK_INNER8, m_check_inner8);
	DDX_Control(pDX, IDC_CHECK_INNER9, m_check_inner9);
	DDX_Control(pDX, IDC_CHECK_INNER10, m_check_inner10);
	DDX_Control(pDX, IDC_CHECK_INNER11, m_check_inner11);
	DDX_Control(pDX, IDC_CHECK_INNER12, m_check_inner12);
	DDX_Control(pDX, IDC_CHECK_INNER13, m_check_inner13);
	DDX_Control(pDX, IDC_CHECK_INNER14, m_check_inner14);
	DDX_Control(pDX, IDC_BOARD_OUTLINE, m_check_outline);
	DDX_Control(pDX, IDC_MOIRES, m_check_moires);
	DDX_Control(pDX, IDC_LAYER_DESC, m_check_layer_text);
	DDX_Control(pDX, IDC_COMBO_CAD_UNITS, m_combo_units);
	DDX_Control(pDX, IDC_EDIT_CAD_PILOT_DIAM, m_edit_pilot_diam);
	DDX_Control(pDX, IDC_CHECK_CAD_PILOT, m_check_pilot);
	DDX_Control(pDX, IDC_EDIT_CAD_MIN_SSW, m_edit_min_ss_w);
	DDX_Text( pDX, IDC_EDIT_FOLDER, m_folder );
	DDX_Control(pDX, IDC_EDIT_CAD_OUTLINE_WID, m_edit_outline_width);
	DDX_Control(pDX, IDC_EDIT_CAD_ANN_PINS, m_edit_ann_pins);
	DDX_Control(pDX, IDC_EDIT_CAD_ANN_VIAS, m_edit_ann_vias);
	DDX_Control(pDX, IDC_CHECK3, m_check_mask_vias);
	DDX_Control(pDX, IDC_CHECK_BD, m_check_board);
	DDX_Control(pDX, IDC_CHECK_TOP_PASTE, m_check_top_paste);
	DDX_Control(pDX, IDC_CHECK_BOTTOM_PASTE, m_check_bottom_paste);
	DDX_Control(pDX, IDC_EDIT_SP_X, m_edit_space_x);
	DDX_Control(pDX, IDC_EDIT_SP_Y, m_edit_space_y);
	DDX_Control(pDX, IDC_EDIT_N_X, m_edit_n_x);
	DDX_Control(pDX, IDC_EDIT_N_Y, m_edit_n_y);
	DDX_Text(pDX, IDC_EDIT_N_X, m_n_x );
	DDX_Text(pDX, IDC_EDIT_N_Y, m_n_y );
	DDV_MinMaxUInt(pDX, m_n_x, 1, 99);
	DDV_MinMaxUInt(pDX, m_n_y, 1, 99);
	DDX_Control(pDX, IDC_EDIT_CAD_SHRINK_PASTE, m_edit_shrink_paste);
	DDX_Control(pDX, IDC_CHECK_ALL_GERBERS, m_check_render_all);
	DDX_Control(pDX, IDC_CHECK_MIRROR_BOTTOM, m_check_mirror_bottom);
	DDX_Control(pDX, IDC_CHECK4, m_check_sm_clearance_for_cutouts);
	if( !pDX->m_bSaveAndValidate )
	{
		// entry
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );
		if( m_units == MIL )
			m_combo_units.SetCurSel( 0 );
		else
			m_combo_units.SetCurSel( 1 );
		SetFields();
		// enable checkboxes for valid gerber layers
		if( m_num_copper_layers < 2 )
			m_check_bottom_copper.EnableWindow( FALSE );
		if( m_num_copper_layers < 3 )
			m_check_inner1.EnableWindow( FALSE );
		if( m_num_copper_layers < 4 )
			m_check_inner2.EnableWindow( FALSE );
		if( m_num_copper_layers < 5 )
			m_check_inner3.EnableWindow( FALSE );
		if( m_num_copper_layers < 6 )
			m_check_inner4.EnableWindow( FALSE );
		if( m_num_copper_layers < 7 )
			m_check_inner5.EnableWindow( FALSE );
		if( m_num_copper_layers < 8 )
			m_check_inner6.EnableWindow( FALSE );
		if( m_num_copper_layers < 9 )
			m_check_inner7.EnableWindow( FALSE );
		if( m_num_copper_layers < 10 )
			m_check_inner8.EnableWindow( FALSE );
		if( m_num_copper_layers < 11 )
			m_check_inner9.EnableWindow( FALSE );
		if( m_num_copper_layers < 12 )
			m_check_inner10.EnableWindow( FALSE );
		if( m_num_copper_layers < 13 )
			m_check_inner11.EnableWindow( FALSE );
		if( m_num_copper_layers < 14 )
			m_check_inner12.EnableWindow( FALSE );
		if( m_num_copper_layers < 15 )
			m_check_inner13.EnableWindow( FALSE );
		if( m_num_copper_layers < 16 )
			m_check_inner14.EnableWindow( FALSE );

		// load saved settings
		SetFields();
		for( int i=0; i<23; i++ )
		{
			if( i<(m_num_copper_layers+4) || i>19 )
			{
				switch(i)
				{
				case 0: m_check_top_silk.SetCheck(m_layers & (1<<i)); break;
				case 1: m_check_bottom_silk.SetCheck(m_layers & (1<<i)); break;
				case 2: m_check_top_solder.SetCheck(m_layers & (1<<i)); break;
				case 3: m_check_bottom_solder.SetCheck(m_layers & (1<<i)); break;
				case 4: m_check_top_copper.SetCheck(m_layers & (1<<i)); break;
				case 5: m_check_bottom_copper.SetCheck(m_layers & (1<<i)); break;
				case 6: m_check_inner1.SetCheck(m_layers & (1<<i)); break;
				case 7: m_check_inner2.SetCheck(m_layers & (1<<i)); break;
				case 8: m_check_inner3.SetCheck(m_layers & (1<<i)); break;
				case 9: m_check_inner4.SetCheck(m_layers & (1<<i)); break;
				case 10: m_check_inner5.SetCheck(m_layers & (1<<i)); break;
				case 11: m_check_inner6.SetCheck(m_layers & (1<<i)); break;
				case 12: m_check_inner7.SetCheck(m_layers & (1<<i)); break;
				case 13: m_check_inner8.SetCheck(m_layers & (1<<i)); break;
				case 14: m_check_inner9.SetCheck(m_layers & (1<<i)); break;
				case 15: m_check_inner10.SetCheck(m_layers & (1<<i)); break;
				case 16: m_check_inner11.SetCheck(m_layers & (1<<i)); break;
				case 17: m_check_inner12.SetCheck(m_layers & (1<<i)); break;
				case 18: m_check_inner13.SetCheck(m_layers & (1<<i)); break;
				case 19: m_check_inner14.SetCheck(m_layers & (1<<i)); break;
				case 20: m_check_board.SetCheck(m_layers & (1<<i)); break;
				case 21: m_check_top_paste.SetCheck(m_layers & (1<<i)); break;
				case 22: m_check_bottom_paste.SetCheck(m_layers & (1<<i)); break;
				default: break;
				}
			}
		}
		m_check_drill.SetCheck( m_drill_file );
		m_check_outline.SetCheck( m_flags & GERBER_BOARD_OUTLINE );
		m_check_moires.SetCheck( m_flags & GERBER_AUTO_MOIRES );
		m_check_layer_text.SetCheck( m_flags & GERBER_LAYER_TEXT );
//BAF		m_check_thermal_pins.SetCheck( !(m_flags & GERBER_NO_PIN_THERMALS) );
//BAF		m_check_thermal_vias.SetCheck( !(m_flags & GERBER_NO_VIA_THERMALS) );
//BAF		m_check_smt_thermals.SetCheck( !(m_flags & GERBER_NO_SMT_THERMALS) );
//BAF		m_check_90.SetCheck( m_flags & GERBER_90_THERMALS );
		m_check_mask_vias.SetCheck( !(m_flags & GERBER_MASK_VIAS) );
		m_check_pilot.SetCheck( m_flags & GERBER_PILOT_HOLES );
		m_check_render_all.SetCheck( m_flags & GERBER_RENDER_ALL );
		m_check_mirror_bottom.SetCheck( m_flags & GERBER_MIRROR_BOTTOM_PNG );
		m_check_sm_clearance_for_cutouts.SetCheck( !(m_flags & GERBER_NO_CLEARANCE_SMCUTOUTS) );
		OnBnClickedCheckCadPilot();
		OnBnClickedRenderAllGerbers();
	}
}


BEGIN_MESSAGE_MAP(CDlgCAD, CDialog)
	ON_BN_CLICKED(ID_GO, OnBnClickedGo)
	ON_CBN_SELCHANGE(IDC_COMBO_CAD_UNITS, OnCbnSelchangeComboCadUnits)
	ON_BN_CLICKED(IDC_CHECK_CAD_PILOT, OnBnClickedCheckCadPilot)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_DEF, OnBnClickedButtonDef)
	ON_BN_CLICKED(IDC_BUTTON_FOLDER, OnBnClickedButtonFolder)
	ON_BN_CLICKED(IDC_CHECK_ALL_GERBERS, OnBnClickedRenderAllGerbers)
	ON_BN_CLICKED(IDC_BUTTON_DONE, OnBnClickedButtonDone)
END_MESSAGE_MAP()

void CDlgCAD::Initialize( double version, CString * folder, CString * project_folder,
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
						 CDlgLog * log )
{
	m_version = version;
	m_folder = *folder;
	m_project_folder = *project_folder;
	m_app_folder = *app_folder;
	m_units = units;
	m_mask_clearance = mask_clearance;
	m_pilot_diameter = pilot_diameter;
	m_min_silkscreen_width = min_silkscreen_wid;
	m_num_copper_layers = num_copper_layers;
	m_flags = flags;
	m_layers = layers;
	m_drill_file = drill_file;
	m_bd = bd;
	m_sm = sm;
	m_pl = pl;
	m_nl = nl;
	m_tl = tl;
	m_dl = dl;
	m_outline_width = outline_width;
	m_annular_ring_pins = annular_ring_pins;
	m_annular_ring_vias = annular_ring_vias;
	m_paste_shrink = shrink_paste;
	m_n_x = n_x;
	m_n_y = n_y;
	m_space_x = space_x;
	m_space_y = space_y;
	m_dlg_log = log;
}

// CDlgCAD message handlers

void CDlgCAD::OnBnClickedGo()
{
	// get CAM folder and create it if necessary
	struct _stat buf;
	m_edit_folder.GetWindowText( m_folder );
	if( m_folder == "" )
	{
		m_folder = m_project_folder + "\\CAM";
		m_edit_folder.SetWindowText( m_folder );
	}
	int err = _stat( m_folder, &buf );
	if( err )
	{
		CString str;
		str.Format( "Folder \"%s\" doesn't exist, create it ?", m_folder );
		int ret = AfxMessageBox( str, MB_YESNO );
		if( ret == IDYES )
		{
			err = _mkdir( m_folder );
			if( err )
			{
				str.Format( "Unable to create folder \"%s\"", m_folder );
				AfxMessageBox( str, MB_OK );
				return;
			}
		}
		else
			return;
	}
	GetFields();

	// show log
	m_dlg_log->ShowWindow( SW_SHOW );
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow();

	BOOL errors = FALSE;	// if errors occur

	// create drill file, if requested
	if( m_drill_file )
	{
		CStdioFile f;
		CString f_name = m_folder + "\\drill_file.drl";
		int ok = f.Open( f_name, CFile::modeCreate | CFile::modeWrite );
		if( !ok )
		{
			CString log_message;
			log_message.Format( "ERROR: Unable to open file \"%s\"\r\n", f_name );
			m_dlg_log->AddLine( log_message );
			errors = TRUE;
		}
		else
		{
			CString log_message;
			log_message.Format( "Writing file: \"%s\"\r\n", f_name );
			m_dlg_log->AddLine( log_message );

			::WriteDrillFile( &f, m_pl, m_nl, m_bd, m_n_x, m_n_y, m_space_x, m_space_y );
			f.Close();
		}
		m_drill_file = 1;
	}

	// create Gerber files for selected layers
	CArray<CString> commands;
	for( int i=0; i<23; i++ )
	{
		CString gerber_name = "";
		int layer = 0;
		if( i<(m_num_copper_layers+4) || i>19 )
		{
			switch(i)
			{
			case 0: if( m_layers & (1<<i) )
					{ gerber_name = "top_silk.grb"; layer = LAY_SILK_TOP; }
					break;
			case 1: if( m_layers & (1<<i) )
					{ gerber_name = "bottom_silk.grb"; layer = LAY_SILK_BOTTOM; }
					break;
			case 2: if( m_layers & (1<<i) )
					{ gerber_name = "top_solder_mask.grb"; layer = LAY_MASK_TOP; }
					break;
			case 3: if( m_layers & (1<<i) )
					{ gerber_name = "bottom_solder_mask.grb"; layer = LAY_MASK_BOTTOM; }
					break;
			case 4: if( m_layers & (1<<i) )
					{ gerber_name = "top_copper.grb"; layer = LAY_TOP_COPPER; }
					break;
			case 5: if( m_layers & (1<<i) )
					{ gerber_name = "bottom_copper.grb"; layer = LAY_BOTTOM_COPPER; }
					break;
			case 6: if( m_layers & (1<<i) )
					{ gerber_name = "inner_copper_1.grb"; layer = LAY_BOTTOM_COPPER+1; }
					break;
			case 7: if( m_layers & (1<<i) )
					{ gerber_name = "inner_copper_2.grb"; layer = LAY_BOTTOM_COPPER+2; }
					break;
			case 8: if( m_layers & (1<<i) )
					{ gerber_name = "inner_copper_3.grb"; layer = LAY_BOTTOM_COPPER+3; }
					break;
			case 9: if( m_layers & (1<<i) )
					{ gerber_name = "inner_copper_4.grb"; layer = LAY_BOTTOM_COPPER+4; }
					break;
			case 10: if( m_layers & (1<<i) )
					 { gerber_name = "inner_copper_5.grb"; layer = LAY_BOTTOM_COPPER+5; }
					 break;
			case 11: if( m_layers & (1<<i) )
					 { gerber_name = "inner_copper_6.grb"; layer = LAY_BOTTOM_COPPER+6; }
					 break;
			case 12: if( m_layers & (1<<i) )
					 { gerber_name = "inner_copper_7.grb"; layer = LAY_BOTTOM_COPPER+7; }
					 break;
			case 13: if( m_layers & (1<<i) )
					 { gerber_name = "inner_copper_8.grb"; layer = LAY_BOTTOM_COPPER+8; }
					 break;
			case 14: if( m_check_inner9.GetCheck() )
					 { gerber_name = "inner_copper_9.grb"; layer = LAY_BOTTOM_COPPER+9; }
					 break;
			case 15: if( m_layers & (1<<i) )
					 { gerber_name = "inner_copper_10.grb"; layer = LAY_BOTTOM_COPPER+10; }
					 break;
			case 16: if( m_layers & (1<<i) )
					 { gerber_name = "inner_copper_11.grb"; layer = LAY_BOTTOM_COPPER+11; }
					 break;
			case 17: if( m_layers & (1<<i) )
					 { gerber_name = "inner_copper_12.grb"; layer = LAY_BOTTOM_COPPER+12; }
					 break;
			case 18: if( m_check_inner13.GetCheck() )
					 { gerber_name = "inner_copper_13.grb"; layer = LAY_BOTTOM_COPPER+13; }
					 break;
			case 19: if( m_layers & (1<<i) )
					 { gerber_name = "inner_copper_14.grb"; layer = LAY_BOTTOM_COPPER+14; }
					 break;
			case 20: if( m_layers & (1<<i) )
					 { gerber_name = "board_outline.grb"; layer = LAY_BOARD_OUTLINE; }
					 break;
			case 21: if( m_layers & (1<<i) )
					 { gerber_name = "top_paste_mask.grb"; layer = LAY_PASTE_TOP; }
					 break;
			case 22: if( m_layers & (1<<i) )
					 { gerber_name = "bottom_paste_mask.grb"; layer = LAY_PASTE_BOTTOM; }
					 break;
			default: ASSERT(0);
				break;
			}
		}
		if( layer )
		{
			// write the gerber file
			m_pl->SetPinAnnularRing( m_annular_ring_pins );
			m_nl->SetViaAnnularRing( m_annular_ring_vias );
			CString f_str = m_folder + "\\" + gerber_name;
			CStdioFile f;
			int ok = f.Open( f_str, CFile::modeCreate | CFile::modeWrite );
			if( !ok )
			{
				CString log_message;
				log_message.Format( "ERROR: Unable to open file \"%s\"\r\n", f_str );
				m_dlg_log->AddLine( log_message );
				errors = TRUE;
			}
			else
			{
				CString log_message;
				log_message.Format( "Writing file: \"%s\"\r\n", f_str );
				m_dlg_log->AddLine( log_message );
				CString line;
				line.Format( "G04 FreePCB version %5.3f*\n", m_version );
				f.WriteString( line );
				line.Format( "G04 %s*\n", f_str );
				f.WriteString( line );
				::WriteGerberFile( &f, m_flags, layer,
					m_dlg_log, m_paste_shrink,
					100000 /* BAF fill clearance */, 
					m_mask_clearance, m_pilot_diameter,
					m_min_silkscreen_width, 
					100000 /*BAF -m_thermal_width */,
					m_outline_width, 
					100000 /* BAF-m_hole_clearance*/,
					m_n_x, m_n_y, m_space_x, m_space_y,
					m_bd, m_sm, m_pl, m_nl, m_tl, m_dl );
				f.WriteString( "M02*\n" );	// end of job
				f.Close();
			}
			if( m_flags & GERBER_RENDER_ALL )
			{
				// create command-line to render layer
				int nlines = commands.GetSize();
				commands.SetSize( nlines + 1 );
				CString mirror_str = "";
				if( layer == LAY_SILK_BOTTOM
					|| layer == LAY_SM_BOTTOM
					|| layer == LAY_BOTTOM_COPPER
					|| layer == LAY_MASK_BOTTOM
					|| layer == LAY_PASTE_BOTTOM )
				{
					if( m_flags & GERBER_MIRROR_BOTTOM_PNG )
						mirror_str = "--mirror ";
				}
				CString command_str = m_app_folder + "\\GerberRender.exe -r 600 " + mirror_str
					+ gerber_name + " " + gerber_name.Left( gerber_name.GetLength()-4 ) + ".png";
				commands[nlines] = command_str;
			}
		}
	}
	if( errors )
		m_dlg_log->AddLine( "****** ERRORS OCCURRED DURING CREATION OF GERBERS ******\r\n" );
	else
		m_dlg_log->AddLine( "************ ALL GERBERS CREATED SUCCESSFULLY **********\r\n" );
	if( commands.GetSize() != 0 )
	{
		_chdir( m_folder );		// change current working directory to CAM folder
		m_dlg_log->AddLine( "*********** RENDERING .PNG FILES FOR LAYERS ************\r\n" );
		for( int i=0; i<commands.GetSize(); i++ )
		{
			m_dlg_log->AddLine( "Run: " + commands[i] + "\r\n" );
			::RunConsoleProcess( commands[i], m_dlg_log );
			m_dlg_log->AddLine( "\r\n" );
		}
		m_dlg_log->AddLine( "************************* DONE *************************\r\n" );
		_chdir( m_app_folder );	// change back
	}
}

void CDlgCAD::GetFields()
{
	CString str;
	double mult;

	if( m_units == MIL )
		mult = NM_PER_MIL;
	else
		mult = 1000000.0;

	m_edit_mask.GetWindowText( str );
	m_mask_clearance = atof( str ) * mult;
	m_edit_pilot_diam.GetWindowText( str );
	m_pilot_diameter = atof( str ) * mult;
	m_edit_min_ss_w.GetWindowText( str );
	m_min_silkscreen_width = atof( str ) * mult;
	m_edit_outline_width.GetWindowText( str );
	m_outline_width = atof( str ) * mult;
	m_edit_ann_pins.GetWindowText( str );
	m_annular_ring_pins = atof( str ) * mult;
	m_edit_ann_vias.GetWindowText( str );
	m_annular_ring_vias = atof( str ) * mult;
	m_edit_shrink_paste.GetWindowText( str );
	m_paste_shrink = atof( str ) * mult;
	m_edit_space_x.GetWindowText( str );
	m_space_x = atof( str ) * mult;
	m_edit_space_y.GetWindowText( str );
	m_space_y = atof( str ) * mult;
	m_edit_n_x.GetWindowText( str );
	m_n_x = atoi( str );
	if( m_n_x < 1 )
	{
		m_n_x = 1;
		m_edit_n_x.SetWindowText( "1" );
	}
	m_edit_n_y.GetWindowText( str );
	m_n_y = atoi( str );
	if( m_n_y < 1 )
	{
		m_n_y = 1;
		m_edit_n_y.SetWindowText( "1" );
	}
	// get flags for Gerbers
	m_flags = 0;
	if( m_check_outline.GetCheck() )
		m_flags |= GERBER_BOARD_OUTLINE;
	if( m_check_moires.GetCheck() )
		m_flags |= GERBER_AUTO_MOIRES;
	if( m_check_layer_text.GetCheck() )
		m_flags |= GERBER_LAYER_TEXT;
	if( m_check_pilot.GetCheck() )
		m_flags |= GERBER_PILOT_HOLES;

//BAF	if( !m_check_thermal_pins.GetCheck() )		m_flags |= GERBER_NO_PIN_THERMALS;
//BAF	if( !m_check_thermal_vias.GetCheck() )		m_flags |= GERBER_NO_VIA_THERMALS;
//BAF	if( !m_check_smt_thermals.GetCheck() )		m_flags |= GERBER_NO_SMT_THERMALS;
//BAF	if( m_check_90.GetCheck() )		m_flags |= GERBER_90_THERMALS;

	if( !m_check_mask_vias.GetCheck() )
		m_flags |= GERBER_MASK_VIAS;
	if( m_check_render_all.GetCheck() )
		m_flags |= GERBER_RENDER_ALL;
	if( m_check_render_all.GetCheck() && m_check_mirror_bottom.GetCheck() )
		m_flags |= GERBER_MIRROR_BOTTOM_PNG;
	if( !m_check_sm_clearance_for_cutouts.GetCheck() )
		m_flags |= GERBER_NO_CLEARANCE_SMCUTOUTS;
	// get layers
	m_layers = 0x0;
	for( int i=0; i<23; i++ )
	{
		if( i<(m_num_copper_layers+4) || i>19 )
		{
			switch(i)
			{
			case 0: if( m_check_top_silk.GetCheck() )
						m_layers |= 1<<i;
					break;
			case 1: if( m_check_bottom_silk.GetCheck() )
						m_layers |= 1<<i;
					break;
			case 2: if( m_check_top_solder.GetCheck() )
						m_layers |= 1<<i;
					break;
			case 3: if( m_check_bottom_solder.GetCheck() )
						m_layers |= 1<<i;
					break;
			case 4: if( m_check_top_copper.GetCheck() )
						m_layers |= 1<<i;
					break;
			case 5: if( m_check_bottom_copper.GetCheck() )
						m_layers |= 1<<i;
					break;
			case 6: if( m_check_inner1.GetCheck() )
						m_layers |= 1<<i;
					break;
			case 7: if( m_check_inner2.GetCheck() )
						m_layers |= 1<<i;
					break;
			case 8: if( m_check_inner3.GetCheck() )
						m_layers |= 1<<i;
					break;
			case 9: if( m_check_inner4.GetCheck() )
						m_layers |= 1<<i;
					break;
			case 10: if( m_check_inner5.GetCheck() )
						m_layers |= 1<<i;
					 break;
			case 11: if( m_check_inner6.GetCheck() )
						m_layers |= 1<<i;
					 break;
			case 12: if( m_check_inner7.GetCheck() )
						m_layers |= 1<<i;
					 break;
			case 13: if( m_check_inner8.GetCheck() )
						m_layers |= 1<<i;
					 break;
			case 14: if( m_check_inner9.GetCheck() )
						m_layers |= 1<<i;
					 break;
			case 15: if( m_check_inner10.GetCheck() )
						m_layers |= 1<<i;
					 break;
			case 16: if( m_check_inner11.GetCheck() )
						m_layers |= 1<<i;
					 break;
			case 17: if( m_check_inner12.GetCheck() )
						m_layers |= 1<<i;
					 break;
			case 18: if( m_check_inner13.GetCheck() )
						m_layers |= 1<<i;
					 break;
			case 19: if( m_check_inner14.GetCheck() )
						m_layers |= 1<<i;
					 break;
			case 20: if( m_check_board.GetCheck() )
						m_layers |= 1<<i;
					 break;
			case 21: if( m_check_top_paste.GetCheck() )
						m_layers |= 1<<i;
					 break;
			case 22: if( m_check_bottom_paste.GetCheck() )
						m_layers |= 1<<i;
					 break;
			default: ASSERT(0);
				break;
			}
		}
	}
	if( m_check_drill.GetCheck() )
		m_drill_file = 1;
	else
		m_drill_file = 0;
}

void CDlgCAD::SetFields()
{
	CString str;
	double mult;

	if( m_units == MIL )
		mult = NM_PER_MIL;
	else
		mult = 1000000.0;

	MakeCStringFromDouble( &str, m_mask_clearance/mult );
	m_edit_mask.SetWindowText( str );
	MakeCStringFromDouble( &str, m_pilot_diameter/mult );
	m_edit_pilot_diam.SetWindowText( str );
	MakeCStringFromDouble( &str, m_min_silkscreen_width/mult );
	m_edit_min_ss_w.SetWindowText( str );
	MakeCStringFromDouble( &str, m_outline_width/mult );
	m_edit_outline_width.SetWindowText( str );
	MakeCStringFromDouble( &str, m_annular_ring_pins/mult );
	m_edit_ann_pins.SetWindowText( str );
	MakeCStringFromDouble( &str, m_annular_ring_vias/mult );
	m_edit_ann_vias.SetWindowText( str );
	MakeCStringFromDouble( &str, m_paste_shrink/mult );
	m_edit_shrink_paste.SetWindowText( str );
	MakeCStringFromDouble( &str, m_space_x/mult );
	m_edit_space_x.SetWindowText( str );
	MakeCStringFromDouble( &str, m_space_y/mult );
	m_edit_space_y.SetWindowText( str );
}
void CDlgCAD::OnCbnSelchangeComboCadUnits()
{
	GetFields();
	if( m_combo_units.GetCurSel() == 0 )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}

void CDlgCAD::OnBnClickedCheckCadPilot()
{
	if( m_check_pilot.GetCheck() )
		m_edit_pilot_diam.EnableWindow( 1 );
	else
		m_edit_pilot_diam.EnableWindow( 0 );
}

void CDlgCAD::OnBnClickedCancel()
{
	OnCancel();
}

void CDlgCAD::OnBnClickedButtonDef()
{
		m_folder = m_project_folder + "\\CAM";
		m_edit_folder.SetWindowText( m_folder );
}

void CDlgCAD::OnBnClickedButtonFolder()
{
	if( m_folder == "" )
	{
		m_folder = m_project_folder + "\\CAM";
		m_edit_folder.SetWindowText( m_folder );
	}
	CPathDialog dlg( "Select Folder", "Set CAM output folder", m_folder );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_folder = dlg.GetPathName();
		m_edit_folder.SetWindowText( m_folder );
	}
}

void CDlgCAD::OnBnClickedRenderAllGerbers()
{
	m_check_mirror_bottom.EnableWindow( m_check_render_all.GetCheck() );
}


void CDlgCAD::OnBnClickedButtonDone()
{
	GetFields();
	OnOK();
}
