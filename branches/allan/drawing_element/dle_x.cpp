#include "stdafx.h"
#include "dle_x.h"

// X
void CDLE_X::_Draw(CDrawInfo const &di) const
{
	int w2 = w/2;
	int _xi = i.x - w2;
	int _xf = i.x + w2;
	int _yi = i.y - w2;
	int _yf = i.y + w2;

	if(   _xi < dlist->m_max_x && _xf > dlist->m_org_x
	   && _yi < dlist->m_max_y && _yf > dlist->m_org_y )
	{
		di.DC->MoveTo( _xi, _yi );
		di.DC->LineTo( _xf, _yf );
		di.DC->MoveTo( _xf, _yi );
		di.DC->LineTo( _xi, _yf );

		di.nlines += 2;
	}
}
