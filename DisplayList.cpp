// DisplayList.cpp : implementation of class CDisplayList
//
// this is a linked-list of display elements
// each element is a primitive graphics object such as a line-segment,
// circle, annulus, etc.
//
#include "stdafx.h"
#include <math.h>
#include "memdc.h"

#include "dle_arc.h"
#include "dle_centroid.h"
#include "dle_circle.h"
#include "dle_donut.h"
#include "dle_hole.h"
#include "dle_line.h"
#include "dle_octagon.h"
#include "dle_oval.h"
#include "dle_rect.h"
#include "dle_rect_rounded.h"
#include "dle_square.h"
#include "dle_x.h"

// dimensions passed to DisplayList from the application are in PCBU (i.e. nm)
// since the Win95/98 GDI uses 16-bit arithmetic, PCBU must be scaled to DU (i.e. mils)
//
//#define PCBU_PER_DU		PCBU_PER_WU
//#define MIL_PER_DU		NM_PER_MIL/PCBU_PER_DU	// conversion from mils to display units
//#define DU_PER_MIL		PCBU_PER_DU/NM_PER_MIL	// conversion from display units to mils

#define DL_MAX_LAYERS	32
#define HILITE_POINT_W	10	// size/2 of selection box for points (mils)

dl_element::dl_element()
{
	prev = next = NULL;
}


