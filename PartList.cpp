// PartList.cpp : implementation of class CPartList
//
// this is a linked-list of parts on a PCB board
//
#include "stdafx.h"

#define PL_MAX_SIZE		5000		// default max. size 

extern Cuid pcb_cuid;

// globals
BOOL g_bShow_header_28mil_hole_warning = TRUE;	
BOOL g_bShow_SIP_28mil_hole_warning = TRUE;	

//********************* part_pin implementation ********************

part_pin::part_pin()
{
	m_uid = pcb_cuid.GetNewUID();
	m_part = NULL;
}

part_pin::~part_pin()
{
}

id part_pin::Id()
{
	id pp_id = m_part->m_id;
	pp_id.Set( id::NOP, id::NOP, ID_SEL_PAD, m_uid );
	pp_id.Resolve();	// get indices and pointer
	return pp_id;
}



//********************* cpart implementation *********************

cpart::cpart()
{
	m_pl = NULL;
	// zero out pointers
	dl_sel = 0;
	dl_ref_sel = 0;
	shape = 0;
	drawn = FALSE;
	m_ref_vis = TRUE;						// CPT:  is this the right place to be doing this?
	// set uids
	m_id.Clear();
	m_id.SetT1(ID_PART);
	m_id.SetU1( pcb_cuid.GetNewUID() );
}

 
cpart::cpart( CPartList * pl )
{
	m_pl = pl;
	// zero out pointers
	dl_sel = 0;
	dl_ref_sel = 0;
	shape = 0;
	drawn = FALSE;
	// set uids
	m_id.Clear();
	m_id.SetT1(ID_PART);
	m_id.SetU1( pcb_cuid.GetNewUID() );
}

cpart::~cpart()
{
}

part_pin * cpart::PinByUID( int uid, int * index )
{
	for( int ip=0; ip<pin.GetSize(); ip++ )
	{
		part_pin * p = &pin[ip];
		if( p->m_uid == uid )
		{
			if( index )
				*index = ip;
			return p;
		}
	}
	return NULL;
}

// return array sizes
//
int cpart::GetNumRefStrokes()
{ 
	return ref_text_stroke.GetSize(); 
}

int cpart::GetNumValueStrokes()
{ 
	return value_stroke.GetSize(); 
}

int cpart::GetNumOutlineStrokes()
{ 
	return m_outline_stroke.GetSize(); 
}


// CPT2.  A few of the changes that I introduced in cpart2 I'm also making in 
// get bounding rect of value text relative to part origin 
// works even if part isn't drawn
//
int GenerateStrokesForPartString( CString * str, int layer, BOOL bMirror,
								  int size, int rel_angle, int rel_xi, int rel_yi, int w, 
								  int x, int y, int angle, int side,
								  CArray<stroke> * strokes, CRect * br,
								  CDisplayList * dlist, SMFontUtil * sm );		// Below...

CRect cpart::GetValueRect()
{
	CArray<stroke> m_stroke;
	CRect br;
	BOOL bMirror = (m_value_layer == LAY_SILK_BOTTOM || m_value_layer == LAY_BOTTOM_COPPER) ;
	int nstrokes = GenerateStrokesForPartString( &value, 0, bMirror, m_value_size,
		m_value_angle, m_value_xi, m_value_yi, m_value_w,
		x, y, angle, side,
		&m_stroke, &br, NULL, m_pl->m_dlist->GetSMFontUtil() );
	return br;
}

void cpart::Move( int _x, int _y, int _angle, int _side )
{
	CPartList *pl = m_pl;
	// remove all display list elements
	pl->UndrawPart( this );
	// move part
	x = x;
	y = y;
	angle = angle % 360;
	side = side;
	// calculate new pin positions
	if( shape )
	{
		for( int ip=0; ip<pin.GetSize(); ip++ )
		{
			CPoint pt = pl->GetPinPoint( this, ip );
			pin[ip].x = pt.x;
			pin[ip].y = pt.y;
		}
	}
	// now redraw it
	pl->DrawPart( this );
}

void cpart::ResizeRefText(int size, int width, BOOL vis )
{
	CPartList *pl = m_pl;
	if( shape )
	{
		// remove all display list elements
		pl->UndrawPart( this );
		// change ref text size
		m_ref_size = size;
		m_ref_w = width;	
		m_ref_vis = vis;
		// now redraw part
		pl->DrawPart( this );
	}
}

void cpart::SetValue(CString *_value, int x, int y, int angle, int size, int w, 
						 BOOL vis, int layer )
{
	m_pl->SetValue(this, _value, x, y, angle, size, w, vis, layer);
}

void cpart::Draw()
{
	m_pl->DrawPart(this);
}


//******************** CPartList implementation *********************

CPartList::CPartList( CDisplayList * dlist ) 
{
	m_start.prev = 0;		// dummy first element in list
	m_start.next = &m_end;
	m_end.next = 0;			// dummy last element in list
	m_end.prev = &m_start;
	m_max_size = PL_MAX_SIZE;	// size limit
	m_size = 0;					// current size
	m_dlist = dlist;
	m_footprint_cache_map = NULL;
}

CPartList::~CPartList()
{
	// traverse list, removing all parts
	while( m_end.prev != &m_start )
		Remove( m_end.prev );
}

cpart * CPartList::GetPartByUID( int uid )
{
	for( cpart * part=GetFirstPart(); part; part=GetNextPart(part) )
	{
		if( part->m_id.U1()  == uid )
			return part;
	}
	return NULL;
}

// Create new empty part and add to end of list
// return pointer to element created.
//
cpart * CPartList::Add( int uid )
{
	if(m_size >= m_max_size )
	{
		CString s ((LPCSTR) IDS_MaximumNumberOfPartsExceeded);
		AfxMessageBox( s );
		return 0;
	}

	// create new instance and link into list
	cpart * part = new cpart( this );
	if( uid != -1 )
	{
		// set uid
		part->m_id.SetU1(uid);
	}
	else
		part->m_id.SetU1( pcb_cuid.GetNewUID() );

	part->prev = m_end.prev;
	part->next = &m_end;
	part->prev->next = part;
	part->next->prev = part;

	return part;
}

// Create new part, add to end of list, set part data 
// return pointer to element created.
//
cpart * CPartList::Add( CShape * shape, CString * ref_des, CString * package, 
							int x, int y, int side, int angle, int visible, int glued, bool ref_vis, int uid )
{
	if(m_size >= m_max_size )
	{
		CString s ((LPCSTR) IDS_MaximumNumberOfPartsExceeded);
		AfxMessageBox( s );
		return 0;
	}

	// create new instance and link into list
	cpart * part = Add( uid );
	// set data
	SetPartData( part, shape, ref_des, package, x, y, side, angle, visible, glued, ref_vis );
	return part;
}

// Set part data, draw part if m_dlist != NULL
//
int CPartList::SetPartData( cpart * part, CShape * shape, CString * ref_des, CString * package, 
							int x, int y, int side, int angle, int visible, int glued, bool ref_vis  )
{
#ifndef CPT2				// Hide incompatible stuff from the compiler
	UndrawPart( part );
	CDisplayList * old_dlist = m_dlist;
	m_dlist = NULL;		// cancel further drawing

	// now copy data into part
	part->visible = visible;
	part->ref_des = *ref_des;
	if( package )
		part->package = *package;
	else
		part->package = "";
	part->x = x;
	part->y = y;
	part->side = side;
	part->angle = angle;
	part->glued = glued;

	if( !shape )
	{
		part->shape = NULL;
		part->pin.SetSize(0);
		part->m_ref_xi = 0;
		part->m_ref_yi = 0;
		part->m_ref_angle = 0;
		part->m_ref_size = 0;
		part->m_ref_w = 0;
		part->m_value_xi = 0;
		part->m_value_yi = 0;
		part->m_value_angle = 0;
		part->m_value_size = 0;
		part->m_value_w = 0;
	}
	else
	{
		part->shape = shape;
		part->pin.SetSize( shape->m_padstack.GetSize() );
		Move( part, x, y, angle, side );	// force setting pin positions
		part->m_ref_xi = shape->m_ref_xi;
		part->m_ref_yi = shape->m_ref_yi;
		part->m_ref_angle = shape->m_ref_angle;
		part->m_ref_size = shape->m_ref_size;
		part->m_ref_w = shape->m_ref_w;
		part->m_ref_layer = shape->m_ref->m_layer;		
		part->m_value_xi = shape->m_value_xi;
		part->m_value_yi = shape->m_value_yi;
		part->m_value_angle = shape->m_value_angle;
		part->m_value_size = shape->m_value_size;
		part->m_value_w = shape->m_value_w;
		part->m_value_layer = shape->m_value->m_layer;		
	}
	part->m_outline_stroke.SetSize(0);
	part->ref_text_stroke.SetSize(0);
	part->value_stroke.SetSize(0);
	part->m_ref_vis = ref_vis;					// CPT
	m_size++;

	// now draw part into display list
	m_dlist = old_dlist;
	if( part->shape )
		DrawPart( part );

#endif
	return 0;
}

// convert footprint layer to part layer
//
int CPartList::FootprintLayer2Layer( int fp_layer )
{
	int layer;
	switch( fp_layer )
	{
	case LAY_FP_SILK_TOP: layer = LAY_SILK_TOP; break;
	case LAY_FP_SILK_BOTTOM: layer = LAY_SILK_BOTTOM; break;
	case LAY_FP_TOP_COPPER: layer = LAY_TOP_COPPER; break;
	case LAY_FP_BOTTOM_COPPER: layer = LAY_BOTTOM_COPPER; break;
	default: ASSERT(0); layer = -1; break;
	}
	return layer;
}

// Highlight part
//
int CPartList::HighlightPart( cpart * part )
{
	// highlight it by making its selection rectangle visible
	m_dlist->Highlight( DL_HOLLOW_RECT, 
				m_dlist->Get_x( part->dl_sel) , 
				m_dlist->Get_y( part->dl_sel),
				m_dlist->Get_xf(part->dl_sel), 
				m_dlist->Get_yf(part->dl_sel), 1 );
	return 0;
}

// Highlight part, value and ref text
//
int CPartList::SelectPart( cpart * part )
{
	// highlight part, value and ref text
	HighlightPart( part );
	SelectRefText( part );
	SelectValueText( part );
	return 0;
}

// Highlight part ref text
//
int CPartList::SelectRefText( cpart * part )
{
	// highlight it by making its selection rectangle visible
	if( part->dl_ref_sel )
	{
		m_dlist->Highlight( DL_HOLLOW_RECT, 
			m_dlist->Get_x(part->dl_ref_sel), 
			m_dlist->Get_y(part->dl_ref_sel),
			m_dlist->Get_xf(part->dl_ref_sel), 
			m_dlist->Get_yf(part->dl_ref_sel), 1 );
	}
	return 0;
}

// Highlight part value
//
int CPartList::SelectValueText( cpart * part )
{
	// highlight it by making its selection rectangle visible
	if( part->dl_value_sel )
	{
		m_dlist->Highlight( DL_HOLLOW_RECT, 
			m_dlist->Get_x(part->dl_value_sel), 
			m_dlist->Get_y(part->dl_value_sel),
			m_dlist->Get_xf(part->dl_value_sel), 
			m_dlist->Get_yf(part->dl_value_sel), 1 );
	}
	return 0;
}

void CPartList:: HighlightAllPadsOnNet( cnet * net )
{
	cpart * part = GetFirstPart();
	while( part )
	{
		if( part->shape )
		{
			for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
			{
				if( net == part->pin[ip].net )
					HighlightPad( part, ip );
			}
		}
		part = GetNextPart( part );
	}
}

// Select part pad
//
int CPartList::HighlightPad( cpart * part, int i )
{
	// select it by making its selection rectangle visible
	if( part->pin[i].dl_sel )
	{
		m_dlist->Highlight( DL_RECT_X, 
			m_dlist->Get_x(part->pin[i].dl_sel), 
			m_dlist->Get_y(part->pin[i].dl_sel),
			m_dlist->Get_xf(part->pin[i].dl_sel), 
			m_dlist->Get_yf(part->pin[i].dl_sel), 
			1, GetPinLayer( part, i ) );
	}
	return 0;
}

// Test for hit on pad
//
BOOL CPartList::TestHitOnPad( cpart * part, CString * pin_name, int x, int y, int layer )
{
#ifndef CPT2
	if( !part )
		return FALSE;
	if( !part->shape )
		return FALSE;
	int pin_index = part->shape->GetPinIndexByName( *pin_name );
	if( pin_index == -1 )
		return FALSE;

	int xx = part->pin[pin_index].x;
	int yy = part->pin[pin_index].y;
	cpadstack * ps = &part->shape->m_padstack[pin_index];
	pad * p;
	if( ps->hole_size == 0 )
	{
		// SMT pad
		if( layer == LAY_TOP_COPPER && part->side == 0 )
			p = &ps->top;
		else if( layer == LAY_BOTTOM_COPPER && part->side == 1 )
			p = &ps->top;
		else
			return FALSE;
	}
	else
	{
		// TH pad
		if( layer == LAY_TOP_COPPER && part->side == 0 )
			p = &ps->top;
		else if( layer == LAY_TOP_COPPER && part->side == 1 )
			p = &ps->bottom;
		else if( layer == LAY_BOTTOM_COPPER && part->side == 1 )
			p = &ps->top;
		else if( layer == LAY_BOTTOM_COPPER && part->side == 0 )
			p = &ps->bottom;
		else
			p = &ps->inner;
	}
	double dx = abs( xx-x );
	double dy = abs( yy-y );
	double dist = sqrt( dx*dx + dy*dy );
	if( dist < ps->hole_size/2 )
		return TRUE;
	switch( p->shape )
	{
	case PAD_NONE: 
		break;
	case PAD_ROUND: 
		if( dist < (p->size_h/2) ) 
			return TRUE; 
		break;
	case PAD_SQUARE:
		if( dx < (p->size_h/2) && dy < (p->size_h/2) )
			return TRUE;
		break;
	case PAD_RECT:
	case PAD_RRECT:
	case PAD_OVAL:
		int pad_angle = part->angle + ps->angle;
		if( pad_angle > 270 )
			pad_angle -= 360;
		if( pad_angle == 0 || pad_angle == 180 )
		{
			if( dx < (p->size_l) && dy < (p->size_h/2) )
				return TRUE;
		}
		else
		{
			if( dx < (p->size_h/2) && dy < (p->size_l) )
				return TRUE;
		}
		break;
	}
#endif
	return FALSE;
}


// Move part to new position, angle and side
// x and y are in world coords
// Does not adjust connections to pins
//
int CPartList::Move( cpart * part, int x, int y, int angle, int side )
{
	// remove all display list elements
	UndrawPart( part );
	// move part
	part->x = x;
	part->y = y;
	part->angle = angle % 360;
	part->side = side;
	// calculate new pin positions
	if( part->shape )
	{
		for( int ip=0; ip<part->pin.GetSize(); ip++ )
		{
			CPoint pt = GetPinPoint( part, ip );
			part->pin[ip].x = pt.x;
			part->pin[ip].y = pt.y;
		}
	}
	// now redraw it
	DrawPart( part );
	return PL_NOERR;
}

// Move ref text with given id to new position and angle
// x and y are in absolute world coords
// angle is relative to part angle
//
int CPartList::MoveRefText( cpart * part, int x, int y, int angle, int size, int w )
{
	// remove all display list elements
	UndrawPart( part );
	
	// get position of new text box origin relative to part
	CPoint part_org, tb_org;
	tb_org.x = x - part->x;
	tb_org.y = y - part->y;
	
	// correct for rotation of part
	RotatePoint( &tb_org, 360-part->angle, zero );
	
	// correct for part on bottom of board (reverse relative x-axis)
	if( part->side == 1 )
		tb_org.x = -tb_org.x;
	
	// reset ref text parameters
	part->m_ref_xi = tb_org.x;
	part->m_ref_yi = tb_org.y;
	part->m_ref_angle = angle % 360;
	part->m_ref_size = size;
	part->m_ref_w = w;
	
	// now redraw part
	DrawPart( part );
	return PL_NOERR;
}

// Resize ref text for part
//
void CPartList::ResizeRefText( cpart * part, int size, int width, BOOL vis )
{
	if( part->shape )
	{
		// remove all display list elements
		UndrawPart( part );
		// change ref text size
		part->m_ref_size = size;
		part->m_ref_w = width;	
		part->m_ref_vis = vis;
		// now redraw part
		DrawPart( part );
	}
}

// Move value text to new position and angle
// x and y are in absolute world coords
// angle is relative to part angle
//
int CPartList::MoveValueText( cpart * part, int x, int y, int angle, int size, int w )
{
	// remove all display list elements
	UndrawPart( part );
	
	// get position of new text box origin relative to part
	CPoint part_org, tb_org;
	tb_org.x = x - part->x;
	tb_org.y = y - part->y;
	
	// correct for rotation of part
	RotatePoint( &tb_org, 360-part->angle, zero );
	
	// correct for part on bottom of board (reverse relative x-axis)
	if( part->side == 1 )
		tb_org.x = -tb_org.x;
	
	// reset value text position
	part->m_value_xi = tb_org.x;
	part->m_value_yi = tb_org.y;
	part->m_value_angle = angle % 360;
	part->m_value_size = size;
	part->m_value_w = w;
	
	// now redraw part
	DrawPart( part );
	return PL_NOERR;
}

// Resize value text for part
//
void CPartList::ResizeValueText( cpart * part, int size, int width, BOOL vis )
{
	if( part->shape )
	{
		// remove all display list elements
		UndrawPart( part );
		// change ref text size
		part->m_value_size = size;
		part->m_value_w = width;
		part->m_value_vis = vis;
		// now redraw part
		DrawPart( part );
	}
}

// Set part value
//
void CPartList::SetValue( cpart * part, CString * value, 
						 int x, int y, int angle, int size, int w, 
						 BOOL vis, int layer )
{
	part->value = *value;
	part->m_value_xi = x;
	part->m_value_yi = y;
	part->m_value_angle = angle;
	part->m_value_size = size;
	part->m_value_w = w;
	part->m_value_vis = vis;
	part->m_value_layer = layer;
	if( part->shape && m_dlist )
	{
		UndrawPart( part );
		DrawPart( part );
	}
}


// Get side of part
//
int CPartList::GetSide( cpart * part )
{
	return part->side;
}

// Get angle of part
//
int CPartList::GetAngle( cpart * part )
{
	return part->angle;
}

// Get angle of ref text for part
//
int CPartList::GetRefAngle( cpart * part )
{
	return part->m_ref_angle;
}

// Get angle of value for part
//
int CPartList::GetValueAngle( cpart * part )
{
	return part->m_value_angle;
}

// get actual position of ref text
//
CPoint CPartList::GetRefPoint( cpart * part )
{
	CPoint ref_pt;

	// move origin of text box to position relative to part
	ref_pt.x = part->m_ref_xi;
	ref_pt.y = part->m_ref_yi;
	// flip if part on bottom
	if( part->side )
	{
		ref_pt.x = -ref_pt.x;
	}
	// rotate with part about part origin
	RotatePoint( &ref_pt, part->angle, zero );
	ref_pt.x += part->x;
	ref_pt.y += part->y;
	return ref_pt;
}

// get actual position of value text
//
CPoint CPartList::GetValuePoint( cpart * part )
{
	CPoint value_pt;

	// move origin of text box to position relative to part
	value_pt.x = part->m_value_xi;
	value_pt.y = part->m_value_yi;
	// flip if part on bottom
	if( part->side )
	{
		value_pt.x = -value_pt.x;
	}
	// rotate with part about part origin
	RotatePoint( &value_pt, part->angle, zero );
	value_pt.x += part->x;
	value_pt.y += part->y;
	return value_pt;
}

// Get pin info from part
//
CPoint CPartList::GetPinPoint( cpart * part, LPCTSTR pin_name )
{
#ifndef CPT2
	// get pin coords relative to part origin
	CPoint pp;
	int pin_index = part->shape->GetPinIndexByName( pin_name );
	if( pin_index == -1 )
		ASSERT(0);
	return GetPinPoint( part, pin_index );
#else
	return CPoint(0,0);
#endif
}

// Get pin info from part
//
CPoint CPartList::GetPinPoint( cpart * part, int pin_index )
{
#ifndef CPT2
	if( !part->shape )
		ASSERT(0);

	// get pin coords relative to part origin
	CPoint pp;
	if( pin_index == -1 )
		ASSERT(0);
	pp.x = part->shape->m_padstack[pin_index].x_rel;
	pp.y = part->shape->m_padstack[pin_index].y_rel;
	// flip if part on bottom
	if( part->side )
	{
		pp.x = -pp.x;
	}
	// rotate if necess.
	int angle = part->angle;
	if( angle > 0 )
	{
		CPoint org;
		org.x = 0;
		org.y = 0;
		RotatePoint( &pp, angle, org );
	}
	// add coords of part origin
	pp.x = part->x + pp.x;
	pp.y = part->y + pp.y;
	return pp;
#else
	CPoint pp;
	return pp;
#endif
}

// Get centroid info from part
//
CPoint CPartList::GetCentroidPoint( cpart * part )
{
	if( part->shape == NULL )
		ASSERT(0);
	// get coords relative to part origin
	CPoint pp;
	pp.x = part->shape->m_centroid->m_x;
	pp.y = part->shape->m_centroid->m_y;
	// flip if part on bottom
	if( part->side )
	{
		pp.x = -pp.x;
	}
	// rotate if necess.
	int angle = part->angle;
	if( angle > 0 )
	{
		CPoint org;
		org.x = 0;
		org.y = 0;
		RotatePoint( &pp, angle, org );
	}
	// add coords of part origin
	pp.x = part->x + pp.x;
	pp.y = part->y + pp.y;
	return pp;
}

// Get glue spot info from part
//
CPoint CPartList::GetGluePoint( cpart * part, int iglue )
{
#ifndef CPT2
	if( part->shape == NULL )
		ASSERT(0);
	if( iglue >= part->shape->m_glue.GetSize() )
		ASSERT(0);
	// get coords relative to part origin
	CPoint pp;
	pp.x = part->shape->m_glue[iglue].x_rel;
	pp.y = part->shape->m_glue[iglue].x_rel;
	// flip if part on bottom
	if( part->side )
	{
		pp.x = -pp.x;
	}
	// rotate if necess.
	int angle = part->angle;
	if( angle > 0 )
	{
		CPoint org;
		org.x = 0;
		org.y = 0;
		RotatePoint( &pp, angle, org );
	}
	// add coords of part origin
	pp.x = part->x + pp.x;
	pp.y = part->y + pp.y;
	return pp;
#else
	return NULL;
#endif
}

// Get pin layer
// returns LAY_TOP_COPPER, LAY_BOTTOM_COPPER or LAY_PAD_THRU
//
int CPartList::GetPinLayer( cpart * part, CString * pin_name )
{
#ifndef CPT2
	int pin_index = part->shape->GetPinIndexByName( *pin_name );
	return GetPinLayer( part, pin_index );
#else
	return 0;
#endif
}

// Get pin layer
// returns LAY_TOP_COPPER, LAY_BOTTOM_COPPER or LAY_PAD_THRU
//
int CPartList::GetPinLayer( cpart * part, int pin_index )
{
#ifndef CPT2
	if( part->shape->m_padstack[pin_index].hole_size )
		return LAY_PAD_THRU;
	else if( part->side == 0 && part->shape->m_padstack[pin_index].top.shape != PAD_NONE 
		|| part->side == 1 && part->shape->m_padstack[pin_index].bottom.shape != PAD_NONE )
		return LAY_TOP_COPPER;
	else
		return LAY_BOTTOM_COPPER;
#else
	return 0;
#endif
}

// Get pin net
//
cnet * CPartList::GetPinNet( cpart * part, CString * pin_name )
{
#ifndef CPT2
	int pin_index = part->shape->GetPinIndexByName( *pin_name );
	if( pin_index == -1 )
		return NULL;
	return part->pin[pin_index].net;
#else
	return NULL;
#endif
}

// Get pin net
//
cnet * CPartList::GetPinNet( cpart * part, int pin_index )
{
	return part->pin[pin_index].net;
}

// Get max pin width, for drawing thermal symbol
// enter with pin_num = pin # (1-based)
//
int CPartList::GetPinWidth( cpart * part, CString * pin_name )
{
#ifndef CPT2
	if( !part->shape )
		ASSERT(0);
	int pin_index = part->shape->GetPinIndexByName( *pin_name );
	int w = part->shape->m_padstack[pin_index].top.size_h;
	w = max( w, part->shape->m_padstack[pin_index].bottom.size_h );
	w = max( w, part->shape->m_padstack[pin_index].hole_size );
	return w;
#else
	return 0;
#endif
}

