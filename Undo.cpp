#include "stdafx.h"
#include <math.h>
#include <stdlib.h>
#include "PcbItem.h"
#include "Net.h"
#include "FreePcbDoc.h"
#include "Part.h"
#include "Text.h"
#include "Shape.h"

extern BOOL bDontShowSelfIntersectionWarning;		// CPT2 TODO make these "sticky" by putting settings into default.cfg.
extern BOOL bDontShowSelfIntersectionArcsWarning;
extern BOOL bDontShowIntersectionWarning;
extern BOOL bDontShowIntersectionArcsWarning;

class CFreePcbDoc;
class cpcb_item;

// Convenience macros
#define VertexFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToVertex())
#define TeeFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToTee())
#define SegFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToSeg())
#define ConnectFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToConnect())
#define PinFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToPin())
#define PartFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToPart())
#define CornerFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToCorner())
#define SideFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToSide())
#define ContourFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToContour())
#define PolylineFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToPolyline())
#define AreaFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToArea())
#define TextFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToText())
#define RefTextFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToRefText())
#define ValueTextFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToValueText())
#define NetFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToNet())
#define PadstackFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToPadstack())
#define CentroidFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToCentroid())
#define GlueFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToGlue())
#define OutlineFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToOutline())
#define ShapeFromUid(uid) ((uid)==-1? NULL: cpcb_item::FindByUid(uid)->ToShape())

#define mallocInt(sz) ((int*) malloc((sz) * sizeof(int)))  // A very old-fashioned sort of macro ;)  So shoot me...

cuvertex::cuvertex( cvertex2 *v )
	: cundo_item (v->doc, v->UID())
{
	// Constructors for the various cundo_item classes copy undo info out of the correponding pcb-item.
	m_con = v->m_con->UID();
	m_net = v->m_net->UID();
	preSeg = v->preSeg? v->preSeg->UID(): -1;
	postSeg = v->postSeg? v->postSeg->UID(): -1;
	tee = v->tee? v->tee->UID(): -1;
	pin = v->pin? v->pin->UID(): -1;
	x = v->x;
	y = v->y;
	bNeedsThermal = v->bNeedsThermal;
	force_via_flag = v->force_via_flag;
	via_w = v->via_w;
	via_hole_w = v->via_hole_w;
}

void cuvertex::CreateTarget()
	{ target = new cvertex2(m_doc, m_uid); }

void cuvertex::FixTarget() 
{
	// Called during undo operations.  All the fields within "replacement" are ready to be set up, including ptrs (which have to be 
	// traslated from UID's)
	cvertex2 *v = target->ToVertex();
	v->m_con = ConnectFromUid(m_con);
	v->m_net = NetFromUid(m_net);
	v->preSeg = SegFromUid(preSeg);
	v->postSeg = SegFromUid(postSeg);
	v->tee = TeeFromUid(tee);
	v->pin = PinFromUid(pin);
	v->x = x, v->y = y;
	v->bNeedsThermal = bNeedsThermal;
	v->force_via_flag = force_via_flag;
	v->via_w = via_w;
	v->via_hole_w = via_hole_w;
}

void cuvertex::AddToLists()
{
	// After FixTarget() has been run for all the reconstructed objects during an undo, we invoke AddToLists() on each item to be sure that each
	// belongs to the carrays that it's supposed to.  In the case of vertices and other classes, this is probably overkill, but best to be safe...
	cvertex2 *v = target->ToVertex();
	if (v->tee)
		v->tee->vtxs.Add(v);
	v->m_con->vtxs.Add(v);
}

cutee::cutee( ctee *t ) 
	: cundo_item (t->doc, t->UID())
{
	nVtxs = t->vtxs.GetSize();
	vtxs = mallocInt(nVtxs);
	citer<cvertex2> iv (&t->vtxs);
	int i = 0;
	for (cvertex2 *v = iv.First(); v; v = iv.Next(), i++)
		vtxs[i] = v->UID();
	via_w = t->via_w;
	via_hole_w = t->via_hole_w;
}

void cutee::CreateTarget()
	{ target = new ctee(m_doc, m_uid); }

void cutee::FixTarget()
{
	ctee *t = target->ToTee();
	t->vtxs.RemoveAll();
	for (int i = 0; i < nVtxs; i++)
		t->vtxs.Add( VertexFromUid(vtxs[i]) );
	t->via_w = via_w;
	t->via_hole_w = via_hole_w;
}

