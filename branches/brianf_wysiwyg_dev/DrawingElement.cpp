#include "stdafx.h"
#include "DrawingElement.h"

extern CFreePcbApp theApp;
const double pi = 3.14159265359;

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


// line segment with round end-caps
void CDLE_LINE::__Draw(CDrawInfo const &di, int mode) const
{
	int width = w/2 + clearancew;

	// only draw line segments which are in viewport
	// viewport bounds, enlarged to account for line thickness
	int Yb = dlist->m_org_y - width;		// y-coord of viewport top
	int Yt = dlist->m_max_y + width;		// y-coord of bottom
	int Xl = dlist->m_org_x - width;		// x-coord of left edge
	int Xr = dlist->m_max_x + width;		// x-coord of right edge
    int xi = i.x;
    int yi = i.y;

	// line segment from (xi,yi) to (f.x,f.y)
	int draw_flag = 0;

	// now test for all conditions where drawing is necessary
	if( xi == f.x )
	{
		// vertical line
		if( yi < f.y )
		{
			// upward
			if( yi<=Yt && f.y>=Yb && xi<=Xr && xi>=Xl )
				draw_flag = 1;
		}
		else
		{
			// downward
			if( f.y<=Yt && yi>=Yb && xi<=Xr && xi>=Xl )
				draw_flag = 1;
		}
	}
	else if( yi==f.y )
	{
		// horizontal line
		if( xi<f.x )
		{
			// rightward
			if( xi<=Xr && f.x>=Xl && yi<=Yt && yi>=Yb )
				draw_flag = 1;
		}
		else
		{
			// leftward
			if( f.x<=Xr && xi>=Xl && yi<=Yt && yi>=Yb )
				draw_flag = 1;
		}
	}
	else if( !((xi<Xl&&f.x<Xl)||(xi>Xr&&f.x>Xr)||(yi<Yb&&f.y<Yb)||(yi>Yt&&f.y>Yt)) )
	{
		// oblique line
		// not entirely above or below or right or left of viewport
		// get slope b and intercept a so that y=a+bx
		double b = (double)(f.y-yi)/(f.x-xi);
		int a = yi - int(b*xi);
		// now test for line in viewport
		if( abs((f.y-yi)*(Xr-Xl)) > abs((f.x-xi)*(Yt-Yb)) )
		{
			// line is more vertical than window diagonal
			int x1 = int((Yb-a)/b);
			if( x1>=Xl && x1<=Xr)
				draw_flag = 1;		// intercepts bottom of screen
			else
			{
				int x2 = int((Yt-a)/b);
				if( x2>=Xl && x2<=Xr )
					draw_flag = 1;	// intercepts top of screen
			}
		}
		else
		{
			// line is more horizontal than window diagonal
			int y1 = int(a + b*Xl);
			if( y1>=Yb && y1<=Yt )
				draw_flag = 1;		// intercepts left edge of screen
			else
			{
				int y2 = a + int(b*Xr);
				if( y2>=Yb && y2<=Yt )
					draw_flag = 1;	// intercepts right edge of screen
			}
		}

	}
	// now draw the line segment if not clipped
	if( draw_flag )
	{
		int linew = w;

		if( mode )
		{
            // Drawing, not erasing - apply special "treatments"
			if( layer == LAY_RAT_LINE )
			{
				CPen pen( PS_SOLID, linew + PCBU_PER_WU / 1024 + (int)dlist->m_scale, di.layer_color[0] );
				di.DC->SelectObject( &pen );
				di.DC->MoveTo( xi, yi );
				di.DC->LineTo( f.x, f.y );
				di.DC->SelectObject( di.line_pen );
			}
			else if( layer == LAY_HILITE )
			{
				CPen pen( PS_SOLID, linew + PCBU_PER_WU / 256 + 3*(int)dlist->m_scale, di.layer_color[0] );
				CPen *old_pen = di.DC_Master->SelectObject( &pen );
				di.DC_Master->MoveTo( xi, yi );
				di.DC_Master->LineTo( f.x, f.y );
				di.DC_Master->SelectObject( old_pen );
			}

			if( linew < 0 ) return;
		}
		else
		{
			linew += clearancew*2;
		}

		CPen pen( PS_SOLID, linew, di.layer_color[mode] );
		di.DC->SelectObject( &pen );
		di.DC->MoveTo( xi, yi );
		di.DC->LineTo( f.x, f.y );
		di.DC->SelectObject( di.line_pen );

		di.nlines++;
	}
}

void CDLE_LINE::_Draw(CDrawInfo const &di) const
{
	__Draw(di, 1);
}

void CDLE_LINE::_DrawClearance(CDrawInfo const &di) const
{
	__Draw(di, 0);
}

int CDLE_LINE::_isHit(CPoint const &point) const
{
	// found selection line, test for hit (within 4 pixels or line width+3 mils )
//**			int w = max( w/2+3*DU_PER_MIL, int(4.0*m_scale) );
	int hit_w = max( w/2, int(4.0 * dlist->m_scale) );
	return TestLineHit( i.x, i.y, f.x, f.y, point.x, point.y, hit_w );
}

