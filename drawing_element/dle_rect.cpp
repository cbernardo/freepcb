#include "stdafx.h"
#include "dle_rect.h"

//-----------------------------------------------------------------------------
// filled rectangle
void CDLE_RECT::_Draw(CDrawInfo const &di) const
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

void CDLE_RECT::_DrawClearance(CDrawInfo const &di) const
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


//-----------------------------------------------------------------------------
// rectangle outline

void CDLE_HOLLOW_RECT::_Draw(CDrawInfo const &di) const
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

void CDLE_HOLLOW_RECT::_DrawClearance(CDrawInfo const &di) const
{
}

int CDLE_HOLLOW_RECT::_isHit(CPoint const &point) const
{
	return (   ( (point.x>i.x && point.x<f.x) || (point.x<i.x && point.x>f.x) )
	        && ( (point.y>i.y && point.y<f.y) || (point.y<i.y && point.y>f.y) ) );
}


//-----------------------------------------------------------------------------
// rectangle outline with X

// rectangle outline with X
void CDLE_RECT_X::_Draw(CDrawInfo const &di) const
{
	if( onScreen() )
	{
		CDLE_HOLLOW_RECT::_Draw(di);

		// Draw the X
		di.DC->MoveTo( i.x, i.y );
		di.DC->LineTo( f.x, f.y );
		di.DC->MoveTo( i.x, f.y );
		di.DC->LineTo( f.x, i.y );

		di.nlines += 4;
	}
}