void cutee::AddToLists()
{
	ctee *t = target->ToTee();
	cvertex2 *v = t->vtxs.First();
	if (!v) return;											// ???
	v->m_net->tees.Add(t);
}

cuseg::cuseg( cseg2 *s )
	: cundo_item (s->doc, s->UID())
{
	m_layer = s->m_layer;
	m_width = s->m_width;
	m_curve = s->m_curve;
	m_net = s->m_net->UID();
	m_con = s->m_con->UID();
	preVtx = s->preVtx->UID();
	postVtx = s->postVtx->UID();
}

void cuseg::CreateTarget()
	{ target = new cseg2(m_doc, m_uid); }

void cuseg::FixTarget()
{
	cseg2 *s = target->ToSeg();
	s->m_layer = m_layer;
	s->m_width = m_width;
	s->m_curve = m_curve;
	s->m_net = NetFromUid(m_net);
	s->m_con = ConnectFromUid(m_con);
	s->preVtx = VertexFromUid(preVtx);
	s->postVtx = VertexFromUid(postVtx);

}

void cuseg::AddToLists()
{
	cseg2 *s = target->ToSeg();
	s->m_con->segs.Add(s);
}


cuconnect::cuconnect( cconnect2 *c )
	: cundo_item (c->doc, c->UID())
{
	m_net = c->m_net->UID();
	nSegs = c->segs.GetSize();
	segs = mallocInt(nSegs);
	citer<cseg2> is (&c->segs);
	int i = 0;
	for (cseg2 *s = is.First(); s; s = is.Next(), i++)
		segs[i] = s->UID();
	nVtxs = c->vtxs.GetSize();
	vtxs = mallocInt(nVtxs);
	citer<cvertex2> iv (&c->vtxs);
	i = 0;
	for (cvertex2 *v = iv.First(); v; v = iv.Next(), i++)
		vtxs[i] = v->UID();
	head = c->head->UID();
	tail = c->tail->UID();
	locked = c->locked;
}

void cuconnect::CreateTarget()
	{ target = new cconnect2(m_doc, m_uid); }

void cuconnect::FixTarget()
{
	cconnect2 *c = target->ToConnect();
	c->m_net = NetFromUid(m_net);
	c->segs.RemoveAll();
	for (int i = 0; i < nSegs; i++)
		c->segs.Add( SegFromUid(segs[i]) );
	c->vtxs.RemoveAll();
	for (int i = 0; i < nVtxs; i++)
		c->vtxs.Add( VertexFromUid(vtxs[i]) );
	c->head = VertexFromUid(head);
	c->tail = VertexFromUid(tail);	
	c->locked = locked;
}

void cuconnect::AddToLists()
{
	cconnect2 *c = target->ToConnect();
	c->m_net->connects.Add(c);
}

cutext::cutext( ctext *t )
	: cundo_item (t->doc, t->UID())
{
	m_x = t->m_x;
	m_y = t->m_y;
	m_layer = t->m_layer;
	m_angle = t->m_angle;
	m_bMirror = t->m_bMirror;
	m_bNegative = t->m_bNegative;
	m_font_size = t->m_font_size;
	m_stroke_width = t->m_stroke_width;
	m_str = t->m_str;
	m_bShown = t->m_bShown;
	m_smfontutil = t->m_smfontutil;
	m_part = t->m_part? t->m_part->UID(): -1;
	m_shape = t->m_shape? t->m_shape->UID(): -1;
}

void cutext::CreateTarget()
	{ target = new ctext(m_doc, m_uid); }

void cutext::FixTarget()
{
	ctext *t = target->ToText();
	t->m_x = m_x;
	t->m_y = m_y;
	t->m_layer = m_layer;
	t->m_angle = m_angle;
	t->m_bMirror = m_bMirror;
	t->m_bNegative = m_bNegative;
	t->m_font_size = m_font_size;
	t->m_stroke_width = m_stroke_width;
	t->m_str = m_str;
	t->m_bShown = m_bShown;
	t->m_smfontutil = m_smfontutil;
	t->m_part = PartFromUid(m_part);
	t->m_shape = ShapeFromUid(m_shape);
}

void cutext::AddToLists()
{
	// Suffices for cureftexts and cuvaluetexts also
	ctext *t = target->ToText();
	if (t->m_part)
		t->m_part->m_tl->texts.Add( t );
	else
		m_doc->m_tlist->texts.Add( t );
}


