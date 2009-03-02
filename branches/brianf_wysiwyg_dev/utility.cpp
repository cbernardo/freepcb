// utility routines
//
#include "stdafx.h"
#include <math.h>
#include <time.h>
#include "DisplayList.h" 
 
// globals for timer functions
LARGE_INTEGER PerfFreq, tStart, tStop; 
int PerfFreqAdjust;
int OverheadTicks;

// function to find inflection-pont to create a "dogleg" of two straight-line segments
// where one segment is vertical or horizontal and the other is at 45 degrees or 90 degrees
// enter with:
//	pi = start point
//	pf = end point
//	mode = IM_90_45 or IM_45_90 or IM_90
//
CPoint GetInflectionPoint( CPoint pi, CPoint pf, int mode )
{
	CPoint p = pi;
	if( mode == IM_NONE )
		return p;

	int dx = pf.x - pi.x;
	int dy = pf.y - pi.y;
	if( dx == 0 || dy == 0 || abs(dx) == abs(dy) )
	{
		// only one segment needed
	}
	else
	{
		if( abs(dy) > abs(dx) )
		{
			// vertical > horizontal
			if( mode == IM_90 )
			{
				p.x = pi.x;
				p.y = pf.y;
			}
			else if( mode == IM_45_90 || mode == IM_90_45 )
			{
				int vert;	// length of vertical line needed
				if( dy > 0 )
					vert = dy - abs(dx);	// positive	
				else
					vert = dy + abs(dx);	// negative
				if( mode == IM_90_45 )
					p.y = pi.y + vert;
				else if( mode == IM_45_90 )
				{
					p.y = pf.y - vert;
					p.x = pf.x;
				}
			}
			else
				ASSERT(0);
		}
		else
		{
			// horizontal > vertical
			if( mode == IM_90 )
			{
				p.x = pf.x;
				p.y = pi.y;
			}
			else if( mode == IM_45_90 || mode == IM_90_45 )
			{
				int hor;	// length of horizontal line needed
				if( dx > 0 )
					hor = dx - abs(dy);	// positive	
				else
					hor = dx + abs(dy);	// negative
				if( mode == IM_90_45 )
					p.x = pi.x + hor;
				else if( mode == IM_45_90 )
				{
					p.x = pf.x - hor;
					p.y = pf.y;
				}
			}
			else
				ASSERT(0);
		}
	}
	return p;
}

// 
// function to rotate a point clockwise about another point
// currently, angle must be 0, 90, 180 or 270
//
void RotatePoint( CPoint *p, int angle, CPoint org ) 
{
	CRect tr;
	if( angle == 90 )
	{
		int tempy = org.y + (org.x - p->x);
		p->x = org.x + (p->y - org.y);
		p->y = tempy;
	}
	else if( angle > 90 )
	{
		for( int i=0; i<(angle/90); i++ )
			RotatePoint( p, 90, org );
	}
}

// function to rotate a rectangle clockwise about a point
// angle must be 0, 90, 180 or 270
// on exit, r->top > r.bottom, r.right > r.left
//
void RotateRect( CRect *r, int angle, CPoint org )
{
	CRect tr;
	if( angle == 90 )
	{
		tr.left = org.x + (r->bottom - org.y);
		tr.right = org.x + (r->top - org.y);
		tr.top = org.y + (org.x - r->right);
		tr.bottom = org.y + (org.x - r->left);
		if( tr.left > tr.right )
		{
			int temp = tr.right;
			tr.left = tr.right;
			tr.left = temp;
		}
		if( tr.left > tr.right )
		{
			int temp = tr.right;
			tr.left = tr.right;
			tr.left = temp;
		}
		if( tr.bottom > tr.top )
		{
			int temp = tr.bottom;
			tr.bottom = tr.top;
			tr.top = temp;
		}
	}
	else if( angle > 90 )
	{
		tr = *r;
		for( int i=0; i<(angle/90); i++ )
			RotateRect( &tr, 90, org );
	}
	*r = tr;
}

// test for hit on line segment
// i.e. cursor within a given distance from segment
// enter with:	x,y = cursor coords
//				(xi,yi) and (xf,yf) are the end-points of the line segment
//				dist = maximum distance for hit
//
int TestLineHit( int xi, int yi, int xf, int yf, int x, int y, double dist )
{
	double dd;

	// test for vertical or horizontal segment
	if( xf==xi )
	{
		// vertical segment
		dd = fabs( (double)(x-xi) );
		if( dd<dist && ( (yf>yi && y<yf && y>yi) || (yf<yi && y>yf && y<yi) ) )
			return 1;
	}
	else if( yf==yi )
	{
		// horizontal segment
		dd = fabs( (double)(y-yi) );
		if( dd<dist && ( (xf>xi && x<xf && x>xi) || (xf<xi && x>xf && x<xi) ) )
			return 1;
	}
	else
	{
		// oblique segment
		// find a,b such that (xi,yi) and (xf,yf) lie on y = a + bx
		double b = (double)(yf-yi)/(xf-xi);
		double a = (double)yi-b*xi;
		// find c,d such that (x,y) lies on y = c + dx where d=(-1/b)
		double d = -1.0/b;
		double c = (double)y-d*x;
		// find nearest point to (x,y) on line segment (xi,yi) to (xf,yf)
		double xp = (a-c)/(d-b);
		double yp = a + b*xp;
		// find distance
		dd = sqrt((x-xp)*(x-xp)+(y-yp)*(y-yp));
		if( fabs(b)>0.7 )
		{
			// line segment more vertical than horizontal
			if( dd<dist && ( (yf>yi && yp<yf && yp>yi) || (yf<yi && yp>yf && yp<yi) ) )
				return 1;
		}
		else
		{
			// line segment more horizontal than vertical
			if( dd<dist && ( (xf>xi && xp<xf && xp>xi) || (xf<xi && xp>xf && xp<xi) ) )
				return 1;
		}
	}	
	return 0;	// no hit
}

// function to read font file
// format is form "default.fnt" file from UnixPCB
//
// enter with:	fn = filename
//							
// return pointer to struct font, or 0 if error
//
int ReadFontFile( char * fn )
{
	return 0;
}

// safer version of strtok which buffers input string
// limited to strings < 256 characters
// returns pointer to substring if delimiter found
// returns 0 if delimiter not found
//
char * mystrtok( LPCTSTR str, LPCTSTR delim )
{
	static char s[256] = "";
	static int pos = 0;
	static int len = 0;
	int delim_len = strlen( delim ); 

	if( str )
	{
		len = strlen(str);
		if( len > 255 || len == 0 || delim_len == 0 )
		{
			len = 0;
			return 0;
		}
		strcpy( s, str );
		pos = 0;
	}
	else if( len == 0 )
		return 0;

	// now find delimiter, starting from pos
	int i = pos;
	while( i<=len )
	{
		for( int id=0; id<delim_len; id++ )
		{
			if( s[i] == delim[id] || s[i] == 0 )
			{
				// found delimiter, update pos and return
				int old_pos = pos;
				pos = i+1;
				s[i] = 0;
				return &s[old_pos]; 
			}
		}
		i++;
	}
	return 0;
}

// function to get dimension in PCB units from string
// string format:	nnn.nnMIL for mils
//					nnn.nnMM for mm.
//					nnnnnnNM for nm.
//					nnnn for default units
// if bRound10 = TRUE, round return value to nearest 10 nm.
// returns the dimension in nanometers, or 0.0 if error
//
double GetDimensionFromString( CString * str, int def_units, BOOL bRound10 )
{
	double dim;
	int mult;

	if( def_units == MM )
		mult = NM_PER_MM;
	else if( def_units == MIL )
		mult = NM_PER_MIL;
	else if( def_units == NM )
		mult = 1;

	int len = str->GetLength();
	if( len > 2 )
	{
		if( str->Right(2) == "MM" )
			mult = NM_PER_MM;
		else if( str->Right(3) == "MIL" )
			mult = NM_PER_MIL;
		else if( str->Right(2) == "NM" )
			mult = 1;
	}
	dim = mult*atof( (LPCSTR)str->GetBuffer() );
	if( bRound10 )
	{
		long ldim;
		if( dim >= 0.0 )
			ldim = dim + 5.0;
		else
			ldim = dim - 5.0;
		ldim = ldim/10 * 10;
		dim = ldim;
	}
	return dim;
}

