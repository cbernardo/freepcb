// Part.cpp --- source file for classes related most closely to parts, many of which are descendants of cpcb_item:
//  cpart2, cpin2, cpartlist

#include "stdafx.h"
#include <math.h>
#include <stdlib.h>
#include "Part.h"
#include "Net.h"
#include "FreePcbDoc.h"
#include "Text.h"

BOOL g_bShow_header_28mil_hole_warning = TRUE;	
BOOL g_bShow_SIP_28mil_hole_warning = TRUE;							// CPT2 TODO used?


cpin2::cpin2(cpart2 *_part, cpadstack *_ps, cnet2 *_net)					// CPT2. Added args
	: cpcb_item (_part->doc)
{
	part = _part;
	part->pins.Add(this);
	ps = _ps;
	if( ps->hole_size ) 
		pad_layer = LAY_PAD_THRU;
	else if( part->side == 0 && ps->top.shape != PAD_NONE || part->side == 1 && ps->bottom.shape != PAD_NONE )
		pad_layer = LAY_TOP_COPPER;
	else
		pad_layer = LAY_BOTTOM_COPPER;
	pin_name = ps->name;
	net = _net;
	if (net)
		net->pins.Add(this);
	dl_hole = dl_thermal = NULL;
	bNeedsThermal = false;
}

cpin2::cpin2(CFreePcbDoc *_doc, int _uid):
	cpcb_item(_doc, _uid)
{
	part = NULL;
	ps = NULL;
	net = NULL;
	dl_hole = dl_thermal = NULL;
}

bool cpin2::IsValid()
{
	if (!part || !part->IsValid()) return false;
	return part->pins.Contains(this);
}

void cpin2::MustRedraw()
	// Override the default behavior of cpcb_item::MustRedraw()
	{ part->MustRedraw(); }

