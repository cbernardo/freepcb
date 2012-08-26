// utility routines
//
#include "stdafx.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include "DisplayList.h" 
 
// globals for timer functions
LARGE_INTEGER PerfFreq, tStart, tStop; 

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
int TestLineHit( int xi, int yi, int xf, int yf, int x, int y, double dist, double *pRet )
{
	// CPT: improved algorithm (old one wasn't working well on short segs).  It also optionally returns a distance value in *pRet (if non-null).
	// First project (x,y) perpendicularly onto segment (xi,yi)-(xf,yf) (a touch of linear algebra).  If the projected point is between the endpoints,
	// we measure the distance along the perpendicular and make that the return distance.  Otherwise determine (x,y)'s distance to each endpoint, 
	// and obtain the minimum of the two. 
	double ret;
	// Translate (xi,yi) to the origin:
	double xf2 = xf-xi, yf2 = yf-yi;
	double x2 = x-xi, y2 = y-yi;
    double lgthSeg = sqrt(xf2*xf2+yf2*yf2);
	double pos = (x2*xf2 + y2*yf2) / lgthSeg;                // This is the relative position of (x,y) when projected onto line 0-(xf,yf)
    if (pos>=0 && pos<=lgthSeg)
		// Projected pt. is between the vertices...
		ret = fabs(xf2*y2 - yf2*x2) / lgthSeg;	             // That's the distance from (x,y) to the line 0-(xf,yf)
	else {
		// Get minimum of the distances from (x,y) to the two endpoints.
		double dx = x-xi, dy = y-yi, d = sqrt(dx*dx+dy*dy);
		ret = d;
		dx = x-xf, dy = y-yf, d = sqrt(dx*dx+dy*dy);
		if (d<ret) ret = d;
		}
	if (pRet) *pRet = ret;
    return ret < dist;
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
// CPT: if bNegateMm is true, negate the return value if unit "mm" is specified.
//
double GetDimensionFromString( CString * str, int def_units, BOOL bRound10, BOOL bNegateMm )
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
		if( str->Right(2).CompareNoCase("MM") == 0 )				// CPT
			mult = bNegateMm? -NM_PER_MM: NM_PER_MM;
		else if( str->Right(3).CompareNoCase("MIL") == 0 )
			mult = NM_PER_MIL;
		else if( str->Right(2).CompareNoCase("NM") == 0 )
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
	f_str.Format( "%%11.%df", max_dp );
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

// CPT
void MakeCStringFromGridVal(CString *str, double val) {
	// CPT.  Utility used for the strings display in the grid-value dropdowns and the grid-val dialogs.  
	// Translate "val" into a mil-or-mm length string, where as usual val<0 => mm units, val>=0 => mil units.  
	// Append "mm" if needed, but not "mil"
	BOOL is_mm = ( val < 0 );
	val = fabs( val );
	if( is_mm )
		str->Format( "%9.3f", val/1000000.0 );
	else
		str->Format( "%9.3f", val/NM_PER_MIL );
	str->Trim();
	int lgth = str->GetLength();
	for (int i=1; i<4; i++)
		if( (*str)[lgth-i] == '0' )
			str->Delete(lgth-i);
		else break;
	if( (*str)[str->GetLength()-1] == '.' )
		str->Delete( str->GetLength()-1 );
	if (is_mm) 
		str->Append(" mm");
	}

int CompareGridVals(const double *gv1, const double *gv2) {
	// CPT.  Compare grid vals for the dropdowns and the grid-val dlgs.
	// Order is like this:  200 mil, 100 mil, 20 mil, 5 mm, 2 mm, 1 mm.
	if (*gv1==*gv2) return 0;
	if (*gv1<0)
		if (*gv2<0) return *gv1<*gv2? -1: 1;
		else        return 1;
	else if (*gv2<0) return -1;
	else return *gv1<*gv2? 1: -1;
	}

int CompareNumeric(CString *s1, CString *s2) {
	// CPT: String comparison where IC9 comes before IC10, etc.  Alpha comparison is caseless.  Used when exporting netlists and (eventually)
	// when saving .fpc's.  Was originally in FreePcbDoc.cpp, but this seems a better place.
	int lgth1 = s1->GetLength(), lgth2 = s2->GetLength();
	for (int i=0; i<lgth1 && i<lgth2; i++) {
		char c1 = toupper((*s1)[i]), c2 = toupper((*s2)[i]);
		if (isdigit(c1) && isdigit(c2)) {
			int val1 = atoi(s1->Mid(i)), val2 = atoi(s2->Mid(i));
			if (val1<val2) return -1;
			if (val1>val2) return 1;
			int i1=i+1, i2=i+1;
			while (i1<lgth1 && isdigit((*s1)[i1])) i1++;
			while (i2<lgth2 && isdigit((*s2)[i2])) i2++;
			if (i1<i2) return -1;									// Fewer leading 0's comes first in this ordering.
			if (i1>i2) return 1;
			i = i1-1;
			continue;												// Numeric segments are identical:  move on to the rest of the string
			}
		if (c1<c2) return -1;
		if (c1>c2) return 1;
		}

	// One string is a subseg of the other
	if (lgth1<lgth2) return -1;
	if (lgth1>lgth2) return 1;
	return strcmp(*s1,*s2);											// Do cased comparison of strings that are caseless-identical
	}


// end CPT


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
// CPT changed:  now allowing 1A.  In that sort of case, astr on return will be 1a and nstr will be "".  I.e.
//  nstr is filled only if pinstr ENDS with a number.

BOOL CheckLegalPinName( CString * pinstr, CString * astr, CString * nstr, int * n )
{
	CString aastr;
	CString nnstr;
	int nn = -1;

	if( *pinstr == "" )
		return FALSE;
	if( -1 != pinstr->FindOneOf( " .,;:/!@#$%^&*(){}[]|<>?\\~\'\"" ) )
		return FALSE;
	int lgth = pinstr->GetLength(), lastNonDigit;
	for (lastNonDigit=lgth-1; lastNonDigit>=0; lastNonDigit--) 
	{
		char c = (*pinstr)[lastNonDigit];
		if (!isdigit(c)) break;
	}
	if (lastNonDigit==lgth-1)
		aastr = *pinstr;			// Doesn't have trailing digits...
	else
		aastr = pinstr->Left(lastNonDigit+1),
		nnstr = pinstr->Mid(lastNonDigit+1),
		nn = atoi(nnstr);
	if( astr )
		*astr = aastr;
	if( nstr )
		*nstr = nnstr;
	if( n )
		*n = nn;
	return TRUE;
}
// end CPT


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
	if( style == CPolyline::ARC_CCW )
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

	if( style != CPolyline::STRAIGHT && style2 != CPolyline::STRAIGHT )
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

	if( style == CPolyline::STRAIGHT && style2 == CPolyline::STRAIGHT )
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
	else if( style == CPolyline::STRAIGHT )
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
	else if( style2 == CPolyline::STRAIGHT )
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
			n = GetArcIntersections( &el1, &el2, &(xr[0]), &(yr[0]), &(xr[1]), &(yr[1]) );
		else
			n = GetArcIntersections( &el2, &el1, &(xr[0]), &(yr[0]), &(xr[1]), &(yr[1]) );
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
		if( style == CPolyline::STRAIGHT || yf == yi )
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
		else if( style == CPolyline::ARC_CW || style == CPolyline::ARC_CCW )
		{
			// arc (quadrant of ellipse)
			// convert to clockwise arc
			int xxi, xxf, yyi, yyf;
			if( style == CPolyline::ARC_CCW )
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
		int test = FindLineSegmentIntersection( a, b, x1i, y1i, x1f, y1f, CPolyline::STRAIGHT,
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
		int test = FindLineSegmentIntersection( a, b, x1i, y1i, x1f, y1f, CPolyline::STRAIGHT,
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
		int test = FindLineSegmentIntersection( a, b, x2i, y2i, x2f, y2f, CPolyline::STRAIGHT,
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
		int test = FindLineSegmentIntersection( a, b, x2i, y2i, x2f, y2f, CPolyline::STRAIGHT,
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
			int test = FindLineSegmentIntersection( a, b, x2i, y2i, x2f, y2f, CPolyline::STRAIGHT,
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


// CPT2 DistancePointEllipseSpecial() and DistancePointEllipse() used to be in a separate file ellipse_newton.cpp, but I've moved them here.

double DistancePointEllipseSpecial (double dU, double dV, double dA,
									double dB, const double dEpsilon, const int iMax, int& riIFinal,
									double& rdX, double& rdY)
{
	// initial guess
	double dT = dB*(dV - dB);

	// Newton’s method
	int i;
	for (i = 0; i < iMax; i++)
	{
		double dTpASqr = dT + dA*dA;
		double dTpBSqr = dT + dB*dB;
		double dInvTpASqr = 1.0/dTpASqr;

		double dInvTpBSqr = 1.0/dTpBSqr;
		double dXDivA = dA*dU*dInvTpASqr;
		double dYDivB = dB*dV*dInvTpBSqr;
		double dXDivASqr = dXDivA*dXDivA;
		double dYDivBSqr = dYDivB*dYDivB;
		double dF = dXDivASqr + dYDivBSqr - 1.0;
		if (dF < dEpsilon)
		{
			// F(t0) is close enough to zero, terminate the iteration
			rdX = dXDivA*dA;
			rdY = dYDivB*dB;
			riIFinal = i;
			break;
		}
		double dFDer = 2.0*(dXDivASqr*dInvTpASqr + dYDivBSqr*dInvTpBSqr);
		double dRatio = dF/dFDer;
		if (dRatio < dEpsilon)
		{
			// t1-t0 is close enough to zero, terminate the iteration
			rdX = dXDivA*dA;
			rdY = dYDivB*dB;
			riIFinal = i;
			break;
		}
		dT += dRatio;
	}
	if (i == iMax)
	{
		// method failed to converge, let caller know
		riIFinal = -1;
		return -FLT_MAX;
	}
	double dDelta0 = rdX - dU, dDelta1 = rdY - dV;
	return sqrt(dDelta0*dDelta0 + dDelta1*dDelta1);
}

double DistancePointEllipse (
							 double dU, double dV, // test point (u,v)
							 double dA, double dB, // ellipse is (x/a)^2 + (y/b)^2 = 1
							 const double dEpsilon, // zero tolerance for Newton’s method
							 const int iMax, // maximum iterations in Newton’s method
							 int& riIFinal, // number of iterations used
							 double& rdX, double& rdY) // a closest point (x,y)
{
	// special case of circle
	if (fabs(dA-dB) < dEpsilon)
	{
		double dLength = sqrt(dU*dU+dV*dV);

		return fabs(dLength - dA);
	}
	// reflect U = -U if necessary, clamp to zero if necessary
	bool bXReflect;
	if (dU > dEpsilon)
	{
		bXReflect = false;
	}
	else if (dU < -dEpsilon)
	{
		bXReflect = true;
		dU = -dU;
	}
	else
	{
		bXReflect = false;
		dU = 0.0;
	}
	// reflect V = -V if necessary, clamp to zero if necessary
	bool bYReflect;
	if (dV > dEpsilon)
	{
		bYReflect = false;
	}
	else if (dV < -dEpsilon)
	{
		bYReflect = true;
		dV = -dV;
	}
	else
	{
		bYReflect = false;
		dV = 0.0;
	}
	// transpose if necessary
	double dSave;
	bool bTranspose;
	if (dA >= dB)
	{
		bTranspose = false;
	}
	else
	{
		bTranspose = true;
		dSave = dA;
		dA = dB;
		dB = dSave;
		dSave = dU;
		dU = dV;

		dV = dSave;
	}
	double dDistance;
	if (dU != 0.0)
	{
		if (dV != 0.0)
		{
			dDistance = DistancePointEllipseSpecial(dU,dV,dA,dB,dEpsilon,iMax,
				riIFinal,rdX,rdY);
		}
		else
		{
			double dBSqr = dB*dB;
			if (dU < dA - dBSqr/dA)
			{
				double dASqr = dA*dA;
				rdX = dASqr*dU/(dASqr-dBSqr);
				double dXDivA = rdX/dA;
				rdY = dB*sqrt(fabs(1.0-dXDivA*dXDivA));
				double dXDelta = rdX - dU;
				dDistance = sqrt(dXDelta*dXDelta+rdY*rdY);
				riIFinal = 0;
			}
			else
			{
				dDistance = fabs(dU - dA);
				rdX = dA;
				rdY = 0.0;
				riIFinal = 0;
			}
		}
	}
	else
	{
		dDistance = fabs(dV - dB);
		rdX = 0.0;
		rdY = dB;
		riIFinal = 0;
	}
	if (bTranspose)
	{
		dSave = rdX;
		rdX = rdY;
		rdY = dSave;
	}
	if (bYReflect)
	{
		rdY = -rdY;
	}

	if (bXReflect)
	{
		rdX = -rdX;
	}
	return dDistance;
}
// these functions are for profiling
// CPT2 simplified

void StartTimer()
{
	if( !QueryPerformanceFrequency( &PerfFreq ) )
		ASSERT(0);
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );
	QueryPerformanceCounter( &tStart );
}

double GetElapsedTime()
{
	QueryPerformanceCounter( &tStop );
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL );
	double start = (double) tStart.QuadPart;
	double stop = (double) tStop.QuadPart;
	double freq = (double) PerfFreq.QuadPart;
	return (stop-start)/freq;
}

// convert angle in degrees from CW to CCW
//
int ccw( int angle )
{
	return (720-angle)%360;
}

// rotate point representing vector between ends of 45-degree curve
// so that it's long axis points along y
// use -octant to reverse the transform
//
CPoint t_octant( int octant, CPoint& pt )
{
	CPoint p;
	switch( octant )
	{
	case 0: p.x =  pt.x;  p.y =  pt.y;  break; 

	case 1: p.x =  pt.y;  p.y = -pt.x;  break; 
	case 2: p.x =  pt.y;  p.y = -pt.x;  break;
	case 3: p.x = -pt.x;  p.y = -pt.y;  break;
	case 4: p.x = -pt.x;  p.y = -pt.y;  break;
	case 5: p.x = -pt.y;  p.y =  pt.x;  break;
	case 6: p.x = -pt.y;  p.y =  pt.x;  break;
	case 7: p.x =  pt.x;  p.y =  pt.y;  break;

	case -1: p.x = -pt.y;  p.y =  pt.x;  break; 
	case -2: p.x = -pt.y;  p.y =  pt.x;  break;
	case -3: p.x = -pt.x;  p.y = -pt.y;  break;
	case -4: p.x = -pt.x;  p.y = -pt.y;  break;
	case -5: p.x =  pt.y;  p.y = -pt.x;  break;
	case -6: p.x =  pt.y;  p.y = -pt.x;  break;
	case -7: p.x =  pt.x;  p.y =  pt.y;  break;
	}
	return p;
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

// Draw a straight line or a curve between xi,yi and xf,yf
// The type of curve depends on the angle between the endpoints
// If a multiple of 90 degrees, draw a straight line,
// else if a multiple of 45 degrees, draw a quadrant of a circle,
// else draw a compound curve consisting of an octant of a circle 
// extended by a straight line at one end or the other
// shape can be DL_LINE, DL_CURVE_CW or DL_CURVE_CCW
//
void DrawCurve( CDC * pDC, int shape, int xxi, int yyi, int xxf, int yyf, BOOL bMeta )
{
	CPoint pt_start(xxi,yyi), pt_end(xxf,yyf);
	if( shape == DL_LINE || xxi == xxf || yyi == yyf || abs(xxf-xxi) == abs(yyf-yyi) )
	{
		// for straight line or 90-degree arc, use DrawArc()
		if( shape == DL_LINE )
			DrawArc( pDC, DL_LINE, xxi, yyi, xxf, yyf, bMeta );
		else if( shape == DL_CURVE_CW )
			DrawArc( pDC, DL_ARC_CW, xxi, yyi, xxf, yyf, bMeta );
		else if( shape == DL_CURVE_CCW )
			DrawArc( pDC, DL_ARC_CCW, xxi, yyi, xxf, yyf, bMeta );
	}
	else if( shape == DL_CURVE_CW || shape == DL_CURVE_CCW ) 
	{
		// for the compound curve, there are 8 possible octants,
		// the straight line can be either vert/horizontal or at 45 degrees 
		// (depending on dy/dx ratio)
		// and the curve can be CW or CCW for a total of 32 different cases
		// to simplify, set endpoints so we can always draw a counter-clockwise arc
		if( shape == DL_CURVE_CW )
		{
			CPoint temp = pt_start;
			pt_start = pt_end;
			pt_end = temp;
		}
		pDC->MoveTo( pt_start );
		// now get the vector from the start to end of curve
		// to simplify further, rotate the vector so dy > 0 and dy > |dx|
		// now there are only 4 cases
		// we'll reverse the rotation at the end
		CPoint d = pt_end - pt_start;	// vector	
		double angle = atan2( (double)d.y, (double)d.x );
		angle -= M_PI_2;
		if( angle < 0.0 )
			angle = 2.0 * M_PI + angle;
		int octant = angle/M_PI_4;
		if ( octant < 0 )
			octant = 0;
		else if( octant > 7 ) 
			octant = 7;
		// rotate vector
		d = t_octant( octant, d );
		double tdx = d.x;	// width of bounding rect for compound curve 
		double tdy = d.y;	// height of bounding rect for compound curve
		double sin_45 = sin(M_PI_4);
		double arc_ratio = abs(sin_45/(1.0 - sin_45)); // dy/dx for octant arc in quadrant 0
		CPoint pt_arc_start, pt_arc_end, pt_line_start, pt_line_end, pt_circle_center;
		int circle_radius;
		if( abs(tdy/tdx) > arc_ratio )
		{
			// height/width ratio of rect is greater than arc, add vertical line to arc
			if( tdx > 0 )
			{
				pt_arc_start.x = 0;
				pt_arc_start.y = 0;
				pt_arc_end.x = tdx;
				pt_arc_end.y = tdx*arc_ratio;
				pt_line_start = pt_arc_end;
				pt_line_end.x = tdx;
				pt_line_end.y = tdy;
				pt_circle_center.y = pt_arc_end.y;
				pt_circle_center.x = - tdx*arc_ratio;
				circle_radius = tdx - pt_circle_center.x;
			}
			else
			{
				pt_arc_start.x = 0;
				pt_arc_start.y = tdy + tdx*arc_ratio;
				pt_arc_end.x = tdx;
				pt_arc_end.y = tdy;
				pt_line_start.x = 0;
				pt_line_start.y = 0;
				pt_line_end = pt_arc_start;
				pt_circle_center.y = pt_arc_start.y;
				pt_circle_center.x = tdx - (tdy - pt_arc_start.y);
				circle_radius = -pt_circle_center.x;
			}
		}
		else
		{
			// height/width ratio of rect is less than arc, add 45-degree line to arc
			if( tdx > 0 )
			{
				double ax = (-tdx+tdy)/(1.0-arc_ratio);
				double ay = -arc_ratio*ax;	
				pt_line_start.x = 0;
				pt_line_start.y = 0;
				pt_line_end.x = tdx + ax;
				pt_line_end.y = tdy - ay;
				pt_arc_start = pt_line_end;
				pt_arc_end.x = tdx;
				pt_arc_end.y = tdy;
				pt_circle_center.x = tdx + ax - ay;
				pt_circle_center.y = tdy;
				circle_radius = ay - ax;
			}
			else
			{
				double ax = (tdx+tdy)/(1.0-arc_ratio);
				double ay = -arc_ratio*ax;	
				pt_arc_start.x = 0;
				pt_arc_start.y = 0;
				pt_arc_end.x = ax;
				pt_arc_end.y = ay;
				pt_line_start = pt_arc_end;
				pt_line_end.x = tdx;
				pt_line_end.y = tdy;
				pt_circle_center.x = pt_arc_end.x - pt_arc_end.y;
				pt_circle_center.y = 0;
				circle_radius = -pt_circle_center.x;
			}

		}
		// now transform points back to original octant
		pt_arc_start = t_octant( -octant, pt_arc_start );
		pt_arc_end = t_octant( -octant, pt_arc_end );
		pt_line_start = t_octant( -octant, pt_line_start );
		pt_line_end = t_octant( -octant, pt_line_end );
		pt_circle_center = t_octant( -octant, pt_circle_center );
		// and offset back to starting point
		CPoint pt_offset = pt_start;
		pt_arc_start += pt_offset;
		pt_arc_end += pt_offset;
		pt_line_start += pt_offset;
		pt_line_end += pt_offset;
		pt_circle_center += pt_offset;
		CRect br;
		br.left = pt_circle_center.x - circle_radius;
		br.right = pt_circle_center.x + circle_radius;
		br.top = pt_circle_center.y - circle_radius;
		br.bottom = pt_circle_center.y + circle_radius;
		// draw arc and line
		pDC->Arc( br, pt_arc_start, pt_arc_end );
		pDC->MoveTo( pt_line_start );
		pDC->LineTo( pt_line_end );
	}
	else
		ASSERT(0);	// illegal shape
	pDC->MoveTo( xxf, yyf );
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

// Find distance from a straight line segment to a pad
//
int GetClearanceBetweenLineSegmentAndPad( int x1, int y1, int x2, int y2, int w,
								  int type, int x, int y, int wid, int len, int radius )
{
	// CPT2 got rid of "angle" arg.  wid and len just have to be right.
	if( type == PAD_NONE )
		return INT_MAX;

	// test for segment entirely within pad
	if( 0 == GetPointToPadDistance( CPoint(x1,y1), type, x, y, wid, len, radius, 0 ) )
		return 0;

	// now get distance from elements of pad outline
	int nc, nr, ns;
	my_circle c[4];
	my_rect r[2];
	my_seg s[8];
	GetPadElements( type, x, y, wid, len, radius, 0,
					&nr, r, &nc, c, &ns, s );
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

// Returns distance between a point and a segment,
// also returns coords of closest point on segment
//
int GetPointToSegmentDistance( CPoint p,
				int xi, int yi, int xf, int yf, int w, int style )
{
	int dist = INT_MAX, xmin, ymin;
	if( style == CPolyline::STRAIGHT )
	{
		// segment is a straight line
		dist = GetPointToLineSegmentDistance( p.x, p.y, xi, yi, xf, yf );
	}
	else
	{
		// segment is an arc of an ellipse
		EllipseKH el;
		MakeEllipseFromArc( xi, yi, xf, yf, style, &el );
		// get quadrant of point in ellipse
		double th_point = atan2( p.y - el.Center.Y, p.x - el.Center.X );
		if( th_point < el.theta1 && th_point > el.theta2 )
		{
			// same quadrant as arc, need to solve for minimum distance
			int nIter;
			double nearX, nearY;
			double d_test = DistancePointEllipse( p.x, p.y, el.xrad, el.yrad, 0.1, 1000, nIter, nearX, nearY );
		}
		else
		{
			// not in same quadrant, just check endpoints
			double x_test, y_test, dx, dy, d_test;
			for( int iq=0; iq<4; iq++ )
			{
				switch( iq )
				{
				case 0: x_test = el.Center.X + el.xrad; y_test = el.Center.Y; break;
				case 1: x_test = el.Center.X; y_test = el.Center.Y + el.yrad; break;
				case 2: x_test = el.Center.X - el.xrad; y_test = el.Center.Y; break;
				case 3: x_test = el.Center.X; y_test = el.Center.Y - el.yrad; break;
				}
				dx = p.x - x_test;
				dy = p.y - y_test;
				d_test = sqrt( dx*dx + dy*dy );
				if( d_test < dist )
				{
					dist = d_test;
					xmin = x_test;
					ymin = y_test;
				}
			}

		}
	}
	return dist - w;
}

// Get clearance between 2 segments
// Returns point in segment closest to other segment in x, y
// if clearance > min_cl, just returns min_cl and doesn't return x,y
//
int GetClearanceBetweenSegments( int x1i, int y1i, int x1f, int y1f, int style1, int w1,
								   int x2i, int y2i, int x2f, int y2f, int style2, int w2,
								   int min_cl, int * x, int * y )
{
	// check clearance between bounding rectangles
	int test = min_cl + w1/2 + w2/2;
	if( min(x1i,x1f)-max(x2i,x2f) > test )
		return min_cl;
	if( min(x2i,x2f)-max(x1i,x1f) > test )
		return min_cl;
	if( min(y1i,y1f)-max(y2i,y2f) > test )
		return min_cl;
	if( min(y2i,y2f)-max(y1i,y1f) > test )
		return min_cl;

	if( style1 == CPolyline::STRAIGHT && style1 == CPolyline::STRAIGHT )
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
	if( style2 == CPolyline::STRAIGHT )
	{
		// style1 = arc, style2 = straight
		MakeEllipseFromArc( x1i, y1i, x1f, y1f, style1, &el1 );
		xi = x2i;
		yi = y2i;
		xf = x2f;
		yf = y2f;
		bArcs = FALSE;
	}
	else if( style1 == CPolyline::STRAIGHT )
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

// Get distance from point to pad
// return 0 if point inside pad
//
double GetPointToPadDistance( CPoint p, 
		int type, int x, int y, int w, int l, int rad, int angle )
{
	int dist = INT_MAX;
	int nr, nc, ns, nrr, ncc, nss;
	my_rect r[2];
	my_circle c[4];
	my_seg s[8];

	if( type == PAD_NONE )
		return INT_MAX;

	GetPadElements( type, x, y, w, l, rad, angle,
					&nr, r, &nc, c, &ns, s );

	for( int ic=0; ic<nc; ic++ )
	{
		int d = Distance( p.x, p.y, c[ic].x, c[ic].y )- c[ic].r;
		dist = min(dist,d);
	}
	for( int ir=0; ir<nr; ir++ )
	{
		if( p.x >= r[ir].xlo && p.x <= r[ir].xhi && p.y >= r[ir].ylo && p.y <= r[ir].yhi )
			return 0;
	}
	for( int is=0; is<ns; is++ )
	{
		int d = GetPointToLineSegmentDistance( p.x, p.y, s[is].xi, s[is].yi, s[is].xf, s[is].yf );
		dist = min(dist,d);
	}
	return dist;
}

// Find clearance between pads
// For each pad:
//	type = PAD_ROUND, PAD_SQUARE, etc.
//	x, y = center position
//	w, l = width and length
//  r = corner radius
//	angle = 0 or 90 (if 0, pad length is along x-axis)  CPT2 got rid of these args:  caller should have w and l correct for the angle
//
int GetClearanceBetweenPads( int type1, int x1, int y1, int w1, int l1, int r1,
							 int type2, int x2, int y2, int w2, int l2, int r2 )
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

	// first, test for one pad entirely within the other
	if( GetPointToPadDistance( CPoint(x1,y1), 
				type2, x2, y2, w2, l2, r2, 0 ) == 0 )
	return 0;
	if( GetPointToPadDistance( CPoint(x2,y2), 
				type1, x1, y1, w1, l1, r1, 0 ) == 0 )
	return 0;

	// now find distance from every element of pad1 to every element of pad2
	GetPadElements( type1, x1, y1, w1, l1, r1, 0,
					&nr, r, &nc, c, &ns, s );
	GetPadElements( type2, x2, y2, w2, l2, r2, 0,
					&nrr, rr, &ncc, cc, &nss, ss );
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
// on return, sets *pNearX and *pNearY to nearest point on line segment
//
double GetPointToLineSegmentDistance( int x, int y, int xi, int yi, int xf, int yf, 
									 double * xp, double * yp )
{
	BOOL bFound = FALSE;
	double nearX, nearY, dist;
	// test for nearest point between ends of line segment
	if( xf==xi )
	{
		// vertical line segment
		if( InRange( y, yi, yf ) )
		{
			dist = abs( x - xi );
			nearX = xi;
			nearY = y;
			bFound = TRUE;
		}
	}
	else if( yf==yi )
	{
		// horizontal line segment
		if( InRange( x, xi, xf ) )
		{
			dist = abs( y - yi );
			nearX = x;
			nearY = yi;
			bFound = TRUE;
		}
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
		{
			dist = Distance( x, y, xp, yp );
			nearX = xp;
			nearY = yp;
			bFound = TRUE;
		}
	}
	if( !bFound )
	{
		double dist_i = Distance( x, y, xi, yi );
		double dist_f = Distance( x, y, xf, yf );
		if( dist_i < dist_f )
		{
			dist = dist_i;
			nearX = xi;
			nearY = yi;
		}
		else
		{
			dist = dist_f;
			nearX = xf;
			nearY = yf;
		}
	}
	if( xp )
		*xp = nearX;
	if( yp )
		*yp = nearY;
	return dist;
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

// Get distance between 2 points.  CPT r294:  changed args to double.
//
double Distance( double x1, double y1, double x2, double y2 )
{
	double d = sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) );
	return d;
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
	// CPT2 TODO.  Not sure if I like the effect of the following.  I'm seeing what it's like without:
	// if( side )
	//	angle = (angle + 180) % 360;
	return ccw(angle);
}

// Returns angle of part as reported in status line, corrected for centroid angle and side
//
int GetPartAngleForReportedAngle( int angle, int cent_angle, int side )
{
	int a = ccw(angle);
	// CPT2.  Not sure I like the following 2 lines, seeing what it's like to omit em:
	// if( side )
	//	a = (a + 180)%360;
	a = (a + cent_angle) % 360;  
	return a;
}

int sign(int thing)
{
	if(thing == 0) return  0;
	if(thing <  0) return -1;
	return 1;
}


// CPT (All that follows):

// CPT.  Functions for utility class CMyBitmap

void CMyBitmap::Line(int x0, int y0, int x1, int y1, char val) {
	// Draw a "line" on the bitmap from (x1,y1) to (x2,y2), using byte value "val".  Successfully clips any out-of-range pixels on the line.
	if (x0<0 && x1<0) return;
	if (x0>=w && x1>=w) return;
	if (y0<0 && y1<0) return;
	if (y0>=h && y1>=h) return;
	int dx = x1-x0, dy = y1-y0, tmp, jog;
	bool bSteep;
	if (dx>0) bSteep = dy>dx || dy<-dx;
	else      bSteep = dy<dx || dy>-dx;
	if (bSteep) {
		if (dy<0)
			tmp = x0, x0 = x1, x1 = tmp,
			tmp = y0, y0 = y1, y1 = tmp,
			dx = -dx, dy = -dy;
		if (dx<0) jog = -1, dx = -dx;
		else      jog = 1;
		for (int y=y0, x=x0, r=0; y<=y1; y++, r+=dx) {
			if (r>=dy) r-=dy, x+=jog;
			if (y>=h) break;
			if (y<0 || x<0 || x>=w) continue;
			data[y*w+x] = val;
			}
		}
	else {
		if (dx<0)
			tmp = x0, x0 = x1, x1 = tmp,
			tmp = y0, y0 = y1, y1 = tmp,
			dx = -dx, dy = -dy;
		if (dy<0) jog = -1, dy = -dy;
		else      jog = 1;
		for (int x=x0, y=y0, r=0; x<=x1; x++, r+=dy) {
			if (r>=dx) r-=dx, y+=jog;
			if (x>=w) break;
			if (x<0 || y<0 || y>=h) continue;
			data[y*w+x] = val;
			}
		}
	}

void CMyBitmap::Arc(int x0, int y0, int x1, int y1, bool bCw, char val) {
	// Draw a quarter-elliptical arc from (x0,y0) to (x1,y1), either clockwise or counterclockwise depending on bCw.  (Just like the arcs that can
	// be used as area edges...)  Fill in pixels using value "val".  Does not need to worry about out-of-range situations --- the CMyBitmap is 
	// guaranteed to be large enough to accommodate any area-edge arcs that we're going to be "drawing" on it.
	if (x0==x1 || y0==y1)
		{ Line(x0,y0, x1,y1, val); return; }
	// Ensure that x increases from x0 to x1:
	int tmp;
	if (x0>x1)
		tmp = x0, x0 = x1, x1 = tmp,
		tmp = y0, y0 = y1, y1 = tmp,
		bCw = !bCw;
	double dx = x1-x0, dy = y1-y0;
	if (dy<0)
		bCw = !bCw;
	// At this point, if bCw, we'll basically use the function y = sqrt(2*x-x**2) (with scaling);  otherwise, use y = 1-sqrt(1-x**2).
	int yPrev = y0;
	for (int x=x0+1; x<=x1; x++) {
		double xx = (x-x0)/dx, yy;
		if (bCw)
			yy = sqrt(2*xx-xx*xx);
		else 
			yy = 1 - sqrt(1-xx*xx);
		int y = floor(yy*dy + y0 + .5);
		// Fill in pixels in columns x-1 and x as appropriate
		if (y==yPrev)
			data[y*w+x-1] = data[y*w+x] = val;
		else if (y>yPrev) {
			int yMid = bCw? (y+yPrev+1)/2: (y+yPrev)/2;
			for (int y1=yPrev; y1<=yMid; y1++)
				data[y1*w+x-1] = val;
			for (int y1=yMid; y1<=y; y1++)
				data[y1*w+x] = val;
			}
		else {
			int yMid = bCw? (y+yPrev)/2: (y+yPrev+1)/2;
			for (int y1=yPrev; y1>=yMid; y1--)
				data[y1*w+x-1] = val;
			for (int y1=yMid; y1>=y; y1--)
				data[y1*w+x] = val;
			}
		yPrev = y;
		}
	}

int CMyBitmap::FloodFill(int x0, int y0, char val) {
	// Start at position (x0,y0), and flood fill, putting in value "val" wherever we encounter values equal to the original value at
	// position (x0,y0).  Returns the number of pixels that got flooded
	if (x0<0 || x0>=w) return 0;
	if (y0<0 || y0>=h) return 0;
	char orig = data[y0*w+x0];
	if (val==orig) return 0;						// Caller goofed...
	int ret = 0;
	char *row = data+y0*w;
	while (x0>0 && row[x0-1]==orig) x0--;			// Move x0 back to the beginning of any run of pixels with value "orig"
	CArray<int> xList, yList;
	xList.SetSize(0, 1000); yList.SetSize(0, 1000);
	xList.Add(x0), yList.Add(y0);
	int p=0;
	while (p<xList.GetSize()) {
		// Get a point (x,y) from the needs-to-be-processed list.  Determine the horizontal run of points starting at (x,y),
		// such that all of them have the value "orig" on the bitmap.  Change their values to "val"
		int x = xList[p], y = yList[p++], x2, x3;
		row = data+y*w;
		for (x2=x; x2<w && row[x2]==orig; x2++)
			row[x2] = val, ret++;
		// Determine if there are points in the preceding row to add to the needs-to-be-processed list.  Only add points that are at
		// the starts of runs with the value "orig"
		if (y>0) {
			char *above = row-w;
			if (above[x]==orig) {
				for (x3=x; x3>0 && above[x3-1]==orig; x3--) ;
				xList.Add(x3);  yList.Add(y-1);
				}
			for (x3=x+1; x3<x2; x3++)
				if (above[x3]==orig && above[x3-1]!=orig)
					xList.Add(x3), yList.Add(y-1);
			}
		// Similarly look for points in the following row to add to the needs-to-be-processed list.
		if (y<h-1) {
			char *below = row+w;
			if (below[x]==orig) {
				for (x3=x; x3>0 && below[x3-1]==orig; x3--) ;
				xList.Add(x3); yList.Add(y+1);
				}
			for (x3=x+1; x3<x2; x3++)
				if (below[x3]==orig && below[x3-1]!=orig)
					xList.Add(x3), yList.Add(y+1);
			}
		}
	return ret;
	}

