#include "stdafx.h"
#include "dle_rect_rounded.h"

// filled rounded rectangle
void CDLE_RRECT::_Draw(CDrawInfo &di, bool)
{
	if( onScreen() )
	{
		if( gtype == DL_HOLLOW_RRECT )
			di.DC->SelectObject( GetStockObject( HOLLOW_BRUSH ) );

		di.DC->RoundRect( i.x, i.y, f.x, f.y, 2*radius, 2*radius );

		if( gtype == DL_HOLLOW_RRECT )
			di.DC->SelectObject( di.fill_brush );

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

/* void CDLE_RRECT::_DrawClearance(CDrawInfo const &di) const
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

	int r = 2 * radius * (_xf-_xi + 2*clearancew) / (_xf-_xi);
	di.DC->RoundRect( _xi - clearancew, _yi - clearancew,
	                  _xf + clearancew, _yf + clearancew,
					  r,r);

	di.DC->SelectObject( di.fill_brush );
	di.DC->SelectObject( di.line_pen );
}
*/