// Get bounding rect for all pins
// Currently, just uses selection rect
// returns 1 if successful
//
int CPartList::GetPartBoundingRect( cpart * part, CRect * part_r )
{
	CRect r;

	if( !part )
		return 0;
	if( !part->shape )
		return 0;
	if( part->dl_sel )
	{
		r.left = min( m_dlist->Get_x( part->dl_sel ), m_dlist->Get_xf( part->dl_sel ) );
		r.right = max( m_dlist->Get_x( part->dl_sel ), m_dlist->Get_xf( part->dl_sel ) );
		r.bottom = min( m_dlist->Get_y( part->dl_sel ), m_dlist->Get_yf( part->dl_sel ) );
		r.top = max( m_dlist->Get_y( part->dl_sel ), m_dlist->Get_yf( part->dl_sel ) );
		*part_r = r;
		return 1;
	}
	return 0;
}

// get bounding rectangle of parts
// return 0 if no parts found, else return 1
//
int CPartList::GetPartBoundaries( CRect * part_r )
{
	int min_x = INT_MAX;
	int max_x = INT_MIN;
	int min_y = INT_MAX;
	int max_y = INT_MIN;
	int parts_found = 0;
	// iterate
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		if( part->dl_sel )
		{
			int x = m_dlist->Get_x( part->dl_sel );
			int y = m_dlist->Get_y( part->dl_sel );
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			x = m_dlist->Get_xf( part->dl_sel );
			y = m_dlist->Get_yf( part->dl_sel );
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			parts_found = 1;
		}
		if( part->dl_ref_sel )
		{
			int x = m_dlist->Get_x( part->dl_ref_sel );
			int y = m_dlist->Get_y( part->dl_ref_sel );
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			x = m_dlist->Get_xf( part->dl_ref_sel );
			y = m_dlist->Get_yf( part->dl_ref_sel );
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			parts_found = 1;
		}
		part = part->next;
	}
	part_r->left = min_x;
	part_r->right = max_x;
	part_r->bottom = min_y;
	part_r->top = max_y;
	return parts_found;
}

// Get pointer to part in part_list with given ref
//
cpart * CPartList::GetPartByName( LPCTSTR ref_des )
{
	// find element with given ref_des, return pointer to element
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		if(  part->ref_des == ref_des  )
			return part;
		part = part->next;
	}
	return NULL;	// if unable to find part
}

// Iterate through parts
//
cpart * CPartList::GetFirstPart()
{
	cpart * p = m_start.next;
	if( p->next )
		return p;
	else
		return NULL;
}

cpart * CPartList::GetNextPart( cpart * part )
{
	cpart * p = part->next;
	if( !p )
		return NULL;
	if( !p->next )
		return NULL;
	else
		return p;
}

// get number of times a particular shape is used
//
int CPartList::GetNumFootprintInstances( CShape * shape )
{
	int n = 0;

	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		if(  part->shape == shape  )
			n++;
		part = part->next;
	}
	return n;
}

// Purge unused footprints from cache
//
void CPartList::PurgeFootprintCache()
{
	POSITION pos;
	CString key;
	void * ptr;

	if( !m_footprint_cache_map )
		ASSERT(0);

	for( pos = m_footprint_cache_map->GetStartPosition(); pos != NULL; )
	{
		m_footprint_cache_map->GetNextAssoc( pos, key, ptr );
		CShape * shape = (CShape*)ptr;
		if( GetNumFootprintInstances( shape ) == 0 )
		{
			// purge this footprint
			delete shape;
			m_footprint_cache_map->RemoveKey( key );
		}
	}
}

// Remove part from list and delete it
//
int CPartList::Remove( cpart * part )
{
	// delete all entries in display list
	UndrawPart( part );

	// remove links to this element
	part->next->prev = part->prev;
	part->prev->next = part->next;
	// destroy part
	m_size--;
	delete( part );

	return 0;
}

// Remove all parts from list
//
void CPartList::RemoveAllParts()
{
	// traverse list, removing all parts
	while( m_end.prev != &m_start )
		Remove( m_end.prev );
}

// Set utility flag for all parts
//
void CPartList::MarkAllParts( int mark )
{
	cpart * part = GetFirstPart();
	while( part )
	{
		part->utility = mark;
		part = GetNextPart( part );
	}
}

// generate an array of strokes for a string that is attached to a part
// enter with:
//  str = pointer to text string
//	bMirror = TRUE to draw mirror-image if part is on top
//	size = height of characters
//	w = stroke width
//	rel_angle = angle of string relative to part
//	rel_x, rel_y = position of string relative to part
//	angle = angle of part
//	x, y = postion of part
//	side = side of PCB that part is on
//	strokes = pointer to CArray of strokes to receive data
//	br = pointer to CRect to receive bounding rectangle
//	dlist = pointer to display list to use for drawing (not used)
//	sm = pointer to SMFontUtil for character data	
// returns number of strokes generated
//
int GenerateStrokesForPartString( CString * str, int layer, BOOL bMirror,
								  int size, int rel_angle, int rel_xi, int rel_yi, int w, 
								  int x, int y, int angle, int side,
								  CArray<stroke> * strokes, CRect * br,
								  CDisplayList * dlist, SMFontUtil * sm )
{
	strokes->SetSize( 10000 );
	double x_scale = (double)size/22.0;
	double y_scale = (double)size/22.0;
	double y_offset = 9.0*y_scale;
	int i = 0;
	double xc = 0.0;
	CPoint si, sf;
	int xmin = INT_MAX;
	int xmax = INT_MIN;
	int ymin = INT_MAX;
	int ymax = INT_MIN;
	int nchars = str->GetLength();
	for( int ic=0; ic<nchars; ic++ )
	{
		// get stroke info for character
		int xi, yi, xf, yf;
		double coord[64][4];
		double min_x, min_y, max_x, max_y;
		int nstrokes;
		if( !bMirror )
		{
			nstrokes = sm->GetCharStrokes( str->GetAt(ic), SIMPLEX, 
				&min_x, &min_y, &max_x, &max_y, coord, 64 );
		}
		else
		{
			nstrokes = sm->GetCharStrokes( str->GetAt(nchars-ic-1), SIMPLEX, 
				&min_x, &min_y, &max_x, &max_y, coord, 64 );
		}
		for( int is=0; is<nstrokes; is++ )
		{
			if( !bMirror )
			{
				xi = (coord[is][0] - min_x)*x_scale + xc;
				yi = coord[is][1]*y_scale + y_offset;
				xf = (coord[is][2] - min_x)*x_scale + xc;
				yf = coord[is][3]*y_scale + y_offset;
			}
			else
			{
				xi = (max_x - coord[is][0])*x_scale + xc;
				yi = coord[is][1]*y_scale + y_offset;
				xf = (max_x - coord[is][2])*x_scale + xc;
				yf = coord[is][3]*y_scale + y_offset;
			}
			// get stroke relative to text box
			if( yi > yf )
			{
				si.x = xi;
				sf.x = xf;
				si.y = yi;
				sf.y = yf;
			}
			else
			{
				si.x = xf;
				sf.x = xi;
				si.y = yf;
				sf.y = yi;
			}
			// rotate about text box origin
			RotatePoint( &si, rel_angle, zero );
			RotatePoint( &sf, rel_angle, zero );
			// move origin of text box to position relative to part
			si.x += rel_xi;
			sf.x += rel_xi;
			si.y += rel_yi;
			sf.y += rel_yi;
			// flip if part on bottom
			if( side )
			{
				si.x = -si.x;
				sf.x = -sf.x;
			}
			// rotate with part about part origin
			RotatePoint( &si, angle, zero );
			RotatePoint( &sf, angle, zero );

			// add x, y to part origin and add stroke to array
			stroke t2 = strokes->GetAt(i);
			t2.layer = layer;
			t2.type = DL_LINE;
			t2.w = w;
			t2.xi = x + si.x;
			t2.yi = y + si.y;
			t2.xf = x + sf.x;
			t2.yf = y + sf.y;
			strokes->SetAt( i, t2 );
			i++;
			if( strokes->GetSize() <= i )
				strokes->SetSize( i + 100 );
			xmax = max( t2.xi, xmax );
			xmax = max( t2.xf, xmax );
			xmin = min( t2.xi, xmin );
			xmin = min( t2.xf, xmin );
			ymax = max( t2.yi, ymax );
			ymax = max( t2.yf, ymax );
			ymin = min( t2.yi, ymin );
			ymin = min( t2.yf, ymin );
		}
		xc += (max_x - min_x + 8.0)*x_scale;
	}
	strokes->SetSize(i);
	br->left = xmin - w/2;
	br->right = xmax + w/2;
	br->bottom = ymin - w/2;
	br->top = ymax + w/2;
	return i;
}

// return the actual PCB layer for part value string
int CPartList::GetValuePCBLayer( cpart * part )
{
	return FlipLayer( part->side, part->m_value_layer );
}

// return the actual PCB layer for part reference string
int CPartList::GetRefPCBLayer( cpart * part )
{
	return FlipLayer( part->side, part->m_ref_layer );
}


// Draw part into display list
//
int CPartList::DrawPart( cpart * part )
{
#ifndef CPT2
	int i;

	if( !m_dlist )
		return PL_NO_DLIST;
	if( !part->shape )
		return PL_NO_FOOTPRINT;
	if( part->drawn )
		UndrawPart( part );		// ideally, should be undrawn when changes made, not now

	// this part
	CShape * shape = part->shape;
	int x = part->x;
	int y = part->y;
	int angle = part->angle;
	id id = part->m_id;

	// draw selection rectangle (layer = top or bottom copper, depending on side)
	CRect sel;
	int sel_layer;
	if( !part->side )
	{
		// part on top
		sel.left = shape->m_sel_xi; 
		sel.right = shape->m_sel_xf;
		sel.bottom = shape->m_sel_yi;
		sel.top = shape->m_sel_yf;
		sel_layer = LAY_SELECTION;
	}
	else
	{
		// part on bottom
		sel.right = - shape->m_sel_xi;
		sel.left = - shape->m_sel_xf;
		sel.bottom = shape->m_sel_yi;
		sel.top = shape->m_sel_yf;
		sel_layer = LAY_SELECTION;
	}
	if( angle > 0 )
		RotateRect( &sel, angle, zero );
	id.SetT2( ID_SEL_RECT );
	part->dl_sel = m_dlist->AddSelector( id, part, sel_layer, DL_HOLLOW_RECT, 1,
		0, 0, x + sel.left, y + sel.bottom, x + sel.right, y + sel.top, x, y );
	m_dlist->Set_sel_vert( part->dl_sel, 0 );
	if( angle == 90 || angle ==  270 )
		m_dlist->Set_sel_vert( part->dl_sel, 1 );

	CArray<stroke> m_stroke;	// used for text
	CRect br;
	CPoint si, sf;

	// draw ref designator text
	part->dl_ref_sel = NULL;
	if( part->m_ref_vis && part->m_ref_size )
	{
		int ref_layer = part->m_ref_layer;
		ref_layer = FlipLayer( part->side, ref_layer );
		BOOL bMirror = (part->m_ref_layer == LAY_SILK_BOTTOM || part->m_ref_layer == LAY_BOTTOM_COPPER) ;
		int nstrokes = ::GenerateStrokesForPartString( &part->ref_des, 
			ref_layer, bMirror, part->m_ref_size,
			part->m_ref_angle, part->m_ref_xi, part->m_ref_yi, part->m_ref_w,
			part->x, part->y, part->angle, part->side,
			&m_stroke, &br, m_dlist, m_dlist->GetSMFontUtil() );
		if( nstrokes )
		{
			int xmin = br.left;
			int xmax = br.right;
			int ymin = br.bottom;
			int ymax = br.top;
			id.SetT1( ID_PART );
			id.SetT2( ID_REF_TXT );
			id.SetT3( ID_STROKE );
			part->ref_text_stroke.SetSize( nstrokes );
			for( int is=0; is<nstrokes; is++ )
			{
				id.SetI3( is );
				m_stroke[is].dl_el = m_dlist->Add( id, this, 
					ref_layer, DL_LINE, 1, m_stroke[is].w, 0, 0,
					m_stroke[is].xi, m_stroke[is].yi, 
					m_stroke[is].xf, m_stroke[is].yf, 0, 0 );
				part->ref_text_stroke[is] = m_stroke[is];
			}
			// draw selection rectangle for ref text
			id.SetT3( ID_SEL_REF_TXT );
			part->dl_ref_sel = m_dlist->AddSelector( id, part, part->m_ref_layer, DL_HOLLOW_RECT, 1,
				0, 0, xmin, ymin, xmax, ymax, xmin, ymin );
		}
	}
	else
	{
		for( int is=0; is<part->ref_text_stroke.GetSize(); is++ )
			m_dlist->Remove( part->ref_text_stroke[is].dl_el );
		part->ref_text_stroke.SetSize(0);
	}

	// draw value text
	part->dl_value_sel = NULL;
	if( part->m_value_vis && part->m_value_size )
	{
		int value_layer = part->m_value_layer;
		value_layer = FlipLayer( part->side, value_layer );
		BOOL bMirror = (part->m_value_layer == LAY_SILK_BOTTOM || part->m_value_layer == LAY_BOTTOM_COPPER) ;
		int nstrokes = ::GenerateStrokesForPartString( &part->value, 
			value_layer, bMirror, part->m_value_size,
			part->m_value_angle, part->m_value_xi, part->m_value_yi, part->m_value_w,
			part->x, part->y, part->angle, part->side,
			&m_stroke, &br, m_dlist, m_dlist->GetSMFontUtil() );

		if( nstrokes )
		{
			int xmin = br.left;
			int xmax = br.right;
			int ymin = br.bottom;
			int ymax = br.top;
			id.SetT1( ID_PART );
			id.SetT2( ID_VALUE_TXT );
			id.SetT3( ID_STROKE );
			part->value_stroke.SetSize( nstrokes );
			for( int is=0; is<nstrokes; is++ )
			{
				id.SetI3( is );
				m_stroke[is].dl_el = m_dlist->Add( id, this, 
					m_stroke[is].layer, DL_LINE, 1, m_stroke[is].w, 0, 0,
					m_stroke[is].xi, m_stroke[is].yi, 
					m_stroke[is].xf, m_stroke[is].yf, 0, 0 );
				part->value_stroke[is] = m_stroke[is];
			}

			// draw selection rectangle for value
			id.SetT3( ID_SEL_VALUE_TXT );
			part->dl_value_sel = m_dlist->AddSelector( id, part, part->m_value_layer, DL_HOLLOW_RECT, 1,
				0, 0, xmin, ymin, xmax, ymax, xmin, ymin );
		}
	}
	else
	{
		for( int is=0; is<part->value_stroke.GetSize(); is++ )
			m_dlist->Remove( part->value_stroke[is].dl_el );
		part->value_stroke.SetSize(0);
	}

	// draw part outline
	part->m_outline_stroke.SetSize(0);
	for( int ip=0; ip<shape->m_outline_poly.GetSize(); ip++ )
	{
		CPolyLine * poly = &shape->m_outline_poly[ip];
		int shape_layer = poly->Layer();
		int poly_layer = FootprintLayer2Layer( shape_layer );
		poly_layer = FlipLayer( part->side, poly_layer );
		int pos = part->m_outline_stroke.GetSize();
		int nsides;
		if( poly->Closed() )
			nsides = poly->NumCorners();
		else
			nsides = poly->NumCorners() - 1;
		part->m_outline_stroke.SetSize( pos + nsides );
		int w = poly->W();
		for( i=0; i<nsides; i++ )
		{
			int g_type;
			if( poly->SideStyle( i ) == CPolyLine::STRAIGHT )
				g_type = DL_LINE;
			else if( poly->SideStyle( i ) == CPolyLine::ARC_CW )
				g_type = DL_ARC_CW;
			else if( poly->SideStyle( i ) == CPolyLine::ARC_CCW )
				g_type = DL_ARC_CCW;
			si.x = poly->X( i );
			si.y = poly->Y( i );
			if( i == (nsides-1) && shape->m_outline_poly[ip].Closed() )
			{
				sf.x = poly->X( 0 );
				sf.y = poly->Y( 0 );
			}
			else
			{
				sf.x = poly->X( i+1 );
				sf.y = poly->Y( i+1 );
			}
			// flip if part on bottom
			if( part->side )
			{
				si.x = -si.x;
				sf.x = -sf.x;
				if( g_type == DL_ARC_CW )
					g_type = DL_ARC_CCW;
				else if( g_type == DL_ARC_CCW )
					g_type = DL_ARC_CW;
			}
			// rotate with part and draw
			RotatePoint( &si, angle, zero );
			RotatePoint( &sf, angle, zero );
			part->m_outline_stroke[i+pos].layer = poly_layer;
			part->m_outline_stroke[i+pos].xi = x+si.x;
			part->m_outline_stroke[i+pos].xf = x+sf.x;
			part->m_outline_stroke[i+pos].yi = y+si.y;
			part->m_outline_stroke[i+pos].yf = y+sf.y;
			part->m_outline_stroke[i+pos].type = g_type;
			part->m_outline_stroke[i+pos].w = w;
			part->m_outline_stroke[i+pos].dl_el = m_dlist->Add( part->m_id, part, poly_layer, 
				g_type, 1, w, 0, 0, x+si.x, y+si.y, x+sf.x, y+sf.y, 0, 0 );
		}
	}

	// draw text
	for( int it=0; it<part->shape->m_tl->text_ptr.GetSize(); it++ )
	{
		CText * t = part->shape->m_tl->text_ptr[it];
		int nstrokes = 0;
		CArray<stroke> m_stroke;
		m_stroke.SetSize( 1000 );

		double x_scale = (double)t->m_font_size/22.0;
		double y_scale = (double)t->m_font_size/22.0;
		double y_offset = 9.0*y_scale;
		i = 0;
		double xc = 0.0;
		CPoint si, sf;
		int w = t->m_stroke_width;
		int xmin = INT_MAX;
		int xmax = INT_MIN;
		int ymin = INT_MAX;
		int ymax = INT_MIN;

		int text_layer = FootprintLayer2Layer( t->m_layer );
		text_layer = FlipLayer( part->side, text_layer );
		nstrokes = ::GenerateStrokesForPartString( &t->m_str, 
			text_layer, t->m_mirror, t->m_font_size,
			t->m_angle, t->m_x, t->m_y, t->m_stroke_width,
			part->x, part->y, part->angle, part->side,
			&m_stroke, &br, m_dlist, m_dlist->GetSMFontUtil() );

		xmin = min( xmin, br.left );
		xmax = max( xmax, br.right );
		ymin = min( ymin, br.bottom );
		ymax = max( ymax, br.top );
		id.SetT1( ID_PART );
		id.SetT2( ID_FP_TXT );
		id.SetI2( it );
		id.SetT3( ID_STROKE );
		for( int is=0; is<nstrokes; is++ )
		{
			id.SetI3( is );
			m_stroke[is].dl_el = m_dlist->Add( id, this, 
				text_layer, DL_LINE, 1, m_stroke[is].w, 0, 0,
				m_stroke[is].xi, m_stroke[is].yi, 
				m_stroke[is].xf, m_stroke[is].yf, 0, 0 );
			part->m_outline_stroke.Add( m_stroke[is] );
		}


	}

	// draw padstacks and save absolute position of pins
	CPoint pin_pt;
	CPoint pad_pi;
	CPoint pad_pf;
	for( i=0; i<shape->GetNumPins(); i++ ) 
	{
		// set layer for pads
		cpadstack * ps = &shape->m_padstack[i];
		part_pin * pin = &part->pin[i];
		pin->dl_els.SetSize(m_layers);
		pad * p;
		int pad_layer;
		// iterate through all copper layers 
		pad * any_pad = NULL;
		for( int il=0; il<m_layers; il++ )
		{
			pin_pt.x = ps->x_rel;
			pin_pt.y = ps->y_rel;
			pad_layer = il + LAY_TOP_COPPER;
			pin->dl_els[il] = NULL;
			// get appropriate pad
			cpadstack * ps = &shape->m_padstack[i];
			pad * p = NULL;
			if( pad_layer == LAY_TOP_COPPER && part->side == 0 )
				p = &ps->top;
			else if( pad_layer == LAY_TOP_COPPER && part->side == 1 )
				p = &ps->bottom;
			else if( pad_layer == LAY_BOTTOM_COPPER && part->side == 0 )
				p = &ps->bottom;
			else if( pad_layer == LAY_BOTTOM_COPPER && part->side == 1 )
				p = &ps->top;
			else if( ps->hole_size )
				p = &ps->inner;
			int sel_layer = pad_layer;
			if( ps->hole_size )
				sel_layer = LAY_SELECTION;
			if( p )
			{
				if( p->shape != PAD_NONE )
					any_pad = p;

				// draw pad
				dl_element * pad_el = NULL;
				if( p->shape == PAD_NONE )
				{
				}
				else if( p->shape == PAD_ROUND )
				{
					// flip if part on bottom
					if( part->side )
						pin_pt.x = -pin_pt.x;
					// rotate
					if( angle > 0 )
						RotatePoint( &pin_pt, angle, zero );
					// add to display list
					id.SetT2( ID_PAD );
					id.SetI2( i );
					id.SetU2( pin->m_uid );
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = m_dlist->Add( id, part, pad_layer, 
						DL_CIRC, 1, 
						p->size_h,
						0, 0,
						x + pin_pt.x, y + pin_pt.y, 0, 0, 0, 0 );
					if( !pin->dl_sel )
					{
						id.SetT2( ID_SEL_PAD );
						pin->dl_sel = m_dlist->AddSelector( id, part, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							pin->x-p->size_h/2,  
							pin->y-p->size_h/2, 
							pin->x+p->size_h/2, 
							pin->y+p->size_h/2, 0, 0 );
					}
				}
				else if( p->shape == PAD_SQUARE )
				{
					// flip if part on bottom
					if( part->side )
					{
						pin_pt.x = -pin_pt.x;
					}
					// rotate
					if( angle > 0 )
						RotatePoint( &pin_pt, angle, zero );
					id.SetT2( ID_PAD );
					id.SetI2( i );
					id.SetU2( pin->m_uid );
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = m_dlist->Add( part->m_id, part, pad_layer, 
						DL_SQUARE, 1, 
						p->size_h,
						0, 0,
						pin->x, pin->y, 
						0, 0, 
						0, 0 );
					if( !pin->dl_sel )
					{
						id.SetT2( ID_SEL_PAD );
						pin->dl_sel = m_dlist->AddSelector( id, part, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							pin->x-p->size_h/2,  
							pin->y-p->size_h/2, 
							pin->x+p->size_h/2, 
							pin->y+p->size_h/2, 0, 0 );
					}
				}
				else if( p->shape == PAD_RECT 
					|| p->shape == PAD_RRECT 
					|| p->shape == PAD_OVAL )
				{
					int gtype;
					if( p->shape == PAD_RECT )
						gtype = DL_RECT;
					else if( p->shape == PAD_RRECT )
						gtype = DL_RRECT;
					else
						gtype = DL_OVAL;
					pad_pi.x = pin_pt.x - p->size_l;
					pad_pi.y = pin_pt.y - p->size_h/2;
					pad_pf.x = pin_pt.x + p->size_r;
					pad_pf.y = pin_pt.y + p->size_h/2;
					// rotate pad about pin if necessary
					if( shape->m_padstack[i].angle > 0 )
					{
						RotatePoint( &pad_pi, ps->angle, pin_pt );
						RotatePoint( &pad_pf, ps->angle, pin_pt );
					}

					// flip if part on bottom
					if( part->side )
					{
						pin_pt.x = -pin_pt.x;
						pad_pi.x = -pad_pi.x;
						pad_pf.x = -pad_pf.x;
					}
					// rotate part about 
					if( angle > 0 )
					{
						RotatePoint( &pin_pt, angle, zero );
						RotatePoint( &pad_pi, angle, zero );
						RotatePoint( &pad_pf, angle, zero );
					}
					id.SetT2( ID_PAD );
					id.SetI2( i );
					id.SetU2( pin->m_uid );
					int radius = p->radius;
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = m_dlist->Add( part->m_id, part, pad_layer, 
						gtype, 1, 
						0,
						0, 0, 
						x + pad_pi.x, y + pad_pi.y, 
						x + pad_pf.x, y + pad_pf.y, 
						x + pin_pt.x, y + pin_pt.y, 
						p->radius );
					if( !pin->dl_sel )
					{
						id.SetT2( ID_SEL_PAD );
						pin->dl_sel = m_dlist->AddSelector( id, part, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							x + pad_pi.x, y + pad_pi.y, 
							x + pad_pf.x, y + pad_pf.y,
							0, 0 );
					}
				}
				else if( p->shape == PAD_OCTAGON )
				{
					// flip if part on bottom
					if( part->side )
					{
						pin_pt.x = -pin_pt.x;
					}
					// rotate
					if( angle > 0 )
						RotatePoint( &pin_pt, angle, zero );
					id.SetT2( ID_PAD );
					id.SetI2( i );
					id.SetU2( pin->m_uid );
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = m_dlist->Add( part->m_id, part, pad_layer, 
						DL_OCTAGON, 1, 
						p->size_h,
						0, 0,
						pin->x, pin->y, 
						0, 0, 
						0, 0 );
					if( !pin->dl_sel )
					{
						id.SetT2( ID_SEL_PAD );
						pin->dl_sel = m_dlist->AddSelector( id, part, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							pin->x-p->size_h/2,  
							pin->y-p->size_h/2, 
							pin->x+p->size_h/2, 
							pin->y+p->size_h/2, 0, 0 );
					}
				}
				pin->dl_els[il] = pad_el;
				pin->dl_hole = pad_el;
			}
		}
		// if through-hole pad, just draw hole and set pin_dl_el;
		if( ps->hole_size )
		{
			pin_pt.x = ps->x_rel;
			pin_pt.y = ps->y_rel;
			// flip if part on bottom
			if( part->side )
			{
				pin_pt.x = -pin_pt.x;
			}
			// rotate
			if( angle > 0 )
				RotatePoint( &pin_pt, angle, zero );
			// add to display list
			id.SetT2( ID_PAD );
			id.SetI2( i );
			id.SetU2( pin->m_uid );
			pin->x = x + pin_pt.x;
			pin->y = y + pin_pt.y;
			pin->dl_hole = m_dlist->Add( id, part, LAY_PAD_THRU, 
								DL_HOLE, 1, 
								ps->hole_size,
								0, 0,
								pin->x, pin->y, 0, 0, 0, 0 );  
			if( !pin->dl_sel )
			{
				// make selector for pin with hole only
				id.SetT2( ID_SEL_PAD );
				pin->dl_sel = m_dlist->AddSelector( id, part, sel_layer, 
					DL_HOLLOW_RECT, 1, 1, 0,
					pin->x-ps->hole_size/2,  
					pin->y-ps->hole_size/2,  
					pin->x+ps->hole_size/2,  
					pin->y+ps->hole_size/2,  
					0, 0 );
			}
		}
		else
			pin->dl_hole = NULL;
	}
	part->drawn = TRUE;
#endif
	return PL_NOERR;
}