// function to make string from dimension in NM, using requested units
// if append_units == TRUE, add unit string, like "10MIL"
// if lower_case == TRUE, use lower case for units, like "10mil"
// if space == TRUE, insert space, like "10 mil"
// max_dp is the maximum number of decimal places to include in string
// if strip = TRUE, strip trailing zeros if decimal point
//
void MakeCStringFromDimension( CString * str, int dim, int units, BOOL append_units, 
							  BOOL lower_case, BOOL space, int max_dp, BOOL strip )
{
	CString f_str;
	f_str.Format( "11.%df", max_dp );
	f_str = "%" + f_str;
	if( units == MM )
		str->Format( f_str, (double)dim/1000000.0 );
	else if( units == MIL )
		str->Format( f_str, (double)dim/NM_PER_MIL );
	else if( units == NM )
		str->Format( "%d", dim );
	else
		ASSERT(0);
	str->Trim();

	// look for decimal point
	str->Trim();
	int dp_pos = str->Find( "." );

	// if decimal point, strip trailing zeros from MIL and MM strings
	if( dp_pos != -1 && strip )
	{
		while(1 )
		{
			if( str->Right(1) == "0" )
				*str = str->Left( str->GetLength() - 1 );
			else if( str->Right(1) == "." )
			{
				*str = str->Left( str->GetLength() - 1 );
				break;
			}
			else
				break;
		}
	}

	// append units if requested
	if( append_units )
	{
		if( units == MM && space == FALSE )
			*str = *str + "MM";
		else if( units == MM && space == TRUE )
			*str = *str + " MM";
		else if( units == MIL && space == FALSE )
			*str = *str + "MIL";
		else if( units == MIL && space == TRUE )
			*str = *str + " MIL";
		else if( units == NM && space == FALSE )
			*str = *str + "NM";
		else if( units == MIL && space == TRUE )
			*str = *str + " NM";
		if( lower_case )
			str->MakeLower();
	}
}

// function to make a CString from a double, stripping trailing zeros and "."
// allows maximum of 4 decimal places
//
void MakeCStringFromDouble( CString * str, double d )
{
	str->Format( "%12.4f", d );
	while(1 )
	{
		if( str->Right(1) == "0" )
			*str = str->Left( str->GetLength() - 1 );
		else if( str->Right(1) == "." )
		{
			*str = str->Left( str->GetLength() - 1 );
			break;
		}
		else
			break;
	}
	str->Trim();
}

// parse reference designator, such as "R1"
// return numeric suffix, or 0 if none
// set prefix to alphanumeric prefix
//
int ParseRef( CString * ref, CString * prefix )
{
	// get length of numeric suffix
	int ref_length = ref->GetLength();
	int num_length = 0;
	for( int i=ref_length-1; i>=0; i-- )
	{
		if( (*ref)[i] < '0' || (*ref)[i] > '9' )
		{
			break;
		}
		num_length++;
	}
	*prefix = ref->Left( ref_length - num_length );
	if( num_length ==  0 )
		return 0;
	CString num_str = ref->Right( num_length );
	int num = atoi( num_str );
	return num;
}

// test for legal pin name, such as "1", "A4", "SOURCE", but not "1A"
// if astr != NULL, set to alphabetic part
// if nstr != NULL, set to numeric part
// if n != NULL, set to value of numeric part
//
BOOL CheckLegalPinName( CString * pinstr, CString * astr, CString * nstr, int * n )
{
	CString aastr;
	CString nnstr;
	int nn = -1;

	if( *pinstr == "" )
		return FALSE;
	if( -1 != pinstr->FindOneOf( " .,;:/!@#$%^&*(){}[]|<>?\\~\'\"" ) )
		return FALSE;
	int asize = pinstr->FindOneOf( "0123456789" );
	if( asize == -1 )
	{
		// starts with a non-number
		aastr = *pinstr;
	}
	else if( asize == 0 )
	{
		// starts with a number, illegal if any non-numbers
		nnstr = *pinstr;
		for( int ic=0; ic<nnstr.GetLength(); ic++ )
		{
			if( nnstr[ic] < '0' || nnstr[ic] > '9' )
				return FALSE;
		}
		nn = atoi( nnstr );
	}
	else
	{
		// both alpha and numeric parts
		// get alpha substring
		aastr = pinstr->Left( asize );
		int test = aastr.FindOneOf( "0123456789" );
		if( test != -1 )
			return FALSE;	// alpha substring contains a number
		// get numeric substring
		nnstr = pinstr->Right( pinstr->GetLength() - asize );
		CString teststr = nnstr.SpanIncluding( "0123456789" );
		if( teststr != nnstr )
			return FALSE;	// numeric substring contains non-number
		nn = atoi( nnstr );
	}
	if( astr )
		*astr = aastr;
	if( nstr )
		*nstr = nnstr;
	if( n )
		*n = nn;
	return TRUE;
}



// find intersection between y = a + bx and y = c + dx;
//
int FindLineIntersection( double a, double b, double c, double d, double * x, double * y )
{
	*x = (c-a)/(b-d);
	*y = a + b*(*x);
	return 0;
}

// Return the coordinates of the extrapolated intersection between the two lines described
//  by the passed endpoints: [(x1a, y1a),(x2a, y2a)] and [(x1b, y1b), (x2b, y2b)]. 
//	Note that it also works for vertical lines (i.e. infinite slope).
//	Returns: -1 if the lines don't intersect, 
//			  1 if the intersection is on both of the line segments, 
//			  0 if the intersection is beyond the ends of both two segments.
//
int FindLineIntersection(double x0, double y0, double x1, double y1,
						 double x2, double y2, double x3, double y3,
						 double *linx, double *liny)
{
	double d=(x1-x0)*(y3-y2)-(y1-y0)*(x3-x2);
	if (fabs(d)<0.001) return -1;

	double AB=((y0-y2)*(x3-x2)-(x0-x2)*(y3-y2))/d;
	*linx=x0+AB*(x1-x0);
	*liny=y0+AB*(y1-y0);

	if (AB>0.0 && AB<1.0)
	{
		double CD=((y0-y2)*(x1-x0)-(x0-x2)*(y1-y0))/d;
		if (CD>0.0 && CD<1.0) return 1;
    }
	return 0;
}


// set EllipseKH struct to describe the ellipse for an arc
//
int MakeEllipseFromArc( int xi, int yi, int xf, int yf, int style, EllipseKH * el )
{
	// arc (quadrant of ellipse)
	// convert to clockwise arc
	int xxi, xxf, yyi, yyf;
	if( style == CPolyLine::ARC_CCW )
	{
		xxi = xf;
		xxf = xi;
		yyi = yf;
		yyf = yi;
	}
	else
	{
		xxi = xi;
		xxf = xf;
		yyi = yi;
		yyf = yf;
	}
	// find center and radii of ellipse
	double xo, yo, rx, ry;
	if( xxf > xxi && yyf > yyi )
	{
		xo = xxf;
		yo = yyi;
		el->theta1 = M_PI;
		el->theta2 = M_PI/2.0;
	}
	else if( xxf < xxi && yyf > yyi )
	{
		xo = xxi;
		yo = yyf;
		el->theta1 = -M_PI/2.0;
		el->theta2 = -M_PI;
	}
	else if( xxf < xxi && yyf < yyi )
	{
		xo = xxf;
		yo = yyi;
		el->theta1 = 0.0;
		el->theta2 = -M_PI/2.0;
	}
	else if( xxf > xxi && yyf < yyi )
	{
		xo = xxi;
		yo = yyf;
		el->theta1 = M_PI/2.0;
		el->theta2 = 0.0;
	}
	el->Center.X = xo;
	el->Center.Y = yo;
	el->xrad = abs(xf-xi);
	el->yrad = abs(yf-yi);
#if 0
	el->Phi = 0.0;
	el->MaxRad = el->xrad;
	el->MinRad = el->yrad;
	if( el->MaxRad < el->MinRad )
	{
		el->MaxRad = el->yrad;
		el->MinRad = el->xrad;
		el->Phi = M_PI/2.0;
	}
#endif
	return 0;
}