CDisplayList::CDisplayList( int pcbu_per_wu, SMFontUtil * fontutil )
{
	m_pcbu_per_wu = pcbu_per_wu;
	m_fontutil = fontutil;

	// create lists for all layers
	for( int layer=0; layer<MAX_LAYERS; layer++ )
	{
		// default colors, these should be overwritten with actual colors
		m_rgb[layer].Set(
			layer*63,      // R
			(layer/4)*127, // G
			(layer/8)*63   // B
		);

		// default order
		m_layer_in_order[layer] = layer;	// will be drawn from highest to lowest
		m_order_for_layer[layer] = layer;
		m_vis[layer] = 0;

	}
	// miscellaneous
	m_ratline_w = 1;
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

CDisplayList::~CDisplayList()
{
	RemoveAll();
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

	// CPT:  new system replacing Brian's.  "element" will unhook itself from its linked list:
	element->Unhook();
	// end CPT

	// destroy element
	id el_id = element->id;
	delete( element );

	return el_id;
}

void CDisplayList::RemoveAllFromLayer( int layer )
{
	// CPT: new system replacing Brian's.  Delete all elements from linked list layers[layer].elements:
	CDisplayLayer *dl = &layers[layer];
	for (dl_element *el = dl->elements, *next; el; el = next)
		next = el->next,
		delete el;
	dl->elements = NULL;
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


void CDisplayList::Scale_pcbu_to_wu(CRect &rect)
{
	rect.top    /= m_pcbu_per_wu;
	rect.bottom /= m_pcbu_per_wu;
	rect.left   /= m_pcbu_per_wu;
	rect.right  /= m_pcbu_per_wu;
}


dl_element * CDisplayList::CreateDLE( int gtype )
{
	// create new element and link into list
	dl_element * new_element;

	switch( gtype )
	{
		case DL_LINE:           new_element = new CDLE_LINE;           break;
		case DL_CIRC:           new_element = new CDLE_CIRC;           break;
		case DL_DONUT:          new_element = new CDLE_DONUT;          break;
		case DL_SQUARE:         new_element = new CDLE_SQUARE;         break;
		case DL_RECT:           new_element = new CDLE_RECT;           break;
		case DL_RRECT:          new_element = new CDLE_RRECT;          break;
		case DL_OVAL:           new_element = new CDLE_OVAL;           break;
		case DL_OCTAGON:        new_element = new CDLE_OCTAGON;        break;
		case DL_HOLE:           new_element = new CDLE_HOLE;           break;
		case DL_HOLLOW_CIRC:    new_element = new CDLE_HOLLOW_CIRC;    break;
		case DL_HOLLOW_RECT:    new_element = new CDLE_HOLLOW_RECT;    break;
		case DL_HOLLOW_RRECT:   new_element = new CDLE_HOLLOW_RRECT;   break;
		case DL_HOLLOW_OVAL:    new_element = new CDLE_HOLLOW_OVAL;    break;
		case DL_HOLLOW_OCTAGON: new_element = new CDLE_HOLLOW_OCTAGON; break;
		case DL_RECT_X:         new_element = new CDLE_RECT_X;         break;
		case DL_ARC_CW:         new_element = new CDLE_ARC_CW;         break;
		case DL_ARC_CCW:        new_element = new CDLE_ARC_CCW;        break;
		case DL_CENTROID:       new_element = new CDLE_CENTROID;       break;
		case DL_X:              new_element = new CDLE_X;              break;

		default:                new_element = new dl_element;          break;
	}

	// now copy data from entry into element
	new_element->magic = DL_MAGIC;
	new_element->dlist = this;
	new_element->gtype = gtype;

    return new_element;
}


dl_element * CDisplayList::CreateDLE( id id, void * ptr, int layer, int gtype, int visible,
                                      int w, int holew, int clearancew,
                                      int x, int y, int xf, int yf, int xo, int yo, int radius,
                                      int orig_layer )
{
	// create new element
	dl_element * new_element = CreateDLE( gtype );

#if 0	// code added by Brian, doesn't work in fp_editor
	if( layer == LAY_RAT_LINE )
	{
		new_element->w = m_ratline_w;
	}
	else
	{
		new_element->w = (w < 0) ? w : w / m_pcbu_per_wu;
	}
#endif

	// now copy data from entry into element
	new_element->id         = id;
	new_element->ptr        = ptr;
	new_element->visible    = visible;
	new_element->w			= w			 / m_pcbu_per_wu;
	new_element->holew      = holew      / m_pcbu_per_wu;
	new_element->clearancew = clearancew / m_pcbu_per_wu;
	new_element->i.x        = x          / m_pcbu_per_wu;
	new_element->i.y        = y          / m_pcbu_per_wu;
	new_element->f.x        = xf         / m_pcbu_per_wu;
	new_element->f.y        = yf         / m_pcbu_per_wu;
	new_element->org.x      = xo         / m_pcbu_per_wu;
	new_element->org.y      = yo         / m_pcbu_per_wu;
	new_element->radius     = radius     / m_pcbu_per_wu;
	new_element->sel_vert   = 0;
	new_element->layer      = layer;
	new_element->orig_layer = orig_layer;

	return new_element;
}


// Add entry to end of list, returns pointer to element created.
//
// Dimensional units for input parameters are PCBU
//
dl_element * CDisplayList::Add( id id, void * ptr, int layer, int gtype, int visible,
	                            int w, int holew, int clearancew,
                                int x, int y, int xf, int yf, int xo, int yo,
	                            int radius, int orig_layer )
{
	// create new element
	dl_element * new_element = CreateDLE( id, ptr, layer, gtype, visible,
	                                      w, holew, clearancew,
	                                      x,y, xf,yf, xo,yo, radius,
	                                      orig_layer );
	layers[layer].Add(new_element);										// CPT:  new system replacing Brian's
	return new_element;
}

// add graphics element used for selection
//
dl_element * CDisplayList::AddSelector( id id, void * ptr, int layer, int gtype, int visible,
							   int w, int holew, int x, int y, int xf, int yf, int xo, int yo,
							   int radius )
{
	// create new element
	//** for debugging
	if( gtype == DL_HOLLOW_RECT )
	{
		int w_in_mils = abs(x-xf)/PCBU_PER_MIL;	
		w_in_mils = w_in_mils;
	}
	//**
	dl_element * new_element = CreateDLE( id, ptr, layer, gtype, visible,
	                                      w, holew, 0,
	                                      x,y, xf,yf, xo,yo, radius,
	                                      layer );

	layers[LAY_SELECTION].Add(new_element);
	return new_element;
}


// BEGIN CPT2.  As I begin contemplating the phase-out of class id, I need versions of dl-elements that make use of cpcb_item pointers instead.

dl_element * CDisplayList::CreateDLE( cpcb_item *item, int usage, int layer, int gtype, int visible,
								int w, int holew, int clearancew,
								int x, int y, int xf, int yf, int xo, int yo, int radius,
								int orig_layer )
{
	// create new element
	dl_element * new_element = CreateDLE( gtype );
	// now copy data from entry into element
	new_element->item       = item;
	new_element->usage      = usage;
	new_element->visible    = visible;
	new_element->w			= w			 / m_pcbu_per_wu;
	new_element->holew      = holew      / m_pcbu_per_wu;
	new_element->clearancew = clearancew / m_pcbu_per_wu;
	new_element->i.x        = x          / m_pcbu_per_wu;
	new_element->i.y        = y          / m_pcbu_per_wu;
	new_element->f.x        = xf         / m_pcbu_per_wu;
	new_element->f.y        = yf         / m_pcbu_per_wu;
	new_element->org.x      = xo         / m_pcbu_per_wu;
	new_element->org.y      = yo         / m_pcbu_per_wu;
	new_element->radius     = radius     / m_pcbu_per_wu;
	new_element->sel_vert   = 0;
	new_element->layer      = layer;
	new_element->orig_layer = orig_layer;

	return new_element;
}


dl_element * CDisplayList::Add( cpcb_item *item, int usage, int layer, int gtype, int visible,
	                            int w, int holew, int clearancew,
                                int x, int y, int xf, int yf, int xo, int yo,
	                            int radius, int orig_layer )
{
	dl_element * new_element = CreateDLE( item, usage, layer, gtype, visible,
	                                      w, holew, clearancew,
	                                      x,y, xf,yf, xo,yo, radius,
	                                      orig_layer );
	layers[layer].Add(new_element);										// CPT:  new system replacing Brian's
	return new_element;
}

dl_element * CDisplayList::AddMain( cpcb_item *item, int layer, int gtype, int visible,
	                            int w, int holew, int clearancew,
                                int x, int y, int xf, int yf, int xo, int yo,
	                            int radius, int orig_layer )
{
	// CPT2 convenience method similar to Add(), so that we don't have to type "dl_element::DL_MAIN" every time we're creating a plain-vanilla dl-element
	dl_element * new_element = CreateDLE( item, dl_element::DL_MAIN, layer, gtype, visible,
	                                      w, holew, clearancew,
	                                      x,y, xf,yf, xo,yo, radius,
	                                      orig_layer );
	layers[layer].Add(new_element);										// CPT:  new system replacing Brian's
	return new_element;
}

// add graphics element used for selection.  Another convenience method similar to Add()
//
dl_element * CDisplayList::AddSelector( cpcb_item *item, int layer, int gtype, int visible,
							   int w, int holew, int x, int y, int xf, int yf, int xo, int yo,
							   int radius )
{
	dl_element * new_element = CreateDLE( item, dl_element::DL_SEL, layer, gtype, visible,
	                                      w, holew, 0,
	                                      x,y, xf,yf, xo,yo, radius,
	                                      layer );

	layers[LAY_SELECTION].Add(new_element);
	return new_element;
}

// END CPT2

void CDisplayList::Add( dl_element * element )
{
	layers[element->layer].Add(element);								// CPT:  new system replacing Brian's
}




// set element parameters in PCBU
//
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
void CDisplayList::Set_clearance( dl_element * el, int clearance )
{
	if( el)
		el->clearancew = clearance/m_pcbu_per_wu;
}
void CDisplayList::Set_holew( dl_element * el, int holew )
{
	if( el)
		el->holew = holew/m_pcbu_per_wu;
}
void CDisplayList::Set_x_org( dl_element * el, int x_org )
{
	if( el)
		el->org.x = x_org/m_pcbu_per_wu;
}
void CDisplayList::Set_y_org( dl_element * el, int y_org )
{
	if( el)
		el->org.y = y_org/m_pcbu_per_wu;
}
void CDisplayList::Set_x( dl_element * el, int x )
{
	if( el)
		el->i.x = x/m_pcbu_per_wu;
}
void CDisplayList::Set_y( dl_element * el, int y )
{
	if( el)
		el->i.y = y/m_pcbu_per_wu;
}
void CDisplayList::Set_xf( dl_element * el, int xf )
{
	if( el)
		el->f.x = xf/m_pcbu_per_wu;
}
void CDisplayList::Set_yf( dl_element * el, int yf )
{
	if( el)
		el->f.y = yf/m_pcbu_per_wu;
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
void CDisplayList::Set_gtype( dl_element * el, int gtype )
{
	if( el )
		el->gtype = gtype;
}



void CDisplayList::Move( dl_element * el, int dx, int dy )
{
	el->i.x += dx;
	el->i.y += dy;
	el->org.x += dx;
	el->org.y += dy;
	el->f.x += dx;
	el->f.y += dy;
}


// get element parameters in PCBU
//
void * CDisplayList::Get_ptr( dl_element * el ) { return el->ptr; }
int CDisplayList::Get_gtype( dl_element * el ) { return el->gtype; }
int CDisplayList::Get_visible( dl_element * el ) { return el->visible; }
int CDisplayList::Get_sel_vert( dl_element * el ) { return el->sel_vert; }
int CDisplayList::Get_w( dl_element * el ) { return el->w*m_pcbu_per_wu; }
int CDisplayList::Get_holew( dl_element * el ) { return el->holew*m_pcbu_per_wu; }
int CDisplayList::Get_x_org( dl_element * el ) { return el->org.x*m_pcbu_per_wu; }
int CDisplayList::Get_y_org( dl_element * el ) { return el->org.y*m_pcbu_per_wu; }
int CDisplayList::Get_x( dl_element * el ) { return el->i.x*m_pcbu_per_wu; }
int CDisplayList::Get_y( dl_element * el ) { return el->i.y*m_pcbu_per_wu; }
int CDisplayList::Get_xf( dl_element * el ) { return el->f.x*m_pcbu_per_wu; }
int CDisplayList::Get_yf( dl_element * el ) { return el->f.y*m_pcbu_per_wu; }
int CDisplayList::Get_radius( dl_element * el ) { return el->radius*m_pcbu_per_wu; }
int CDisplayList::Get_layer( dl_element * el ) { return el->layer; }
id  CDisplayList::Get_id( dl_element * el ) { return el->id; }

void CDisplayList::Get_Endpoints(CPoint *cpi, CPoint *cpf)
{
	cpi->x = m_drag_xi*m_pcbu_per_wu; cpi->y = m_drag_yi*m_pcbu_per_wu;
	cpf->x = m_drag_xf*m_pcbu_per_wu; cpf->y = m_drag_yf*m_pcbu_per_wu;
}


// Draw the display list using device DC or memory DC.  Routine is mostly CPT.
//
void CDisplayList::Draw( CDC * dDC )
{
	CDC * pDC;
	if( memDC )
		pDC = memDC;
	else
		pDC = dDC;

	pDC->SetBkColor( C_RGB::black );

	// create pens and brushes
	CPen black_pen  ( PS_SOLID, 1, C_RGB::black );					// CPT fixed (was C_RGB::white)
	CPen white_pen  ( PS_SOLID, 1, C_RGB::white );					// CPT fixed (was C_RGB::black)
	CPen grid_pen   ( PS_SOLID, 1, m_rgb[LAY_VISIBLE_GRID] );
	CPen backgnd_pen( PS_SOLID, 1, m_rgb[LAY_BACKGND] );
	CBrush black_brush( C_RGB::black );
	CBrush backgnd_brush( m_rgb[LAY_BACKGND] );

	CBrush * old_brush;
	CPen * old_pen;

	// paint the background color
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
				pDC->SetPixel( ix, iy, m_rgb[LAY_VISIBLE_GRID] );
			}
		}
	}


	// now traverse the lists, starting with the layer in the last element
	// of the m_order[] array
	CDrawInfo di;
	di.DC_Master = pDC;

	for( int order=(MAX_LAYERS-1); order>=0; order-- )
	{
		int layer = m_layer_in_order[order];
		if( !m_vis[layer] || layer == LAY_SELECTION )
			continue;

#if 0
		if( layer > LAY_BOARD_OUTLINE )
		{
			// Use transparent DC in dcMemory
			di.DC = &dcMemory;

			dcMemory.mono();

			di.layer_color[0] = C_RGB::mono_off;
			di.layer_color[1] = C_RGB::mono_on;
		}
		else
#endif
		{
			// Draw directly on main DC (di.DC_Master) for speed
			di.DC = di.DC_Master;
			di.layer_color[0] = m_rgb[LAY_BACKGND];
			di.layer_color[1] = m_rgb[layer];
		}

		// CPT: Draw layer!
		if (layer==LAY_HILITE || layer==LAY_RAT_LINE) 
		{
			// For the highlight and ratline layers, we are going to perform some alpha-blending before drawing the highlights proper.
			// Onto memDC2, we will first BitBlt the current contents of pDC.  Then we draw fatter versions of all the line segments in the
			// highlight layer, using the background color.  Finally we perform the alpha-blending of memDC2 onto pDC, using a constant alpha of
			// .5.  That way the pixels next to highlighted segs are lightened (in the case of a white bkgnd) or darkened (in the case of a black
			// bkgnd).
			CRect r (m_client_r), r2 = r;
			pDC->DPtoLP(r);
			memDC2->BitBlt(r.left, r.top, r.Width(), r.Height(), pDC, r.left, r.top, SRCCOPY);
			// Draw the fattened segs within the layer.  Setting di.bHiliteSegs before calling CDisplayLayer.Draw() does the trick:
			di.DC = memDC2;
			layers[layer].Draw(di, true);
			di.DC = pDC;
			// Now do the alpha-blending!  Unfortunately AlphaBlend() screws up with the typical current mapping-mode values for pDC and memDC2 (because 
			// y-values are negated, I think).  Therefore, we have to temporarily change the mapping-mode on both to MM_TEXT.
			int mm = pDC->GetMapMode();
			CSize wext = pDC->GetWindowExt();
			CSize vext = pDC->GetViewportExt();
			pDC->SetMapMode(MM_TEXT); memDC2->SetMapMode(MM_TEXT);
			pDC->DPtoLP(r2);
			int left = r2.left, top = r2.top, w = r2.Width(), h = r2.Height();
			BLENDFUNCTION bf;														// Argument for AlphaBlend...
			bf.BlendOp = AC_SRC_OVER;
			bf.BlendFlags = 0;
			bf.SourceConstantAlpha = 128;
			bf.AlphaFormat = 0;
			pDC->AlphaBlend(left, top, w, h, memDC2, left, top, w, h, bf);
			pDC->SetMapMode(mm); memDC2->SetMapMode(mm);
			pDC->SetWindowExt(wext); memDC2->SetWindowExt(wext);
			pDC->SetViewportExt(vext); memDC2->SetViewportExt(vext);
		}

		layers[layer].Draw(di, false);

		/*  CPT:  obsolete?
		if( di.DC != di.DC_Master )
		{
			// di.DC is a monochrome mask

			// DC_Master &= ~mask
			//   0 = transparent (AND with ~RGB(0,0,0) -> no effect, D AND 1 = D)
			//   1 = solid       (AND with ~RGB(255,255,255) -> clear the masked area to RGB(0,0,0)
			di.DC_Master->SetTextColor(RGB(0,0,0));
			di.DC_Master->SetBkColor(RGB(255,255,255));

			di.DC_Master->BitBlt(m_org_x, m_org_y, m_max_x-m_org_x, m_max_y-m_org_y,
			                     di.DC,
			                     m_org_x, m_org_y, 0x00220326);

			// DC_Master |= mask
			//   0 = transparent (OR with RBG(0,0,0) -> no effect D OR 0 = D)
			//   1 = solid       (OR the masked area with layer color -> 0 OR C = C)
			di.DC_Master->SetBkColor( m_rgb[layer] );

			di.DC_Master->BitBlt(m_org_x, m_org_y, m_max_x-m_org_x, m_max_y-m_org_y,
			                     di.DC,
			                     m_org_x, m_org_y, SRCPAINT);
		}
		*/
	}

//**	dcMemory.DeleteDC();

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
		CPen drag_pen( PS_SOLID, 1, m_rgb[m_drag_layer] );
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
		// draw ratline array, dragging endpoint m_drag_ratline_end_pt
		CPen drag_pen( PS_SOLID, m_drag_ratline_width, m_rgb[m_drag_layer] );
		CPen * old_pen = pDC->SelectObject( &drag_pen );
		for( int il=0; il<m_drag_num_ratlines; il++ )
		{
			pDC->MoveTo( m_drag_ratline_start_pt[il] );
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
			CPen pen0( PS_SOLID, m_drag_w0, m_rgb[m_drag_layer_0] );
			CPen * old_pen = pDC->SelectObject( &pen0 );
			pDC->LineTo( m_drag_xi, m_drag_yi );

			// draw second segment
			CPen pen1( PS_SOLID, m_drag_w1, m_rgb[m_drag_layer_1] );
			pDC->SelectObject( &pen1 );
			pDC->LineTo( m_drag_xf, m_drag_yf );

			// draw third segment
			if(m_drag_style2 != DSS_NONE)
			{
				CPen pen2( PS_SOLID, m_drag_w2, m_rgb[m_drag_layer_2] );
				pDC->SelectObject( &pen2 );
				pDC->LineTo( m_drag_xe, m_drag_ye );
			}
			pDC->SelectObject( old_pen );
		}
		// draw drag shape, if used
		if( m_drag_shape == DS_LINE_VERTEX || m_drag_shape == DS_LINE )
		{
			CPen pen_w( PS_SOLID, m_drag_w1, m_rgb[m_drag_layer_1] );

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
			else
				ASSERT(0);
			if( m_drag_shape == DS_LINE_VERTEX )
			{
				CPen pen( PS_SOLID, m_drag_w2, m_rgb[m_drag_layer_2] );

				pDC->SelectObject( &pen );
				if( m_drag_style2 == DSS_STRAIGHT )
					pDC->LineTo( m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_ARC_CW )
					DrawArc( pDC, DL_ARC_CW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_ARC_CCW )
					DrawArc( pDC, DL_ARC_CCW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
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
//				CPen pen( PS_SOLID, thick, m_rgb[LAY_PAD_THRU] );
				CPen pen( PS_SOLID, thick, m_rgb[m_drag_layer_1] );
				CPen * old_pen = pDC->SelectObject( &pen );
				CBrush black_brush( C_RGB::black );
				CBrush * old_brush = pDC->SelectObject( &black_brush );
				pDC->Ellipse( m_drag_xi - w/2, m_drag_yi - w/2, m_drag_xi + w/2, m_drag_yi + w/2 );
				pDC->SelectObject( old_brush );
				pDC->SelectObject( old_pen );
			}

		}
		else if( m_drag_shape == DS_ARC_STRAIGHT || m_drag_shape == DS_ARC_CW || m_drag_shape == DS_ARC_CCW )
		{
			CPen pen_w( PS_SOLID, m_drag_w1, m_rgb[m_drag_layer_1] );

			// redraw dragged shape
			pDC->SelectObject( &pen_w );
			if( m_drag_shape == DS_ARC_STRAIGHT )
				DrawArc( pDC, DL_LINE, m_drag_x, m_drag_y, m_drag_xi, m_drag_yi );
			else if( m_drag_shape == DS_ARC_CW )
				DrawArc( pDC, DL_ARC_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_drag_shape == DS_ARC_CCW )
				DrawArc( pDC, DL_ARC_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			pDC->SelectObject( old_pen );
			m_last_drag_shape = m_drag_shape;
		}
	}

	// if cross hairs, draw them
	if( m_cross_hairs )
	{
		// draw cross-hairs
		COLORREF bk = pDC->SetBkColor( C_RGB::black );
		CPen pen( PS_DOT, 0, C_RGB::grey );
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
		dDC->BitBlt( m_org_x, m_org_y, m_max_x-m_org_x, m_max_y-m_org_y, pDC, m_org_x, m_org_y, SRCCOPY );
	}
}

// set the display color for a layer
//
void CDisplayList::SetLayerRGB( int layer, C_RGB color )
{
	m_rgb[layer] = color;
}

void CDisplayList::SetLayerVisible( int layer, BOOL vis )
{
	m_vis[layer] = vis;
}

COLORREF CDisplayList::GetLayerColor( int layer )
{
	return m_rgb[layer];
}

int CompareHits(const CHitInfo *h1, const CHitInfo *h2) {
	// CPT.  Used when sorting a list of hits (see TestSelect() just below). 
	if (h1->priority==h2->priority)
		return h1->dist < h2->dist? -1: 1;
	return h1->priority < h2->priority? 1: -1; 
	}

// CPT r294, reworked arguments, and made corresponding changes to function body.
// Test x,y for a hit on an item in the selection layer.
// CPT2.  Now fills the CHitInfo::item member.  TODO will dump ID and ptr elements eventually.
// Creates array hit_info containing layer and id of each hit item
//   and also priority and distance values from (x,y).  
// CPT2.  Selection mask is implemented by looking at argument maskBits.  If a given item returns a GetTypeBit() that is part of the mask, it
//   is allowed for selection.
// New arg bCtrl is true if user is currently ctrl-clicking.  In that case we exclude e.g. vertices and ratlines, which can't belong to groups.
// Arg "net," if non-null, means that we must limit the results to members of that net.  Likewise layer, if >=0, indicates a layer filter.
// Function now always sorts hit_info.  Return value is now the number of elements in hit_info.
//
// CPT: also removed references to Brian's CDL_job classes, and tidied up.

int CDisplayList::TestSelect( int x, int y, CArray<CHitInfo> *hit_info, int maskBits, bool bCtrl, cnet2 *net, int layer )
{
	if(!m_vis[LAY_SELECTION] ) return -1;				// CPT: irrelevant??

	double xx = double(x)/m_pcbu_per_wu;				// CPT (was int, which caused range issues)
	double yy = double(y)/m_pcbu_per_wu;				// CPT (was int)

	// Get array of info for _all_ elements that hit (xx,yy).
	CArray<CHitInfo> hit_info0;
	int num_hits = TestForHits(xx, yy, &hit_info0);

	hit_info->RemoveAll();
	// Now cull the array out based on include_id and other criteria.  Put the winners from hit_info0 into hit_info.
	for( int i=0; i<num_hits; i++ )
	{
		CHitInfo *this_hit = &hit_info0[i];
		cpcb_item *this_item = this_hit->item;
		if (bCtrl && !this_item->IsSelectableForGroup()) 
			continue;
		if (!(this_item->GetTypeBit() & maskBits)) 
			continue;
		if (net && this_item->GetNet()!=net)
			continue;
		if (layer>=0 && this_item->GetLayer()!=layer && this_item->GetLayer()!=LAY_PAD_THRU)
			continue;

		// OK, valid hit, now add to final array hit_info, and assign priority
		// start with reversed layer drawing order * 10
		// i.e. last drawn = highest priority
		hit_info->Add(*this_hit);
		int priority = (MAX_LAYERS - m_order_for_layer[this_hit->layer])*10;

		// bump priority for small items which may be overlapped by larger items on same layer
		if (this_item->IsRefText())
			priority++; 
		else if (this_item->IsValueText())
			priority++;
		else if	(this_item->IsCorner())
			priority++;
		else if ((this_item->IsVertex() || this_item->IsTee()) &&! this_item->IsVia())
			priority++;

		this_hit->priority = priority;
	}

	// Sort output array and return.
	num_hits = hit_info->GetCount();
	qsort(hit_info->GetData(), num_hits, sizeof(CHitInfo), (int (*)(const void*,const void*)) CompareHits);
	return num_hits;
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
int CDisplayList::StartDraggingRectangle( CDC * pDC, int x, int y, int xi, int yi, int xf, int yf, int vert, int layer )
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
int CDisplayList::StartDraggingRatLine( CDC * pDC, int x, int y, int xi, int yi, int layer, int w, int crosshair )
{
	// create drag line
	CPoint p1(xi,yi);
	CPoint p2(x,y);
	MakeDragRatlineArray( 1, 1 );
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
int CDisplayList::StartDraggingArc( CDC * pDC, int style, int xx, int yy, int xi, int yi, int layer, int w, int crosshair )
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
	// both endpoints of each line are dragged
	if( m_drag_num_lines )
	{
		CPen drag_pen( PS_SOLID, 1, m_rgb[m_drag_layer] );
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
	// one endpoint of each line is dragged, the other is fixed
	if( m_drag_num_ratlines )
	{
		CPen drag_pen( PS_SOLID, m_drag_ratline_width, m_rgb[m_drag_layer] );
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
		CPen pen_w( PS_SOLID, m_drag_w1, m_rgb[m_drag_layer_1] );

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
			else
				ASSERT(0);
			if( m_drag_shape == DS_LINE_VERTEX )
			{
				// undraw second segment
				CPen pen( PS_SOLID, m_drag_w2, m_rgb[m_drag_layer_2] );

				CPen * old_pen = pDC->SelectObject( &pen );
				if( m_drag_style2 == DSS_STRAIGHT )
					pDC->LineTo( m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_ARC_CW )
					DrawArc( pDC, DL_ARC_CW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_ARC_CCW )
					DrawArc( pDC, DL_ARC_CCW, m_drag_x, m_drag_y, m_drag_xf, m_drag_yf );
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
			else
				ASSERT(0);
			if( m_drag_shape == DS_LINE_VERTEX )
			{
				// draw second segment
				CPen pen( PS_SOLID, m_drag_w2, m_rgb[m_drag_layer_2] );

				CPen * old_pen = pDC->SelectObject( &pen );
				if( m_drag_style2 == DSS_STRAIGHT )
					pDC->LineTo( m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_ARC_CW )
					DrawArc( pDC, DL_ARC_CW, xx, yy, m_drag_xf, m_drag_yf );
				else if( m_drag_style2 == DSS_ARC_CCW )
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
//				CPen pen( PS_SOLID, thick, m_rgb[LAY_PAD_THRU] );
				CPen pen( PS_SOLID, thick, m_rgb[m_drag_layer_1] );
				CPen * old_pen = pDC->SelectObject( &pen );
				{
					CBrush black_brush( C_RGB::black );
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
			CPen pen0( PS_SOLID, m_drag_w0, m_rgb[m_drag_layer_0] );
			CPen * old_pen = pDC->SelectObject( &pen0 );
			pDC->LineTo( m_drag_xi, m_drag_yi );

			// undraw second segment
			ASSERT(m_drag_style1 == DSS_STRAIGHT);
			CPen pen1( PS_SOLID, m_drag_w1, m_rgb[m_drag_layer_1] );
			pDC->SelectObject( &pen1 );
			pDC->LineTo( m_drag_xf, m_drag_yf );

			// undraw third segment
			if(m_drag_style2 == DSS_STRAIGHT)		// Could also be DSS_NONE (this segment only)
			{
				CPen pen2( PS_SOLID, m_drag_w2, m_rgb[m_drag_layer_2] );
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
			CPen pen0( PS_SOLID, m_drag_w0, m_rgb[m_drag_layer_0] );
			CPen * old_pen = pDC->SelectObject( &pen0 );
			pDC->LineTo( m_drag_xi, m_drag_yi );

			// draw second segment
			CPen pen1( PS_SOLID, m_drag_w1, m_rgb[m_drag_layer_1] );
			pDC->SelectObject( &pen1 );
			pDC->LineTo( m_drag_xf, m_drag_yf );

			if(m_drag_style2 == DSS_STRAIGHT)
			{
				// draw third segment
				CPen pen2( PS_SOLID, m_drag_w2, m_rgb[m_drag_layer_2] );
				pDC->SelectObject( &pen2 );
				pDC->LineTo( m_drag_xe, m_drag_ye );
			}
			pDC->SelectObject( old_pen );
		}

	}
	else if( m_drag_flag && (m_drag_shape == DS_ARC_STRAIGHT || m_drag_shape == DS_ARC_CW || m_drag_shape == DS_ARC_CCW) )
	{
		CPen pen_w( PS_SOLID, m_drag_w1, m_rgb[m_drag_layer_1] );

		// undraw old arc
		CPen * old_pen = pDC->SelectObject( &pen_w );
		{
			if( m_last_drag_shape == DS_ARC_STRAIGHT )
				DrawArc( pDC, DL_LINE, m_drag_x, m_drag_y, m_drag_xi, m_drag_yi );
			else if( m_last_drag_shape == DS_ARC_CW )
				DrawArc( pDC, DL_ARC_CW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );
			else if( m_last_drag_shape == DS_ARC_CCW )
				DrawArc( pDC, DL_ARC_CCW, m_drag_xi, m_drag_yi, m_drag_x, m_drag_y );

			// draw new arc
			if( m_drag_shape == DS_ARC_STRAIGHT )
				DrawArc( pDC, DL_LINE, xx, yy, m_drag_xi, m_drag_yi );
			else if( m_drag_shape == DS_ARC_CW )
				DrawArc( pDC, DL_ARC_CW, m_drag_xi, m_drag_yi, xx, yy );
			else if( m_drag_shape == DS_ARC_CCW )
				DrawArc( pDC, DL_ARC_CCW, m_drag_xi, m_drag_yi, xx, yy );
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
		COLORREF bk = pDC->SetBkColor( C_RGB::black );
		CPen cross_pen( PS_DOT, 0, C_RGB::grey );
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
		CPen pen_old( PS_SOLID, 1, m_rgb[m_drag_layer_2] );
		CPen pen_old_w( PS_SOLID, m_drag_w1, m_rgb[m_drag_layer_1] );
		CPen pen( PS_SOLID, 1, m_rgb[layer2] );
		CPen pen_w( PS_SOLID, w/m_pcbu_per_wu, m_rgb[layer1] );

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
			CPen pen( PS_SOLID, thick, m_rgb[m_drag_layer_1] );
			CPen * old_pen = pDC->SelectObject( &pen );
			CBrush black_brush( C_RGB::black );
			CBrush * old_brush = pDC->SelectObject( &black_brush );
			pDC->Ellipse( m_drag_xi - w/2, m_drag_yi - w/2, m_drag_xi + w/2, m_drag_yi + w/2 );
			pDC->SelectObject( old_brush );
			pDC->SelectObject( old_pen );
			m_drag_via_drawn = 1 - m_drag_via_drawn;
		}
	}
	else if( m_drag_shape == DS_LINE )
	{
		CPen pen_old_w( PS_SOLID, m_drag_w1, m_rgb[m_drag_layer_1] );
		CPen pen( PS_SOLID, 1, m_rgb[layer2] );
		CPen pen_w( PS_SOLID, w/m_pcbu_per_wu, m_rgb[layer1] );

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
			CPen pen( PS_SOLID, thick, m_rgb[m_drag_layer_1] );
			CPen * old_pen = pDC->SelectObject( &pen );
			CBrush black_brush( C_RGB::black );
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

	CPen drag_pen( PS_SOLID, 1, m_rgb[m_drag_layer] );
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

	CPen drag_pen( PS_SOLID, 1, m_rgb[m_drag_layer] );
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
	m_drag_ratline_drag_pt.SetSize(0);
	m_drag_ratline_stat_pt.SetSize(0);

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

// CPoint pi is the stationary point, pf is the offset to the cursor for the dragged point
//
int CDisplayList::AddDragRatline( CPoint pi, CPoint pf )
{
	m_drag_ratline_drag_pt.Add( pi );
	m_drag_ratline_stat_pt.Add( pf );

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
int CDisplayList::Highlight( int gtype, int x, int y, int xf, int yf, int w, int orig_layer )
{
	id h_id;
	Add( h_id, NULL, LAY_HILITE, gtype, 1, w, 0, 0, x, y, xf, yf, x, y, 0, orig_layer );
	return 0;
}

int CDisplayList::CancelHighlight()
{
	RemoveAllFromLayer( LAY_HILITE );
	return 0;
}

// Set the device context and memory context to world coords
//
void CDisplayList::SetDCToWorldCoords( CDC * pDC, CDC * mDC, CDC * mDC2,			// CPT added mDC2 (experimental)
							int pcbu_org_x, int pcbu_org_y )
{
	memDC = memDC2 = NULL;
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
	// CPT experimental
	if( mDC2->m_hDC )
	{
		mDC2->SetMapMode( MM_ANISOTROPIC );
		mDC2->SetWindowExt( w_ext_x, w_ext_y );
		mDC2->SetWindowOrg( pcbu_org_x/m_pcbu_per_wu, pcbu_org_y/m_pcbu_per_wu );
		mDC2->SetViewportExt( v_ext_x, v_ext_y );
		mDC2->SetViewportOrg( m_pane_org_x, m_pane_org_y );
		memDC2 = mDC2;
	}
	// end CPT
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



void CDisplayLayer::Draw(CDrawInfo &di, bool bHiliteSegs) 
{
	CPen * old_pen;
	CBrush * old_brush;

	// Create drawing objects
	{
		di.erase_pen.CreatePen( PS_SOLID, 1, di.layer_color[0] );
		di.erase_brush.CreateSolidBrush( di.layer_color[0] );
		di.line_pen.CreatePen( PS_SOLID, 1, di.layer_color[1] );
		di.fill_brush.CreateSolidBrush( di.layer_color[1] );

		old_pen   = di.DC->SelectObject( &di.line_pen );
		old_brush = di.DC->SelectObject( &di.fill_brush );
	}

	if (bHiliteSegs)
		for (dl_element *el = elements; el; el = el->next)
			el->DrawHiliteSeg(di);
	else
		for (dl_element *el = elements; el; el = el->next)
			el->Draw(di);

	// Restore original drawing objects
	{
		di.DC->SelectObject( old_pen );
		di.DC->SelectObject( old_brush );

		di.erase_pen.DeleteObject();
		di.erase_brush.DeleteObject();
		di.line_pen.DeleteObject();
		di.fill_brush.DeleteObject();
	}
}


// CPT:  Brian had this function in class CDL_job.  I've moved it into CDisplayList.  (Also changed name from TestForHit to TestForHits.)
// Tests x,y for hits on items in the selection layer.
// Puts layer, id, and ptr info for each hit item into hitInfo.
// r294: altered args and made the relevant changes to the function body.
// CPT2 rewrote to set CHitInfo::item member.  TODO will dump the ID and ptr business.
int CDisplayList::TestForHits( double x, double y, CArray<CHitInfo> *hitInfo ) 
{
	double d;
	// traverse the list, looking for selection shapes
	for (dl_element *el = layers[LAY_SELECTION].elements; el; el = el->next) {
		if( el->IsHit(x, y, d) )
		{
			CHitInfo hit;
			hit.layer = el->layer;
			hit.item = el->item;						// CPT2.
			hit.ID = el->id;
			hit.ptr = el->ptr;
			hit.dist = d;								// CPT r294.  Introduced distance values into CHitInfo;  el->IsHit() now provides these.
			hitInfo->Add(hit);
		}
	}

	return hitInfo->GetCount();
}

