#include "stdafx.h"
#include "dle_donut.h"

// annulus
void CDLE_DONUT::_Draw(CDrawInfo const &di) const
{
	if( onScreen() )
	{
		int thick = (w - holew)/2;
		int ww = w - thick;
		int _holew = holew;
		int size_of_2_pixels = dlist->m_scale;
		if( thick < size_of_2_pixels )
 		{
			_holew = w - 2*size_of_2_pixels;
			if( _holew < 0 )
				_holew = 0;

			thick = (w - _holew)/2;
			ww = w - thick;
		}
		if( w-_holew > 0 )
		{
			CPen pen( PS_SOLID, thick, di.layer_color[1] );
			di.DC->SelectObject( &pen );
			di.DC->SelectObject( di.erase_brush );
			di.DC->Ellipse( i.x - ww/2, i.y - ww/2, i.x + ww/2, i.y + ww/2 );
			di.DC->SelectObject( di.line_pen );
			di.DC->SelectObject( di.fill_brush );
		}
		else
		{
			CPen backgnd_pen( PS_SOLID, 1, di.layer_color[0] );

			di.DC->SelectObject( di.erase_pen );
			di.DC->SelectObject( di.erase_brush );
			di.DC->Ellipse( i.x - _holew/2, i.y - _holew/2, i.x + _holew/2, i.y + _holew/2 );
			di.DC->SelectObject( di.line_pen );
			di.DC->SelectObject( di.fill_brush );
		}
	}
}

void CDLE_DONUT::_DrawClearance(CDrawInfo const &di) const
{
}


