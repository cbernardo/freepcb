// DisplayList.cpp : implementation of class CDisplayList
//
// this is a linked-list of display elements
// each element is a primitive graphics object such as a line-segment,
// circle, annulus, etc.
//
#include "stdafx.h" 
#include <math.h>

// dimensions passed to DisplayList from the application are in PCBU (i.e. nm)
// since the Win95/98 GDI uses 16-bit arithmetic, PCBU must be scaled to DU (i.e. mils) 
//
//#define PCBU_PER_DU		PCBU_PER_WU
//#define MIL_PER_DU		NM_PER_MIL/PCBU_PER_DU	// conversion from mils to display units
//#define DU_PER_MIL		PCBU_PER_DU/NM_PER_MIL	// conversion from display units to mils

#define DL_MAX_LAYERS	32
#define HILITE_POINT_W	10	// size/2 of selection box for points (mils)  

// constructor
//
CDisplayList::CDisplayList( int pcbu_per_wu )
{
	m_pcbu_per_wu = pcbu_per_wu;
	// create lists for all layers
	for( int layer=0; layer<MAX_LAYERS; layer++ )
	{
		// linked list for layer
		m_start[layer].prev = 0;		// first element
		m_start[layer].next = &m_end[layer];
		m_start[layer].magic = 0;
		m_end[layer].next = 0;			// last element
		m_end[layer].prev = &m_start[layer];
		m_end[layer].magic = 0;
		// default colors, these should be overwritten with actual colors
		m_rgb[layer][0] = layer*63;			// R 
		m_rgb[layer][1] = (layer/4)*127;	// G
		m_rgb[layer][2] = (layer/8)*63;		// B
		// default order
		m_layer_in_order[layer] = layer;	// will be drawn from highest to lowest
		m_order_for_layer[layer] = layer;
	}
	// miscellaneous
	m_drag_flag = 0;
	m_drag_num_lines = 0;
	m_drag_line_pt = 0;
	m_drag_num_ratlines = 0;
	m_drag_ratline_start_pt = 0;
	m_drag_ratline_end_pt = 0;
	m_drag_ratline_width = 0;
	m_cross_hairs = 0;
	m_visual_grid_on = 0;
	m_visual_grid_spacing = 0;
	m_inflection_mode = IM_NONE;
}

// destructor
//
CDisplayList::~CDisplayList()
{
	RemoveAll();
}

void CDisplayList::RemoveAllFromLayer( int layer )
{
	// traverse list for layer, removing all elements
	while( m_end[layer].prev != &m_start[layer] )
		Remove( m_end[layer].prev );
}

void CDisplayList::RemoveAll()
{
	// traverse lists for all layers, removing all elements
	for( int layer=0; layer<MAX_LAYERS; layer++ )
	{
		RemoveAllFromLayer( layer );
	}
	if( m_drag_line_pt )
	{
		free( m_drag_line_pt );
		m_drag_line_pt = 0;
	}
	if( m_drag_ratline_start_pt )
	{
		free( m_drag_ratline_start_pt );
		m_drag_ratline_start_pt = 0;
	}
	if( m_drag_ratline_end_pt )
	{
		free( m_drag_ratline_end_pt );
		m_drag_ratline_end_pt = 0;
	}
}

// Set conversion parameters between world coords (used by elements in
// display list) and window coords (pixels)
//
// enter with:
//	client_r = window client rectangle (pixels)
//	screen_r = window client rectangle in screen coords (pixels)
//	pane_org_x = starting x-coord of PCB drawing area in client rect (pixels)
//  pane_bottom_h = height of bottom pane (pixels)
//	pcb units per pixel = nm per pixel
//	org_x = x-coord of left edge of drawing area (pcb units)
//	org_y = y-coord of bottom edge of drawing area (pcb units)
//
// note that the actual scale factor is set by the arguments to 
// SetWindowExt and SetViewportExt, and may be slightly different for x and y
//
void CDisplayList::SetMapping( CRect *client_r, CRect *screen_r, int pane_org_x, int pane_bottom_h, 
							double pcbu_per_pixel, int org_x, int org_y )
{
	m_client_r = client_r;				// pixels
	m_screen_r = screen_r;				// pixels
	m_pane_org_x = pane_org_x;			// pixels
	m_bottom_pane_h = pane_bottom_h;	// pixels
	m_pane_org_y = client_r->bottom - pane_bottom_h;	// pixels
	m_scale = pcbu_per_pixel/m_pcbu_per_wu;	// world units per pixel
	m_org_x = org_x/m_pcbu_per_wu;		// world units
	m_org_y = org_y/m_pcbu_per_wu;		// world units

	//now set extents
	double rw = m_client_r.Width();	// width of client area (pixels)
	double rh = m_client_r.Height();	// height of client area (pixels)
	double ext_x = rw*pcbu_per_pixel/m_pcbu_per_wu;	// width in WU
	double ext_y = rh*pcbu_per_pixel/m_pcbu_per_wu;	// height in WU
	int div = 1, mult = 1;
	if( m_pcbu_per_wu >=25400 )
	{
		// this is necessary for Win95/98 (16-bit GDI)
		int ext_max = max( ext_x, ext_y );
		if( ext_max > 30000 )
		div = ext_max/15000;
	}
	else
		mult = m_pcbu_per_wu;
	
	if( ext_x*mult/div > INT_MAX )
		ASSERT(0);
	if( ext_y*mult/div > INT_MAX )
		ASSERT(0);
	w_ext_x = ext_x*mult/div;
	v_ext_x = rw*mult/div;
	w_ext_y = ext_y*mult/div;
	v_ext_y = -rh*mult/div;
	m_wu_per_pixel_x = (double)w_ext_x/v_ext_x;		// actual ratios
	m_wu_per_pixel_y = (double)w_ext_y/v_ext_y;
	m_pcbu_per_pixel_x = m_wu_per_pixel_x * m_pcbu_per_wu;		
	m_pcbu_per_pixel_y = m_wu_per_pixel_y * m_pcbu_per_wu;
	m_max_x = m_org_x + m_wu_per_pixel_x*(client_r->right-pane_org_x) + 2;	// world units
	m_max_y = m_org_y - m_wu_per_pixel_y*client_r->bottom + 2;				// world units
}

// add graphics element used for selection
//
dl_element * CDisplayList::AddSelector( id id, void * ptr, int layer, int gtype, int visible,
							   int w, int holew, int x, int y, int xf, int yf, int xo, int yo,
							   int radius )
{
	dl_element * test = Add( id, ptr, LAY_SELECTION, gtype, visible,
							   w, holew, x, y, xf, yf, xo, yo, radius, layer );
	test->layer = layer;
	return test;
}

// Add entry to end of list, returns pointer to element created.
//
// Dimensional units for input parameters are PCBU
//
dl_element * CDisplayList::Add( id id, void * ptr, int layer, int gtype, int visible,
							   int w, int holew, int x, int y, int xf, int yf, int xo, int yo,
							   int radius, int orig_layer )
{
	// create new element and link into list
	dl_element * new_element = new dl_element;
	new_element->prev = m_end[layer].prev;
	new_element->next = &m_end[layer];
	new_element->prev->next = new_element;
	new_element->next->prev = new_element;
	// now copy data from entry into element
	new_element->magic = DL_MAGIC;
	new_element->id = id;
	new_element->ptr = ptr;
	new_element->gtype = gtype;
	new_element->visible = visible;
	new_element->w = w/m_pcbu_per_wu;
	new_element->holew = holew/m_pcbu_per_wu;
	new_element->x = x/m_pcbu_per_wu;
	new_element->xf = xf/m_pcbu_per_wu;
	new_element->y = y/m_pcbu_per_wu;
	new_element->yf = yf/m_pcbu_per_wu;
	new_element->x_org = xo/m_pcbu_per_wu;
	new_element->y_org = yo/m_pcbu_per_wu;
	new_element->radius = radius/m_pcbu_per_wu;
	new_element->sel_vert = 0;
	new_element->layer = layer;
	new_element->orig_layer = orig_layer;
	new_element->dlist = this;
	return new_element;
}

// set element parameters in PCBU
//
void CDisplayList::Set_gtype( dl_element * el, int gtype )
{ 
	if( el)
		el->gtype = gtype; 
}
void CDisplayList::Set_visible( dl_element * el, int visible )
{ 
	if( el)
		el->visible = visible; 
}
void CDisplayList::Set_sel_vert( dl_element * el, int sel_vert )
{ 
	if( el)
		el->sel_vert = sel_vert; 
}
void CDisplayList::Set_w( dl_element * el, int w )
{ 
	if( el)
		el->w = w/m_pcbu_per_wu; 
}
void CDisplayList::Set_holew( dl_element * el, int holew )
{ 
	if( el)
		el->holew = holew/m_pcbu_per_wu; 
}
void CDisplayList::Set_x_org( dl_element * el, int x_org )
{ 
	if( el)
		el->x_org = x_org/m_pcbu_per_wu; 
} 
void CDisplayList::Set_y_org( dl_element * el, int y_org )
{ 
	if( el)
		el->y_org = y_org/m_pcbu_per_wu; 
}
void CDisplayList::Set_x( dl_element * el, int x )
{ 
	if( el)
		el->x = x/m_pcbu_per_wu; 
}
void CDisplayList::Set_y( dl_element * el, int y )
{ 
	if( el)
		el->y = y/m_pcbu_per_wu; 
}
void CDisplayList::Set_xf( dl_element * el, int xf )
{ 
	if( el)
		el->xf = xf/m_pcbu_per_wu; 
}
void CDisplayList::Set_yf( dl_element * el, int yf )
{ 
	if( el)
		el->yf = yf/m_pcbu_per_wu; 
}
void CDisplayList::Set_layer( dl_element * el, int layer )
{ 
	if( el)
		el->layer = layer; 
}
void CDisplayList::Set_radius( dl_element * el, int radius )
{ 
	if( el)
		el->radius = radius/m_pcbu_per_wu; 
}
void CDisplayList::Set_id( dl_element * el, id * id )
{ 
	if( el)
		el->id = *id; 
}

void CDisplayList::Move( dl_element * el, int dx, int dy )
{
	el->x += dx;
	el->y += dy;
	el->x_org += dx;
	el->y_org += dy;
	el->xf += dx;
	el->yf += dy;
}


// get element parameters in PCBU
//
void * CDisplayList::Get_ptr( dl_element * el ) { return el->ptr; }
int CDisplayList::Get_gtype( dl_element * el ) { return el->gtype; }
int CDisplayList::Get_visible( dl_element * el ) { return el->visible; }
int CDisplayList::Get_sel_vert( dl_element * el ) { return el->sel_vert; }
int CDisplayList::Get_w( dl_element * el ) { return el->w*m_pcbu_per_wu; }
int CDisplayList::Get_holew( dl_element * el ) { return el->holew*m_pcbu_per_wu; }
int CDisplayList::Get_x_org( dl_element * el ) { return el->x_org*m_pcbu_per_wu; } 
int CDisplayList::Get_y_org( dl_element * el ) { return el->y_org*m_pcbu_per_wu; }
int CDisplayList::Get_x( dl_element * el ) { return el->x*m_pcbu_per_wu; }
int CDisplayList::Get_y( dl_element * el ) { return el->y*m_pcbu_per_wu; }	
int CDisplayList::Get_xf( dl_element * el ) { return el->xf*m_pcbu_per_wu; }
int CDisplayList::Get_yf( dl_element * el ) { return el->yf*m_pcbu_per_wu; }
int CDisplayList::Get_radius( dl_element * el ) { return el->radius*m_pcbu_per_wu; }
int CDisplayList::Get_layer( dl_element * el ) { return el->layer; }
id CDisplayList::Get_id( dl_element * el ) { return el->id; }

void CDisplayList::Get_Endpoints(CPoint *cpi, CPoint *cpf) 
{ 
	cpi->x = m_drag_xi*m_pcbu_per_wu; cpi->y = m_drag_yi*m_pcbu_per_wu;
	cpf->x = m_drag_xf*m_pcbu_per_wu; cpf->y = m_drag_yf*m_pcbu_per_wu;
}

// Remove element from list, return id
//
id CDisplayList::Remove( dl_element * element )
{
	if( !element )
	{
		id no_id;
		return no_id;
	}
	if( element->magic != DL_MAGIC )
	{
		ASSERT(0);
		id no_id;
		return no_id;
	}
	// remove links to this element
	element->next->prev = element->prev;
	element->prev->next = element->next;
	// destroy element
	id el_id = element->id;
	delete( element );
	return el_id;
}

