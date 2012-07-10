#include "stdafx.h"
#include "dle_square.h"

extern CFreePcbApp theApp;

// filled square
void CDLE_SQUARE::_Draw(CDrawInfo &di, bool)
{
	if( onScreen() )
	{
		di.DC->Rectangle( i.x - w/2, i.y - w/2, i.x + w/2, i.y + w/2 );
		if( holew != 0 )
		{
			di.DC->SelectObject( di.erase_brush );
			di.DC->SelectObject( di.erase_pen );

			di.DC->Ellipse( i.x - holew/2, i.y - holew/2, i.x + holew/2, i.y + holew/2 );

			di.DC->SelectObject( di.fill_brush );
			di.DC->SelectObject( di.line_pen );
		}
	}
}


/* void CDLE_SQUARE::_DrawClearance(CDrawInfo const &di) const
{
	int sz = w/2 + clearancew;

	di.DC->SelectObject( di.erase_brush );
	di.DC->SelectObject( di.erase_pen );

	di.DC->Rectangle( i.x - sz, i.y - sz, i.x + sz, i.y + sz );

	di.DC->SelectObject( di.fill_brush );
	di.DC->SelectObject( di.line_pen );
}
*/

void CDLE_SQUARE::_DrawThermalRelief(CDrawInfo &di)
{
	CFreePcbDoc * doc = theApp.m_Doc;

	int conn_tracew = doc->m_thermal_width / 2540;
	int therm_clearance = doc->m_thermal_clearance / 2540;

	int sz = w/2 + therm_clearance;

	di.DC->SelectObject( di.erase_brush );
	di.DC->SelectObject( di.erase_pen );

	di.DC->Rectangle( i.x - sz, i.y - sz, i.x + sz, i.y + sz );

	di.DC->SelectObject( di.fill_brush );

	CPen pen( PS_SOLID, conn_tracew, di.layer_color[1] );

	di.DC->SelectObject( &pen );
	di.DC->MoveTo( i.x - sz, i.y );
	di.DC->LineTo( i.x + sz, i.y );
	di.DC->MoveTo( i.x, i.y - sz );
	di.DC->LineTo( i.x, i.y + sz );

	di.DC->SelectObject( di.line_pen );
}