// Undraw part from display list
//
int CPartList::UndrawPart( cpart * part )
{
	int i;

	if( !m_dlist )
		return 0;

	if( part->drawn == FALSE )
		return 0;

	CShape * shape = part->shape;
	if( shape )
	{
		// undraw selection rectangle
		m_dlist->Remove( part->dl_sel );
		part->dl_sel = 0;

		// undraw selection rectangle for ref text
		m_dlist->Remove( part->dl_ref_sel );
		part->dl_ref_sel = 0;

		// undraw ref designator text
		int nstrokes = part->ref_text_stroke.GetSize();
		for( i=0; i<nstrokes; i++ )
		{
			m_dlist->Remove( part->ref_text_stroke[i].dl_el );
			part->ref_text_stroke[i].dl_el = 0;
		}

		// undraw selection rectangle for value
		m_dlist->Remove( part->dl_value_sel );
		part->dl_value_sel = 0;

		// undraw  value text
		nstrokes = part->value_stroke.GetSize();
		for( i=0; i<nstrokes; i++ )
		{
			m_dlist->Remove( part->value_stroke[i].dl_el );
			part->value_stroke[i].dl_el = 0;
		}

		// undraw part outline (this also includes footprint free text)
		for( i=0; i<part->m_outline_stroke.GetSize(); i++ )
		{
			m_dlist->Remove( (dl_element*)part->m_outline_stroke[i].dl_el );
			part->m_outline_stroke[i].dl_el = 0;
		}

		// undraw padstacks
		for( i=0; i<part->pin.GetSize(); i++ )
		{
			part_pin * pin = &part->pin[i];
			if( pin->dl_els.GetSize()>0 )
			{
				for( int il=0; il<pin->dl_els.GetSize(); il++ )
				{
					if( pin->dl_els[il] != pin->dl_hole )
						m_dlist->Remove( pin->dl_els[il] );
				}
				pin->dl_els.RemoveAll();
			}
			m_dlist->Remove( pin->dl_hole );
			m_dlist->Remove( pin->dl_sel );
			pin->dl_hole = NULL;
			pin->dl_sel = NULL;
		}
	}
	part->drawn = FALSE;
	return PL_NOERR;
}

// the footprint was changed for a particular part
// note that this function also updates the netlist
//
void CPartList::PartFootprintChanged( cpart * part, CShape * new_shape )
{
	UndrawPart( part );
	// new footprint
	part->shape = new_shape;
	part->pin.SetSize( new_shape->GetNumPins() );
	for( int ip=0; ip<part->pin.GetSize(); ip++ )
		part->pin[ip].m_part = part;	// set parent part for each pin
	// calculate new pin positions
	if( part->shape )
	{
		for( int ip=0; ip<part->pin.GetSize(); ip++ )
		{
			CPoint pt = GetPinPoint( part, ip );
			part->pin[ip].x = pt.x;
			part->pin[ip].y = pt.y;
		}
	}
	DrawPart( part );
	m_nlist->PartFootprintChanged( part );
}

// the footprint was modified, apply to all parts using it
//
void CPartList::FootprintChanged( CShape * shape )
{
	// find all parts with given footprint and update them
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		if( part->shape )
		{
			if(  part->shape->m_name == shape->m_name  )
			{
				PartFootprintChanged( part, shape );
			}
		}
		part = part->next;
	}
}

// the ref text height and width were modified, apply to all parts using it
//
void CPartList::RefTextSizeChanged( CShape * shape )
{
#ifndef CPT2	// Hide incompatible stuff from compiler
	// find all parts with given shape and update them
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		if(  part->shape->m_name == shape->m_name  )
		{
			ResizeRefText( part, shape->m_ref_size, shape->m_ref_w );
		}
		part = part->next;
	}
#endif
}

// Make part visible or invisible, including thermal reliefs
//
void CPartList::MakePartVisible( cpart * part, BOOL bVisible )
{
	// make part elements invisible, including copper area connections
	// outline strokes
	for( int i=0; i<part->m_outline_stroke.GetSize(); i++ )
	{
		dl_element * el = part->m_outline_stroke[i].dl_el;
		el->visible = bVisible;
	}
	// pins
	for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
	{
		// pin pads
		dl_element * el = part->pin[ip].dl_hole;
		if( el )
			el->visible = bVisible;
		for( int i=0; i<part->pin[ip].dl_els.GetSize(); i++ )
		{
			if( part->pin[ip].dl_els[i] )
				part->pin[ip].dl_els[i]->visible = bVisible;
		}
		// pin copper area connections
		cnet * net = (cnet*)part->pin[ip].net;
		if( net )
		{
			for( int ia=0; ia<net->NumAreas(); ia++ )
			{
				for( int i=0; i<net->area[ia].npins; i++ )
				{
					if( net->pin[net->area[ia].pin[i]].part == part )
					{
						m_dlist->Set_visible( net->area[ia].dl_thermal[i], bVisible );
					}
				}
			}
		}
	}
	// ref text strokes
	for( int is=0; is<part->ref_text_stroke.GetSize(); is++ )
	{
		void * ptr = part->ref_text_stroke[is].dl_el;
		dl_element * el = (dl_element*)ptr;
		el->visible = bVisible;
	}
	// value strokes
	for( int is=0; is<part->value_stroke.GetSize(); is++ )
	{
		void * ptr = part->value_stroke[is].dl_el;
		dl_element * el = (dl_element*)ptr;
		el->visible = bVisible;
	}
}

// Start dragging part by setting up display list
// if bRatlines == FALSE, no rubber-band ratlines
// else if bBelowPinCount == TRUE, only use ratlines for nets with less than pin_count pins
//
int CPartList::StartDraggingPart( CDC * pDC, cpart * part, BOOL bRatlines, 
								 BOOL bBelowPinCount, int pin_count )
{
#ifndef CPT2
	// make part invisible
	MakePartVisible( part, FALSE );
	m_dlist->CancelHighlight();

	// create drag lines
	CPoint zero(0,0);
	m_dlist->MakeDragLineArray( 2*part->shape->m_padstack.GetSize() + 4 );
	CArray<CPoint> pin_points;
	pin_points.SetSize( part->shape->m_padstack.GetSize() );
	int xi = part->shape->m_sel_xi;
	int xf = part->shape->m_sel_xf;
	if( part->side )
	{
		xi = -xi;
		xf = -xf;
	}
	int yi = part->shape->m_sel_yi;
	int yf = part->shape->m_sel_yf;
	CPoint p1( xi, yi );
	CPoint p2( xf, yi );
	CPoint p3( xf, yf );
	CPoint p4( xi, yf );
	RotatePoint( &p1, part->angle, zero );
	RotatePoint( &p2, part->angle, zero );
	RotatePoint( &p3, part->angle, zero );
	RotatePoint( &p4, part->angle, zero );
	m_dlist->AddDragLine( p1, p2 ); 
	m_dlist->AddDragLine( p2, p3 ); 
	m_dlist->AddDragLine( p3, p4 ); 
	m_dlist->AddDragLine( p4, p1 ); 
	for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
	{
		// make X for each pin
		int d = part->shape->m_padstack[ip].top.size_h/2;
		CPoint p(part->shape->m_padstack[ip].x_rel,part->shape->m_padstack[ip].y_rel);
		int xi = p.x-d;
		int yi = p.y-d;
		int xf = p.x+d;
		int yf = p.y+d;
		// reverse if on other side of board
		if( part->side )
		{
			xi = -xi;
			xf = -xf;
			p.x = -p.x;
		}
		CPoint p1, p2, p3, p4;
		p1.x = xi;
		p1.y = yi;
		p2.x = xf;
		p2.y = yi;
		p3.x = xf;
		p3.y = yf;
		p4.x = xi;
		p4.y = yf;
		// rotate by part.angle
		RotatePoint( &p1, part->angle, zero );
		RotatePoint( &p2, part->angle, zero );
		RotatePoint( &p3, part->angle, zero );
		RotatePoint( &p4, part->angle, zero );
		RotatePoint( &p, part->angle, zero );
		// save pin position
		pin_points[ip].x = p.x;
		pin_points[ip].y = p.y;
		// draw X
		m_dlist->AddDragLine( p1, p3 ); 
		m_dlist->AddDragLine( p2, p4 );
	}
	// create drag lines for ratlines connected to pins
	if( bRatlines ) 
	{
		m_dlist->MakeDragRatlineArray( 2*part->shape->m_padstack.GetSize(), 1 );
		// zero utility flags for all nets
		CIterator_cnet iter_net(m_nlist);
		cnet * n = iter_net.GetFirst();
		while( n )
		{
			n->utility = 0;
			n = iter_net.GetNext();
		}

		// now loop through all pins in part to find nets that connect
		for( int ipp=0; ipp<part->shape->m_padstack.GetSize(); ipp++ )
		{
			n = (cnet*)part->pin[ipp].net;
			if( n )
			{
				// only look at visible nets, only look at each net once
				if( n->visible && n->utility == 0 )
				{
					// zero utility flags for all connections
					CIterator_cconnect iter_con(n);
					for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
						c->utility = 0;
					// now iterate through all connections in this net
					for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
					{
						if( c->utility )
							continue;	// skip this connection

						// check for connection to part
						cpin * pin1 = c->StartPin();
						cpin * pin2 = c->EndPin();
						cpart * pin1_part = NULL;
						if( pin1 != NULL )
							pin1_part = pin1->part;
						cpart * pin2_part = NULL;
						if( pin2 != NULL )
							pin2_part = pin2->part;
						if( pin1_part != part && pin2_part != part )
							continue;	// no

						// OK, this connection is attached to our part 
						if( pin1_part == part )
						{
							int ip = pin1_part->shape->GetPinIndexByName( pin1->pin_name );
							if( ip != -1 )
							{
								// ip is the start pin for the connection
								// hide segment
								cseg * first_seg = &c->SegByIndex(0);
								cvertex * v = &first_seg->GetPostVtx();
								first_seg->SetVisibility( 0 );
								for( int i=0; i<v->dl_el.GetSize(); i++ )
									m_dlist->Set_visible( v->dl_el[i], 0 );
								if( v->dl_hole )
									m_dlist->Set_visible( v->dl_hole, 0 );
								// add ratline
//**								if( !bBelowPinCount || n->npins <= pin_count )
								{
									BOOL bDraw = FALSE;
									if( pin2_part == part )
									{
										// connection starts and ends on this part,
										// only drag if 3 or more segments
										if( c->NumSegs() > 2 )
											bDraw = TRUE;
									}
									else if( pin2_part == NULL )
									{
										// stub trace starts on this part,
										// drag if more than 1 segment or next vertex is a tee
										if( c->NumSegs() > 1 || v->tee_ID )
											bDraw = TRUE;
									}
									else if( pin2_part )
									{
										// connection ends on another part
										bDraw = TRUE;
									}
									if( bDraw )
									{
										// add dragline from pin to second vertex
										CPoint vx( v->x, v->y );
										m_dlist->AddDragRatline( vx, pin_points[ip] );
									}
								}
							}
						}
						if( pin2_part == part )
						{
							int ip = -1;
							ip = pin2_part->shape->GetPinIndexByName( pin2->pin_name );
							if( ip != -1 )
							{
								// ip is the end pin for the connection
								cseg * last_seg = &c->SegByIndex(c->NumSegs()-1);
								cvertex * v = &last_seg->GetPreVtx();
								c->SegByIndex(c->NumSegs()-1).SetVisibility( 0 );
								if( v->dl_el.GetSize() )
									for( int i=0; i<v->dl_el.GetSize(); i++ )
										m_dlist->Set_visible( v->dl_el[i], 0 );
								if( v->dl_hole )
									m_dlist->Set_visible( v->dl_hole, 0 );
								// OK, get prev vertex, add ratline and hide segment
//**								if( !bBelowPinCount || n->npins <= pin_count )
								{
									BOOL bDraw = FALSE;
									if( pin1_part == part )
									{
										// starts and ends on part
										if( c->NumSegs() > 2 )
											bDraw = TRUE;
									}
									else
										bDraw = TRUE;
									if( bDraw )
									{
										CPoint vx( v->x, v->y );
										m_dlist->AddDragRatline( vx, pin_points[ip] );
									}
								}
							}
						}
						c->utility = 1;	// this connection has been checked
					}
				}
				n->utility = 1;	// all connections for this net have been checked
			}
		}
	}
	int vert = 0;
	if( part->angle == 90 || part->angle == 270 )
		vert = 1;
	m_dlist->StartDraggingArray( pDC, part->x, part->y, vert, LAY_SELECTION );
#endif
	return 0;
}

// start dragging ref text
//
int CPartList::StartDraggingRefText( CDC * pDC, cpart * part )
{
	// make ref text elements invisible
	for( int is=0; is<part->ref_text_stroke.GetSize(); is++ )
	{
		void * ptr = part->ref_text_stroke[is].dl_el;
		dl_element * el = (dl_element*)ptr;
		el->visible = 0;
	}
	// cancel selection 
	m_dlist->CancelHighlight();
	// drag
	m_dlist->StartDraggingRectangle( pDC, 
						m_dlist->Get_x(part->dl_ref_sel), 
						m_dlist->Get_y(part->dl_ref_sel),
						m_dlist->Get_x(part->dl_ref_sel) - m_dlist->Get_x_org(part->dl_ref_sel),
						m_dlist->Get_y(part->dl_ref_sel) - m_dlist->Get_y_org(part->dl_ref_sel),
						m_dlist->Get_xf(part->dl_ref_sel) - m_dlist->Get_x_org(part->dl_ref_sel),
						m_dlist->Get_yf(part->dl_ref_sel) - m_dlist->Get_y_org(part->dl_ref_sel), 
						0, LAY_SELECTION );
	return 0;
}

// start dragging value
//
int CPartList::StartDraggingValue( CDC * pDC, cpart * part )
{
	// make value text elements invisible
	for( int is=0; is<part->value_stroke.GetSize(); is++ )
	{
		void * ptr = part->value_stroke[is].dl_el;
		dl_element * el = (dl_element*)ptr;
		el->visible = 0;
	}
	// cancel selection 
	m_dlist->CancelHighlight();
	// drag
	m_dlist->StartDraggingRectangle( pDC, 
						m_dlist->Get_x(part->dl_value_sel), 
						m_dlist->Get_y(part->dl_value_sel),
						m_dlist->Get_x(part->dl_value_sel) - m_dlist->Get_x_org(part->dl_value_sel),
						m_dlist->Get_y(part->dl_value_sel) - m_dlist->Get_y_org(part->dl_value_sel),
						m_dlist->Get_xf(part->dl_value_sel) - m_dlist->Get_x_org(part->dl_value_sel),
						m_dlist->Get_yf(part->dl_value_sel) - m_dlist->Get_y_org(part->dl_value_sel), 
						0, LAY_SELECTION );
	return 0;
}

// cancel dragging, return to pre-dragging state
//
int CPartList::CancelDraggingPart( cpart * part )
{
	// make part visible again
	MakePartVisible( part, TRUE );

	// get any connecting segments and make visible
	for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
	{
		cnet * net = (cnet*)part->pin[ip].net;
		if( net )
		{
			if( net->visible )
			{
				CIterator_cconnect iter_con(net);
				for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
				{
					cpin * pin1 = c->StartPin();
					cpin * pin2 = c->EndPin();
					int nsegs = c->NumSegs();
					if( pin1->part == part )
					{
						// start pin
						cseg * first_seg = &c->SegByIndex(0);
						cvertex * v = &first_seg->GetPostVtx();
						first_seg->SetVisibility( 1 );
						for( int i=0; i<v->dl_el.GetSize(); i++ )
							m_dlist->Set_visible( v->dl_el[i], 1 );
						if( v->dl_hole )
							m_dlist->Set_visible( v->dl_hole, 1 );
					}
					if( pin2 != NULL )
					{
						if( pin2->part == part )
						{
							// end pin
							cseg * last_seg = &c->SegByIndex(nsegs-1);
							cvertex * v = &last_seg->GetPreVtx();
							last_seg->SetVisibility( 1 );
							if( v->dl_el.GetSize() )
								for( int i=0; i<v->dl_el.GetSize(); i++ )
									m_dlist->Set_visible( v->dl_el[i], 1 );
							if( v->dl_hole )
								m_dlist->Set_visible( v->dl_hole, 1 );
						}
					}
				}
			}
		}
	}
	m_dlist->StopDragging();
	return 0;
}

// cancel dragging of ref text, return to pre-dragging state
int CPartList::CancelDraggingRefText( cpart * part )
{
	// make ref text elements invisible
	for( int is=0; is<part->ref_text_stroke.GetSize(); is++ )
	{
		void * ptr = part->ref_text_stroke[is].dl_el;
		dl_element * el = (dl_element*)ptr;
		el->visible = 1;
	}
	m_dlist->StopDragging();
	return 0;
}

// cancel dragging value, return to pre-dragging state
int CPartList::CancelDraggingValue( cpart * part )
{
	// make ref text elements invisible
	for( int is=0; is<part->value_stroke.GetSize(); is++ )
	{
		void * ptr = part->value_stroke[is].dl_el;
		dl_element * el = (dl_element*)ptr;
		el->visible = 1;
	}
	m_dlist->StopDragging();
	return 0;
}

// normal completion of any dragging operation
//
int CPartList::StopDragging()
{
	m_dlist->StopDragging();
	return 0;
}

// create part from string
//
cpart * CPartList::AddFromString( CString * str )
{
	// set defaults
	CShape * s = NULL;
	CString in_str, key_str;
	CArray<CString> p;
	int pos = 0;
	int len = str->GetLength();
	int np;
	CString ref_des;
	BOOL ref_vis = TRUE;
	int ref_size = 0;
	int ref_width = 0;
	int ref_angle = 0;
	int ref_xi = 0;
	int ref_yi = 0;
	int ref_layer = LAY_SILK_TOP;
	CString value;
	BOOL value_vis = TRUE;
	int value_size = 0;
	int value_width = 0;
	int value_angle = 0;
	int value_xi = 0;
	int value_yi = 0;
	int value_layer = LAY_SILK_TOP;

	// add part to partlist
	CString package;
	int x;
	int y;
	int side;
	int angle;
	int glued;
	cpart * part = Add();

	// so we only draw once
	CDisplayList * old_dlist = m_dlist;
	m_dlist = NULL;

	in_str = str->Tokenize( "\n", pos );
	while( in_str != "" )
	{
		np = ParseKeyString( &in_str, &key_str, &p );
		if( key_str == "ref" )
		{
			ref_des = in_str.Right( in_str.GetLength()-4 );
			ref_des.Trim();
			ref_des = ref_des.Left(MAX_REF_DES_SIZE);
		}
		else if( key_str == "part" )
		{
			ref_des = in_str.Right( in_str.GetLength()-5 );
			ref_des.Trim();
			ref_des = ref_des.Left(MAX_REF_DES_SIZE);
		}
		else if( np >= 6 && key_str == "ref_text" )
		{
			ref_size = my_atoi( &p[0] );
			ref_width = my_atoi( &p[1] );
			ref_angle = my_atoi( &p[2] );
			ref_xi = my_atoi( &p[3] );
			ref_yi = my_atoi( &p[4] );
			if( np > 6 )
				ref_vis = my_atoi( &p[5] );
			else
			{
				if( ref_size )
					ref_vis = TRUE;
				else
					ref_vis = FALSE;
			}
			if( np > 7 )
				ref_layer = my_atoi( &p[6] );
		}
		else if( np >= 7 && key_str == "value" )
		{
			value = p[0];
			value_size = my_atoi( &p[1] );
			value_width = my_atoi( &p[2] );
			value_angle = my_atoi( &p[3] );
			value_xi = my_atoi( &p[4] );
			value_yi = my_atoi( &p[5] );
			if( np > 7 )
				value_vis = my_atoi( &p[6] );
			else
			{
				if( value_size )
					value_vis = TRUE;
				else
					value_vis = FALSE;
			}
			if( np > 8 )
				value_layer = my_atoi( &p[7] );
		}
		else if( key_str == "package" )
		{
			if( np >= 2 )
				package = p[0];
			else
				package = "";
			package = package.Left(CShape::MAX_NAME_SIZE);
		}
		else if( np >= 2 && key_str == "shape" )
		{
			// lookup shape in cache
			s = NULL;
			void * ptr;
			CString name = p[0];
			name = name.Left(CShape::MAX_NAME_SIZE);
			int err = m_footprint_cache_map->Lookup( name, ptr );
			if( err )
			{
				// found in cache
				s = (CShape*)ptr; 
			}
		}
		else if( key_str == "pos" )
		{
			if( np >= 6 )
			{
				x = my_atoi( &p[0] );
				y = my_atoi( &p[1] );
				side = my_atoi( &p[2] );
				angle = my_atoi( &p[3] );
				glued = my_atoi( &p[4] );
			}
			else
			{
				x = 0;
				y = 0;
				side = 0;
				angle = 0;
				glued = 0;
			}
		}
		in_str = str->Tokenize( "\n", pos );
	}
	SetPartData( part, s, &ref_des, &package, x, y, side, angle, 1, glued, ref_vis );
	SetValue( part, &value, value_xi, value_yi, value_angle, value_size, value_width, 
		value_vis, value_layer );
	if( part->shape ) 
	{
		part->m_ref_xi = ref_xi;
		part->m_ref_yi = ref_yi;
		part->m_ref_angle = ref_angle;
		part->m_ref_layer = ref_layer;
		ResizeRefText( part, ref_size, ref_width, ref_vis );
	}
	m_dlist = old_dlist;
	DrawPart( part );
	return part;
}