// find intersections between line segment (xi,yi) to (xf,yf)
// and line segment (xi2,yi2) to (xf2,yf2)
// the line segments may be arcs (i.e. quadrant of an ellipse) or straight
// returns number of intersections found (max of 2)
// returns coords of intersections in arrays x[2], y[2]
//
int FindSegmentIntersections( int xi, int yi, int xf, int yf, int style, 
								 int xi2, int yi2, int xf2, int yf2, int style2,
								 double x[], double y[] )
{
	double xr[12], yr[12];
	int iret = 0;

	if( max(xi,xf) < min(xi2,xf2) 
		|| min(xi,xf) > max(xi2,xf2) 
		|| max(yi,yf) < min(yi2,yf2) 
		|| min(yi,yf) > max(yi2,yf2) )
		return 0;

	if( style != CPolyLine::STRAIGHT && style2 != CPolyLine::STRAIGHT )
	{
		// two identical arcs intersect
		if( style == style2 && xi == xi2 && yi == yi2 && xf == xf2 && yf == yf2 )
		{
			if( x && y )
			{
				x[0] = xi;
				y[0] = yi;
			}
			return 1;
		}
		else if( style != style2 && xi == xf2 && yi == yf2 && xf == xi2 && yf == yi2 )
		{
			if( x && y )
			{
				x[0] = xi;
				y[0] = yi;
			}
			return 1;
		}
	}

	if( style == CPolyLine::STRAIGHT && style2 == CPolyLine::STRAIGHT )
	{
		// both straight-line segments
		int x, y;
		BOOL bYes = TestForIntersectionOfStraightLineSegments( xi, yi, xf, yf, xi2, yi2, xf2, yf2, &x, &y );
		if( !bYes )
			return 0;
		xr[0] = x;
		yr[0] = y;
		iret = 1;
	}
	else if( style == CPolyLine::STRAIGHT )
	{
		// first segment is straight, second segment is an arc
		int ret;
		double x1r, y1r, x2r, y2r;
		if( xf == xi )
		{
			// vertical first segment
			double a = xi;
			double b = DBL_MAX/2.0;
			ret = FindLineSegmentIntersection( a, b, xi2, yi2, xf2, yf2, style2,
				&x1r, &y1r, &x2r, &y2r );
		}
		else
		{
			double b = (double)(yf-yi)/(double)(xf-xi);
			double a = yf - b*xf;
			ret = FindLineSegmentIntersection( a, b, xi2, yi2, xf2, yf2, style2,
				&x1r, &y1r, &x2r, &y2r );
		}
		if( ret == 0 )
			return 0;
		if( InRange( x1r, xi, xf ) && InRange( y1r, yi, yf ) )
		{
			xr[iret] = x1r;
			yr[iret] = y1r;
			iret++;
		}
		if( ret == 2 )
		{
			if( InRange( x2r, xi, xf ) && InRange( y2r, yi, yf ) )
			{
				xr[iret] = x2r;
				yr[iret] = y2r;
				iret++;
			}
		}
	}
	else if( style2 == CPolyLine::STRAIGHT )
	{
		// first segment is an arc, second segment is straight
		int ret;
		double x1r, y1r, x2r, y2r;
		if( xf2 == xi2 )
		{
			// vertical second segment
			double a = xi2;
			double b = DBL_MAX/2.0;
			ret = FindLineSegmentIntersection( a, b, xi, yi, xf, yf, style,
				&x1r, &y1r, &x2r, &y2r );
		}
		else
		{
			double b = (double)(yf2-yi2)/(double)(xf2-xi2);
			double a = yf2 - b*xf2;
			ret = FindLineSegmentIntersection( a, b, xi, yi, xf, yf, style,
				&x1r, &y1r, &x2r, &y2r );
		}
		if( ret == 0 )
			return 0;
		if( InRange( x1r, xi2, xf2 ) && InRange( y1r, yi2, yf2 ) )
		{
			xr[iret] = x1r;
			yr[iret] = y1r;
			iret++;
		}
		if( ret == 2 )
		{
			if( InRange( x2r, xi2, xf2 ) && InRange( y2r, yi2, yf2 ) )
			{
				xr[iret] = x2r;
				yr[iret] = y2r;
				iret++;
			}
		}
	}
	else
	{
		// both segments are arcs
		EllipseKH el1;
		EllipseKH el2;
		Point IntPts[12];
		MakeEllipseFromArc( xi, yi, xf, yf, style, &el1 );
		MakeEllipseFromArc( xi2, yi2, xf2, yf2, style2, &el2 );
		int n;
		if( el1.xrad+el1.yrad > el2.xrad+el2.yrad )
			n = GetArcIntersections( &el1, &el2 );
		else
			n = GetArcIntersections( &el2, &el1 );
		iret = n;
	}
	if( x && y )
	{
		for( int i=0; i<iret; i++ )
		{
			x[i] = xr[i];
			y[i] = yr[i];
		}
	}
	return iret;
}

// find intersection between line y = a + bx and line segment (xi,yi) to (xf,yf)
// if b > DBL_MAX/10, assume vertical line at x = a
// the line segment may be an arc (i.e. quadrant of an ellipse)
// return 0 if no intersection
// returns 1 or 2 if intersections found
// sets coords of intersections in *x1, *y1, *x2, *y2
// if no intersection, returns min distance in dist
//
int FindLineSegmentIntersection( double a, double b, int xi, int yi, int xf, int yf, int style, 
								double * x1, double * y1, double * x2, double * y2,
								double * dist )
{
	double xx, yy;
	BOOL bVert = FALSE;
	if( b > DBL_MAX/10.0 )
		bVert = TRUE;

	if( xf != xi )
	{
		// non-vertical segment, get intersection
		if( style == CPolyLine::STRAIGHT || yf == yi )
		{
			// horizontal or oblique straight segment
			// put into form y = c + dx;
			double d = (double)(yf-yi)/(double)(xf-xi);
			double c = yf - d*xf;
			if( bVert )
			{
				// if vertical line, easy
				if( InRange( a, xi, xf ) )
				{
					*x1 = a;
					*y1 = c + d*a;
					return 1;
				}
				else
				{
					if( dist )
						*dist = min( abs(a-xi), abs(a-xf) );
					return 0;
				}
			}
			if( fabs(b-d) < 1E-12 )
			{
				// parallel lines
				if( dist )
				{
					*dist = GetPointToLineDistance( a, b, xi, xf );
				}
				return 0;	// lines parallel
			}
			// calculate intersection
			xx = (c-a)/(b-d);
			yy = a + b*(xx);
			// see if intersection is within the line segment
			if( yf == yi )
			{
				// horizontal line
				if( (xx>=xi && xx>xf) || (xx<=xi && xx<xf) )
					return 0;
			}
			else
			{
				// oblique line
				if( (xx>=xi && xx>xf) || (xx<=xi && xx<xf) 
					|| (yy>yi && yy>yf) || (yy<yi && yy<yf) )
					return 0;
			}
		}
		else if( style == CPolyLine::ARC_CW || style == CPolyLine::ARC_CCW )
		{
			// arc (quadrant of ellipse)
			// convert to clockwise arc
			int xxi, xxf, yyi, yyf;
			if( style == CPolyLine::ARC_CCW )
			{
				xxi = xf;
				xxf = xi;
				yyi = yf;
				yyf = yi;
			}
			else
			{
				xxi = xi;
				xxf = xf;
				yyi = yi;
				yyf = yf;
			}
			// find center and radii of ellipse
			double xo, yo, rx, ry;
			if( xxf > xxi && yyf > yyi )
			{
				xo = xxf;
				yo = yyi;
			}
			else if( xxf < xxi && yyf > yyi )
			{
				xo = xxi;
				yo = yyf;
			}
			else if( xxf < xxi && yyf < yyi )
			{
				xo = xxf;
				yo = yyi;
			}
			else if( xxf > xxi && yyf < yyi )
			{
				xo = xxi;
				yo = yyf;
			}
			rx = fabs( (double)(xxi-xxf) );
			ry = fabs( (double)(yyi-yyf) );
			BOOL test;
			double xx1, xx2, yy1, yy2, aa;
			if( bVert )
			{
				// shift vertical line to coordinate system of ellipse
				aa = a - xo;
				test = FindVerticalLineEllipseIntersections( rx, ry, aa, &yy1, &yy2 );
				if( !test )
					return 0;
				// shift back to PCB coordinates
				yy1 += yo;
				yy2 += yo;
				xx1 = a;
				xx2 = a;
			}
			else
			{
				// shift line to coordinate system of ellipse
				aa = a + b*xo - yo;
				test = FindLineEllipseIntersections( rx, ry, aa, b, &xx1, &xx2 );
				if( !test )
					return 0;
				// shift back to PCB coordinates
				yy1 = aa + b*xx1;
				xx1 += xo;
				yy1 += yo;
				yy2 = aa + b*xx2;
				xx2 += xo;
				yy2 += yo;
			}
			int npts = 0;
			if( (xxf>xxi && xx1<xxf && xx1>xxi) || (xxf<xxi && xx1<xxi && xx1>xxf) )
			{
				if( (yyf>yyi && yy1<yyf && yy1>yyi) || (yyf<yyi && yy1<yyi && yy1>yyf) )
				{
					*x1 = xx1;
					*y1 = yy1;
					npts = 1;
				}
			}
			if( (xxf>xxi && xx2<xxf && xx2>xxi) || (xxf<xxi && xx2<xxi && xx2>xxf) )
			{
				if( (yyf>yyi && yy2<yyf && yy2>yyi) || (yyf<yyi && yy2<yyi && yy2>yyf) )
				{
					if( npts == 0 )
					{
						*x1 = xx2;
						*y1 = yy2;
						npts = 1;
					}
					else
					{
						*x2 = xx2;
						*y2 = yy2;
						npts = 2;
					}
				}
			}
			return npts;
		}
		else
			ASSERT(0);
	}
	else
	{
		// vertical line segment
		if( bVert )
			return 0;
		xx = xi;
		yy = a + b*xx;
		if( (yy>=yi && yy>yf) || (yy<=yi && yy<yf) )
			return 0;
	}
	*x1 = xx;
	*y1 = yy;
	return 1;
}