void cpin2::Disconnect() 
{
	// Detach pin from its part, and whichever net it's attached to.  Rip out any attached connections completely.
	part->pins.Remove(this);
	part = NULL;
	if (!net) return;
	citer<cconnect2> ic (&net->connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
		if (c->head->pin == this || c->tail->pin == this)
			c->Remove();
	net->pins.Remove(this);
	net = NULL;
}

int cpin2::GetWidth()													// CPT2. Derived from CPartList::GetPinWidth()
{
	ASSERT(part->shape!=NULL);
	int w = ps->top.size_h;
	w = max( w, ps->bottom.size_h );
	w = max( w, ps->hole_size );
	return w;
}

void cpin2::GetVtxs(carray<cvertex2> *vtxs)
{
	// CPT2 new.  Fill "vtxs" with the vertices that are associated with this pin
	vtxs->RemoveAll();
	if (!net) return;
	citer<cconnect2> ic (&net->connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
	{
		if (c->head->pin == this)
			vtxs->Add(c->head);
		if (c->tail->pin == this)
			vtxs->Add(c->tail);
	}
}

bool cpin2::TestHit( int _x, int _y, int _layer )
{
	// CPT2 Derived from CPartList::TestHitOnPad
	if( !part )
		return FALSE;
	if( !part->shape )
		return FALSE;

	cpad * p;
	if( ps->hole_size == 0 )
	{
		// SMT pad
		if( _layer == LAY_TOP_COPPER && part->side == 0 )
			p = &ps->top;
		else if( _layer == LAY_BOTTOM_COPPER && part->side == 1 )
			p = &ps->top;
		else
			return FALSE;
	}
	else
	{
		// TH pad
		if( _layer == LAY_TOP_COPPER && part->side == 0 )
			p = &ps->top;
		else if( _layer == LAY_TOP_COPPER && part->side == 1 )
			p = &ps->bottom;
		else if( _layer == LAY_BOTTOM_COPPER && part->side == 1 )
			p = &ps->top;
		else if( _layer == LAY_BOTTOM_COPPER && part->side == 0 )
			p = &ps->bottom;
		else
			p = &ps->inner;
	}
	double dx = abs( x - _x );
	double dy = abs( y - _y );
	double dist = sqrt( dx*dx + dy*dy );
	if( dist < ps->hole_size/2 )
		return true;

	switch( p->shape )
	{
	case PAD_NONE: 
		return false;
	case PAD_ROUND: 
		return dist < (p->size_h/2);
	case PAD_SQUARE:
		return dx < (p->size_h/2) && dy < (p->size_h/2);
	case PAD_RECT:
	case PAD_RRECT:
	case PAD_OVAL:
		int pad_angle = part->angle + ps->angle;
		if( pad_angle > 270 )
			pad_angle -= 360;
		if( pad_angle == 0 || pad_angle == 180 )
			return dx < (p->size_l) && dy < (p->size_h/2);
		else
			return dx < (p->size_h/2) && dy < (p->size_l);
	}
	return false;
}


void cpin2::SetPosition()												
{
	// CPT2 New, but derived from CPartList::GetPinPoint().
	// Derives correct values for this->(x,y) based on the part's position and on the padstack position.
	// NB Doesn't call MustRedraw().
	ASSERT( part->shape!=NULL );

	// get pin coords relative to part origin
	CPoint pp;
	pp.x = ps->x_rel;
	pp.y = ps->y_rel;
	// flip if part on bottom
	if( part->side )
		pp.x = -pp.x;
	// rotate if necess.
	int angle = part->angle;
	if( angle > 0 )
	{
		CPoint org (0,0);
		RotatePoint( &pp, angle, org );
	}
	// add coords of part origin
	x = part->x + pp.x;
	y = part->y + pp.y;

	SetNeedsThermal();
}

bool cpin2::SetNeedsThermal()
{
	// CPT2 new, but related to the old CNetList::SetAreaConnections.  Sets bNeedsThermal to true if some net area overlaps this pin.
	// CPT2 TODO needs an SMT check?
	bool bOldVal = bNeedsThermal;
	bNeedsThermal = false;
	if (net) 
	{
		citer<carea2> ia (&net->areas);
		for (carea2 *a = ia.First(); a; a = ia.Next())
			if (a->TestPointInside(x, y))
				{ bNeedsThermal = true; break; }
	}
	if (bNeedsThermal!=bOldVal)
		MustRedraw();
	return bNeedsThermal;
}

void cpin2::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	if( !dl_sel ) return;							// CPT2 Necessary check?  NB selector was created drawn by cpart2::Draw
	dl->Highlight( DL_RECT_X, 
				dl->Get_x(dl_sel), dl->Get_y(dl_sel),
				dl->Get_xf(dl_sel), dl->Get_yf(dl_sel), 
				1, pad_layer );
}

cseg2 *cpin2::AddRatlineToPin( cpin2 *p2 )
{
	// Add new connection to net, consisting of one unrouted segment
	// from this to p2.
	// CPT2 Adapted from old cnet::AddConnectFromPinToPin.  NOW RETURNS THE RATLINE SEG, AND DOES NO DRAWING.
	cpin2 *p1 = this;
	if (!p1->part || !p2->part)
		return NULL;								// Seems mighty unlikely.
	if (!p1->part->shape || !p2->part->shape)
		return NULL;								// Ditto

	// create connection with a single vertex
	cconnect2 *c = new cconnect2 (net);
	cvertex2 *v1 = new cvertex2 (c, p1->x, p1->y);
	c->Start(v1);
	c->AppendSegment(p2->x, p2->y, LAY_RAT_LINE, 0);
	v1->pin = p1;
	c->tail->pin = p2;
	return c->head->postSeg;
}

bool cpin2::GetDrawInfo(int layer,	bool bUse_TH_thermals, bool bUse_SMT_thermals,  int mask_clearance, int paste_mask_shrink,
					  int * type, int * w, int * l, int * r, int * hole,
					  int * connection_status, int * pad_connect_flag, int * clearance_type )
{
	// CPT2 derived from CPartList::GetPadDrawInfo().  Used during DRC to determine the pin's pad info on a particular layer.
	// Got rid of some args that are no longer useful.  In particular got rid of "angle" and am just returning directly the
	// correct width and length, depending on the angle.
	CShape * s = part->shape;
	if( !s )
		return false;

	// get pin and padstack info
	BOOL bUseDefault = FALSE; // if TRUE, use copper pad for mask
	int connect_status = GetConnectionStatus( layer );
	// set default return values for no pad and no hole
	bool ret_code = false;
	int ttype = PAD_NONE;
	int ww = 0;
	int ll = 0;
	int rr = 0;
	int angle = (ps->angle + part->angle) % 180;
	int hole_size = ps->hole_size;
	int clear_type = CLEAR_NORMAL;	
	int connect_flag = PAD_CONNECT_DEFAULT;

	// get pad info
	cpad * p = NULL;
	if( (layer == LAY_TOP_COPPER && part->side == 0 )
		|| (layer == LAY_BOTTOM_COPPER && part->side == 1 ) ) 
		// top copper pad is on this layer 
		p = &ps->top;
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
		// bottom copper pad is on this layer
		p = &ps->bottom;
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
		// inner pad is on this layer
		p = &ps->inner;

	// now set parameters for return
	if( p )
		connect_flag = p->connect_flag;
	if( p == NULL )
		// no pad definition, return defaults
		;
	else if( p->shape == PAD_NONE && ps->hole_size == 0 )
		// no hole, no pad, return defaults
		;
	else if( p->shape == PAD_NONE )
	{
		// hole, no pad
		ret_code = true;
		if( connect_status > ON_NET )
		{
			// connected to copper area or trace
			// make annular ring
			ret_code = true;
			ttype = PAD_ROUND;
			ww = 2*doc->m_annular_ring_pins + hole_size;
		}
		else if( ( layer == LAY_MASK_TOP || layer == LAY_MASK_BOTTOM ) && bUseDefault )
		{
			// if solder mask layer and no mask pad defined, treat hole as pad to get clearance
			ret_code = true;
			ttype = PAD_ROUND;
			ww = hole_size;
		}
	}
	else if( p->shape != PAD_NONE )
	{
		// normal pad
		ret_code = true;
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
		ww += 2*mask_clearance;
		ll += 2*mask_clearance;
		rr += mask_clearance;
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
			ASSERT(0);										// shouldn't happen, this is an error by GetConnectionStatus(...)
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

	// CPT2 Get length and width right, depending on angle (used to be done within DRC(), but I think it makes more sense here)
	if( ttype != PAD_RECT && ttype != PAD_RRECT && ttype != PAD_OVAL )
		ll = ww;
	int tmp;
	if( angle == 90 )
		tmp = ll, ll = ww, ww = tmp;

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
	if( connection_status )
		*connection_status = connect_status;
	if( pad_connect_flag )
		*pad_connect_flag = connect_flag;
	if( clearance_type )
		*clearance_type = clear_type;
	return ret_code;

}

int cpin2::GetConnectionStatus( int layer )
{
	// CPT2 derived from CPartList::GetPinConnectionStatus
	// checks to see if a pin is connected to a trace or a copper area on a
	// particular layer
	//
	// returns: ON_NET | TRACE_CONNECT | AREA_CONNECT
	// where:
	//		ON_NET = 1 if pin is on a net
	//		TRACE_CONNECT = 2 if pin connects to a trace
	//		AREA_CONNECT = 4 if pin connects to copper area
	if( !net )
		return NOT_CONNECTED;
	int status = ON_NET;

	// now check for traces
	citer<cconnect2> ic (&net->connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
	{
		if (c->head->pin == this && c->head->postSeg->m_layer == layer)
			{ status |= TRACE_CONNECT; break; }
		if (c->tail->pin == this && c->tail->preSeg->m_layer == layer)
			{ status |= TRACE_CONNECT; break; }
	}
	// now check for connection to copper area
	citer<carea2> ia (&net->areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		if (a->m_layer == layer && a->TestPointInside(x, y))
			{ status |= AREA_CONNECT; break; }

	return status;
}


cpart2::cpart2( cpartlist * pl )			// CPT2 TODO.  Will probably add more args...
	: cpcb_item (pl->m_doc)
{ 
	m_pl = pl;
	if (pl)
		pl->parts.Add(this);
	x = y = side = angle = 0;
	glued = false;
	m_ref = new creftext(this, 0, 0, 0, false, false, 0, 0, 0, doc->m_smfontutil, &CString(""), true);
	m_value = new cvaluetext(this, 0, 0, 0, false, false, 0, 0, 0, doc->m_smfontutil, &CString(""), false);
	m_tl = new ctextlist(doc);
	shape = NULL;
}

cpart2::cpart2(CFreePcbDoc *_doc, int _uid):
	cpcb_item(_doc, _uid)
{
	m_pl = NULL;
	m_ref = NULL;
	m_value = NULL;
	m_tl = new ctextlist(doc);
	shape = NULL;
}

bool cpart2::IsValid()
	{ return doc->m_plist->parts.Contains(this); }

void cpart2::SaveUndoInfo(bool bSaveAttachedNets)
{
	doc->m_undo_items.Add( new cupart(this) );
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		doc->m_undo_items.Add( new cupin(pin) );
		if (bSaveAttachedNets && pin->net)
			pin->net->SaveUndoInfo( cnet2::SAVE_CONNECTS );
	}
	doc->m_undo_items.Add( new cureftext(m_ref) );
	doc->m_undo_items.Add( new cuvaluetext(m_value) );
	citer<ctext> it (&m_tl->texts);
	for (ctext *t = it.First(); t; t = it.Next())
		doc->m_undo_items.Add( new cutext(t) );
}

void cpart2::Move( int _x, int _y, int _angle, int _side )
{
	// Move part (includes resetting pin positions and undrawing).  If _angle and/or _side are -1 (the default values), don't change 'em.
	MustRedraw();
	// move part
	x = _x;
	y = _y;
	if (_angle != -1)
		angle = _angle % 360;
	if (_side != -1)
		side = _side;
	// calculate new pin positions
	if( shape )
	{
		citer<cpin2> ip (&pins);
		for (cpin2* p = ip.First(); p; p = ip.Next())
			p->SetPosition();
	}
}

// Part moved, so unroute starting and ending segments of connections
// to this part, and update positions of endpoints
// CPT:  added dx and dy params whereby caller may indicate how much the part moved (both are 1 by default).  If, say, dx==0
// and an attached seg is vertical, then we don't have to unroute it.
// CPT2:  Derived from CNetList::PartMoved.

void cpart2::PartMoved( int dx, int dy )
{
	// We need to ensure that the part is redrawn first, so that the pins have correct x/y values (critical in what follows):
	doc->Redraw();

	// first, mark all nets unmodified
	doc->m_nlist->nets.SetUtility(0);

	// find nets that connect to this part, and unroute segments adjacent to pins.  
	// CPT2 I'll let the subroutines take care of calling MustUndraw() for the appropriate objects.
	citer<cpin2> ip (&pins);
	for (cpin2 *pin = ip.First(); pin; pin = ip.Next())
	{
		cnet2 *net = pin->net;
		if (!net || net->utility) continue;
		citer<cconnect2> ic (&net->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
		{
			if (c->segs.GetSize()==0) continue;				// Unlikely but hey...
			if (c->head->pin && c->head->pin->part == this)
			{
				cvertex2 *head = c->head;
				head->postSeg->Unroute( dx, dy, 0 );
				head->postSeg->MustRedraw();				// Even if it wasn't unrouted...
				head->x = head->pin->x;
				head->y = head->pin->y;
				net->utility = 1;	// mark net modified
				// CPT2 TODO decide if it's really safe to dump the following:
				/*
				if( part->shape->m_padstack[pin_index1].hole_size )
					// through-hole pad
					v0->pad_layer = LAY_PAD_THRU;
				else if( part->side == 0 && part->shape->m_padstack[pin_index1].top.shape != PAD_NONE
					|| part->side == 1 && part->shape->m_padstack[pin_index1].bottom.shape != PAD_NONE )
					// SMT pad on top
					v0->pad_layer = LAY_TOP_COPPER;
				else
					// SMT pad on bottom
					v0->pad_layer = LAY_BOTTOM_COPPER;
				if( part->pin[pin_index1].net != net )
					part->pin[pin_index1].net = net;
				*/
			}
			if (c->tail->pin && c->tail->pin->part == this)
			{
				// end pin is on part, unroute last segment
				cvertex2 *tail = c->tail;
				tail->preSeg->Unroute( dx, dy, 1 );
				tail->preSeg->MustRedraw();
				tail->x = tail->pin->x;
				tail->y = tail->pin->y;
				net->utility = 1;	// mark net modified
				// CPT2 TODO similar to issue in previous if-statement
			}
		}
	}
}

void cpart2::Remove(bool bEraseTraces, bool bErasePart)
{
	// CPT2 mostly new.  Implements my new system for deleting parts, where one can either erase all traces emanating from it, or not.
	// Also added bErasePart param (true by default);  calling Remove(true, false) allows one to delete attached traces only.
	if (bErasePart)
		Undraw(),
		m_pl->parts.Remove(this);

	// Now go through the pins.  If bEraseTraces, rip out connects attached to each one.  Otherwise, gather a list of vertices associated
	// with each one (such vertices will later get united into a tee).  TODO for each pin, consider maintaining a list of vtxs associated with it
	// at all times?
	citer<cpin2> ipin (&pins);
	carray<cvertex2> vtxs;
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		if (!pin->net) continue;
		pin->net->pins.Remove(pin);							// CPT2 Prevent phantom pins in nets!
		pin->net->MustRedraw();
		if (bEraseTraces)
		{
			citer<cconnect2> ic (&pin->net->connects);
			for (cconnect2 *c = ic.First(); c; c = ic.Next())
				if (c->head->pin==pin || c->tail->pin==pin)
					c->Remove();
		}
		else
		{
			// Make a list of vertices that are attached to the current pin.
			pin->GetVtxs(&vtxs);
			if (vtxs.IsEmpty()) continue;
			// Loop thru each vtx, and if the incoming seg is a ratline, erase it (and the vertex).  Otherwise, null out the vtx's "pin" member, and
			// assign it instead to a new tee structure.
			ctee *tee = new ctee(pin->net);
			citer<cvertex2> iv (&vtxs);
			for (cvertex2 *v = iv.First(); v; v = iv.Next())
				if (v->preSeg && v->preSeg->IsValid() && v->preSeg->m_layer == LAY_RAT_LINE)
					v->preSeg->RemoveBreak();
				else if (v->postSeg && v->postSeg->IsValid() && v->postSeg->m_layer == LAY_RAT_LINE)
					v->postSeg->RemoveBreak();
				else
					v->pin = NULL,
					tee->Add(v);
			tee->Adjust();
		}
	}
}

void cpart2::SetData(CShape * _shape, CString * _ref_des, CString * _value_text, CString * _package, 
					 int _x, int _y, int _side, int _angle, int _visible, int _glued  )
{
	// Derived from old CPartList::SetPartData.  Initializes data members, including "pins".  Ref and value-text related
	// members are initialized according to _shape if possible.
	// CPT2 Delete the following?
	// CDisplayList * old_dlist = m_dlist;
	// m_dlist = NULL;		// cancel further drawing

	// now copy data into part
	MustRedraw();
	visible = _visible;
	ref_des = *_ref_des;
	m_ref->m_str = *_ref_des;
	if (_value_text)
		value_text = m_value->m_str = *_value_text;
	else
		value_text = m_value->m_str = "";
	if( _package )
		package = *_package;
	else
		package = "";
	x = _x;
	y = _y;
	side = _side;
	angle = _angle;
	glued = _glued;

	if( !_shape )
	{
		shape = NULL;
		m_ref->Move(0, 0, 0, 0, 0);
		m_value->Move(0, 0, 0, 0, 0);
	}
	else
	{
		shape = _shape;
		InitPins();
		Move( x, y, angle, side );	// force setting pin positions

		ctext *txt = shape->m_ref;
		int layer = cpartlist::FootprintLayer2Layer( txt->m_layer );
		m_ref->Move( txt->m_x, txt->m_y, txt->m_angle, 
			false, false, layer, txt->m_font_size, txt->m_stroke_width );
		txt = shape->m_value;
		layer = cpartlist::FootprintLayer2Layer( txt->m_layer );
		m_value->Move( txt->m_x, txt->m_y, txt->m_angle, 
			false, false, layer, txt->m_font_size, txt->m_stroke_width );
		citer<ctext> it (&shape->m_tl->texts);
		for (txt = it.First(); txt; txt = it.Next())
		{
			layer = cpartlist::FootprintLayer2Layer( txt->m_layer );
			m_tl->AddText(txt->m_x, txt->m_y, txt->m_angle, txt->m_bMirror, txt->m_bNegative, 
							layer, txt->m_font_size, txt->m_stroke_width, &txt->m_str, this);
		}
	}

	m_outline_stroke.SetSize(0);
	m_ref->m_stroke.SetSize(0);
	m_value->m_stroke.SetSize(0);
}

void cpart2::SetValue(CString *_value, int x, int y, int angle, int size, int w, 
						 BOOL vis, int layer )
{
	value_text = *_value;
	m_value->m_layer = layer;
	m_value->Move(x, y, angle, size, w);
	m_value->m_bShown = vis;
}

/*
void cpart2::ResizeRefText(int size, int width, BOOL vis )
{
	m_ref->Move(m_ref->m_x, m_ref->m_y, m_ref->m_angle, size, width);
	m_ref->m_bShown = vis;
}
*/

CRect cpart2::CalcSelectionRect()
{
	CRect sel;
	if( !side )
	{
		// part on top
		sel.left = shape->m_sel_xi; 
		sel.right = shape->m_sel_xf;
		sel.bottom = shape->m_sel_yi;
		sel.top = shape->m_sel_yf;
	}
	else
	{
		// part on bottom
		sel.right = - shape->m_sel_xi;
		sel.left = - shape->m_sel_xf;
		sel.bottom = shape->m_sel_yi;
		sel.top = shape->m_sel_yf;
	}
	if( angle > 0 )
		RotateRect( &sel, angle, zero );
	sel.left += x;
	sel.right += x;
	sel.bottom += y;
	sel.top += y;
	return sel;
}

void cpart2::InitPins()
{
	// CPT2 New.  Initialize pins carray based on the padstacks in the shape.
	ASSERT(shape);
	citer<cpadstack> ips (&shape->m_padstack);
	for (cpadstack *ps = ips.First(); ps; ps = ips.Next())
		pins.Add( new cpin2(this, ps, NULL) );
}

cpin2 *cpart2::GetPinByName(CString *name)
{
	// CPT2 new.
	citer<cpin2> ip (&pins);
	for (cpin2 *p = ip.First(); p; p = ip.Next())
		if (p->pin_name == *name)
			return p;
	return NULL;
}


int cpart2::GetBoundingRect( CRect * part_r )
{
	// CPT2 TODO this function is kind of a loser, since it only works for drawn parts.  Phase it out in favor of CalcSelectionRect()...
	CRect r;
	if( !shape || !dl_sel ) 
		return 0;
	CDisplayList *dl = doc->m_dlist;
	r.left = min( dl->Get_x( dl_sel ), dl->Get_xf( dl_sel ) );
	r.right = max( dl->Get_x( dl_sel ), dl->Get_xf( dl_sel ) );
	r.bottom = min( dl->Get_y( dl_sel ), dl->Get_yf( dl_sel ) );
	r.top = max( dl->Get_y( dl_sel ), dl->Get_yf( dl_sel ) );
	*part_r = r;
	return 1;
}

CPoint cpart2::GetCentroidPoint()
{
	if( shape == NULL )
		ASSERT(0);
	// get coords relative to part origin
	CPoint pp;
	pp.x = shape->m_centroid->m_x;
	pp.y = shape->m_centroid->m_y;
	// flip if part on bottom
	if( side )
		pp.x = -pp.x;
	// rotate if necess.
	if( angle > 0 )
	{
		CPoint org (0, 0);
		RotatePoint( &pp, angle, org );
	}
	// add coords of part origin
	pp.x = x + pp.x;
	pp.y = y + pp.y;
	return pp;
}

// Get glue spot info from part
//
CPoint cpart2::GetGluePoint( cglue *g )
{
	if( shape == NULL )
		ASSERT(0);
	// get coords relative to part origin
	CPoint pp (g->x, g->y);
	// flip if part on bottom
	if( side )
		pp.x = -pp.x;
	// rotate if necess.
	if( angle > 0 )
	{
		CPoint org (0, 0);
		RotatePoint( &pp, angle, org );
	}
	// add coords of part origin
	pp.x += x; 
	pp.y += y;
	return pp;
}

bool cpart2::IsAttachedToConnects()
{
	// CPT2 new.  Return true if there are connects attached to any of this part's pins.
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		if (!pin->net) continue;
		citer<cconnect2> ic (&pin->net->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
			if (c->head->pin && c->head->pin->part==this)
				return true;
			else if (c->tail->pin && c->tail->pin->part==this)
				return true;
	}
	return false;
}

void cpart2::ChangeFootprint(CShape *_shape)
{
	// CPT2.  Loosely derived from CPartList::PartFootprintChanged && CNetList::PartFootprintChanged.
	// The passed-in shape is the one that will replace this->shape.  Setup pins corresponding to the new shape, reusing old pins where 
	// possible.  Mark used pins with
	// a utility value of 1.  Pins that are unused at the end get axed.
	MustRedraw();
	carray<cnet2> nets;									// Maintain a list of attached nets so that we can redraw them afterwards
	citer<cpin2> ip (&pins);
	for (cpin2 *p = ip.First(); p; p = ip.Next())
		if (p->net)
			nets.Add(p->net),
			p->net->MustRedraw();

	shape = _shape;
	pins.SetUtility(0);
	int cPins = shape->m_padstack.GetSize();
	carray<cvertex2> vtxs;
	citer<cpadstack> ips (&shape->m_padstack);
	for (cpadstack *ps = ips.First(); ps; ps = ips.Next())
	{
		cpin2 *old = GetPinByName(&ps->name);
		cpin2 *p;
		int oldX, oldY, oldLayer;
		if (old)
		{
			oldX = old->x, oldY = old->y;
			oldLayer = old->pad_layer;
			old->ps = ps;
			p = old;
			if( ps->hole_size ) 
				p->pad_layer = LAY_PAD_THRU;
			else if( side == 0 && ps->top.shape != PAD_NONE || side == 1 && ps->bottom.shape != PAD_NONE )
				p->pad_layer = LAY_TOP_COPPER;
			else
				p->pad_layer = LAY_BOTTOM_COPPER;
			p->SetPosition();
			bool bLayerChange = p->pad_layer!=oldLayer && p->pad_layer!=LAY_PAD_THRU;
			if (oldX != p->x || oldY != p->y || bLayerChange)
			{
				// Pin's position has changed.  Find vertices associated with it, and unroute their adjacent segs as needed
				p->GetVtxs(&vtxs);
				citer<cvertex2> iv (&vtxs);
				for (cvertex2 *v = iv.First(); v; v = iv.Next())
					if (v->preSeg && v->preSeg->m_layer!=LAY_RAT_LINE)
						v->preSeg->Unroute();
					else if (v->postSeg && v->postSeg->m_layer!=LAY_RAT_LINE)
						v->postSeg->Unroute();
			}
		}
		else
			p = new cpin2(this, ps, NULL),
			p->SetPosition();
		p->utility = 1;
	}

	// Look for disused pins (utility==0) and dump 'em.  NB Disconnect() rips out all attached traces completely.  TODO change or allow user an option to 
	// keep traces?
	for (cpin2 *p = ip.First(); p; p = ip.Next())
		if (!p->utility) 
			p->Disconnect();

	// Dump texts from the old footprint and add texts from the new one:
	m_tl->texts.RemoveAll();
	citer<ctext> it (&shape->m_tl->texts);
	for (ctext *txt = it.First(); txt; txt = it.Next()) 
	{
		int layer = cpartlist::FootprintLayer2Layer( txt->m_layer );
		m_tl->AddText(txt->m_x, txt->m_y, txt->m_angle, txt->m_bMirror, txt->m_bNegative, 
							layer, txt->m_font_size, txt->m_stroke_width, &txt->m_str, this);
	}

	// If old ref or value text had font-size 0, potentially we should change that (otherwise these can become permanently invisible)
	if (m_ref->m_font_size == 0)
		m_ref->m_font_size = shape->m_ref->m_font_size;
	if (m_value->m_font_size == 0)
		m_value->m_font_size = shape->m_value->m_font_size;
}

void cpart2::OptimizeConnections(BOOL bLimitPinCount, BOOL bVisibleNetsOnly )
{
	if (!shape) return;

	// find nets which connect to this part, and mark them all unoptimized
	citer<cpin2> ip (&pins);
	for (cpin2 *pin = ip.First(); pin; pin = ip.Next())
		if (pin->net)
			pin->net->utility = 0;

	// optimize each net and mark it optimized so it won't be repeated
	for (cpin2 *pin = ip.First(); pin; pin = ip.Next())
		if (pin->net && !pin->net->utility)
			pin->net->OptimizeConnections(bLimitPinCount, bVisibleNetsOnly),
			pin->net->utility = 1;
}

int cpart2::Draw()
{
	// CPT2 TODO:  Think about the CDisplayList arg business
	CDisplayList *dl = doc->m_dlist;
	if( !dl )
		return NO_DLIST;
	if( !shape )
		return NO_FOOTPRINT;
	if (bDrawn)
		return ALREADY_DRAWN;

	// draw selection rectangle (layer = top or bottom copper, depending on side)
	CRect sel = CalcSelectionRect();
	dl_sel = dl->AddSelector( this, LAY_SELECTION, DL_HOLLOW_RECT, 1,
		0, 0, sel.left, sel.bottom, sel.right, sel.top, x, y );
	/* dl->Set_sel_vert( dl_sel, 0 );											// CPT2 TODO this function appears axeable
	if( angle == 90 || angle ==  270 )
		dl->Set_sel_vert( dl_sel, 1 ); */

	// draw ref designator text
	m_ref->dl_el = m_ref->dl_sel = NULL;
	if( m_ref->m_bShown && m_ref->m_font_size )
		m_ref->DrawRelativeTo(this);
	else
		m_ref->Undraw();
	
	// draw value text
	m_value->dl_el = m_value->dl_sel = NULL;
	if( m_value->m_bShown && m_value->m_font_size )
		m_value->DrawRelativeTo(this);
	else
		m_value->Undraw();

	// draw part outline (code similar to but sadly not identical to cpolyline::Draw())
	CPoint si, sf;
	m_outline_stroke.SetSize(0);
	citer<coutline> io (&shape->m_outline_poly);
	for (cpolyline *poly = io.First(); poly; poly = io.Next())
	{
		int shape_layer = poly->GetLayer();
		int poly_layer = cpartlist::FootprintLayer2Layer( shape_layer );
		poly_layer = FlipLayer( side, poly_layer );
		int w = poly->W();
		citer<ccontour> ictr (&poly->contours);
		for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		{
			// Draw polyline sides
			citer<cside> is (&ctr->sides);
			for (cside *s = is.First(); s; s = is.Next())
			{
				si.x = s->preCorner->x, si.y = s->preCorner->y;
				sf.x = s->postCorner->x, sf.y = s->postCorner->y;
				if( si.x == sf.x || si.y == sf.y )
					// if endpoints not angled, make side STRAIGHT
					s->m_style = STRAIGHT;
				int g_type = DL_LINE;
				if( s->m_style == STRAIGHT )
					g_type = DL_LINE;
				else if( s->m_style == ARC_CW )
					g_type = DL_ARC_CW;
				else if( s->m_style == ARC_CCW )
					g_type = DL_ARC_CCW;
				// flip if part on bottom
				if( side )
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
				stroke str;
				str.layer = poly_layer;
				str.xi = x+si.x;
				str.xf = x+sf.x;
				str.yi = y+si.y;
				str.yf = y+sf.y;
				str.type = g_type;
				str.w = w;
				str.dl_el = dl->AddMain( this, poly_layer,					// CPT2 TODO Is AddMain() appropriate?
					g_type, 1, w, 0, 0, x+si.x, y+si.y, x+sf.x, y+sf.y, 0, 0 );
				m_outline_stroke.Add(str);
			}
		}
	}

	// draw footprint texts
	citer<ctext> it (&m_tl->texts);
	for (ctext *t = it.First(); t; t = it.Next())
		t->DrawRelativeTo(this, false);

	// draw padstacks and save absolute position of pins
	CPoint pin_pt;
	CPoint pad_pi;
	CPoint pad_pf;
	int nLayers = doc->m_plist->m_layers;
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next()) 
	{
		// CPT2 TODO check:  is pin->x/y always getting set correctly?
		// set layer for pads
		cpadstack * ps = pin->ps;
		pin->dl_els.SetSize(nLayers);
		cpad * p;
		int pad_layer;
		// iterate through all copper layers 
		for( int il=0; il<nLayers; il++ )
		{
			pin_pt.x = ps->x_rel;
			pin_pt.y = ps->y_rel;
			pad_layer = il + LAY_TOP_COPPER;
			pin->dl_els[il] = NULL;
			// get appropriate pad
			cpad * p = NULL;
			if( pad_layer == LAY_TOP_COPPER && side == 0 )
				p = &ps->top;
			else if( pad_layer == LAY_TOP_COPPER && side == 1 )
				p = &ps->bottom;
			else if( pad_layer == LAY_BOTTOM_COPPER && side == 0 )
				p = &ps->bottom;
			else if( pad_layer == LAY_BOTTOM_COPPER && side == 1 )
				p = &ps->top;
			else if( ps->hole_size )
				p = &ps->inner;
			int sel_layer = pad_layer;
			if( ps->hole_size )
				sel_layer = LAY_SELECTION;
			if( p )
			{
				// draw pad
				dl_element * pad_el = NULL;
				if( p->shape == PAD_NONE )
					;
				else if( p->shape == PAD_ROUND )
				{
					// flip if part on bottom
					if( side )
						pin_pt.x = -pin_pt.x;
					// rotate
					if( angle > 0 )
						RotatePoint( &pin_pt, angle, zero );
					// add to display list
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = dl->AddMain( pin, pad_layer, 
						DL_CIRC, 1, 
						p->size_h, 0, 0,
						x + pin_pt.x, y + pin_pt.y, 0, 0, 0, 0 );
					if( !pin->dl_sel )
						pin->dl_sel = dl->AddSelector( pin, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							pin->x-p->size_h/2,	pin->y-p->size_h/2, 
							pin->x+p->size_h/2,	pin->y+p->size_h/2, 0, 0 );
				}

				else if( p->shape == PAD_SQUARE )
				{
					// flip if part on bottom
					if( side )
						pin_pt.x = -pin_pt.x;
					// rotate
					if( angle > 0 )
						RotatePoint( &pin_pt, angle, zero );
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = dl->AddMain( pin, pad_layer, 
						DL_SQUARE, 1, 
						p->size_h,
						0, 0,
						pin->x, pin->y, 0, 0, 0, 0 );
					if( !pin->dl_sel )
						pin->dl_sel = dl->AddSelector( pin, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							pin->x-p->size_h/2,	pin->y-p->size_h/2, 
							pin->x+p->size_h/2,	pin->y+p->size_h/2, 0, 0 );
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
					if( ps->angle > 0 )
					{
						RotatePoint( &pad_pi, ps->angle, pin_pt );
						RotatePoint( &pad_pf, ps->angle, pin_pt );
					}
					// flip if part on bottom
					if( side )
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
					int radius = p->radius;
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = dl->AddMain( pin, pad_layer, 
						gtype, 1, 
						0, 0, 0, 
						x + pad_pi.x, y + pad_pi.y, 
						x + pad_pf.x, y + pad_pf.y, 
						x + pin_pt.x, y + pin_pt.y, 
						p->radius );
					if( !pin->dl_sel )
						pin->dl_sel = dl->AddSelector( pin, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							x + pad_pi.x, y + pad_pi.y, 
							x + pad_pf.x, y + pad_pf.y,
							0, 0 );
				}

				else if( p->shape == PAD_OCTAGON )
				{
					// flip if part on bottom
					if( side )
						pin_pt.x = -pin_pt.x;
					// rotate
					if( angle > 0 )
						RotatePoint( &pin_pt, angle, zero );
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = dl->AddMain( pin, pad_layer, 
						DL_OCTAGON, 1, 
						p->size_h, 0, 0,
						pin->x, pin->y, 0, 0, 0, 0 );
					if( !pin->dl_sel )
						pin->dl_sel = dl->AddSelector( pin, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							pin->x-p->size_h/2, pin->y-p->size_h/2, 
							pin->x+p->size_h/2, pin->y+p->size_h/2, 0, 0 );
				}

				pin->dl_els[il] = pad_el;
				pin->dl_hole = pad_el;
			}
		}
		
		// if through-hole pad, just draw hole and set pin->dl_hole
		if( ps->hole_size )
		{
			pin_pt.x = ps->x_rel;
			pin_pt.y = ps->y_rel;
			// flip if part on bottom
			if( side )
				pin_pt.x = -pin_pt.x;
			// rotate
			if( angle > 0 )
				RotatePoint( &pin_pt, angle, zero );
			// add to display list
			pin->x = x + pin_pt.x;
			pin->y = y + pin_pt.y;
			pin->dl_hole = dl->AddMain( pin, LAY_PAD_THRU, 
								DL_HOLE, 1,	
								ps->hole_size, 0, 0,
								pin->x, pin->y, 0, 0, 0, 0 );  
			if( !pin->dl_sel )
				// make selector for pin with hole only
				pin->dl_sel = dl->AddSelector( pin, LAY_SELECTION, 
					DL_HOLLOW_RECT, 1, 1, 0,
					pin->x-ps->hole_size/2, pin->y-ps->hole_size/2,  
					pin->x+ps->hole_size/2, pin->y+ps->hole_size/2,  
					0, 0 );
		}
		else
			pin->dl_hole = NULL;

		if (pin->bNeedsThermal)
			pin->dl_thermal = dl->Add( this, dl_element::DL_OTHER, LAY_RAT_LINE, DL_X, 1,
				2*ps->hole_size/3, 0, 0, pin->x, pin->y, 0, 0, 0, 0 );
		else
			pin->dl_thermal = NULL;
	}

	bDrawn = true;
	return NOERR;
}

void cpart2::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl || !IsDrawn() ) return;
	if (!shape)
		{ bDrawn = false; return; }

	// undraw selection rectangle
	dl->Remove( dl_sel );
	dl_sel = 0;

	// undraw ref text and value text
	m_ref->Undraw();
	m_value->Undraw();

	// undraw part outline (this also includes footprint free text)
	for( int i=0; i<m_outline_stroke.GetSize(); i++ )
	{
		dl->Remove( m_outline_stroke[i].dl_el );
		m_outline_stroke[i].dl_el = 0;
	}

	// undraw footprint texts
	citer<ctext> it (&m_tl->texts);
	for (ctext *t = it.First(); t; t = it.Next())
		t->Undraw();

	// undraw padstacks
	citer<cpin2> ip (&pins);
	for (cpin2 *pin = ip.First(); pin; pin = ip.Next())
	{
		if( pin->dl_els.GetSize()>0 )
		{
			for( int il=0; il<pin->dl_els.GetSize(); il++ )
				if( pin->dl_els[il] != pin->dl_hole )
					dl->Remove( pin->dl_els[il] );
			pin->dl_els.RemoveAll();
		}
		dl->Remove( pin->dl_hole );
		dl->Remove( pin->dl_sel );
		dl->Remove( pin->dl_thermal );
		pin->dl_hole = pin->dl_sel = pin->dl_thermal = NULL;
	}

	bDrawn = false;
}

