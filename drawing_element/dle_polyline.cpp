#include "stdafx.h"
#include "dle_polyline.h"

// draw a closed CPolyLine
void CDLE_POLYLINE::_Draw(CDrawInfo const &di) const
{
	CPolyLine * poly = (CPolyLine*)ptr;
	if( onScreen() && poly->GetClosed() )
	{
		// start by just drawing the outer contour
		int n = poly->GetContourEnd(0) + 1;
		POINT *lpPoints = new POINT[n];
		for( int i=0; i<n; i++ )
		{
			lpPoints[i].x = poly->GetX(i)/dlist->Get_pcbu_per_wu();
			lpPoints[i].y = poly->GetY(i)/dlist->Get_pcbu_per_wu();
		}
		di.DC->SetPolyFillMode( WINDING );
		BOOL bOK = di.DC->Polygon( lpPoints, n );
#if 0
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
#endif
	}
}

int CDLE_POLYLINE::onScreen(void) const
{
	CPolyLine * poly = (CPolyLine*)ptr;
	CRect bounds = poly->GetCornerBounds();
	bounds = bounds.MulDiv(1, dlist->Get_pcbu_per_wu() );
	if( bounds.left < dlist->m_max_x && bounds.right > dlist->m_org_x)
		return 1;
	if( bounds.bottom < dlist->m_max_y && bounds.top > dlist->m_org_y)
		return 1;
	return 0;
}