// read partlist
//
int CPartList::ReadParts( CStdioFile * pcb_file )
{
	int pos, err;
	CString in_str, key_str;
	CArray<CString> p;

	// find beginning of [parts] section
	do
	{
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			// error reading pcb file
			CString mess ((LPCSTR) IDS_UnableToFindPartsSectionInFile);
			AfxMessageBox( mess );
			return 0;
		}
		in_str.Trim();
	}
	while( in_str != "[parts]" );

	// get each part in [parts] section
	while( 1 )
	{
		pos = pcb_file->GetPosition();
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
			throw err_str;
		}
		in_str.Trim();
		if( in_str[0] == '[' && in_str != "[parts]" )
		{
			pcb_file->Seek( pos, CFile::begin );
			break;		// next section, exit
		}
		else if( in_str.Left(4) == "ref:" || in_str.Left(5) == "part:" )
		{
			CString str;
			do
			{
				str.Append( in_str );
				str.Append( "\n" );
				pos = pcb_file->GetPosition();
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
					throw err_str;
				}
				in_str.Trim();
			} while( (in_str.Left(4) != "ref:" && in_str.Left(5) != "part:" )
						&& in_str[0] != '[' );
			pcb_file->Seek( pos, CFile::begin );

			// now add part to partlist
			cpart * part = AddFromString( &str );
		}
	}
	return 0;
}

// set CString to description of part
//
int CPartList::SetPartString( cpart * part, CString * str )
{
	CString line;

	line.Format( "part: %s\n", part->ref_des );  
	*str = line;
	if( part->shape )
		line.Format( "  ref_text: %d %d %d %d %d %d %d\n", 
					part->m_ref_size, part->m_ref_w, part->m_ref_angle%360,
					part->m_ref_xi, part->m_ref_yi, part->m_ref_vis, part->m_ref_layer );
	else
		line.Format( "  ref_text: \n" );
	str->Append( line );
	line.Format( "  package: \"%s\"\n", part->package );
	str->Append( line );
	if( part->value != "" ) 
	{
		if( part->shape )
			line.Format( "  value: \"%s\" %d %d %d %d %d %d %d\n", 
			part->value, part->m_value_size, 
			part->m_value_w, part->m_value_angle%360,
			part->m_value_xi, part->m_value_yi,
			part->m_value_vis, part->m_value_layer );
		else
			line.Format( "  value: \"%s\"\n", part->value );
		str->Append( line );
	}
	if( part->shape )
		line.Format( "  shape: \"%s\"\n", part->shape->m_name );
	else
		line.Format( "  shape: \n" );
	str->Append( line );
	line.Format( "  pos: %d %d %d %d %d\n", part->x, part->y, part->side, part->angle%360, part->glued );
	str->Append( line );

	line.Format( "\n" );
	str->Append( line );
	return 0;
}

// create record describing part for use by CUndoList
// if part == NULL, just set m_plist and new_ref_des
//
undo_part * CPartList::CreatePartUndoRecord( cpart * part, CString * new_ref_des )
{
	int size = sizeof( undo_part );
	if( part )
		size += part->shape->GetNumPins()*( sizeof(int) + CShape::MAX_PIN_NAME_SIZE+1 );
	undo_part * upart = (undo_part*)malloc( size );
	upart->size = size;
	upart->m_plist = this;
	if( part )
	{
		char * chptr = (char*)upart;
		chptr += sizeof(undo_part);
		upart->m_id = part->m_id;
		upart->visible = part->visible;
		upart->x = part->x;
		upart->y = part->y;
		upart->side = part->side;
		upart->angle = part->angle;
		upart->glued = part->glued;
		upart->m_ref_vis = part->m_ref_vis;
		upart->m_ref_xi = part->m_ref_xi;
		upart->m_ref_yi = part->m_ref_yi;
		upart->m_ref_angle = part->m_ref_angle;
		upart->m_ref_size = part->m_ref_size;
		upart->m_ref_w = part->m_ref_w;
		upart->m_ref_layer =part->m_ref_layer;
		upart->m_value_vis = part->m_value_vis;
		upart->m_value_xi = part->m_value_xi;
		upart->m_value_yi = part->m_value_yi;
		upart->m_value_angle = part->m_value_angle;
		upart->m_value_size = part->m_value_size;
		upart->m_value_w = part->m_value_w;
		upart->m_value_layer =part->m_value_layer;
		strcpy( upart->ref_des, part->ref_des );
		strcpy( upart->package , part->package );
		strcpy( upart->shape_name, part->shape->m_name );
		strcpy( upart->value, part->value );
		upart->shape = part->shape;
		if( part->shape )
		{
			// save names of nets attached to each pin
			for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
			{
				if( cnet * net = part->pin[ip].net )
					strcpy( chptr, net->name );
				else
					*chptr = 0;
				chptr += CShape::MAX_PIN_NAME_SIZE + 1;
			}
			// save UID for each pin
			for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
			{
				int * intptr = (int*)chptr;
				*intptr = part->pin[ip].m_uid;
				chptr += sizeof(int);
			}
		}
	}
	if( new_ref_des )
		strcpy( upart->new_ref_des, *new_ref_des );
	else
		strcpy( upart->new_ref_des, part->ref_des );
	return upart;
}

#if 0
// create special record for use by CUndoList
//
void * CPartList::CreatePartUndoRecordForRename( cpart * part, CString * old_ref_des )
{
	int size = sizeof( undo_part );
	undo_part * upart = (undo_part*)malloc( size );
	upart->m_plist = this;
	strcpy( upart->ref_des, part->ref_des );
	strcpy( upart->package, *old_ref_des );
	return (void*)upart;
}
#endif

// write all parts and footprints to file
//
int CPartList::WriteParts( CStdioFile * file )
{
	// CPT: Sort the part lines.
	CMapStringToPtr shape_map;
	cpart * el = m_start.next;
	CString line;
	CString key;
	CArray<CString> lines;
	try
	{
		// now write all parts
		line.Format( "[parts]\n\n" );
		file->WriteString( line );
		el = m_start.next;
		while( el->next != 0 )
		{
			// test
			CString test;
			SetPartString( el, &test );
			lines.Add(test);
			el = el->next;
		}
		extern int strcmpNumeric(CString *s1, CString *s2);															// In FreePcbDoc.cpp
		qsort(lines.GetData(), lines.GetSize(), sizeof(CString), (int (*)(const void*,const void*)) strcmpNumeric);			
		for (int i=0; i<lines.GetSize(); i++)
			file->WriteString( lines[i] );
	}
	// end CPT
	catch( CFileException * e )
	{
		CString str, s ((LPCSTR) IDS_FileError1), s2 ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			str.Format( s, e->m_cause );
		else
			str.Format( s2, e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		return 1;
	}

	return 0;
}

// utility function to rotate a point clockwise about another point
// currently, angle must be 0, 90, 180 or 270
//
int CPartList::RotatePoint( CPoint *p, int angle, CPoint org )
{
	CRect tr;
	if( angle == 90 )
	{
		int tempy = org.y + (org.x - p->x);
		p->x = org.x + (p->y - org.y);
		p->y = tempy;
	}
	else if( angle > 90 )
	{
		for( int i=0; i<(angle/90); i++ )
			RotatePoint( p, 90, org );
	}
	return PL_NOERR;
}

// utility function to rotate a rectangle clockwise about a point
// currently, angle must be 0, 90, 180 or 270
// assumes that (r->right) > (r->left), (r->top) > (r->bottom)
//
int CPartList::RotateRect( CRect *r, int angle, CPoint org )
{
	CRect tr;
	if( angle == 90 )
	{
		tr.left = org.x + (r->bottom - org.y);
		tr.right = org.x + (r->top - org.y);
		tr.bottom = org.y + (org.x - r->right);
		tr.top = org.y + (org.x - r->left);
	}
	else if( angle > 90 )
	{
		tr = *r;
		for( int i=0; i<(angle/90); i++ )
			RotateRect( &tr, 90, org );
	}
	*r = tr;
	return PL_NOERR;
}

// export part list data into partlist_info structure for editing in dialog
// if test_part != NULL, returns index of test_part in partlist_info
//
int CPartList::ExportPartListInfo( partlist_info * pl, cpart * test_part )
{
#ifndef CPT2										// Hide from compiler the old version's incompatibilities with the new
	// traverse part list to find number of parts
	int ipart = -1;
	int nparts = 0;
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		nparts++;
		part = part->next;
	}
	// now make struct
	pl->SetSize( nparts );
	int i = 0;
	part = m_start.next;
	while( part->next != 0 )
	{
		if( part == test_part )
			ipart = i;
		(*pl)[i].part = part;
		(*pl)[i].shape = part->shape;
		(*pl)[i].bShapeChanged = FALSE;
		(*pl)[i].ref_des = part->ref_des;
		(*pl)[i].ref_vis = part->m_ref_vis;
		(*pl)[i].ref_size = part->m_ref_size;
		(*pl)[i].ref_width = part->m_ref_w;
		(*pl)[i].package = part->package;
		(*pl)[i].value = part->value;
		(*pl)[i].value_vis = part->m_value_vis;
		(*pl)[i].value_layer = part->m_value_layer;
		(*pl)[i].x = part->x;
		(*pl)[i].y = part->y;
		(*pl)[i].angle = part->angle;
		(*pl)[i].side = part->side;
		(*pl)[i].deleted = FALSE;
		(*pl)[i].bOffBoard = FALSE;
		i++;
		part = part->next;
	}
	return ipart;
#endif
	return 0;
}

// import part list data from struct partlist_info
//
void CPartList::ImportPartListInfo( partlist_info * pl, int flags, CDlgLog * log )
{
#ifndef CPT2			// Hiding compile-time errors caused by changing the innards of type partlist
	CString mess; 

	// undraw all parts and disable further drawing
	CDisplayList * old_dlist = m_dlist;
	if( m_dlist )
	{
		cpart * part = GetFirstPart();
		while( part )
		{
			UndrawPart( part );
			part = GetNextPart( part );
		}
	}
	m_dlist = NULL;		

	// grid for positioning parts off-board
	int pos_x = 0;
	int pos_y = 0;
	enum { GRID_X = 100, GRID_Y = 50 };
	BOOL * grid = (BOOL*)calloc( GRID_X*GRID_Y, sizeof(BOOL) );
	int grid_num = 0;

	// first, look for parts in project whose ref_des has been changed
	for( int i=0; i<pl->GetSize(); i++ )
	{
		part_info * pi = &(*pl)[i];
		if( pi->part )
		{
			if( pi->ref_des != pi->part->ref_des )
			{
				m_nlist->PartRefChanged( &pi->part->ref_des, &pi->ref_des );
				pi->part->ref_des = pi->ref_des;
			}
		}
	}

	// now find parts in project that are not in partlist_info
	// loop through all parts in project
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		// loop through the partlist_info array
		BOOL bFound = FALSE;
		part->bPreserve = FALSE;
		for( int i=0; i<pl->GetSize(); i++ )
		{
			part_info * pi = &(*pl)[i];
			if( pi->ref_des == part->ref_des )
			{
				// part exists in partlist_info
				bFound = TRUE;
				break;
			}
		}
		cpart * next_part = part->next;
		if( !bFound )
		{
			// part in project but not in partlist_info
			if( flags & KEEP_PARTS_AND_CON )
			{
				// set flag to preserve this part
				part->bPreserve = TRUE;
				if( log )
				{
					CString s ((LPCSTR) IDS_KeepingPartAndConnections);
					mess.Format( s, part->ref_des );
					log->AddLine( mess );
				}
			}
			else if( flags & KEEP_PARTS_NO_CON )
			{
				// keep part but remove connections from netlist
				if( log )
				{
					CString s ((LPCSTR) IDS_KeepingPartButRemovingConnections);
					mess.Format( s, part->ref_des );
					log->AddLine( mess );
				}
				m_nlist->PartDeleted( part );
			}
			else
			{
				// remove part
				if( log )
				{
					CString s ((LPCSTR) IDS_RemovingPart);
					mess.Format( s, part->ref_des );
					log->AddLine( mess );
				}
				m_nlist->PartDeleted( part );
				Remove( part );
			}
		}
		part = next_part;
	}

	// loop through partlist_info array, changing partlist as necessary
	for( int i=0; i<pl->GetSize(); i++ )
	{
		part_info * pi = &(*pl)[i];
		if( pi->part == 0 && pi->deleted )
		{
			// new part was added but then deleted, ignore it
			continue;
		}
		if( pi->part != 0 && pi->deleted )
		{
			// old part was deleted, remove it
			m_nlist->PartDisconnected( pi->part );
			Remove( pi->part );
			continue;
		}

		if( pi->part == 0 )
		{
			// the partlist_info does not include a pointer to an existing part
			// the part might not exist in the project, or we are importing a netlist file
			cpart * old_part = GetPartByName( pi->ref_des );
			if( old_part )
			{
				// an existing part has the same ref_des as the new part
				if( old_part->shape )
				{
					// the existing part has a footprint
					// see if the incoming package name matches the old package or footprint
					if( (flags & KEEP_FP) 
						|| (pi->package == "") 
						|| (pi->package == old_part->package)
						|| (pi->package == old_part->shape->m_name) )
					{
						// use footprint and parameters from existing part
						pi->part = old_part;
						pi->ref_size = old_part->m_ref_size; 
						pi->ref_width = old_part->m_ref_w;
						pi->ref_vis = old_part->m_ref_vis;				// CPT
						pi->value = old_part->value;
						pi->value_vis = old_part->m_value_vis;
						pi->x = old_part->x; 
						pi->y = old_part->y;
						pi->angle = old_part->angle;
						pi->side = old_part->side;
						pi->shape = old_part->shape;
					}
					else if( pi->shape )
					{
						// use new footprint, but preserve position
						pi->ref_size = old_part->m_ref_size; 
						pi->ref_width = old_part->m_ref_w;
						pi->ref_vis = old_part->m_ref_vis;				// CPT
						pi->value = old_part->value;
						pi->value_vis = old_part->m_value_vis;
						pi->x = old_part->x; 
						pi->y = old_part->y;
						pi->angle = old_part->angle;
						pi->side = old_part->side;
						pi->part = old_part;
						pi->bShapeChanged = TRUE;
						if( log && old_part->shape->m_name != pi->package )
						{
							CString s ((LPCSTR) IDS_ChangingFootprintOfPart);
							mess.Format( s, old_part->ref_des, old_part->shape->m_name, pi->shape->m_name );
							log->AddLine( mess );
						}
					}
					else
					{
						// new part does not have footprint, remove old part
						if( log && old_part->shape->m_name != pi->package )
						{
							CString s ((LPCSTR) IDS_ChangingFootprintOfPart2);
							mess.Format( s, old_part->ref_des, old_part->shape->m_name, pi->package );
							log->AddLine( mess );
						}
						m_nlist->PartDisconnected( old_part );
						Remove( old_part );
					}
				}
				else
				{
					// remove old part (which did not have a footprint)
					if( log && old_part->package != pi->package )
					{
						CString s ((LPCSTR) IDS_ChangingFootprintOfPart);
						mess.Format( s, old_part->ref_des, old_part->package, pi->package );
						log->AddLine( mess );
					}
					m_nlist->PartDisconnected( old_part );
					Remove( old_part );
				}
			}
		}

		if( pi->part )
		{
			if( pi->part->shape != pi->shape || pi->bShapeChanged == TRUE )
			{
				// old part exists, but footprint was changed
				if( pi->part->shape == NULL )
				{
					// old part did not have a footprint before, so remove it
					// and treat as new part
					m_nlist->PartDisconnected( pi->part );
					Remove( pi->part );
					pi->part = NULL;
				}
			}
		}

		if( pi->part == 0 )
		{
			// new part is being imported (with or without footprint)
			if( pi->shape && pi->bOffBoard )
			{
				// place new part offboard, using grid 
				int ix, iy;	// grid indices
				// find size of part in 100 mil units
				BOOL OK = FALSE;
				int w = abs( pi->shape->m_sel_xf - pi->shape->m_sel_xi )/(100*PCBU_PER_MIL)+2;
				int h = abs( pi->shape->m_sel_yf - pi->shape->m_sel_yi )/(100*PCBU_PER_MIL)+2;
				// now find space in grid for part
				for( ix=0; ix<GRID_X; ix++ )
				{
					iy = 0;
					while( iy < (GRID_Y - h) )
					{
						if( !grid[ix+GRID_X*iy] )
						{
							// see if enough space
							OK = TRUE;
							for( int iix=ix; iix<(ix+w); iix++ )
								for( int iiy=iy; iiy<(iy+h); iiy++ )
									if( grid[iix+GRID_X*iiy] )
										OK = FALSE;
							if( OK )
								break;
						}
						iy++;
					}
					if( OK )
						break;
				}
				if( OK )
				{
					// place part
					pi->side = 0;
					pi->angle = 0;
					if( grid_num == 0 )
					{
						// first grid, to left and above origin
						pi->x = -(ix+w)*100*PCBU_PER_MIL;
						pi->y = iy*100*PCBU_PER_MIL;
					}
					else if( grid_num == 1 )
					{
						// second grid, to left and below origin
						pi->x = -(ix+w)*100*PCBU_PER_MIL;
						pi->y = -(iy+h)*100*PCBU_PER_MIL;
					}
					else if( grid_num == 2 )
					{
						// third grid, to right and below origin
						pi->x = ix*100*PCBU_PER_MIL;
						pi->y = -(iy+h)*100*PCBU_PER_MIL;
					}
					// remove space in grid
					for( int iix=ix; iix<(ix+w); iix++ )
						for( int iiy=iy; iiy<(iy+h); iiy++ )
							grid[iix+GRID_X*iiy] = TRUE;
				}
				else
				{
					// fail, go to next grid
					if( grid_num == 2 )
						ASSERT(0);		// ran out of grids
					else
					{
						// zero grid
						for( int j=0; j<GRID_Y; j++ )
							for( int i=0; i<GRID_X; i++ )
								grid[j*GRID_X+i] = FALSE;
						grid_num++;
					}
				}
				// now offset for part origin
				pi->x -= pi->shape->m_sel_xi;
				pi->y -= pi->shape->m_sel_yi;
			}
			// now place part
			cpart * part = Add( pi->shape, &pi->ref_des, &pi->package, pi->x, pi->y,
				pi->side, pi->angle, TRUE, FALSE, &pi->ref_vis );
			if( part->shape )
			{
				ResizeRefText( part, pi->ref_size, pi->ref_width );
				SetValue( part, &pi->value, 
					part->shape->m_value_xi, 
					part->shape->m_value_yi,
					part->shape->m_value_angle, 
					part->shape->m_value_size, 
					part->shape->m_value_w,
					pi->value_vis, 
					FootprintLayer2Layer( part->shape->m_ref->m_layer )
					);
			}
			else
				SetValue( part, &pi->value, 0, 0, 0, 0, 0, FALSE, LAY_SILK_TOP );
			m_nlist->PartAdded( part );
		}
		else
		{
			// part existed before but may have been modified
			if( pi->part->package != pi->package )
			{
				// package changed
				pi->part->package = pi->package;
			}
			if( pi->part->value != pi->value || pi->part->m_value_vis != pi->value_vis )
			{
				// value changed, keep size, position and visibility
				SetValue( pi->part, 
					&pi->value, 
					pi->part->shape->m_value_xi, 
					pi->part->shape->m_value_yi,
					pi->part->shape->m_value_angle, 
					pi->part->shape->m_value_size, 
					pi->part->shape->m_value_w,
					pi->value_vis,
					pi->value_layer
					);

			}
			if( pi->part->m_ref_vis != pi->ref_vis )
			{
				// ref visibility changed
				pi->part->m_ref_vis = pi->ref_vis;
			}
			if( pi->part->m_value_vis != pi->value_vis )
			{
				// value visibility changed
				pi->part->m_value_vis = pi->value_vis;
			}
			if( pi->part->shape != pi->shape || pi->bShapeChanged == TRUE )
			{
				// footprint was changed
				if( pi->part->shape == NULL )
				{
					ASSERT(0);	// should never get here
				}
				else if( pi->shape && !(flags & KEEP_FP) )
				{
					// change footprint to new one
					PartFootprintChanged( pi->part, pi->shape );
					ResizeRefText( pi->part, pi->ref_size, pi->ref_width );
//** 420					m_nlist->PartFootprintChanged( part );
//** 420					m_nlist->PartMoved( pi->part );
				}
			}
			if( pi->x != pi->part->x 
				|| pi->y != pi->part->y
				|| pi->angle != pi->part->angle
				|| pi->side != pi->part->side )
			{
				// part was moved
				Move( pi->part, pi->x, pi->y, pi->angle, pi->side );
				m_nlist->PartMoved( pi->part );
			}
		}
	}
	// PurgeFootprintCache();
	free( grid );

	// redraw partlist
	m_dlist = old_dlist;
	part = GetFirstPart();
	while( part )
	{
		DrawPart( part );
		part = GetNextPart( part );
	}
#endif
}

// undo an operation on a part
// note that this is a static function, for use as a callback
//
void CPartList::PartUndoCallback( int type, void * ptr, BOOL undo )
{
	undo_part * upart = (undo_part*)ptr;

	if( undo )
	{
		// perform undo
		CString new_ref_des = upart->new_ref_des;
		CString old_ref_des = upart->ref_des;
		CPartList * pl = upart->m_plist;
		CDisplayList * old_dlist = pl->m_dlist;
		cpart * part = pl->GetPartByName( new_ref_des );
		if( type == UNDO_PART_ADD )
		{
			// part was added, just delete it
			pl->m_nlist->PartDeleted( part );
			pl->Remove( part );
		}
		else if( type == UNDO_PART_DELETE )
		{
			// part was deleted, lookup shape in cache and add part
			pl->m_dlist = NULL;		// prevent drawing
			CShape * s;
			void * s_ptr;
			int err = pl->m_footprint_cache_map->Lookup( upart->shape_name, s_ptr );
			if( err )
			{
				// found in cache
				s = (CShape*)s_ptr; 
			}
			else
				ASSERT(0);	// shape not found
			CString ref_des = upart->ref_des;
			CString package = upart->package;
			part = pl->Add( s, &ref_des, &package, upart->x, upart->y,
				upart->side, upart->angle, upart->visible, upart->glued, upart->m_id.U1()  );
			part->m_id = upart->m_id;
			part->m_ref_vis = upart->m_ref_vis;
			part->m_ref_xi = upart->m_ref_xi;
			part->m_ref_yi = upart->m_ref_yi;
			part->m_ref_angle = upart->m_ref_angle;
			part->m_ref_size = upart->m_ref_size;
			part->m_ref_w = upart->m_ref_w;
			part->m_ref_layer = upart->m_ref_layer;
			part->value = upart->value;
			part->m_value_vis = upart->m_value_vis;
			part->m_value_xi = upart->m_value_xi;
			part->m_value_yi = upart->m_value_yi;
			part->m_value_angle = upart->m_value_angle;
			part->m_value_size = upart->m_value_size;
			part->m_value_w = upart->m_value_w;
			part->m_value_layer = upart->m_value_layer;
			char * chptr = (char*)ptr + sizeof( undo_part );
			for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
			{
				if( *chptr != 0 )
				{
					CString net_name = chptr;
					cnet * net = pl->m_nlist->GetNetPtrByName( &net_name );
					part->pin[ip].net = net;
				}
				else
					part->pin[ip].net = NULL;
				chptr += MAX_NET_NAME_SIZE + 1;
			}
			for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
			{
				int * intptr = (int*)chptr;
				part->pin[ip].m_uid = *intptr;
				chptr += sizeof(int);
			}
			// if part was renamed
			if( new_ref_des != old_ref_des )
			{
				pl->m_nlist->PartRefChanged( &new_ref_des, &old_ref_des );
				part->ref_des = old_ref_des;
			}
			pl->m_dlist = old_dlist;	// turn drawing back on;
			pl->DrawPart( part );
			pl->m_nlist->PartAdded( part );
		}
		else if( type == UNDO_PART_MODIFY )
		{
			// part was moved or modified
			pl->UndrawPart( part );
			pl->m_dlist = NULL;		// prevent further drawing
			if( upart->shape != part->shape )
			{
				// footprint was changed
				pl->PartFootprintChanged( part, upart->shape );
				pl->m_nlist->PartFootprintChanged( part );
			}
			if( upart->x != part->x
				|| upart->y != part->y 
				|| upart->angle != part->angle 
				|| upart->side != part->side )
			{
				pl->Move( part, upart->x, upart->y, upart->angle, upart->side );
				pl->m_nlist->PartMoved( part );
			}
			part->glued = upart->glued; 
			part->m_ref_vis = upart->m_ref_vis;
			part->m_ref_xi = upart->m_ref_xi;
			part->m_ref_yi = upart->m_ref_yi;
			part->m_ref_angle = upart->m_ref_angle;
			part->m_ref_size = upart->m_ref_size;
			part->m_ref_w = upart->m_ref_w;
			part->m_ref_layer = upart->m_ref_layer;
			part->value = upart->value;
			part->m_value_vis = upart->m_value_vis;
			part->m_value_xi = upart->m_value_xi;
			part->m_value_yi = upart->m_value_yi;
			part->m_value_angle = upart->m_value_angle;
			part->m_value_size = upart->m_value_size;
			part->m_value_w = upart->m_value_w;
			part->m_value_layer = upart->m_value_layer;
			char * chptr = (char*)ptr + sizeof( undo_part );
			for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
			{
				if( *chptr != 0 )
				{
					CString net_name = chptr;
					cnet * net = pl->m_nlist->GetNetPtrByName( &net_name );
					part->pin[ip].net = net;
				}
				else
					part->pin[ip].net = NULL;
				chptr += MAX_NET_NAME_SIZE + 1;
			}
			for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
			{
				int * intptr = (int*)chptr;
				part->pin[ip].m_uid = *intptr;
				chptr += sizeof(int);
			}
			// if part was renamed
			if( new_ref_des != old_ref_des )
			{
				pl->m_nlist->PartRefChanged( &new_ref_des, &old_ref_des );
				part->ref_des = old_ref_des;
			}
			pl->m_dlist = old_dlist;	// turn drawing back on
			pl->DrawPart( part );
		}
		else
			ASSERT(0);
	}
	free(ptr);	// delete the undo record
}