void cpart2::Highlight( )
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	if (!shape) return;
	dl->Highlight( DL_HOLLOW_RECT, 
				dl->Get_x( dl_sel ), dl->Get_y( dl_sel ),
				dl->Get_xf( dl_sel ), dl->Get_yf( dl_sel ), 1 );

	// Also highlight ref and value texts if possible
	if (dl_element *ref_sel = m_ref->dl_sel)
		dl->Highlight( DL_HOLLOW_RECT, 
				dl->Get_x( ref_sel ), dl->Get_y( ref_sel ),
				dl->Get_xf( ref_sel ), dl->Get_yf( ref_sel ), 1 );
	if (dl_element *val_sel = m_value->dl_sel)
		dl->Highlight( DL_HOLLOW_RECT, 
				dl->Get_x( val_sel ), dl->Get_y( val_sel ),
				dl->Get_xf( val_sel ), dl->Get_yf( val_sel ), 1 );
	
}

void cpart2::SetVisible(bool bVisible)
{
	// CPT2, derived from CPartList::MakePartVisible()
	// Make part visible or invisible, including thermal reliefs
	// outline strokes
	for( int i=0; i<m_outline_stroke.GetSize(); i++ )
	{
		dl_element * el = m_outline_stroke[i].dl_el;
		el->visible = bVisible;
	}
	// pins
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		// pin pads
		if (pin->dl_hole)
			pin->dl_hole->visible = bVisible;
		for( int i=0; i < pin->dl_els.GetSize(); i++ )
			if( pin->dl_els[i] )
				pin->dl_els[i]->visible = bVisible;
		if (pin->dl_thermal)
			pin->dl_thermal->visible = bVisible;
	}
	m_ref->SetVisible( bVisible );
	m_value->SetVisible( bVisible );
	// CPT2 TODO also set visibility for texts within footprint
}