// Test for intersection of line segments
// If lines are parallel, returns FALSE
// If TRUE, returns intersection coords in x, y
// if FALSE, returns min. distance in dist (may be 0.0 if parallel)
// and coords on nearest point in one of the segments in (x,y)
//
BOOL TestForIntersectionOfStraightLineSegments( int x1i, int y1i, int x1f, int y1f, 
									   int x2i, int y2i, int x2f, int y2f,
									   int * x, int * y, double * d )
{
	double a, b, dist;

	// first, test for intersection
	if( x1i == x1f && x2i == x2f )
	{
		// both segments are vertical, can't intersect
	}
	else if( y1i == y1f && y2i == y2f )
	{
		// both segments are horizontal, can't intersect
	}
	else if( x1i == x1f && y2i == y2f )
	{
		// first seg. vertical, second horizontal, see if they cross
		if( InRange( x1i, x2i, x2f )
			&& InRange( y2i, y1i, y1f ) )
		{
			if( x )
				*x = x1i;
			if( y )
				*y = y2i;
			if( d )
				*d = 0.0;
			return TRUE;
		}
	}
	else if( y1i == y1f && x2i == x2f )
	{
		// first seg. horizontal, second vertical, see if they cross
		if( InRange( y1i, y2i, y2f )
			&& InRange( x2i, x1i, x1f ) )
		{
			if( x )
				*x = x2i;
			if( y )
				*y = y1i;
			if( d )
				*d = 0.0;
			return TRUE;
		}
	}
	else if( x1i == x1f )
	{
		// first segment vertical, second oblique
		// get a and b for second line segment, so that y = a + bx;
		b = (double)(y2f-y2i)/(x2f-x2i);
		a = (double)y2i - b*x2i;
		double x1, y1, x2, y2;
		int test = FindLineSegmentIntersection( a, b, x1i, y1i, x1f, y1f, CPolyLine::STRAIGHT,
			&x1, &y1, &x2, &y2 );
		if( test )
		{
			if( InRange( y1, y1i, y1f ) && InRange( x1, x2i, x2f ) && InRange( y1, y2i, y2f ) )
			{
				if( x )
					*x = x1;
				if( y )
					*y = y1;
				if( d )
					*d = 0.0;
				return TRUE;
			}
		}
	}
	else if( y1i == y1f )
	{
		// first segment horizontal, second oblique
		// get a and b for second line segment, so that y = a + bx;
		b = (double)(y2f-y2i)/(x2f-x2i);
		a = (double)y2i - b*x2i;
		double x1, y1, x2, y2;
		int test = FindLineSegmentIntersection( a, b, x1i, y1i, x1f, y1f, CPolyLine::STRAIGHT,
			&x1, &y1, &x2, &y2 );
		if( test )
		{
			if( InRange( x1, x1i, x1f ) && InRange( x1, x2i, x2f ) && InRange( y1, y2i, y2f ) )
			{
				if( x )
					*x = x1;
				if( y )
					*y = y1;
				if( d )
					*d = 0.0;
				return TRUE;
			}
		}
	}
	else if( x2i == x2f )
	{
		// second segment vertical, first oblique
		// get a and b for first line segment, so that y = a + bx;
		b = (double)(y1f-y1i)/(x1f-x1i);
		a = (double)y1i - b*x1i;
		double x1, y1, x2, y2;
		int test = FindLineSegmentIntersection( a, b, x2i, y2i, x2f, y2f, CPolyLine::STRAIGHT,
			&x1, &y1, &x2, &y2 );
		if( test )
		{
			if( InRange( x1, x1i, x1f ) &&  InRange( y1, y1i, y1f ) && InRange( y1, y2i, y2f ) )
			{
				if( x )
					*x = x1;
				if( y )
					*y = y1;
				if( d )
					*d = 0.0;
				return TRUE;
			}
		}
	}
	else if( y2i == y2f )
	{
		// second segment horizontal, first oblique
		// get a and b for second line segment, so that y = a + bx;
		b = (double)(y1f-y1i)/(x1f-x1i);
		a = (double)y1i - b*x1i;
		double x1, y1, x2, y2;
		int test = FindLineSegmentIntersection( a, b, x2i, y2i, x2f, y2f, CPolyLine::STRAIGHT,
			&x1, &y1, &x2, &y2 );
		if( test )
		{
			if( InRange( x1, x1i, x1f ) && InRange( y1, y1i, y1f ) )
			{
				if( x )
					*x = x1;
				if( y )
					*y = y1;
				if( d )
					*d = 0.0;
				return TRUE;
			}
		}
	}
	else
	{
		// both segments oblique
		if( (long)(y1f-y1i)*(x2f-x2i) != (long)(y2f-y2i)*(x1f-x1i) )
		{
			// not parallel, get a and b for first line segment, so that y = a + bx;
			b = (double)(y1f-y1i)/(x1f-x1i);
			a = (double)y1i - b*x1i;
			double x1, y1, x2, y2;
			int test = FindLineSegmentIntersection( a, b, x2i, y2i, x2f, y2f, CPolyLine::STRAIGHT,
				&x1, &y1, &x2, &y2 );
			// both segments oblique
			if( test )
			{
				if( InRange( x1, x1i, x1f ) && InRange( y1, y1i, y1f ) )
				{
					if( x )
						*x = x1;
					if( y )
						*y = y1;
					if( d )
						*d = 0.0;
					return TRUE;
				}
			}
		}
	}
	// don't intersect, get shortest distance between each endpoint and the other line segment
	dist = GetPointToLineSegmentDistance( x1i, y1i, x2i, y2i, x2f, y2f );
	double xx = x1i;
	double yy = y1i;
	double dd = GetPointToLineSegmentDistance( x1f, y1f, x2i, y2i, x2f, y2f );
	if( dd < dist )
	{
		dist = dd;
		xx = x1f;
		yy = y1f;
	}
	dd = GetPointToLineSegmentDistance( x2i, y2i, x1i, y1i, x1f, y1f );
	if( dd < dist )
	{
		dist = dd;
		xx = x2i;
		yy = y2i;
	}
	dd = GetPointToLineSegmentDistance( x2f, y2f, x1i, y1i, x1f, y1f );
	if( dd < dist )
	{
		dist = dd;
		xx = x2f;
		yy = y2f;
	}
	if( x )
		*x = xx;
	if( y )
		*y = yy;
	if( d )
		*d = dist;
	return FALSE;
}

