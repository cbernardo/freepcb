// utility routines
//
#pragma once 

typedef struct PointTag
{
	double X,Y;
} Point;

typedef struct EllipseTag
{
	Point Center;			/* ellipse center	 */
//	double MaxRad,MinRad;	/* major and minor axis */
//	double Phi;				/* major axis rotation  */
	double xrad, yrad;		// radii on x and y
	double theta1, theta2;	// start and end angle for arc 
} EllipseKH;

const CPoint zero(0,0);

class my_circle {
public:
	my_circle(){};
	my_circle( int xx, int yy, int rr )
	{
		x = xx;
		y = yy;
		r = rr;
	};
	int x, y, r; 
};

class my_rect {
public:
	my_rect(){};
	my_rect( int xi, int yi, int xf, int yf )
	{
		xlo = min(xi,xf);
		xhi = max(xi,xf);
		ylo = min(yi,yf);
		yhi = max(yi,yf);
	};
	int xlo, ylo, xhi, yhi; 
};

class my_seg { 
public:
	my_seg(){};
	my_seg( int xxi, int yyi, int xxf, int yyf )
	{
		xi = xxi;
		yi = yyi;
		xf = xxf;
		yf = yyf;
	};
	int xi, yi, xf, yf; 
};

// map part angle to reported part angle
int GetReportedAngleForPart( int part_angle, int cent_angle, int side );
int GetPartAngleForReportedAngle( int angle, int cent_angle, int side );

// handle strings
char * mystrtok( LPCTSTR str, LPCTSTR delim );
double GetDimensionFromString( CString * str, int def_units=MIL, BOOL bRound10=TRUE, BOOL bNegateMm=FALSE ); // CPT
void MakeCStringFromDimension( CString * str, int dim, int units, BOOL append_units=TRUE, 
							  BOOL lower_case = FALSE, BOOL space=FALSE, int max_dp=8, BOOL strip=TRUE );
void MakeCStringFromGridVal(CString *str, double val);  // CPT
int CompareGridVals(const double *gv1, const double *gv2); // CPT
void MakeCStringFromDouble( CString * str, double d );
BOOL CheckLegalPinName( CString * pinstr, 
					   CString * astr=NULL, 
					   CString * nstr=NULL, 
					   int * n=NULL );
int ParseRef( CString * ref, CString * prefix );
void SetGuidFromString( CString * str, GUID * guid  );
void GetStringFromGuid( GUID * guid, CString * str );
BOOL SplitString( CString * str, CString * a, CString * b, char split_at, BOOL bReverseFind=FALSE );

// for profiling
void CalibrateTimer();
void StartTimer();
double GetElapsedTime();

// math stuff for graphics
int ccw( int angle );
int sign(int thing);
BOOL Quadratic( double a, double b, double c, double *x1, double *x2 );
void DrawArc( CDC * pDC, int shape, int xxi, int yyi, int xxf, int yyf, BOOL bMeta=FALSE );
void DrawCurve( CDC * pDC, int shape, int xxi, int yyi, int xxf, int yyf, BOOL bMeta=FALSE );
void RotatePoint( CPoint *p, int angle, CPoint org );
void RotateRect( CRect *r, int angle, CPoint org );
int TestLineHit( int xi, int yi, int xf, int yf, int x, int y, double dist );
int FindLineIntersection( double a, double b, double c, double d, double * x, double * y );
int FindLineIntersection( double x0, double y0, double x1, double y1,
						  double x2, double y2, double x3, double y3,
						  double *linx, double *liny);
int FindLineSegmentIntersection( double a, double b, int xi, int yi, int xf, int yf, int style, 
				double * x1, double * y1, double * x2, double * y2, double * dist=NULL );
int FindSegmentIntersections( int xi, int yi, int xf, int yf, int style, 
								 int xi2, int yi2, int xf2, int yf2, int style2,
								 double x[]=NULL, double y[]=NULL );
BOOL FindLineEllipseIntersections( double a, double b, double c, double d, double *x1, double *x2 );
BOOL FindVerticalLineEllipseIntersections( double a, double b, double x, double *y1, double *y2 );
BOOL TestForIntersectionOfStraightLineSegments( int x1i, int y1i, int x1f, int y1f, 
									   int x2i, int y2i, int x2f, int y2f,
									   int * x=NULL, int * y=NULL, double * dist=NULL );
void GetPadElements( int type, int x, int y, int wid, int len, int radius, int angle,
					int * nr, my_rect r[], int * nc, my_circle c[], int * ns, my_seg s[] );
int GetClearanceBetweenPads( int type1, int x1, int y1, int w1, int l1, int r1, int angle1,
							 int type2, int x2, int y2, int w2, int l2, int r2, int angle2 );
int GetClearanceBetweenSegmentAndPad( int x1, int y1, int x2, int y2, int w,
								  int type, int x, int y, int wid, int len, 
								  int radius, int angle );
int GetClearanceBetweenSegments( int x1i, int y1i, int x1f, int y1f, int style1, int w1,
								   int x2i, int y2i, int x2f, int y2f, int style2, int w2,
								   int max_cl, int * x, int * y );
double GetPointToLineSegmentDistance( int xi, int yi, int xf, int yf, int x, int y );
double GetPointToLineDistance( double a, double b, int x, int y, double * xp=NULL, double * yp=NULL );
BOOL InRange( double x, double xi, double xf );
double Distance( int x1, int y1, int x2, int y2 );
int GetArcIntersections( EllipseKH * el1, EllipseKH * el2, 
						double * x1=NULL, double * y1=NULL, 
						double * x2=NULL, double * y2=NULL );						
CPoint GetInflectionPoint( CPoint pi, CPoint pf, int mode );

// quicksort (2-way or 3-way)
void quickSort(int numbers[], int index[], int array_size);
void q_sort(int numbers[], int index[], int left, int right);
void q_sort_3way( int a[], int b[], int left, int right );

class CMyBitmap {
	// CPT.  A handy class used during the new phase I've added to PartList::DRC().  Represents a 2-d array of 1-byte values ("pixels").
	int w, h;
	char *data;

	public:
	CMyBitmap(int w0, int h0) {
		w = w0; h = h0;
		data = (char*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, w*h);
		};
	~CMyBitmap() {
		HeapFree(GetProcessHeap(), 0, data);
		};
	inline char Get(int x, int y) 
		{ return data[y*w+x]; }							// NB no range check!
	inline void Set(int x, int y, char val) 
		{ data[y*w+x] = val; }							// NB no range check!

	void Line(int x0, int y0, int x1, int y1, char val) {
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

	int FloodFill(int x0, int y0, char val) {
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

	};