void cpart2::StartDragging( CDC * pDC, BOOL bRatlines, BOOL bBelowPinCount, int pin_count ) 
{
	// CPT2, derived from CPartList::StartDraggingPart
	// make part invisible
	CDisplayList *dl = doc->m_dlist;
	SetVisible( FALSE );
	dl->CancelHighlight();

	// create drag lines
	CPoint zero(0,0);
	dl->MakeDragLineArray( 2*pins.GetSize() + 4 );
	int xi = shape->m_sel_xi;
	int xf = shape->m_sel_xf;
	if( side )
		xi = -xi,
		xf = -xf;
	int yi = shape->m_sel_yi;
	int yf = shape->m_sel_yf;
	CPoint p1( xi, yi );
	CPoint p2( xf, yi );
	CPoint p3( xf, yf );
	CPoint p4( xi, yf );
	RotatePoint( &p1, angle, zero );
	RotatePoint( &p2, angle, zero );
	RotatePoint( &p3, angle, zero );
	RotatePoint( &p4, angle, zero );
	dl->AddDragLine( p1, p2 ); 
	dl->AddDragLine( p2, p3 ); 
	dl->AddDragLine( p3, p4 ); 
	dl->AddDragLine( p4, p1 );
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		// CPT2 old code used to recalculate pin's x/y-position.  I'm going to try relying on the pin's preexisting x/y member values.  Note
		// that these values must be made relative to the part origin, however.
		// make X for each pin
		int d = pin->ps->top.size_h/2;
		int xRel = pin->x - x, yRel = pin->y - y;
		CPoint p1 (xRel - d, yRel - d);
		CPoint p2 (xRel + d, yRel - d);
		CPoint p3 (xRel + d, yRel + d);
		CPoint p4 (xRel - d, yRel + d);
		// draw X
		dl->AddDragLine( p1, p3 ); 
		dl->AddDragLine( p2, p4 );
	}

	// create drag lines for ratlines (OR OTHER SEGS --- CPT2) connected to pins
	if( bRatlines ) 
	{
		dl->MakeDragRatlineArray( 2*pins.GetSize(), 1 );
		// CPT2 Make a list of segs that have an endpoint (or 2) attached to one of the part's pins.  For efficiency, don't scan any net more than once
		m_pl->m_nlist->nets.SetUtility(0);
		carray<cseg2> segs;
		for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
		{
			if (!pin->net) continue;
			if (pin->net->utility) continue;
			pin->net->utility = 1;
			citer<cconnect2> ic (&pin->net->connects);
			for (cconnect2 *c = ic.First(); c; c = ic.Next())
			{
				if (c->head->pin && c->head->pin->part == this)
					segs.Add(c->head->postSeg);
				if (c->tail->pin && c->tail->pin->part == this)
					segs.Add(c->tail->preSeg);
			}
		}
		// Loop through the new list of segs that are attached to part's pins:
		citer<cseg2> is (&segs);
		for (cseg2 *s = is.First(); s; s = is.Next())
		{
			s->SetVisible(false);
			// CPT2 old code spent effort worrying about hiding vias on the end of the seg opposite from the pin.  
			// I'm going to try ignoring that issue...
			// As in the old code, I'll suppress drag-ratlines if the seg has both ends attached to the part's pins.  The old code
			// suppressed segs that belonged to single-seg connects with a loose end.  I'm going to try ignoring that issue too:
			CPoint vtxPt, pinPt;
			if (s->preVtx->pin && s->preVtx->pin->part==this)
				if (s->postVtx->pin && s->postVtx->pin->part==this)
					continue;												// Both ends of s are pins on "this"...
				else
					vtxPt = CPoint (s->postVtx->x, s->postVtx->y),
					pinPt = CPoint (s->preVtx->x - x, s->preVtx->y - y);	// NB coords relative to part
			else if (s->postVtx->pin && s->postVtx->pin->part==this)
				vtxPt = CPoint (s->preVtx->x, s->preVtx->y),
				pinPt = CPoint (s->postVtx->x - x, s->postVtx->y - y);		// NB coords relative to part
			dl->AddDragRatline( vtxPt, pinPt );
		}
	}
	
	int vert = 0;
	if( angle == 90 || angle == 270 )
		vert = 1;
	dl->StartDraggingArray( pDC, x, y, vert, LAY_SELECTION );
}