// checks to see if a pin is connected to a trace or a copper area on a
// particular layer
//
// returns: ON_NET | TRACE_CONNECT | AREA_CONNECT
// where:
//		ON_NET = 1 if pin is on a net
//		TRACE_CONNECT = 2 if pin connects to a trace
//		AREA_CONNECT = 4 if pin connects to copper area
//
int CPartList::GetPinConnectionStatus( cpart * part, CString * pin_name, int layer )
{
#ifndef CPT2
	int pin_index = part->shape->GetPinIndexByName( *pin_name );
	cnet * net = part->pin[pin_index].net;
	if( !net )
		return NOT_CONNECTED;

	int status = ON_NET;

	// now check for traces
	CIterator_cconnect iter_con(net);
	for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
	{
		int nsegs = c->NumSegs();
		cpin * p1 = c->StartPin();
		cpin * p2 = c->EndPin();
		if( p1 && p1->part == part &&
			p1->pin_name == *pin_name &&
			c->SegByIndex(0).m_layer == layer )
		{
			// first segment connects to pin on this layer
			status |= TRACE_CONNECT;
		}
		else if( p2 == NULL )
		{
			// stub trace, ignore end pin
		}
		else if( p2->part == part &&
			p2->pin_name == *pin_name &&
			c->SegByIndex(nsegs-1).m_layer == layer )
		{
			// last segment connects to pin on this layer
			status |= TRACE_CONNECT;
			break;
		}
	}
	// now check for connection to copper area
	for( int ia=0; ia<net->NumAreas(); ia++ )
	{
		carea * a = &net->area[ia];
		for( int ip=0; ip<a->npins; ip++ )
		{
			cpin * pin = &net->pin[a->pin[ip]];
			if( pin->part == part 
				&& pin->pin_name == *pin_name
				&& a->Layer() == layer )
			{
				status |= AREA_CONNECT;
				break;
			}
		}
	}
	return status;
#else
	return 0;
#endif
}

// Function to provide info to draw pad in Gerber file (also used by DRC routines)
// On return: 
//	if no footprint for part, or no pad and no hole on this layer, returns 0
//	else returns 1 with:
//		*type = pad shape
//		*x = pin x, 
//		*y = pin y, 
//		*w = pad width, 
//		*l = pad length, 
//		*r = pad corner radius, 
//		*hole = pin hole diameter, 
//		*angle = pad angle, 
//		**net = pin net, 
//		*connection_status = ON_NET | TRACE_CONNECT | AREA_CONNECT, where:
//			ON_NET = 1 if pin is on a net
//			TRACE_CONNECT = 2 if pin connects to a trace on this layer
//			AREA_CONNECT = 4 if pin connects to copper area on this layer
//		*pad_connect_flag = 
//			PAD_CONNECT_DEFAULT if pad uses default from project
//			PAD_CONNECT_NEVER if pad never connects to copper area
//			PAD_CONNECT_THERMAL if pad connects to copper area with thermal
//			PAD_CONNECT_NOTHERMAL if pad connects to copper area without thermal
//		*clearance_type = 
//			CLEAR_NORMAL if clearance from copper area required
//			CLEAR_THERMAL if thermal connection to copper area
//			CLEAR_NONE if no clearance from copper area
// For copper layers:
//	if no pad, uses annular ring if connected
//	Uses GetPinConnectionStatus() to determine connections, this uses the area
//	connection info from the netlist
//
int CPartList::GetPadDrawInfo( cpart * part, int ipin, int layer, 
							  BOOL bUse_TH_thermals, BOOL bUse_SMT_thermals,
							  int solder_mask_swell, int paste_mask_shrink,
							  int * type, int * x, int * y, int * w, int * l, int * r, int * hole,
							  int * angle, cnet ** net, 
							  int * connection_status, int * pad_connect_flag, 
							  int * clearance_type )
{
#ifndef CPT2
	// get footprint
	CShape * s = part->shape;
	if( !s )
		return 0;

	// get pin and padstack info
	cpadstack * ps = &s->m_padstack[ipin];
	BOOL bUseDefault = FALSE; // if TRUE, use copper pad for mask
	CString pin_name = s->GetPinNameByIndex( ipin );
	int connect_status = GetPinConnectionStatus( part, &pin_name, layer );
	// set default return values for no pad and no hole
	int ret_code = 0;
	int ttype = PAD_NONE;
	int xx = part->pin[ipin].x;
	int yy = part->pin[ipin].y;
	int ww = 0;
	int ll = 0;
	int rr = 0;
	int aangle = s->m_padstack[ipin].angle + part->angle;
	aangle = aangle%180;
	int hole_size = s->m_padstack[ipin].hole_size;
	cnet * nnet = part->pin[ipin].net;
	int clear_type = CLEAR_NORMAL;	
	int connect_flag = PAD_CONNECT_DEFAULT;

	// get pad info
	pad * p = NULL;
	if( (layer == LAY_TOP_COPPER && part->side == 0 )
		|| (layer == LAY_BOTTOM_COPPER && part->side == 1 ) ) 
	{
		// top copper pad is on this layer 
		p = &ps->top;
	}
	else if( (layer == LAY_MASK_TOP && part->side == 0 )
		|| (layer == LAY_MASK_BOTTOM && part->side == 1 ) ) 
	{
		// top mask pad is on this layer 
		if( ps->top_mask.shape != PAD_DEFAULT )
			p = &ps->top_mask;
		else
		{
			bUseDefault = TRUE;		// get mask pad from copper pad
			p = &ps->top;
		}
	}
	else if( (layer == LAY_PASTE_TOP && part->side == 0 )
		|| (layer == LAY_PASTE_BOTTOM && part->side == 1 ) ) 
	{
		// top paste pad is on this layer 
		if( ps->top_paste.shape != PAD_DEFAULT )
			p = &ps->top_paste;
		else
		{
			bUseDefault = TRUE;
			p = &ps->top;
		}
	}
	else if( (layer == LAY_TOP_COPPER && part->side == 1 )
			|| (layer == LAY_BOTTOM_COPPER && part->side == 0 ) ) 
	{
		// bottom copper pad is on this layer
		p = &ps->bottom;
	}
	else if( (layer == LAY_MASK_TOP && part->side == 1 )
		|| (layer == LAY_MASK_BOTTOM && part->side == 0 ) ) 
	{
		// bottom mask pad is on this layer 
		if( ps->bottom_mask.shape != PAD_DEFAULT )
			p = &ps->bottom_mask;
		else
		{
			bUseDefault = TRUE;
			p = &ps->bottom;
		}
	}
	else if( (layer == LAY_PASTE_TOP && part->side == 1 )
		|| (layer == LAY_PASTE_BOTTOM && part->side == 0 ) ) 
	{
		// bottom paste pad is on this layer 
		if( ps->bottom_paste.shape != PAD_DEFAULT )
			p = &ps->bottom_paste;
		else
		{
			bUseDefault = TRUE;
			p = &ps->bottom;
		}
	}
	else if( layer > LAY_BOTTOM_COPPER && ps->hole_size > 0 )
	{
		// inner pad is on this layer
		p = &ps->inner;
	}

	// now set parameters for return
	if( p )
		connect_flag = p->connect_flag;
	if( p == NULL )
	{
		// no pad definition, return defaults
	}
	else if( p->shape == PAD_NONE && ps->hole_size == 0 )
	{
		// no hole, no pad, return defaults
	}
	else if( p->shape == PAD_NONE )
	{
		// hole, no pad
		ret_code = 1;
		if( connect_status > ON_NET )
		{
			// connected to copper area or trace
			// make annular ring
			ret_code = 1;
			ttype = PAD_ROUND;
			ww = 2*m_annular_ring + hole_size;
		}
		else if( ( layer == LAY_MASK_TOP || layer == LAY_MASK_BOTTOM ) && bUseDefault )
		{
			// if solder mask layer and no mask pad defined, treat hole as pad to get clearance
			ret_code = 1;
			ttype = PAD_ROUND;
			ww = hole_size;
		}
	}
	else if( p->shape != PAD_NONE )
	{
		// normal pad
		ret_code = 1;
		ttype = p->shape;
		ww = p->size_h;
		ll = 2*p->size_l;
		rr = p->radius;
	}
	else
		ASSERT(0);	// error

	// adjust mask and paste pads if necessary
	if( (layer == LAY_MASK_TOP || layer == LAY_MASK_BOTTOM) && bUseDefault )
	{
		ww += 2*solder_mask_swell;
		ll += 2*solder_mask_swell;
		rr += solder_mask_swell;
	}
	else if( (layer == LAY_PASTE_TOP || layer == LAY_PASTE_BOTTOM) && bUseDefault )
	{
		if( ps->hole_size == 0 )
		{
			ww -= 2*paste_mask_shrink;
			ll -= 2*paste_mask_shrink;
			rr -= paste_mask_shrink;
			if( rr < 0 )
				rr = 0;
			if( ww <= 0 || ll <= 0 )
			{
				ww = 0;
				ll = 0;
				ret_code = 0;
			}
		}
		else
		{
			ww = ll = 0;	// no paste for through-hole pins
			ret_code = 0;
		}
	}

	// if copper layer connection, decide on thermal
	if( layer >= LAY_TOP_COPPER && (connect_status & AREA_CONNECT) )
	{
		// copper area connection, thermal or not?
		if( p->connect_flag == PAD_CONNECT_NEVER )
			ASSERT(0);	// shouldn't happen, this is an error by GetPinConnectionStatus(...)
		else if( p->connect_flag == PAD_CONNECT_NOTHERMAL )
			clear_type = CLEAR_NONE;
		else if( p->connect_flag == PAD_CONNECT_THERMAL )
			clear_type = CLEAR_THERMAL;
		else if( p->connect_flag == PAD_CONNECT_DEFAULT )
		{
			if( bUse_TH_thermals && ps->hole_size )
				clear_type = CLEAR_THERMAL;
			else if( bUse_SMT_thermals && !ps->hole_size )
				clear_type = CLEAR_THERMAL;
			else
				clear_type = CLEAR_NONE;
		}
		else
			ASSERT(0);
	}
	if( x )
		*x = xx;
	if( y )
		*y = yy;
	if( type )
		*type = ttype;
	if( w )
		*w = ww;
	if( l )
		*l = ll;
	if( r )
		*r = rr;
	if( hole )
		*hole = hole_size;
	if( angle )
		*angle = aangle;
	if( connection_status )
		*connection_status = connect_status;
	if( net )
		*net = nnet;
	if( pad_connect_flag )
		*pad_connect_flag = connect_flag;
	if( clearance_type )
		*clearance_type = clear_type;
	return ret_code;
#else
	return 0;
#endif
}

