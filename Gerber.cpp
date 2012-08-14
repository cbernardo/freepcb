// make Gerber file from partlist, netlist, textlist, etc.
//
#include "stdafx.h"
#include "Gerber.h"
#include "PcbItem.h"
#include "PartListNew.h"
#include "NetListNew.h"
#include "TextListNew.h"

#define pi  3.14159265359

#define CLEARANCE_POLY_STROKE_MILS 1

// CPT2 obsolete:
/*
class c_cutout {
public:
	cnet * net;
	int ia;
//	int ic;
};


class c_area {
public:
	carea2 *a;

	cnet * net;
	int ia;
	CArray< c_cutout > containers;
};
*/

// constructor
CAperture::CAperture()
{
	m_type = 0;
	m_size1 = 0;
	m_size2 = 0;
}

// constructor
CAperture::CAperture( int type, int size1, int size2, int size3 ) 
{
	m_type = type;
	m_size1 = size1;
	m_size2 = size2;
	m_size3 = size3;
}

// destructor
CAperture::~CAperture()
{
}

// test for equality
//
BOOL CAperture::Equals( CAperture * ap )
{
	if( m_type == ap->m_type )
	{
		if( m_type == AP_CIRCLE && m_size1 == ap->m_size1 )
			return TRUE;
		if( m_type == AP_SQUARE && m_size1 == ap->m_size1 )
			return TRUE;
		if( m_type == AP_OCTAGON && m_size1 == ap->m_size1 )
			return TRUE;
		if( m_type == AP_RECT && m_size1 == ap->m_size1 && m_size2 == ap->m_size2 )
			return TRUE;
		if( m_type == AP_RRECT && m_size1 == ap->m_size1 && m_size2 == ap->m_size2 
				&& m_size3 == ap->m_size3 )
			return TRUE;
		if( m_type == AP_RECT_THERMAL && m_size1 == ap->m_size1 && m_size2 == ap->m_size2 
				&& m_size3 == ap->m_size3 )
			return TRUE;
		if( m_type == AP_THERMAL && m_size1 == ap->m_size1 && m_size2 == ap->m_size2 )
			return TRUE;
		if( m_type == AP_MOIRE && m_size1 == ap->m_size1 && m_size2 == ap->m_size2 )
			return TRUE;
		if( m_type == AP_OVAL && m_size1 == ap->m_size1 && m_size2 == ap->m_size2 )
			return TRUE;
	}
	return FALSE;
}

// find aperture in array 
// if not found and ok_to_add == TRUE, add to array
// return posiiton in array, or -1 if not found and not added
//
int CAperture::FindInArray( aperture_array * ap_array, BOOL ok_to_add )
{
	int na = ap_array->GetSize();
	int ifound = -1;
	for( int ia=0; ia<na; ia++ )
	{
		if( this->Equals( &(ap_array->GetAt(ia)) ) )
		{
			ifound = ia;
			break;
		}
	}
	if( ifound != -1 )
		return ifound;
	if( ok_to_add )
	{
		ap_array->Add( *this );
		return ap_array->GetSize()-1;
	}
	ASSERT(0);
	return -1;
}

// things that you can do with a light
enum {
	LIGHT_NONE,
	LIGHT_ON,
	LIGHT_OFF,
	LIGHT_FLASH
};

// generate a "moveto" string for the Gerber file
// enter with:
//	x, y = coords in NM
//
void WriteMoveTo( CStdioFile * f, int x, int y, int light_state )
{
	_int64 x_10 = (_int64)10*x/NM_PER_MIL;	// 2.4
	_int64 y_10 = (_int64)10*y/NM_PER_MIL;	// 2.4
	CString str;
	if( light_state == LIGHT_NONE )
		ASSERT(0);
	else
		str.Format( "G01X%I64dY%I64dD0%d*\n", x_10, y_10, light_state );
	f->WriteString( str );
}

// draw one side of a CPolyLine by writing commands to Gerber file
// the side may be straight or an arc
// arc is aproximated by straight-line segments
// assumes that plotter already at x1, y1
// does not turn the light on or off
// dimensions are in NM
//
void WritePolygonSide( CStdioFile * f, int x1, int y1, int x2, int y2, int style, int nsteps, int light_state )
{
	int n;	// number of steps for arcs
	n = (abs(x2-x1)+abs(y2-y1))/(5*NM_PER_MIL);	// step size approx. 3 to 5 mil
	n = max( n, 18 );	// or at most 5 degrees of arc
	CString line;
	double xo, yo, a, b, theta1, theta2;
	a = fabs( (double)(x1 - x2) );
	b = fabs( (double)(y1 - y2) );

	if( style == CPolyLine::STRAIGHT )
	{
		// just draw a straight line with linear interpolation
		WriteMoveTo( f, x2, y2, light_state );
		return;
	}
	else if( style == CPolyLine::ARC_CW )
	{
		// clockwise arc (ie.quadrant of ellipse)
		int i=0, j=0;
		if( x2 > x1 && y2 > y1 )
		{
			// first quadrant, draw second quadrant of ellipse
			xo = x2;	
			yo = y1;
			theta1 = pi;
			theta2 = pi/2.0;
		}
		else if( x2 < x1 && y2 > y1 )
		{
			// second quadrant, draw third quadrant of ellipse
			xo = x1;	
			yo = y2;
			theta1 = 3.0*pi/2.0;
			theta2 = pi;
		}
		else if( x2 < x1 && y2 < y1 )	
		{
			// third quadrant, draw fourth quadrant of ellipse
			xo = x2;	
			yo = y1;
			theta1 = 2.0*pi;
			theta2 = 3.0*pi/2.0;
		}
		else
		{
			xo = x1;	// fourth quadrant, draw first quadrant of ellipse
			yo = y2;
			theta1 = pi/2.0;
			theta2 = 0.0;
		}
	}
	else if( style == CPolyLine::ARC_CCW )
	{
		// counter-clockwise arc
		int i=0, j=0;
		if( x2 > x1 && y2 > y1 )
		{
			xo = x1;	// first quadrant, draw fourth quadrant of ellipse
			yo = y2;
			theta1 = 3.0*pi/2.0;
			theta2 = 2.0*pi;
		}
		else if( x2 < x1 && y2 > y1 )
		{
			xo = x2;	// second quadrant
			yo = y1;
			theta1 = 0.0;
			theta2 = pi/2.0;
		}
		else if( x2 < x1 && y2 < y1 )	
		{
			xo = x1;	// third quadrant
			yo = y2;
			theta1 = pi/2.0;
			theta2 = pi;
		}
		else
		{
			xo = x2;	// fourth quadrant
			yo = y1;
			theta1 = pi;
			theta2 = 3.0*pi/2.0;
		}
	}
	else
		ASSERT(0);
	// now write steps
	for( int is=1; is<=n; is++ )
	{
		double theta = theta1 + ((theta2-theta1)*(double)is)/n;
		double x = xo + a*cos(theta);
		double y = yo + b*sin(theta);
		if( is == n )
		{
			x = x2;
			y = y2;
		}
		WriteMoveTo( f, x, y, light_state );
	}
}