void cpart2::CancelDragging()
{
	// CPT2 derived from CPartList::CancelDraggingPart()
	// make part visible again
	SetVisible(true);

	// get any connecting segments and make visible
	CDisplayList *dl = doc->m_dlist;
	m_pl->m_nlist->nets.SetUtility(0);
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		if (!pin->net || !pin->net->bVisible || pin->net->utility)
			continue;
		pin->net->utility = 1;
		citer<cconnect2> ic (&pin->net->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
		{
			if (c->head->pin && c->head->pin->part==this)
				c->head->postSeg->SetVisible(true);
			if (c->tail->pin && c->tail->pin->part==this)
				c->tail->preSeg->SetVisible(true);
		}
	}
	dl->StopDragging();
}

void cpart2::MakeString( CString * str )
{
	// CPT2 derived from old CPartList::SetPartString().  Used when saving to file, etc.
	CString line;
	line.Format( "part: %s\n", ref_des );  
	*str = line;
	if( shape )
		line.Format( "  ref_text: %d %d %d %d %d %d %d\n", 
					m_ref->m_font_size, m_ref->m_stroke_width, m_ref->m_angle%360,
					m_ref->m_x, m_ref->m_y, m_ref->m_bShown,m_ref->m_layer );
	else
		line.Format( "  ref_text: \n" );
	str->Append( line );
	line.Format( "  package: \"%s\"\n", package );
	str->Append( line );
	if( value_text != "" ) 
	{
		if( shape )
			line.Format( "  value: \"%s\" %d %d %d %d %d %d %d\n", 
				value_text, m_value->m_font_size, m_value->m_stroke_width, m_value->m_angle%360,
				m_value->m_x, m_value->m_y, m_value->m_bShown, m_value->m_layer );
		else
			line.Format( "  value: \"%s\"\n", value_text );
		str->Append( line );
	}
	if( shape )
		line.Format( "  shape: \"%s\"\n", shape->m_name );
	else
		line.Format( "  shape: \n" );
	str->Append( line );
	line.Format( "  pos: %d %d %d %d %d\n", x, y, side, angle%360, glued );
	str->Append( line );

	line.Format( "\n" );
	str->Append( line );
}

