#include "stdafx.h"
#include "dle_line.h"

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
// AMW r272 				CPen pen( PS_SOLID, linew + PCBU_PER_WU / 256 + 3*(int)dlist->m_scale, di.layer_color[0] );
				CPen pen( PS_SOLID, linew + PCBU_PER_WU / 1024 + 3*(int)dlist->m_scale, di.layer_color[0] );
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
