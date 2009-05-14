#include "stdafx.h"
#include "dle_polyline.h"


// draw a closed CPolyLine
void CDLE_POLYLINE::_Draw(CDrawInfo const &di) const
{
	CPolyLine * poly = (CPolyLine*)ptr;
	int layer = poly->GetLayer();
	if( onScreen() && poly->GetClosed() )
	{
		// start by drawing the first contour (no cutouts)
		int n = poly->GetContourEnd(0) + 1;
		POINT *lpPoints = new POINT[n];
		for( int i=0; i<n; i++ )
		{
			lpPoints[i].x = poly->GetX(i)/dlist->Get_pcbu_per_wu();
			lpPoints[i].y = poly->GetY(i)/dlist->Get_pcbu_per_wu();
		}
		di.DC->SetPolyFillMode( WINDING );
		BOOL bOK = di.DC->Polygon( lpPoints, n );
	}
}

int CDLE_POLYLINE::onScreen(void) const
{
	CPolyLine * poly = (CPolyLine*)ptr;
	CRect br = poly->GetCornerBounds( 0 );
	br = br.MulDiv(1, dlist->Get_pcbu_per_wu() );
	if( br.left < dlist->m_max_x && br.right > dlist->m_org_x)
		return 1;
	if( br.bottom < dlist->m_max_y && br.top > dlist->m_org_y)
		return 1;
	return 0;
}