void cpart2::GetDRCInfo()
{
	// CPT2 new, extracted from parts of the old CPartList::DRC().  Fill in the drc-related fields for this part and for all its pins.
	if (!shape)
		return;
	hole_flag = FALSE;
	min_x = INT_MAX;
	max_x = INT_MIN;
	min_y = INT_MAX;
	max_y = INT_MIN;
	// CPT2 NB the "layers" bitmask used to be the "or" of 1<<(layer-LAY_TOP_COPPER) for each layer touched by the part.  I've gotten rid of the
	// "-LAY_TOP_COPPER" business.  Since there are a max of 32 layers anyway, shouldn't be a problem with bit overflow.
	layers = 0;

	// iterate through copper graphics elements
	for( int igr = 0; igr < m_outline_stroke.GetSize(); igr++ )
	{
		stroke * stk = &m_outline_stroke[igr];
		if( stk->layer >= LAY_TOP_COPPER )
		{
			layers |= 1<<(stk->layer);
			min_x = min( min_x, stk->xi - stk->w/2 );
			max_x = max( max_x, stk->xi + stk->w/2 );
			min_x = min( min_x, stk->xf - stk->w/2 );
			max_x = max( max_x, stk->xf + stk->w/2 );
			min_y = min( min_y, stk->yi - stk->w/2 );
			max_y = max( max_y, stk->yi + stk->w/2 );
			min_y = min( min_y, stk->yf - stk->w/2 );
			max_y = max( max_y, stk->yf + stk->w/2 );
		}
	}

	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		drc_pin * drp = &pin->drc;
		int x = pin->x, y = pin->y;

		drp->hole_size = 0;
		drp->min_x = INT_MAX;
		drp->max_x = INT_MIN;
		drp->min_y = INT_MAX;
		drp->max_y = INT_MIN;
		drp->max_r = INT_MIN;
		drp->layers = 0;
		int hole = pin->ps->hole_size;
		if (hole)
		{
			drp->hole_size = hole;
			drp->min_x = min( drp->min_x, x - hole/2 );
			drp->max_x = max( drp->max_x, x + hole/2 );
			drp->min_y = min( drp->min_y, y - hole/2 );
			drp->max_y = max( drp->max_y, y + hole/2 );
			drp->max_r = max( drp->max_r, hole/2 );
			min_x = min( min_x, x - hole/2 );
			max_x = max( max_x, x + hole/2 );
			min_y = min( min_y, y - hole/2 );
			max_y = max( max_y, y + hole/2 );
			hole_flag = TRUE;
		}
		int num_layers = LAY_TOP_COPPER + doc->m_num_copper_layers;
		for( int layer = LAY_TOP_COPPER; layer < num_layers; layer++ )
		{
			// Check pads on each layer.
			int wid, len, r, type, hole, connect;
			BOOL bPad = pin->GetDrawInfo( layer, 0, 0, 0, 0,
				&type, &wid, &len, &r, &hole, &connect );
			if (!bPad || type == PAD_NONE )
				continue;
			drp->min_x = min( drp->min_x, x - len/2 );
			drp->max_x = max( drp->max_x, x + len/2 );
			drp->min_y = min( drp->min_y, y - wid/2 );
			drp->max_y = max( drp->max_y, y + wid/2 );
			drp->max_r = max( drp->max_r, Distance( 0, 0, len/2, wid/2 ) );
			min_x = min( min_x, x - len/2 );
			max_x = max( max_x, x + len/2 );
			min_y = min( min_y, y - wid/2 );
			max_y = max( max_y, y + wid/2 );
			drp->layers |= 1<<layer;
			layers |= 1<<layer;
		}
	}
}



cpartlist::cpartlist( CFreePcbDoc *_doc )
{
	m_doc = _doc;
	m_dlist = m_doc->m_dlist;
	m_layers = m_doc->m_num_copper_layers;
	m_annular_ring = m_doc->m_annular_ring_pins;					// (CPT2 TODO check)
	m_nlist = NULL;
	m_footprint_cache_map = NULL;
}

cpart2 *cpartlist::GetPartByName( CString *ref_des )
{
	// Find part in partlist by ref-des
	citer<cpart2> ip (&parts);
	for (cpart2 *p = ip.First(); p; p = ip.Next())
		if (p->ref_des == *ref_des)
			return p;
	return NULL;
}

cpin2 * cpartlist::GetPinByNames ( CString *ref_des, CString *pin_name)
{
	cpart2 *part = GetPartByName(ref_des);
	if (!part) return NULL;
	citer<cpin2> ip (&part->pins);
	for (cpin2 *p = ip.First(); p; p = ip.Next())
		if (p->pin_name == *pin_name)
			return p;
	return NULL;
}

