#include "stdafx.h"
#include "dle_centroid.h"

// circle and X
void CDLE_CENTROID::_Draw(CDrawInfo &di, bool) 
{
	// x,y are center coords; w = width;
	// f.x,f.y define arrow end-point for P&P orientation
	if( onScreen() )
	{
        int w2 = w/2;
        int xi = i.x - w2;
        int yi = i.y - w2;
        int _xf = i.x + w2;
        int _yf = i.y + w2;

		di.DC->SelectObject( GetStockObject( HOLLOW_BRUSH ) );
		di.DC->Ellipse( i.x - w2/2, i.y - w2/2, i.x + w2/2, i.y + w2/2 );
		di.DC->SelectObject( di.fill_brush );

		di.DC->MoveTo( xi, yi );
		di.DC->LineTo( _xf, _yf );
		di.DC->MoveTo( _xf, yi );
		di.DC->LineTo( xi, _yf );
		di.DC->MoveTo( i.x, i.y );	// p&p arrow
		di.DC->LineTo( f.x, f.y );
		if( i.y == f.y )
		{
			// horizontal arrow
			di.DC->LineTo( f.x - (f.x - i.x)/4,	f.y + w/4 );
			di.DC->LineTo( f.x - (f.x - i.x)/4,	f.y - w/4 );
			di.DC->LineTo( f.x,                 f.y );
		}
		else if( i.x == f.x )
		{
			// vertical arrow
			di.DC->LineTo( f.x + w/4, f.y - (f.y - i.y)/4 );
			di.DC->LineTo( f.x - w/4, f.y - (f.y - i.y)/4 );
			di.DC->LineTo( f.x,       f.y );
		}
		else
			ASSERT(0);

		di.nlines += 2;
	}
}