void cureftext::CreateTarget()
	{ target = new creftext(m_doc, m_uid); }

void cuvaluetext::CreateTarget()
	{ target = new cvaluetext(m_doc, m_uid); }


cupin::cupin( cpin2 *p )
	: cundo_item (p->doc, p->UID())
{
	pin_name = p->pin_name;
	part = p->part->UID();
	x = p->x;
	y = p->y;
	ps = p->ps->UID();					// CPT2 TODO think more about how padstacks and other footprint types will fit into this.
	pad_layer = p->pad_layer;
	net = p->net? p->net->UID(): -1;
	bNeedsThermal = p->bNeedsThermal;	
}

void cupin::CreateTarget()
	{ target = new cpin2(m_doc, m_uid); }

void cupin::FixTarget()
{
	cpin2 *p = target->ToPin();
	p->pin_name = pin_name;
	p->part = PartFromUid(part);
	p->x = x, p->y = y;
	p->ps = PadstackFromUid(ps);
	p->pad_layer = pad_layer;
	p->net = NetFromUid(net);
	p->bNeedsThermal = bNeedsThermal;
}

void cupin::AddToLists()
{
	cpin2 *p = target->ToPin();
	p->part->pins.Add(p);
	if (p->net)
		p->net->pins.Add(p);
}

cupart::cupart( cpart2 *p )
	: cundo_item (p->doc, p->UID())
{
	m_pl = p->m_pl;
	nPins = p->pins.GetSize();
	pins = mallocInt(nPins);
	citer<cpin2> ipin (&p->pins);
	int i = 0;
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next(), i++)
		pins[i] = pin->UID();
	visible = p->visible;
	x = p->x; 
	y = p->y;
	side = p->side;
	angle = p->angle;
	glued = p->glued;
	ref_des = p->ref_des;
	m_ref = p->m_ref->UID();
	value_text = p->value_text;
	m_value = p->m_value->UID();
	nTexts = p->m_tl->texts.GetSize();
	texts = mallocInt(nTexts);
	citer<ctext> it (&p->m_tl->texts);
	i = 0;
	for (ctext *t = it.First(); t; t = it.Next(), i++)
		texts[i] = t->UID();
	shape = p->shape? p->shape->UID(): -1;
}

void cupart::CreateTarget()
	{ target = new cpart2(m_doc, m_uid); }

void cupart::FixTarget()
{
	cpart2 *p = target->ToPart();
	p->m_pl = m_pl;
	p->pins.RemoveAll();
	for (int i = 0; i < nPins; i++)
		p->pins.Add( PinFromUid(pins[i]) );
	p->visible = visible;
	p->x = x, p->y = y;
	p->side = side;
	p->angle = angle;
	p->glued = glued;
	p->ref_des = ref_des;
	p->m_ref = RefTextFromUid(m_ref);
	p->value_text = value_text;
	p->m_value = ValueTextFromUid(m_value);
	p->m_tl->texts.RemoveAll();
	for (int i = 0; i < nTexts; i++)
		p->m_tl->texts.Add( TextFromUid(texts[i]) );
	p->shape = ShapeFromUid(shape);
}

void cupart::AddToLists()
{
	cpart2 *p = target->ToPart();
	m_doc->m_plist->parts.Add( p );
}

cucorner::cucorner(ccorner *c)
	: cundo_item (c->doc, c->UID())
{
	x = c->x;
	y = c->y;
	contour = c->contour->UID();
	preSide = c->preSide? c->preSide->UID(): -1;
	postSide = c->postSide? c->postSide->UID(): -1;
}

void cucorner::CreateTarget()
	{ target = new ccorner(m_doc, m_uid); }

void cucorner::FixTarget()
{
	ccorner *c = target->ToCorner();
	c->x = x, c->y = y;
	c->contour = ContourFromUid(contour);
	c->preSide = SideFromUid(preSide);
	c->postSide = SideFromUid(postSide);
}

void cucorner::AddToLists()
{
	ccorner *c = target->ToCorner();
	c->contour->corners.Add( c );
}


cuside::cuside(cside *s)
	: cundo_item (s->doc, s->UID())
{
	m_style = s->m_style;
	contour = s->contour->UID();
	preCorner = s->preCorner->UID();
	postCorner = s->postCorner->UID();
}

void cuside::CreateTarget()
	{ target = new cside(m_doc, m_uid); }