// get bounding rectangle of parts
// return 0 if no parts found, else return 1
//
int cpartlist::GetPartBoundaries( CRect * part_r )
{
	int min_x = INT_MAX;
	int max_x = INT_MIN;
	int min_y = INT_MAX;
	int max_y = INT_MIN;
	int parts_found = 0;
	
	citer<cpart2> ip (&parts);
	for (cpart2 *part = ip.First(); part; part = ip.Next())
	{
		if( part->dl_sel )
		{
			// TODO the ugly Get_x(), Get_y()... functions might be rethought some day
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
		if( dl_element *ref_sel = part->m_ref->dl_sel )
		{
			int x = m_dlist->Get_x( ref_sel );
			int y = m_dlist->Get_y( ref_sel );
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			x = m_dlist->Get_xf( ref_sel );
			y = m_dlist->Get_yf( ref_sel );
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			parts_found = 1;
		}
	}
	part_r->left = min_x;
	part_r->right = max_x;
	part_r->bottom = min_y;
	part_r->top = max_y;
	return parts_found;
}


void cpartlist::SetThermals()
{
	// CPT2 new.  Updates the bNeedsThermal flags for all pins in all parts.
	citer<cpart2> ip (&parts);
	for (cpart2 *p = ip.First(); p; p = ip.Next())
	{
		citer<cpin2> ipin (&p->pins);
		for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
			pin->SetNeedsThermal();
	}
}

cpart2 *cpartlist::Add( CShape * shape, CString * ref_des, CString *value_text, CString * package, 
	int x, int y, int side, int angle, int visible, int glued )
{
	// CPT2.  Derived from old cpartlist func.  Removed uid and ref_vis args.
	cpart2 *part = new cpart2(this);
	part->SetData(shape, ref_des, value_text, package, x, y, side, angle, visible, glued);
	return part;
}

cpart2 * cpartlist::AddFromString( CString * str )
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
	bool ref_specified = false;				// CPT2
	int ref_size = 0;
	int ref_width = 0;
	int ref_angle = 0;
	int ref_xi = 0;
	int ref_yi = 0;
	int ref_layer = LAY_SILK_TOP;
	CString value;
	BOOL value_vis = TRUE;
	bool value_specified = false;			// CPT2
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
			ref_specified = true;					// CPT2
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
			value_specified = true;					// CPT2
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
			if ( m_footprint_cache_map->Lookup( name, ptr ) )
				// found in cache
				s = (CShape*)ptr; 
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

	cpart2 * part = new cpart2(this);
	part->SetData( s, &ref_des, &value, &package, x, y, side, angle, 1, glued );			// CPT2.  Also initializes pins and puts ref and value
																							// into the positions indicated by the shape
	part->m_ref->m_bShown = ref_vis;
	if (ref_specified)
		part->m_ref->Move(ref_xi, ref_yi, ref_angle,
				false, false, ref_layer, ref_size, ref_width);
	part->m_value->m_bShown = value_vis;
	if (value_specified)
		part->m_value->Move(value_xi, value_yi, value_angle, 
				false, false, value_layer, value_size, value_width );
	part->MustRedraw();																			// CPT2. TODO is this the place to do this?
	return part;
}


int cpartlist::ReadParts( CStdioFile * pcb_file )
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
		if (!pcb_file->ReadString( in_str ))
			throw new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
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
				if (!pcb_file->ReadString( in_str ))
					throw new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
				in_str.Trim();
			} while( (in_str.Left(4) != "ref:" && in_str.Left(5) != "part:" )
						&& in_str[0] != '[' );
			pcb_file->Seek( pos, CFile::begin );

			// now add part to partlist.  (AddFromString() also calls MustRedraw().)
			cpart2 * part = AddFromString( &str );
		}
	}
	return 0;
}

int cpartlist::WriteParts( CStdioFile * file )
{
	// CPT2. Converted.
	// CPT: Sort the part lines.
	try
	{
		// now write all parts
		CString line;
		CArray<CString> lines;
		line.Format( "[parts]\n\n" );
		file->WriteString( line );
		citer<cpart2> ip (&parts);
		for (cpart2 * p = ip.First(); p; p = ip.Next())
			p->MakeString( &line ),
			lines.Add(line);
		extern int CompareNumeric(CString *s1, CString *s2);															// In utility.cpp
		qsort(lines.GetData(), lines.GetSize(), sizeof(CString), (int (*)(const void*,const void*)) CompareNumeric);			
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


int cpartlist::GetNumFootprintInstances( CShape * shape )
{
	int n = 0;
	citer<cpart2> ip (&parts);
	for (cpart2 *part = ip.First(); part; part = ip.Next())
		if(  part->shape == shape  )
			n++;
	return n;
}


int cpartlist::FootprintLayer2Layer( int fp_layer )
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

int cpartlist::ExportPartListInfo( partlist_info * pli, cpart2 *part0 )
{
	// CPT2 converted.  
	// export part list data into partlist_info structure for editing in dialog.
	// Returns the index of part0's info within pli (-1 if not found)
	pli->SetSize( parts.GetSize() );
	int i = 0, ret = -1;
	citer<cpart2> ip (&parts);
	for (cpart2 *part = ip.First(); part; part = ip.Next())
	{
		part_info *pi = &(*pli)[i];
		pi->part = part;
		if (part==part0) ret = i;
		pi->shape = part->shape;
		pi->bShapeChanged = FALSE;
		pi->ref_des = part->ref_des;
		pi->ref_vis = part->m_ref->m_bShown;
		pi->ref_layer = part->m_ref->m_layer;					// CPT2 added this for consistency
		pi->ref_size = part->m_ref->m_font_size;				// CPT2 TODO why do we have ref size/width in partlistinfo, but not value size/width?
		pi->ref_width = part->m_ref->m_stroke_width;
		pi->package = part->package;
		pi->value = part->value_text;
		pi->value_vis = part->m_value->m_bShown;
		pi->value_layer = part->m_value->m_layer;
		pi->x = part->x;
		pi->y = part->y;
		pi->angle = part->angle;
		pi->side = part->side;
		pi->deleted = FALSE;
		pi->bOffBoard = FALSE;
		i++;
	}
	return ret;
}

void cpartlist::ImportPartListInfo( partlist_info * pli, int flags, CDlgLog * log )
{
	// CPT2 converted.  Routine relies on lower-level subroutines to call Undraw() or MustRedraw(), as appropriate, for individual parts and 
	// attached nets.
	CString mess; 

	// grid for positioning parts off-board
	int pos_x = 0;
	int pos_y = 0;
	enum { GRID_X = 100, GRID_Y = 50 };
	BOOL * grid = (BOOL*)calloc( GRID_X*GRID_Y, sizeof(BOOL) );
	int grid_num = 0;

	// first, look for parts in project whose ref_des has been changed
	/* CPT2 obsolete
	for( int i=0; i<pli->GetSize(); i++ )
	{
		part_info * pi = &(*pli)[i];
		if( pi->part )
		{
			if( pi->ref_des != pi->part->ref_des )
			{
				m_nlist->PartRefChanged( &pi->part->ref_des, &pi->ref_des );
				pi->part->ref_des = pi->ref_des;
			}
		}
	}
	*/

	// CPT2 when importing from netlist file,  go through and try to find pre-existing parts with the names indicated 
	// in pli's entries, and enter them into the part_info::part field.
	if (flags & IMPORT_FROM_NETLIST_FILE)
		for (int i=0; i<pli->GetSize(); i++)
		{
			part_info *pi = &(*pli)[i];
			if (pi->part) continue;
			pi->part = GetPartByName( &pi->ref_des );
		}

	// now find parts in project that are not in partlist_info
	// loop through all parts in project
	citer<cpart2> ip (&parts);
	for (cpart2 *part = ip.First(); part; part = ip.Next())
	{
		// loop through the partlist_info array
		BOOL bFound = FALSE;
		part->bPreserve = FALSE;
		for( int i=0; i<pli->GetSize(); i++ )
		{
			part_info * pi = &(*pli)[i];
			if( pi->part == part )
			{
				// part exists in partlist_info.  CPT2 TODO Before this check was based on comparing ref-designators, but I think this
				// check will work better in case we're changing the part's ref-des only.
				bFound = TRUE;
				break;
			}
		}
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
				// CPT2 I would have thought it's more useful to delete the part and keep the connections.  But anyway,
				// I'll make it do what it did before...  TODO provide the opposite option.
				// keep part but remove connections from netlist
				if( log )
				{
					CString s ((LPCSTR) IDS_KeepingPartButRemovingConnections);
					mess.Format( s, part->ref_des );
					log->AddLine( mess );
				}
				part->Remove(true, false);
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
				part->Remove(true);			// CPT2 I guess...
			}
		}
	}

	// loop through partlist_info array, changing partlist as necessary
	for( int i=0; i<pli->GetSize(); i++ )
	{
		part_info * pi = &(*pli)[i];
		if( pi->part == 0 && pi->deleted )
			// new part was added but then deleted, ignore it
			continue;
		if( pi->part != 0 && pi->deleted )
		{
			// old part was deleted, remove it
			pi->part->Remove(true);
			continue;
		}

		if( pi->part && !pi->part->shape && (pi->shape || pi->bShapeChanged == TRUE) )
		{
			// old part exists, but it had a null footprint that has now changed.
			// Remove old part (we'll create the new one from scratch)
			pi->part->Remove(true);
			pi->part = NULL;
		}

		if( pi->part == 0 )
		{
			// the partlist_info does not include a pointer to an existing part
			// the part might not exist in the project
			cpart2 * old_part = GetPartByName( &pi->ref_des );
			if( old_part && old_part->shape)
			{
				// an existing part has the same ref_des as the new part, and has a footprint
				// see if the incoming package name matches the old package or footprint
				if( (flags & KEEP_FP) 
					|| (pi->package == "") 
					|| (pi->package == old_part->package)
					|| (pi->package == old_part->shape->m_name) )
				{
					// use footprint and parameters from existing part
					pi->part = old_part;
					pi->ref_size = old_part->m_ref->m_font_size; 
					pi->ref_width = old_part->m_ref->m_stroke_width;
					pi->ref_vis = old_part->m_ref->m_bShown;					// CPT
					pi->value = old_part->value_text;
					pi->value_vis = old_part->m_ref->m_bShown;
					pi->x = old_part->x; 
					pi->y = old_part->y;
					pi->angle = old_part->angle;
					pi->side = old_part->side;
					pi->shape = old_part->shape;
				}
				else if( pi->shape )
				{
					// use new footprint, but preserve position
					pi->ref_size = old_part->m_ref->m_font_size; 
					pi->ref_width = old_part->m_ref->m_stroke_width;
					pi->ref_vis = old_part->m_ref->m_bShown;					// CPT
					pi->value = old_part->value_text;
					pi->value_vis = old_part->m_value->m_bShown;
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
					old_part->Remove(true);
				}
			}
			else if (old_part && !old_part->shape)
			{
				// remove old part (which did not have a footprint)
				if( log && old_part->package != pi->package )
				{
					CString s ((LPCSTR) IDS_ChangingFootprintOfPart);						// CPT2 TODO figure out what's going on with footprints & pkgs.
					mess.Format( s, old_part->ref_des, old_part->package, pi->package );
					log->AddLine( mess );
				}
				old_part->Remove(true);
			}
			else
			{
				// Will create part from scratch!
				CString s ((LPCSTR) IDS_CreatingPartWithFootprint);
				mess.Format( s, pi->ref_des, pi->shape->m_name );
				log->AddLine( mess );
			}

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
					for (iy = 0; iy < (GRID_Y - h); iy++ )
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
								goto end_loop;
						}
					}

				end_loop:
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
			
			cpart2 *part = new cpart2(this);
			CShape *shape = pi->shape;
			// The following line also positions ref and value according to "shape", if possible, and calls part->MustRedraw():
			part->SetData( shape, &pi->ref_des, &pi->value, &pi->package, pi->x, pi->y,
				pi->side, pi->angle, TRUE, FALSE );
			part->m_ref->m_bShown = pi->ref_vis;
			part->m_value->m_bShown = pi->value_vis;
			// CPT2 TODO.  The point of the following line is to allow "phantom pins" (those within a net that refer to deleted parts)
			// to be reattached when the part is reinstated.  Since I'm proposing that we drop phantom pins, I'm commenting it out.
			// m_nlist->PartAdded( part );
		}

		else 
		{
			// part existed before but may have been modified
			cpart2 *part = pi->part;
			part->MustRedraw();
			if( part->shape != pi->shape || pi->bShapeChanged )
			{
				// footprint was changed
				ASSERT( part->shape != NULL );
				if( pi->shape && !(flags & KEEP_FP) )
					// change footprint to new one
					part->ChangeFootprint( pi->shape );
			}
			// CPT2:  if importing from netlist file, then the only worthwhile data in "pi" is the shape.  Ignore the rest...
			if (flags & IMPORT_FROM_NETLIST_FILE)
				continue;
			part->package = pi->package;
			part->m_ref->m_bShown = pi->ref_vis;
			part->m_ref->m_layer = pi->ref_layer;
			part->m_ref->Resize( pi->ref_size, pi->ref_width );
			if (part->ref_des != pi->ref_des)
				part->ref_des = pi->ref_des,
				part->m_ref->m_str = pi->ref_des;
			part->m_value->m_bShown = pi->value_vis;
			part->m_value->m_layer = pi->value_layer;
			if( part->value_text != pi->value )
				part->value_text = pi->value,
				part->m_value->m_str = pi->value;
			if( pi->x != part->x 
				|| pi->y != part->y
				|| pi->angle != part->angle
				|| pi->side != part->side )
			{
				// part was moved
				int dx = pi->x - part->x, dy = pi->y - part->y;
				if (pi->angle != part->angle || pi->side != part->side)
					// If angle/side has changed, we must ensure that attached traces are all unrouted by PartMoved():
					dx = dy = 1;
				part->Move( pi->x, pi->y, pi->angle, pi->side );
				part->PartMoved( dx, dy );
			}
		}
	}

	// PurgeFootprintCache();
	free( grid );
}