// these functions are for profiling
//
void DunselFunction() { return; }

void CalibrateTimer()
{
	void (*pFunc)() = DunselFunction;

	if( QueryPerformanceFrequency( &PerfFreq ) )
	{
		// use hires timer
		PerfFreqAdjust = 0;
		int High32 = PerfFreq.HighPart;
		int Low32 = PerfFreq.LowPart;
		while( High32 )
		{
			High32 >>= 1;
			PerfFreqAdjust++;
		}
		return;
	}
	else
		ASSERT(0);
}

void StartTimer()
{
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );
	QueryPerformanceCounter( &tStart );
}

double GetElapsedTime()
{
	QueryPerformanceCounter( &tStop );
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL );

	double time;
	int ReduceMag = 0;
	LARGE_INTEGER Freq = PerfFreq;
	unsigned int High32 = tStop.HighPart - tStart.HighPart;
	while( High32 )
	{
		High32 >>= 1;
		ReduceMag++;
	}
	if( PerfFreqAdjust || ReduceMag )
	{
		if( PerfFreqAdjust > ReduceMag )
			ReduceMag = PerfFreqAdjust;
		tStart.QuadPart = Int64ShrlMod32( tStart.QuadPart, ReduceMag );
		tStop.QuadPart = Int64ShrlMod32( tStop.QuadPart, ReduceMag );
		Freq.QuadPart = Int64ShrlMod32( Freq.QuadPart, ReduceMag );
	}
	if( Freq.LowPart == 0 )
		time = 0.0;
	else
		time = ((double)(tStop.LowPart - tStart.LowPart))/Freq.LowPart;
	return time;
}

// quicksort algorithm
// sorts array numbers[], also moves elements of another array index[]
//
#define Q3WAY
void quickSort(int numbers[], int index[], int array_size)
{
#ifdef Q3WAY
  q_sort_3way(numbers, index, 0, array_size - 1);
#else
  q_sort(numbers, index, 0, array_size - 1);
#endif
}

// standard quicksort
//
void q_sort(int numbers[], int index[], int left, int right)
{
	int pivot, pivot_index, l_hold, r_hold;

	l_hold = left;
	r_hold = right;
	pivot = numbers[left];
	pivot_index = index[left];
	while (left < right)
	{
		while ((numbers[right] >= pivot) && (left < right))
			right--;
		if (left != right)
		{
			numbers[left] = numbers[right];
			index[left] = index[right];
			left++;
		}
		while ((numbers[left] <= pivot) && (left < right))
			left++;
		if (left != right)
		{
			numbers[right] = numbers[left];
			index[right] = index[left];
			right--;
		}
	}
	numbers[left] = pivot;
	index[left] = pivot_index;

	pivot = left;
	left = l_hold;
	right = r_hold;
	if (left < pivot)
		q_sort(numbers, index, left, pivot-1);
	if (right > pivot)
		q_sort(numbers, index, pivot+1, right);
}

// 3-way quicksort...useful where there are duplicate values
//
void q_sort_3way( int a[], int b[], int l, int r )
{
	#define EXCH(i,j) {int temp=a[i]; a[i]=a[j]; a[j]=temp; temp=b[i]; b[i]=b[j]; b[j]=temp;}

	int i = l - 1;
	int j = r;
	int p = l - 1;
	int q = r;
	int v = a[r];

	if( r <= l )
		return;

	for(;;)
	{
		while( a[++i] < v );
		while( v < a[--j] )
			if( j == 1 )
				break;
		if( i >= j )
			break;
		EXCH( i, j );
		if( a[i] == v )
		{
			p++;
			EXCH( p, i );
		}
		if( v == a[j] )
		{
			q--;
			EXCH( j, q );
		}
	}
	EXCH( i, r );
	j = i - 1;
	i = i + 1;
	for( int k=l; k<p; k++, j-- )
		EXCH( k, j );
	for( int k=r-1; k>q; k--, i++ )
		EXCH( i, k );
	q_sort_3way( a, b, l, j );
	q_sort_3way( a, b, i, r );
}

// convert angle in degrees from CW to CCW
//
int ccw( int angle )
{
	return (720-angle)%360;
}

// solves quadratic equation
// i.e.   ax**2 + bx + c = 0
// returns TRUE if solution exist, with solutions in x1 and x2
// else returns FALSE
//
BOOL Quadratic( double a, double b, double c, double *x1, double *x2 )
{
	double root = b*b - 4.0*a*c;
	if( root < 0.0 )
		return FALSE;
	root = sqrt( root );
	*x1 = (-b+root)/(2.0*a);
	*x2 = (-b-root)/(2.0*a);
	return TRUE;
}

// finds intersections of vertical line at x
// with ellipse defined by (x^2)/(a^2) + (y^2)/(b^2) = 1;
// returns TRUE if solution exist, with solutions in y1 and y2
// else returns FALSE
//
BOOL FindVerticalLineEllipseIntersections( double a, double b, double x, double *y1, double *y2 )
{
	double y_sqr = (1.0-(x*x)/(a*a))*b*b;
	if( y_sqr < 0.0 )
		return FALSE;
	*y1 = sqrt(y_sqr);
	*y2 = -*y1;
	return TRUE;
}

// finds intersections of straight line y = c + dx
// with ellipse defined by (x^2)/(a^2) + (y^2)/(b^2) = 1;
// returns TRUE if solution exist, with solutions in x1 and x2
// else returns FALSE
//
BOOL FindLineEllipseIntersections( double a, double b, double c, double d, double *x1, double *x2 )
{
	// quadratic terms
	double A = d*d+b*b/(a*a);
	double B = 2.0*c*d;
	double C = c*c-b*b;
	return Quadratic( A, B, C, x1, x2 );
}

// draw a straight line or an arc between xi,yi and xf,yf
//
void DrawArc( CDC * pDC, int shape, int xxi, int yyi, int xxf, int yyf, BOOL bMeta )
{
	int xi, yi, xf, yf;
	if( shape == DL_LINE || xxi == xxf || yyi == yyf )
	{
		// draw straight line
		pDC->MoveTo( xxi, yyi );
		pDC->LineTo( xxf, yyf );
	}
	else if( shape == DL_ARC_CCW || shape == DL_ARC_CW ) 
	{
		// set endpoints so we can always draw counter-clockwise arc
		if( shape == DL_ARC_CW )
		{
			xi = xxf;
			yi = yyf;
			xf = xxi; 
			yf = yyi;
		}
		else
		{
			xi = xxi;
			yi = yyi;
			xf = xxf;
			yf = yyf;
		}
		pDC->MoveTo( xi, yi );
		if( xf > xi && yf > yi )
		{
			// quadrant 1
			int w = (xf-xi)*2;
			int h = (yf-yi)*2;
			if( !bMeta )
				pDC->Arc( xf-w, yi+h, xf, yi,
					xi, yi, xf, yf );
			else
				pDC->Arc( xf-w, yi, xf, yi+h,
					xf, yf, xi, yi );
		}
		else if( xf < xi && yf > yi )
		{
			// quadrant 2
			int w = -(xf-xi)*2;
			int h = (yf-yi)*2;
			if( !bMeta )
				pDC->Arc( xi-w, yf, xi, yf-h,
					xi, yi, xf, yf );
			else
				pDC->Arc( xi-w, yf-h, xi, yf,
					xf, yf, xi, yi );
		}
		else if( xf < xi && yf < yi )
		{
			// quadrant 3
			int w = -(xf-xi)*2;
			int h = -(yf-yi)*2;
			if( !bMeta )
				pDC->Arc( xf, yi, xf+w, yi-h,
					xi, yi, xf, yf ); 
			else
				pDC->Arc( xf, yi-h, xf+w, yi,
					xf, yf, xi, yi );
		}
		else if( xf > xi && yf < yi )
		{
			// quadrant 4
			int w = (xf-xi)*2;
			int h = -(yf-yi)*2;
			if( !bMeta )
				pDC->Arc( xi, yf+h, xi+w, yf,
					xi, yi, xf, yf );
			else
				pDC->Arc( xi, yf, xi+w, yf+h,
					xf, yf, xi, yi );
		}
		pDC->MoveTo( xxf, yyf );
	}
	else
		ASSERT(0);	// oops
}

