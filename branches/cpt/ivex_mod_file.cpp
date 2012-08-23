// functions to convert Ivex .mod file to FreePCB .lib file
//
#include "stdafx.h"
#include "file_io.h"
#include "Shape.h"
#include "ivex_mod_file.h"
#include "math.h"

// define layer masks
enum {
	LAY_T = 0x1,
	LAY_I = 0x2,
	LAY_B = 0x4,
	LAY_C = 0x8,
	LAY_P = 0x10
};

// structures to hold padstack data from mod file
struct pad_stack_entry {
	CString shape;
	int pad_shape;
	int orientation;
	int x_wid, y_wid;
	int x_off, y_off;
	int layer_mask;
};
struct mod_padstack {
	CString name;
	int hole_diam;		// units = NM
	CArray<pad_stack_entry> entry;
};
typedef mod_padstack ps;

ps * mod_ps;


// this is the main conversion function
//
int ConvertIvex( CStdioFile * mod_file, CStdioFile * lib_file, CEdit * edit_ctrl )
{
	// state-machine
	enum { IDLE,	// idle (i.e. no state)
		PAD_STACK,	// parsing pad stack
		MODULE		// parsing module
	};

	CArray<mtg_hole> m_mtg_hole;
	extern CFreePcbApp theApp;
	CFreePcbDoc *doc = theApp.m_doc;
	cshape s (doc);
	m_mtg_hole.SetSize(0);
	s.m_author = "Ivex";

	int min_x;
	int max_x;
	int min_y;
	int max_y;

	#define pi  3.14159265359	
	double sin45 = sin( pi/4.0 );
	double cos45 = sin45;
	double sin22 = sin( pi/8.0 );
	double cos22 = cos( pi/8.0 );
	double sin67 = sin( 3.0*pi/8.0 );
	double cos67 = cos( 3.0*pi/8.0 );

	// state
	int state = IDLE;
	int units = MIL;
	int mult = NM_PER_MIL;
	CString instr, keystr, subkey;
	CArray<CString> p;
	mod_padstack * new_ps;
	int current_x, current_y;
	int current_angle;

	// map of pad identifiers
	CMapStringToPtr mod_padstack_map;

	// start reading mod file
	int ret = mod_file->ReadString( instr );
	instr.Trim();
	while( ret )
	{
		instr.MakeUpper();
		if( instr.Left(9).MakeUpper() == "// SOURCE" )
		{
			// get source of footprint
			CString source = instr.Right( instr.GetLength() - 9);
			source.Remove( '\"' );
			source.Trim();
			s.m_source = source;
		}
		int np = ParseKeyString( &instr, &keystr, &p ); 
		if( np )
		{
			// now handle incoming line
			if( state == IDLE && keystr == "UNITS" )
			{
				subkey = p[0];
				if( subkey == "MIL" )
				{
					units = MIL;
					mult = NM_PER_MIL;
				}
				else if( subkey == "MM" )
				{
					units = MM;
					mult = 1000000;
				}
				else if( subkey == "NM" )
				{
					units = NM;
					mult = 1;
				}
				else if( subkey == "MICRON" )
				{
					units = MM;
					mult = 1000;
				}
				else
					ASSERT(0);
			}
			else if( state == IDLE && keystr == "PAD" && p[0] == "STACK" )
			{
				// parse PAD STACK
				state = PAD_STACK;
				CString name = p[1];
				double hole_diam = my_atof(&p[2]);
				if( p[3] != "{" )
					ASSERT(0);
				// store it in map
				new_ps = new mod_padstack;
				new_ps->name = name;
				new_ps->hole_diam = hole_diam * mult;
				mod_padstack_map.SetAt( name, new_ps );
			}
			else if( state == PAD_STACK && ( keystr == "OVAL" || keystr == "RECT" ) )
			{
				// parse PAD STACK entry
				int i = new_ps->entry.GetSize();
				new_ps->entry.SetSize( i+1 );
				new_ps->entry[i].shape = keystr;
				new_ps->entry[i].pad_shape = PAD_NONE;
				if( keystr == "OVAL" )
					new_ps->entry[i].pad_shape = PAD_ROUND;
				else if( keystr == "RECT" )
					new_ps->entry[i].pad_shape = PAD_RECT;
				else
					ASSERT(0);
				new_ps->entry[i].orientation = my_atof(&p[0]);
				new_ps->entry[i].x_wid = my_atof(&p[1])*mult;
				new_ps->entry[i].y_wid = my_atof(&p[2])*mult;
				if( new_ps->entry[i].x_wid == new_ps->entry[i].y_wid && new_ps->entry[i].pad_shape == PAD_RECT )
					new_ps->entry[i].pad_shape = PAD_SQUARE;	
				new_ps->entry[i].x_off = my_atof(&p[3])*mult;
				new_ps->entry[i].y_off = my_atof(&p[4])*mult;
				CString layers = p[5];
				if( np >= 8 )
					layers += p[6];
				if( np >= 9 )
					layers += p[7];
				new_ps->entry[i].layer_mask = 0;
				if( layers.Find('T') != -1 )
					new_ps->entry[i].layer_mask |= LAY_T;
				if( layers.Find('I') != -1 )
					new_ps->entry[i].layer_mask |= LAY_I;
				if( layers.Find('B') != -1 )
					new_ps->entry[i].layer_mask |= LAY_B;
				if( layers.Find('C') != -1 )
					new_ps->entry[i].layer_mask |= LAY_C;
				if( layers.Find('P') != -1 )
					new_ps->entry[i].layer_mask |= LAY_P;
			}
			else if( state == PAD_STACK && keystr == "}" )
			{
				state = IDLE;
			}
			else if( state == IDLE && keystr == "BEGIN" )
			{
				// start new module (i.e. footprint) definition
				state = MODULE;
				s.m_units = units;
				m_mtg_hole.SetSize(0);
				// default ref text params
				s.m_ref->m_font_size = 50 * NM_PER_MIL;
				s.m_ref->m_x = 50 * NM_PER_MIL;
				s.m_ref->m_y = 50 * NM_PER_MIL;
				s.m_ref->m_angle = 0;
				s.m_ref->m_stroke_width = 7 * NM_PER_MIL;
				// track min and max dimensions to create selection rectangle
				min_x = INT_MAX;
				max_x = INT_MIN;
				min_y = INT_MAX;
				max_y = INT_MIN;
			}
			else if( state == MODULE && keystr == "END" )
			{
				// end module def.
				state = IDLE;
				// check for all pins defined
				BOOL error = FALSE;
				citer<cpadstack> ips (&s.m_padstacks);
				for (cpadstack *ps = ips.First(); ps; ps = ips.Next())
					if( !ps->exists )
						{ error = TRUE; break; }
				if( !error )
				{
					// convert mounting holes to pads
					for( int ih=0; ih<m_mtg_hole.GetSize(); ih++ )
					{
						cpadstack *ps = new cpadstack(&s);
						ps->exists = 1;
						ps->name.Format( "MH%d", ih+1 );
						ps->hole_size = m_mtg_hole[ih].diam;
						ps->x_rel = m_mtg_hole[ih].x;
						ps->y_rel = m_mtg_hole[ih].y;
						ps->angle = 0;
						ps->top.shape = m_mtg_hole[ih].pad_shape;
						ps->top.size_h = m_mtg_hole[ih].pad_diam;
						ps->top.size_l = 0;
						ps->top.size_r = 0;
						int inner_pad_diam;
						if( m_mtg_hole[ih].pad_shape == PAD_NONE )
							ps->inner.shape = PAD_NONE;
						else
						{
							ps->inner.shape = PAD_ROUND;
							inner_pad_diam = m_mtg_hole[ih].diam + 20*NM_PER_MIL;
							if( inner_pad_diam < m_mtg_hole[ih].pad_diam )
								ps->inner.size_h = inner_pad_diam;
							else
								ps->inner.size_h = m_mtg_hole[ih].pad_diam;
							ps->inner.size_l = 0;
							ps->inner.size_r = 0;
						}
						ps->bottom.shape = m_mtg_hole[ih].pad_shape;
						ps->bottom.size_h = m_mtg_hole[ih].pad_diam;
						ps->bottom.size_l = 0;
						ps->bottom.size_r = 0;
						s.m_padstacks.Add(ps);
					}
					m_mtg_hole.RemoveAll();
					// make sure selection rectangle surrounds pads
					for (cpadstack *ps = ips.First(); ps; ps = ips.Next())
					{
						int x = ps->x_rel, y = ps->y_rel;
						int dx = ps->top.size_h/2, dy = dx;
						if( (x+dx) > max_x )
							max_x = x + dx;
						if( (x-dx) < min_x )
							min_x = x - dx;
						if( (y+dy) > max_y )
							max_y = y + dy;
						if( (y-dy) < min_y )
							min_y = y - dy;
					}
					s.m_sel_xi = min_x - 8*NM_PER_MIL;
					s.m_sel_yi = min_y - 8*NM_PER_MIL;
					s.m_sel_xf = max_x + 8*NM_PER_MIL;
					s.m_sel_yf = max_y + 8*NM_PER_MIL;
					s.WriteFootprint( lib_file );
				}
				else
				{
					CString str;
					str.Format( "    missing pins, aborting conversion\r\n" );
					edit_ctrl->ReplaceSel( str );
				}
			}
			else if( state == MODULE && keystr == "NAME" )
			{
				CString name = p[0];
				s.m_name = name;
				CString str;
				str.Format( "  converting part \"%s\"\r\n", name );
				edit_ctrl->ReplaceSel( str );
			}
			else if( state == MODULE && keystr == "OUTLINE" )
			{
				if( p[0] == "BOX" )
				{
					int xi = my_atof(&p[1]) * mult;
					int yi = -(my_atof(&p[2]) * mult);
					int xf = my_atof(&p[3]) * mult;
					int yf = -(my_atof(&p[4]) * mult);
					int np = s.m_outlines.GetSize();
					coutline *out = new coutline(&s, LAY_FP_SILK_TOP, 7*NM_PER_MIL);
					s.m_outlines.Add(out);
					ccontour *ctr = new ccontour(out, true);
					ctr->AppendCorner( xi, yi );
					ctr->AppendCorner( xf, yi );
					ctr->AppendCorner( xf, yf );
					ctr->AppendCorner( xi, yf );
					ctr->Close();
					if( xi > max_x )
						max_x = xi;
					if( xi < min_x )
						min_x = xi;
					if( yi > max_y )
						max_y = yi;
					if( yi < min_y )
						min_y = yi;
					if( xf > max_x )
						max_x = xf;
					if( xf < min_x )
						min_x = xf;
					if( yf > max_y )
						max_y = yf;
					if( yf < min_y )
						min_y = yf;
				}
				else if( p[0] == "LINE" )
				{
					int xi = my_atof(&p[1]) * mult;
					int yi = -(my_atof(&p[2]) * mult);
					int xf = my_atof(&p[3]) * mult;
					int yf = -(my_atof(&p[4]) * mult);
					int np = s.m_outlines.GetSize();
					coutline *out = new coutline(&s, LAY_FP_SILK_TOP, 7*NM_PER_MIL);
					s.m_outlines.Add(out);
					ccontour *ctr = new ccontour(out, true);
					ctr->AppendCorner( xi, yi );
					ctr->AppendCorner( xf, yf );
					if( xi > max_x )
						max_x = xi;
					if( xi < min_x )
						min_x = xi;
					if( yi > max_y )
						max_y = yi;
					if( yi < min_y )
						min_y = yi;
					if( xf > max_x )
						max_x = xf;
					if( xf < min_x )
						min_x = xf;
					if( yf > max_y )
						max_y = yf;
					if( yf < min_y )
						min_y = yf;
				}
				else if( p[0] == "ARC" )
				{
					// make an arc with a polyline
					int xc = my_atof(&p[1]) * mult;
					int yc = -(my_atof(&p[2]) * mult);
					int r = my_atof(&p[3]) * mult;
					int quadrant = my_atoi(&p[4]);
					coutline *out = new coutline(&s, LAY_FP_SILK_TOP, 7*NM_PER_MIL);
					s.m_outlines.Add(out);
					ccontour *ctr = new ccontour(out, true);
					if( quadrant == 1 )
					{
						ctr->AppendCorner( xc+r, yc );
						ctr->AppendCorner( xc, yc+r, cpolyline::ARC_CCW );
					}
					else if( quadrant == 2 )
					{
						ctr->AppendCorner( xc, yc+r );
						ctr->AppendCorner( xc-r, yc, cpolyline::ARC_CCW );
					}
					else if( quadrant == 3 )
					{
						ctr->AppendCorner( xc-r, yc );
						ctr->AppendCorner( xc, yc-r, cpolyline::ARC_CCW );
					}
					else
					{
						ctr->AppendCorner( xc, yc-r );
						ctr->AppendCorner( xc+r, yc, cpolyline::ARC_CCW );
					}
					if( xc+r > max_x )
						max_x = xc+r;
					if( xc-r < min_x )
						min_x = xc-r;
					if( yc+r > max_y )
						max_y = yc+r;
					if( yc-r < min_y )
						min_y = yc-r;
				}
				else if( p[0] == "CIRCLE" )
				{
					// make a circle with a polyline
					int xc = my_atof(&p[1]) * mult;
					int yc = -(my_atof(&p[2]) * mult);
					int r = my_atof(&p[3]) * mult;
					coutline *out = new coutline(&s, LAY_FP_SILK_TOP, 7*NM_PER_MIL);
					s.m_outlines.Add(out);
					ccontour *ctr = new ccontour(out, true);
					ctr->AppendCorner( xc+r, yc );
					ctr->AppendCorner( xc, yc+r, cpolyline::ARC_CCW );
					ctr->AppendCorner( xc-r, yc, cpolyline::ARC_CCW );
					ctr->AppendCorner( xc, yc-r, cpolyline::ARC_CCW );
					ctr->Close( cpolyline::ARC_CCW );
					if( xc+r > max_x )
						max_x = xc+r;
					if( xc-r < min_x )
						min_x = xc-r;
					if( yc+r > max_y )
						max_y = yc+r;
					if( yc-r < min_y )
						min_y = yc-r;
				}
			}
			else if( state == MODULE && keystr == "MOUNTING" )
			{
				// mounting hole
				int ih = m_mtg_hole.GetSize();
				m_mtg_hole.SetSize( ih+1 );
				if( p[0] == "EMPTY" )
					m_mtg_hole[ih].pad_shape = PAD_NONE;
				else if( p[0] == "SQUARE" )
					m_mtg_hole[ih].pad_shape = PAD_SQUARE;
				else if( p[0] == "ROUND" )
					m_mtg_hole[ih].pad_shape = PAD_ROUND;
				else
					ASSERT(0);
				int x = my_atof( &p[1])*mult; 
				int y = my_atof( &p[2])*mult; 
				int diam = my_atof( &p[3])*mult; 
				int pad_diam = my_atof( &p[4])*mult;	// only used for ROUND or SQUARE
				m_mtg_hole[ih].x = x;
				m_mtg_hole[ih].y = y;
				m_mtg_hole[ih].diam = diam;
				m_mtg_hole[ih].pad_diam = pad_diam;
				int d = diam;
				if( pad_diam > diam )
					d = pad_diam;
				int r = d/2;
				if( x+r > max_x )
					max_x = x+r;
				if( x-r < min_x )
					min_x = x-r;
				if( y+r > max_y )
					max_y = y+r;
				if( y-r < min_y )
					min_y = y-r;
			}
			else if( state == MODULE && keystr == "TEXT" )
			{
				if( p[7] == "REF" )
				{
					int x = my_atof(&p[0]) * mult;
					int y = -(my_atof(&p[1]) * mult);
					int angle = my_atof(&p[2]);
					s.m_ref->m_font_size = 50 * NM_PER_MIL;
					s.m_ref->m_x = x;
					s.m_ref->m_y = y;
					s.m_ref->m_angle = angle;
					s.m_ref->m_stroke_width = 7 * NM_PER_MIL;
				}
			}
			else if( state == MODULE && keystr == "PAD" 
				&& ( p[0] == "DEF" || p[0] == "STEP" ) )
			{
				// add pad to shape
				CString pin_str;
				if( p[0] == "DEF" )
				{
					current_x = my_atof(&p[1]) * mult;
					current_y = my_atof(&p[2]) * mult;
					current_angle = my_atof(&p[3]);
					CString pad_name = p[4];
					void * ptr;
					BOOL found = mod_padstack_map.Lookup( pad_name, ptr );
					if( !found )
						ASSERT(0);
					mod_ps = (mod_padstack *)ptr;
					pin_str = p[5];
				}
				else if( p[0] == "STEP" )
				{
					CString step_str = p[1];
					CString step = step_str.Right( step_str.GetLength()-2 );
					int is = my_atof( &step ) * mult;
					if( step_str.Left(2) == "X+" )
						current_x = current_x + is;
					if( step_str.Left(2) == "X-" )
						current_x = current_x - is;
					if( step_str.Left(2) == "Y+" )
						current_y = current_y + is;
					if( step_str.Left(2) == "Y-" )
						current_y = current_y - is;
					pin_str = p[2];
				}
				if( pin_str != "" )
				{
					cpadstack *ps = new cpadstack(&s);
					ps->exists = 1;
					ps->name = pin_str;
					ps->hole_size = mod_ps->hole_diam;
					ps->x_rel = current_x;
					ps->y_rel = -(current_y);	// since Ivex reverses y-axis
					ps->angle = current_angle;
					ps->top.shape = PAD_NONE;
					ps->inner.shape = PAD_NONE;
					ps->bottom.shape = PAD_NONE;
					if( mod_ps->entry[0].layer_mask & (LAY_T | LAY_C) )
					{
						ps->top.shape = mod_ps->entry[0].pad_shape;
						ps->top.size_l = mod_ps->entry[0].x_wid/2;
						ps->top.size_r = mod_ps->entry[0].x_wid/2;
						ps->top.size_h = mod_ps->entry[0].y_wid;
					}
					if( mod_ps->entry[0].layer_mask & LAY_I || mod_ps->hole_diam > 0 )
					{
						// inner pads are forced round
						ps->inner.shape = PAD_ROUND;
						ps->inner.size_l = mod_ps->entry[0].x_wid/2;
						ps->inner.size_r = mod_ps->entry[0].x_wid/2;
						ps->inner.size_h = mod_ps->entry[0].y_wid;
					}
					if( mod_ps->entry[0].layer_mask & LAY_B )
					{
						ps->bottom.shape = mod_ps->entry[0].pad_shape;
						ps->bottom.size_l = mod_ps->entry[0].x_wid/2;
						ps->bottom.size_r = mod_ps->entry[0].x_wid/2;
						ps->bottom.size_h = mod_ps->entry[0].y_wid;
					}
				}
				else
				{
					// illegal pin number
					CString str;
					str.Format( "    illegal pin \"%s\", abort conversion\r\n", pin_str );
					edit_ctrl->ReplaceSel( str );
					state = IDLE;
				}
			}
		}
		ret = mod_file->ReadString( instr );
	}
	// now delete map of padstacks
	POSITION pos;
	CString key;
	void * ptr;
	for( pos = mod_padstack_map.GetStartPosition(); pos != NULL; )
	{
		mod_padstack_map.GetNextAssoc( pos, key, ptr );
		delete ptr;
	}
	return 0;
}