// draw clearance for a pad or trace segment in intersecting areas in foreign nets
// enter with shape = pad shape or -1 for trace segment
//
void DrawClearanceInForeignAreas( cnet2 *net, int shape,
									int w, int xi, int yi, int xf, int yf,	// for line segment
									int wid, int len, int radius, int angle, // for pad
								    CStdioFile * f, int flags, int layer,
									int fill_clearance,
									carray<carea2> *area_list
									)
{
	double seg_angle, cw;

	if( shape == -1 )
	{
		// segment
		seg_angle = atan2( (double)yf - yi, (double)xf - xi );
		cw = (double)fill_clearance + (double)w/2.0
			- (double)CLEARANCE_POLY_STROKE_MILS*NM_PER_MIL/2;
	}
	BOOL bClearanceMade = FALSE;
	gpc_vertex_list gpc_contour;
	gpc_polygon gpc_seg_poly;
	// now loop through all areas in foreign nets
	citer<carea2> ia (area_list);
	for (carea2 *area = ia.First(); area; area = ia.Next())
	{
		if (area->m_net == net)
			continue;
		// foreign net, see if possible intersection
		BOOL bIntersection = FALSE;
		if( shape == PAD_NONE )
			ASSERT(0);
		else if( shape == -1 )
		{
			// trace segment
			if( area->TestPointInside( xi, yi ) )
				bIntersection = TRUE;
			if( !bIntersection )
				if( area->TestPointInside( xf, yf ) )
					bIntersection = TRUE;
			if( !bIntersection )
			{
				int min_d = fill_clearance;
				citer<ccontour> ictr (&area->contours);
				for (ccontour *ctr = ictr.First(); ctr && !bIntersection; ctr = ictr.Next())
				{
					citer<cside> is (&ctr->sides);
					for (cside *s = is.First(); s; s = is.Next())
					{
						int d = GetClearanceBetweenSegments( xi, yi, xf, yf, cpolyline::STRAIGHT, w,
							s->preCorner->x, s->preCorner->y, s->postCorner->x, s->postCorner->y, s->m_style, 0, 
							min_d, NULL, NULL );
						if( d < min_d )
						{
							bIntersection = TRUE;
							break;
						}
					}
				}
			}
		}
		else if( shape == PAD_ROUND || shape == PAD_OCTAGON  )
		{
			int min_d = wid/2 + fill_clearance;
			citer<ccontour> ictr (&area->contours);
			for (ccontour *ctr = ictr.First(); ctr && !bIntersection; ctr = ictr.Next())
			{
				citer<cside> is (&ctr->sides);
				for (cside *s = is.First(); s; s = is.Next())
				{
					int d = GetClearanceBetweenSegments( xi, yi, xi+10, yi+10, CPolyLine::STRAIGHT, w,
						s->preCorner->x, s->preCorner->y, s->postCorner->x, s->postCorner->y, s->m_style, 0, 
						min_d, NULL, NULL );
					if( d < min_d )
					{
						bIntersection = TRUE;
						break;
					}
				}
			}
		}
		else
		{
			int min_d;
			if( shape == PAD_SQUARE )
				min_d = sqrt((double)2.0*wid*wid)/2 + fill_clearance;
			else
				min_d = sqrt((double)wid*wid +(double)len*len)/2 + fill_clearance;
			citer<ccontour> ictr (&area->contours);
			for (ccontour *ctr = ictr.First(); ctr && !bIntersection; ctr = ictr.Next())
			{
				citer<cside> is (&ctr->sides);
				for (cside *s = is.First(); s; s = is.Next())
				{
					int d = GetClearanceBetweenSegments( xi, yi, xi+10, yi+10, CPolyLine::STRAIGHT, w,
						s->preCorner->x, s->preCorner->y, s->postCorner->x, s->postCorner->y, s->m_style, 0, 
						min_d, NULL, NULL );
					if( d < min_d )
					{
						bIntersection = TRUE;
						break;
					}
				}
			}
		}

		if( !bIntersection )
			continue;
		if( !bClearanceMade ) 
		{
			// construct a gpc_poly for the clearance
			if( shape == -1 )
			{
				int npoints = 18;	// number of points in poly
				gpc_contour.num_vertices = npoints;
				gpc_contour.vertex = (gpc_vertex*)calloc( 2 * npoints, sizeof(double) );
				if( !gpc_contour.vertex )
					ASSERT(0);
				double x,y;
				// create points around beginning of segment
				double angle = seg_angle + pi/2.0;		// rotate 90 degrees ccw
				double angle_step = pi/(npoints/2-1);
				for( int i=0; i<npoints/2; i++ )
				{
					x = xi + cw*cos(angle);
					y = yi + cw*sin(angle);
					gpc_contour.vertex[i].x = x;
					gpc_contour.vertex[i].y = y;
					angle += angle_step;
				}
				// create points around end of segment
				angle = seg_angle - pi/2.0;
				for( int i=npoints/2; i<npoints; i++ )
				{
					x = xf + cw*cos(angle);
					y = yf + cw*sin(angle);
					gpc_contour.vertex[i].x = x;
					gpc_contour.vertex[i].y = y;
					angle += angle_step;
				}
				gpc_seg_poly.num_contours = 1;
				gpc_seg_poly.hole = new int;
				gpc_seg_poly.hole[0] = 0;
				gpc_seg_poly.contour = &gpc_contour;
				bClearanceMade = TRUE;
			}
			else if( shape == PAD_ROUND || shape == PAD_OCTAGON )
			{
				// round clearance
				int npoints = 32;	// number of points in poly
				gpc_contour.num_vertices = npoints;
				gpc_contour.vertex = (gpc_vertex*)calloc( 2 * npoints, sizeof(double) );
				if( !gpc_contour.vertex )
					ASSERT(0);
				double x,y;
				// create points around pad
				double angle = 0.0;
				double angle_step = pi/(npoints/2);
				cw = wid/2 + fill_clearance;
				for( int i=0; i<npoints; i++ )
				{
					x = xi + cw*cos(angle);
					y = yi + cw*sin(angle);
					gpc_contour.vertex[i].x = x;
					gpc_contour.vertex[i].y = y;
					angle += angle_step;
				}
				gpc_seg_poly.num_contours = 1;
				gpc_seg_poly.hole = new int;
				gpc_seg_poly.hole[0] = 0;
				gpc_seg_poly.contour = &gpc_contour;
				bClearanceMade = TRUE;
			}
			else
			{
				int npoints = 4;	// number of points in poly 
				gpc_contour.num_vertices = npoints;
				gpc_contour.vertex = (gpc_vertex*)calloc( 2 * npoints, sizeof(double) );
				if( !gpc_contour.vertex ) 
					ASSERT(0);
				int dx = len/2 + fill_clearance;
				int dy = wid/2 + fill_clearance;
				if( shape == PAD_SQUARE )
					dy = dx;
				if( angle % 180 )
				{
					int t = dx;
					dx = dy;
					dy = t;
				}
				// create points around pad
				gpc_contour.vertex[0].x = xi + dx;
				gpc_contour.vertex[0].y = yi + dy;
				gpc_contour.vertex[1].x = xi + dx;
				gpc_contour.vertex[1].y = yi - dy;
				gpc_contour.vertex[2].x = xi - dx;
				gpc_contour.vertex[2].y = yi - dy;
				gpc_contour.vertex[3].x = xi - dx;
				gpc_contour.vertex[3].y = yi + dy;
				gpc_seg_poly.num_contours = 1;
				gpc_seg_poly.hole = new int;
				gpc_seg_poly.hole[0] = 0;
				gpc_seg_poly.contour = &gpc_contour;
				bClearanceMade = TRUE;
			}
		}

		// intersect area and clearance polys
		gpc_polygon gpc_intersection;
		gpc_intersection.num_contours = 0;
		gpc_intersection.hole = NULL;
		gpc_intersection.contour = NULL;
		// make area GpcPoly if not already made, including cutouts
		if( area->GetGpcPoly()->num_contours == 0 )
			area->MakeGpcPoly();
		gpc_polygon_clip( GPC_INT, &gpc_seg_poly, area->GetGpcPoly(),
			&gpc_intersection );
		int ncontours = gpc_intersection.num_contours;
		for( int ic=0; ic<ncontours; ic++ )
		{
			// draw clearance
			gpc_vertex * gpv = gpc_intersection.contour[ic].vertex;
			int nv = gpc_intersection.contour[ic].num_vertices;
			if( nv )
			{
				f->WriteString( "G36*\n" );
				WriteMoveTo( f, gpv[0].x, gpv[0].y, LIGHT_OFF );
				for( int iv=1; iv<nv; iv++ )
					WriteMoveTo( f, gpv[iv].x, gpv[iv].y, LIGHT_ON );
				WriteMoveTo( f, gpv[0].x, gpv[0].y, LIGHT_ON );
				f->WriteString( "G37*\n" );
				// now stroke outline to remove any truncation artifacts
				// assumes aperture is already set to 
				// CLEARANCE_POLY_STROKE_MILS*NM_PER_MIL
				WriteMoveTo( f, gpv[0].x, gpv[0].y, LIGHT_OFF );
				for( int iv=1; iv<nv; iv++ )
					WriteMoveTo( f, gpv[iv].x, gpv[iv].y, LIGHT_ON );
				WriteMoveTo( f, gpv[0].x, gpv[0].y, LIGHT_ON );
			}
		}
		for( int ic=0; ic<ncontours; ic++ )
			free( gpc_intersection.contour[ic].vertex );
		// free intersection
		free( gpc_intersection.hole );
		free( gpc_intersection.contour );
	}

	if( bClearanceMade )
	{
		free( gpc_contour.vertex );
		delete gpc_seg_poly.hole;
	}
}

// Change aperture if necessary
// If PASS0, just make sure that the aperture is in ap_array
// Otherwise, write aperture change to file if necessary
//
void ChangeAperture( CAperture * new_ap,			// new aperture
					CAperture * current_ap,			// current aperture
					aperture_array * ap_array,		// array of apertures
					BOOL pass0,						// flag for PASS0
					CStdioFile * f )				// file to write to
{
	int current_iap;
	CString line;

	if( !(*current_ap).Equals( new_ap ) )
	{
		// change aperture
		current_iap = new_ap->FindInArray( ap_array, pass0 );
		if( !pass0 )
		{
			*current_ap = *new_ap;
			line.Format( "G54D%2d*\n", current_iap+10 );
			f->WriteString( line );	 // select new aperture
		}
	}
}