void cpartlist::MoveOrigin( int dx, int dy )
{
	// CPT2 derived from function of the same name in CPartlist.
	citer<cpart2> ip (&parts);
	for (cpart2 *p = ip.First(); p; p = ip.Next())
		if (p->shape)
		{
			p->MustRedraw();
			p->x += dx;
			p->y += dy;
			// Pins will get repositioned when we draw
		}
}

void cpartlist::MarkAllParts( int util )
{
	// CPT2 derived from old CPartList func.  But I also mark the pins within each part, just for completeness (worth it?)
	citer<cpart2> ip (&parts);
	for (cpart2 *p = ip.First(); p; p = ip.Next())
	{
		p->utility = util;
		p->pins.SetUtility(util);
	}
}

int cpartlist::CheckPartlist( CString * logstr )
{
	int nerrors = 0;
	int nwarnings = 0;
	CString str;
	CMapStringToPtr map;
	void * ptr;

	CString s ((LPCSTR) IDS_CheckingParts2);
	*logstr += s;

	// first, check for duplicate parts
	citer<cpart2> ip (&parts);
	for (cpart2 *part = ip.First(); part; part = ip.Next())
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
	}

	// now check all parts.  CPT2 a lot of these errors seem very very unlikely under the new system.  The appropriate response to a lot of them
	// should probably be ASSERT(0).
	for (cpart2 *part = ip.First(); part; part = ip.Next())
	{
		str = "";
		CString * ref_des = &part->ref_des;
		if( !part->shape )
		{
			// no footprint
			CString s ((LPCSTR) IDS_WarningPartHasNoFootprint);
			str.Format( s, *ref_des );
			*logstr += str;
			nwarnings++;
			continue;
		}

		citer<cpin2> ipin (&part->pins);
		for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
		{
			// check this pin
			if( !pin->net )
				// part->pin->net is NULL, pin unconnected
				// this is not an error
				continue;
			else if (!m_nlist->nets.Contains( pin->net ))
			{
				// part->pin->net doesn't exist in netlist
				CString s ((LPCSTR) IDS_ErrorPartPinConnectedToNetWhichDoesntExist);
				str.Format( s, ref_des, pin->pin_name, pin->net->name );
				*logstr += str;
				nerrors++;
			}
			// try to find pin in pin list for net
			else if (!pin->net->pins.Contains(pin))
			{
				// pin not found
				CString s ((LPCSTR) IDS_ErrorPartPinConnectedToNetButPinNotInNet);
				str.Format( s, ref_des, pin->pin_name, pin->net->name );
				*logstr += str;
				nerrors++;
			}
			else 
				// OK
				;
		}
	}
	CString s0 ((LPCSTR) IDS_ErrorsWarnings);
	str.Format( s0, nerrors, nwarnings );
	*logstr += str;

	return nerrors;
}

bool cpartlist::CheckForProblemFootprints()
{
	BOOL bHeaders_28mil_holes = FALSE;   
	citer<cpart2> ip (&parts);
	for (cpart2 *part = ip.First(); part; part = ip.Next())
		if( part->shape && part->shape->m_name.Right(7) == "HDR-100")
			if (cpadstack *ps = part->shape->m_padstack.First())
				if (ps->hole_size == 28*NM_PER_MIL )
					{ bHeaders_28mil_holes = TRUE; break; }

	if( g_bShow_header_28mil_hole_warning && bHeaders_28mil_holes )   
	{
		CDlgMyMessageBox dlg;
		CString s ((LPCSTR) IDS_WarningYouAreLoadingFootprintsForThroughHoleHeaders);
		dlg.Initialize( s );
		dlg.DoModal();
		g_bShow_header_28mil_hole_warning = !dlg.bDontShowBoxState;
	}
	return bHeaders_28mil_holes;
}

