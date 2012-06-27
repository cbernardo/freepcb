#include "stdafx.h"
#include "dle_rect.h"

//-----------------------------------------------------------------------------
// filled rectangle
void CDLE_RECT::_Draw(CDrawInfo &di, bool)
{
	if( onScreen() )
	{
		di.DC->Rectangle( i.x, i.y, f.x, f.y );

		if( holew != 0 )
		{
			di.DC->SelectObject( di.erase_brush );
			di.DC->SelectObject( di.erase_pen );

			di.DC->Ellipse( org.x - holew/2, org.y - holew/2, org.x + holew/2, org.y + holew/2 );

			di.DC->SelectObject( di.fill_brush );
			di.DC->SelectObject( di.line_pen );
		}
	}
}

/* void CDLE_RECT::_DrawClearance(CDrawInfo const &di) const
{
	di.DC->SelectObject( di.erase_brush );
	di.DC->SelectObject( di.erase_pen );

    int _xi = i.x;
    int _yi = i.y;
	int _xf = f.x;
	int _yf = f.y;

	if( _xf < _xi )
	{
		_xf = _xi;
		_xi = f.x;
	}
	if( _yf < _yi )
	{
		_yf = _yi;
		_yi = f.y;
	}

	di.DC->Rectangle( _xi - clearancew, _yi - clearancew, _xf + clearancew, _yf + clearancew);

	di.DC->SelectObject( di.fill_brush );
	di.DC->SelectObject( di.line_pen );
}
*/

//-----------------------------------------------------------------------------
// rectangle outline

void CDLE_HOLLOW_RECT::_Draw(CDrawInfo &di, bool) 
{
	if( onScreen() )
	{
		di.DC->MoveTo( i.x, i.y );
		di.DC->LineTo( f.x, i.y );
		di.DC->LineTo( f.x, f.y );
		di.DC->LineTo( i.x, f.y );
		di.DC->LineTo( i.x, i.y );

		di.nlines += 4;
	}
}

/* void CDLE_HOLLOW_RECT::_DrawClearance(CDrawInfo const &di) const
{
}
*/

int CDLE_HOLLOW_RECT::_IsHit(double x, double y, double &d)
{
	double xCenter = (i.x+f.x) / 2., yCenter = (i.y+f.y) / 2.;
	double w2 = abs(i.x-f.x) / 2., h2 = abs(i.y-f.y) / 2.;
	if (item->IsVertex() || item->IsAreaCorner() || item->IsOutlineCorner()) {
		// CPT r294.  Feature #27 (obparham's idea).  Still not certain if this is a winner...
		// For vertices on connects and area edges, don't let the selectable region get too small (total width & ht < 6 pixels), or
		// too big (total width & ht > 16 pixels).  But for pins & parts etc. don't make this adjustment.  Also skip the adjustment
		// if it's a via.
		double scale = dlist->m_scale;
		if (item->IsVia()) ;
		else if (w2 < 3.0*scale) w2 = h2 = 3.0*scale;				// NB assuming that squares are always appropriate
		else if (w2 > 8.0*scale) w2 = h2 = 8.0*scale;
	}
	double dx = x-xCenter, dy = y-yCenter;
	if (fabs(dx) < w2 && fabs(dy) < h2) 
		{ d = sqrt(dx*dx+dy*dy); return true; }
	return false;
}


//-----------------------------------------------------------------------------
// rectangle outline with X

// rectangle outline with X
void CDLE_RECT_X::_Draw(CDrawInfo &di, bool)
{
	if( onScreen() )
	{
		CDLE_HOLLOW_RECT::_Draw(di, false);

		// Draw the X
		di.DC->MoveTo( i.x, i.y );
		di.DC->LineTo( f.x, f.y );
		di.DC->MoveTo( i.x, f.y );
		di.DC->LineTo( f.x, i.y );

		di.nlines += 4;
	}
}
