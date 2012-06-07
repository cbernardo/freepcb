#include "stdafx.h"
#include "dle_hole.h"

extern CFreePcbApp theApp;

// hole, shown as circle
void CDLE_HOLE::_Draw(CDrawInfo &di, bool)
{
	int size_of_2_pixels = dlist->m_scale * 2;

	if( onScreen() )
    {
    	if( w > size_of_2_pixels )
		{
    		di.DC->Ellipse( i.x - w/2, i.y - w/2, i.x + w/2, i.y + w/2 );
		}
    }
}

/* void CDLE_HOLE::_DrawClearance(CDrawInfo const &di) const
{
	CFreePcbDoc * doc = theApp.m_Doc;

	// Pick the larger of the two clearances
	int sz = doc->m_hole_clearance / 2540;
	if (clearancew > sz) sz = clearancew;

	sz += w/2;	// Add hole width

	di.DC->SelectObject( di.erase_brush );
	di.DC->SelectObject( di.erase_pen );

	di.DC->Ellipse( i.x - sz, i.y - sz, i.x + sz, i.y + sz );

	di.DC->SelectObject( di.fill_brush );
	di.DC->SelectObject( di.line_pen );
}
*/

int CDLE_HOLE::_getBoundingRect(CRect &rect)
{
	CFreePcbDoc * doc = theApp.m_Doc;

	// Pick the larger of the two clearances
	int sz = doc->m_hole_clearance / 2540;
	if (clearancew > sz) sz = clearancew;

	sz += w/2;	// Add hole width

	rect.left   = i.x - sz;
	rect.right  = i.x + sz;
	rect.top    = i.y + sz;
	rect.bottom = i.y - sz;

	return 1;
}
