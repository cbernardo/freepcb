// DlgReport.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgReport.h"
#include "FreePcbDoc.h"
#include "Gerber.h"
#include "Net_iter.h"
#include "PartListNew.h"
#include "NetListNew.h"
#include "utility.h"

// CDlgReport dialog

IMPLEMENT_DYNAMIC(CDlgReport, CDialog)
CDlgReport::CDlgReport(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgReport::IDD, pParent) 
{
}

CDlgReport::~CDlgReport()
{
}

void CDlgReport::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_STATS, m_check_board);
	DDX_Control(pDX, IDC_CHECK_DRILL, m_check_drill);
	DDX_Control(pDX, IDC_CHECK_PARTS, m_check_parts);
	DDX_Control(pDX, IDC_CHECK_CAM, m_check_cam);
	DDX_Control(pDX, IDC_CHECK_DRC_PARAMS, m_check_drc_params);
	DDX_Control(pDX, IDC_CHECK_DRC, m_check_drc);
	DDX_Control(pDX, IDC_CHECK_CONNECTIVITY, m_check_connectivity);
	DDX_Control(pDX, IDC_RADIO_INCH, m_radio_inch);
	DDX_Control(pDX, IDC_RADIO_MM, m_radio_mm);
	DDX_Control(pDX, IDC_RADIO_CCW, m_radio_ccw);
	DDX_Control(pDX, IDC_RADIO_TOP, m_radio_top);
	DDX_Radio(pDX, IDC_RADIO_CCW, m_ccw);
	DDX_Radio(pDX, IDC_RADIO_TOP, m_top);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_check_board.SetCheck( !(m_flags & NO_PCB_STATS) );
		m_check_drill.SetCheck( !(m_flags & NO_DRILL_LIST) );
		m_check_parts.SetCheck( !(m_flags & NO_PARTS_LIST) );
		m_check_cam.SetCheck( !(m_flags & NO_CAM_PARAMS) );
		m_check_drc_params.SetCheck( !(m_flags & NO_DRC_PARAMS) );
		m_check_drc.SetCheck( m_flags & DRC_LIST );
		m_check_connectivity.SetCheck( m_flags & CONNECTIVITY_LIST );
		if( m_flags & USE_MM )
			m_radio_mm.SetCheck( 1 );
		else
			m_radio_inch.SetCheck( 1 );
	}
	else
	{
		// outgoing
	}
}

void CDlgReport::Initialize( CFreePcbDoc * doc )
{
	m_doc = doc;
	m_pl = doc->m_plist;
	m_nl = doc->m_nlist;
	m_flags = doc->m_report_flags;
	m_ccw = (m_flags & CW)/CW;		// set to 0 to use CCW (default)
	m_top = 1 - (m_flags & TOP)/TOP;	// set to 1 to use bottom (default)
}


