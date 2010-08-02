#include "stdafx.h"
#include "math.h"
#include "dle_octagon.h"

static const double pi = 3.14159265359;

// filled octagon
void CDLE_OCTAGON::_Draw(CDrawInfo const &di) const
{
	if( onScreen() )
	{
		POINT pt[8];
		double angle = pi/8.0;
		for( int iv=0; iv<8; iv++ )
		{
			pt[iv].x = i.x + w/2 * cos(angle);
			pt[iv].y = i.y + w/2 * sin(angle);
			angle += pi/4.0;
		}

		if( gtype == DL_HOLLOW_OCTAGON )
			di.DC->SelectObject( GetStockObject( HOLLOW_BRUSH ) );

		di.DC->Polygon( pt, 8 );

		if( gtype == DL_HOLLOW_OCTAGON )
			di.DC->SelectObject( di.fill_brush );

		if( holew != 0)
		{
			di.DC->SelectObject( di.erase_brush );
			di.DC->SelectObject( di.erase_pen );

			di.DC->Ellipse( i.x - holew/2, i.y - holew/2, i.x + holew/2, i.y + holew/2 );

			di.DC->SelectObject( di.fill_brush );
			di.DC->SelectObject( di.line_pen );
		}
	}
}

void CDLE_OCTAGON::_DrawClearance(CDrawInfo const &di) const
{
	int sz = w/2 + clearancew;

	di.DC->SelectObject( di.erase_brush );
	di.DC->SelectObject( di.erase_pen );

	POINT pt[8];
	double angle = pi/8.0;
	for( int iv=0; iv<8; iv++ )
	{
		pt[iv].x = i.x + sz * cos(angle);
		pt[iv].y = i.y + sz * sin(angle);
		angle += pi/4.0;
	}

	di.DC->Polygon( pt, 8 );

	di.DC->SelectObject( di.fill_brush );
	di.DC->SelectObject( di.line_pen );
}

void CDLE_OCTAGON::_DrawThermalRelief(CDrawInfo const &di) const
{
	CFreePcbDoc * doc = theApp.m_Doc;

	int conn_tracew = doc->m_thermal_width / 2540;
	int therm_clearance = doc->m_thermal_clearance / 2540;

	int sz = w/2 + therm_clearance;

	di.DC->SelectObject( di.erase_brush );
	di.DC->SelectObject( di.erase_pen );

	di.DC->Ellipse( i.x - sz, i.y - sz, i.x + sz, i.y + sz );

	di.DC->SelectObject( di.fill_brush );

	CPen pen( PS_SOLID, conn_tracew, di.layer_color[1] );
	sz -= (sz * 3) / 10; // 1/sqrt(2)

	di.DC->SelectObject( &pen );
	di.DC->MoveTo( i.x - sz, i.y + sz );
	di.DC->LineTo( i.x + sz, i.y - sz );
	di.DC->MoveTo( i.x - sz, i.y - sz );
	di.DC->LineTo( i.x + sz, i.y + sz );

	di.DC->SelectObject( di.line_pen );
}