// Get arrays of circles, rects and line segments to represent pad
// for purposes of drawing pad or calculating clearances
// margins of circles and line segments represent pad outline
// circles and rects are used to find points inside pad
//
void GetPadElements( int type, int x, int y, int wid, int len, int radius, int angle,
					int * nr, my_rect r[], int * nc, my_circle c[], int * ns, my_seg s[] )
{
	*nc = 0;
	*nr = 0;
	*ns = 0;
	if( type == PAD_ROUND )
	{
		*nc = 1;
		c[0] = my_circle(x,y,wid/2);
		return;
	}
	if( type == PAD_SQUARE )
	{
		*nr = 1;
		r[0] = my_rect(x-wid/2, y-wid/2,x+wid/2, y+wid/2);
		*ns = 4;
		s[0] = my_seg(x-wid/2, y+wid/2,x+wid/2, y+wid/2);	// top
		s[1] = my_seg(x-wid/2, y-wid/2,x+wid/2, y-wid/2);	// bottom
		s[2] = my_seg(x-wid/2, y-wid/2,x-wid/2, y+wid/2);	// left
		s[3] = my_seg(x+wid/2, y-wid/2,x+wid/2, y+wid/2);	// right
		return;
	}
	if( type == PAD_OCTAGON )
	{
		const double pi = 3.14159265359;
		*nc = 1;	// circle represents inside of polygon
		c[0] = my_circle(x, y, wid/2);
		*ns = 8;	// now create sides of polygon
		double theta = pi/8.0;
		double radius = 0.5*(double)wid/cos(theta);
		double last_x = x + radius*cos(theta);
		double last_y = y + radius*sin(theta);
		for( int is=0; is<8; is++ )
		{
			theta += pi/4.0;
			double dx = x + radius*cos(theta);
			double dy = y + radius*sin(theta);
			s[is] = my_seg(last_x, last_y, x, y);
			last_x = dx;
			last_y = dy;
		}
		return;
	}
	// 
	int h;
	int v;
	if( angle == 90 || angle == 270 )
	{
		h = wid;
		v = len;
	}
	else
	{
		v = wid;
		h = len;
	}
	if( type == PAD_RECT )
	{
		*nr = 1;
		r[0] = my_rect(x-h/2, y-v/2, x+h/2, y+v/2);
		*ns = 4;
		s[0] = my_seg(x-h/2, y+v/2,x+h/2, y+v/2);	// top
		s[1] = my_seg(x-h/2, y-v/2,x+h/2, y-v/2);	// bottom
		s[2] = my_seg(x-h/2, y-v/2,x-h/2, y+v/2);	// left
		s[3] = my_seg(x+h/2, y-v/2,x+h/2, y+v/2);	// right
		return;
	}
	if( type == PAD_RRECT )
	{
		*nc = 4;
		c[0] = my_circle(x-h/2+radius, y-v/2+radius, radius);	// bottom left circle
		c[1] = my_circle(x+h/2-radius, y-v/2+radius, radius);	// bottom right circle
		c[2] = my_circle(x-h/2+radius, y+v/2-radius, radius);	// top left circle
		c[3] = my_circle(x+h/2-radius, y+v/2-radius, radius);	// top right circle
		*ns = 4;
		s[0] = my_seg(x-h/2+radius, y+v/2, x+h/2-radius, y+v/2);	// top
		s[1] = my_seg(x-h/2+radius, y-v/2, x+h/2-radius, y+v/2);	// bottom
		s[2] = my_seg(x-h/2, y-v/2+radius, x-h/2, y+v/2-radius);	// left
		s[3] = my_seg(x+h/2, y-v/2+radius, x+h/2, y+v/2-radius);	// right
		return;
	}
	if( type == PAD_OVAL )
	{
		if( h > v )
		{
			// horizontal
			*nc = 2;
			c[0] = my_circle(x-h/2+v/2, y, v/2);	// left circle
			c[1] = my_circle(x+h/2-v/2, y, v/2);	// right circle
			*nr = 1;
			r[0] = my_rect(x-h/2+v/2, y-v/2, x+h/2-v/2, y+v/2);
			*ns = 2;
			s[0] = my_seg(x-h/2+v/2, y+v/2, x+h/2-v/2, y+v/2);	// top
			s[1] = my_seg(x-h/2+v/2, y-v/2, x+h/2-v/2, y-v/2);	// bottom
		}
		else
		{
			// vertical
			*nc = 2;
			c[0] = my_circle(x, y+v/2-h/2, h/2);	// top circle
			c[1] = my_circle(x, y-v/2+h/2, h/2);	// bottom circle
			*nr = 1;
			r[0] = my_rect(x-h/2, y-v/2+h/2, x+h/2, y+v/2-h/2);
			*ns = 2;
			s[0] = my_seg(x-h/2, y-v/2+h/2, x-h/2, y+v/2-h/2);	// left
			s[1] = my_seg(x+h/2, y-v/2+h/2, x+h/2, y+v/2-h/2);	// left
		}
		return;
	}
	ASSERT(0);
}

// Find distance from a staright line segment to a pad
//
int GetClearanceBetweenSegmentAndPad( int x1, int y1, int x2, int y2, int w,
								  int type, int x, int y, int wid, int len, int radius, int angle )
{
	if( type == PAD_NONE )
		return INT_MAX;
	else
	{
		int nc, nr, ns;
		my_circle c[4];
		my_rect r[2];
		my_seg s[8];
		GetPadElements( type, x, y, wid, len, radius, angle,
						&nr, r, &nc, c, &ns, s );
		// first test for endpoints of line segment in rectangle
		for( int ir=0; ir<nr; ir++ )
		{
			if( x1 >= r[ir].xlo && x1 <= r[ir].xhi && y1 >= r[ir].ylo && y1 <= r[ir].yhi )
				return 0;
			if( x2 >= r[ir].xlo && x2 <= r[ir].xhi && y2 >= r[ir].ylo && y2 <= r[ir].yhi )
				return 0;
		}
		// now get distance from elements of pad outline
		int dist = INT_MAX;
		for( int ic=0; ic<nc; ic++ )
		{
			int d = GetPointToLineSegmentDistance( c[ic].x, c[ic].y, x1, y1, x2, y2 ) - c[ic].r - w/2;
			dist = min(dist,d);
		}
		for( int is=0; is<ns; is++ )
		{
			double d;
			TestForIntersectionOfStraightLineSegments( s[is].xi, s[is].yi, s[is].xf, s[is].yf,
					x1, y1, x2, y2, NULL, NULL, &d );
			d -= w/2;
			dist = min(dist,d);
		}
		return max(0,dist);
	}
}