void cuside::FixTarget()
{
	cside *s = target->ToSide();
	s->m_style = m_style;
	s->contour = ContourFromUid(contour);
	s->preCorner = CornerFromUid(preCorner);
	s->postCorner = CornerFromUid(postCorner);
}

void cuside::AddToLists()
{
	cside *s = target->ToSide();
	s->contour->sides.Add( s );
}

cucontour::cucontour(ccontour *ctr)
	: cundo_item (ctr->doc, ctr->UID())
{
	nCorners = ctr->corners.GetSize();
	corners = mallocInt(nCorners);
	citer<ccorner> ic (&ctr->corners);
	int i = 0;
	for (ccorner *c = ic.First(); c; c = ic.Next(), i++)
		corners[i] = c->UID();
	nSides = ctr->sides.GetSize();
	sides = mallocInt(nSides);
	citer<cside> is (&ctr->sides);
	i = 0;
	for (cside *s = is.First(); s; s = is.Next(), i++)
		sides[i] = s->UID();
	head = ctr->head->UID();
	tail = ctr->tail->UID();
	poly = ctr->poly->UID();
}

void cucontour::CreateTarget()
	{ target = new ccontour(m_doc, m_uid); }

void cucontour::FixTarget()
{
	ccontour *c = target->ToContour();
	c->corners.RemoveAll();
	for (int i=0; i<nCorners; i++)
		c->corners.Add( CornerFromUid(corners[i]) );
	c->sides.RemoveAll();
	for (int i=0; i<nSides; i++)
		c->sides.Add( SideFromUid(sides[i]) );
	c->head = CornerFromUid(head);
	c->tail = CornerFromUid(tail);
	c->poly = PolylineFromUid(poly);
}

void cucontour::AddToLists()
{
	ccontour *c = target->ToContour();
	c->poly->contours.Add(c);
}

cupolyline::cupolyline(cpolyline *poly)
	: cundo_item (poly->doc, poly->UID())
{
	main = poly->main->UID();
	nContours = poly->contours.GetSize();
	contours = mallocInt(nContours);
	citer<ccontour> ictr (&poly->contours);
	int i = 0;
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next(), i++)
		contours[i] = ctr->UID();
	m_layer = poly->m_layer;
	m_w = poly->m_w;
	m_sel_box = poly->m_sel_box;
	m_hatch = poly->m_hatch;
}

void cupolyline::FixTarget()
{
	// This routine also suffices for cusmcutout and cuboard, and it's almost sufficient for cuarea.
	cpolyline *p = target->ToPolyline();
	p->main = ContourFromUid(main);
	p->contours.RemoveAll();
	for (int i=0; i<nContours; i++)
		p->contours.Add( ContourFromUid(contours[i]) );
	p->m_layer = m_layer;
	p->m_w = m_w;
	p->m_sel_box = m_sel_box;
	p->m_hatch = m_hatch;
}

cuarea::cuarea(carea2 *a)
	: cupolyline (a)
	{ m_net = a->m_net->UID(); }

void cuarea::CreateTarget()
	{ target = new carea2(m_doc, m_uid); }

void cuarea::FixTarget()
{
	cupolyline::FixTarget();
	target->ToArea()->m_net = NetFromUid(m_net);
}

void cuarea::AddToLists()
{
	carea2 *a = target->ToArea();
	a->m_net->areas.Add( a );
}

cusmcutout::cusmcutout(csmcutout *sm)
	: cupolyline (sm)
	{ }

void cusmcutout::CreateTarget()
	{ target = new csmcutout(m_doc, m_uid); }

void cusmcutout::AddToLists()
{
	csmcutout *sm = target->ToSmCutout();
	m_doc->smcutouts.Add( sm );
}

cuboard::cuboard(cboard *b)
	: cupolyline (b)
	{ }

void cuboard::CreateTarget()
	{ target = new cboard(m_doc, m_uid); }

void cuboard::AddToLists()
{
	cboard *b = target->ToBoard();
	m_doc->boards.Add( b );
}

cuoutline::cuoutline(coutline *o)
	: cupolyline (o)
	{ shape = o->shape->UID(); }

void cuoutline::CreateTarget()
	{ target = new coutline(m_doc, m_uid); }

void cuoutline::FixTarget()
{
	cupolyline::FixTarget();
	target->ToOutline()->shape = ShapeFromUid( shape );
}

