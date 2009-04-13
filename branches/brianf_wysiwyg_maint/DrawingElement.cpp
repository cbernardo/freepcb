#include "stdafx.h"
#include "DrawingElement.h"


void dl_element::Draw(CDrawInfo const &di) const
{
	if( visible && dlist->m_vis[ orig_layer ] ) _Draw(di);
}


void dl_element::DrawThermalRelief(CDrawInfo const &di) const
{
	if( visible && dlist->m_vis[ orig_layer ] ) _DrawThermalRelief(di);
}


int dl_element::isHit(CPoint const &point) const
{
	// don't select anything on an invisible layer or element
	if( visible && dlist->m_vis[layer] ) return _isHit(point);
	else return 0;
}


void dl_element::Remove(void)
{
	if( this == NULL ) return;

	dlist->Remove(this);
}



int CDLE_Symmetric::onScreen(void) const
{
	int sz = w/2;
	return (    i.x-sz < dlist->m_max_x && i.x+sz > dlist->m_org_x
	         && i.y-sz < dlist->m_max_y && i.y+sz > dlist->m_org_y );
}

int CDLE_Symmetric::_getBoundingRect(CRect &rect) const
{
	int sz = w/2 + clearancew;

	rect.left   = i.x - sz;
	rect.right  = i.x + sz;
	rect.top    = i.y + sz;
	rect.bottom = i.y - sz;

	return 1;
}



int CDLE_Rectangular::onScreen(void) const
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

	return (    _xi < dlist->m_max_x && _xf > dlist->m_org_x
	         && _yi < dlist->m_max_y && _yf > dlist->m_org_y );
}


int CDLE_Rectangular::_getBoundingRect(CRect &rect) const
{
	int sz = clearancew;

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

	rect.left   = _xi - sz;
	rect.right  = _xf + sz;
	rect.top    = _yi - sz;
	rect.bottom = _yf + sz;

	return 1;
}


void CDLE_Rectangular::_DrawThermalRelief(CDrawInfo const &di) const
{
	CFreePcbDoc * doc = theApp.m_Doc;

	int conn_tracew = doc->m_thermal_width / 2540;
	int therm_clearance = doc->m_thermal_clearance / 2540;

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

	di.DC->SelectObject( di.erase_brush );
	di.DC->SelectObject( di.erase_pen );

	di.DC->Rectangle( _xi - therm_clearance, _yi - therm_clearance, _xf + therm_clearance, _yf + therm_clearance);

	di.DC->SelectObject( di.fill_brush );

	CPen pen( PS_SOLID, conn_tracew, di.layer_color[1] );

	di.DC->SelectObject( &pen );
	di.DC->MoveTo( (_xi + _xf) / 2, _yi - therm_clearance );
	di.DC->LineTo( (_xi + _xf) / 2, _yf + therm_clearance );
	di.DC->MoveTo( _xi - therm_clearance, (_yi + _yf) / 2 );
	di.DC->LineTo( _xf + therm_clearance, (_yi + _yf) / 2 );

	di.DC->SelectObject( di.line_pen );
}