// Get clearance between 2 segments
// Returns point in segment closest to other segment in x, y
// in clearance > max_cl, just returns max_cl and doesn't return x,y
//
int GetClearanceBetweenSegments( int x1i, int y1i, int x1f, int y1f, int style1, int w1,
								   int x2i, int y2i, int x2f, int y2f, int style2, int w2,
								   int max_cl, int * x, int * y )
{
	// check clearance between bounding rectangles
	int test = max_cl + w1/2 + w2/2;
	if( min(x1i,x1f)-max(x2i,x2f) > test )
		return max_cl;
	if( min(x2i,x2f)-max(x1i,x1f) > test )
		return max_cl;
	if( min(y1i,y1f)-max(y2i,y2f) > test )
		return max_cl;
	if( min(y2i,y2f)-max(y1i,y1f) > test )
		return max_cl;

	if( style1 == CPolyLine::STRAIGHT && style1 == CPolyLine::STRAIGHT )
	{
		// both segments are straight lines
		int xx, yy;
		double dd;
		TestForIntersectionOfStraightLineSegments( x1i, y1i, x1f, y1f, 
			x2i, y2i, x2f, y2f, &xx, &yy, &dd );
		int d = max( 0, dd - w1/2 - w2/2 );
		if( x )
			*x = xx;
		if( y )
			*y = yy;
		return d;
	}

	// not both straight-line segments
	// see if segments intersect
	double xr[2];
	double yr[2];
	test = FindSegmentIntersections( x1i, y1i, x1f, y1f, style1, x2i, y2i, x2f, y2f, style2, xr, yr );
	if( test ) 
	{
		if( x )
			*x = xr[0];
		if( y )
			*y = yr[0];
		return 0.0;
	}

	// at least one segment is an arc
	EllipseKH el1;
	EllipseKH el2;
	BOOL bArcs;
	int xi, yi, xf, yf;
	if( style2 == CPolyLine::STRAIGHT )
	{
		// style1 = arc, style2 = straight
		MakeEllipseFromArc( x1i, y1i, x1f, y1f, style1, &el1 );
		xi = x2i;
		yi = y2i;
		xf = x2f;
		yf = y2f;
		bArcs = FALSE;
	}
	else if( style1 == CPolyLine::STRAIGHT )
	{
		// style2 = arc, style1 = straight
		xi = x1i;
		yi = y1i;
		xf = x1f;
		yf = y1f;
		MakeEllipseFromArc( x2i, y2i, x2f, y2f, style2, &el1 );
		bArcs = FALSE;
	}
	else
	{
		// style1 = arc, style2 = arc
		MakeEllipseFromArc( x1i, y1i, x1f, y1f, style1, &el1 );
		MakeEllipseFromArc( x2i, y2i, x2f, y2f, style2, &el2 );
		bArcs = TRUE;
	}
	const int NSTEPS = 32;

	if( el1.theta2 > el1.theta1 )
		ASSERT(0);
	if( bArcs && el2.theta2 > el2.theta1 )
		ASSERT(0);

	// test multiple points in both segments
	double th1;
	double th2;
	double len2;
	if( bArcs )
	{
		th1 = el2.theta1;
		th2 = el2.theta2;
		len2 = max(el2.xrad, el2.yrad);
	}
	else
	{
		th1 = 1.0;
		th2 = 0.0;
		len2 = abs(xf-xi)+abs(yf-yi);
	}
	double s_start = el1.theta1;
	double s_end = el1.theta2;
	double s_start2 = th1;
	double s_end2 = th2;
	double dmin = Distance( x1i, y1i, x2i, y2i );;
	double xmin = x1i;
	double ymin = y1i;
	double smin = DBL_MAX, smin2 = DBL_MAX;

	int nsteps = NSTEPS;
	int nsteps2 = NSTEPS;
	double step = (s_start-s_end)/(nsteps-1);
	double step2 = (s_start2-s_end2)/(nsteps2-1);
	while( (step * max(el1.xrad, el1.yrad)) > 0.1*NM_PER_MIL 
		&& (step2 * len2) > 0.1*NM_PER_MIL )
	{
		step = (s_start-s_end)/(nsteps-1);
		for( int i=0; i<nsteps; i++ )
		{
			double s;
			if( i < nsteps-1 )
				s = s_start - i*step;
			else
				s = s_end;
			double x = el1.Center.X + el1.xrad*cos(s);
			double y = el1.Center.Y + el1.yrad*sin(s);
			// if not an arc, use s2 as fractional distance along line
			step2 = (s_start2-s_end2)/(nsteps2-1);
			for( int i2=0; i2<nsteps2; i2++ )
			{
				double s2;
				if( i2 < nsteps2-1 )
					s2 = s_start2 - i2*step2;
				else
					s2 = s_end2;
				double x2, y2;
				if( !bArcs )
				{
					x2 = xi + (xf-xi)*s2;
					y2 = yi + (yf-yi)*s2;
				}
				else
				{
					x2 = el2.Center.X + el2.xrad*cos(s2);
					y2 = el2.Center.Y + el2.yrad*sin(s2);
				}
				double d = Distance( x, y, x2, y2 );
				if( d < dmin )
				{
					dmin = d;
					xmin = x;
					ymin = y;
					smin = s;
					smin2 = s2;
				}
			}
		}
		if( step > step2 )
		{
			s_start = min(el1.theta1, smin + step);
			s_end = max(el1.theta2, smin - step);
			step = (s_start - s_end)/nsteps;
		}
		else
		{
			s_start2 = min(th1, smin2 + step2);
			s_end2 = max(th2, smin2 - step2);
			step2 = (s_start2 - s_end2)/nsteps2;
		}
	}
	if( x )
		*x = xmin;
	if( y )
		*y = ymin;
	return max(0,dmin-w1/2-w2/2);	// allow for widths
}



// Find clearance between pads
// For each pad:
//	type = PAD_ROUND, PAD_SQUARE, etc.
//	x, y = center position
//	w, l = width and length
//  r = corner radius
//	angle = 0 or 90 (if 0, pad length is along x-axis)
//
int GetClearanceBetweenPads( int type1, int x1, int y1, int w1, int l1, int r1, int angle1,
							 int type2, int x2, int y2, int w2, int l2, int r2, int angle2 )
{
	if( type1 == PAD_NONE )
		return INT_MAX;
	if( type2 == PAD_NONE )
		return INT_MAX;

	int dist = INT_MAX;
	int nr, nc, ns, nrr, ncc, nss;
	my_rect r[2], rr[2];
	my_circle c[4], cc[4];
	my_seg s[8], ss[8];

	GetPadElements( type1, x1, y1, w1, l1, r1, angle1,
					&nr, r, &nc, c, &ns, s );
	GetPadElements( type2, x2, y2, w2, l2, r2, angle2,
					&nrr, rr, &ncc, cc, &nss, ss );
	// now find distance from every element of pad1 to every element of pad2
	for( int ic=0; ic<nc; ic++ )
	{
		for( int icc=0; icc<ncc; icc++ )
		{
			int d = Distance( c[ic].x, c[ic].y, cc[icc].x, cc[icc].y )
						- c[ic].r - cc[icc].r;
			dist = min(dist,d);
		}
		for( int iss=0; iss<nss; iss++ )
		{
			int d = GetPointToLineSegmentDistance( c[ic].x, c[ic].y, 
						ss[iss].xi, ss[iss].yi, ss[iss].xf, ss[iss].yf ) - c[ic].r;
			dist = min(dist,d);
		}
	}
	for( int is=0; is<ns; is++ )
	{
		for( int icc=0; icc<ncc; icc++ )
		{
			int d = GetPointToLineSegmentDistance( cc[icc].x, cc[icc].y, 
						s[is].xi, s[is].yi, s[is].xf, s[is].yf ) - cc[icc].r;
			dist = min(dist,d);
		}
		for( int iss=0; iss<nss; iss++ )
		{
			double d;
			TestForIntersectionOfStraightLineSegments( s[is].xi, s[is].yi, s[is].xf, s[is].yf,
						ss[iss].xi, ss[iss].yi, ss[iss].xf, ss[iss].yf, NULL, NULL, &d );
			dist = min(dist,d);
		}
	}
	return max(dist,0);
}

// Get min. distance from (x,y) to line y = a + bx
// if b > DBL_MAX/10, assume vertical line at x = a
// returns closest point on line in xp, yp
//
double GetPointToLineDistance( double a, double b, int x, int y, double * xpp, double * ypp )
{
	if( b > DBL_MAX/10 )
	{
		// vertical line
		if( xpp && ypp )
		{
			*xpp = a;
			*ypp = y;
		}
		return abs(a-x);
	}
	// find c,d such that (x,y) lies on y = c + dx where d=(-1/b)
	double d = -1.0/b;
	double c = (double)y-d*x;
	// find nearest point to (x,y) on line through (xi,yi) to (xf,yf)
	double xp = (a-c)/(d-b);
	double yp = a + b*xp;
	if( xpp && ypp )
	{
		*xpp = xp;
		*ypp = yp;
	}
	// find distance
	return Distance( x, y, xp, yp );
}

// Get distance between line segment and point
// enter with:	x,y = point
//				(xi,yi) and (xf,yf) are the end-points of the line segment
//
double GetPointToLineSegmentDistance( int x, int y, int xi, int yi, int xf, int yf )
{
	// test for vertical or horizontal segment
	if( xf==xi )
	{
		// vertical line segment
		if( InRange( y, yi, yf ) )
			return abs( x - xi );
		else
			return min( Distance( x, y, xi, yi ), Distance( x, y, xf, yf ) );
	}
	else if( yf==yi )
	{
		// horizontal line segment
		if( InRange( x, xi, xf ) )
			return abs( y - yi );
		else
			return min( Distance( x, y, xi, yi ), Distance( x, y, xf, yf ) );
	}
	else
	{
		// oblique segment
		// find a,b such that (xi,yi) and (xf,yf) lie on y = a + bx
		double b = (double)(yf-yi)/(xf-xi);
		double a = (double)yi-b*xi;
		// find c,d such that (x,y) lies on y = c + dx where d=(-1/b)
		double d = -1.0/b;
		double c = (double)y-d*x;
		// find nearest point to (x,y) on line through (xi,yi) to (xf,yf)
		double xp = (a-c)/(d-b);
		double yp = a + b*xp;
		// find distance
		if( InRange( xp, xi, xf ) && InRange( yp, yi, yf ) )
			return Distance( x, y, xp, yp );
		else
			return min( Distance( x, y, xi, yi ), Distance( x, y, xf, yf ) );
	}
}