BEGIN_MESSAGE_MAP(CDlgReport, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


// CDlgReport message handlers

// global helper for qsort(), for quicksort of array of part pointers
//
int mycompare( const void *arg1, const void *arg2 )
{
	cpart2 * p1 = *(cpart2**)arg1;
	cpart2* p2 = *(cpart2**)arg2;
	return CompareNumeric(&p1->ref_des, &p2->ref_des );		
}

void CDlgReport::OnBnClickedOk()
{
	CString line, str1, str2, str3, str4, str_units;
	int dp;			// decimal places for dimensions
	m_flags = 0;

	if( m_radio_ccw.GetCheck() == 0 )
		m_flags = m_flags | CW;	// set flag if cw
	if( m_radio_top.GetCheck() )
		m_flags = m_flags | TOP;	// set flag if top
	if( m_radio_mm.GetCheck() )
	{
		m_flags |= USE_MM;
		m_units = MM;
		dp = 3;
		str_units = "MM";
	}
	else
	{
		m_units = MIL;
		dp = 1;
		str_units = "MIL";
	}
	if( !m_check_board.GetCheck() )
		m_flags |= NO_PCB_STATS;
	if( !m_check_drill.GetCheck() )
		m_flags |= NO_DRILL_LIST;
	if( !m_check_parts.GetCheck() )
		m_flags |= NO_PARTS_LIST;
	if( !m_check_cam.GetCheck() )
		m_flags |= NO_CAM_PARAMS;
	if( !m_check_drc_params.GetCheck() )
		m_flags |= NO_DRC_PARAMS;
	if( m_check_drc.GetCheck() )
		m_flags |= DRC_LIST;
	if( m_check_connectivity.GetCheck() )
		m_flags |= CONNECTIVITY_LIST;
	m_doc->m_report_flags = m_flags;
	m_doc->ProjectModified( TRUE );

	CString fn = m_doc->m_path_to_folder + "\\" + "report.txt"; 
	CStdioFile file;
	int ok = file.Open( fn, CFile::modeCreate | CFile::modeWrite ); 
	if( !ok )     
	{ 
		CString s ((LPCSTR) IDS_UnableToOpenFile), mess;
		mess.Format(s, fn);
		AfxMessageBox( mess, MB_OK ); 
		OnCancel();
	}
	CString s ((LPCSTR) IDS_FreePCBProjectReport), mess;
	mess.Format(s,  str_units);
	file.WriteString( mess );
	s.LoadStringA(IDS_ProjectName);
	mess.Format(s, m_doc->m_name);
	file.WriteString( mess );
	s.LoadStringA(IDS_ProjectFile);
	mess.Format(s, m_doc->m_pcb_full_path);
	file.WriteString( mess );
	s.LoadStringA(IDS_DefaultLibraryFolder);
	mess.Format(s, m_doc->m_full_lib_dir);
	file.WriteString( mess );
	CMap <int,int,int,int> hole_size_map;

	if( !(m_flags & NO_PCB_STATS) )
	{
		CString s ((LPCSTR) IDS_BoardStatistics);
		file.WriteString( s );
		s.LoadStringA(IDS_NumberOfCopperLayers);
		mess.Format(s, m_doc->m_num_copper_layers);
		file.WriteString( mess );
		s.LoadStringA(IDS_NumberOfBoardOutlines);
		mess.Format(s, m_doc->boards.GetSize());
		file.WriteString( mess );
		CRect all_board_bounds;
		all_board_bounds.left = INT_MAX;
		all_board_bounds.bottom = INT_MAX;
		all_board_bounds.right = INT_MIN;
		all_board_bounds.top = INT_MIN;
		citer<cboard> ib (&m_doc->boards);
		for (cboard *b = ib.First(); b; b = ib.Next())
		{
			CRect r = b->GetBounds();
			all_board_bounds.left = min( all_board_bounds.left, r.left );
			all_board_bounds.right = max( all_board_bounds.right, r.right );
			all_board_bounds.bottom = min( all_board_bounds.bottom, r.bottom );
			all_board_bounds.top = max( all_board_bounds.top, r.top );
		}
		int x = abs(all_board_bounds.right - all_board_bounds.left);
		int y = abs(all_board_bounds.top - all_board_bounds.bottom);
		::MakeCStringFromDimension( &str1, x, m_units, TRUE, FALSE, TRUE, 3 ); 
		::MakeCStringFromDimension( &str2, y, m_units, TRUE, FALSE, TRUE, 3 );
		s.LoadStringA(IDS_BoardOutlinesExtent);
		mess.Format(s, str1, str2);
		file.WriteString( mess );
	}

	int num_parts = 0;
	int num_parts_with_fp = 0;
	int num_pins = 0;
	int num_th_pins = 0;
	int num_nets = 0;
	int num_vias = 0;
	citer<cpart2> ip (&m_pl->parts);
	for (cpart2 *part = ip.First(); part; part = ip.Next())
	{
		num_parts++;
		if( !part->shape )
			continue;
		num_parts_with_fp++;
		citer<cpin2> ipin (&part->pins);
		for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
		{
			num_pins++;
			int hole_size = pin->ps->hole_size;
			if( hole_size > 0 )
			{
				int num_holes;
				BOOL bFound = hole_size_map.Lookup( hole_size, num_holes );
				if( bFound )
					hole_size_map.SetAt( hole_size, num_holes+1 );
				else
					hole_size_map.SetAt( hole_size, 1 );
				num_th_pins++;
			}
		}
	}
	if( !(m_flags & NO_PCB_STATS) )
	{
		if( num_parts_with_fp != num_parts )
			s.LoadStringA(IDS_NumberOfParts),
			line.Format( s, num_parts, num_parts-num_parts_with_fp );
		else
			s.LoadStringA(IDS_NumberOfParts2),
			line.Format( s, num_parts );
		file.WriteString( line );
		s.LoadStringA(IDS_NumberOfPins);
		line.Format( s,	num_pins, num_th_pins, num_pins-num_th_pins );
		file.WriteString( line );
	}

	citer<cnet2> in (&m_nl->nets);
	for (cnet2 *net = in.First(); net; net = in.Next())
	{
		num_nets++;
		citer<cconnect2> ic (&net->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
		{
			citer<cvertex2> iv (&c->vtxs);
			for (cvertex2 *v = iv.First(); v; v = iv.Next())
			{
				int hole_size = v->via_hole_w; 
				if( hole_size > 0 )
				{
					int num_holes;
					BOOL bFound = hole_size_map.Lookup( hole_size, num_holes );
					if( bFound )
						hole_size_map.SetAt( hole_size, num_holes+1 );
					else
						hole_size_map.SetAt( hole_size, 1 );
					num_vias++;
				}
			}
		}
	}
	if( !(m_flags & NO_PCB_STATS) )
	{
		s.LoadStringA(IDS_NumberOfVias);
		line.Format( s, num_vias );
		file.WriteString( line );
		s.LoadStringA(IDS_NumberOfHoles);
		line.Format( s, num_vias + num_th_pins );
		file.WriteString( line );
	}

	if( !(m_flags & NO_DRILL_LIST) )
	{
		s.LoadStringA(IDS_DrillList);
		file.WriteString( s ); 
		POSITION pos = hole_size_map.GetStartPosition();
		int hole_size;
		int num_holes;
		while (pos != NULL)
		{
			hole_size_map.GetNextAssoc( pos, hole_size, num_holes );
			if( m_units == MIL )
			{
				::MakeCStringFromDimension( &str1, hole_size, MIL, TRUE, FALSE, TRUE, 1 );
				line.Format( "%5d   %s\n", num_holes, str1 );
			}
			else
			{
				::MakeCStringFromDimension( &str1, hole_size, MM, TRUE, FALSE, TRUE, 3 );
				::MakeCStringFromDimension( &str2, hole_size, MIL, TRUE, FALSE, TRUE, 1 );
				line.Format( "%5d   %s (%s)\n", num_holes, str1, str2 );
			}
			file.WriteString( line );
		}
	}
	if( !(m_flags & NO_PARTS_LIST) )
	{
		// make array of pointers to all parts, used for sorting
		int nparts = m_pl->parts.GetSize();
		cpart2 ** parts = (cpart2**) malloc( nparts * sizeof(cpart2*) );
		citer<cpart2> ip (&m_pl->parts);
		int i = 0;
		for (cpart2 *p = ip.First(); p; p = ip.Next())
			parts[i++] = p;
		// quicksort
		qsort( parts, nparts, sizeof(cpart2*), mycompare );
		// make arrays of strings for table
		CArray <CString> ref_des, package, value, footprint, pins, holes; 
		CArray <CString> side, angle, c_x, c_y, p1_x, p1_y;
		CArray< CArray <CString> > dot_w, dot_x, dot_y;
		ref_des.SetSize(nparts);
		package.SetSize(nparts);
		value.SetSize(nparts);
		footprint.SetSize(nparts); 
		pins.SetSize(nparts); 
		holes.SetSize(nparts); 
		side.SetSize(nparts);
		angle.SetSize(nparts);
		c_x.SetSize(nparts);
		c_y.SetSize(nparts);
		p1_x.SetSize(nparts);
		p1_y.SetSize(nparts);
		dot_w.SetSize(nparts);
		dot_x.SetSize(nparts);
		dot_y.SetSize(nparts);
		int maxlen_ref_des = 3;
		int maxlen_package = 7;
		int maxlen_value = 5;
		int maxlen_footprint = 9;
		int maxlen_pins = 4;
		int maxlen_holes = 5;
		int maxlen_side = 6;
		int maxlen_angle = 5;
		int maxlen_y = 5;
		int maxlen_c_x = 6;
		int maxlen_c_y = 6;
		int maxlen_p1_x = 6;
		int maxlen_p1_y = 6;
		int maxnum_dots = 0;
		int maxlen_dot_w = 6;
		int maxlen_dot_x = 6;
		int maxlen_dot_y = 6;
		int dp;
		CString s ((LPCSTR) IDS_PartList);
		file.WriteString( s );
		CString cw_str ((LPCSTR) IDS_Counterclockwise);
		if( m_flags & CW )
			cw_str.LoadStringA(IDS_Clockwise);
		s.LoadStringA(IDS_ForPartsOnTopTheAngleIs);
		line.Format( s, cw_str );
		file.WriteString( line );
		if( m_flags & TOP )
			s.LoadStringA(IDS_ForPartsOnTheBottomTheAngleIs);
		else
			s.LoadStringA(IDS_ForPartsOnTheBottomTheAngleIs2);
		line.Format(s, cw_str );
		file.WriteString( line );
		s.LoadStringA(IDS_AllCentroidCoordinatesAreAsViewedFromTheTop);
		file.WriteString( s );
		if( m_units == MIL )
			dp = 1;
		else if( m_units == MM )
			dp = 3;
		else
			ASSERT(0);
		for( int ip = 0; ip < nparts; ip++ )
		{
			cpart2 *part = parts[ip];
			ref_des[ip] = part->ref_des;
			package[ip] = part->package;
			value[ip] = part->value_text;
			footprint[ip] = "";
			pins[ip] = "";
			holes[ip] = "";
			c_x[ip] = "";
			c_y[ip] = "";
			p1_x[ip] = "";
			p1_y[ip] = "";
			angle[ip] = "";
			if( part->shape )  
			{
				BOOL bSMT = TRUE;
				int nholes = 0;
				citer<cpin2> ipin (&part->pins);
				for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
					if( pin->ps->hole_size > 0 )
					{
						bSMT = FALSE;
						nholes++;
					}
				footprint.SetAtGrow( ip, part->shape->m_name );
				str1.Format( "%d", part->pins.GetSize() );
				pins[ip] = str1;
				str1.Format( "%d", nholes );
				holes[ip] = str1;
				if( part->side == 0 )
					str1.LoadStringA(IDS_Top2);
				else
					str1.LoadStringA(IDS_Bottom);
				side[ip] = str1;
				int a = ::GetReportedAngleForPart( part->angle, part->shape->m_centroid->m_angle, part->side );				
				if( m_flags & CW )
					a = ccw(a);		// degrees cw
				if( !(m_flags & TOP) && (a == 0 || a == 180) )
					a = 180 - a;	// viewed from bottom, flipped left-right
				str1.Format( "%d", a );
				angle[ip] = str1;
				CPoint centroid_pt = part->GetCentroidPoint();
				::MakeCStringFromDimension( &str1, centroid_pt.x, m_units, FALSE, FALSE, TRUE, dp );
				c_x[ip] = str1;
				::MakeCStringFromDimension( &str1, centroid_pt.y, m_units, FALSE, FALSE, TRUE, dp );
				c_y[ip] = str1;
				cpin2 *pin1 = part->GetPinByName( &CString("1") );
				if( !pin1 )
					pin1 = part->GetPinByName( &CString("A1") );
				if( pin1 )
				{
					CPoint pt1 (pin1->x, pin1->y );
					::MakeCStringFromDimension( &str1, pt1.x, m_units, FALSE, FALSE, TRUE, dp );
					p1_x[ip] = str1;
					::MakeCStringFromDimension( &str1, pt1.y, m_units, FALSE, FALSE, TRUE, dp );
					p1_y[ip] = str1;
				}

				int ndots = part->shape->m_glues.GetSize();
				if( ndots == 0 && bSMT )
					ndots = 1;
				maxnum_dots = ndots;
				CArray<CString> * g_w_str = &dot_w[ip];	// pointer to array of glue widths
				CArray<CString> * g_x_str = &dot_x[ip];	// pointer to array of glue x
				CArray<CString> * g_y_str = &dot_y[ip];	// pointer to array of glue y
				g_w_str->SetSize( ndots );
				g_x_str->SetSize( ndots );
				g_y_str->SetSize( ndots );
				if (part->shape->m_glues.GetSize()==0 && bSMT)
				{
					int g_w = m_doc->m_default_glue_w;
					CPoint g_pt = part->GetCentroidPoint();
					CString temp;
					::MakeCStringFromDimension( &temp, g_w, m_units, FALSE, FALSE, TRUE, dp );
					(*g_w_str)[0] = temp;
					maxlen_dot_w = max( maxlen_dot_w, temp.GetLength() );
					::MakeCStringFromDimension( &temp, g_pt.x, m_units, FALSE, FALSE, TRUE, dp );
					(*g_x_str)[0] = temp;
					maxlen_dot_x = max( maxlen_dot_x, temp.GetLength() );
					::MakeCStringFromDimension( &temp, g_pt.y, m_units, FALSE, FALSE, TRUE, dp );
					(*g_y_str)[0] = temp;
					maxlen_dot_y = max( maxlen_dot_y, temp.GetLength() );
				}
				else
				{
					citer<cglue> ig (&part->shape->m_glues);
					int idot = 0;
					for (cglue *g = ig.First(); g; g = ig.Next(), idot++)
					{
						int g_w = g->w;
						if( g_w == 0 )
							g_w = m_doc->m_default_glue_w;
						CPoint g_pt = part->GetGluePoint( g );
						CString temp;
						::MakeCStringFromDimension( &temp, g_w, m_units, FALSE, FALSE, TRUE, dp );
						(*g_w_str)[idot] = temp;
						maxlen_dot_w = max( maxlen_dot_w, temp.GetLength() );
						::MakeCStringFromDimension( &temp, g_pt.x, m_units, FALSE, FALSE, TRUE, dp );
						(*g_x_str)[idot] = temp;
						maxlen_dot_x = max( maxlen_dot_x, temp.GetLength() );
						::MakeCStringFromDimension( &temp, g_pt.y, m_units, FALSE, FALSE, TRUE, dp );
						(*g_y_str)[idot] = temp;
						maxlen_dot_y = max( maxlen_dot_y, temp.GetLength() );
					}
				}
			}

			maxlen_ref_des = max( maxlen_ref_des, ref_des[ip].GetLength() );
			maxlen_package = max( maxlen_package, package[ip].GetLength() );
			maxlen_value = max( maxlen_value, value[ip].GetLength() );
			maxlen_footprint = max( maxlen_footprint, footprint[ip].GetLength() );
			maxlen_pins = max( maxlen_pins, pins[ip].GetLength() );
			maxlen_holes = max( maxlen_holes, holes[ip].GetLength() );
			maxlen_c_x = max( maxlen_c_x, c_x[ip].GetLength() );
			maxlen_c_y = max( maxlen_c_y, c_y[ip].GetLength() );
			maxlen_p1_x = max( maxlen_p1_x, p1_x[ip].GetLength() );
			maxlen_p1_y = max( maxlen_p1_y, p1_y[ip].GetLength() );
		}

		CString format_str;
		format_str.Format( "%%%ds  %%%ds  %%%ds  %%%ds  %%%ds  %%%ds  %%%ds  %%%ds  %%%ds  %%%ds  %%%ds  %%%ds", 
			maxlen_ref_des, maxlen_package, maxlen_value, maxlen_footprint, maxlen_pins, maxlen_holes,
			maxlen_side, maxlen_angle, 
			maxlen_c_x, maxlen_c_y, maxlen_p1_x, maxlen_p1_y );
		CString dot_format_str, fields[15];
		dot_format_str.Format( "  %%%ds  %%%ds  %%%ds", 
				maxlen_dot_w, maxlen_dot_x, maxlen_dot_y );
		for (int i=0; i<15; i++)
			fields[i].LoadStringA(IDS_ReportFields+i);
		str1.Format( format_str, fields[0], fields[1], fields[2], fields[3], fields[4], fields[5],
			fields[6], fields[7], fields[8], fields[9], fields[10], fields[11] );
		for( int id=0; id<maxnum_dots; id++ ) 
		{
			CString temp;
			temp.Format( dot_format_str, fields[12], fields[13], fields[14] );
			str1 += temp;
		}
		file.WriteString( str1 + "\n" );
		str1.Format( format_str, "---", "-------", "-----", "---------", "----", "-----",
			"----", "-----", "------", "------", "------", "------" );
		for( int id=0; id<maxnum_dots; id++ )
		{
			CString temp;
			temp.Format( dot_format_str, "------", "------", "------" );
			str1 += temp;
		}
		file.WriteString( str1 + "\n" );

		for( int ip=0; ip<ref_des.GetSize(); ip++ )
		{
			CString pad_ref_des;
			str1.Format( format_str, ref_des[ip], package[ip], value[ip], footprint[ip],
				pins[ip], holes[ip], side[ip], angle[ip], c_x[ip], c_y[ip], p1_x[ip], p1_y[ip] );
			CArray< CString > * g_w_str = &dot_w[ip];
			for( int id=0; id<g_w_str->GetSize(); id++ )
			{
				CString temp;
				temp.Format( dot_format_str, dot_w[ip][id], dot_x[ip][id], dot_y[ip][id] );
				str1 += temp;
			}
			file.WriteString( str1 + "\n" );
		}
	}

	if( !(m_flags & NO_CAM_PARAMS) )  
	{
		CString s ((LPCSTR) IDS_GerberFileSettings);
		file.WriteString( s );
		CString str_SMT_connect ((LPCSTR) IDS_No);
		CString str_board_outline ((LPCSTR) IDS_No);
		CString str_moires ((LPCSTR) IDS_No);
		CString str_layer_desc_text ((LPCSTR) IDS_No);
		CString str_pilot_holes ((LPCSTR) IDS_No);
		CString str_SMT_thermals ((LPCSTR) IDS_Yes);
		CString str_pin_thermals ((LPCSTR) IDS_Yes);
		CString str_via_thermals ((LPCSTR) IDS_Yes);
		CString str_SM_cutouts_vias ((LPCSTR) IDS_No);
		CString str_90_degree_thermals((LPCSTR) IDS_No);
		if( m_doc->m_cam_flags & GERBER_BOARD_OUTLINE )
			str_board_outline.LoadStringA(IDS_Yes);
		if( m_doc->m_cam_flags & GERBER_AUTO_MOIRES )
			str_moires.LoadStringA(IDS_Yes);
		if( m_doc->m_cam_flags & GERBER_LAYER_TEXT )
			str_layer_desc_text.LoadStringA(IDS_Yes);
		if( m_doc->m_cam_flags & GERBER_PILOT_HOLES )
			str_pilot_holes.LoadStringA(IDS_Yes);
		if( m_doc->m_bSMT_copper_connect )
			str_SMT_connect.LoadStringA(IDS_Yes);
		if( m_doc->m_cam_flags & GERBER_NO_SMT_THERMALS )
			str_SMT_thermals.LoadStringA(IDS_No);
		if( m_doc->m_cam_flags & GERBER_NO_PIN_THERMALS )
			str_pin_thermals.LoadStringA(IDS_No);
		if( m_doc->m_cam_flags & GERBER_NO_VIA_THERMALS )
			str_via_thermals.LoadStringA(IDS_No);
		if( m_doc->m_cam_flags & GERBER_MASK_VIAS )
			str_SM_cutouts_vias.LoadStringA(IDS_Yes);
		if( m_doc->m_cam_flags & GERBER_90_THERMALS )
			str_90_degree_thermals.LoadStringA(IDS_Yes);
		CString line;
		s.LoadStringA(IDS_IncludeBoardOutline);
		line.Format(s, str_board_outline);
		file.WriteString( line );
		s.LoadStringA(IDS_IncludeMoires);
		line.Format(s, str_moires);
		file.WriteString( line );
		s.LoadStringA(IDS_IncludeLayerDescriptionText);
		line.Format(s, str_layer_desc_text);
		file.WriteString( line );
		s.LoadStringA(IDS_IncludePilotHoles);
		line.Format(s, str_pilot_holes);
		file.WriteString( line );
		s.LoadStringA(IDS_ConnectSMTPinsToCopperAreas);
		line.Format(s, str_SMT_connect);
		file.WriteString( line );
		s.LoadStringA(IDS_UseThermalsForSMTPins);
		line.Format(s, str_SMT_thermals);
		file.WriteString( line );
		s.LoadStringA(IDS_UseThermalsForThroughHolePins);
		line.Format(s, str_pin_thermals);
		file.WriteString( line );
		s.LoadStringA(IDS_UseThermalsForVias);
		line.Format(s, str_via_thermals);
		file.WriteString( line );
		s.LoadStringA(IDS_MakeSolderMaskCutoutsForVias);
		line.Format(s, str_SM_cutouts_vias);
		file.WriteString( line );
		s.LoadStringA(IDS_Use90DegreeThermalsForRoundPins);
		line.Format(s, str_90_degree_thermals);
		file.WriteString( line );
		CString str_fill_clearance;
		CString str_mask_clearance;
		CString str_paste_shrink;
		CString str_thermal_width;
		CString str_min_silkscreen_stroke_wid;
		CString str_pilot_diameter;
		CString str_outline_width;
		CString str_hole_clearance;
		CString str_cam_flags;
		CString str_cam_layers;
		CString str_cam_drill_file;
		CString str_annular_ring_pins;
		CString str_annular_ring_vias;
		CString str_n_x; 
		CString str_n_y; 
		CString str_space_x; 
		CString str_space_y;
		::MakeCStringFromDimension( &str_fill_clearance, m_doc->m_fill_clearance,
			m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_mask_clearance, m_doc->m_mask_clearance,
			m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_paste_shrink, m_doc->m_paste_shrink,
			m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_thermal_width, m_doc->m_thermal_width,
			m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_min_silkscreen_stroke_wid, 
			m_doc->m_min_silkscreen_stroke_wid, m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_pilot_diameter, m_doc->m_pilot_diameter,
			m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_outline_width, m_doc->m_outline_width,
			m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_hole_clearance, m_doc->m_hole_clearance,
			m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_annular_ring_pins, m_doc->m_annular_ring_pins,
			m_units, TRUE, FALSE, TRUE, dp );
		::MakeCStringFromDimension( &str_annular_ring_vias, m_doc->m_annular_ring_vias,
			m_units, TRUE, FALSE, TRUE, dp );
		s.LoadStringA(IDS_CopperToCopperFillClearance);
		line.Format(s, str_fill_clearance);
		file.WriteString( line );
		s.LoadStringA(IDS_HoleEdgeToCopperFillClearance);
		line.Format(s, str_hole_clearance);
		file.WriteString( line );
		s.LoadStringA(IDS_SolderMaskClearance);
		line.Format(s, str_mask_clearance);
		file.WriteString( line );
		s.LoadStringA(IDS_PilotHoleDiameter);
		line.Format(s, str_pilot_diameter);
		file.WriteString( line );
		s.LoadStringA(IDS_MinimumSilkscreenLineWidth);
		line.Format(s, str_min_silkscreen_stroke_wid);
		file.WriteString( line );
		s.LoadStringA(IDS_ThermalReliefLineWidth);
		line.Format(s, str_thermal_width);
		file.WriteString( line );
		s.LoadStringA(IDS_BoardOutlineLineWidth);
		line.Format(s, str_outline_width);
		file.WriteString( line );
		s.LoadStringA(IDS_MinimumAnnularRingWidthPins);
		line.Format(s, str_annular_ring_pins);
		file.WriteString( line );
		s.LoadStringA(IDS_MinimumAnnularRingWidthVias);
		line.Format(s, str_annular_ring_vias);
		file.WriteString( line );
		s.LoadStringA(IDS_AmountToShrinkSMTPadsForPasteMask);
		line.Format(s, str_paste_shrink);
		file.WriteString( line );
	}

	if( !(m_flags & NO_DRC_PARAMS) )  
	{
		CString s ((LPCSTR) IDS_DRCSettings);
		file.WriteString( s );
		CString str_min_trace_width;
		::MakeCStringFromDimension( &str_min_trace_width, m_doc->m_dr.trace_width,
			m_units, TRUE, FALSE, TRUE, dp ); 
		s.LoadStringA(IDS_MinimumTraceWidth);
		line.Format(s, str_min_trace_width);
		file.WriteString( line );
		CString str_min_pad_pad;
		::MakeCStringFromDimension( &str_min_pad_pad, m_doc->m_dr.pad_pad,
			m_units, TRUE, FALSE, TRUE, dp ); 
		s.LoadStringA(IDS_MinimumPadToPadClearance);
		line.Format(s, str_min_pad_pad);
		file.WriteString( line );
		CString str_min_pad_trace;
		::MakeCStringFromDimension( &str_min_pad_trace, m_doc->m_dr.pad_trace,
			m_units, TRUE, FALSE, TRUE, dp ); 
		s.LoadStringA(IDS_MinimumPadToTraceClearance);
		line.Format(s, str_min_pad_trace);
		file.WriteString( line );
		CString str_min_trace_trace;
		::MakeCStringFromDimension( &str_min_trace_trace, m_doc->m_dr.trace_trace,
			m_units, TRUE, FALSE, TRUE, dp ); 
		s.LoadStringA(IDS_MinimumTraceToTraceClearance);
		line.Format(s, str_min_trace_trace);
		file.WriteString( line );
		CString str_min_hole_copper;
		::MakeCStringFromDimension( &str_min_hole_copper, m_doc->m_dr.hole_copper,
			m_units, TRUE, FALSE, TRUE, dp );
		s.LoadStringA(IDS_MinimumHoleToCopperClearance);
		line.Format(s, str_min_hole_copper);
		file.WriteString( line );
		CString str_min_hole_hole;
		::MakeCStringFromDimension( &str_min_hole_hole, m_doc->m_dr.hole_hole,
			m_units, TRUE, FALSE, TRUE, dp );
		s.LoadStringA(IDS_MinimumHoleToHoleClearance);
		line.Format(s, str_min_hole_hole);
		file.WriteString( line );
		CString str_min_annular_ring_pins;
		::MakeCStringFromDimension( &str_min_annular_ring_pins, m_doc->m_dr.annular_ring_pins,
			m_units, TRUE, FALSE, TRUE, dp );
		s.LoadStringA(IDS_MinimumAnnularRingPins);
		line.Format(s, str_min_annular_ring_pins);
		file.WriteString( line );
		CString str_min_annular_ring_vias;
		::MakeCStringFromDimension( &str_min_annular_ring_vias, m_doc->m_dr.annular_ring_vias,
			m_units, TRUE, FALSE, TRUE, dp ); 
		s.LoadStringA(IDS_MinimumAnnularRingVias);
		line.Format(s, str_min_annular_ring_vias);
		file.WriteString( line );
		CString str_min_board_edge_copper;
		::MakeCStringFromDimension( &str_min_board_edge_copper, m_doc->m_dr.board_edge_copper,
			m_units, TRUE, FALSE, TRUE, dp ); 
		s.LoadStringA(IDS_MinimumBoardEdgeToCopperClearance);
		line.Format(s, str_min_board_edge_copper);
		file.WriteString( line );
		CString str_min_board_edge_hole;
		::MakeCStringFromDimension( &str_min_board_edge_hole, m_doc->m_dr.board_edge_hole,
			m_units, TRUE, FALSE, TRUE, dp ); 
		s.LoadStringA(IDS_MinimumBoardEdgeToHoleClearance);
		line.Format(s, str_min_board_edge_hole);
		file.WriteString( line );
		CString str_min_copper_copper;
		::MakeCStringFromDimension( &str_min_copper_copper, m_doc->m_dr.copper_copper,
			m_units, TRUE, FALSE, TRUE, dp ); 
		s.LoadStringA(IDS_MinimumCopperAreaToCopperAreaClearance);
		line.Format(s, str_min_copper_copper);
		file.WriteString( line );
	}

	if( m_flags & (DRC_LIST | CONNECTIVITY_LIST) )
	{
		BOOL bDrc = m_flags & DRC_LIST;
		BOOL bCon = m_flags & CONNECTIVITY_LIST;
		BOOL bConHeading = FALSE;
		DesignRules dr = m_doc->m_dr;
		dr.bCheckUnrouted = bCon;
		m_doc->m_drelist->Clear();
		m_doc->DRC( m_units, TRUE, &dr );
		if( bDrc )
		{
			CString s ((LPCSTR) IDS_DRCErrors);
			file.WriteString( s );
		}
		citer<cdre> id (&m_doc->m_drelist->dres);
		for (cdre *dre = id.First(); dre; dre = id.Next())
		{
			CString str = dre->str, s ((LPCSTR) IDS_RoutedConnectionFrom);
			BOOL bConError = (str.Find( s ) != -1);
			if( bConError && bCon && !bConHeading )
			{
				s.LoadStringA(IDS_ConnectivityErrors);
				file.WriteString( s );
				bConHeading = TRUE;
			}
			if( bCon && bConError || bDrc && !bConError )
			{
				int colon = str.Find( ":" ); 
				if( colon > -1 )
					str = str.Right( str.GetLength() - colon - 2 );
				file.WriteString( str ); 
			}
		}
		m_doc->m_drelist->Clear();
	}
	file.Close();
	// CPT2 TODO provide some user feedback that it's been done
	OnOK();
}

void CDlgReport::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}
