#include "stdafx.h"
#include "dle_oval.h"

// filled oval
void CDLE_OVAL::_Draw(CDrawInfo const &di) const
{
	if( onScreen() )
	{
		int h = abs(f.x-i.x);
		int v = abs(f.y-i.y);
		int r = min(h,v);

		if( gtype == DL_HOLLOW_OVAL )
			di.DC->SelectObject( GetStockObject( HOLLOW_BRUSH ) );

		di.DC->RoundRect( i.x, i.y, f.x, f.y, r, r );

		if( gtype == DL_HOLLOW_OVAL )
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

void CDLE_OVAL::_DrawClearance(CDrawInfo const &di) const
{
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

	int h = _xf-_xi;
	int v = _yf-_yi;
	int r = min(h,v) + 2*clearancew;

	di.DC->SelectObject( di.erase_brush );
	di.DC->SelectObject( di.erase_pen );

	di.DC->RoundRect( _xi - clearancew, _yi - clearancew, _xf + clearancew, _yf + clearancew, r, r );

	di.DC->SelectObject( di.fill_brush );
	di.DC->SelectObject( di.line_pen );
}