int CDLE_LINE::_getBoundingRect(CRect &rect) const
{
	int linew = w/2 + clearancew;

	if (i.x < f.x)
	{
		rect.left  = i.x - linew;
		rect.right = f.x + linew;

		if (i.y < f.y)
		{
			rect.top    = i.y - linew;
			rect.bottom = f.y + linew;
		}
		else
		{
			rect.top    = i.y + linew;
			rect.bottom = f.y - linew;
		}
	}
	else
	{
		rect.left  = f.x - linew;
		rect.right = i.x + linew;

		if (i.y < f.y)
		{
			rect.top    = f.y + linew;
			rect.bottom = i.y - linew;
		}
		else
		{
			rect.top    = f.y - linew;
			rect.bottom = i.y + linew;
		}
	}

	return 1;
}




// filled circle
void CDLE_CIRC::_Draw(CDrawInfo const &di) const
{
	if( onScreen() )
	{
		di.DC->Ellipse( i.x - w/2, i.y - w/2, i.x + w/2, i.y + w/2 );
	}
}

void CDLE_CIRC::_DrawClearance(CDrawInfo const &di) const
{
	int sz = w/2 + clearancew;

	di.DC->SelectObject( di.erase_brush );
	di.DC->SelectObject( di.erase_pen );

	di.DC->Ellipse( i.x - sz, i.y - sz, i.x + sz, i.y + sz );

	di.DC->SelectObject( di.fill_brush );
	di.DC->SelectObject( di.line_pen );
}

void CDLE_CIRC::_DrawThermalRelief(CDrawInfo const &di) const
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
	sz -= (sz * 3) / 10; // sz *= 1/sqrt(2)

	di.DC->SelectObject( &pen );
	di.DC->MoveTo( i.x - sz, i.y + sz );
	di.DC->LineTo( i.x + sz, i.y - sz );
	di.DC->MoveTo( i.x - sz, i.y - sz );
	di.DC->LineTo( i.x + sz, i.y + sz );

	di.DC->SelectObject( di.line_pen );
}

int CDLE_CIRC::_isHit(CPoint const &point) const
{
	int hit_w = w/2;
	int d = Distance( i.x, i.y, point.x, point.y );

	return ( d < hit_w );
}


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




// filled square
void CDLE_SQUARE::_Draw(CDrawInfo const &di) const
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


void CDLE_SQUARE::_DrawClearance(CDrawInfo const &di) const
{
	int sz = w/2 + clearancew;

	di.DC->SelectObject( di.erase_brush );
	di.DC->SelectObject( di.erase_pen );

	di.DC->Rectangle( i.x - sz, i.y - sz, i.x + sz, i.y + sz );

	di.DC->SelectObject( di.fill_brush );
	di.DC->SelectObject( di.line_pen );
}

void CDLE_SQUARE::_DrawThermalRelief(CDrawInfo const &di) const
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


// filled rounded rectangle
void CDLE_RRECT::_Draw(CDrawInfo const &di) const
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

void CDLE_RRECT::_DrawClearance(CDrawInfo const &di) const
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



// hole, shown as circle
void CDLE_HOLE::_Draw(CDrawInfo const &di) const
{
	int size_of_2_pixels = dlist->m_scale * 2;

	if( onScreen() )
    {
    	if( w>size_of_2_pixels )
    		di.DC->Ellipse( i.x - w/2, i.y - w/2, i.x + w/2, i.y + w/2 );
    }
}

void CDLE_HOLE::_DrawClearance(CDrawInfo const &di) const
{
	CFreePcbDoc * doc = theApp.m_Doc;

	int sz = w/2 + doc->m_hole_clearance / 2540;

	di.DC->SelectObject( di.erase_brush );
	di.DC->SelectObject( di.erase_pen );

	di.DC->Ellipse( i.x - sz, i.y - sz, i.x + sz, i.y + sz );

	di.DC->SelectObject( di.fill_brush );
	di.DC->SelectObject( di.line_pen );
}

int CDLE_HOLE::_getBoundingRect(CRect &rect) const
{
	int sz = w/2 + clearancew;

	rect.left   = i.x - sz;
	rect.right  = i.x + sz;
	rect.top    = i.y + sz;
	rect.bottom = i.y - sz;

	return 1;
}



// circle outline
void CDLE_HOLLOW_CIRC::_Draw(CDrawInfo const &di) const
{
	if( onScreen() )
	{
		di.DC->SelectObject( GetStockObject( HOLLOW_BRUSH ) );

		di.DC->Ellipse( i.x - w/2, i.y - w/2, i.x + w/2, i.y + w/2 );

		di.DC->SelectObject( di.fill_brush );
	}
}

void CDLE_HOLLOW_CIRC::_DrawClearance(CDrawInfo const &di) const
{
}