// Design rule check
//
void CPartList::DRC( CDlgLog * log, int copper_layers, 
					int units, BOOL check_unrouted,
					CArray<CPolyLine> * board_outline,
					DesignRules * dr, DRErrorList * drelist )
{
#ifndef CPT2
	CString d_str, x_str, y_str;
	CString str;
	CString str2;
	long nerrors = 0;

	// iterate through parts, setting DRC params and
	// checking for following errors:
	//	RING_PAD	
	//	BOARDEDGE_PAD
	//	BOARDEDGE_PADHOLE
	//	COPPERGRAPHIC_PAD
	//	COPPERGRAPHIC_PADHOLE
	str.LoadString( IDS_CheckingParts );
	if( log )
		log->AddLine( str );
	cpart * part = GetFirstPart();
	while( part )
	{
		CShape * s = part->shape;
		if( s )
		{
			// set DRC params for part
			// i.e. bounding rectangle and layer mask
			part->hole_flag = FALSE;
			part->min_x = INT_MAX;
			part->max_x = INT_MIN;
			part->min_y = INT_MAX;
			part->max_y = INT_MIN;
			part->layers = 0;

			// iterate through copper graphics elements
			for( int igr=0; igr<part->m_outline_stroke.GetSize(); igr++ )
			{
				stroke * stk = &part->m_outline_stroke[igr];
				int il = stk->layer - LAY_TOP_COPPER;
				if( il >= 0 )
				{
					int layer_bit = 1<<il;
					part->layers |= layer_bit;
					part->min_x = min( part->min_x, stk->xi - stk->w/2 );
					part->max_x = max( part->max_x, stk->xi + stk->w/2 );
					part->min_x = min( part->min_x, stk->xf - stk->w/2 );
					part->max_x = max( part->max_x, stk->xf + stk->w/2 );
					part->min_y = min( part->min_y, stk->yi - stk->w/2 );
					part->max_y = max( part->max_y, stk->yi + stk->w/2 );
					part->min_y = min( part->min_y, stk->yf - stk->w/2 );
					part->max_y = max( part->max_y, stk->yf + stk->w/2 );
				}
			}

			// iterate through pins in part, set DRC info for each pin
			for( int ip=0; ip<s->GetNumPins(); ip++ )
			{
				drc_pin * drp = &part->pin[ip].drc;
				drp->hole_size = 0;
				drp->min_x = INT_MAX;
				drp->max_x = INT_MIN;
				drp->min_y = INT_MAX;
				drp->max_y = INT_MIN;
				drp->max_r = INT_MIN;
				drp->layers = 0;

				id id1 = part->m_id;
				id1.SetT2( ID_PAD );
				id1.SetI2( ip );

				// iterate through copper layers
				for( int il=0; il<copper_layers; il++ )
				{
					int layer = LAY_TOP_COPPER + il;
					int layer_bit = 1<<il;

					// get test pad info
					int x, y, w, l, r, type, hole, connect, angle;
					cnet * net;
					BOOL bPad = GetPadDrawInfo( part, ip, layer, 0, 0, 0, 0,
						&type, &x, &y, &w, &l, &r, &hole, &angle,
						&net, &connect );
					if( bPad )
					{
						// pad or hole present
						if( hole )
						{
							drp->hole_size = hole;
							drp->min_x = min( drp->min_x, x - hole/2 );
							drp->max_x = max( drp->max_x, x + hole/2 );
							drp->min_y = min( drp->min_y, y - hole/2 );
							drp->max_y = max( drp->max_y, y + hole/2 );
							drp->max_r = max( drp->max_r, hole/2 );
							part->min_x = min( part->min_x, x - hole/2 );
							part->max_x = max( part->max_x, x + hole/2 );
							part->min_y = min( part->min_y, y - hole/2 );
							part->max_y = max( part->max_y, y + hole/2 );
							part->hole_flag = TRUE;
							// test clearance to board edge
							for( int ib=0; ib<board_outline->GetSize(); ib++ )
							{
								CPolyLine * b = &(*board_outline)[ib];
								for( int ibc=0; ibc<b->NumCorners(); ibc++ )
								{
									int x1 = b->X(ibc);
									int y1 = b->Y(ibc);
									int x2 = b->X(0);
									int y2 = b->Y(0);
									if( ibc != b->NumCorners()-1 )
									{
										x2 = b->X(ibc+1);
										y2 = b->Y(ibc+1);
									}
									// for now, only works for straight board edge segments
									if( b->SideStyle(ibc) == CPolyLine::STRAIGHT )
									{
										int d = ::GetClearanceBetweenLineSegmentAndPad( x1, y1, x2, y2, 0,
											PAD_ROUND, x, y, hole, 0, 0, 0 );
										if( d < dr->board_edge_copper )
										{
											// BOARDEDGE_PADHOLE error
											::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
											::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
											::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
											CString s0 ((LPCSTR) IDS_PadHoleToBoardEdgeEquals);
											str.Format( s0, nerrors+1, part->ref_des, s->m_padstack[ip].name, d_str, x_str, y_str );
											DRError * dre = drelist->Add( nerrors, DRError::BOARDEDGE_PADHOLE, &str,
												&part->ref_des, NULL, id1, id1, x, y, 0, 0, w+20*NM_PER_MIL, 0 );
											if( dre )
											{
												nerrors++;
												if( log )
													log->AddLine( str );
											}
										}
									}
								}
							}
							// test clearance from graphic elements
							for( int igr=0; igr<part->m_outline_stroke.GetSize(); igr++ )
							{
								stroke * stk = &part->m_outline_stroke[igr];
								if( stk->layer == layer )
								{
									int d = ::GetClearanceBetweenLineSegmentAndPad( stk->xi, stk->yi, stk->xf, stk->yf, stk->w,
										PAD_ROUND, x, y, hole, 0, 0, 0 );
									if( d <= dr->hole_copper )
									{
										// COPPERGRAPHIC_PADHOLE error
										::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
										CString s0 ((LPCSTR) IDS_PadHoleToCopperGraphics);
										str.Format( s0, nerrors+1, part->ref_des, s->m_padstack[ip].name, 
											part->ref_des, s->m_padstack[ip].name, d_str, x_str, y_str );
										DRError * dre = drelist->Add( nerrors, DRError::COPPERGRAPHIC_PADHOLE, &str,
											&part->ref_des, NULL, id1, id1, x, y, 0, 0, w+20*NM_PER_MIL, 0 );
										if( dre )
										{
											nerrors++;
											if( log )
												log->AddLine( str );
										}
									}
								}
							}
						}
						if( type != PAD_NONE )
						{
							int wid = w;
							int len = wid;
							if( type == PAD_RECT || type == PAD_RRECT || type == PAD_OVAL )
								len = l;
							if( angle == 90 )
							{
								wid = len;
								len = w;
							}
							drp->min_x = min( drp->min_x, x - len/2 );
							drp->max_x = max( drp->max_x, x + len/2 );
							drp->min_y = min( drp->min_y, y - wid/2 );
							drp->max_y = max( drp->max_y, y + wid/2 );
							drp->max_r = max( drp->max_r, Distance( 0, 0, len/2, wid/2 ) );
							part->min_x = min( part->min_x, x - len/2 );
							part->max_x = max( part->max_x, x + len/2 );
							part->min_y = min( part->min_y, y - wid/2 );
							part->max_y = max( part->max_y, y + wid/2 );
							drp->layers |= layer_bit;
							part->layers |= layer_bit;
							if( hole && part->pin[ip].net )
							{
								// test annular ring
								int d = (w - hole)/2;
								if( type == PAD_RECT || type == PAD_RRECT || type == PAD_OVAL )
									d = (min(w,l) - hole)/2;
								if( d < dr->annular_ring_pins )
								{
									// RING_PAD
									::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
									::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
									::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
									CString s0 ((LPCSTR) IDS_AnnularRingEquals);
									str.Format( s0, nerrors+1, part->ref_des, s->m_padstack[ip].name, d_str, x_str, y_str );
									DRError * dre = drelist->Add( nerrors, DRError::RING_PAD, &str,
										&part->ref_des, NULL, id1, id1, x, y, 0, 0, w+20*NM_PER_MIL, 0 );
									if( dre )
									{
										nerrors++;
										if( log )
											log->AddLine( str );
									}
								}
							}
							// test clearance to board edge
							for( int ib=0; ib<board_outline->GetSize(); ib++ )
							{
								CPolyLine * b = &(*board_outline)[ib];
								for( int ibc=0; ibc<b->NumCorners(); ibc++ )
								{
									int x1 = b->X(ibc);
									int y1 = b->Y(ibc);
									int x2 = b->X(0);
									int y2 = b->Y(0);
									if( ibc != b->NumCorners()-1 )
									{
										x2 = b->X(ibc+1);
										y2 = b->Y(ibc+1);
									}
									// for now, only works for straight board edge segments
									if( b->SideStyle(ibc) == CPolyLine::STRAIGHT )
									{
										int d = ::GetClearanceBetweenLineSegmentAndPad( x1, y1, x2, y2, 0,
											type, x, y, w, l, r, angle );
										if( d < dr->board_edge_copper )
										{
											// BOARDEDGE_PAD error
											::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
											::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
											::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
											CString s0 ((LPCSTR) IDS_PadToBoardEdgeEquals);
											str.Format( s0, nerrors+1, part->ref_des, s->m_padstack[ip].name, d_str, x_str, y_str );
											DRError * dre = drelist->Add( nerrors, DRError::BOARDEDGE_PAD, &str,
												&part->ref_des, NULL, id1, id1, x, y, 0, 0, w+20*NM_PER_MIL, 0 );
											if( dre )
											{
												nerrors++;
												if( log )
													log->AddLine( str );
											}
										}
									}
								}
							}
							// test clearance from graphic elements
							for( int igr=0; igr<part->m_outline_stroke.GetSize(); igr++ )
							{
								stroke * stk = &part->m_outline_stroke[igr];
								if( stk->layer == layer )
								{
									int d = ::GetClearanceBetweenLineSegmentAndPad( stk->xi, stk->yi, stk->xf, stk->yf, stk->w,
										type, x, y, w, l, r, angle );
									if( d <= dr->copper_copper )
									{
										// COPPERGRAPHIC_PAD error
										::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
										CString s0 ((LPCSTR) IDS_PadToCopperGraphics);
										str.Format( s0, nerrors+1, part->ref_des, s->m_padstack[ip].name, 
											part->ref_des, s->m_padstack[ip].name, d_str, x_str, y_str );
										DRError * dre = drelist->Add( nerrors, DRError::COPPERGRAPHIC_PAD, &str,
											&part->ref_des, NULL, id1, id1, x, y, 0, 0, w+20*NM_PER_MIL, 0 );
										if( dre )
										{
											nerrors++;
											if( log )
												log->AddLine( str );
										}
									}
								}
							}
						}
					}
				}
			}
		}
		part = GetNextPart( part );
	}

	// iterate through parts again, checking against all other parts
	// check for following errors:
	//	PADHOLE_PADHOLE
	//	PAD_PADHOLE
	//	PAD_PAD
	//	COPPERGRAPHIC_PAD,			
	//	COPPERGRAPHIC_PADHOLE,
	int t_part_index = -1;	// use index to avoid duplicate tests
	for( cpart * t_part=GetFirstPart(); t_part; t_part=GetNextPart(t_part) )
	{
		t_part_index++;		
		CShape * t_s = t_part->shape;
		if( t_s )
		{
			// now iterate through parts that follow in the partlist
			int part_index = -1;
			for( cpart * part=GetFirstPart(); part; part=GetNextPart(part) )
			{
				part_index++;
				CShape * s = part->shape;
				if( s )
				{
					// now see if part and t_part elements might intersect
					// get max. clearance violation
					int clr = max( dr->pad_pad, dr->hole_copper );
					clr = max( clr, dr->hole_hole );
					clr = max( clr, dr->copper_copper );

					// now check for clearance of rectangles
					if( part->min_x - t_part->max_x > clr )
						continue;	// next part
					if( t_part->min_x - part->max_x > clr )
						continue;	// next part
					if( part->min_y - t_part->max_y > clr )
						continue;	// next part
					if( t_part->min_y - part->max_y > clr )
						continue;	// next part

					// see if elements on same layers
					if( !(part->layers & t_part->layers) )
					{
						// no elements on same layers, check for holes
						if( !part->hole_flag && !t_part->hole_flag ) 
							continue;	// no, go to next part
					}

					// we need to test elements in these parts
					// iterate through pins in t_part
					for( int t_ip=0; t_ip<t_s->GetNumPins(); t_ip++ )
					{
						cpadstack * t_ps = &t_s->m_padstack[t_ip];
						part_pin * t_pin = &t_part->pin[t_ip];
						drc_pin * t_drp = &t_pin->drc;
						id id1 = part->m_id;
						id1.SetT2( ID_PAD );
						id1.SetI2( t_ip );

						// iterate through copper graphic elements in part
						// first, get mask of all copper layers used by elements
						int graph_lay_mask = 0;
						for( int igr=0; igr<part->m_outline_stroke.GetSize(); igr++ )
						{
							stroke * stk = &part->m_outline_stroke[igr];
							int il = stk->layer - LAY_TOP_COPPER;
							if( il >= 0 )
							{
								int lay_mask = 1<<il;
								graph_lay_mask |= lay_mask;
							}
						}
						if( graph_lay_mask != 0 )
						{
							// iterate through all copper layers
							for( int il=0; il<copper_layers; il++ )
							{
								int lay_mask = 1<<il;
								if( lay_mask & graph_lay_mask )
								{
									// there are graphics strokes on this layer
									int layer = il + LAY_TOP_COPPER;
									// check pads
									int t_pad_x, t_pad_y, t_pad_w, t_pad_l, t_pad_r;
									int t_pad_type, t_pad_hole, t_pad_connect, t_pad_angle;
									cnet * t_pad_net;
									BOOL t_bPad = GetPadDrawInfo( t_part, t_ip, layer, 0, 0, 0, 0,
										&t_pad_type, &t_pad_x, &t_pad_y, &t_pad_w, &t_pad_l, &t_pad_r, 
										&t_pad_hole, &t_pad_angle,
										&t_pad_net, &t_pad_connect );
									if( t_bPad )
									{
										for( int igr=0; igr<part->m_outline_stroke.GetSize(); igr++ )
										{
											stroke * stk = &part->m_outline_stroke[igr];
											if( stk->layer == layer )
											{
												//** TODO: test for hole-copper violation
												//** TODO: test for pad-copper violation
											}
										}
									}
								}
							}
						}

						// if part follows t_part in list, iterate through pins in part
						if( part_index > t_part_index )
						{
							for( int ip=0; ip<s->GetNumPins(); ip++ )
							{
								cpadstack * ps = &s->m_padstack[ip];
								part_pin * pin = &part->pin[ip];
								drc_pin * drp = &pin->drc;
								id id2 = part->m_id;
								id2.SetT2( ID_PAD );
								id2.SetI2( ip );

								// test for hole-hole violation
								if( drp->hole_size && t_drp->hole_size )
								{
									// test for hole-to-hole violation
									int dist = Distance( pin->x, pin->y, t_pin->x, t_pin->y );
									int h_h = max( 0, dist - (ps->hole_size + t_ps->hole_size)/2 );
									if( h_h < dr->hole_hole )
									{
										// PADHOLE_PADHOLE
										::MakeCStringFromDimension( &d_str, h_h, units, TRUE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &x_str, pin->x, units, FALSE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &y_str, pin->y, units, FALSE, TRUE, TRUE, 1 );
										CString s0 ((LPCSTR) IDS_PadHoleToPadeHoleEquals);
										str.Format( s0, nerrors+1, part->ref_des, s->m_padstack[ip].name,
											t_part->ref_des, t_s->m_padstack[t_ip].name,
											d_str, x_str, y_str );
										DRError * dre = drelist->Add( nerrors, DRError::PADHOLE_PADHOLE, &str,
											&t_part->ref_des, &part->ref_des, id1, id2, 
											pin->x, pin->y, t_pin->x, t_pin->y, 0, 0 );
										if( dre )
										{
											nerrors++;
											if( log )
												log->AddLine( str );
										}
									}
								}

								// see if pads on same layers
								if( !(drp->layers & t_drp->layers) )
								{
									// no, see if either has a hole
									if( !drp->hole_size && !t_drp->hole_size )
									{
										// no, go to next pin
										continue;
									}
								}

								// see if padstacks might intersect
								if( drp->min_x - t_drp->max_x > clr )
									continue;	// no, next pin
								if( t_drp->min_x - drp->max_x > clr )
									continue;	// no, next pin
								if( drp->min_y - t_drp->max_y > clr )
									continue;	// no, next pin
								if( t_drp->min_y - drp->max_y > clr )
									continue;	// no, next pin

								// OK, pads might be too close
								// check for pad clearance violations on each layer
								for( int il=0; il<copper_layers; il++ )
								{
									int layer = il + LAY_TOP_COPPER;
									int t_pad_x, t_pad_y, t_pad_w, t_pad_l, t_pad_r;
									int t_pad_type, t_pad_hole, t_pad_connect, t_pad_angle;
									cnet * t_pad_net;

									// test for pad-pad violation
									BOOL t_bPad = GetPadDrawInfo( t_part, t_ip, layer, 0, 0, 0, 0,
										&t_pad_type, &t_pad_x, &t_pad_y, &t_pad_w, &t_pad_l, &t_pad_r, 
										&t_pad_hole, &t_pad_angle,
										&t_pad_net, &t_pad_connect );
									if( t_bPad )
									{
										// get pad info for pin
										int pad_x, pad_y, pad_w, pad_l, pad_r;
										int pad_type, pad_hole, pad_connect, pad_angle;
										cnet * pad_net;
										BOOL bPad = GetPadDrawInfo( part, ip, layer, 0, 0, 0, 0, 
											&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r, 
											&pad_hole, &pad_angle, &pad_net, &pad_connect );
										if( bPad )
										{
											if( pad_hole )
											{
												// test for pad-padhole violation
												int dist = GetClearanceBetweenPads( t_pad_type, t_pad_x, t_pad_y, 
													t_pad_w, t_pad_l, t_pad_r, t_pad_angle,
													PAD_ROUND, pad_x, pad_y, pad_hole, 0, 0, 0 );
												if( dist < dr->hole_copper )
												{
													// PAD_PADHOLE 
													::MakeCStringFromDimension( &d_str, dist, units, TRUE, TRUE, TRUE, 1 );
													::MakeCStringFromDimension( &x_str, pad_x, units, FALSE, TRUE, TRUE, 1 );
													::MakeCStringFromDimension( &y_str, pad_y, units, FALSE, TRUE, TRUE, 1 );
													CString s0 ((LPCSTR) IDS_PadHoleToPadEquals);
													str.Format( s0, nerrors+1, part->ref_des, s->m_padstack[ip].name,
														t_part->ref_des, t_s->m_padstack[t_ip].name,
														d_str, x_str, y_str );
													DRError * dre = drelist->Add( nerrors, DRError::PAD_PADHOLE, &str, 
														&t_part->ref_des, &part->ref_des, id1, id2, 
														pad_x, pad_y, t_pad_x, t_pad_y, 0, layer );
													if( dre )
													{
														nerrors++;
														if( log )
															log->AddLine( str );
													}
													break;		// skip any more layers, go to next pin
												}
											}
											// test for pad-pad violation
											int dist = GetClearanceBetweenPads( t_pad_type, t_pad_x, t_pad_y, 
												t_pad_w, t_pad_l, t_pad_r, t_pad_angle,
												pad_type, pad_x, pad_y, pad_w, pad_l, pad_r, pad_angle );
											if( dist < dr->pad_pad )
											{
												// PAD_PAD 
												::MakeCStringFromDimension( &d_str, dist, units, TRUE, TRUE, TRUE, 1 );
												::MakeCStringFromDimension( &x_str, pad_x, units, FALSE, TRUE, TRUE, 1 );
												::MakeCStringFromDimension( &y_str, pad_y, units, FALSE, TRUE, TRUE, 1 );
												CString s0 ((LPCSTR) IDS_PadToPadEquals);
												str.Format( s0, nerrors+1, part->ref_des, s->m_padstack[ip].name,
													t_part->ref_des, t_s->m_padstack[t_ip].name,
													d_str, x_str, y_str );
												DRError * dre = drelist->Add( nerrors, DRError::PAD_PAD, &str, 
													&t_part->ref_des, &part->ref_des, id1, id2, 
													pad_x, pad_y, t_pad_x, t_pad_y, 0, layer );
												if( dre )
												{
													nerrors++;
													if( log )
														log->AddLine( str );
												}
												break;		// skip any more layers, go to next pin
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// iterate through all nets
	// check for following errors:
	//	BOARDEDGE_COPPERAREA
	//	TRACE_WIDTH
	//	BOARDEDGE_SEG
	//	RING_VIA
	//	BOARDEDGE_VIA
	//	BOARDEDGE_VIAHOLE
	//	SEG_PAD
	//	VIA_PAD
	//  SEG_COPPERGRAPHIC
	//  VIA_COPPERGRAPHIC
	str.LoadStringA( IDS_CheckingNetsAndParts );
	if( log )
		log->AddLine( str );
	POSITION pos;
	void * ptr;
	CString name;
	for( pos = m_nlist->m_map.GetStartPosition(); pos != NULL; )
	{
		m_nlist->m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		// iterate through copper areas, checking for BOARDEDGE_COPPERAREA errors
		for( int ia=0; ia<net->NumAreas(); ia++ )
		{
			carea * a = &net->area[ia];
			// iterate through contours
			for( int icont=0; icont<a->NumContours(); icont++ )
			{
				// iterate through corners and sides
				int istart = a->ContourStart(icont);
				int iend = a->ContourEnd(icont);
				for( int ic=istart; ic<=iend; ic++ )
				{
					id id_a = net->m_id;
					id_a.SetT2( ID_AREA );
					id_a.SetI2( ia );
					id_a.SetT3( ID_SIDE );
					id_a.SetI3( ic );
					int x1 = a->X(ic);
					int y1 = a->Y(ic);
					int x2, y2;
					if( ic < iend )
					{
						x2 = a->X(ic+1);
						y2 = a->Y(ic+1);
					}
					else
					{
						x2 = a->X(istart);
						y2 = a->Y(istart);
					}
					int style = a->SideStyle(ic);

					// test clearance to board edge
					// iterate through board outlines
					for( int ib=0; ib<board_outline->GetSize(); ib++ )
					{
						CPolyLine * b = &(*board_outline)[ib];
						// iterate through sides
						for( int ibc=0; ibc<b->NumCorners(); ibc++ )
						{
							int bx1 = b->X(ibc);
							int by1 = b->Y(ibc);
							int bx2 = b->X(0);
							int by2 = b->Y(0);
							if( ibc != b->NumCorners()-1 )
							{
								bx2 = b->X(ibc+1);
								by2 = b->Y(ibc+1);
							}
							int bstyle = b->SideStyle(ibc);
							int x, y;
							int d = ::GetClearanceBetweenSegments( bx1, by1, bx2, by2, bstyle, 0,
								x1, y1, x2, y2, style, 0, dr->board_edge_copper, &x, &y );
							if( d < dr->board_edge_copper )
							{
								// BOARDEDGE_COPPERAREA error
								::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
								::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
								::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
								CString s ((LPCSTR) IDS_CopperAreaToBoardEdgeEquals);
								str.Format( s, nerrors+1, net->name, d_str, x_str, y_str );
								DRError * dre = drelist->Add( nerrors, DRError::BOARDEDGE_COPPERAREA, &str,
									&net->name, NULL, id_a, id_a, x, y, 0, 0, 0, 0 );
								if( dre )
								{
									nerrors++;
									if( log )
										log->AddLine( str );
								}
							}
						}
					}
				}
			}
		}
		// iterate through all connections
		CIterator_cconnect iter_con(net);
		for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
		{
			int ic = iter_con.GetIndex();
			// get DRC info for this connection
			// iterate through all segments and vertices
			c->min_x = INT_MAX;		// bounding box for connection
			c->max_x = INT_MIN;
			c->min_y = INT_MAX;
			c->max_y = INT_MIN;
			c->vias_present = FALSE;
			c->seg_layers = 0;
			int max_trace_w = 0;	// maximum trace width for connection
			CIterator_cseg iter_seg(c);
			for( cseg * s=iter_seg.GetFirst(); s; s=iter_seg.GetNext() )
			{
				int is = iter_seg.GetIndex();
				id id_seg = net->m_id;
				id_seg.SetT2( ID_CONNECT );
				id_seg.SetI2( ic );
				id_seg.SetT3( ID_SEG );
				id_seg.SetI3( is );
				int x1 = s->GetPreVtx().x;
				int y1 = s->GetPreVtx().y;
				int x2 = s->GetPostVtx().x;
				int y2 = s->GetPostVtx().y;
				int w = s->m_width;
				int layer = s->m_layer;
				if( s->m_layer >= LAY_TOP_COPPER )
				{
					int layer_bit = s->m_layer - LAY_TOP_COPPER;
					c->seg_layers |= 1<<layer_bit;
				}
				// add segment to bounding box
				int seg_min_x = min( x1, x2 );
				int seg_min_y = min( y1, y2 );
				int seg_max_x = max( x1, x2 );
				int seg_max_y = max( y1, y2 );
				c->min_x = min( c->min_x, seg_min_x - w/2 );
				c->max_x = max( c->max_x, seg_max_x + w/2 );
				c->min_y = min( c->min_y, seg_min_y - w/2 );
				c->max_y = max( c->max_y, seg_max_y + w/2 );
				// test trace width
				// CPT: noticed a dre for a ratline with width 1 nm (not sure how that width got set...)  Anyway, no width errors for ratlines:
				if( w > 0 && w < dr->trace_width && c->SegByIndex(is).m_layer!=LAY_RAT_LINE)		
				{
					// TRACE_WIDTH error
					int x = (x1+x2)/2;
					int y = (y1+y2)/2;
					::MakeCStringFromDimension( &d_str, w, units, TRUE, TRUE, TRUE, 1 );
					::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
					::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
					CString s ((LPCSTR) IDS_TraceWidthEquals);
					str.Format( s, nerrors+1, net->name, d_str, x_str, y_str );
					DRError * dre = drelist->Add( nerrors, DRError::TRACE_WIDTH, &str, 
						&net->name, NULL, id_seg, id_seg, x, y, 0, 0, 0, layer );
					if( dre )
					{
						nerrors++;
						if( log )
							log->AddLine( str );
					}
				}
				// test clearance to board edge
				if( w > 0 )
				{
					for( int ib=0; ib<board_outline->GetSize(); ib++ )
					{
						CPolyLine * b = &(*board_outline)[ib];
						for( int ibc=0; ibc<b->NumCorners(); ibc++ )
						{
							int bx1 = b->X(ibc);
							int by1 = b->Y(ibc);
							int bx2 = b->X(0);
							int by2 = b->Y(0);
							if( ibc != b->NumCorners()-1 )
							{
								bx2 = b->X(ibc+1);
								by2 = b->Y(ibc+1);
							}
							int bstyle = b->SideStyle(ibc);
							int x, y;
							int d = ::GetClearanceBetweenSegments( bx1, by1, bx2, by2, bstyle, 0,
								x1, y1, x2, y2, CPolyLine::STRAIGHT, w, dr->board_edge_copper, &x, &y );
							if( d < dr->board_edge_copper )
							{
								// BOARDEDGE_SEG error
								::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
								::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
								::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
								CString s ((LPCSTR) IDS_TraceToBoardEdgeEquals);
								str.Format( s, nerrors+1, net->name, d_str, x_str, y_str );
								DRError * dre = drelist->Add( nerrors, DRError::BOARDEDGE_SEG, &str,
									&net->name, NULL, id_seg, id_seg, x, y, 0, 0, 0, layer );
								if( dre )
								{
									nerrors++;
									if( log )
										log->AddLine( str );
								}
							}
						}
					}
				}
			}
			for( int iv=0; iv<c->NumSegs()+1; iv++ )
			{
				cvertex * vtx = &c->VtxByIndex(iv);
				if( vtx->via_w )
				{
					// via present
					id id_via = net->m_id;
					id_via.SetT2( ID_CONNECT );
					id_via.SetI2( ic );
					id_via.SetT3( ID_VIA );
					id_via.SetI3( iv );
					c->vias_present = TRUE;
					int min_via_w = INT_MAX;	// minimum via pad diameter
					int max_via_w = INT_MIN;	// maximum via_pad diameter
					for( int il=0; il<copper_layers; il++ )
					{
						int layer = il + LAY_TOP_COPPER;
						int test;
						int pad_w;
						int hole_w;
						m_nlist->GetViaPadInfo( net, ic, iv, layer, 
							&pad_w, &hole_w, &test );
						if( pad_w > 0 )
							min_via_w = min( min_via_w, pad_w );
						max_via_w = max( max_via_w, pad_w );
					}
					if( max_via_w == 0 )
						ASSERT(0);
					c->min_x = min( c->min_x, vtx->x - max_via_w/2 );
					c->max_x = max( c->max_x, vtx->x + max_via_w/2 );
					c->min_y = min( c->min_y, vtx->y - max_via_w/2 );
					c->max_y = max( c->max_y, vtx->y + max_via_w/2 );
					int d = (min_via_w - vtx->via_hole_w)/2;
					if( d < dr->annular_ring_vias )
					{
						// RING_VIA
						::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
						::MakeCStringFromDimension( &x_str, vtx->x, units, FALSE, TRUE, TRUE, 1 );
						::MakeCStringFromDimension( &y_str, vtx->y, units, FALSE, TRUE, TRUE, 1 );
						CString s ((LPCSTR) IDS_ViaAnnularRingEquals);
						str.Format( s, nerrors+1, net->name, d_str, x_str, y_str );
						DRError * dre = drelist->Add( nerrors, DRError::RING_VIA, &str, 
							&net->name, NULL, id_via, id_via, vtx->x, vtx->y, 0, 0, vtx->via_w+20*NM_PER_MIL, 0 );
						if( dre )
						{
							nerrors++;
							if( log )
								log->AddLine( str );
						}
					}
					// test clearance to board edge
					for( int ib=0; ib<board_outline->GetSize(); ib++ )
					{
						CPolyLine * b = &(*board_outline)[ib];
						for( int ibc=0; ibc<b->NumCorners(); ibc++ )
						{
							int bx1 = b->X(ibc);
							int by1 = b->Y(ibc);
							int bx2 = b->X(0);
							int by2 = b->Y(0);
							if( ibc != b->NumCorners()-1 )
							{
								bx2 = b->X(ibc+1);
								by2 = b->Y(ibc+1);
							}
							//** for now, only works for straight board edge segments
							if( b->SideStyle(ibc) == CPolyLine::STRAIGHT )
							{
								int d = ::GetClearanceBetweenLineSegmentAndPad( bx1, by1, bx2, by2, 0,
									PAD_ROUND, vtx->x, vtx->y, max_via_w, 0, 0, 0 );
								if( d < dr->board_edge_copper )
								{
									// BOARDEDGE_VIA error
									::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
									::MakeCStringFromDimension( &x_str, vtx->x, units, FALSE, TRUE, TRUE, 1 );
									::MakeCStringFromDimension( &y_str, vtx->y, units, FALSE, TRUE, TRUE, 1 );
									CString s ((LPCSTR) IDS_ViaToBoardEdgeEquals);
									str.Format( s, nerrors+1, net->name, d_str, x_str, y_str );
									DRError * dre = drelist->Add( nerrors, DRError::BOARDEDGE_VIA, &str,
										&net->name, NULL, id_via, id_via, vtx->x, vtx->y, 0, 0, vtx->via_w+20*NM_PER_MIL, 0 );
									if( dre )
									{
										nerrors++;
										if( log )
											log->AddLine( str );
									}
								}
								int dh = ::GetClearanceBetweenLineSegmentAndPad( bx1, by1, bx2, by2, 0,
									PAD_ROUND, vtx->x, vtx->y, vtx->via_hole_w, 0, 0, 0 );
								if( dh < dr->board_edge_hole )
								{
									// BOARDEDGE_VIAHOLE error
									::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
									::MakeCStringFromDimension( &x_str, vtx->x, units, FALSE, TRUE, TRUE, 1 );
									::MakeCStringFromDimension( &y_str, vtx->y, units, FALSE, TRUE, TRUE, 1 );
									CString s ((LPCSTR) IDS_ViaHoleToBoardEdgeEquals);
									str.Format( s, nerrors+1, net->name, d_str, x_str, y_str );
									DRError * dre = drelist->Add( nerrors, DRError::BOARDEDGE_VIAHOLE, &str,
										&net->name, NULL, id_via, id_via, vtx->x, vtx->y, 0, 0, vtx->via_w+20*NM_PER_MIL, 0 );
									if( dre )
									{
										nerrors++;
										if( log )
											log->AddLine( str );
									}
								}
							}
						}
					}
				}
			}
			// iterate through all parts, checking for
			//	SEG_PAD, VIA_PAD, SEG_COPPERGRAPHIC, VIA_COPPERGRAPHIC errors
			cpart * part = GetFirstPart();
			for( ; part; part = GetNextPart( part ) )
			{
				CShape * s = part->shape;

				// if not on same layers, can't conflict
				if( !part->hole_flag && !c->vias_present && !(part->layers & c->seg_layers) )
					continue;	// next part

				// if bounding boxes don't overlap, can't conflict
				if( part->min_x - c->max_x > dr->pad_trace )
					continue;	// next part
				if( c->min_x - part->max_x > dr->pad_trace )
					continue;	// next part
				if( part->min_y - c->max_y > dr->pad_trace )
					continue;	// next part
				if( c->min_y - part->max_y > dr->pad_trace )
					continue;	// next part

				// OK, now we have to test each pad
				for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
				{
					cpadstack * ps = &s->m_padstack[ip];
					part_pin * pin = &part->pin[ip];
					drc_pin * drp = &pin->drc;
					id id_pad = part->m_id;
					id_pad.SetT2( ID_PAD );
					id_pad.SetI2( ip );

					// if pin and connection bounds are separated enough, skip pin
					if( drp->min_x - c->max_x > dr->pad_trace )
						continue;	// no, next pin
					if( c->min_x - drp->max_x > dr->pad_trace )
						continue;	// no, next pin
					if( drp->min_y - c->max_y > dr->pad_trace )
						continue;	// no, next pin
					if( c->min_y - drp->max_y > dr->pad_trace )
						continue;	// no, next pin

					// possible clearance violation, now test pad against each segment and via on each layer
					int pad_x, pad_y, pad_w, pad_l, pad_r;
					int pad_type, pad_hole, pad_connect, pad_angle;
					cnet * pad_net;
					BOOL bPad;
					BOOL pin_info_valid = FALSE;
					int pin_info_layer = 0;

					CIterator_cseg iter_seg(c);
					for( cseg * s=iter_seg.GetFirst(); s; s=iter_seg.GetNext() )
					{
						int is = iter_seg.GetIndex();
						cvertex * pre_vtx = &s->GetPreVtx();
						cvertex * post_vtx = &s->GetPostVtx();
						cvertex * pre_vtx_test = &s->GetPreVtx();
						cvertex * post_vtx_test = &s->GetPostVtx();
						int w = s->m_width;
						int xi = pre_vtx->x;
						int yi = pre_vtx->y;
						int xf = post_vtx->x;
						int yf = post_vtx->y;
						int min_x = min( xi, xf ) - w/2;
						int max_x = max( xi, xf ) + w/2;
						int min_y = min( yi, yf ) - w/2;
						int max_y = max( yi, yf ) + w/2;
						// ids
						id id_seg = net->m_id;
						id_seg.SetT2( ID_CONNECT );
						id_seg.SetI2( ic );
						id_seg.SetT3( ID_SEG );
						id_seg.SetI3( is );
						id id_via = net->m_id;
						id_via.SetT2( ID_CONNECT );
						id_via.SetI2( ic );
						id_via.SetT3( ID_VIA );
						id_via.SetI3( is+1 );

						// check all layers
						for( int il=0; il<copper_layers; il++ )
						{
							int layer = il + LAY_TOP_COPPER;
							int layer_bit = 1<<il;

							if( s->m_layer == layer )
							{
								// check segment clearances
								cnet * pin_net = part->pin[ip].net;
								if( drp->hole_size && net != pin_net )
								{
									// pad has hole, check segment to pad_hole clearance
									if( !(pin_info_valid && layer == pin_info_layer) )
									{
										bPad = GetPadDrawInfo( part, ip, layer, 0, 0, 0, 0,
											&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r, 
											&pad_hole, &pad_angle, &pad_net, &pad_connect );
										pin_info_valid = TRUE;
										pin_info_layer = layer;
									}
									int d = GetClearanceBetweenLineSegmentAndPad( xi, yi, xf, yf, w,
										PAD_ROUND, pad_x, pad_y, pad_hole, 0, 0, 0 );
									if( d < dr->hole_copper ) 
									{
										// SEG_PADHOLE
										::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &x_str, pad_x, units, FALSE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &y_str, pad_y, units, FALSE, TRUE, TRUE, 1 );
										CString s ((LPCSTR) IDS_TraceToPadHoleEquals);
										str.Format( s, nerrors+1, net->name, part->ref_des, ps->name,
											d_str, x_str, y_str );
										DRError * dre = drelist->Add( nerrors, DRError::SEG_PAD, &str, 
											&net->name, &part->ref_des, id_seg, id_pad, pad_x, pad_y, pad_x, pad_y, 
											max(pad_w,pad_l)+20*NM_PER_MIL, layer );
										if( dre )
										{
											nerrors++;
											if( log )
												log->AddLine( str );
										}
									}
								}
								if( layer_bit & drp->layers )
								{
									// pad is on this layer
									// get pad info for pin if necessary
									if( !(pin_info_valid && layer == pin_info_layer) )
									{
										bPad = GetPadDrawInfo( part, ip, layer, 0, 0, 0, 0, 
											&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r,
											&pad_hole, &pad_angle, &pad_net, &pad_connect );
										pin_info_valid = TRUE;
										pin_info_layer = layer;
									}
									if( bPad && pad_type != PAD_NONE && net != pad_net )
									{
										// check segment to pad clearance
										int d = GetClearanceBetweenLineSegmentAndPad( xi, yi, xf, yf, w,
											pad_type, pad_x, pad_y, pad_w, pad_l, pad_r, pad_angle );
										if( d < dr->pad_trace ) 
										{
											// SEG_PAD
											::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
											::MakeCStringFromDimension( &x_str, pad_x, units, FALSE, TRUE, TRUE, 1 );
											::MakeCStringFromDimension( &y_str, pad_y, units, FALSE, TRUE, TRUE, 1 );
											CString s ((LPCSTR) IDS_TraceToPadEquals);
											str.Format( s, nerrors+1, net->name, part->ref_des, ps->name,
												d_str, x_str, y_str );
											DRError * dre = drelist->Add( nerrors, DRError::SEG_PAD, &str, 
												&net->name, &part->ref_des, id_seg, id_pad, pad_x, pad_y, pad_x, pad_y, 
												max(pad_w,pad_l)+20*NM_PER_MIL, layer );
											if( dre )
											{
												nerrors++;
												if( log )
													log->AddLine( str );
											}
										}
									}
								}
							}
							// get next via
							if( post_vtx->via_w )
							{
								// via exists
								int test;
								int via_w;
								int via_hole_w;
								m_nlist->GetViaPadInfo( net, ic, is+1, layer, 
									&via_w, &via_hole_w, &test );
								int w = 0;
								if( via_w )
								{
									// check via_pad to pin_pad clearance
									if( !(pin_info_valid && layer == pin_info_layer) )
									{
										bPad = GetPadDrawInfo( part, ip, layer, 0, 0, 0, 0, 
											&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r, 
											&pad_hole, &pad_angle, &pad_net, &pad_connect );
										pin_info_valid = TRUE;
										pin_info_layer = layer;
									}
									if( bPad && pad_type != PAD_NONE && pad_net != net )
									{
										int d = GetClearanceBetweenPads( PAD_ROUND, xf, yf, via_w, 0, 0, 0,
											pad_type, pad_x, pad_y, pad_w, pad_l, pad_r, pad_angle );
										if( d < dr->pad_trace )
										{
											// VIA_PAD
											::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
											::MakeCStringFromDimension( &x_str, pad_x, units, FALSE, TRUE, TRUE, 1 );
											::MakeCStringFromDimension( &y_str, pad_y, units, FALSE, TRUE, TRUE, 1 );
											CString s ((LPCSTR) IDS_ViaPadToPadEquals);
											str.Format( s, nerrors+1, net->name, part->ref_des, ps->name,
												d_str, x_str, y_str );
											DRError * dre = drelist->Add( nerrors, DRError::VIA_PAD, &str, 
												&net->name, &part->ref_des, id_via, id_pad, xf, yf, pad_x, pad_y, 0, layer );
											if( dre )
											{
												nerrors++;
												if( log )
													log->AddLine( str );
											}
											break;  // skip more layers
										}
									}
									if( drp->hole_size && pad_net != net )
									{
										// pin has a hole, check via_pad to pin_hole clearance
										int d = Distance( xf, yf, pin->x, pin->y );
										d = max( 0, d - drp->hole_size/2 - via_w/2 );
										if( d < dr->hole_copper )
										{
											// VIA_PADHOLE
											::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
											::MakeCStringFromDimension( &x_str, pad_x, units, FALSE, TRUE, TRUE, 1 );
											::MakeCStringFromDimension( &y_str, pad_y, units, FALSE, TRUE, TRUE, 1 );
											CString s ((LPCSTR) IDS_ViaPadToPadHoleEquals);
											str.Format( s, nerrors+1, net->name, part->ref_des, ps->name,
												d_str, x_str, y_str );
											DRError * dre = drelist->Add( nerrors, DRError::VIA_PAD, &str, 
												&net->name, &part->ref_des, id_via, id_pad, xf, yf, pad_x, pad_y, 0, layer );
											if( dre )
											{
												nerrors++;
												if( log )
													log->AddLine( str );
											}
											break;  // skip more layers
										}
									}
								}
								if( !(pin_info_valid && layer == pin_info_layer) )
								{
									bPad = GetPadDrawInfo( part, ip, layer, 0, 0, 0, 0,
										&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r,
										&pad_hole, &pad_angle, &pad_net, &pad_connect );
									pin_info_valid = TRUE;
									pin_info_layer = layer;
								}
								if( bPad && pad_type != PAD_NONE && pad_net != net )
								{
									// check via_hole to pin_pad clearance
									int d = GetClearanceBetweenPads( PAD_ROUND, xf, yf, post_vtx->via_hole_w, 0, 0, 0,
										pad_type, pad_x, pad_y, pad_w, pad_l, pad_r, pad_angle );
									if( d < dr->hole_copper )
									{
										// VIAHOLE_PAD
										::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &x_str, pad_x, units, FALSE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &y_str, pad_y, units, FALSE, TRUE, TRUE, 1 );
										CString s ((LPCSTR) IDS_ViaHoleToPadEquals);
										str.Format( s, nerrors+1, net->name, part->ref_des, ps->name,
											d_str, x_str, y_str );
										DRError * dre = drelist->Add( nerrors, DRError::VIA_PAD, &str, 
											&net->name, &part->ref_des, id_via, id_pad, xf, yf, pad_x, pad_y, 0, layer );
										if( dre )
										{
											nerrors++;
											if( log )
												log->AddLine( str );
										}
										break;  // skip more layers
									}
								}
								if( drp->hole_size && layer == LAY_TOP_COPPER )
								{
									// pin has a hole, check via_hole to pin_hole clearance
									int d = Distance( xf, yf, pin->x, pin->y );
									d = max( 0, d - drp->hole_size/2 - post_vtx->via_hole_w/2 );
									if( d < dr->hole_hole )
									{
										// VIAHOLE_PADHOLE
										::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &x_str, pad_x, units, FALSE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &y_str, pad_y, units, FALSE, TRUE, TRUE, 1 );
										CString s ((LPCSTR) IDS_ViaHoleToPadHoleEquals);
										str.Format( s, nerrors+1, net->name, part->ref_des, ps->name,
											d_str, x_str, y_str );
										DRError * dre = drelist->Add( nerrors, DRError::VIA_PAD, &str, 
											&net->name, &part->ref_des, id_via, id_pad, xf, yf, pad_x, pad_y, 0, layer );
										if( dre )
										{
											nerrors++;
											if( log )
												log->AddLine( str );
										}
										break;  // skip more layers
									}
								}
							}
						}
					}
				}

				// test each copper graphic stroke
				int n_ref = part->GetNumRefStrokes();
				int n_value = part->GetNumValueStrokes();
				int n_outline = part->GetNumOutlineStrokes();
				int n_total_strokes = n_ref + n_value + n_outline;
				for( int istk=0; istk<n_total_strokes; istk++ )
				{
					// get stroke to test
					stroke * test_stroke;
					if( istk < n_ref )
						test_stroke = &part->ref_text_stroke[istk];
					else if( istk < (n_ref + n_value) )
						test_stroke = &part->value_stroke[istk-n_ref];
					else if( istk < (n_ref + n_value + n_outline) )
						test_stroke = &part->m_outline_stroke[istk-n_ref-n_value];
					int cglayer = test_stroke->layer;
					int cgxi = test_stroke->xi;
					int cgyi = test_stroke->yi;
					int cgxf = test_stroke->xf;
					int cgyf = test_stroke->yf;
					int cgstyle;
					if( test_stroke->type == DL_LINE )
						cgstyle = CPolyLine::STRAIGHT;
					else if( test_stroke->type == DL_ARC_CW )
						cgstyle = CPolyLine::ARC_CW;
					else if( test_stroke->type == DL_ARC_CCW )
						cgstyle = CPolyLine::ARC_CCW;
					else
						ASSERT(0);

					// test against each segment in connection
					CIterator_cseg iter_seg(c);
					for( cseg * s=iter_seg.GetFirst(); s; s=iter_seg.GetNext() )
					{
						int is = iter_seg.GetIndex();
						cvertex * pre_vtx = &s->GetPreVtx();
						cvertex * post_vtx = &s->GetPostVtx();
						int w = s->m_width;
						int xi = pre_vtx->x;
						int yi = pre_vtx->y;
						int xf = post_vtx->x;
						int yf = post_vtx->y;
						int min_x = min( xi, xf ) - w/2;
						int max_x = max( xi, xf ) + w/2;
						int min_y = min( yi, yf ) - w/2;
						int max_y = max( yi, yf ) + w/2;
						// ids
						id id_seg = net->m_id;
						id_seg.SetT2( ID_CONNECT );
						id_seg.SetI2( ic );
						id_seg.SetT3( ID_SEG );
						id_seg.SetI3( is );
						id id_via = net->m_id;
						id_via.SetT2( ID_CONNECT );
						id_via.SetI2( ic );
						id_via.SetT3( ID_VIA );
						id_via.SetI3( is+1 );

						if( s->m_layer == test_stroke->layer )
						{
							// check segment clearances
							int x, y;
							int d = ::GetClearanceBetweenSegments( cgxi, cgyi, cgxf, cgyf, cgstyle, 0,
								xi, yi, xf, yf, CPolyLine::STRAIGHT, w,
								dr->board_edge_copper, &x, &y );
							if( d < dr->copper_copper )
							{
								// COPPERGRAPHIC_SEG error
								::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
								::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
								::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
								CString s0 ((LPCSTR) IDS_TraceToCopperGraphic);
								str.Format( s0, nerrors+1, net->name, d_str, x_str, y_str );
								DRError * dre = drelist->Add( nerrors, DRError::COPPERGRAPHIC_SEG, &str,
									&net->name, NULL, id_seg, id_seg, x, y, 0, 0, 0, cglayer );
								if( dre )
								{
									nerrors++;
									if( log )
										log->AddLine( str );
								}
							}
						}
							// get next via
						if( post_vtx->via_w )
						{
							// via exists
							int x = post_vtx->x;
							int y = post_vtx->y;
							int test;
							int via_w;
							int via_hole_w;
							m_nlist->GetViaPadInfo( net, ic, is+1, test_stroke->layer, 
								&via_w, &via_hole_w, &test );
							if( via_w )
							{
								// check via
								int d = GetClearanceBetweenLineSegmentAndPad( cgxi, cgyi, cgxf, cgyf, w,
									PAD_ROUND, x, y, via_w, 0, 0, 0 );
								if( d < dr->copper_copper )
								{
									// COPPERGRAPHIC_SEG error
									::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
									::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
									::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
									CString s0 ((LPCSTR) IDS_ViaToCopperGraphic);
									str.Format( s0,	nerrors+1, net->name, d_str, x_str, y_str );
									DRError * dre = drelist->Add( nerrors, DRError::COPPERGRAPHIC_VIA, &str,
										&net->name, NULL, id_seg, id_seg, post_vtx->x, post_vtx->y, 
										0, 0, 0, cglayer );
									if( dre )
									{
										nerrors++;
										if( log )
											log->AddLine( str );
									}
								}
							}
							if( via_hole_w )
							{
								// check via hole
								int d = GetClearanceBetweenLineSegmentAndPad( cgxi, cgyi, cgxf, cgyf, w,
									PAD_ROUND, x, y, via_hole_w, 0, 0, 0 );
								if( d < dr->hole_copper )
								{
									// COPPERGRAPHIC_VIAHOLE error
									::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
									::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
									::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
									CString s0 ((LPCSTR) IDS_ViaHoleToCopperGraphic);
									str.Format( s0, nerrors+1, net->name, d_str, x_str, y_str );
									DRError * dre = drelist->Add( nerrors, DRError::COPPERGRAPHIC_VIAHOLE, &str,
										&net->name, NULL, id_seg, id_seg, post_vtx->x, post_vtx->y, 
										0, 0, 0, cglayer );
									if( dre )
									{
										nerrors++;
										if( log )
											log->AddLine( str );
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// now check nets against other nets
	// check for following errors:
	//	SEG_SEG
	//	SEG_VIA
	//	SEG_VIAHOLE
	//	VIA_VIA
	//	VIA_VIAHOLE
	//	VIAHOLE_VIAHOLE
	//	COPPERAREA_INSIDE_COPPERAREA
	//	COPPERAREA_COPPERAREA
	str.LoadStringA( IDS_CheckingNets2 );
	if( log )
		log->AddLine( str );
	// get max clearance
	int cl = max( dr->hole_copper, dr->hole_hole );
	cl = max( cl, dr->trace_trace );
	// iterate through all nets
	for( pos = m_nlist->m_map.GetStartPosition(); pos != NULL; )
	{
		m_nlist->m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		// iterate through all connections
		CIterator_cconnect iter_con(net);
		for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
		{
			int ic = iter_con.GetIndex();

			// iterate through all nets again
			POSITION pos2 = pos;
			void * ptr2;
			CString name2;
			while( pos2 != NULL )
			{
				m_nlist->m_map.GetNextAssoc( pos2, name2, ptr2 );
				cnet * net2 = (cnet*)ptr2;
				// iterate through all connections
				CIterator_cconnect iter_con2(net2);
				for( cconnect * c2=iter_con2.GetFirst(); c2; c2=iter_con2.GetNext() )
				{
					int ic2 = iter_con2.GetIndex();

					// look for possible clearance violations between c and c2
					if( c->min_x - c2->max_x > cl )
						continue;	// no, next connection
					if( c->min_y - c2->max_y > cl )
						continue;	// no, next connection
					if( c2->min_x - c->max_x > cl )
						continue;	// no, next connection
					if( c2->min_y - c->max_y > cl )
						continue;	// no, next connection

					// now we have to test all segments and vias in c
					CIterator_cseg iter_seg(c);
					for( cseg * s=iter_seg.GetFirst(); s; s=iter_seg.GetNext() )
					{
						// get next segment and via
						int is = iter_seg.GetIndex();
						cvertex * pre_vtx = &s->GetPreVtx();
						cvertex * post_vtx = &s->GetPostVtx();
						int seg_w = s->m_width;
						int vw = post_vtx->via_w;
						int max_w = max( seg_w, vw );
						int xi = pre_vtx->x;
						int yi = pre_vtx->y;
						int xf = post_vtx->x;
						int yf = post_vtx->y;
						// get bounding rect for segment and vias
						int min_x = min( xi, xf ) - max_w/2;
						int max_x = max( xi, xf ) + max_w/2;
						int min_y = min( yi, yf ) - max_w/2;
						int max_y = max( yi, yf ) + max_w/2;
						// ids
						id id_seg1( ID_NET, -1, ID_CONNECT, -1, ic, ID_SEG, -1, is );
						id id_via1( ID_NET, -1, ID_CONNECT, -1, ic, ID_VIA, -1, is+1 );

						// iterate through all segments and vias in c2
						CIterator_cseg iter_seg2(c2);
						for( cseg * s2=iter_seg2.GetFirst(); s2; s2=iter_seg2.GetNext() )
						{
							// get next segment and via
							int is2 = iter_seg2.GetIndex();
							cvertex * pre_vtx2 = &s2->GetPreVtx();
							cvertex * post_vtx2 = &s2->GetPostVtx();
							int seg_w2 = s2->m_width;
							int vw2 = post_vtx2->via_w;
							int max_w2 = max( seg_w2, vw2 );
							int xi2 = pre_vtx2->x;
							int yi2 = pre_vtx2->y;
							int xf2 = post_vtx2->x;
							int yf2 = post_vtx2->y;
							// get bounding rect for this segment and attached vias
							int min_x2 = min( xi2, xf2 ) - max_w2/2;
							int max_x2 = max( xi2, xf2 ) + max_w2/2;
							int min_y2 = min( yi2, yf2 ) - max_w2/2;
							int max_y2 = max( yi2, yf2 ) + max_w2/2;
							// ids
							id id_seg2( ID_NET, -1, ID_CONNECT, -1, ic2, ID_SEG, -1, is2 );
							id id_via2( ID_NET, -1, ID_CONNECT, -1, ic2, ID_VIA, -1, is2+1 );

							// see if segment bounding rects are too close
							if( min_x - max_x2 > cl )
								continue;	// no, next segment
							if( min_y - max_y2 > cl )
								continue;
							if( min_x2 - max_x > cl )
								continue;
							if( min_y2 - max_y > cl )
								continue;

							// check if segments on same layer
							if( s->m_layer == s2->m_layer && s->m_layer >= LAY_TOP_COPPER ) 
							{
								// yes, test clearances between segments
								int xx, yy; 
								int d = ::GetClearanceBetweenSegments( xi, yi, xf, yf, CPolyLine::STRAIGHT, seg_w, 
									xi2, yi2, xf2, yf2, CPolyLine::STRAIGHT, seg_w2, dr->trace_trace, &xx, &yy );
								if( d < dr->trace_trace )
								{
									// SEG_SEG
									::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
									::MakeCStringFromDimension( &x_str, xx, units, FALSE, TRUE, TRUE, 1 );
									::MakeCStringFromDimension( &y_str, yy, units, FALSE, TRUE, TRUE, 1 );
									CString s0 ((LPCSTR) IDS_TraceToTraceEquals);
									str.Format( s0, nerrors+1, net->name, net2->name,
										d_str, x_str, y_str );
									DRError * dre = drelist->Add( nerrors, DRError::SEG_SEG, &str, 
										&net->name, &net2->name, id_seg1, id_seg2, xx, yy, xx, yy, 0, s->m_layer );
									if( dre )
									{
										nerrors++;
										if( log )
											log->AddLine( str );
									}
								}
							}
							// test clearances between net->segment and net2->via
							int layer = s->m_layer;
							if( layer >= LAY_TOP_COPPER && post_vtx2->via_w )
							{
								// via exists
								int test = m_nlist->GetViaConnectionStatus( net2, ic2, is2+1, layer );
								int via_w2 = post_vtx2->via_w;	// normal via pad
								if( layer > LAY_BOTTOM_COPPER && test == CNetList::VIA_NO_CONNECT )
								{
									// inner layer and no trace or thermal, so no via pad
									via_w2 = 0;
								}
								else if( layer > LAY_BOTTOM_COPPER && (test & CNetList::VIA_AREA) && !(test & CNetList::VIA_TRACE) )
								{
									// inner layer with small thermal, use annular ring
									via_w2 = post_vtx2->via_hole_w + 2*dr->annular_ring_vias;
								}
								// check clearance
								if( via_w2 )
								{
									// check clearance between segment and via pad
									int d = GetClearanceBetweenLineSegmentAndPad( xi, yi, xf, yf, seg_w,
										PAD_ROUND, post_vtx2->x, post_vtx2->y, post_vtx2->via_w, 0, 0, 0 );
									if( d < dr->trace_trace )
									{
										// SEG_VIA
										::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &x_str, post_vtx2->x, units, FALSE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &y_str, post_vtx2->y, units, FALSE, TRUE, TRUE, 1 );
										CString s0 ((LPCSTR) IDS_TraceToViaPadEquals);
										str.Format( s0, nerrors+1, net->name, net2->name,
											d_str, x_str, y_str );
										DRError * dre = drelist->Add( nerrors, DRError::SEG_VIA, &str, 
											&net->name, &net2->name, id_seg1, id_via2, xf2, yf2, xf2, yf2, 0, s->m_layer );
										if( dre )
										{
											nerrors++;
											if( log )
												log->AddLine( str );
										}
									}
								}
								// check clearance between segment and via hole
								int d = GetClearanceBetweenLineSegmentAndPad( xi, yi, xf, yf, seg_w,
									PAD_ROUND, post_vtx2->x, post_vtx2->y, post_vtx2->via_hole_w, 0, 0, 0 );
								if( d < dr->hole_copper )
								{
									// SEG_VIAHOLE
									::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
									::MakeCStringFromDimension( &x_str, post_vtx2->x, units, FALSE, TRUE, TRUE, 1 );
									::MakeCStringFromDimension( &y_str, post_vtx2->y, units, FALSE, TRUE, TRUE, 1 );
									CString s0 ((LPCSTR) IDS_TraceToViaHoleEquals);
									str.Format( s0, nerrors+1, net->name, net2->name,
										d_str, x_str, y_str );
									DRError * dre = drelist->Add( nerrors, DRError::SEG_VIAHOLE, &str, 
										&net->name, &net2->name, id_seg1, id_via2, xf2, yf2, xf2, yf2, 0, s->m_layer );
									if( dre )
									{
										nerrors++;
										if( log )
											log->AddLine( str );
									}
								}
							}
							// test clearances between net2->segment and net->via
							layer = s2->m_layer;
							if( post_vtx->via_w )
							{
								// via exists
								int test = m_nlist->GetViaConnectionStatus( net, ic, is+1, layer );
								int via_w = post_vtx->via_w;	// normal via pad
								if( layer > LAY_BOTTOM_COPPER && test == CNetList::VIA_NO_CONNECT )
								{
									// inner layer and no trace or thermal, so no via pad
									via_w = 0;
								}
								else if( layer > LAY_BOTTOM_COPPER && (test & CNetList::VIA_AREA) && !(test & CNetList::VIA_TRACE) )
								{
									// inner layer with small thermal, use annular ring
									via_w = post_vtx->via_hole_w + 2*dr->annular_ring_vias;
								}
								// check clearance
								if( via_w )
								{
									// check clearance between net2->segment and net->via_pad
									if( layer >= LAY_TOP_COPPER )
									{
										int d = GetClearanceBetweenLineSegmentAndPad( xi2, yi2, xf2, yf2, seg_w2,
											PAD_ROUND, post_vtx->x, post_vtx->y, post_vtx->via_w, 0, 0, 0 );
										if( d < dr->trace_trace )
										{
											// SEG_VIA
											::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
											::MakeCStringFromDimension( &x_str, post_vtx->x, units, FALSE, TRUE, TRUE, 1 );
											::MakeCStringFromDimension( &y_str, post_vtx->y, units, FALSE, TRUE, TRUE, 1 );
											CString s ((LPCSTR) IDS_ViaPadToTraceEquals);
											str.Format( s, nerrors+1, net->name, net2->name,
												d_str, x_str, y_str );
											DRError * dre = drelist->Add( nerrors, DRError::SEG_VIA, &str, 
												&net2->name, &net->name, id_seg2, id_via1, xf, yf, xf, yf, 
												post_vtx->via_w+20*NM_PER_MIL, 0 );
											if( dre )
											{
												nerrors++;
												if( log )
													log->AddLine( str );
											}
										}
									}
								}
								// check clearance between net2->segment and net->via_hole
								if( layer >= LAY_TOP_COPPER )
								{
									int d = GetClearanceBetweenLineSegmentAndPad( xi2, yi2, xf2, yf2, seg_w2,
										PAD_ROUND, post_vtx->x, post_vtx->y, post_vtx->via_hole_w, 0, 0, 0 );
									if( d < dr->hole_copper )
									{
										// SEG_VIAHOLE
										::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &x_str, post_vtx->x, units, FALSE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &y_str, post_vtx->y, units, FALSE, TRUE, TRUE, 1 );
										CString s ((LPCSTR) IDS_TraceToViaHoleEquals);
										str.Format( s, nerrors+1, net2->name, net->name,
											d_str, x_str, y_str );
										DRError * dre = drelist->Add( nerrors, DRError::SEG_VIAHOLE, &str, 
											&net2->name, &net->name, id_seg2, id_via1, xf, yf, xf, yf, 
											post_vtx->via_w+20*NM_PER_MIL, 0 );
										if( dre )
										{
											nerrors++;
											if( log )
												log->AddLine( str );
										}
									}
								}
								// test clearances between net->via and net2->via
								if( post_vtx->via_w && post_vtx2->via_w )
								{
									for( int layer=LAY_TOP_COPPER; layer<(LAY_TOP_COPPER+copper_layers); layer++ )
									{
										// get size of net->via_pad
										int test = m_nlist->GetViaConnectionStatus( net, ic, is+1, layer );
										int via_w = post_vtx->via_w;	// normal via pad
										if( layer > LAY_BOTTOM_COPPER && test == CNetList::VIA_NO_CONNECT )
										{
											// inner layer and no trace or thermal, so no via pad
											via_w = 0;
										}
										else if( layer > LAY_BOTTOM_COPPER && (test & CNetList::VIA_AREA) && !(test & CNetList::VIA_TRACE) )
										{
											// inner layer with small thermal, use annular ring
											via_w = post_vtx->via_hole_w + 2*dr->annular_ring_vias;
										}
										// get size of net2->via_pad
										test = m_nlist->GetViaConnectionStatus( net2, ic2, is2+1, layer );
										int via_w2 = post_vtx2->via_w;	// normal via pad
										if( layer > LAY_BOTTOM_COPPER && test == CNetList::VIA_NO_CONNECT )
										{
											// inner layer and no trace or thermal, so no via pad
											via_w2 = 0;
										}
										else if( layer > LAY_BOTTOM_COPPER && (test & CNetList::VIA_AREA) && !(test & CNetList::VIA_TRACE) )
										{
											// inner layer with small thermal, use annular ring
											via_w2 = post_vtx2->via_hole_w + 2*dr->annular_ring_vias;
										}
										if( via_w && via_w2 )
										{
											//check net->via_pad to net2->via_pad clearance
											int d = GetClearanceBetweenPads( PAD_ROUND, post_vtx->x, post_vtx->y, post_vtx->via_w, 0, 0, 0, 
												PAD_ROUND, post_vtx2->x, post_vtx2->y, post_vtx2->via_w, 0, 0, 0 );
											if( d < dr->trace_trace )
											{
												// VIA_VIA
												::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
												::MakeCStringFromDimension( &x_str, post_vtx->x, units, FALSE, TRUE, TRUE, 1 );
												::MakeCStringFromDimension( &y_str, post_vtx->y, units, FALSE, TRUE, TRUE, 1 );
												CString s ((LPCSTR) IDS_ViaPadToViaPadEquals);
												str.Format( s, nerrors+1, net->name, net2->name,
													d_str, x_str, y_str );
												DRError * dre = drelist->Add( nerrors, DRError::VIA_VIA, &str, 
													&net->name, &net2->name, id_via1, id_via2, xf, yf, xf2, yf2, 0, layer );
												if( dre )
												{
													nerrors++;
													if( log )
														log->AddLine( str );
												}
											}
											// check net->via to net2->via_hole clearance
											d = GetClearanceBetweenPads( PAD_ROUND, post_vtx->x, post_vtx->y, post_vtx->via_w, 0, 0, 0,
												PAD_ROUND, post_vtx2->x, post_vtx2->y, post_vtx2->via_hole_w, 0, 0, 0 );
											if( d < dr->hole_copper )
											{
												// VIA_VIAHOLE
												::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
												::MakeCStringFromDimension( &x_str, post_vtx->x, units, FALSE, TRUE, TRUE, 1 );
												::MakeCStringFromDimension( &y_str, post_vtx->y, units, FALSE, TRUE, TRUE, 1 );
												CString s ((LPCSTR) IDS_ViaPadToViaHoleEquals);
												str.Format( s, nerrors+1, net->name, net2->name,
													d_str, x_str, y_str );
												DRError * dre = drelist->Add( nerrors, DRError::VIA_VIAHOLE, &str, 
													&net->name, &net2->name, id_via1, id_via2, xf, yf, xf2, yf2, 0, layer );
												if( dre )
												{
													nerrors++;
													if( log )
														log->AddLine( str );
												}
											}
											// check net2->via to net->via_hole clearance
											d = GetClearanceBetweenPads( PAD_ROUND, post_vtx->x, post_vtx->y, post_vtx->via_hole_w, 0, 0, 0,
												PAD_ROUND, post_vtx2->x, post_vtx2->y, post_vtx2->via_w, 0, 0, 0 );
											if( d < dr->hole_copper )
											{
												// VIA_VIAHOLE
												::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
												::MakeCStringFromDimension( &x_str, post_vtx->x, units, FALSE, TRUE, TRUE, 1 );
												::MakeCStringFromDimension( &y_str, post_vtx->y, units, FALSE, TRUE, TRUE, 1 );
												CString s ((LPCSTR) IDS_ViaPadToViaHoleEquals);
												str.Format( s, nerrors+1, net2->name, net->name,
													d_str, x_str, y_str );
												DRError * dre = drelist->Add( nerrors, DRError::VIA_VIAHOLE, &str, 
													&net2->name, &net->name, id_via2, id_via1, xf, yf, xf2, yf2, 0, layer );
												if( dre )
												{
													nerrors++;
													if( log )
														log->AddLine( str );
												}
											}
										}
									}
									// check net->via_hole to net2->via_hole clearance
									int d = GetClearanceBetweenPads( PAD_ROUND, post_vtx->x, post_vtx->y, post_vtx->via_hole_w, 0, 0, 0,
										PAD_ROUND, post_vtx2->x, post_vtx2->y, post_vtx2->via_hole_w, 0, 0,0  );
									if( d < dr->hole_hole )
									{
										// VIA_VIAHOLE
										::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &x_str, post_vtx->x, units, FALSE, TRUE, TRUE, 1 );
										::MakeCStringFromDimension( &y_str, post_vtx->y, units, FALSE, TRUE, TRUE, 1 );
										CString s ((LPCSTR) IDS_ViaHoleToViaHoleEquals);
										str.Format( s, nerrors+1, net2->name, net->name,
											d_str, x_str, y_str );
										DRError * dre = drelist->Add( nerrors, DRError::VIAHOLE_VIAHOLE, &str, 
											&net->name, &net2->name, id_via1, id_via2, xf, yf, xf2, yf2, 0, 0 );
										if( dre )
										{
											nerrors++;
											if( log )
												log->AddLine( str );
										}
									}
								}
							}
						}
					}
				}
			}
		}
		// now iterate through all areas
		for( int ia=0; ia<net->NumAreas(); ia++ )
		{
			carea * a = &net->area[ia];
			// iterate through all nets again
			POSITION pos2 = pos;
			void * ptr2;
			CString name2;
			while( pos2 != NULL )
			{
				m_nlist->m_map.GetNextAssoc( pos2, name2, ptr2 );
				cnet * net2 = (cnet*)ptr2;
				for( int ia2=0; ia2<net2->NumAreas(); ia2++ )
				{
					carea * a2 = &net2->area[ia2];
					// test for same layer
					if( a->Layer() == a2->Layer() ) 
					{
						// test for points inside one another
						for( int ic=0; ic<a->NumCorners(); ic++ )
						{
							int x = a->X(ic);
							int y = a->Y(ic);
							if( a2->TestPointInside( x, y ) )
							{
								// COPPERAREA_COPPERAREA error
								id id_a = net->m_id;
								id_a.SetT2( ID_AREA );
								id_a.SetI2( ia );
								id_a.SetT3( ID_SEL_CORNER );
								id_a.SetI3( ic );
								CString s ((LPCSTR) IDS_CopperAreaInsideCopperArea);
								str.Format( s, nerrors+1, net->name, net2->name );
								DRError * dre = drelist->Add( nerrors, DRError::COPPERAREA_INSIDE_COPPERAREA, &str,
									&net->name, &net2->name, id_a, id_a, x, y, x, y, 0, 0 );
								if( dre )
								{
									nerrors++;
									if( log )
										log->AddLine( str );
								}
							}
						}
						for( int ic2=0; ic2<a2->NumCorners(); ic2++ )
						{
							int x = a2->X(ic2);
							int y = a2->Y(ic2);
							if( a->TestPointInside( x, y ) )
							{
								// COPPERAREA_COPPERAREA error
								id id_a = net2->m_id;
								id_a.SetT2( ID_AREA );
								id_a.SetI2( ia2 );
								id_a.SetT3( ID_SEL_CORNER );
								id_a.SetI3( ic2 );
								CString s ((LPCSTR) IDS_CopperAreaInsideCopperArea);
								str.Format( s, nerrors+1, net2->name, net->name );
								DRError * dre = drelist->Add( nerrors, DRError::COPPERAREA_INSIDE_COPPERAREA, &str,
									&net2->name, &net->name, id_a, id_a, x, y, x, y, 0, 0 );
								if( dre )
								{
									nerrors++;
									if( log )
										log->AddLine( str );
								}
							}
						}
						// now test spacing between areas
						for( int icont=0; icont<a->NumContours(); icont++ )
						{
							int ic_start = a->ContourStart( icont );
							int ic_end = a->ContourEnd( icont );
							for( int ic=ic_start; ic<=ic_end; ic++ ) 
							{
								id id_a = net->m_id;
								id_a.SetT2( ID_AREA );
								id_a.SetI2( ia );
								id_a.SetT3( ID_SIDE );
								id_a.SetI3( ic );
								int ax1 = a->X(ic);
								int ay1 = a->Y(ic);
								int ax2, ay2;
								if( ic == ic_end )
								{
									ax2 = a->X(ic_start);
									ay2 = a->Y(ic_start);
								}
								else
								{
									ax2 = a->X(ic+1);
									ay2 = a->Y(ic+1);
								}
								int astyle = a->SideStyle(ic);
								for( int icont2=0; icont2<a2->NumContours(); icont2++ )
								{
									int ic_start2 = a2->ContourStart( icont2 );
									int ic_end2 = a2->ContourEnd( icont2 );
									for( int ic2=ic_start2; ic2<=ic_end2; ic2++ )
									{
										id id_b = net2->m_id;
										id_b.SetT2( ID_AREA );
										id_b.SetI2( ia2 );
										id_b.SetT3( ID_SIDE );
										id_b.SetI3( ic2 );
										int bx1 = a2->X(ic2);
										int by1 = a2->Y(ic2);
										int bx2, by2;
										if( ic2 == ic_end2 )
										{
											bx2 = a2->X(ic_start2);
											by2 = a2->Y(ic_start2);
										}
										else
										{
											bx2 = a2->X(ic2+1);
											by2 = a2->Y(ic2+1);
										}
										int bstyle = a2->SideStyle(ic2);
										int x, y;
										int d = ::GetClearanceBetweenSegments( bx1, by1, bx2, by2, bstyle, 0,
											ax1, ay1, ax2, ay2, astyle, 0, dr->copper_copper, &x, &y );
										if( d < dr->copper_copper )
										{
											// COPPERAREA_COPPERAREA error
											::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
											::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
											::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
											CString s ((LPCSTR) IDS_CopperAreaToCopperArea);
											str.Format( s, nerrors+1, net->name, net2->name, d_str, x_str, y_str );
											DRError * dre = drelist->Add( nerrors, DRError::COPPERAREA_COPPERAREA, &str,
												&net->name, &net2->name, id_a, id_b, x, y, x, y, 0, 0 );
											if( dre )
											{
												nerrors++;
												if( log )
													log->AddLine( str );
											}
										}
									}
								}
							}
						}
					}
				}
			}

			// CPT: new broken area check (function defined below)
			CheckBrokenArea(a, net, log, units, drelist, nerrors);
		}
	}
	// now check for unrouted connections, if requested
	if( check_unrouted )
	{
		for( pos = m_nlist->m_map.GetStartPosition(); pos != NULL; )
		{
			m_nlist->m_map.GetNextAssoc( pos, name, ptr );
			cnet * net = (cnet*)ptr;
			// iterate through all connections
			// now check connections
			CIterator_cconnect iter_con(net);
			for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
			{
				int ic = iter_con.GetIndex();

				// check for unrouted or partially routed connection
				BOOL bUnrouted = FALSE;
				for( int is=0; is<c->NumSegs(); is++ )
				{
					if( c->SegByIndex(is).m_layer == LAY_RAT_LINE )
					{
						bUnrouted = TRUE;
						break;
					}
				}
				if( bUnrouted )
				{
					// unrouted or partially routed connection
					CString start_pin, end_pin;
					cpin * p1 = c->StartPin();
					cpart * start_part = p1->part;
					start_pin = p1->ref_des + "." + p1->pin_name;
					cpin * p2 = c->EndPin();
					if( p2 == NULL )
					{
						CString s ((LPCSTR) IDS_PartiallyRoutedStubTrace);
						str.Format( s, nerrors+1, net->name, start_pin );
						CPoint pt = GetPinPoint( start_part, p1->pin_name );
						id id_a = net->m_id;
						DRError * dre = drelist->Add( nerrors, DRError::UNROUTED, &str,
							&net->name, NULL, id_a, id_a, pt.x, pt.y, pt.x, pt.y, 0, 0 );
						if( dre )
						{
							nerrors++;
							if( log )
								log->AddLine( str );
						}
					}
					else
					{
						end_pin = p2->ref_des + "." + p2->pin_name;
						if( c->NumSegs() > 1 )
						{
							CString s ((LPCSTR) IDS_PartiallyRoutedConnection);
							str.Format( s, nerrors+1, net->name, start_pin, end_pin );
						}
						else
						{
							CString s ((LPCSTR) IDS_UnroutedConnection);
							str.Format( s, nerrors+1, net->name, start_pin, end_pin );
						}
						CPoint pt = GetPinPoint( start_part, p1->pin_name );
						id id_a = net->m_id;
						DRError * dre = drelist->Add( nerrors, DRError::UNROUTED, &str,
							&net->name, NULL, id_a, id_a, pt.x, pt.y, pt.x, pt.y, 0, 0 );
						if( dre )
						{
							nerrors++;
							if( log )
								log->AddLine( str );
						}
					}
				}
			}
		}
	}
	str.LoadStringA(IDS_Done4);
	if( log )
		log->AddLine( str );
#endif
}

// check partlist for errors
//
int CPartList::CheckPartlist( CString * logstr )
{
#ifndef CPT2
	int nerrors = 0;
	int nwarnings = 0;
	CString str;
	CMapStringToPtr map;
	void * ptr;

	CString s ((LPCSTR) IDS_CheckingParts2);
	*logstr += s;

	// first, check for duplicate parts
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		CString ref_des = part->ref_des;
		BOOL test = map.Lookup( ref_des, ptr );
		if( test )
		{
			CString s ((LPCSTR) IDS_PartDuplicated);
			str.Format( s, ref_des );
			*logstr += str;
			nerrors++;
		}
		else
			map.SetAt( ref_des, NULL );

		// next part
		part = part->next;
	}

	// now check all parts
	part = m_start.next;
	while( part->next != 0 )
	{
		// check this part
		str = "";
		CString * ref_des = &part->ref_des;
		if( !part->shape )
		{
			// no footprint
			CString s ((LPCSTR) IDS_WarningPartHasNoFootprint);
			str.Format( s, *ref_des );
			*logstr += str;
			nwarnings++;
		}
		else
		{
			for( int ip=0; ip<part->pin.GetSize(); ip++ )
			{
				// check this pin
				cnet * net = part->pin[ip].net;
				CString * pin_name = &part->shape->m_padstack[ip].name;
				if( !net )
				{
					// part->pin->net is NULL, pin unconnected
					// this is not an error
					//				str.Format( "%s.%s unconnected\r\n",
					//					*ref_des, *pin_name );
				}
				else
				{
					cnet * netlist_net = m_nlist->GetNetPtrByName( &net->name );
					if( !netlist_net )
					{
						// part->pin->net->name doesn't exist in netlist
						CString s ((LPCSTR) IDS_ErrorPartPinConnectedToNetWhichDoesntExist);
						str.Format( s, *ref_des, *pin_name, net->name );
						*logstr += str;
						nerrors++;
					}
					else
					{
						if( net != netlist_net )
						{
							// part->pin->net doesn't match netlist->net
							CString s ((LPCSTR) IDS_ErrorPartPinConnectedToNetWhichDoesntMatchNetlist);
							str.Format( s, *ref_des, *pin_name, net->name );
							*logstr += str;
							nerrors++;
						}
						else
						{
							// try to find pin in pin list for net
							int net_pin = -1;
							for( int ip=0; ip<net->NumPins(); ip++ )
							{
								if( net->pin[ip].part == part )
								{
									if( net->pin[ip].pin_name == *pin_name )
									{
										net_pin = ip;
										break;
									}
								}
							}
							if( net_pin == -1 )
							{
								// pin not found
								CString s ((LPCSTR) IDS_ErrorPartPinConnectedToNetButPinNotInNet);
								str.Format( s, *ref_des, *pin_name, net->name );
								*logstr += str;
								nerrors++;
							}
							else
							{
								// OK
							}

						}
					}
				}
			}
		}

		// next part
		part = part->next;
	}
	CString s0 ((LPCSTR) IDS_ErrorsWarnings);
	str.Format( s0, nerrors, nwarnings );
	*logstr += str;

	return nerrors;
#else
	return 0;
#endif
}

void CPartList::MoveOrigin( int x_off, int y_off )
{
	cpart * part = GetFirstPart();
	while( part )
	{
		if( part->shape )
		{
			// move this part
			UndrawPart( part );
			part->x += x_off;
			part->y += y_off;
			for( int ip=0; ip<part->pin.GetSize(); ip++ )
			{
				part->pin[ip].x += x_off;
				part->pin[ip].y += y_off;
			}
			DrawPart( part );
		}
		part = GetNextPart(part);
	}
}

BOOL CPartList::CheckForProblemFootprints()
{
#ifndef CPT2
	BOOL bHeaders_28mil_holes = FALSE;   
	cpart * part = GetFirstPart();
	while( part )
	{
		if( part->shape)
		{
			if( part->shape->m_name.Right(7) == "HDR-100" 
				&& part->shape->m_padstack[0].hole_size == 28*NM_PER_MIL )
			{
				bHeaders_28mil_holes = TRUE;
			}
		}
		part = GetNextPart( part );
	}
	if( g_bShow_header_28mil_hole_warning && bHeaders_28mil_holes )   
	{
		CDlgMyMessageBox dlg;
		CString s ((LPCSTR) IDS_WarningYouAreLoadingFootprintsForThroughHoleHeaders);
		dlg.Initialize( s );
		dlg.DoModal();
		g_bShow_header_28mil_hole_warning = !dlg.bDontShowBoxState;
	}
	return bHeaders_28mil_holes;
#else
	return 0;
#endif
}


// CPT (All that follows)
void CPartList::CheckBrokenArea(carea *a, cnet *net, CDlgLog * log, int units, DRErrorList * drelist, long &nerrors) {
	// CPT: new check for areas that have been broken or nearly broken by the clearances for traces from other nets.
	// Create a "bitmap" (just a 2-d array of 1-byte flags, indicating whether a given location is part of the area's copper).
	// NB class CMyBitmap is found in utility.h.
	CString x_str, y_str;
	int layer = a->Layer();
	int nContours = a->NumContours();
	// Get area's bounds and dimensions in nm.
	CRect bounds = a->GetCornerBounds();
	int wNm = bounds.right - bounds.left, hNm = bounds.top - bounds.bottom;
	// Determine the granularity of the bitmap.  By default it's 10 mils per pixel, but that value may increase if area is big
	int gran = 10*NM_PER_MIL;	
	int w = wNm/gran, h = hNm/gran;
	while (w*h>1000000)
		w /= 2, h /= 2, gran *= 2;
	w += 3, h += 3;												// A little extra wiggle room...
	int x0 = bounds.left - gran, y0 = bounds.bottom - gran;
	int xSample = 0, ySample = 0;								// Set below:  an arbitrarily chosen edge-corner position
	CMyBitmap *bmp  = new CMyBitmap(w, h);

	// Draw the area's edges onto bmp, filling the relevant pixels in with byte value xEdge for outer edges
	// (edges belonging to a's contour #0) or xHole for edges of the area's holes (contours 1, 2, ...)
	#define xEdge 0x01
	#define xHole 0x02
	for( int i=0; i<nContours; i++ ) {
		int start = a->ContourStart(i), end = a->ContourEnd(i);
		for (int j=start; j<=end; j++) {
			int x1 = a->X(j), y1 = a->Y(j);
			if (!xSample)
				xSample = x1, ySample = y1;
			int x2, y2;
			if (j==end) 
				x2 = a->X(start), y2 = a->Y(start);
			else
				x2 = a->X(j+1), y2 = a->Y(j+1);
			x1 = (x1-x0)/gran, y1 = (y1-y0)/gran;
			x2 = (x2-x0)/gran, y2 = (y2-y0)/gran;
			char val = i==0? xEdge: xHole;
			int style = a->SideStyle(j);
			if (style==CPolyLine::STRAIGHT)
				bmp->Line(x1,y1, x2,y2, val);
			else
				bmp->Arc(x1,y1, x2,y2, style==CPolyLine::ARC_CW, val);
			}
		}

	// Now draw on the bitmap any segments that are in the correct layer but belong to nets other than a's.
	#define xSeg 0x03
	POSITION pos2 = m_nlist->m_map.GetStartPosition();
	void * ptr2;
	CString name2;
	while( pos2 != NULL ) {
		m_nlist->m_map.GetNextAssoc( pos2, name2, ptr2 );
		cnet * net2 = (cnet*)ptr2;
		if (net2==net) continue;
		for( int ic=0; ic<net2->NumCons(); ic++ ) {
			cconnect * c2 = net2->ConByIndex(ic);
			for (int is=0; is<c2->NumSegs(); is++) {
				cseg *s = &c2->SegByIndex(is);
				if (s->m_layer!=layer) continue;
				int x1 = c2->VtxByIndex(is).x, y1 = c2->VtxByIndex(is).y;
				int x2 = c2->VtxByIndex(is+1).x, y2 = c2->VtxByIndex(is+1).y;
				x1 = (x1-x0)/gran, y1 = (y1-y0)/gran;
				x2 = (x2-x0)/gran, y2 = (y2-y0)/gran;
				bmp->Line(x1,y1, x2,y2, xSeg);
				}
			}
		}

	// Now scan the bitmap's rows, looking for one where the initial pixel values are 0/xSeg, 0/xSeg, ..., 0/xSeg, xEdge, 0.
	// The last pixel there is surely in the area's interior.  If we don't find such a pixel, the area is weirdly scrawny:  generate 
	// a DRC warning and bail out.
	int xInt = -1, yInt = -1;
	for (int y=1; y<h-1; y++) {
		int x=0;
		while (x<w && (bmp->Get(x,y)==0 || bmp->Get(x,y)==xSeg)) x++;
		if (x>=w-2) continue;
		if (bmp->Get(x,y)==xEdge && bmp->Get(x+1,y)==0) 
			{ xInt = x+1, yInt = y; break; }
		}
	if (xInt==-1) {
		CString s ((LPCSTR) IDS_WarningCopperAreaIsTooNarrow), str;
		::MakeCStringFromDimension( &x_str, xSample, units, FALSE, TRUE, TRUE, 1 );
		::MakeCStringFromDimension( &y_str, ySample, units, FALSE, TRUE, TRUE, 1 );
		str.Format( s, nerrors+1, net->name, x_str, y_str );
		DRError * dre = drelist->Add( nerrors, DRError::COPPERAREA_BROKEN, &str,
									&net->name, 0, net->Id(), 0, xSample, ySample, xSample, ySample, 0, 0 );
		if (dre) { 
			nerrors++;
			if( log ) log->AddLine( str );
			}
		delete(bmp);
		return;
		}

	// Flood-fill the area's interior, starting from the just-found point (xInt,yInt)
	#define xInterior 0x04
	bmp->FloodFill(xInt,yInt, xInterior);

	// Now scan the bitmap for any xEdge/xHole pixels that are not adjacent to an xInterior, and convert them to value xBad.
	//  If the area is nicely connected, then only a few odd corner pixels will be xBad
	#define xBad 0x05
	for (int y=1; y<h-1; y++) 
		for (int x=1; x<w-1; x++) {
			if (bmp->Get(x,y) != xEdge && bmp->Get(x,y) != xHole) continue;
			if (bmp->Get(x-1,y) != xInterior && bmp->Get(x+1,y) != xInterior &&
				bmp->Get(x,y-1) != xInterior && bmp->Get(x,y+1) != xInterior) 
					bmp->Set(x,y, xBad);
			}

	// Now determine the connected components among the xBad pixels.  Each component that contains more than 2 (?) pixels results in
	// its own DRE.
	#define xBad2 0x06
	for (int y=1; y<h-1; y++) 
		for (int x=1; x<w-1; x++) {
			if (bmp->Get(x,y) != xBad) continue;
			int nPix = bmp->FloodFill(x, y, xBad2);
			if (nPix<3) continue;
			int x2 = x*gran + x0, y2 = y*gran + y0;
			CString s ((LPCSTR) IDS_WarningCopperAreaIsTooNarrow), str;
			::MakeCStringFromDimension( &x_str, x2, units, FALSE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &y_str, y2, units, FALSE, TRUE, TRUE, 1 );
			str.Format( s, nerrors+1, net->name, x_str, y_str );
			DRError * dre = drelist->Add( nerrors, DRError::COPPERAREA_BROKEN, &str,
										&net->name, 0, net->Id(), 0, x2, y2, x2, y2, 0, 0 );
			if (!dre) continue;
			nerrors++;
			if( log ) log->AddLine( str );
			}
	
	// Done!
	delete(bmp);
	}