cunet::cunet(cnet2 *n)
	: cundo_item (n->doc, n->UID())
{
	m_nlist = n->m_nlist;
	name = n->name;
	nConnects = n->connects.GetSize();
	connects = mallocInt(nConnects);
	citer<cconnect2> ic (&n->connects);
	int i = 0;
	for (cconnect2 *c = ic.First(); c; c = ic.Next(), i++)
		connects[i] = c->UID();
	nPins = n->pins.GetSize();
	pins = mallocInt(nPins);
	citer<cpin2> ip (&n->pins);
	i = 0;
	for (cpin2 *p = ip.First(); p; p = ip.Next(), i++)
		pins[i] = p->UID();
	nAreas = n->areas.GetSize();
	areas = mallocInt(nAreas);
	citer<carea2> ia (&n->areas);
	i = 0;
	for (carea2 *a = ia.First(); a; a = ia.Next(), i++)
		areas[i] = a->UID();
	nTees = n->tees.GetSize();
	tees = mallocInt(nTees);
	citer<ctee> it (&n->tees);
	i = 0;
	for (ctee *t = it.First(); t; t = it.Next(), i++)
		tees[i] = t->UID();
	def_w = n->def_w;
	def_via_w = n->def_via_w;
	def_via_hole_w = n->def_via_hole_w;
	bVisible = n->bVisible;
}

void cunet::CreateTarget()
	{ target = new cnet2(m_doc, m_uid); }

void cunet::FixTarget()
{
	cnet2 *n = target->ToNet();
	n->m_nlist = m_nlist;
	n->name = name;
	n->connects.RemoveAll();
	for (int i=0; i<nConnects; i++)
		n->connects.Add( ConnectFromUid(connects[i]) );
	n->pins.RemoveAll();
	for (int i=0; i<nPins; i++)
		n->pins.Add( PinFromUid(pins[i]) );
	n->areas.RemoveAll();
	for (int i=0; i<nAreas; i++)
		n->areas.Add( AreaFromUid(areas[i]) );
	n->tees.RemoveAll();
	for (int i=0; i<nTees; i++)
		n->tees.Add( TeeFromUid(tees[i]) );
	n->def_w = def_w;
	n->def_via_w = def_via_w;
	n->def_via_hole_w = def_via_hole_w;
	n->bVisible = bVisible;
}

void cunet::AddToLists()
{
	cnet2 *n = target->ToNet();
	n->m_nlist->nets.Add( n );
}

cudre::cudre( cdre *d )
	: cundo_item (d->doc, d->UID())
{
	index = d->index;
	type = d->type;
	str = d->str;
	item1 = d->item1->UID();
	item2 = d->item1->UID();
	x = d->x, y = d->y;
	w = d->w;
	layer = d->layer;
}

void cudre::CreateTarget()
	{ target = new cdre(m_doc, m_uid); }

void cudre::FixTarget()
{
	cdre *d = target->ToDRE();
	d->index = index;
	d->type = type;
	d->str = str;
	d->item1 = item1==-1? NULL: cpcb_item::FindByUid(item1);
	d->item2 = item2==-1? NULL: cpcb_item::FindByUid(item2);
	d->x = x, d->y = y;
	d->w = w;
	d->layer = layer;
}

void cudre::AddToLists()
{
	cdre *d = target->ToDRE();
	m_doc->m_drelist->dres.Add( d );
}

cupadstack::cupadstack(cpadstack *ps)
	: cundo_item (ps->doc, ps->UID())
{
	name = ps->name;
	hole_size = ps->hole_size;
	x_rel = ps->x_rel; y_rel = ps->y_rel;
	angle = ps->angle;
	ps->top.CopyToArray(top); ps->top_mask.CopyToArray(top_mask); ps->top_paste.CopyToArray(top_paste);
	ps->bottom.CopyToArray(bottom); ps->bottom_mask.CopyToArray(bottom_mask); ps->bottom_paste.CopyToArray(bottom_paste);
	ps->inner.CopyToArray(inner);
	shape = ps->shape? ps->shape->UID(): -1;
}

void cupadstack::CreateTarget()
	{ target = new cpadstack(m_doc, m_uid); }