// write the Gerber file for a layer
// assumes that the file is already open for writing
// returns 0 if successful
//
int WriteGerberFile( CStdioFile * f, int flags, int layer,
					CDlgLog * log, int paste_mask_shrink,
					int fill_clearance, int mask_clearance, int pilot_diameter,
					int min_silkscreen_stroke_wid, int thermal_wid,
					int outline_width, int hole_clearance,
					int n_x, int n_y, int step_x, int step_y,
					carray<cboard> * bd, carray<csmcutout> * sm, cpartlist * pl, 
					cnetlist * nl, ctextlist * tl, CDisplayList * dl )
{
#define LAYER_TEXT_HEIGHT			100*NM_PER_MIL	// for layer ID sring
#define	LAYER_TEXT_STROKE_WIDTH		10*NM_PER_MIL
#define PASS0 (ipass==0)	
#define PASS1 (ipass==1)
	BOOL bUsePinThermals = !(flags & GERBER_NO_PIN_THERMALS);

	aperture_array ap_array;
	int current_iap = -1;
	CAperture current_ap;
	int bd_min_x = INT_MAX;
	int bd_min_y = INT_MAX;
	int bd_max_x = INT_MIN;
	int bd_max_y = INT_MIN;
	const double cos_oct = cos( pi/8.0 ); 
	CString str;

	// get boundaries of board outline (in nm)
	citer<cboard> ib (bd);
	for( cboard *b = ib.First(); b; b = ib.Next() ) 
	{
		citer<ccorner> ic (&b->main->corners);
		for (ccorner *c = ic.First(); c; c = ic.Next())
		{
			if( c->x < bd_min_x )
				bd_min_x = c->x;
			if( c->x > bd_max_x )
				bd_max_x = c->x;
			if( c->y < bd_min_y )
				bd_min_y = c->y;
			if( c->y > bd_max_y )
				bd_max_y = c->y;
		}
	}

	// perform two passes through data, first just get apertures, then write file
	for( int ipass=0; ipass<2; ipass++ )
	{
		CString line;
		if( PASS1 )
		{
			// ******************** apertures created, now write them **********************
			CString thermal_angle_str = "45.0";
			if( flags & GERBER_90_THERMALS )
				thermal_angle_str = "0.0";
			CString layer_name_str;
			if( layer == LAY_MASK_TOP )
				layer_name_str = "top solder mask";
			else if( layer == LAY_PASTE_TOP )
				layer_name_str = "top paste mask";
			else if( layer == LAY_MASK_BOTTOM )
				layer_name_str = "bottom solder mask";
			else if( layer == LAY_PASTE_BOTTOM )
				layer_name_str = "bottom paste mask";
			else
				layer_name_str.Format( "%s", &layer_str[layer][0] );
			line.Format( "G04 %s layer *\n", layer_name_str );
			f->WriteString( line );
			f->WriteString( "G04 Scale: 100 percent, Rotated: No, Reflected: No *\n" );
			f->WriteString( "%FSLAX24Y24*%\n" );	// 2.4
			f->WriteString( "%MOIN*%\n" );
			f->WriteString( "%LN " + layer_name_str + " *%\n" );
			// macros
			f->WriteString( "G04 Rounded Rectangle Macro, params: W/2, H/2, R *\n" );
			f->WriteString( "%AMRNDREC*\n" );
			f->WriteString( "21,1,$1+$1,$2+$2-$3-$3,0,0,0*\n" );
			f->WriteString( "21,1,$1+$1-$3-$3,$2+$2,0,0,0*\n" );
			f->WriteString( "1,1,$3+$3,$1-$3,$2-$3*\n" );
			f->WriteString( "1,1,$3+$3,$3-$1,$2-$3*\n" );
			f->WriteString( "1,1,$3+$3,$1-$3,$3-$2*\n" );
			f->WriteString( "1,1,$3+$3,$3-$1,$3-$2*%\n" );
			f->WriteString( "G04 Rectangular Thermal Macro, params: W/2, H/2, T/2 *\n" );
			f->WriteString( "%AMRECTHERM*\n" );  
			f->WriteString( "$4=$3/2*\n" );		// T/4 
			f->WriteString( "21,1,$1-$3,$2-$3,0-$1/2-$4,0-$2/2-$4,0*\n" ); 
			f->WriteString( "21,1,$1-$3,$2-$3,0-$1/2-$4,$2/2+$4,0*\n" );
			f->WriteString( "21,1,$1-$3,$2-$3,$1/2+$4,0-$2/2-$4,0*\n" );
			f->WriteString( "21,1,$1-$3,$2-$3,$1/2+$4,$2/2+$4,0*%\n" );
			// define all of the apertures
			for( int ia=0; ia<ap_array.GetSize(); ia++ )
			{
				if( ap_array[ia].m_type == CAperture::AP_CIRCLE )
				{
					line.Format( "ADD%dC,%.6f*", ia+10, (double)ap_array[ia].m_size1/25400000.0 );
					f->WriteString( "%" + line + "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_SQUARE )
				{
					line.Format( "ADD%dR,%.6fX%.6f*", ia+10, 
						(double)ap_array[ia].m_size1/25400000.0, (double)ap_array[ia].m_size1/25400000.0 );
					f->WriteString( "%" + line + "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_THERMAL )
				{
					line.Format( "AMTHERM%d*7,0,0,%.6f,%.6f,%.6f,%s*", ia+10,
						(double)ap_array[ia].m_size1/25400000.0,	// outer diam
						(double)ap_array[ia].m_size2/25400000.0,	// inner diam
						(double)thermal_wid/25400000.0,				// cross-hair width
						thermal_angle_str );						// thermal spoke angle
					f->WriteString( "%" + line + "%\n" );
					line.Format( "ADD%dTHERM%d*", ia+10, ia+10 );
					f->WriteString( "%" + line + "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_RECT_THERMAL )
				{
					line.Format( "ADD%dRECTHERM,%.6fX%.6fX%.6f*", ia+10, 
						(double)ap_array[ia].m_size1/(2*25400000.0), (double)ap_array[ia].m_size2/(2*25400000.0),
						(double)ap_array[ia].m_size3/(2*25400000.0) );
					f->WriteString( "%" + line + "%\n" ); 
				}
				else if( ap_array[ia].m_type == CAperture::AP_MOIRE )
				{
					line.Format( "AMMOIRE%d*6,0,0,%.6f,0.005,0.050,3,0.005,%.6f,0.0*", ia+10,
						(double)ap_array[ia].m_size2/25400000.0,
						(double)ap_array[ia].m_size1/25400000.0 );
					f->WriteString( "%" + line + "%\n" );
					line.Format( "ADD%dMOIRE%d*", ia+10, ia+10 );
					f->WriteString( "%" + line + "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_OCTAGON )
				{
					line.Format( "ADD%dP,%.6fX8X22.5*", ia+10, 
						((double)ap_array[ia].m_size1/25400000.0 )/cos_oct );
					f->WriteString( "%" + line + "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_OVAL ) 
				{
					line.Format( "ADD%dO,%.6fX%.6f*", ia+10, 
						(double)ap_array[ia].m_size2/25400000.0, (double)ap_array[ia].m_size1/25400000.0 );
					f->WriteString( "%" + line + "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_RECT ) 
				{
					line.Format( "ADD%dR,%.6fX%.6f*", ia+10, 
						(double)ap_array[ia].m_size2/25400000.0, (double)ap_array[ia].m_size1/25400000.0 );
					f->WriteString( "%" + line + "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_RRECT ) 
				{
					line.Format( "ADD%dRNDREC,%.6fX%.6fX%.6f*", ia+10, 
						(double)ap_array[ia].m_size2/(2*25400000.0), (double)ap_array[ia].m_size1/(2*25400000.0),
						(double)ap_array[ia].m_size3/25400000.0 );
					f->WriteString( "%" + line + "%\n" ); 
				}
				else
					ASSERT(0);
			}
			f->WriteString( "G90*\n" );			// absolute format
			f->WriteString( "G70D02*\n" );	// use inches
		}

		// draw moires
		double f_step_x = (bd_max_x - bd_min_x + (double)step_x)/NM_PER_MIL;	// mils
		double f_step_y = (bd_max_y - bd_min_y + (double)step_y)/NM_PER_MIL;	// mils
		if( bd && (flags & GERBER_AUTO_MOIRES) )
		{
			if( PASS1 )
			{
				f->WriteString( "\nG04 ----------------------- Draw moires (positive)*\n" );
				f->WriteString( "%LPD*%\n" );
				current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
			}
			CAperture moire_ap( CAperture::AP_MOIRE, 400*NM_PER_MIL, 350*NM_PER_MIL );
			ChangeAperture( &moire_ap, &current_ap, &ap_array, PASS0, f );
			if( PASS1 )
			{
				// flash 3 moires
				int x = bd_min_x - 500*NM_PER_MIL;
				int y = bd_min_y;
				::WriteMoveTo( f, x, y, LIGHT_FLASH );	// lower left
				x = bd_max_x + ((n_x-1)*f_step_x + 500)*NM_PER_MIL;
				::WriteMoveTo( f, x, y, LIGHT_FLASH );	// lower right
				x = bd_min_x - 500*NM_PER_MIL;
				y = bd_max_y + ((n_y-1)*f_step_y*NM_PER_MIL);
				::WriteMoveTo( f, x, y, LIGHT_FLASH );	// upper left
			}
		}

		//draw layer identification string if requested
		if( tl && (flags & GERBER_LAYER_TEXT) )
		{
			if( PASS1 )
				f->WriteString( "\nG04 Draw Layer Name*\n" );
			CString str = "";
			switch( layer )
			{
			case LAY_BOARD_OUTLINE: str = "Board Outline"; break;
			case LAY_MASK_TOP: str = "Top Solder Mask"; break;
			case LAY_MASK_BOTTOM: str = "Bottom Solder Mask"; break;
			case LAY_PASTE_TOP: str = "Top Solder Paste Mask"; break;
			case LAY_PASTE_BOTTOM: str = "Bottom Solder Paste Mask"; break;
			case LAY_SILK_TOP: str = "Top Silkscreen"; break;
			case LAY_SILK_BOTTOM: str = "Bottom Silkscreen"; break;
			case LAY_TOP_COPPER: str = "Top Copper Layer"; break;
			case LAY_BOTTOM_COPPER: str = "Bottom Copper Layer"; break;
			}
			if( layer > LAY_BOTTOM_COPPER )
				str.Format( "Inner %d Copper Layer", layer - LAY_BOTTOM_COPPER );
			ctext *t = new ctext (tl->m_doc, bd_min_x, bd_min_y-LAYER_TEXT_HEIGHT*2, 0, 0, 0, 
				LAY_SILK_TOP, LAYER_TEXT_HEIGHT, LAYER_TEXT_STROKE_WIDTH, tl->m_smfontutil, &str );
			// draw text
			int w = t->m_stroke_width;
			CAperture text_ap( CAperture::AP_CIRCLE, w, 0 );
			ChangeAperture( &text_ap, &current_ap, &ap_array, PASS0, f );
			if( PASS1 )
				for( int istroke=0; istroke<t->m_stroke.GetSize(); istroke++ )
				{
					::WriteMoveTo( f, t->m_stroke[istroke].xi, t->m_stroke[istroke].yi, LIGHT_OFF );
					::WriteMoveTo( f, t->m_stroke[istroke].xf, t->m_stroke[istroke].yf, LIGHT_ON );
				}
		}

		// step and repeat for panelization
		if( PASS1 )
		{
			f->WriteString( "\nG04 Step and Repeat for panelization *\n" );
			if( n_x > 1 || n_y > 1 )
			{
				CString str;
				str.Format( "SRX%dY%dI%fJ%f*", n_x, n_y, f_step_x/1000.0, f_step_y/1000.0 );
				f->WriteString( "%" + str + "%\n" );
			}
		}
		// draw board outline
		if( (flags & GERBER_BOARD_OUTLINE) || layer == LAY_BOARD_OUTLINE )
		{
			if( PASS1 )
			{
				f->WriteString( "\nG04 ----------------------- Draw board outline (positive)*\n" );
				f->WriteString( "%LPD*%\n" );
				current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
			}
			CAperture bd_ap( CAperture::AP_CIRCLE, outline_width, 0 );
			ChangeAperture( &bd_ap, &current_ap, &ap_array, PASS0, f );
			if( PASS1 )
			{
				citer<cboard> ib (bd);
				for (cboard *b = ib.First(); b; b = ib.Next())
				{
					// turn on linear interpolation, move to first corner
					ccorner *head = b->main->head;
					::WriteMoveTo( f, head->x, head->y, LIGHT_OFF );
					int index = 1;
					for (cside *s = head->postSide; 1; s = s->postCorner->postSide)
					{
						::WritePolygonSide( f, s->preCorner->x, s->preCorner->y,
							s->postCorner->x, s->postCorner->y, s->m_style, 10, LIGHT_ON ); 
						if (s->postCorner == head)
							break;
						line.Format( "G04 end of side %d*\n", index++ );
						f->WriteString( line );
					}
				}
			}
		}
		// establish nesting order of copper areas within the layer.
		// CPT2 trying a slightly modified algorithm.  In the following description "drawn" and "undrawn" are metaphorical terms;  actual
		// drawing occurs later.
		// 1. Set utility value on all areas to 0 ("undrawn").
		// 2. Loop through undrawn areas.  For each, see if there exists an undrawn area containing it.  Those which have no such container get 
		//    marked with utility value 1 (drawn), and get added to "draw_list".    
		// 3. If at the end there are remaining undrawn areas (contained within the cutout of some other area), go back to 2.
		carray<cnet2> area_net_list;
		carray<carea2> area_list;
		carray<carea2> draw_list;
		citer<cnet2> in (&nl->nets);
		for (cnet2 *net = in.First(); net; net = in.Next())
		{
			citer<carea2> ia (&net->areas);
			for (carea2 *a = ia.First(); a; a = ia.Next())
			{
				if (a->m_layer != layer)
					continue;
				// area on this layer, add to lists
				area_net_list.Add( net );				// NB takes advantage of carray::Add's uniqueness check
				area_list.Add( a );
				a->utility = 0;
			}
		}

		if( PASS1 )
		{
			bool bUndrawnAreas;
			do
			{
				bUndrawnAreas = false;
				citer<carea2> ia1 (&area_list), ia2 (&area_list);
				for (carea2 *a1 = ia1.First(); a1; a1 = ia1.Next())
				{
					if (a1->utility)
						continue;
					int x = a1->main->head->x;
					int y = a1->main->head->y;
					// Area not yet "drawn";  see if it lies within any other undrawn area.  If not, we can "draw" it now.
					bool bContainedInUndrawn = false;
					for (carea2 *a2 = ia2.First(); a2; a2 = ia2.Next())
					{
						if (a2==a1 || a2->utility)
							continue;
						citer<ccontour> ictr (&a2->contours);
						for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
							if (ctr!=a2->main && ctr->TestPointInside(x, y))
								{ bContainedInUndrawn = true; goto testEnd; }
					}
					
					testEnd:
					if (bContainedInUndrawn)
						bUndrawnAreas = true;
					else
						a1->utility = 1,
						draw_list.Add(a1);
				}
			}
			while (bUndrawnAreas);

			// now actually draw copper areas and cutouts.  We rely on the fact that when we iterate through carray draw_list, we'll go in the order
			// in which areas were added (this, because we've never removed any items from draw_list).
			citer<carea2> ia (&draw_list);
			BOOL bLastLayerNegative = FALSE;
			for (carea2 *a = ia.First(); a; a = ia.Next())
			{
				current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
				// draw outline polygon
				// make GpcPoly for outer contour of area
				a->MakeGpcPoly();
				// draw area's main contour
				f->WriteString( "\nG04 ----------------------- Draw copper area (positive)*\n" );
				if( bLastLayerNegative )
				{
					f->WriteString( "%LPD*%\n" );
					bLastLayerNegative = FALSE;
				}
				f->WriteString( "G36*\n" );
				ccorner *head = a->main->head;
				::WriteMoveTo( f, head->x, head->y, LIGHT_OFF );
				for (cside *s = head->postSide; 1; s = s->postCorner->postSide )
				{
					::WritePolygonSide( f, s->preCorner->x, s->preCorner->y, s->postCorner->x, s->postCorner->y,
						s->m_style, 10, LIGHT_ON );
					if (s->postCorner==head) 
						break; 
				}
				f->WriteString( "G37*\n" );

				// Now draw the cutouts if any
				citer<ccontour> ictr (&a->contours);
				for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
				{
					if (ctr == a->main)
						continue;
					// draw it
					f->WriteString( "\nG04 -------------------- Draw copper area cutout (negative)*\n" );
					if( !bLastLayerNegative )
					{
						f->WriteString( "%LPC*%\n" );
						bLastLayerNegative = TRUE;
					}
					f->WriteString( "G36*\n" );
					ccorner *head = ctr->head;
					::WriteMoveTo( f, head->x, head->y, LIGHT_OFF );
					for (cside *s = head->postSide; 1; s = s->postCorner->postSide )
					{
						::WritePolygonSide( f, s->preCorner->x, s->preCorner->y, s->postCorner->x, s->postCorner->y,
							s->m_style, 10, LIGHT_ON );
						if (s->postCorner==head) 
							break; 
					}
					f->WriteString( "G37*\n" );
				}
			}
		}

		if( area_list.GetSize() )
		{
			// ********** draw pad, trace, and via clearances and thermals ***********
			// CPT2 NB.  Old code had checks of the form "if (pl)" and "if (nl)".  I omitted them (really can't imagine when they'd be necessary)
			// first, remove all GpcPolys
			citer<carea2> ia (&area_list);
			for (carea2 *a = ia.First(); a; a = ia.Next())
				a->FreeGpcPoly();
			if( PASS1 ) 
			{
				f->WriteString( "\nG04 -------------------- Draw copper area clearances (negative)*\n" );
				f->WriteString( "%LPC*%\n" );
				current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
				f->WriteString( "\nG04 Draw clearances for pads*\n" );
			}

			citer<cpart2> ip (&pl->parts);
			for (cpart2 *part = ip.First(); part; part = ip.Next())
			{
				if (!part->shape)
					continue;
				citer<cpin2> ipin (&part->pins);
				for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
				{
					// get pad info
					int pad_type, pad_w, pad_l, pad_r, pad_hole;
					int pad_connect_status, pad_connect_flag, clearance_type ;
					BOOL bPad = pin->GetDrawInfo( layer, !(flags & GERBER_NO_PIN_THERMALS), !(flags & GERBER_NO_SMT_THERMALS),
						mask_clearance, paste_mask_shrink,
						&pad_type, &pad_w, &pad_l, &pad_r, &pad_hole, &pad_connect_status, &pad_connect_flag, &clearance_type );
					if (!bPad)
						continue;
					// pad or hole exists on this layer
					// parameters for clearance on adjacent areas (if needed)
					int area_pad_type = PAD_NONE;
					int area_pad_wid = 0;
					int area_pad_len = 0;
					int area_pad_radius = 0;
					int area_pad_clearance = 0;
					// parameters for clearance aperture
					int type = CAperture::AP_NONE;
					int size1=0, size2=0, size3=0; 
					if( pad_type == PAD_NONE && pad_hole > 0 )
					{
						// through-hole pin, no pad, just annular ring and hole
						if( pad_connect_status & CPartList::AREA_CONNECT )
						{
							// hole connects to copper area
							if( flags & GERBER_NO_PIN_THERMALS || pad_connect_flag == PAD_CONNECT_NOTHERMAL )
							{
								// no thermal, no clearance needed except on adjacent areas for hole
								area_pad_type = PAD_ROUND;
								area_pad_wid = pad_hole + 2*hole_clearance;
								area_pad_clearance = 0;
								// testing
								if( clearance_type != CLEAR_NONE )
									ASSERT(0);
							}
							else
							{
								// make thermal for annular ring and hole
								type = CAperture::AP_THERMAL;
								size1 = max( pad_w + 2*fill_clearance, pad_hole + 2*hole_clearance );
								size2 = pad_w;	// inner diameter
								area_pad_type = PAD_ROUND;
								area_pad_wid = size1;
								area_pad_clearance = 0;
								// testing
								if( clearance_type != CLEAR_THERMAL )
									ASSERT(0);
							}
						}
						else
						{
							// no area connection, just make clearance for annular ring and hole
							type = CAperture::AP_CIRCLE;
							size1 = max( pad_w + 2*fill_clearance, pad_hole + 2*hole_clearance );
							// testing
							if( clearance_type != CLEAR_NORMAL )
								ASSERT(0);
						}
					}
					else if( pad_type != PAD_NONE )
					{
						if( pad_connect_status & cpin2::AREA_CONNECT ) 
						{
							// pad connects to copper area, make clearance or thermal
							// see if we need to make a thermal
							BOOL bMakeThermal = (clearance_type == CLEAR_THERMAL);
							if( pad_type == PAD_ROUND )
							{
								size1 = max( pad_w + 2*fill_clearance, pad_hole + 2*hole_clearance );
								if( bMakeThermal )
								{
									// make thermal for pad
									type = CAperture::AP_THERMAL;
									size2 = pad_w;	// inner diameter
								}
								// make clearance for adjacent areas
								area_pad_type = pad_type;
								area_pad_wid = size1;
							}
							else if( pad_type == PAD_OCTAGON )
							{
								size1 = (max( pad_w + 2*fill_clearance, pad_hole + 2*hole_clearance ))/cos_oct;
								if( bMakeThermal )
								{
									// make thermal for pad
									type = CAperture::AP_THERMAL; 
									size2 = pad_w;	// inner diameter
								}
								area_pad_type = pad_type;
								area_pad_wid = size1;
							}
							else if( pad_type == PAD_RECT || pad_type == PAD_RRECT || pad_type == PAD_OVAL 
								|| pad_type == PAD_SQUARE )
							{
								if( bMakeThermal )
								{
									// make thermal for pad
									// can't use an aperture for this pad, need to draw a polygon
									// if hole, check hole clearance
									int x1, x2, y1, y2;
									if( pad_type == PAD_RECT || pad_type == PAD_RRECT || pad_type == PAD_OVAL )
									{
										// CPT2 TODO must test this (old code with its pad_angle business was the opposite of what I expected)
										x1 = pin->x - pad_l/2;
										x2 = pin->x + pad_l/2;
										y1 = pin->y - pad_w/2;
										y2 = pin->y + pad_w/2;
									}
									else
									{
										x1 = pin->x - pad_w/2;
										x2 = pin->x + pad_w/2;
										y1 = pin->y - pad_w/2;
										y2 = pin->y + pad_w/2;
									}
									int x_clearance = fill_clearance;
									int y_clearance = fill_clearance;
									if( pad_hole )
									{
										// if necessary, adjust for hole clearance
										x_clearance = max( fill_clearance, hole_clearance+pad_hole/2-((x2-x1)/2));
										y_clearance = max( fill_clearance, hole_clearance+pad_hole/2-((y2-y1)/2));
									}
									// add clearance
									x1 -= x_clearance;
									x2 += x_clearance;
									y1 -= y_clearance;
									y2 += y_clearance;
									// make thermal for pad
									type = CAperture::AP_RECT_THERMAL;   
									size1 = x2 - x1;	// width
									size2 = y2 - y1;	// height
									size3 = thermal_wid;
								}
								area_pad_type = pad_type;
								area_pad_wid = pad_w;
								area_pad_len = pad_l;
								area_pad_clearance = fill_clearance;
							}
						}
						else
						{
							// pad doesn't connect to area, make clearance for pad and hole
							size1 = max ( pad_w + 2*fill_clearance, pad_hole + 2*hole_clearance );
							size2 = 0;
							if( pad_type == PAD_ROUND )
							{
								type = CAperture::AP_CIRCLE;
							}
							else if( pad_type == PAD_SQUARE )
							{
								type = CAperture::AP_SQUARE;
							}
							else if( pad_type == PAD_OCTAGON )
							{
								type = CAperture::AP_OCTAGON;
							}
							else if( pad_type == PAD_OVAL || pad_type == PAD_RECT || pad_type == PAD_RRECT ) 
							{
								if( pad_type == PAD_OVAL )
									type = CAperture::AP_OVAL;
								if( pad_type == PAD_RECT )
									type = CAperture::AP_RECT;
								if( pad_type == PAD_RRECT )
									type = CAperture::AP_RRECT;
								size2 = max ( pad_l + 2*fill_clearance, pad_hole + 2*hole_clearance );
								if( pad_type == PAD_RRECT )
									size3 = pad_r + fill_clearance;
							}
						}
					}
					// now flash the aperture
					if( type != CAperture::AP_NONE )  
					{
						CAperture pad_ap( type, size1, size2, size3 );
						ChangeAperture( &pad_ap, &current_ap, &ap_array, PASS0, f );
						if( PASS1 )
						{
							// now flash the pad
							::WriteMoveTo( f, pin->x, pin->y, LIGHT_FLASH );
						}
					}
					// now create clearances on adjacent areas for pins connected
					// to copper area
					if( area_pad_type != PAD_NONE ) 
					{
						int type = CAperture::AP_CIRCLE;
						int size1 = CLEARANCE_POLY_STROKE_MILS*NM_PER_MIL;
						CAperture pad_ap( type, size1, 0 );
						ChangeAperture( &pad_ap, &current_ap, &ap_array, PASS0, f );
						if( PASS1 ) 
						{
							DrawClearanceInForeignAreas( pin->net, area_pad_type, 0, pin->x, pin->y,
								0, 0, area_pad_wid, area_pad_len, area_pad_radius, 0,
								f, flags, layer, area_pad_clearance, &area_list );
						}
					}
				}
			}

			// iterate through all nets and draw trace and via clearances.
			// CPT2:  we'll do trace clearances in one loop, and vias in a separate one afterwards.  Old code had
			// the problem that vias at the head of a connect wouldn't get done.
			if( PASS1 )
				f->WriteString( "\nG04 Draw clearances for traces and vias*\n" );
			citer<cnet2> in (&nl->nets);
			for (cnet2 *net = in.First(); net; net = in.Next())
			{
				citer<cconnect2> ic (&net->connects);
				for (cconnect2 *c = ic.First(); c; c = ic.Next())
				{
					citer<cseg2> is (&c->segs);
					for (cseg2 *s = is.First(); s; s = is.Next())
					{
						if (s->m_layer != layer)
							continue;
						int xi = s->preVtx->x, yi = s->preVtx->y;
						int xf = s->postVtx->x, yf = s->postVtx->y;
						if( area_net_list.GetSize() == 1 && area_net_list.First() != net ) 
						{
							// all areas on this layer belong to a single net, which is not s's net:  draw clearance
							int type = CAperture::AP_CIRCLE;
							int size1 = s->m_width + 2*fill_clearance;
							CAperture seg_ap( type, size1, 0 );
							ChangeAperture( &seg_ap, &current_ap, &ap_array, PASS0, f );
							if( PASS1 )
							{
								WriteMoveTo( f, xi, yi, LIGHT_OFF );
								WriteMoveTo( f, xf, yf, LIGHT_ON );
							}
						}
						else if( area_net_list.GetSize() > 1 )
						{
							// test for segment intersection with area on own net
							BOOL bIntOwnNet = FALSE;
							citer<carea2> ia (&net->areas);
							for (carea2 *a = ia.First(); a; a = ia.Next())
							{
								if( a->TestPointInside( xi, yi ) || a->TestPointInside( xi, yi ) )
									{ bIntOwnNet = TRUE; break; }
								citer<ccontour> ictr (&a->contours);
								for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
								{
									citer<cside> is (&ctr->sides);
									for (cside *sd = is.First(); sd; sd = is.Next())
									{
										// test for clearance from area sides < fill_clearance
										int x2i = sd->preCorner->x, y2i = sd->preCorner->y;
										int x2f = sd->postCorner->x, y2f = sd->postCorner->y;
										int style2 = sd->m_style;
										int d = ::GetClearanceBetweenSegments( xi, yi, xf, yf, CPolyLine::STRAIGHT, s->m_width,
											x2i, y2i, x2f, y2f, style2, 0, fill_clearance, 0, 0 );
										if( d < fill_clearance )
											{ bIntOwnNet = TRUE; goto end_loop; }
									}
								}
							}

							end_loop:
							if( bIntOwnNet )
							{
								// set aperture for stroke outline
								int type = CAperture::AP_CIRCLE;
								int size1 = CLEARANCE_POLY_STROKE_MILS*NM_PER_MIL;
								CAperture seg_ap( type, size1, 0 );
								ChangeAperture( &seg_ap, &current_ap, &ap_array, PASS0, f );
								if( PASS1 )
								{
									// handle segment that crosses from an area on its own net to
									// an area on a foreign net
									int type = CAperture::AP_CIRCLE;
									int size1 = CLEARANCE_POLY_STROKE_MILS*NM_PER_MIL;
									CAperture pad_ap( type, size1, 0 );
									ChangeAperture( &pad_ap, &current_ap, &ap_array, PASS0, f );
									if( PASS1 ) 
										DrawClearanceInForeignAreas( net, -1, s->m_width, xi, yi, xf, yf,
											0, 0, 0, 0, f, flags, layer, fill_clearance, 
											&area_list );
								}
							}
							else
							{
								// segment does not intersect area on own net, just make clearance
								int w = s->m_width + 2*fill_clearance;
								CAperture seg_ap( CAperture::AP_CIRCLE, w, 0 );
								ChangeAperture( &seg_ap, &current_ap, &ap_array, PASS0, f );
								if( PASS1 )
								{
									WriteMoveTo( f, s->preVtx->x, s->preVtx->y, LIGHT_OFF );
									WriteMoveTo( f, s->postVtx->x, s->postVtx->y, LIGHT_ON );
								}
							}
						}
					}

					// Now check connect's vias.  Note that vias on tees may have their clearances drawn repeatedly (oh well)
					citer<cvertex2> iv (&c->vtxs);
					for (cvertex2 *v = iv.First(); v; v = iv.Next())
					{
						int test;
						int pad_w;
						int hole_w;
						v->GetViaPadInfo( layer, &pad_w, &hole_w, &test );
						// flash the via clearance if necessary
						if (hole_w == 0 || layer < LAY_TOP_COPPER)
							continue;
						// this is a copper layer
						// set aperture to draw normal via clearance
						int type = CAperture::AP_CIRCLE;
						int size1 = max( pad_w + 2*fill_clearance, hole_w + 2*hole_clearance );
						int size2 = 0;
						// set parameters for no foreign area clearance
						int area_pad_type = PAD_NONE;
						int area_pad_wid = 0;
						int area_pad_len = 0;
						int area_pad_radius = 0;
						int area_pad_angle = 0;
						int area_pad_clearance = 0;
						if( pad_w == 0 )
						{
							// no pad, just make hole clearance 
							type = CAperture::AP_CIRCLE;
							size1 = hole_w + 2*hole_clearance;
						}
						else if( test & cvertex2::VIA_AREA )
						{
							// inner layer and connected to copper area
							if( flags & GERBER_NO_VIA_THERMALS )
							{
								// no thermal, therefore no clearance
								type = CAperture::AP_NONE;
								// except on adjacent foreign nets
								area_pad_type = PAD_ROUND;
								area_pad_wid = max( pad_w + 2*fill_clearance, hole_w + 2*hole_clearance );
							}
							else
							{
								// thermal
								type = CAperture::AP_THERMAL;
								size1 = max( pad_w + 2*fill_clearance, hole_w + 2*hole_clearance );
								size2 = pad_w;
								area_pad_type = PAD_ROUND;
								area_pad_wid = size1;
							}
						}
						if( type != CAperture::AP_NONE )
						{
							CAperture via_ap( type, size1, size2 );
							ChangeAperture( &via_ap, &current_ap, &ap_array, PASS0, f );
							if( PASS1 )
								// flash the via clearance
								WriteMoveTo( f, v->x, v->y, LIGHT_FLASH );
						}
						if( area_pad_type == PAD_ROUND )
						{
							int type = CAperture::AP_CIRCLE;
							int size1 = CLEARANCE_POLY_STROKE_MILS*NM_PER_MIL;
							CAperture stroke_ap( type, size1, 0 );
							ChangeAperture( &stroke_ap, &current_ap, &ap_array, PASS0, f );
							if( PASS1 )
								DrawClearanceInForeignAreas( net, area_pad_type, 0,
									v->x, v->y, 0, 0, area_pad_wid, 0, 0, 0,
									f, flags, layer, 0, &area_list );
						}
					}
				}
			}

			// draw clearances for text
			if( PASS1 )
				f->WriteString( "\nG04 Draw clearances for text*\n" );
			citer<ctext> it (&tl->texts);
			for (ctext *t = it.First(); t; t = it.Next())
				if( t->m_layer == layer )
				{
					// draw text
					int w = t->m_stroke_width + 2*fill_clearance;
					if( t->m_bNegative )
						w = t->m_stroke_width;	// if negative text, just draw the text
					CAperture text_ap( CAperture::AP_CIRCLE, w, 0 );
					ChangeAperture( &text_ap, &current_ap, &ap_array, PASS0, f );
					if( PASS1 )
					{
						for( int istroke=0; istroke<t->m_stroke.GetSize(); istroke++ )
						{
							::WriteMoveTo( f, t->m_stroke[istroke].xi, t->m_stroke[istroke].yi, LIGHT_OFF );
							::WriteMoveTo( f, t->m_stroke[istroke].xf, t->m_stroke[istroke].yf, LIGHT_ON );
						}
					}
				}
		}

		// ********************** draw pads, vias and traces **************************
		if( PASS1 )
		{
			f->WriteString( "\nG04 -------------- Draw Parts, Pads, Traces, Vias and Text (positive)*\n" );
			f->WriteString( "%LPD*%\n" );
			current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
		}
		// draw pads, silkscreen items, reference designators and value
		// iterate through all parts and draw pads
		citer<cpart2> ip (&pl->parts);
		for (cpart2 *part = ip.First(); part; part = ip.Next())
		{
			if (!part->shape)
				continue;
			if( PASS1 )
			{
				line.Format( "G04 Draw part %s*\n", part->ref_des ); 
				f->WriteString( line );
			}
			citer<cpin2> ipin (&part->pins);
			for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
			{
				// get pad info
				int pad_w, pad_l, pad_r, pad_type, pad_hole, pad_connect;
				BOOL bPad = pin->GetDrawInfo( layer, 0, 0, mask_clearance, paste_mask_shrink,
					&pad_type, &pad_w, &pad_l, &pad_r, &pad_hole, &pad_connect );
				if (!bPad || pad_type == PAD_NONE || pad_w == 0)
					continue;
				// draw pad
				int type, size1, size2, size3;
				if( pad_type == PAD_ROUND || pad_type == PAD_SQUARE 
					|| pad_type == PAD_OCTAGON || pad_type == PAD_OVAL
					|| pad_type == PAD_RECT || pad_type == PAD_RRECT )
				{
					type = CAperture::AP_CIRCLE;
					size1 = pad_w;
					size2 = 0;
					size3 = 0;
					if( pad_type == PAD_SQUARE )
						type = CAperture::AP_SQUARE;
					else if( pad_type == PAD_OCTAGON )
						type = CAperture::AP_OCTAGON;
					else if( pad_type == PAD_OVAL || pad_type == PAD_RECT || pad_type == PAD_RRECT )
					{
						if( pad_type == PAD_OVAL )
							type = CAperture::AP_OVAL;
						if( pad_type == PAD_RECT )
							type = CAperture::AP_RECT;
						if( pad_type == PAD_RRECT )
							type = CAperture::AP_RRECT;
						size2 = pad_l; 
						size3 = pad_r;
					}
					CAperture pad_ap( type, size1, size2, size3 ); 
					ChangeAperture( &pad_ap, &current_ap, &ap_array, PASS0, f );
					if( PASS1 )
						// now flash the pad
						::WriteMoveTo( f, pin->x, pin->y, LIGHT_FLASH );
				}
			}
			// draw part outline and text strings if any strokes on this layer
			BOOL bOnLayer = FALSE;
			int nstrokes = part->m_outline_stroke.GetSize();
			for( int ips=0; ips<nstrokes; ips++ )
				if( part->m_outline_stroke[ips].layer == layer )
					{ bOnLayer = TRUE; break; }
			if( bOnLayer )
			{
				if( PASS1 )
				{
					line.Format( "G04 draw outline and text for part %s*\n", part->ref_des ); 
					f->WriteString( line );
				}
				for( int ips=0; ips<nstrokes; ips++ )
				{
					if( part->m_outline_stroke[ips].layer == layer )
					{
						int s_w = max( part->m_outline_stroke[ips].w, min_silkscreen_stroke_wid );
						CAperture outline_ap( CAperture::AP_CIRCLE, s_w, 0 );
						ChangeAperture( &outline_ap, &current_ap, &ap_array, PASS0, f );
						if( PASS1 )
						{
							// move to start of stroke
							::WriteMoveTo( f, part->m_outline_stroke[ips].xi, 
								part->m_outline_stroke[ips].yi, LIGHT_OFF );
							int type;
							if( part->m_outline_stroke[ips].type == DL_LINE )
								type = CPolyLine::STRAIGHT;
							else if( part->m_outline_stroke[ips].type == DL_ARC_CW )
								type = CPolyLine::ARC_CW;
							else if( part->m_outline_stroke[ips].type == DL_ARC_CCW )
								type = CPolyLine::ARC_CCW;
							else
								ASSERT(0);
							::WritePolygonSide( f, 
								part->m_outline_stroke[ips].xi, 
								part->m_outline_stroke[ips].yi, 
								part->m_outline_stroke[ips].xf, 
								part->m_outline_stroke[ips].yf, 
								type, 10, LIGHT_ON );
						}
					}
				}
			}
			// draw reference designator text
			if( part->m_ref->m_font_size && part->m_ref->m_bShown && part->m_ref->GetLayer() == layer)
			{
				if( PASS1 )
				{
					line.Format( "G04 draw reference designator for part %s*\n", part->ref_des ); 
					f->WriteString( line );
				}
				int s_w = max( part->m_ref->m_stroke_width, min_silkscreen_stroke_wid );
				CAperture ref_ap( CAperture::AP_CIRCLE, s_w, 0 );
				ChangeAperture( &ref_ap, &current_ap, &ap_array, PASS0, f );
				if( PASS1 )
					for( int istroke=0; istroke<part->m_ref->m_stroke.GetSize(); istroke++ )
					{
						::WriteMoveTo( f, part->m_ref->m_stroke[istroke].xi, part->m_ref->m_stroke[istroke].yi, LIGHT_OFF );
						::WriteMoveTo( f, part->m_ref->m_stroke[istroke].xf, part->m_ref->m_stroke[istroke].yf, LIGHT_ON );
					}
			}
			// draw value text
			if( part->m_value->m_font_size && part->m_value->m_bShown && part->m_value->GetLayer() == layer)
			{
				if( PASS1 )
				{
					line.Format( "G04 draw value for part %s*\n", part->ref_des ); 
					f->WriteString( line );
				}
				int s_w = max( part->m_value->m_stroke_width, min_silkscreen_stroke_wid );
				CAperture value_ap( CAperture::AP_CIRCLE, s_w, 0 );
				ChangeAperture( &value_ap, &current_ap, &ap_array, PASS0, f );
				if( PASS1 )
					for( int istroke=0; istroke<part->m_value->m_stroke.GetSize(); istroke++ )
					{
						::WriteMoveTo( f, part->m_value->m_stroke[istroke].xi, part->m_value->m_stroke[istroke].yi, LIGHT_OFF );
						::WriteMoveTo( f, part->m_value->m_stroke[istroke].xf, part->m_value->m_stroke[istroke].yf, LIGHT_ON );
					}
			}
		}

		// draw vias and traces
		if( PASS1 )
			f->WriteString( "\nG04 Draw traces*\n" );
		for (cnet2 *net = in.First(); net; net = in.Next())
		{
			citer<cconnect2> ic (&net->connects);
			for (cconnect2 *c = ic.First(); c; c = ic.Next())
			{
				citer<cseg2> is (&c->segs);
				for (cseg2 *s = is.First(); s; s = is.Next())
					if( s->m_layer == layer )
					{
						// segment is on this layer, draw it
						int w = s->m_width;
						CAperture seg_ap( CAperture::AP_CIRCLE, w, 0 );
						ChangeAperture( &seg_ap, &current_ap, &ap_array, PASS0, f );
						if( PASS1 )
						{
							WriteMoveTo( f, s->preVtx->x, s->preVtx->y, LIGHT_OFF );
							WriteMoveTo( f, s->postVtx->x, s->postVtx->y, LIGHT_ON );
						}
					}
				citer<cvertex2> iv (&c->vtxs);
				for (cvertex2 *v = iv.First(); v; v = iv.Next())
				{
					// get following via info
					int test, pad_w, hole_w;
					v->GetViaPadInfo( layer, &pad_w, &hole_w, &test );
					if( !pad_w )
						continue;
					// via exists
					CAperture via_ap( CAperture::AP_CIRCLE, 0, 0 );
					int w = 0;
					if( layer == LAY_MASK_TOP || layer == LAY_MASK_BOTTOM )
					{
						if( !(flags & GERBER_MASK_VIAS) )
							// solder mask layer, add mask clearance
							w = pad_w + 2*mask_clearance;
					}
					else if( layer >= LAY_TOP_COPPER )
					{
						// copper layer, set aperture
						w = pad_w;							
					}
					if( !w )
						continue;
					via_ap.m_size1 = w;
					ChangeAperture( &via_ap, &current_ap, &ap_array, PASS0, f );
					// flash the via
					if( PASS1 )
						WriteMoveTo( f, v->x, v->y, LIGHT_FLASH );
				}
			}
		}

		// draw text
		if( PASS1 )
			f->WriteString( "\nG04 Draw Text*\n" );
		citer<ctext> it (&tl->texts);
		for (ctext *t = it.First(); t; t = it.Next())
			if( !t->m_bNegative && t->m_font_size && t->m_layer == layer )
			{
				// draw text
				int w = t->m_stroke_width;
				if( layer == LAY_SILK_TOP || layer == LAY_SILK_BOTTOM )
					w = max( t->m_stroke_width, min_silkscreen_stroke_wid );
				CAperture text_ap( CAperture::AP_CIRCLE, w, 0 );
				ChangeAperture( &text_ap, &current_ap, &ap_array, PASS0, f );
				if( PASS1 )
					for( int istroke=0; istroke<t->m_stroke.GetSize(); istroke++ )
					{
						::WriteMoveTo( f, t->m_stroke[istroke].xi, t->m_stroke[istroke].yi, LIGHT_OFF );
						::WriteMoveTo( f, t->m_stroke[istroke].xf, t->m_stroke[istroke].yf, LIGHT_ON );
					}
			}

		// draw solder mask cutouts.  CPT2.  Now that smcutouts are generalized polylines, we have to do a nesting check like we did with copper areas.
		if (layer == LAY_MASK_TOP || layer == LAY_MASK_BOTTOM)
		{
			if( !(flags & GERBER_NO_CLEARANCE_SMCUTOUTS) )
			{
				// we will draw stroke outline with aperture to create clearance
				CAperture sm_ap( CAperture::AP_CIRCLE, mask_clearance*2, 0 );
				ChangeAperture( &sm_ap, &current_ap, &ap_array, PASS0, f );
			}

			if( !PASS1 )
				continue;
			f->WriteString( "\nG04 Draw solder mask cutouts*\n" );
			carray<csmcutout> sm_list;
			carray<csmcutout> draw_list;
			citer<csmcutout> ism0 (sm);
			for (csmcutout *sm0 = ism0.First(); sm0; sm0 = ism0.Next())
			{
				if (sm0->m_layer != layer)
					continue;
				sm_list.Add( sm0 );
				sm0->utility = 0;
			}

			bool bUndrawnSms;
			do
			{
				bUndrawnSms = false;
				citer<csmcutout> ism1 (&sm_list), ism2 (&sm_list);
				for (csmcutout *sm1 = ism1.First(); sm1; sm1 = ism1.Next())
				{
					if (sm1->utility)
						continue;
					int x = sm1->main->head->x;
					int y = sm1->main->head->y;
					// Sm not yet "drawn";  see if it lies within any other undrawn sm.  If not, we can "draw" it now.
					bool bContainedInUndrawn = false;
					for (csmcutout *sm2 = ism2.First(); sm2; sm2 = ism2.Next())
					{
						if (sm2==sm1 || sm2->utility)
							continue;
						citer<ccontour> ictr (&sm2->contours);
						for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
							if (ctr!=sm2->main && ctr->TestPointInside(x, y))
								{ bContainedInUndrawn = true; goto testEnd2; }
					}
					
					testEnd2:
					if (bContainedInUndrawn)
						bUndrawnSms = true;
					else
						sm1->utility = 1,
						draw_list.Add(sm1);
				}
			}
			while (bUndrawnSms);

			// now actually draw smcutouts.  We rely on the fact that when we iterate through carray draw_list, we'll go in the order
			// in which sm's were added (this, because we've never removed any items from draw_list).
			// Note that positive and negative are opposite compared to the case of copper areas.
			citer<csmcutout> ism1 (&draw_list);
			BOOL bLastLayerNegative = FALSE;
			for (csmcutout *sm1 = ism1.First(); sm1; sm1 = ism1.Next())
			{
				current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
				// draw outline polygon
				// make GpcPoly for outer contour of sm.  CPT2 TODO consider dumping
				sm1->MakeGpcPoly();
				// draw area's main contour
				f->WriteString( "\nG04 ----------------------- Draw smcutout (negative)*\n" );
				if( !bLastLayerNegative )
				{
					f->WriteString( "%LPC*%\n" );
					bLastLayerNegative = true;
				}
				f->WriteString( "G36*\n" );
				ccorner *head = sm1->main->head;
				::WriteMoveTo( f, head->x, head->y, LIGHT_OFF );
				for (cside *s = head->postSide; 1; s = s->postCorner->postSide )
				{
					::WritePolygonSide( f, s->preCorner->x, s->preCorner->y, s->postCorner->x, s->postCorner->y,
						s->m_style, 10, LIGHT_ON );
					if (s->postCorner==head) 
						break; 
				}
				f->WriteString( "G37*\n" );

				// Now draw the sub-cutouts if any
				citer<ccontour> ictr (&sm1->contours);
				for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
				{
					if (ctr == sm1->main)
						continue;
					// draw it
					f->WriteString( "\nG04 -------------------- Draw smcutout (positive)*\n" );
					if( bLastLayerNegative )
					{
						f->WriteString( "%LPD*%\n" );
						bLastLayerNegative = false;
					}
					f->WriteString( "G36*\n" );
					ccorner *head = ctr->head;
					::WriteMoveTo( f, head->x, head->y, LIGHT_OFF );
					for (cside *s = head->postSide; 1; s = s->postCorner->postSide )
					{
						::WritePolygonSide( f, s->preCorner->x, s->preCorner->y, s->postCorner->x, s->postCorner->y,
							s->m_style, 10, LIGHT_ON );
						if (s->postCorner==head) 
							break; 
					}
					f->WriteString( "G37*\n" );
				}

				// Draw main contour with aperature for clearance
				if( !(flags & GERBER_NO_CLEARANCE_SMCUTOUTS) )
				{
					ccorner *head = sm1->main->head;
					::WriteMoveTo( f, head->x, head->y, LIGHT_OFF );
					for (cside *s = head->postSide; 1; s = s->postCorner->postSide )
					{
						::WritePolygonSide( f, s->preCorner->x, s->preCorner->y, s->postCorner->x, s->postCorner->y,
							s->m_style, 10, LIGHT_ON );
						if (s->postCorner==head) 
							break; 
					}
				}
			}
		}

		// draw pilot holes for pads and vias
		if( (flags & GERBER_PILOT_HOLES) && pilot_diameter && (layer == LAY_TOP_COPPER || layer == LAY_BOTTOM_COPPER ) )
		{
			if( PASS1 )
			{
				f->WriteString( "\nG04 ----------------------- Draw Pilot Holes (scratch)*\n" );
				f->WriteString( "%LPC*%\n" );
				current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
			}
			// iterate through all parts
			citer<cpart2> ip (&pl->parts);
			for (cpart2 *part = ip.First(); part; part = ip.Next())
			{
				if (!part->shape)
					continue;
				if( PASS1 )
				{
					line.Format( "G04 draw pilot holes for part %s*\n", part->ref_des ); 
					f->WriteString( line );
				}
				citer<cpin2> ipin (&part->pins);
				for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
					if( pin->ps->hole_size )
					{
						// check current aperture and change if needed
						CAperture pad_ap( CAperture::AP_CIRCLE, pilot_diameter, 0 );
						ChangeAperture( &pad_ap, &current_ap, &ap_array, PASS0, f );
						// now flash the pad
						if( PASS1 )
							::WriteMoveTo( f, pin->x, pin->y, LIGHT_FLASH );
					}
			}
			// draw pilot holes for vias
			// iterate through all nets
			if( PASS1 )
				f->WriteString( "\nG04 Draw pilot holes for vias*\n" );
			citer<cnet2> in (&nl->nets);
			for (cnet2 *net = in.First(); net; net = in.Next())
			{
				citer<cconnect2> ic (&net->connects);
				for (cconnect2 *c = ic.First(); c; c = ic.Next())
				{
					citer<cvertex2> iv (&c->vtxs);
					for (cvertex2 *v = iv.First(); v; v = iv.Next())
						if( v->via_w )
						{
							// via exists:  flash it
							CAperture via_ap( CAperture::AP_CIRCLE, pilot_diameter, 0 );
							ChangeAperture( &via_ap, &current_ap, &ap_array, PASS0, f );
							if( PASS1 )
								::WriteMoveTo( f, v->x, v->y, LIGHT_FLASH );
						}
				}
			}
		}

		// end of file
		if( PASS1 )
			f->WriteString( "M00*\n" );
	}	// end of pass

	return 0;
}

/* CPT2 TODO Obsolete?
// find value in CArray<int> and return position in array
// if not found, add to array if add_ok = TRUE, otherwise return -1
//
int AddToArray( int value, CArray<int,int> * array )
{
	for( int i=0; i<array->GetSize(); i++ )
		if( value == array->GetAt(i) )
			return i;
	array->Add( value );
	return array->GetSize()-1;
}
*/

class Triple 
{
	// CPT2 helper class for WriteDrillFile().  
public:
	int diameter;
	int x, y;
	Triple( int _diameter=0, int _x=0, int _y=0 )
		{ diameter = _diameter; x = _x; y = _y; }
};

int CompareTriples(const Triple *t1, const Triple *t2) 
	{ return t1->diameter < t2->diameter? -1: 1; }

// write NC drill file
//
int WriteDrillFile( CStdioFile * file, cpartlist * pl, cnetlist * nl, carray<cboard> * bd,
				   int n_x, int n_y, int space_x, int space_y )
{
	CArray<Triple> data;

	// first, find gather hole data for parts
	citer<cpart2> ip (&pl->parts);
	for (cpart2 *part = ip.First(); part; part = ip.Next())
	{
		if (!part->shape)
			continue;
		citer<cpin2> ipin (&part->pins);
		for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
			if( pin->ps->hole_size )
				data.Add( Triple(pin->ps->hole_size, pin->x, pin->y) ) ;
	}
	// gather hole data for vias
	citer<cnet2> in (&nl->nets);
	for (cnet2 *net = in.First(); net; net = in.Next())
	{
		citer<cconnect2> ic (&net->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
		{
			citer<cvertex2> iv (&c->vtxs);
			for (cvertex2 *v = iv.First(); v; v = iv.Next())
				if( v->via_w  && v->via_hole_w )
					data.Add( Triple( v->via_hole_w, v->x, v->y ));
		}
	}
	// Sort in ascending hole size order:
	qsort(data.GetData(), data.GetSize(), sizeof(Triple), (int (*)(const void*,const void*)) CompareTriples);

	// now, write data to file
	CString str;
	int prev = -1, index = 1;
	for( int id=0; id<data.GetSize(); id++ )
	{
		int dia = data[id].diameter;
		if (dia==prev) continue;
		prev = dia;
		str.Format( ";Holesize %d = %6.1f PLATED MILS\n", 
			index++, (double)dia / NM_PER_MIL );
		file->WriteString( str );
	}
	file->WriteString( "M48\n" );	// start header
	file->WriteString( "INCH,00.0000\n" );	// format (inch,retain all zeros,2.4)
	prev = -1, index = 1;
	for( int id=0; id<data.GetSize(); id++ )
	{
		// write hole sizes
		int dia = data[id].diameter;
		if (dia==prev) continue;
		prev = dia;
		str.Format( "T%02dC%5.3f\n", index++, (double)dia / (1000.0*NM_PER_MIL) ); 
		file->WriteString( str );
	}
	file->WriteString( "%\n" );		// start data
	file->WriteString( "G05\n" );	// drill mode
	file->WriteString( "G90\n" );	// absolute data

	// get boundaries of board outline
	int bd_min_x = INT_MAX;
	int bd_min_y = INT_MAX;
	int bd_max_x = INT_MIN;
	int bd_max_y = INT_MIN;
	citer<cboard> ib (bd);
	for (cboard *b = ib.First(); b; b = ib.Next())
	{
		citer<ccorner> ic (&b->main->corners);
		for (ccorner *c = ic.First(); c; c = ic.Next())
		{
			if( c->x < bd_min_x )
				bd_min_x = c->x;
			if( c->x > bd_max_x )
				bd_max_x = c->x;
			if( c->y < bd_min_y )
				bd_min_y = c->y;
			if( c->y > bd_max_y )
				bd_max_y = c->y;
		}
	}
	int x_step = bd_max_x - bd_min_x + space_x;
	int y_step = bd_max_y - bd_min_y + space_y;
	prev = -1, index = 1;
	for( int id=0; id<data.GetSize(); id++ )
	{
		// now write hole size and all holes
		int dia = data[id].diameter;
		if (dia!=prev)
			str.Format( "T%02d\n", index++ ),
			file->WriteString( str );
		prev = dia;
		// loop for panelization
		for( int ix=0; ix<n_x; ix++ )
			for (int iy=0; iy<n_y; iy++ )
			{
				int x_offset = ix * x_step;
				int y_offset = iy * y_step;
				str.Format( "X%.6dY%.6d\n", (data[id].x + x_offset) / (NM_PER_MIL/10), 
											(data[id].y + y_offset) / (NM_PER_MIL/10) );
				file->WriteString( str );
			}
	}
	file->WriteString( "M30\n" );	// program end
	return 0;
}