// Draw the display list using device DC or memory DC
//
void CDisplayList::Draw( CDC * dDC )
{
	CDC * pDC;
	if( memDC )
		pDC = memDC;
	else
		pDC = dDC;

	pDC->SetBkColor( RGB(0, 0, 0) );

	// create pens and brushes
	CPen * old_pen;
	CPen black_pen( PS_SOLID, 1, RGB( 0, 0, 0 ) );
	CPen white_pen( PS_SOLID, 1, RGB( 255, 255, 255 ) );
	CPen grid_pen( PS_SOLID, 1, 
						RGB( m_rgb[LAY_VISIBLE_GRID][0], 
						     m_rgb[LAY_VISIBLE_GRID][1], 
						     m_rgb[LAY_VISIBLE_GRID][2] ) );
	CPen backgnd_pen( PS_SOLID, 1, 
						RGB( m_rgb[LAY_BACKGND][0],
						     m_rgb[LAY_BACKGND][1], 
						     m_rgb[LAY_BACKGND][2] ) );
	CPen board_pen( PS_SOLID, 1, 
						RGB( m_rgb[LAY_BOARD_OUTLINE][0],
						     m_rgb[LAY_BOARD_OUTLINE][1], 
						     m_rgb[LAY_BOARD_OUTLINE][2] ) );
	CBrush * old_brush;
	CBrush black_brush( RGB( 0, 0, 0 ) );
	CBrush backgnd_brush( RGB( m_rgb[LAY_BACKGND][0],
								m_rgb[LAY_BACKGND][1],
								m_rgb[LAY_BACKGND][2] ) );
	
	// paint it background color
	old_brush = pDC->SelectObject( &backgnd_brush );
	old_pen = pDC->SelectObject( &black_pen );
	CRect client_rect;
	client_rect.left = m_org_x;
	client_rect.right = m_max_x;
	client_rect.bottom = m_org_y;
	client_rect.top = m_max_y;
	pDC->Rectangle( &client_rect );

	// visual grid
	if( m_visual_grid_on && (m_visual_grid_spacing/m_scale)>5 && m_vis[LAY_VISIBLE_GRID] )
	{
		int startix = m_org_x/m_visual_grid_spacing;
		int startiy = m_org_y/m_visual_grid_spacing;
		double start_grid_x = startix*m_visual_grid_spacing;
		double start_grid_y = startiy*m_visual_grid_spacing;
		for( double ix=start_grid_x; ix<m_max_x; ix+=m_visual_grid_spacing )
		{
			for( double iy=start_grid_y; iy<m_max_y; iy+=m_visual_grid_spacing )
			{
				pDC->SetPixel( ix, iy, 
					RGB( m_rgb[LAY_VISIBLE_GRID][0], 
						 m_rgb[LAY_VISIBLE_GRID][1], 
						 m_rgb[LAY_VISIBLE_GRID][2] ) );
			}
		}
	}

	// now traverse the lists, starting with the layer in the last element 
	// of the m_order[] array
	int nlines = 0;
	int size_of_2_pixels = 2*m_scale;
	for( int order=(MAX_LAYERS-1); order>=0; order-- )
	{
		int layer = m_layer_in_order[order];
		if( !m_vis[layer] || layer == LAY_SELECTION )
			continue;

		CPen line_pen( PS_SOLID, 1, RGB( m_rgb[layer][0], m_rgb[layer][1], m_rgb[layer][2] ) );
		CBrush fill_brush( RGB( m_rgb[layer][0], m_rgb[layer][1], m_rgb[layer][2] ) );
		pDC->SelectObject( &line_pen );
		pDC->SelectObject( &fill_brush );
		dl_element * el = &m_start[layer];
		while( el->next->next )
		{
			el = el->next;
			if( el->visible && m_vis[el->orig_layer] )
			{
				int xi = el->x;
				int xf = el->xf;
				int yi = el->y;
				int yf = el->yf;
				int w = el->w;
				if( el->gtype == DL_CIRC || el->gtype == DL_HOLLOW_CIRC )
				{
					if( xi-w/2 < m_max_x && xi+w/2 > m_org_x 
						&& yi-w/2 < m_max_y && yi+w/2 > m_org_y )
					{
						if( el->gtype == DL_HOLLOW_CIRC )
							pDC->SelectObject( GetStockObject( HOLLOW_BRUSH ) );
						pDC->Ellipse( xi - w/2, yi - w/2, xi + w/2, yi + w/2 );
						if( el->gtype == DL_HOLLOW_CIRC )
							pDC->SelectObject( fill_brush );
					}
				}
				if( el->gtype == DL_HOLE )
				{
					if( xi-w/2 < m_max_x && xi+w/2 > m_org_x 
						&& yi-w/2 < m_max_y && yi+w/2 > m_org_y )
					{
						if( w>size_of_2_pixels )
							pDC->Ellipse( xi - w/2, yi - w/2, xi + w/2, yi + w/2 );
					}
				}
				else if( el->gtype == DL_CENTROID )
				{
					// x,y are center coords; w = width; 
					// xf,yf define arrow end-point for P&P orientation
					if( xi-w/2 < m_max_x && xi+w/2 > m_org_x 
						&& yi-w/2 < m_max_y && yi+w/2 > m_org_y )
					{
						pDC->SelectObject( GetStockObject( HOLLOW_BRUSH ) ); 
						pDC->Ellipse( xi - w/4, yi - w/4, xi + w/4, yi + w/4 );
						pDC->SelectObject( fill_brush );
						xi = el->x - el->w/2;
						xf = el->x + el->w/2;
						yi = el->y - el->w/2;
						yf = el->y + el->w/2;
						pDC->MoveTo( xi, yi );
						pDC->LineTo( xf, yf );
						pDC->MoveTo( xf, yi );
						pDC->LineTo( xi, yf );
						pDC->MoveTo( el->x, el->y );	// p&p arrow
						pDC->LineTo( el->xf, el->yf );	// 
						if( el->y == el->yf )   
						{
							// horizontal arrow
							pDC->LineTo( el->xf - (el->xf - el->x)/4,
										el->yf + w/4 );
							pDC->LineTo( el->xf - (el->xf - el->x)/4,
										el->yf - w/4 );
							pDC->LineTo( el->xf, el->yf );
						}
						else if( el->x == el->xf )  
						{
							// vertical arrow
							pDC->LineTo( el->xf + w/4, el->yf - (el->yf - el->y)/4 );
							pDC->LineTo( el->xf - w/4, el->yf - (el->yf - el->y)/4 );
							pDC->LineTo( el->xf, el->yf );
						}
						else
							ASSERT(0);
						int w_pp = el->w/10;
						nlines += 2;
					}
				}
				else if( el->gtype == DL_DONUT )
				{
					if( xi-w/2 < m_max_x && xi+w/2 > m_org_x 
						&& yi-w/2 < m_max_y && yi+w/2 > m_org_y )
					{
						int thick = (w - el->holew)/2;
						int ww = w - thick;	
						int holew = el->holew;
						int size_of_2_pixels = m_scale; 
						if( thick < size_of_2_pixels )
						{
							holew = w - 2*size_of_2_pixels;
							if( holew < 0 )
								holew = 0;
							thick = (w - holew)/2;
							ww = w - thick;
						}
						if( w-el->holew > 0 ) 
						{
							CPen pen( PS_SOLID, thick, RGB( m_rgb[layer][0], m_rgb[layer][1], m_rgb[layer][2] ) );
							pDC->SelectObject( &pen );
							pDC->SelectObject( &backgnd_brush );
							pDC->Ellipse( xi - ww/2, yi - ww/2, xi + ww/2, yi + ww/2 );
							pDC->SelectObject( line_pen );
							pDC->SelectObject( fill_brush );
						}
						else
						{
							CPen backgnd_pen( PS_SOLID, 1, 
								RGB( m_rgb[LAY_BACKGND][0],
								m_rgb[LAY_BACKGND][1], 
								m_rgb[LAY_BACKGND][2] ) );
							pDC->SelectObject( &backgnd_pen );
							pDC->SelectObject( &backgnd_brush );
							pDC->Ellipse( xi - holew/2, yi - holew/2, xi + holew/2, yi + holew/2 );
							pDC->SelectObject( line_pen );
							pDC->SelectObject( fill_brush );
						}
					}
				}
				else if( el->gtype == DL_SQUARE )
				{
					if( xi-w/2 < m_max_x && xi+w/2 > m_org_x 
						&& yi-w/2 < m_max_y && yi+w/2 > m_org_y )
					{
						int holew = el->holew;
						pDC->Rectangle( xi - w/2, yi - w/2, xi + w/2, yi + w/2 );
						if( holew )
						{
							pDC->SelectObject( &backgnd_brush );
							pDC->SelectObject( &backgnd_pen );
							pDC->Ellipse( xi - holew/2, yi - holew/2, 
								xi + holew/2, yi + holew/2 );
							pDC->SelectObject( fill_brush );
							pDC->SelectObject( line_pen );
						}
					}
				}
				else if( el->gtype == DL_OCTAGON || el->gtype == DL_HOLLOW_OCTAGON )
				{
					if( xi-w/2 < m_max_x && xi+w/2 > m_org_x 
						&& yi-w/2 < m_max_y && yi+w/2 > m_org_y )
					{
						const double pi = 3.14159265359;
						POINT pt[8];
						double angle = pi/8.0;
						for( int iv=0; iv<8; iv++ )
						{
							pt[iv].x = el->x + 0.5 * el->w * cos(angle);
							pt[iv].y = el->y + 0.5 * el->w * sin(angle);
							angle += pi/4.0;
						}
						int holew = el->holew;
						if( el->gtype == DL_HOLLOW_OCTAGON )
							pDC->SelectObject( GetStockObject( HOLLOW_BRUSH ) );
						pDC->Polygon( pt, 8 );
						if( el->gtype == DL_HOLLOW_OCTAGON )
							pDC->SelectObject( fill_brush );
						if( holew )
						{
							pDC->SelectObject( &backgnd_brush );
							pDC->SelectObject( &backgnd_pen );
							pDC->Ellipse( xi - holew/2, yi - holew/2, 
								xi + holew/2, yi + holew/2 );
							pDC->SelectObject( fill_brush );
							pDC->SelectObject( line_pen );
						}

					}
				}
				else if( el->gtype == DL_RECT )
				{
					if( xf < xi )
					{
						xf = xi;
						xi = el->xf;
					}
					if( yf < yi )
					{
						yf = yi;
						yi = el->yf;
					}
					if( xi < m_max_x && xf > m_org_x && yi < m_max_y && yf > m_org_y )
					{
						int holew = el->holew;
						pDC->Rectangle( xi, yi, xf, yf );
						if( holew )
						{
							pDC->SelectObject( &black_brush );
							pDC->SelectObject( &black_pen );
							pDC->Ellipse( el->x_org - holew/2, el->y_org - holew/2, 
								el->x_org + holew/2, el->y_org + holew/2 );
							pDC->SelectObject( fill_brush );
							pDC->SelectObject( line_pen );
						}
					}
				}
				else if( el->gtype == DL_HOLLOW_RECT )
				{
					if( xf < xi )
					{
						xf = xi;
						xi = el->xf;
					}
					if( yf < yi )
					{
						yf = yi;
						yi = el->yf;
					}
					if( xi < m_max_x && xf > m_org_x
						&& yi < m_max_y && yf > m_org_y )
					{
						pDC->MoveTo( xi, yi );
						pDC->LineTo( xf, yi );
						pDC->LineTo( xf, yf );
						pDC->LineTo( xi, yf );
						pDC->LineTo( xi, yi );
						nlines += 4;
					}
				}
				else if( el->gtype == DL_OVAL || el->gtype == DL_HOLLOW_OVAL )
				{
					if( xf < xi )
					{
						xf = xi;
						xi = el->xf;
					}
					if( yf < yi )
					{
						yf = yi;
						yi = el->yf;
					}
					if( xi < m_max_x && xf > m_org_x && yi < m_max_y && yf > m_org_y )
					{
						int h = abs(xf-xi);
						int v = abs(yf-yi);
						int r = min(h,v);
						if( el->gtype == DL_HOLLOW_OVAL )
							pDC->SelectObject( GetStockObject( HOLLOW_BRUSH ) );
						pDC->RoundRect( xi, yi, xf, yf, r, r );
						if( el->gtype == DL_HOLLOW_OVAL )
							pDC->SelectObject( fill_brush );
						int holew = el->holew;
						if( el->holew )
						{
							pDC->SelectObject( &black_brush );
							pDC->SelectObject( &black_pen );
							pDC->Ellipse( el->x_org - holew/2, el->y_org - holew/2, 
								el->x_org + holew/2, el->y_org + holew/2 );
							pDC->SelectObject( fill_brush );
							pDC->SelectObject( line_pen );
						}
					}
				}
				else if( el->gtype == DL_RRECT || el->gtype == DL_HOLLOW_RRECT )
				{
					if( xf < xi )
					{
						xf = xi;
						xi = el->xf;
					}
					if( yf < yi )
					{
						yf = yi;
						yi = el->yf;
					}
					if( xi < m_max_x && xf > m_org_x && yi < m_max_y && yf > m_org_y )
					{
						int holew = el->holew;
						if( el->gtype == DL_HOLLOW_RRECT )
							pDC->SelectObject( GetStockObject( HOLLOW_BRUSH ) );
						pDC->RoundRect( xi, yi, xf, yf, 2*el->radius, 2*el->radius );
						if( el->gtype == DL_HOLLOW_RRECT )
							pDC->SelectObject( fill_brush );
						if( holew )
						{
							pDC->SelectObject( &black_brush );
							pDC->SelectObject( &black_pen );
							pDC->Ellipse( el->x_org - holew/2, el->y_org - holew/2, 
								el->x_org + holew/2, el->y_org + holew/2 );
							pDC->SelectObject( fill_brush );
							pDC->SelectObject( line_pen );
						}
					}
				}
				else if( el->gtype == DL_RECT_X )
				{
					if( xf < xi )
					{
						xf = xi;
						xi = el->xf;
					}
					if( yf < yi )
					{
						yf = yi;
						yi = el->yf;
					}
					if( xi < m_max_x && xf > m_org_x
						&& yi < m_max_y && yf > m_org_y )
					{
						pDC->MoveTo( el->x, el->y );
						pDC->LineTo( el->xf, el->y );
						pDC->LineTo( el->xf, el->yf );
						pDC->LineTo( el->x, el->yf );
						pDC->LineTo( el->x, el->y );
						pDC->MoveTo( el->x, el->y );
						pDC->LineTo( el->xf, el->yf );
						pDC->MoveTo( el->x, el->yf );
						pDC->LineTo( el->xf, el->y );
						nlines += 4;
					}
				}
				else if( el->gtype == DL_ARC_CW )
				{
					if( ( (el->xf >= el->x && el->x < m_max_x && el->xf > m_org_x)
						|| (el->xf < el->x && el->xf < m_max_x && el->x > m_org_x) )
						&& ( (el->yf >= el->y && el->y < m_max_y && el->yf > m_org_y)
						|| (el->yf < el->y && el->yf < m_max_y && el->y > m_org_y) ) )
					{
						CPen pen( PS_SOLID, el->w, RGB( m_rgb[layer][0], m_rgb[layer][1], m_rgb[layer][2] ) );
						pDC->SelectObject( &pen );
						DrawArc( pDC, DL_ARC_CW, el->x, el->y, el->xf, el->yf );
						pDC->SelectObject( line_pen );
					}
				}
				else if( el->gtype == DL_ARC_CCW )
				{
					if( ( (el->xf >= el->x && el->x < m_max_x && el->xf > m_org_x)
						|| (el->xf < el->x && el->xf < m_max_x && el->x > m_org_x) )
						&& ( (el->yf >= el->y && el->y < m_max_y && el->yf > m_org_y)
						|| (el->yf < el->y && el->yf < m_max_y && el->y > m_org_y) ) )
					{
						CPen pen( PS_SOLID, el->w, RGB( m_rgb[layer][0], m_rgb[layer][1], m_rgb[layer][2] ) );
						pDC->SelectObject( &pen );
						DrawArc( pDC, DL_ARC_CCW, el->x, el->y, el->xf, el->yf );
						pDC->SelectObject( line_pen );
					}
				}
				else if( el->gtype == DL_CURVE_CW )
				{
					if( ( (el->xf >= el->x && el->x < m_max_x && el->xf > m_org_x)
						|| (el->xf < el->x && el->xf < m_max_x && el->x > m_org_x) )
						&& ( (el->yf >= el->y && el->y < m_max_y && el->yf > m_org_y)
						|| (el->yf < el->y && el->yf < m_max_y && el->y > m_org_y) ) )
					{
						CPen pen( PS_SOLID, el->w, RGB( m_rgb[layer][0], m_rgb[layer][1], m_rgb[layer][2] ) );
						pDC->SelectObject( &pen );
						DrawCurve( pDC, DL_CURVE_CW, el->x, el->y, el->xf, el->yf );
						pDC->SelectObject( line_pen );
					}
				}
				else if( el->gtype == DL_CURVE_CCW )
				{
					if( ( (el->xf >= el->x && el->x < m_max_x && el->xf > m_org_x)
						|| (el->xf < el->x && el->xf < m_max_x && el->x > m_org_x) )
						&& ( (el->yf >= el->y && el->y < m_max_y && el->yf > m_org_y)
						|| (el->yf < el->y && el->yf < m_max_y && el->y > m_org_y) ) )
					{
						CPen pen( PS_SOLID, el->w, RGB( m_rgb[layer][0], m_rgb[layer][1], m_rgb[layer][2] ) );
						pDC->SelectObject( &pen );
						DrawCurve( pDC, DL_CURVE_CCW, el->x, el->y, el->xf, el->yf );
						pDC->SelectObject( line_pen );
					}
				}
				else if( el->gtype == DL_X )
				{
					xi = el->x - el->w/2;
					xf = el->x + el->w/2;
					yi = el->y - el->w/2;
					yf = el->y + el->w/2;
					if( xi < m_max_x && xf > m_org_x
						&& yi < m_max_y && yf > m_org_y )
					{
						pDC->MoveTo( xi, yi );
						pDC->LineTo( xf, yf );
						pDC->MoveTo( xf, yi );
						pDC->LineTo( xi, yf );
						nlines += 2;
					}
				}
				else if( el->gtype == DL_LINE )
				{
					// only draw line segments which are in viewport
					// viewport bounds, enlarged to account for line thickness
					int Yb = m_org_y - w/2;		// y-coord of viewport top	
					int Yt = m_max_y + w/2;		// y-coord of bottom
					int Xl = m_org_x - w/2;		// x-coord of left edge
					int Xr = m_max_x - w/2;		// x-coord of right edge
					// line segment from (xi,yi) to (xf,yf)
					int draw_flag = 0;
					// now test for all conditions where drawing is necessary
					if( xi==xf )
					{
						// vertical line
						if( yi<yf )
						{
							// upward
							if( yi<=Yt && yf>=Yb && xi<=Xr && xi>=Xl )
								draw_flag = 1;
						}
						else
						{
							// downward
							if( yf<=Yt && yi>=Yb && xi<=Xr && xi>=Xl )
								draw_flag = 1;
						}
					}
					else if( yi==yf )
					{
						// horizontal line
						if( xi<xf )
						{
							// rightward
							if( xi<=Xr && xf>=Xl && yi<=Yt && yi>=Yb )
								draw_flag = 1;
						}
						else
						{
							// leftward
							if( xf<=Xr && xi>=Xl && yi<=Yt && yi>=Yb )
								draw_flag = 1;
						}
					}
					else if( !((xi<Xl&&xf<Xl)||(xi>Xr&&xf>Xr)||(yi<Yb&&yf<Yb)||(yi>Yt&&yf>Yt)) )
					{
						// oblique line
						// not entirely above or below or right or left of viewport
						// get slope b and intercept a so that y=a+bx
						double b = (double)(yf-yi)/(xf-xi);
						int a = yi - int(b*xi);
						// now test for line in viewport
						if( abs((yf-yi)*(Xr-Xl)) > abs((xf-xi)*(Yt-Yb)) )
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
						CPen pen( PS_SOLID, w, RGB( m_rgb[layer][0], m_rgb[layer][1], m_rgb[layer][2] ) );
						pDC->SelectObject( &pen );
						pDC->MoveTo( xi, yi );
						pDC->LineTo( xf, yf );
						pDC->SelectObject( line_pen );
						nlines++;
					}
				}
			}
		}
		// restore original objects
		pDC->SelectObject( old_pen );
		pDC->SelectObject( old_brush );
	}

	// origin
	CRect r;
	r.top = 25*NM_PER_MIL/m_pcbu_per_wu;
	r.left = -25*NM_PER_MIL/m_pcbu_per_wu;
	r.right = 25*NM_PER_MIL/m_pcbu_per_wu;
	r.bottom = -25*NM_PER_MIL/m_pcbu_per_wu;
	pDC->SelectObject( &grid_pen );
	pDC->SelectObject( GetStockObject( HOLLOW_BRUSH ) );
	pDC->MoveTo( -100*NM_PER_MIL/m_pcbu_per_wu, 0 );
	pDC->LineTo( -25*NM_PER_MIL/m_pcbu_per_wu, 0 );
	pDC->MoveTo( 100*NM_PER_MIL/m_pcbu_per_wu, 0 );
	pDC->LineTo( 25*NM_PER_MIL/m_pcbu_per_wu, 0 );
	pDC->MoveTo( 0, -100*NM_PER_MIL/m_pcbu_per_wu );
	pDC->LineTo( 0, -25*NM_PER_MIL/m_pcbu_per_wu );
	pDC->MoveTo( 0, 100*NM_PER_MIL/m_pcbu_per_wu );
	pDC->LineTo( 0, 25*NM_PER_MIL/m_pcbu_per_wu );
	pDC->Ellipse( r );
	pDC->SelectObject( old_pen );
	pDC->SelectObject( old_brush );

	// if dragging, draw drag lines or shape 
	int old_ROP2 = pDC->GetROP2();
	pDC->SetROP2( R2_XORPEN );

	if( m_drag_num_lines )
	{
		// draw line array
		CPen drag_pen( PS_SOLID, 1, RGB( m_rgb[m_drag_layer][0], 
			m_rgb[m_drag_layer][1], m_rgb[m_drag_layer][2] ) );
		CPen * old_pen = pDC->SelectObject( &drag_pen );
		for( int il=0; il<m_drag_num_lines; il++ )
		{
			pDC->MoveTo( m_drag_x+m_drag_line_pt[2*il].x, m_drag_y+m_drag_line_pt[2*il].y );
			pDC->LineTo( m_drag_x+m_drag_line_pt[2*il+1].x, m_drag_y+m_drag_line_pt[2*il+1].y );
		}
		pDC->SelectObject( old_pen );
	}
	
	if( m_drag_num_ratlines )
	{
		// draw ratline array
		CPen drag_pen( PS_SOLID, m_drag_ratline_width, RGB( m_rgb[m_drag_layer][0], 
			m_rgb[m_drag_layer][1], m_rgb[m_drag_layer][2] ) );
		CPen * old_pen = pDC->SelectObject( &drag_pen );
		for( int il=0; il<m_drag_num_ratlines; il++ )
		{
			pDC->MoveTo( m_drag_ratline_start_pt[il].x, m_drag_ratline_start_pt[il].y );
			pDC->LineTo( m_drag_x+m_drag_ratline_end_pt[il].x, m_drag_y+m_drag_ratline_end_pt[il].y );
		}
		pDC->SelectObject( old_pen );
	}
	
	if( m_drag_flag )
	{
		// 4. Redraw the three segments:
		if(m_drag_shape == DS_SEGMENT)
		{
			pDC->MoveTo( m_drag_xb, m_drag_yb );

			// draw first segment
			CPen pen0( PS_SOLID, m_drag_w0, RGB( m_rgb[m_drag_layer_0][0], 
				m_rgb[m_drag_layer_0][1], m_rgb[m_drag_layer_0][2] ) );
			CPen * old_pen = pDC->SelectObject( &pen0 );
			pDC->LineTo( m_drag_xi, m_drag_yi );

			// draw second segment
			CPen pen1( PS_SOLID, m_drag_w1, RGB( m_rgb[m_drag_layer_1][0], 
				m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );
			pDC->SelectObject( &pen1 );
			pDC->LineTo( m_drag_xf, m_drag_yf );

			// draw third segment
			if(m_drag_style2 != DSS_NONE)
			{
				CPen pen2( PS_SOLID, m_drag_w2, RGB( m_rgb[m_drag_layer_2][0], 
					m_rgb[m_drag_layer_2][1], m_rgb[m_drag_layer_2][2] ) );
				pDC->SelectObject( &pen2 );
				pDC->LineTo( m_drag_xe, m_drag_ye );
			}
			pDC->SelectObject( old_pen );
		}
		// draw drag shape, if used
		if( m_drag_shape == DS_LINE_VERTEX || m_drag_shape == DS_LINE )
		{
			CPen pen_w( PS_SOLID, m_drag_w1, RGB( m_rgb[m_drag_layer_1][0], 
				m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );
			// draw dragged shape
			pDC->SelectObject( &pen_w );
			if( m_drag_style1 == DSS_STRAIGHT )
			{
				if( m_inflection_mode == IM_NONE )
				{
					pDC->MoveTo( m_drag_xi, m_drag_yi );
					pDC->LineTo( m_drag_x, m_drag_y );
				}
				else
				{
					CPoint pi( m_drag_xi, m_drag_yi );
					CPoint pf( m_drag_x, m_drag_y );
					CPoint p = GetInflectionPoint( pi, pf, m_inflection_mode );
					pDC->MoveTo( m_drag_xi, m_drag_yi );
					if( p != pi )
						pDC->LineTo( p.x, p.y );
					pDC->LineTo( m_drag_x, m_drag_y );
				}
				m_last_inflection_mode = m_inflection_mode;
			}
			else if( m_drag_style1 == DSS_ARC_CW )
				DrawArc( pDC, DL_ARC_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_drag_style1 == DSS_ARC_CCW )
				DrawArc( pDC, DL_ARC_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_drag_style1 == DSS_CURVE_CW )
				DrawCurve( pDC, DL_CURVE_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_drag_style1 == DSS_CURVE_CCW )
				DrawCurve( pDC, DL_CURVE_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else
				ASSERT(0);
			if( m_drag_shape == DS_LINE_VERTEX )
			{
				CPen pen( PS_SOLID, m_drag_w2, RGB( m_rgb[m_drag_layer_2][0], 
					m_rgb[m_drag_layer_2][1], m_rgb[m_drag_layer_2][2] ) );
				pDC->SelectObject( &pen );
				if( m_drag_style2 == DSS_STRAIGHT )
					pDC->LineTo( m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_ARC_CW )
					DrawArc( pDC, DL_ARC_CW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_ARC_CCW )
					DrawArc( pDC, DL_ARC_CCW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_CURVE_CW )
					DrawCurve( pDC, DL_CURVE_CW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_CURVE_CCW )
					DrawCurve( pDC, DL_CURVE_CCW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
				else
					ASSERT(0);
				pDC->SelectObject( old_pen );
			}
			pDC->SelectObject( old_pen );

			// see if leading via needs to be drawn
			if( m_drag_via_drawn )
			{
				// draw or undraw via
				int thick = (m_drag_via_w - m_drag_via_holew)/2;
				int w = m_drag_via_w - thick;
				int holew = m_drag_via_holew;
//				CPen pen( PS_SOLID, thick, RGB( m_rgb[LAY_PAD_THRU][0], m_rgb[LAY_PAD_THRU][1], m_rgb[LAY_PAD_THRU][2] ) );
				CPen pen( PS_SOLID, thick, RGB( m_rgb[m_drag_layer_1][0], m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );
				CPen * old_pen = pDC->SelectObject( &pen );
				CBrush black_brush( RGB( 0, 0, 0 ) );
				CBrush * old_brush = pDC->SelectObject( &black_brush );
				pDC->Ellipse( m_drag_xi - w/2, m_drag_yi - w/2, m_drag_xi + w/2, m_drag_yi + w/2 );
				pDC->SelectObject( old_brush );
				pDC->SelectObject( old_pen );
			}

		}
		else if( m_drag_shape == DS_ARC_STRAIGHT 
			|| m_drag_shape == DS_ARC_CW || m_drag_shape == DS_ARC_CCW 
			|| m_drag_shape == DS_CURVE_CW || m_drag_shape == DS_CURVE_CCW 
			)
		{
			CPen pen_w( PS_SOLID, m_drag_w1, RGB( m_rgb[m_drag_layer_1][0], 
				m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );
			// redraw dragged shape
			pDC->SelectObject( &pen_w );
			if( m_drag_shape == DS_ARC_STRAIGHT )
				DrawArc( pDC, DL_LINE, m_drag_x, m_drag_y, m_drag_xi, m_drag_yi );
			else if( m_drag_shape == DS_ARC_CW )
				DrawArc( pDC, DL_ARC_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_drag_shape == DS_ARC_CCW )
				DrawArc( pDC, DL_ARC_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_drag_shape == DS_CURVE_CW )
				DrawCurve( pDC, DL_CURVE_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_drag_shape == DS_CURVE_CCW )
				DrawCurve( pDC, DL_CURVE_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			pDC->SelectObject( old_pen );	
			m_last_drag_shape = m_drag_shape;
		}
	}

	// if cross hairs, draw them
	if( m_cross_hairs )
	{
		// draw cross-hairs
		COLORREF bk = pDC->SetBkColor( RGB(0,0,0) );
		CPen pen( PS_DOT, 0, RGB( 128, 128, 128 ) );
		pDC->SelectObject( &pen );
		int x = m_cross_bottom.x;
		int y = m_cross_left.y;
		m_cross_left.x = m_org_x;
		m_cross_right.x = m_max_x;
		m_cross_bottom.y = m_org_y;
		m_cross_top.y = m_max_y;
		if( x-m_org_x > y-m_org_y )
		{
			// bottom-left cursor line intersects m_org_y
			m_cross_botleft.x = x - (y - m_org_y);
			m_cross_botleft.y = m_org_y;
		}
		else
		{
			// bottom-left cursor line intersects m_org_x
			m_cross_botleft.x = m_org_x;
			m_cross_botleft.y = y - (x - m_org_x);
		}
		if( m_max_x-x > y-m_org_y )
		{
			// bottom-right cursor line intersects m_org_y
			m_cross_botright.x = x + (y - m_org_y);
			m_cross_botright.y = m_org_y;
		}
		else
		{
			// bottom-right cursor line intersects m_max_x
			m_cross_botright.x = m_max_x;
			m_cross_botright.y = y - (m_max_x - x);
		}

		if( x-m_org_x > m_max_y-y )
		{
			// top-left cursor line intersects m_max_y
			m_cross_topleft.x = x - (m_max_y - y);
			m_cross_topleft.y = m_max_y;
		}
		else
		{
			// top-left cursor line intersects m_org_x
			m_cross_topleft.x = m_org_x;
			m_cross_topleft.y = y + (x - m_org_x);
		}
		if( m_max_x-x > m_max_y-y )
		{
			// top-right cursor line intersects m_max_y
			m_cross_topright.x = x + (m_max_y - y);
			m_cross_topright.y = m_max_y;
		}
		else
		{
			// top-right cursor line intersects m_max_x
			m_cross_topright.x = m_max_x;
			m_cross_topright.y = y + (m_max_x - x);
		}
		pDC->MoveTo( m_cross_left );
		pDC->LineTo( m_cross_right );
		pDC->MoveTo( m_cross_bottom );
		pDC->LineTo( m_cross_top );
		if( m_cross_hairs == 2 )
		{
			pDC->MoveTo( m_cross_topleft );
			pDC->LineTo( m_cross_botright );
			pDC->MoveTo( m_cross_botleft );
			pDC->LineTo( m_cross_topright );
		}
		pDC->SelectObject( old_pen );
		pDC->SetBkColor( bk );
	}
	pDC->SetROP2( old_ROP2 );

	// restore original objects
	pDC->SelectObject( old_pen );
	pDC->SelectObject( old_brush );

	// double-buffer to screen
	if( memDC )
	{
		dDC->BitBlt( m_org_x, m_org_y, m_max_x-m_org_x, m_max_y-m_org_y,
			pDC, m_org_x, m_org_y, SRCCOPY );
	}
}

// set the display color for a layer
//
void CDisplayList::SetLayerRGB( int layer, int r, int g, int b )
{
	m_rgb[layer][0] = r;
	m_rgb[layer][1] = g;
	m_rgb[layer][2] = b;
}

void CDisplayList::SetLayerVisible( int layer, BOOL vis )
{
	m_vis[layer] = vis;
}

// test x,y for a hit on an item in the selection layer
// creates arrays with layer and id of each hit item
// assigns priority based on layer and id
// then returns pointer to item with highest priority
// If exclude_id != NULL, excludes item with 
// id == exclude_id and ptr == exclude_ptr 
// If include_id != NULL, only include items that match include_id[]
// where n_include_ids is size of array, and
// where 0's in include_id[] fields are treated as wildcards
//
void * CDisplayList::TestSelect( int x, int y, id * sel_id, int * sel_layer, 
								id * exclude_id, void * exclude_ptr, 
								id * include_id, int n_include_ids )
{
	enum {
		MAX_HITS = 10000
	};
	int  nhits = 0;
	int hit_layer[MAX_HITS];
	id hit_id[MAX_HITS];
	void * hit_ptr[MAX_HITS];
	int hit_priority[MAX_HITS];

	int xx = x/m_pcbu_per_wu;
	int yy = y/m_pcbu_per_wu;

	// traverse the list, looking for selection shapes
	dl_element * el = &m_start[LAY_SELECTION];
	while( el->next != &m_end[LAY_SELECTION] )
	{
		el = el->next;

		// don't select anything on an invisible layer
		if( !m_vis[el->layer] )
			continue;
		// don't select anything invisible
		if( el->visible == 0 )
			continue;

		if( el->gtype == DL_HOLLOW_RECT || el->gtype == DL_RECT_X )
		{
			// found selection rectangle, test for hit
			if( ( (xx>el->x && xx<el->xf) || (xx<el->x && xx>el->xf) )
				&& ( (yy>el->y && yy<el->yf) || (yy<el->y && yy>el->yf) ) )
			{
				// OK, hit
				hit_layer[nhits] = el->layer;
				hit_id[nhits] = el->id;
				hit_ptr[nhits] = el->ptr;
				nhits++;
				if( nhits >= MAX_HITS )
					ASSERT(0);
			}
		}
		else if( el->gtype == DL_LINE )
		{
			// found selection line, test for hit (within 4 pixels or line width+3 mils )
//**			int w = max( el->w/2+3*DU_PER_MIL, int(4.0*m_scale) );
			int w = max( el->w/2, int(4.0*m_scale) );
			int test = TestLineHit( el->x, el->y, el->xf, el->yf, xx, yy, w );
			if( test )
			{
				// OK, hit
				hit_layer[nhits] = el->layer;
				hit_id[nhits] = el->id;
				hit_ptr[nhits] = el->ptr;
				nhits++;
				if( nhits >= MAX_HITS )
					ASSERT(0);
			}
		}
		else if( el->gtype == DL_HOLLOW_CIRC )
		{
			// found hollow circle, test for hit (within 3 mils or 4 pixels)
//**			int dr = max( 3*DU_PER_MIL, int(4.0*m_scale) );
			int dr = 4.0*m_scale;
			int w = el->w/2;
			int x = el->x;
			int y = el->y;
			int d = Distance( x, y, xx, yy );
			if( abs(w-d) < dr )
			{
				// OK, hit
				hit_layer[nhits] = el->layer;
				hit_id[nhits] = el->id;
				hit_ptr[nhits] = el->ptr;
				nhits++;
				if( nhits >= MAX_HITS )
					ASSERT(0);
			}
		}
		else if( el->gtype == DL_ARC_CW || el->gtype == DL_ARC_CCW )
		{
			// found selection rectangle, test for hit
			if( ( (xx>el->x && xx<el->xf) || (xx<el->x && xx>el->xf) )
				&& ( (yy>el->y && yy<el->yf) || (yy<el->y && yy>el->yf) ) )
			{
				// OK, hit
				hit_layer[nhits] = el->layer;
				hit_id[nhits] = el->id;
				hit_ptr[nhits] = el->ptr;
				nhits++;
				if( nhits >= MAX_HITS )
					ASSERT(0);
			}
		}
		else if( el->gtype == DL_CURVE_CW || el->gtype == DL_CURVE_CCW )
		{
			// found selection rectangle, test for hit
			if( ( (xx>el->x && xx<el->xf) || (xx<el->x && xx>el->xf) )
				&& ( (yy>el->y && yy<el->yf) || (yy<el->y && yy>el->yf) ) )
			{
				// OK, hit
				hit_layer[nhits] = el->layer;
				hit_id[nhits] = el->id;
				hit_ptr[nhits] = el->ptr;
				nhits++;
				if( nhits >= MAX_HITS )
					ASSERT(0);
			}
		}
		else
			ASSERT(0);		// bad element
	}
	// now return highest priority hit
	if( nhits == 0 )
	{
		// no hit
		id no_id;
		*sel_id = no_id;
		*sel_layer = 0;
		return NULL;
	}
	else
	{
		// assign priority to each hit, track maximum, exclude exclude_id item
		int best_hit = -1;	
		int best_hit_priority = 0;
		for( int i=0; i<nhits; i++ )
		{
			BOOL excluded_hit = FALSE;
			BOOL included_hit = TRUE;
			if( exclude_id )
			{
				if( hit_id[i] == *exclude_id && hit_ptr[i] == exclude_ptr )
					excluded_hit = TRUE;
			}
			if( include_id )
			{
				included_hit = FALSE;
				for( int inc=0; inc<n_include_ids; inc++ )
				{
					id * inc_id = &include_id[inc];
					if( inc_id->type == hit_id[i].type
						&& ( inc_id->st == 0 || inc_id->st == hit_id[i].st )
						&& ( inc_id->i == 0 || inc_id->i == hit_id[i].i )
						&& ( inc_id->sst == 0 || inc_id->sst == hit_id[i].sst )
						&& ( inc_id->ii == 0 || inc_id->ii == hit_id[i].ii ) )
					{
						included_hit = TRUE;
						break;
					}
				}
			}
			if( !excluded_hit && included_hit )
			{
				// OK, valid hit, now assign priority
				// start with reversed layer drawing order * 10
				// i.e. last drawn = highest priority
				int priority = (MAX_LAYERS - m_order_for_layer[hit_layer[i]])*10;
				// bump priority for small items which may be overlapped by larger items on same layer
				if( hit_id[i].type == ID_PART && hit_id[i].st == ID_SEL_REF_TXT )
					priority++;
				else if( hit_id[i].type == ID_PART && hit_id[i].st == ID_SEL_VALUE_TXT )
					priority++;
				else if( hit_id[i].type == ID_BOARD && hit_id[i].st == ID_BOARD_OUTLINE && hit_id[i].sst == ID_SEL_CORNER )
					priority++;
				else if( hit_id[i].type == ID_NET && hit_id[i].st == ID_AREA && hit_id[i].sst == ID_SEL_CORNER )
					priority++;
				hit_priority[i] = priority;
				if( priority >= best_hit_priority )
				{
					best_hit_priority = priority;
					best_hit = i;
				}
			}
		}
		if( best_hit == -1 )
		{
			id no_id;
			*sel_id = no_id;
			*sel_layer = 0;
			return NULL;
		}

		*sel_id = hit_id[best_hit];
		*sel_layer = hit_layer[best_hit];
		return hit_ptr[best_hit];
	}
}

// Start dragging arrays of drag lines and ratlines
// Assumes that arrays have already been set up using MakeLineArray, etc.
// If no arrays, just drags point
//
int CDisplayList::StartDraggingArray( CDC * pDC, int xx, int yy, int vert, int layer, int crosshair )
{
	// convert to display units
	int x = xx/m_pcbu_per_wu;
	int y = yy/m_pcbu_per_wu;

	// cancel dragging non-array shape
	m_drag_flag = 0;
	m_drag_shape = 0;

	// set up for dragging array
	m_drag_x = x;			// "grab point"
	m_drag_y = y;
	m_drag_vert = vert;		// set axis for flipping item to opposite side of PCB
	m_drag_layer = layer;
	m_drag_angle = 0;
	m_drag_side = 0;
	SetUpCrosshairs( crosshair, x, y );
	
	// done
	return 0;
}

// Start dragging rectangle 
//
int CDisplayList::StartDraggingRectangle( CDC * pDC, int x, int y, int xi, int yi,
										 int xf, int yf, int vert, int layer )
{
	// create drag lines
	CPoint p1(xi,yi);
	CPoint p2(xf,yi);
	CPoint p3(xf,yf);
	CPoint p4(xi,yf);
	MakeDragLineArray( 4 );
	AddDragLine( p1, p2 ); 
	AddDragLine( p2, p3 ); 
	AddDragLine( p3, p4 ); 
	AddDragLine( p4, p1 ); 

	StartDraggingArray( pDC, x, y, vert, layer );
	
	// done
	return 0;
}


// Start dragging line 
//
int CDisplayList::StartDraggingRatLine( CDC * pDC, int x, int y, int xi, int yi, 
									   int layer, int w, int crosshair )
{
	// create drag line
	CPoint p1(xi,yi);
	CPoint p2(x,y);
	MakeDragRatlineArray( 1, w );
	AddDragRatline( p1, p2 ); 

	StartDraggingArray( pDC, xi, yi, 0, layer, crosshair );
	
	// done
	return 0;
}

// set style of arc being dragged, using CPolyLine styles
//
void CDisplayList::SetDragArcStyle( int style )
{
	if( style == CPolyLine::STRAIGHT )
		m_drag_shape = DS_ARC_STRAIGHT;
	else if( style == CPolyLine::ARC_CW )
		m_drag_shape = DS_ARC_CW;
	else if( style == CPolyLine::ARC_CCW )
		m_drag_shape = DS_ARC_CCW;
}

// Start dragging arc endpoint, using style from CPolyLine
// Use the layer color and width w
//
int CDisplayList::StartDraggingArc( CDC * pDC, int style, int xx, int yy, int xi, int yi, 
								   int layer, int w, int crosshair )
{
	int x = xx/m_pcbu_per_wu;
	int y = yy/m_pcbu_per_wu;

	// set up for dragging
	m_drag_flag = 1;
	if( style == CPolyLine::STRAIGHT )
		m_drag_shape = DS_ARC_STRAIGHT;
	else if( style == CPolyLine::ARC_CW )
		m_drag_shape = DS_ARC_CW;
	else if( style == CPolyLine::ARC_CCW )
		m_drag_shape = DS_ARC_CCW;
	m_drag_x = x;	// position of endpoint (at cursor)
	m_drag_y = y;
	m_drag_xi = xi/m_pcbu_per_wu;	// start point
	m_drag_yi = yi/m_pcbu_per_wu;
	m_drag_side = 0;
	m_drag_layer_1 = layer;
	m_drag_w1 = w/m_pcbu_per_wu;

	// set up cross hairs
	SetUpCrosshairs( crosshair, x, y );

	//Redraw
//	Draw( pDC );

	// done
	return 0;
}

// Start dragging the selected line endpoint
// Use the layer1 color and width w
//
int CDisplayList::StartDraggingLine( CDC * pDC, int x, int y, int xi, int yi, int layer1, int w,
									int layer_no_via, int via_w, int via_holew,
									int crosshair, int style, int inflection_mode )
{
	// set up for dragging
	m_drag_flag = 1;
	m_drag_shape = DS_LINE;
	m_drag_x = x/m_pcbu_per_wu;	// position of endpoint (at cursor)
	m_drag_y = y/m_pcbu_per_wu;
	m_drag_xi = xi/m_pcbu_per_wu;	// initial vertex
	m_drag_yi = yi/m_pcbu_per_wu;
	m_drag_side = 0;
	m_drag_layer_1 = layer1;
	m_drag_w1 = w/m_pcbu_per_wu;
	m_drag_style1 = style;
	m_drag_layer_no_via = layer_no_via;
	m_drag_via_w = via_w/m_pcbu_per_wu;
	m_drag_via_holew = via_holew/m_pcbu_per_wu;
	m_drag_via_drawn = 0;
	m_inflection_mode = inflection_mode;
	m_last_inflection_mode = IM_NONE;

	// set up cross hairs
	SetUpCrosshairs( crosshair, x, y );

	//Redraw
//	Draw( pDC );

	// done
	return 0;
}

// Start dragging line vertex (i.e. vertex between 2 line segments)
// Use the layer1 color and width w1 for the first segment from (xi,yi) to the vertex,
// Use the layer2 color and width w2 for the second segment from the vertex to (xf,yf)
// While dragging, insert via at start point of first segment if layer1 != layer_no_via
// using via parameters via_w and via_holew
// Note that layer1 may be changed while dragging by ChangeRouting Layer()
// If dir = 1, swap start and end points
//
int CDisplayList::StartDraggingLineVertex( CDC * pDC, int x, int y, 
									int xi, int yi, int xf, int yf,
									int layer1, int layer2, int w1, int w2, int style1, int style2,
									int layer_no_via, int via_w, int via_holew, int dir,
									int crosshair )
{
	// set up for dragging
	m_drag_flag = 1;
	m_drag_shape = DS_LINE_VERTEX;
	m_drag_x = x/m_pcbu_per_wu;	// position of central vertex (at cursor)
	m_drag_y = y/m_pcbu_per_wu;
	if( dir == 0 )
	{
		// routing forward
		m_drag_xi = xi/m_pcbu_per_wu;	// initial vertex
		m_drag_yi = yi/m_pcbu_per_wu;
		m_drag_xf = xf/m_pcbu_per_wu;	// final vertex
		m_drag_yf = yf/m_pcbu_per_wu;
	}
	else
	{
		// routing backward
		m_drag_xi = xf/m_pcbu_per_wu;	// initial vertex
		m_drag_yi = yf/m_pcbu_per_wu;
		m_drag_xf = xi/m_pcbu_per_wu;	// final vertex
		m_drag_yf = yi/m_pcbu_per_wu;
	}
	m_drag_side = 0;
	m_drag_layer_1 = layer1;
	m_drag_layer_2 = layer2;
	m_drag_w1 = w1/m_pcbu_per_wu;
	m_drag_w2 = w2/m_pcbu_per_wu;
	m_drag_style1 = style1;
	m_drag_style2 = style2;
	m_drag_layer_no_via = layer_no_via;
	m_drag_via_w = via_w/m_pcbu_per_wu;
	m_drag_via_holew = via_holew/m_pcbu_per_wu;
	m_drag_via_drawn = 0;

	// set up cross hairs
	SetUpCrosshairs( crosshair, x, y );

	//Redraw
	Draw( pDC );

	// done
	return 0;
}


// Start dragging line segment (i.e. line segment between 2 vertices)
// Use the layer0 color and width w0 for the "before" segment from (xb,yb) to (xi, yi),
// Use the layer1 color and width w1 for the moving segment from (xi,yi) to (xf, yf),
// Use the layer2 color and width w2 for the ending segment from (xf, yf) to (xe,ye)
// While dragging, insert via at (xi, yi) if layer1 != layer_no_via1, insert via
// at (xf, yf) if layer2 != layer_no_via.
// using via parameters via_w and via_holew
// Note that layer1 may be changed while dragging by ChangeRouting Layer()
// If dir = 1, swap start and end points
//
int CDisplayList::StartDraggingLineSegment( CDC * pDC, int x, int y,
									int xb, int yb,
									int xi, int yi, 
									int xf, int yf,
									int xe, int ye,
									int layer0, int layer1, int layer2,
									int w0,		int w1,		int w2,
									int style0, int style1, int style2,
									
									int layer_no_via, int via_w, int via_holew, 
									int crosshair )
{
	// set up for dragging
	m_drag_flag = 1;
	m_drag_shape = DS_SEGMENT;

	m_drag_x = x/m_pcbu_per_wu;	// position of reference point (at cursor)
	m_drag_y = y/m_pcbu_per_wu;

	m_drag_xb = xb/m_pcbu_per_wu;	// vertex before
	m_drag_yb = yb/m_pcbu_per_wu;
	m_drag_xi = xi/m_pcbu_per_wu;	// initial vertex
	m_drag_yi = yi/m_pcbu_per_wu;
	m_drag_xf = xf/m_pcbu_per_wu;	// final vertex
	m_drag_yf = yf/m_pcbu_per_wu;
	m_drag_xe = xe/m_pcbu_per_wu;	// End vertex
	m_drag_ye = ye/m_pcbu_per_wu;

	m_drag_side = 0;
	m_drag_layer_0 = layer0;
	m_drag_layer_1 = layer1;
	m_drag_layer_2 = layer2;

	m_drag_w0 = w0/m_pcbu_per_wu;
	m_drag_w1 = w1/m_pcbu_per_wu;
	m_drag_w2 = w2/m_pcbu_per_wu;

	m_drag_style0 = style0;
	m_drag_style1 = style1;
	m_drag_style2 = style2;

	m_drag_layer_no_via = layer_no_via;
	m_drag_via_w = via_w/m_pcbu_per_wu;
	m_drag_via_holew = via_holew/m_pcbu_per_wu;
	m_drag_via_drawn = 0;

	// set up cross hairs
	SetUpCrosshairs( crosshair, x, y );

	//Redraw
	Draw( pDC );

	// done
	return 0;
}

// Drag graphics with cursor 
//
void CDisplayList::Drag( CDC * pDC, int x, int y )
{	
	// convert from PCB to display coords
	int xx = x/m_pcbu_per_wu;
	int yy = y/m_pcbu_per_wu;
	
	// set XOR pen mode for dragging
	int old_ROP2 = pDC->SetROP2( R2_XORPEN );

	//**** there are three dragging modes, which may be used simultaneously ****//

	// drag array of lines, used to make complex graphics like a part
	if( m_drag_num_lines )
	{
		CPen drag_pen( PS_SOLID, 1, RGB( m_rgb[m_drag_layer][0], 
			m_rgb[m_drag_layer][1], m_rgb[m_drag_layer][2] ) );
		CPen * old_pen = pDC->SelectObject( &drag_pen );
		for( int il=0; il<m_drag_num_lines; il++ )
		{
			// undraw
			pDC->MoveTo( m_drag_x+m_drag_line_pt[2*il].x, m_drag_y+m_drag_line_pt[2*il].y );
			pDC->LineTo( m_drag_x+m_drag_line_pt[2*il+1].x, m_drag_y+m_drag_line_pt[2*il+1].y );
			// redraw
			pDC->MoveTo( xx+m_drag_line_pt[2*il].x, yy+m_drag_line_pt[2*il].y );
			pDC->LineTo( xx+m_drag_line_pt[2*il+1].x, yy+m_drag_line_pt[2*il+1].y );
		}
		pDC->SelectObject( old_pen );
	}

	// drag array of rubberband lines, used for ratlines to dragged part
	if( m_drag_num_ratlines )
	{
		CPen drag_pen( PS_SOLID, 1, RGB( m_rgb[m_drag_layer][0], 
			m_rgb[m_drag_layer][1], m_rgb[m_drag_layer][2] ) );
		CPen * old_pen = pDC->SelectObject( &drag_pen );
		for( int il=0; il<m_drag_num_ratlines; il++ )
		{
			// undraw
			pDC->MoveTo( m_drag_ratline_start_pt[il].x, m_drag_ratline_start_pt[il].y );
			pDC->LineTo( m_drag_x+m_drag_ratline_end_pt[il].x, m_drag_y+m_drag_ratline_end_pt[il].y );
			// draw
			pDC->MoveTo( m_drag_ratline_start_pt[il].x, m_drag_ratline_start_pt[il].y );
			pDC->LineTo( xx+m_drag_ratline_end_pt[il].x, yy+m_drag_ratline_end_pt[il].y );
		}
		pDC->SelectObject( old_pen );
	}

	// draw special shapes, used for polyline sides and trace segments
	if( m_drag_flag && ( m_drag_shape == DS_LINE_VERTEX || m_drag_shape == DS_LINE ) )
	{
		// drag rubberband trace segment, or vertex between two rubberband segments
		// used for routing traces
		CPen pen_w( PS_SOLID, m_drag_w1, RGB( m_rgb[m_drag_layer_1][0], 
			m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );

		// undraw first segment
		CPen * old_pen = pDC->SelectObject( &pen_w );
		{
			if( m_drag_style1 == DSS_STRAIGHT )
			{
				if( m_last_inflection_mode == IM_NONE )
				{
					pDC->MoveTo( m_drag_xi, m_drag_yi );
					pDC->LineTo( m_drag_x, m_drag_y );
				}
				else
				{
					CPoint pi( m_drag_xi, m_drag_yi );
					CPoint pf( m_drag_x, m_drag_y );
					CPoint p = GetInflectionPoint( pi, pf, m_last_inflection_mode );
					pDC->MoveTo( m_drag_xi, m_drag_yi );
					if( p != pi )
						pDC->LineTo( p.x, p.y );
					pDC->LineTo( m_drag_x, m_drag_y );
				}
			}
			else if( m_drag_style1 == DSS_ARC_CW )
				DrawArc( pDC, DL_ARC_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_drag_style1 == DSS_ARC_CCW )
				DrawArc( pDC, DL_ARC_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_drag_style1 == DSS_CURVE_CW )
				DrawCurve( pDC, DL_CURVE_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_drag_style1 == DSS_CURVE_CCW )
				DrawCurve( pDC, DL_CURVE_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else 
				ASSERT(0);
			if( m_drag_shape == DS_LINE_VERTEX )
			{
				// undraw second segment
				CPen pen( PS_SOLID, m_drag_w2, RGB( m_rgb[m_drag_layer_2][0], 
					m_rgb[m_drag_layer_2][1], m_rgb[m_drag_layer_2][2] ) );
				CPen * old_pen = pDC->SelectObject( &pen );
				if( m_drag_style2 == DSS_STRAIGHT )
					pDC->LineTo( m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_ARC_CW )
					DrawArc( pDC, DL_ARC_CW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_ARC_CCW )
					DrawArc( pDC, DL_ARC_CCW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_CURVE_CW )
					DrawCurve( pDC, DL_CURVE_CW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_CURVE_CCW )
					DrawCurve( pDC, DL_CURVE_CCW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
				else
					ASSERT(0);
				pDC->SelectObject( old_pen );
			}

			// draw first segment
			if( m_drag_style1 == DSS_STRAIGHT ) 
			{
				if( m_inflection_mode == IM_NONE )
				{
					pDC->MoveTo( m_drag_xi, m_drag_yi );
					pDC->LineTo( xx, yy );
				}
				else
				{
					CPoint pi( m_drag_xi, m_drag_yi );
					CPoint pf( xx, yy );
					CPoint p = GetInflectionPoint( pi, pf, m_inflection_mode );
					pDC->MoveTo( m_drag_xi, m_drag_yi );
					if( p != pi )
						pDC->LineTo( p.x, p.y );
					pDC->LineTo( xx, yy );
				}
				m_last_inflection_mode = m_inflection_mode;
			}
			else if( m_drag_style1 == DSS_ARC_CW )
				DrawArc( pDC, DL_ARC_CW, m_drag_xi, m_drag_yi, xx, yy );
			else if( m_drag_style1 == DSS_ARC_CCW )
				DrawArc( pDC, DL_ARC_CCW, m_drag_xi, m_drag_yi, xx, yy );
			else if( m_drag_style1 == DSS_CURVE_CW )
				DrawCurve( pDC, DL_CURVE_CW, m_drag_xi, m_drag_yi, xx, yy );
			else if( m_drag_style1 == DSS_CURVE_CCW )
				DrawCurve( pDC, DL_CURVE_CCW, m_drag_xi, m_drag_yi, xx, yy );
			else
				ASSERT(0);
			if( m_drag_shape == DS_LINE_VERTEX )
			{
				// draw second segment
				CPen pen( PS_SOLID, m_drag_w2, RGB( m_rgb[m_drag_layer_2][0], 
					m_rgb[m_drag_layer_2][1], m_rgb[m_drag_layer_2][2] ) );
				CPen * old_pen = pDC->SelectObject( &pen );
				if( m_drag_style2 == DSS_STRAIGHT )
					pDC->LineTo( m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_ARC_CW )
					DrawArc( pDC, DL_ARC_CW, xx, yy, m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_ARC_CCW )
					DrawArc( pDC, DL_ARC_CCW, xx, yy, m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_CURVE_CW )
					DrawArc( pDC, DL_ARC_CW, xx, yy, m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_CURVE_CCW )
					DrawArc( pDC, DL_ARC_CCW, xx, yy, m_drag_xf, m_drag_yf );
				else
					ASSERT(0);
				pDC->SelectObject( old_pen );
			}

			// see if leading via needs to be changed
			if( ( (m_drag_layer_no_via != 0 && m_drag_layer_1 != m_drag_layer_no_via) && !m_drag_via_drawn )
				|| (!(m_drag_layer_no_via != 0 && m_drag_layer_1 != m_drag_layer_no_via) && m_drag_via_drawn ) )
			{
				// draw or undraw via
				int thick = (m_drag_via_w - m_drag_via_holew)/2;
				int w = m_drag_via_w - thick;
				int holew = m_drag_via_holew;
//				CPen pen( PS_SOLID, thick, RGB( m_rgb[LAY_PAD_THRU][0], m_rgb[LAY_PAD_THRU][1], m_rgb[LAY_PAD_THRU][2] ) );
				CPen pen( PS_SOLID, thick, RGB( m_rgb[m_drag_layer_1][0], m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );
				CPen * old_pen = pDC->SelectObject( &pen );
				{
					CBrush black_brush( RGB( 0, 0, 0 ) );
					CBrush * old_brush = pDC->SelectObject( &black_brush );
					pDC->Ellipse( m_drag_xi - w/2, m_drag_yi - w/2, m_drag_xi + w/2, m_drag_yi + w/2 );
					pDC->SelectObject( old_brush );
				}
				pDC->SelectObject( old_pen );
				m_drag_via_drawn = 1 - m_drag_via_drawn;
			}	
		}
		pDC->SelectObject( old_pen );
	}
	// move a trace segment
	else if( m_drag_flag && m_drag_shape == DS_SEGMENT )
	{

		{
			ASSERT(m_drag_style0 == DSS_STRAIGHT);
			pDC->MoveTo( m_drag_xb, m_drag_yb );

			// undraw first segment
			CPen pen0( PS_SOLID, m_drag_w0, RGB( m_rgb[m_drag_layer_0][0], 
				m_rgb[m_drag_layer_0][1], m_rgb[m_drag_layer_0][2] ) );
			CPen * old_pen = pDC->SelectObject( &pen0 );
			pDC->LineTo( m_drag_xi, m_drag_yi );

			// undraw second segment
			ASSERT(m_drag_style1 == DSS_STRAIGHT);
			CPen pen1( PS_SOLID, m_drag_w1, RGB( m_rgb[m_drag_layer_1][0], 
				m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );
			pDC->SelectObject( &pen1 );
			pDC->LineTo( m_drag_xf, m_drag_yf );

			// undraw third segment
			if(m_drag_style2 == DSS_STRAIGHT)		// Could also be DSS_NONE (this segment only)
			{
				CPen pen2( PS_SOLID, m_drag_w2, RGB( m_rgb[m_drag_layer_2][0], 
					m_rgb[m_drag_layer_2][1], m_rgb[m_drag_layer_2][2] ) );
				pDC->SelectObject( &pen2 );
				pDC->LineTo( m_drag_xe, m_drag_ye );
			}
			pDC->SelectObject( old_pen );
		}

		// Adjust the two vertices, (xi, yi) and (xf, yf) based on the movement of xx and yy
		//  relative to m_drag_x and m_drag_y:

		// 1. Move the endpoints of (xi, yi), (xf, yf) of the line by the mouse movement. This
		//		is just temporary, since the final ending position is determined by the intercept
		//		points with the leading and trailing segments:
		int new_xi = m_drag_xi + xx - m_drag_x;			// Check sign here.
		int new_yi = m_drag_yi + yy - m_drag_y;

		int new_xf = m_drag_xf + xx - m_drag_x;
		int new_yf = m_drag_yf + yy - m_drag_y;

		int old_xb_dir = sign(m_drag_xi - m_drag_xb);
		int old_yb_dir = sign(m_drag_yi - m_drag_yb);

		int old_xi_dir = sign(m_drag_xf - m_drag_xi);
		int old_yi_dir = sign(m_drag_yf - m_drag_yi);

		int old_xe_dir = sign(m_drag_xe - m_drag_xf);
		int old_ye_dir = sign(m_drag_ye - m_drag_yf);

		// 2. Find the intercept between the extended segment in motion and the leading segment.
		double d_new_xi;
		double d_new_yi;
		FindLineIntersection(m_drag_xb, m_drag_yb, m_drag_xi, m_drag_yi,
								new_xi,    new_yi,	   new_xf,    new_yf,
								&d_new_xi, &d_new_yi);
		int i_drag_xi = floor(d_new_xi + .5);
		int i_drag_yi = floor(d_new_yi + .5);

		// 3. Find the intercept between the extended segment in motion and the trailing segment:
		int i_drag_xf, i_drag_yf;
		if(m_drag_style2 == DSS_STRAIGHT)
		{
			double d_new_xf;
			double d_new_yf;
			FindLineIntersection(new_xi,    new_yi,	   new_xf,    new_yf,
									m_drag_xf, m_drag_yf, m_drag_xe, m_drag_ye,
									&d_new_xf, &d_new_yf);

			i_drag_xf = floor(d_new_xf + .5);
			i_drag_yf = floor(d_new_yf + .5);
		} else {
			i_drag_xf = new_xf;
			i_drag_yf = new_yf;
		}
		
		// If we drag too far, the line segment can reverse itself causing a little triangle to form.
		//   That's a bad thing.
		if(sign(i_drag_xf - i_drag_xi) == old_xi_dir && sign(i_drag_yf - i_drag_yi) == old_yi_dir &&
		   sign(i_drag_xi - m_drag_xb) == old_xb_dir && sign(i_drag_yi - m_drag_yb) == old_yb_dir &&
		   sign(m_drag_xe - i_drag_xf) == old_xe_dir && sign(m_drag_ye - i_drag_yf) == old_ye_dir   )
		{
			m_drag_xi = i_drag_xi;
			m_drag_yi = i_drag_yi;
			m_drag_xf = i_drag_xf;
			m_drag_yf = i_drag_yf;
		}
		else
		{
			xx = m_drag_x;
			yy = m_drag_y;
		}

		// 4. Redraw the three segments:
		{
			pDC->MoveTo( m_drag_xb, m_drag_yb );

			// draw first segment
			CPen pen0( PS_SOLID, m_drag_w0, RGB( m_rgb[m_drag_layer_0][0], 
				m_rgb[m_drag_layer_0][1], m_rgb[m_drag_layer_0][2] ) );
			CPen * old_pen = pDC->SelectObject( &pen0 );
			pDC->LineTo( m_drag_xi, m_drag_yi );

			// draw second segment
			CPen pen1( PS_SOLID, m_drag_w1, RGB( m_rgb[m_drag_layer_1][0], 
				m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );
			pDC->SelectObject( &pen1 );
			pDC->LineTo( m_drag_xf, m_drag_yf );

			if(m_drag_style2 == DSS_STRAIGHT)
			{
				// draw third segment
				CPen pen2( PS_SOLID, m_drag_w2, RGB( m_rgb[m_drag_layer_2][0], 
					m_rgb[m_drag_layer_2][1], m_rgb[m_drag_layer_2][2] ) );
				pDC->SelectObject( &pen2 );
				pDC->LineTo( m_drag_xe, m_drag_ye );
			}
			pDC->SelectObject( old_pen );
		}

	}
	else if( m_drag_flag && (m_drag_shape == DS_ARC_STRAIGHT 
		|| m_drag_shape == DS_ARC_CW || m_drag_shape == DS_ARC_CCW
		|| m_drag_shape == DS_CURVE_CW || m_drag_shape == DS_CURVE_CCW
		) )
	{
		CPen pen_w( PS_SOLID, m_drag_w1, RGB( m_rgb[m_drag_layer_1][0], 
			m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );

		// undraw old arc
		CPen * old_pen = pDC->SelectObject( &pen_w );
		{
			if( m_last_drag_shape == DS_ARC_STRAIGHT )
				DrawArc( pDC, DL_LINE, m_drag_x, m_drag_y, m_drag_xi, m_drag_yi );
			else if( m_last_drag_shape == DS_ARC_CW )
				DrawArc( pDC, DL_ARC_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_last_drag_shape == DS_ARC_CCW )
				DrawArc( pDC, DL_ARC_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_last_drag_shape == DS_CURVE_CW )
				DrawCurve( pDC, DL_CURVE_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_last_drag_shape == DS_CURVE_CCW )
				DrawCurve( pDC, DL_CURVE_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );

			// draw new arc
			if( m_drag_shape == DS_ARC_STRAIGHT )
				DrawArc( pDC, DL_LINE, xx, yy, m_drag_xi, m_drag_yi );
			else if( m_drag_shape == DS_ARC_CW )
				DrawArc( pDC, DL_ARC_CW, m_drag_xi, m_drag_yi, xx, yy );
			else if( m_drag_shape == DS_ARC_CCW )
				DrawArc( pDC, DL_ARC_CCW, m_drag_xi, m_drag_yi, xx, yy );
			else if( m_drag_shape == DS_CURVE_CW )
				DrawCurve( pDC, DL_CURVE_CW, m_drag_xi, m_drag_yi, xx, yy );
			else if( m_drag_shape == DS_CURVE_CCW )
				DrawCurve( pDC, DL_CURVE_CCW, m_drag_xi, m_drag_yi, xx, yy );
			m_last_drag_shape = m_drag_shape;	// used only for polylines
		}
		pDC->SelectObject( old_pen );
	}
	// remember shape data
	m_drag_x = xx;
	m_drag_y = yy;
	
	// now undraw and redraw cross-hairs, if necessary
	if( m_cross_hairs )
	{
		int x = xx;
		int y = yy;
		COLORREF bk = pDC->SetBkColor( RGB(0,0,0) );
		CPen cross_pen( PS_DOT, 0, RGB( 128, 128, 128 ) );
		CPen * old_pen = pDC->SelectObject( &cross_pen );
		{
			pDC->MoveTo( m_cross_left );
			pDC->LineTo( m_cross_right );
			pDC->MoveTo( m_cross_bottom );
			pDC->LineTo( m_cross_top );
			if( m_cross_hairs == 2 )
			{
				pDC->MoveTo( m_cross_topleft );
				pDC->LineTo( m_cross_botright );
				pDC->MoveTo( m_cross_botleft );
				pDC->LineTo( m_cross_topright );
			}
			m_cross_left.x = m_org_x;
			m_cross_left.y = y;
			m_cross_right.x = m_max_x;
			m_cross_right.y = y;
			m_cross_bottom.x = x;
			m_cross_bottom.y = m_org_y;
			m_cross_top.x = x;
			m_cross_top.y = m_max_y;
			if( x-m_org_x > y-m_org_y )
			{
				// bottom-left cursor line intersects m_org_y
				m_cross_botleft.x = x - (y - m_org_y);
				m_cross_botleft.y = m_org_y;
			}
			else
			{
				// bottom-left cursor line intersects m_org_x
				m_cross_botleft.x = m_org_x;
				m_cross_botleft.y = y - (x - m_org_x);
			}
			if( m_max_x-x > y-m_org_y )
			{
				// bottom-right cursor line intersects m_org_y
				m_cross_botright.x = x + (y - m_org_y);
				m_cross_botright.y = m_org_y;
			}
			else
			{
				// bottom-right cursor line intersects m_max_x
				m_cross_botright.x = m_max_x;
				m_cross_botright.y = y - (m_max_x - x);
			}

			if( x-m_org_x > m_max_y-y )
			{
				// top-left cursor line intersects m_max_y
				m_cross_topleft.x = x - (m_max_y - y);
				m_cross_topleft.y = m_max_y;
			}
			else
			{
				// top-left cursor line intersects m_org_x
				m_cross_topleft.x = m_org_x;
				m_cross_topleft.y = y + (x - m_org_x);
			}
			if( m_max_x-x > m_max_y-y )
			{
				// top-right cursor line intersects m_max_y
				m_cross_topright.x = x + (m_max_y - y);
				m_cross_topright.y = m_max_y;
			}
			else
			{
				// top-right cursor line intersects m_max_x
				m_cross_topright.x = m_max_x;
				m_cross_topright.y = y + (m_max_x - x);
			}
			pDC->MoveTo( m_cross_left );
			pDC->LineTo( m_cross_right );
			pDC->MoveTo( m_cross_bottom );
			pDC->LineTo( m_cross_top );
			if( m_cross_hairs == 2 )
			{
				pDC->MoveTo( m_cross_topleft );
				pDC->LineTo( m_cross_botright );
				pDC->MoveTo( m_cross_botleft );
				pDC->LineTo( m_cross_topright );
			}
		}
		pDC->SelectObject( old_pen );
		pDC->SetBkColor( bk );
	}

	// restore drawing mode
	pDC->SetROP2( old_ROP2 );

	return;
}

// Stop dragging 
//
int CDisplayList::StopDragging()
{
	m_drag_flag = 0;
	m_drag_num_lines = 0;
	m_drag_num_ratlines = 0;
	m_cross_hairs = 0;
	m_last_drag_shape = DS_NONE;
	return 0;
}

// Change the drag layer and/or width for DS_LINE_VERTEX
// if ww = 0, don't change width
//
void CDisplayList::ChangeRoutingLayer( CDC * pDC, int layer1, int layer2, int ww )
{
	int w = ww;
	if( !w )
		w = m_drag_w1*m_pcbu_per_wu;

	int old_ROP2 = pDC->GetROP2();
	pDC->SetROP2( R2_XORPEN );
	
	if( m_drag_shape == DS_LINE_VERTEX )
	{
		CPen pen_old( PS_SOLID, 1, RGB( m_rgb[m_drag_layer_2][0], 
						m_rgb[m_drag_layer_2][1], m_rgb[m_drag_layer_2][2] ) );
		CPen pen_old_w( PS_SOLID, m_drag_w1, RGB( m_rgb[m_drag_layer_1][0], 
						m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );
		CPen pen( PS_SOLID, 1, RGB( m_rgb[layer2][0], 
						m_rgb[layer2][1], m_rgb[layer2][2] ) );
		CPen pen_w( PS_SOLID, w/m_pcbu_per_wu, RGB( m_rgb[layer1][0], 
						m_rgb[layer1][1], m_rgb[layer1][2] ) );

		// undraw segments
		CPen * old_pen = pDC->SelectObject( &pen_old_w );
		pDC->MoveTo( m_drag_xi, m_drag_yi );
		pDC->LineTo( m_drag_x, m_drag_y );
		pDC->SelectObject( &pen_old );
		pDC->LineTo( m_drag_xf, m_drag_yf );
		
		// redraw segments
		pDC->SelectObject( &pen_w );
		pDC->MoveTo( m_drag_xi, m_drag_yi );
		pDC->LineTo( m_drag_x, m_drag_y );
		pDC->SelectObject( &pen );
		pDC->LineTo( m_drag_xf, m_drag_yf );								
								
		// update variables
		m_drag_layer_1 = layer1;
		m_drag_layer_2 = layer2;
		m_drag_w1 = w/m_pcbu_per_wu;

		// see if leading via needs to be changed
		if( ( (m_drag_layer_no_via != 0 && m_drag_layer_1 != m_drag_layer_no_via) && !m_drag_via_drawn )
			|| (!(m_drag_layer_no_via != 0 && m_drag_layer_1 != m_drag_layer_no_via) && m_drag_via_drawn ) )
		{
			// draw or undraw via
			int thick = (m_drag_via_w - m_drag_via_holew)/2;
			int w = m_drag_via_w - thick;
			int holew = m_drag_via_holew;
//			CPen pen( PS_SOLID, thick, RGB( m_rgb[LAY_PAD_THRU][0], m_rgb[LAY_PAD_THRU][1], m_rgb[LAY_PAD_THRU][2] ) );
			CPen pen( PS_SOLID, thick, RGB( m_rgb[m_drag_layer_1][0], m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );
			CPen * old_pen = pDC->SelectObject( &pen );
			CBrush black_brush( RGB( 0, 0, 0 ) );
			CBrush * old_brush = pDC->SelectObject( &black_brush );
			pDC->Ellipse( m_drag_xi - w/2, m_drag_yi - w/2, m_drag_xi + w/2, m_drag_yi + w/2 );
			pDC->SelectObject( old_brush );
			pDC->SelectObject( old_pen );
			m_drag_via_drawn = 1 - m_drag_via_drawn;
		}
	}
	else if( m_drag_shape == DS_LINE )
	{
		CPen pen_old_w( PS_SOLID, m_drag_w1, RGB( m_rgb[m_drag_layer_1][0], 
						m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );
		CPen pen( PS_SOLID, 1, RGB( m_rgb[layer2][0], 
						m_rgb[layer2][1], m_rgb[layer2][2] ) );
		CPen pen_w( PS_SOLID, w/m_pcbu_per_wu, RGB( m_rgb[layer1][0], 
						m_rgb[layer1][1], m_rgb[layer1][2] ) );

		// undraw segments
		CPen * old_pen = pDC->SelectObject( &pen_old_w );
		pDC->MoveTo( m_drag_xi, m_drag_yi );
		pDC->LineTo( m_drag_x, m_drag_y );
		
		// redraw segments
		pDC->SelectObject( &pen_w );
		pDC->MoveTo( m_drag_xi, m_drag_yi );
		pDC->LineTo( m_drag_x, m_drag_y );
								
		// update variables
		m_drag_layer_1 = layer1;
		m_drag_w1 = w/m_pcbu_per_wu;

		// see if leading via needs to be changed
		if( ( (m_drag_layer_no_via != 0 && m_drag_layer_1 != m_drag_layer_no_via) && !m_drag_via_drawn )
			|| (!(m_drag_layer_no_via != 0 && m_drag_layer_1 != m_drag_layer_no_via) && m_drag_via_drawn ) )
		{
			// draw or undraw via
			int thick = (m_drag_via_w - m_drag_via_holew)/2;
			int w = m_drag_via_w - thick;
			int holew = m_drag_via_holew;
//			CPen pen( PS_SOLID, thick, RGB( m_rgb[LAY_PAD_THRU][0], m_rgb[LAY_PAD_THRU][1], m_rgb[LAY_PAD_THRU][2] ) );
			CPen pen( PS_SOLID, thick, RGB( m_rgb[m_drag_layer_1][0], m_rgb[m_drag_layer_1][1], m_rgb[m_drag_layer_1][2] ) );
			CPen * old_pen = pDC->SelectObject( &pen );
			CBrush black_brush( RGB( 0, 0, 0 ) );
			CBrush * old_brush = pDC->SelectObject( &black_brush );
			pDC->Ellipse( m_drag_xi - w/2, m_drag_yi - w/2, m_drag_xi + w/2, m_drag_yi + w/2 );
			pDC->SelectObject( old_brush );
			pDC->SelectObject( old_pen );
			m_drag_via_drawn = 1 - m_drag_via_drawn;
		}

		pDC->SelectObject( old_pen );
	}
	
	// restore drawing mode
	pDC->SetROP2( old_ROP2 );

	return;
}

// change angle by adding 90 degrees clockwise
//
void CDisplayList::IncrementDragAngle( CDC * pDC )
{
	m_drag_angle = (m_drag_angle + 90) % 360;

	CPoint zero(0,0);

	CPen drag_pen( PS_SOLID, 1, RGB( m_rgb[m_drag_layer][0], 
					m_rgb[m_drag_layer][1], m_rgb[m_drag_layer][2] ) );
	CPen *old_pen = pDC->SelectObject( &drag_pen );

	int old_ROP2 = pDC->GetROP2();
	pDC->SetROP2( R2_XORPEN );
	
	// erase lines
	for( int il=0; il<m_drag_num_lines; il++ )
	{
		pDC->MoveTo( m_drag_x+m_drag_line_pt[2*il].x, m_drag_y+m_drag_line_pt[2*il].y );
		pDC->LineTo( m_drag_x+m_drag_line_pt[2*il+1].x, m_drag_y+m_drag_line_pt[2*il+1].y );
	}
	for( int il=0; il<m_drag_num_ratlines; il++ )
	{
		pDC->MoveTo( m_drag_ratline_start_pt[il].x, m_drag_ratline_start_pt[il].y );
		pDC->LineTo( m_drag_x+m_drag_ratline_end_pt[il].x, m_drag_y+m_drag_ratline_end_pt[il].y );
	}
		
	// rotate points, redraw lines
	for( int il=0; il<m_drag_num_lines; il++ )
	{
		RotatePoint( &m_drag_line_pt[2*il], 90, zero );
		RotatePoint( &m_drag_line_pt[2*il+1], 90, zero );
		pDC->MoveTo( m_drag_x+m_drag_line_pt[2*il].x, m_drag_y+m_drag_line_pt[2*il].y );
		pDC->LineTo( m_drag_x+m_drag_line_pt[2*il+1].x, m_drag_y+m_drag_line_pt[2*il+1].y );
	}
	for( int il=0; il<m_drag_num_ratlines; il++ )
	{
		RotatePoint( &m_drag_ratline_end_pt[il], 90, zero );
		pDC->MoveTo( m_drag_ratline_start_pt[il].x, m_drag_ratline_start_pt[il].y );
		pDC->LineTo( m_drag_x+m_drag_ratline_end_pt[il].x, m_drag_y+m_drag_ratline_end_pt[il].y );
	}

	pDC->SelectObject( old_pen );
	pDC->SetROP2( old_ROP2 );
	m_drag_vert = 1 - m_drag_vert;
}

// flip to opposite side of board
//
void CDisplayList::FlipDragSide( CDC * pDC )
{
	m_drag_side = 1 - m_drag_side;

	CPen drag_pen( PS_SOLID, 1, RGB( m_rgb[m_drag_layer][0], 
					m_rgb[m_drag_layer][1], m_rgb[m_drag_layer][2] ) );
	CPen *old_pen = pDC->SelectObject( &drag_pen );

	int old_ROP2 = pDC->GetROP2();
	pDC->SetROP2( R2_XORPEN );

	// erase lines
	for( int il=0; il<m_drag_num_lines; il++ )
	{
		pDC->MoveTo( m_drag_x+m_drag_line_pt[2*il].x, m_drag_y+m_drag_line_pt[2*il].y );
		pDC->LineTo( m_drag_x+m_drag_line_pt[2*il+1].x, m_drag_y+m_drag_line_pt[2*il+1].y );
	}
	for( int il=0; il<m_drag_num_ratlines; il++ )
	{
		pDC->MoveTo( m_drag_ratline_start_pt[il].x, m_drag_ratline_start_pt[il].y );
		pDC->LineTo( m_drag_x+m_drag_ratline_end_pt[il].x, m_drag_y+m_drag_ratline_end_pt[il].y );
	}

	// modify drag lines
	for( int il=0; il<m_drag_num_lines; il++ )
	{
		if( m_drag_vert == 0 )
		{
			m_drag_line_pt[2*il].x = -m_drag_line_pt[2*il].x;
			m_drag_line_pt[2*il+1].x = -m_drag_line_pt[2*il+1].x;
		}
		else
		{
			m_drag_line_pt[2*il].y = -m_drag_line_pt[2*il].y;
			m_drag_line_pt[2*il+1].y = -m_drag_line_pt[2*il+1].y;
		}
	}
	for( int il=0; il<m_drag_num_ratlines; il++ )
	{
		if( m_drag_vert == 0 )
		{
			m_drag_ratline_end_pt[il].x = -m_drag_ratline_end_pt[il].x;
		}
		else
		{
			m_drag_ratline_end_pt[il].y = -m_drag_ratline_end_pt[il].y;
		}
	}

	// redraw lines
	for( int il=0; il<m_drag_num_lines; il++ )
	{
		pDC->MoveTo( m_drag_x+m_drag_line_pt[2*il].x, m_drag_y+m_drag_line_pt[2*il].y );
		pDC->LineTo( m_drag_x+m_drag_line_pt[2*il+1].x, m_drag_y+m_drag_line_pt[2*il+1].y );
	}
	for( int il=0; il<m_drag_num_ratlines; il++ )
	{
		pDC->MoveTo( m_drag_ratline_start_pt[il].x, m_drag_ratline_start_pt[il].y );
		pDC->LineTo( m_drag_x+m_drag_ratline_end_pt[il].x, m_drag_y+m_drag_ratline_end_pt[il].y );
	}

	pDC->SelectObject( old_pen );
	pDC->SetROP2( old_ROP2 );

	// if part was vertical, rotate 180 degrees
	if( m_drag_vert )
	{
		IncrementDragAngle( pDC );
		IncrementDragAngle( pDC );
	}
}

// get any changes in side which occurred while dragging
//
int CDisplayList::GetDragSide()
{
	return m_drag_side;
}

// get any changes in angle which occurred while dragging
//
int CDisplayList::GetDragAngle()
{
	return m_drag_angle;
}

int CDisplayList::MakeDragLineArray( int num_lines )
{
	if( m_drag_line_pt )
		free( m_drag_line_pt );
	m_drag_line_pt = (CPoint*)calloc( 2*num_lines, sizeof(CPoint) );
	if( !m_drag_line_pt )
		return 1;

	m_drag_max_lines = num_lines;
	m_drag_num_lines = 0;
	return 0;
}

int CDisplayList::MakeDragRatlineArray( int num_ratlines, int width )
{
	if( m_drag_ratline_start_pt )
		free(m_drag_ratline_start_pt );
	m_drag_ratline_start_pt = (CPoint*)calloc( num_ratlines, sizeof(CPoint) );
	if( !m_drag_ratline_start_pt )
		return 1;

	if( m_drag_ratline_end_pt )
		free(m_drag_ratline_end_pt );
	m_drag_ratline_end_pt = (CPoint*)calloc( num_ratlines, sizeof(CPoint) );
	if( !m_drag_ratline_end_pt )
		return 1;

	m_drag_ratline_width = width;
	m_drag_max_ratlines = num_ratlines;
	m_drag_num_ratlines = 0;
	return 0;
}

int CDisplayList::AddDragLine( CPoint pi, CPoint pf )
{
	if( m_drag_num_lines >= m_drag_max_lines )
		return  1;

	m_drag_line_pt[2*m_drag_num_lines].x = pi.x/m_pcbu_per_wu;
	m_drag_line_pt[2*m_drag_num_lines].y = pi.y/m_pcbu_per_wu;
	m_drag_line_pt[2*m_drag_num_lines+1].x = pf.x/m_pcbu_per_wu;
	m_drag_line_pt[2*m_drag_num_lines+1].y = pf.y/m_pcbu_per_wu;
	m_drag_num_lines++;
	return 0;
}

int CDisplayList::AddDragRatline( CPoint pi, CPoint pf )
{
	if( m_drag_num_ratlines == m_drag_max_ratlines )
		return  1;

	m_drag_ratline_start_pt[m_drag_num_ratlines].x = pi.x/m_pcbu_per_wu;
	m_drag_ratline_start_pt[m_drag_num_ratlines].y = pi.y/m_pcbu_per_wu;
	m_drag_ratline_end_pt[m_drag_num_ratlines].x = pf.x/m_pcbu_per_wu;
	m_drag_ratline_end_pt[m_drag_num_ratlines].y = pf.y/m_pcbu_per_wu;
	m_drag_num_ratlines++;
	return 0;
}

// add element to highlight layer (if the orig_layer is visible)
//
int CDisplayList::HighLight( int gtype, int x, int y, int xf, int yf, int w, int orig_layer )
{
	id h_id;
	Add( h_id, NULL, LAY_HILITE, gtype, 1, w, 0, x, y, xf, yf, x, y, 0, orig_layer );
	return 0;
}

int CDisplayList::CancelHighLight()
{
	RemoveAllFromLayer( LAY_HILITE );
	return 0;
}

// Set the device context and memory context to world coords
//
void CDisplayList::SetDCToWorldCoords( CDC * pDC, CDC * mDC, 
							int pcbu_org_x, int pcbu_org_y )
{
	memDC = NULL;
	if( pDC )
	{
		// set window scale (WU per pixel) and origin (WU)
		pDC->SetMapMode( MM_ANISOTROPIC );
		pDC->SetWindowExt( w_ext_x, w_ext_y );
		pDC->SetWindowOrg( pcbu_org_x/m_pcbu_per_wu, pcbu_org_y/m_pcbu_per_wu );

		// set viewport to client rect with origin in lower left
		// leave room for m_left_pane to the left of the PCB drawing area
		pDC->SetViewportExt( v_ext_x, v_ext_y );
		pDC->SetViewportOrg( m_pane_org_x, m_pane_org_y );
	}
	if( mDC->m_hDC )
	{
		// set window scale (units per pixel) and origin (units)
		mDC->SetMapMode( MM_ANISOTROPIC );
		mDC->SetWindowExt( w_ext_x, w_ext_y );
		mDC->SetWindowOrg( pcbu_org_x/m_pcbu_per_wu, pcbu_org_y/m_pcbu_per_wu );

		// set viewport to client rect with origin in lower left
		// leave room for m_left_pane to the left of the PCB drawing area
		mDC->SetViewportExt( v_ext_x, v_ext_y );
		mDC->SetViewportOrg( m_pane_org_x, m_pane_org_y );
		
		// update pointer
		memDC = mDC;
	}
}

void CDisplayList::SetVisibleGrid( BOOL on, double grid )
{
	m_visual_grid_on = on;
	m_visual_grid_spacing = grid/m_pcbu_per_wu;
}


void CDisplayList::SetUpCrosshairs( int type, int x, int y )
{
	// set up cross hairs
	m_cross_hairs = type;
	m_cross_left.x = m_org_x;
	m_cross_left.y = y;
	m_cross_right.x = m_max_x;
	m_cross_right.y = y;
	m_cross_bottom.x = x;
	m_cross_bottom.y = m_org_y;
	m_cross_top.x = x;
	m_cross_top.y = m_max_y;
	if( x-m_org_x > y-m_org_y )
	{
		// bottom-left cursor line intersects m_org_y
		m_cross_botleft.x = x - (y - m_org_y);
		m_cross_botleft.y = m_org_y;
	}
	else
	{
		// bottom-left cursor line intersects m_org_x
		m_cross_botleft.x = m_org_x;
		m_cross_botleft.y = y - (x - m_org_x);
	}
	if( m_max_x-x > y-m_org_y )
	{
		// bottom-right cursor line intersects m_org_y
		m_cross_botright.x = x + (y - m_org_y);
		m_cross_botright.y = m_org_y;
	}
	else
	{
		// bottom-right cursor line intersects m_max_x
		m_cross_botright.x = m_max_x;
		m_cross_botright.y = y - (m_max_x - x);
	}

	if( x-m_org_x > m_max_y-y )
	{
		// top-left cursor line intersects m_max_y
		m_cross_topleft.x = x - (m_max_y - y);
		m_cross_topleft.y = m_max_y;
	}
	else
	{
		// top-left cursor line intersects m_org_x
		m_cross_topleft.x = m_org_x;
		m_cross_topleft.y = y - (x - m_org_x);
	}
	if( m_max_x-x > m_max_y-y )
	{
		// top-right cursor line intersects m_max_y
		m_cross_topright.x = x + (m_max_y - y);
		m_cross_topright.y = m_max_y;
	}
	else
	{
		// top-right cursor line intersects m_max_x
		m_cross_topright.x = m_max_x;
		m_cross_topright.y = y + (m_max_x - x);
	}
}

// Convert point in window coords to PCB units (i.e. nanometers)
//
CPoint CDisplayList::WindowToPCB( CPoint point )
{
	CPoint p;     
	double test = ((point.x-m_pane_org_x)*m_wu_per_pixel_x + m_org_x)*m_pcbu_per_wu;
	p.x = test;    
	test = ((point.y-m_pane_org_y)*m_wu_per_pixel_y + m_org_y)*m_pcbu_per_wu;
	p.y = test;
	return p;
}

// Convert point in screen coords to PCB units
//
CPoint CDisplayList::ScreenToPCB( CPoint point )
{
	CPoint p;
	p.x = point.x - m_screen_r.left;
	p.y = point.y - m_screen_r.top;
	p = WindowToPCB( p );
	return p;
}

// Convert point in PCB units to screen coords
//
CPoint CDisplayList::PCBToScreen( CPoint point )
{
	CPoint p;
	p.x = (point.x - m_org_x*m_pcbu_per_wu)/m_pcbu_per_pixel_x+m_pane_org_x+m_screen_r.left;
	p.y = (point.y - m_org_y*m_pcbu_per_wu)/m_pcbu_per_pixel_y-m_bottom_pane_h+m_screen_r.bottom; 
	return p;
}