void cupadstack::FixTarget()
{
	cpadstack *ps = target->ToPadstack();
	ps->name = name;
	ps->hole_size = hole_size;
	ps->x_rel = x_rel; ps->y_rel = y_rel;
	ps->angle = angle;
	ps->top.CopyFromArray(top); ps->top_mask.CopyFromArray(top_mask); ps->top_paste.CopyFromArray(top_paste);
	ps->bottom.CopyFromArray(bottom); ps->bottom_mask.CopyFromArray(bottom_mask); ps->bottom_paste.CopyFromArray(bottom_paste);
	ps->inner.CopyFromArray(inner);
	ps->shape = ShapeFromUid(shape);
}


cucentroid::cucentroid(ccentroid *c)
	: cundo_item (c->doc, c->UID())
{
	m_type = c->m_type;
	m_x = c->m_x; m_y = c->m_y;
	m_angle = c->m_angle;
	m_shape = c->m_shape? c->m_shape->UID(): -1;
}

void cucentroid::CreateTarget()
	{ target = new ccentroid(m_doc, m_uid); }

void cucentroid::FixTarget()
{
	ccentroid *c = target->ToCentroid();
	c->m_type = m_type;
	c->m_x = m_x; c->m_y = m_y;
	c->m_angle = m_angle;
	c->m_shape = ShapeFromUid( m_shape );
}


cuglue::cuglue(cglue *g)
	: cundo_item (g->doc, g->UID())
{
	type = g->type;
	w = g->w; 
	x = g->x; y = g->y;
	shape = g->shape? g->shape->UID(): -1;
}

void cuglue::CreateTarget()
	{ target = new cglue(m_doc, m_uid); }

void cuglue::FixTarget()
{
	cglue *g = target->ToGlue();
	g->type = type;
	g->w = w;
	g->x = x; g->y = y;
	g->shape = ShapeFromUid( shape );
}

cushape::cushape( cshape *s )
	: cundo_item (s->doc, s->UID())
{	
	m_name = s->m_name;
	m_author = s->m_author;
	m_source = s->m_source;
	m_desc = s->m_desc;
	m_units = s->m_units;
	m_sel_xi = s->m_sel_xi, m_sel_yi = s->m_sel_yi;
	m_sel_xf = s->m_sel_xf, m_sel_yf = s->m_sel_yf;
	m_ref = s->m_ref->UID();
	m_value = s->m_value->UID();
	m_centroid = s->m_centroid->UID();
	m_nPadstacks = s->m_padstacks.GetSize();
	m_padstacks = mallocInt(m_nPadstacks);
	citer<cpadstack> ips (&s->m_padstacks);
	int i = 0;
	for (cpadstack *ps = ips.First(); ps; ps = ips.Next(), i++)
		m_padstacks[i] = ps->UID();
	m_nOutlines = s->m_outlines.GetSize();
	m_outlines = mallocInt(m_nOutlines);
	citer<coutline> io (&s->m_outlines);
	i = 0;
	for (coutline *o = io.First(); o; o = io.Next(), i++)
		m_outlines[i] = o->UID();
	m_nTexts = s->m_tl->texts.GetSize();
	m_texts = mallocInt(m_nTexts);
	citer<ctext> it (&s->m_tl->texts);
	i = 0;
	for (ctext *t = it.First(); t; t = it.Next(), i++)
		m_texts[i] = t->UID();
	m_nGlues = s->m_glues.GetSize();
	m_glues = mallocInt(m_nGlues);
	citer<cglue> ig (&s->m_glues);
	i = 0;
	for (cglue *g = ig.First(); g; g = ig.Next(), i++)
		m_glues[i] = g->UID();
}

void cushape::CreateTarget()
	{ target = new cshape(m_doc, m_uid); }

void cushape::FixTarget()
{
	cshape *s = target->ToShape();
	s->m_name = m_name;
	s->m_author = m_author;
	s->m_source = m_source;
	s->m_desc = m_desc;
	s->m_units = m_units;
	s->m_sel_xi = m_sel_xi, s->m_sel_yi = m_sel_yi;
	s->m_sel_xf = m_sel_xf, s->m_sel_yf = m_sel_yf;
	s->m_ref = RefTextFromUid(m_ref);
	s->m_value = ValueTextFromUid(m_value);
	s->m_centroid = CentroidFromUid(m_centroid);
	s->m_padstacks.RemoveAll();
	for (int i=0; i<m_nPadstacks; i++)
		s->m_padstacks.Add( PadstackFromUid(m_padstacks[i]) );
	s->m_outlines.RemoveAll();
	for (int i=0; i<m_nOutlines; i++)
		s->m_outlines.Add( OutlineFromUid(m_outlines[i]) );
	s->m_tl->texts.RemoveAll();
	for (int i=0; i<m_nTexts; i++)
		s->m_tl->texts.Add( TextFromUid(m_texts[i]) );
	s->m_glues.RemoveAll();
	for (int i=0; i<m_nGlues; i++)
		s->m_glues.Add( GlueFromUid(m_glues[i]) );
}