// test for value within range
//
BOOL InRange( double x, double xi, double xf )
{
	if( xf>xi )
	{
		if( x >= xi && x <= xf )
			return TRUE;
	}
	else
	{
		if( x >= xf && x <= xi )
			return TRUE;
	}
	return FALSE;
}

// Get distance between 2 points
//
double Distance( int x1, int y1, int x2, int y2 )
{
	double d;
	d = sqrt( (double)(x1-x2)*(x1-x2) + (double)(y1-y2)*(y1-y2) );
	if( d > INT_MAX || d < INT_MIN )
		ASSERT(0);
	return (int)d;
}

// this finds approximate solutions
// note: this works best if el2 is smaller than el1
//
int GetArcIntersections( EllipseKH * el1, EllipseKH * el2, 
						double * x1, double * y1, double * x2, double * y2 )						
{
	if( el1->theta2 > el1->theta1 )
		ASSERT(0);
	if( el2->theta2 > el2->theta1 )
		ASSERT(0);

	const int NSTEPS = 32;
	double xret[2], yret[2];

	double xscale = 1.0/el1->xrad;
	double yscale = 1.0/el1->yrad;
	// now transform params of second ellipse into reference frame
	// with origin at center if first ellipse, 
	// scaled so the first ellipse is a circle of radius = 1.0
	double xo = (el2->Center.X - el1->Center.X)*xscale;
	double yo = (el2->Center.Y - el1->Center.Y)*yscale;
	double xr = el2->xrad*xscale;
	double yr = el2->yrad*yscale;
	// now test NSTEPS positions in arc, moving clockwise (ie. decreasing theta)
	double step = M_PI/((NSTEPS-1)*2.0);
	double d_prev, th_prev;
	double th_interp;
	double th1;
	int n = 0;
	for( int i=0; i<NSTEPS; i++ )
	{
		double theta;
		if( i < NSTEPS-1 )
			theta = el2->theta1 - i*step;
		else
			theta = el2->theta2;
		double x = xo + xr*cos(theta);
		double y = yo + yr*sin(theta);
		double d = 1.0 - sqrt(x*x + y*y);
		if( i>0 )
		{
			BOOL bInt = FALSE;
			if( d >= 0.0 && d_prev <= 0.0 )
			{
				th_interp = theta + (step*(-d_prev))/(d-d_prev);
				bInt = TRUE;
			}
			else if( d <= 0.0 && d_prev >= 0.0 )
			{
				th_interp = theta + (step*d_prev)/(d_prev-d);
				bInt = TRUE;
			}
			if( bInt )
			{
				x = xo + xr*cos(th_interp);
				y = yo + yr*sin(th_interp);
				th1 = atan2( y, x );
				if( th1 <= el1->theta1 && th1 >= el1->theta2 )
				{
					xret[n] = x*el1->xrad + el1->Center.X;
					yret[n] = y*el1->yrad + el1->Center.Y;
					n++;
					if( n > 2 )
						ASSERT(0);
				}
			}
		}
		d_prev = d;
		th_prev = theta;
	}
	if( x1 )
		*x1 = xret[0];
	if( y1 )
		*y1 = yret[0];
	if( x2 )
		*x2 = xret[1];
	if( y2 )
		*y2 = yret[1];
	return n;
}

// this finds approximate solution
//
//double GetSegmentClearance( EllipseKH * el1, EllipseKH * el2, 
double GetArcClearance( EllipseKH * el1, EllipseKH * el2, 
					 double * x1, double * y1 )						
{
	const int NSTEPS = 32;

	if( el1->theta2 > el1->theta1 )
		ASSERT(0);
	if( el2->theta2 > el2->theta1 )
		ASSERT(0);

	// test multiple positions in both arcs, moving clockwise (ie. decreasing theta)
	double th_start = el1->theta1;
	double th_end = el1->theta2;
	double th_start2 = el2->theta1;
	double th_end2 = el2->theta2;
	double dmin = DBL_MAX;
	double xmin, ymin, thmin, thmin2;

	int nsteps = NSTEPS;
	int nsteps2 = NSTEPS;
	double step = (th_start-th_end)/(nsteps-1);
	double step2 = (th_start2-th_end2)/(nsteps2-1);
	while( (step * max(el1->xrad, el1->yrad)) > 1.0*NM_PER_MIL 
		&& (step2 * max(el2->xrad, el2->yrad)) > 1.0*NM_PER_MIL )
	{
		step = (th_start-th_end)/(nsteps-1);
		for( int i=0; i<nsteps; i++ )
		{
			double theta;
			if( i < nsteps-1 )
				theta = th_start - i*step;
			else
				theta = th_end;
			double x = el1->Center.X + el1->xrad*cos(theta);
			double y = el1->Center.Y + el1->yrad*sin(theta);
			step2 = (th_start2-th_end2)/(nsteps2-1);
			for( int i2=0; i2<nsteps2; i2++ )
			{
				double theta2;
				if( i2 < nsteps2-1 )
					theta2 = th_start2 - i2*step2;
				else
					theta2 = th_end2;
				double x2 = el2->Center.X + el2->xrad*cos(theta2);
				double y2 = el2->Center.Y + el2->yrad*sin(theta2);
				double d = Distance( x, y, x2, y2 );
				if( d < dmin )
				{
					dmin = d;
					xmin = x;
					ymin = y;
					thmin = theta;
					thmin2 = theta2;
				}
			}
		}
		if( step > step2 )
		{
			th_start = min(el1->theta1, thmin + step);
			th_end = max(el1->theta2, thmin - step);
			step = (th_start - th_end)/nsteps;
		}
		else
		{
			th_start2 = min(el2->theta1, thmin2 + step2);
			th_end2 = max(el2->theta2, thmin2 - step2);
			step2 = (th_start2 - th_end2)/nsteps2;
		}
	}
	if( x1 )
		*x1 = xmin;
	if( y1 )
		*y1 = ymin;
	return dmin;
}

void SetGuidFromString( CString * str, GUID * guid  )
{
	unsigned char x[80];
	strcpy( (char*)x, *str );
	::UuidFromString( x, guid );
}

void GetStringFromGuid( GUID * guid, CString * str )
{
	unsigned char x[80];
	unsigned char * y = x;
	::UuidToString( guid, &y );
	*str = y;
}

// split string at first instance of split_at
// return TRUE if split_at found, set a and b
// return FALSE if not found, return with a = str, b = ""
//
BOOL SplitString( CString * str, CString * a, CString * b, char split_at, BOOL bReverseFind )
{
	int n;
	CString in_str = *str;
	if( bReverseFind )
		n = in_str.ReverseFind( split_at );
	else
		n = in_str.Find( split_at );
	if( n == -1 )
	{
		if( a ) *a = in_str;
		if( b ) *b = "";
		return FALSE;
	}
	else
	{
		if( a ) *a = in_str.Left(n);;
		if( b ) *b = in_str.Right(in_str.GetLength()-n-1);
		return TRUE;
	}
}

// Get angle of part as reported in status line, corrected for centroid angle and side
//
int GetReportedAngleForPart( int part_angle, int cent_angle, int side )
{
	int angle = (360 + part_angle - cent_angle) % 360; 
	if( side )
		angle = (angle + 180) % 360;
	return ccw(angle);
}

// Returns angle of part as reported in status line, corrected for centroid angle and side
//
int GetPartAngleForReportedAngle( int angle, int cent_angle, int side )
{
	int a = ccw(angle);
	if( side )
		a = (a + 180)%360;
	a = (a + cent_angle) % 360;  
	return a;
}

int sign(int thing)
{
	if(thing == 0) return  0;
	if(thing <  0) return -1;
	return 1;
}