int CDLE_HOLLOW_CIRC::_isHit(CPoint const &point) const
{
	// test for hit (within 3 mils or 4 pixels)
//**	int dr = max( 3*DU_PER_MIL, int(4.0*m_scale) );
	int dr = 4.0 * dlist->m_scale;
	int hit_w = w/2;
	int d = Distance( i.x, i.y, point.x, point.y );

	return ( abs(hit_w-d) < dr );
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
void CDLE_RECT_X::_Draw(CDrawInfo const &di) const
{
	if( onScreen() )
	{
		di.DC->MoveTo( i.x, i.y );
		di.DC->LineTo( f.x, i.y );
		di.DC->LineTo( f.x, f.y );
		di.DC->LineTo( i.x, f.y );
		di.DC->LineTo( i.x, i.y );
		di.DC->MoveTo( i.x, i.y );
		di.DC->LineTo( f.x, f.y );
		di.DC->MoveTo( i.x, f.y );
		di.DC->LineTo( f.x, i.y );

		di.nlines += 4;
	}
}

int CDLE_RECT_X::_isHit(CPoint const &point) const
{
	return (   ( (point.x>i.x && point.x<f.x) || (point.x<i.x && point.x>f.x) )
	        && ( (point.y>i.y && point.y<f.y) || (point.y<i.y && point.y>f.y) ) );
}


//-----------------------------------------------------------------------------


// shape to highlight a point
void CDLE_POINT::_Draw(CDrawInfo const &di) const
{
}


//-----------------------------------------------------------------------------


// arcs
int CDLE_ARC::onScreen(void) const
{
	return ( (    (f.x >= i.x && i.x < dlist->m_max_x && f.x > dlist->m_org_x)
	         ||   (f.x <  i.x && f.x < dlist->m_max_x && i.x > dlist->m_org_x) )
	         && ( (f.y >= i.y && i.y < dlist->m_max_y && f.y > dlist->m_org_y)
	         ||   (f.y <  i.y && f.y < dlist->m_max_y && i.y > dlist->m_org_y) ) );
}

int CDLE_ARC::_isHit(CPoint const &point) const
{
    // found selection rectangle, test for hit
    return (   ( (point.x>i.x && point.x<f.x) || (point.x<i.x && point.x>f.x) )
	        && ( (point.y>i.y && point.y<f.y) || (point.y<i.y && point.y>f.y) ) );
}


// arc with clockwise curve
void CDLE_ARC_CW::_Draw(CDrawInfo const &di) const
{
	if( onScreen() )
	{
		CPen pen( PS_SOLID, w, di.layer_color[1] );
		di.DC->SelectObject( &pen );
		DrawArc( di.DC, DL_ARC_CW, i.x, i.y, f.x, f.y );
		di.DC->SelectObject( di.line_pen );
	}
}


// arc with counter-clockwise curve
void CDLE_ARC_CCW::_Draw(CDrawInfo const &di) const
{
	if( onScreen() )
	{
		CPen pen( PS_SOLID, w, di.layer_color[1] );
		di.DC->SelectObject( &pen );
		DrawArc( di.DC, DL_ARC_CCW, i.x, i.y, f.x, f.y );
		di.DC->SelectObject( di.line_pen );
	}
}


//-----------------------------------------------------------------------------


// circle and X
void CDLE_CENTROID::_Draw(CDrawInfo const &di) const
{
	// x,y are center coords; w = width;
	// f.x,f.y define arrow end-point for P&P orientation
	if( onScreen() )
	{
        int w2 = w/2;
        int xi = i.x - w2;
        int yi = i.y - w2;
        int _xf = i.x + w2;
        int _yf = i.y + w2;

		di.DC->SelectObject( GetStockObject( HOLLOW_BRUSH ) );
		di.DC->Ellipse( i.x - w2/2, i.y - w2/2, i.x + w2/2, i.y + w2/2 );
		di.DC->SelectObject( di.fill_brush );

		di.DC->MoveTo( xi, yi );
		di.DC->LineTo( _xf, _yf );
		di.DC->MoveTo( _xf, yi );
		di.DC->LineTo( xi, _yf );
		di.DC->MoveTo( i.x, i.y );	// p&p arrow
		di.DC->LineTo( f.x, f.y );
		if( i.y == f.y )
		{
			// horizontal arrow
			di.DC->LineTo( f.x - (f.x - i.x)/4,	f.y + w/4 );
			di.DC->LineTo( f.x - (f.x - i.x)/4,	f.y - w/4 );
			di.DC->LineTo( f.x,                 f.y );
		}
		else if( i.x == f.x )
		{
			// vertical arrow
			di.DC->LineTo( f.x + w/4, f.y - (f.y - i.y)/4 );
			di.DC->LineTo( f.x - w/4, f.y - (f.y - i.y)/4 );
			di.DC->LineTo( f.x,       f.y );
		}
		else
			ASSERT(0);

		di.nlines += 2;
	}
}


//-----------------------------------------------------------------------------


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