void cushape::AddToLists()
{
	cshape *s = target->ToShape();
	m_doc->m_slist->shapes.Add( s );
}



cundo_record::cundo_record( CArray<cundo_item*> *_items )
{
	nItems = _items->GetSize();
	items = (cundo_item**) malloc(nItems * sizeof(cundo_item*));
	for (int i=0; i<nItems; i++)
		items[i] = (*_items)[i];
	moveOriginDx = moveOriginDy = 0;
}

cundo_record::cundo_record( int _moveOriginDx, int _moveOriginDy )
{
	nItems = 0;
	items = NULL;
	moveOriginDx = _moveOriginDx;
	moveOriginDy = _moveOriginDy;
}

bool cundo_record::Execute( int op ) 
{
	// This is the biggie!  Performs an undo, redo, or undo-without-redo operation, depending on argument "op".
	// Returns true if the operation was a move-origin op.
	if (moveOriginDx || moveOriginDy)
	{
		extern CFreePcbApp theApp;
		CFreePcbView *view = theApp.m_view;
		view->MoveOrigin(-moveOriginDx, -moveOriginDy);
		moveOriginDx = -moveOriginDx;							// Negating these turns an undo record into a redo, and vice-versa.
		moveOriginDy = -moveOriginDy;
		return true;
	}

	// Step 1.  For each undo-item in the record, including the creations, find the corresponding existing pcb-item and fill the
	//			"target" field.  "target" may be an invalid object (if user removed it during the edit) or it may even be null
	//          (if user removed it AND it was garbage-collected).  In both cases we will have to recreate it.
	for (int i=0; i<nItems; i++)
		items[i]->target = cpcb_item::FindByUid( items[i]->m_uid );
	
	// Step 2.  Gather items for the redo record.  At the end, the current contents of "this" will be replaced with the redo items.  
	//  If op is OP_REDO, it's actually the same exact process:  the modified record on exit will be appropriate as an undo record if user
	//  resumes hitting ctrl-z.  If op is OP_UNDO_NO_REDO, we don't bother.
	CArray<cundo_item*> redoItems;
	if (op != OP_UNDO_NO_REDO)
		for (int i=0; i<nItems; i++)
		{
			cpcb_item *t = items[i]->target;
			CFreePcbDoc *doc = items[i]->m_doc;
			int uid = items[i]->m_uid;
			if (t && t->IsOnPcb())
				redoItems.Add( t->MakeUndoItem() );
			else
				// From the point of view of the redo, undo will be creating the item with this uid from scratch;  therefore 
				// include a new undo_item with bWasCreated set.
				redoItems.Add( new cundo_item (doc, uid, true) );
		}

	// Step 2a. Make sure all target objects are undrawn
	for (int i=0; i<nItems; i++)
	{
		cpcb_item *t = items[i]->target;
		if (t) 
			t->Undraw();
	}
	
	// Step 3. For any targets that are null, create replacement pcb-items.  These will be empty objects with no outgoing pointers at all, but 
	//         their uid values will be set according to the undo-items' uids.
	for (int i=0; i<nItems; i++)
		if (!items[i]->target)
			items[i]->CreateTarget();

	// Step 4.  Remove the bWasCreated items.  Mark the others for redrawing, and revise their contents on the basis of the data within the 
	// undo-items.
	for (int i=0; i<nItems; i++)
		if (items[i]->m_bWasCreated)
			items[i]->target->RemoveForUndo();
		else
			items[i]->FixTarget(),
			items[i]->target->MustRedraw();

	// Step 5.  Invoke AddToLists for all items, other than the bWasCreated ones which are now destroyed:
	for (int i=0; i<nItems; i++)
		if (!items[i]->m_bWasCreated)
			items[i]->AddToLists();

	// Step 6.  Clean up.  If appropriate, replace the current contents of the record with redo items.
	if (op == OP_UNDO_NO_REDO)
		return false;						// Caller will "delete this", which cleans everything out...
	for (int i=0; i<nItems; i++)
		delete items[i];
	for (int i=0; i<nItems; i++)
		items[i] = redoItems[i];
	return false;
}

