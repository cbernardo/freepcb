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
class CPcbItem;

// Convenience macros
#define VertexFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToVertex())
#define TeeFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToTee())
#define SegFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToSeg())
#define ConnectFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToConnect())
#define PinFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToPin())
#define PartFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToPart())
#define CornerFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToCorner())
#define SideFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToSide())
#define ContourFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToContour())
#define PolylineFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToPolyline())
#define AreaFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToArea())
#define TextFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToText())
#define RefTextFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToRefText())
#define ValueTextFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToValueText())
#define NetFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToNet())
#define PadstackFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToPadstack())
#define CentroidFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToCentroid())
#define GlueFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToGlue())
#define OutlineFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToOutline())
#define ShapeFromUid(uid) ((uid)==-1? NULL: CPcbItem::FindByUid(uid)->ToShape())

#define mallocInt(sz) ((int*) malloc((sz) * sizeof(int)))  // A very old-fashioned sort of macro ;)  So shoot me...

CUVertex::CUVertex( CVertex *v )
	: CUndoItem (v->doc, v->UID())
{
	// Constructors for the various CUndoItem classes copy undo info out of the correponding pcb-item.
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

void CUVertex::CreateTarget()
	{ target = new CVertex(m_doc, m_uid); }

void CUVertex::FixTarget() 
{
	// Called during undo operations.  All the fields within "replacement" are ready to be set up, including ptrs (which have to be 
	// traslated from UID's)
	CVertex *v = target->ToVertex();
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

void CUVertex::AddToLists()
{
	// After FixTarget() has been run for all the reconstructed objects during an undo, we invoke AddToLists() on each item to be sure that each
	// belongs to the carrays that it's supposed to.  In the case of vertices and other classes, this is probably overkill, but best to be safe...
	CVertex *v = target->ToVertex();
	if (v->tee)
		v->tee->vtxs.Add(v);
	v->m_con->vtxs.Add(v);
}

CUTee::CUTee( CTee *t ) 
	: CUndoItem (t->doc, t->UID())
{
	nVtxs = t->vtxs.GetSize();
	vtxs = mallocInt(nVtxs);
	CIter<CVertex> iv (&t->vtxs);
	int i = 0;
	for (CVertex *v = iv.First(); v; v = iv.Next(), i++)
		vtxs[i] = v->UID();
	via_w = t->via_w;
	via_hole_w = t->via_hole_w;
}

void CUTee::CreateTarget()
	{ target = new CTee(m_doc, m_uid); }

void CUTee::FixTarget()
{
	CTee *t = target->ToTee();
	t->vtxs.RemoveAll();
	for (int i = 0; i < nVtxs; i++)
		t->vtxs.Add( VertexFromUid(vtxs[i]) );
	t->via_w = via_w;
	t->via_hole_w = via_hole_w;
}

void CUTee::AddToLists()
{
	CTee *t = target->ToTee();
	CVertex *v = t->vtxs.First();
	if (!v) return;											// ???
	v->m_net->tees.Add(t);
}

CUSeg::CUSeg( CSeg *s )
	: CUndoItem (s->doc, s->UID())
{
	m_layer = s->m_layer;
	m_width = s->m_width;
	m_curve = s->m_curve;
	m_net = s->m_net->UID();
	m_con = s->m_con->UID();
	preVtx = s->preVtx->UID();
	postVtx = s->postVtx->UID();
}

void CUSeg::CreateTarget()
	{ target = new CSeg(m_doc, m_uid); }

void CUSeg::FixTarget()
{
	CSeg *s = target->ToSeg();
	s->m_layer = m_layer;
	s->m_width = m_width;
	s->m_curve = m_curve;
	s->m_net = NetFromUid(m_net);
	s->m_con = ConnectFromUid(m_con);
	s->preVtx = VertexFromUid(preVtx);
	s->postVtx = VertexFromUid(postVtx);

}

void CUSeg::AddToLists()
{
	CSeg *s = target->ToSeg();
	s->m_con->segs.Add(s);
}


CUConnect::CUConnect( CConnect *c )
	: CUndoItem (c->doc, c->UID())
{
	m_net = c->m_net->UID();
	nSegs = c->segs.GetSize();
	segs = mallocInt(nSegs);
	CIter<CSeg> is (&c->segs);
	int i = 0;
	for (CSeg *s = is.First(); s; s = is.Next(), i++)
		segs[i] = s->UID();
	nVtxs = c->vtxs.GetSize();
	vtxs = mallocInt(nVtxs);
	CIter<CVertex> iv (&c->vtxs);
	i = 0;
	for (CVertex *v = iv.First(); v; v = iv.Next(), i++)
		vtxs[i] = v->UID();
	head = c->head->UID();
	tail = c->tail->UID();
	locked = c->locked;
}

void CUConnect::CreateTarget()
	{ target = new CConnect(m_doc, m_uid); }

void CUConnect::FixTarget()
{
	CConnect *c = target->ToConnect();
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

void CUConnect::AddToLists()
{
	CConnect *c = target->ToConnect();
	c->m_net->connects.Add(c);
}

CUText::CUText( CText *t )
	: CUndoItem (t->doc, t->UID())
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

void CUText::CreateTarget()
	{ target = new CText(m_doc, m_uid); }

void CUText::FixTarget()
{
	CText *t = target->ToText();
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

void CUText::AddToLists()
{
	// Suffices for cureftexts and cuvaluetexts also
	CText *t = target->ToText();
	if (t->m_part)
		t->m_part->m_tl->texts.Add( t );
	else
		m_doc->m_tlist->texts.Add( t );
}


void CURefText::CreateTarget()
	{ target = new CRefText(m_doc, m_uid); }

void CUValueText::CreateTarget()
	{ target = new CValueText(m_doc, m_uid); }


CUPin::CUPin( CPin *p )
	: CUndoItem (p->doc, p->UID())
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

void CUPin::CreateTarget()
	{ target = new CPin(m_doc, m_uid); }

void CUPin::FixTarget()
{
	CPin *p = target->ToPin();
	p->pin_name = pin_name;
	p->part = PartFromUid(part);
	p->x = x, p->y = y;
	p->ps = PadstackFromUid(ps);
	p->pad_layer = pad_layer;
	p->net = NetFromUid(net);
	p->bNeedsThermal = bNeedsThermal;
}

void CUPin::AddToLists()
{
	CPin *p = target->ToPin();
	p->part->pins.Add(p);
	if (p->net)
		p->net->pins.Add(p);
}

CUPart::CUPart( CPart *p )
	: CUndoItem (p->doc, p->UID())
{
	m_pl = p->m_pl;
	nPins = p->pins.GetSize();
	pins = mallocInt(nPins);
	CIter<CPin> ipin (&p->pins);
	int i = 0;
	for (CPin *pin = ipin.First(); pin; pin = ipin.Next(), i++)
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
	CIter<CText> it (&p->m_tl->texts);
	i = 0;
	for (CText *t = it.First(); t; t = it.Next(), i++)
		texts[i] = t->UID();
	shape = p->shape? p->shape->UID(): -1;
}

void CUPart::CreateTarget()
	{ target = new CPart(m_doc, m_uid); }

void CUPart::FixTarget()
{
	CPart *p = target->ToPart();
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

void CUPart::AddToLists()
{
	CPart *p = target->ToPart();
	m_doc->m_plist->parts.Add( p );
}

CUCorner::CUCorner(CCorner *c)
	: CUndoItem (c->doc, c->UID())
{
	x = c->x;
	y = c->y;
	contour = c->contour->UID();
	preSide = c->preSide? c->preSide->UID(): -1;
	postSide = c->postSide? c->postSide->UID(): -1;
}

void CUCorner::CreateTarget()
	{ target = new CCorner(m_doc, m_uid); }

void CUCorner::FixTarget()
{
	CCorner *c = target->ToCorner();
	c->x = x, c->y = y;
	c->contour = ContourFromUid(contour);
	c->preSide = SideFromUid(preSide);
	c->postSide = SideFromUid(postSide);
}

void CUCorner::AddToLists()
{
	CCorner *c = target->ToCorner();
	c->contour->corners.Add( c );
}


CUSide::CUSide(CSide *s)
	: CUndoItem (s->doc, s->UID())
{
	m_style = s->m_style;
	contour = s->contour->UID();
	preCorner = s->preCorner->UID();
	postCorner = s->postCorner->UID();
}

void CUSide::CreateTarget()
	{ target = new CSide(m_doc, m_uid); }

void CUSide::FixTarget()
{
	CSide *s = target->ToSide();
	s->m_style = m_style;
	s->contour = ContourFromUid(contour);
	s->preCorner = CornerFromUid(preCorner);
	s->postCorner = CornerFromUid(postCorner);
}

void CUSide::AddToLists()
{
	CSide *s = target->ToSide();
	s->contour->sides.Add( s );
}

CUContour::CUContour(CContour *ctr)
	: CUndoItem (ctr->doc, ctr->UID())
{
	nCorners = ctr->corners.GetSize();
	corners = mallocInt(nCorners);
	CIter<CCorner> ic (&ctr->corners);
	int i = 0;
	for (CCorner *c = ic.First(); c; c = ic.Next(), i++)
		corners[i] = c->UID();
	nSides = ctr->sides.GetSize();
	sides = mallocInt(nSides);
	CIter<CSide> is (&ctr->sides);
	i = 0;
	for (CSide *s = is.First(); s; s = is.Next(), i++)
		sides[i] = s->UID();
	head = ctr->head->UID();
	tail = ctr->tail->UID();
	poly = ctr->poly->UID();
}

void CUContour::CreateTarget()
	{ target = new CContour(m_doc, m_uid); }

void CUContour::FixTarget()
{
	CContour *c = target->ToContour();
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

void CUContour::AddToLists()
{
	CContour *c = target->ToContour();
	c->poly->contours.Add(c);
}

CUPolyline::CUPolyline(CPolyline *poly)
	: CUndoItem (poly->doc, poly->UID())
{
	main = poly->main->UID();
	nContours = poly->contours.GetSize();
	contours = mallocInt(nContours);
	CIter<CContour> ictr (&poly->contours);
	int i = 0;
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next(), i++)
		contours[i] = ctr->UID();
	m_layer = poly->m_layer;
	m_w = poly->m_w;
	m_sel_box = poly->m_sel_box;
	m_hatch = poly->m_hatch;
}

void CUPolyline::FixTarget()
{
	// This routine also suffices for CUSmCutout and CUBoard, and it's almost sufficient for CUArea and CUOutline.
	CPolyline *p = target->ToPolyline();
	p->main = ContourFromUid(main);
	p->contours.RemoveAll();
	for (int i=0; i<nContours; i++)
		p->contours.Add( ContourFromUid(contours[i]) );
	p->m_layer = m_layer;
	p->m_w = m_w;
	p->m_sel_box = m_sel_box;
	p->m_hatch = m_hatch;
}

CUArea::CUArea(CArea *a)
	: CUPolyline (a)
	{ m_net = a->m_net->UID(); }

void CUArea::CreateTarget()
	{ target = new CArea(m_doc, m_uid); }

void CUArea::FixTarget()
{
	CUPolyline::FixTarget();
	target->ToArea()->m_net = NetFromUid(m_net);
}

void CUArea::AddToLists()
{
	CArea *a = target->ToArea();
	a->m_net->areas.Add( a );
}

CUSmCutout::CUSmCutout(CSmCutout *sm)
	: CUPolyline (sm)
	{ }

void CUSmCutout::CreateTarget()
	{ target = new CSmCutout(m_doc, m_uid); }

void CUSmCutout::AddToLists()
{
	CSmCutout *sm = target->ToSmCutout();
	m_doc->smcutouts.Add( sm );
}

CUBoard::CUBoard(CBoard *b)
	: CUPolyline (b)
	{ }

void CUBoard::CreateTarget()
	{ target = new CBoard(m_doc, m_uid); }

void CUBoard::AddToLists()
{
	CBoard *b = target->ToBoard();
	m_doc->boards.Add( b );
}

CUOutline::CUOutline(COutline *o)
	: CUPolyline (o)
	{ shape = o->shape->UID(); }

void CUOutline::CreateTarget()
	{ target = new COutline(m_doc, m_uid); }

void CUOutline::FixTarget()
{
	CUPolyline::FixTarget();
	target->ToOutline()->shape = ShapeFromUid( shape );
}

CUNet::CUNet(CNet *n)
	: CUndoItem (n->doc, n->UID())
{
	m_nlist = n->m_nlist;
	name = n->name;
	nConnects = n->connects.GetSize();
	connects = mallocInt(nConnects);
	CIter<CConnect> ic (&n->connects);
	int i = 0;
	for (CConnect *c = ic.First(); c; c = ic.Next(), i++)
		connects[i] = c->UID();
	nPins = n->pins.GetSize();
	pins = mallocInt(nPins);
	CIter<CPin> ip (&n->pins);
	i = 0;
	for (CPin *p = ip.First(); p; p = ip.Next(), i++)
		pins[i] = p->UID();
	nAreas = n->areas.GetSize();
	areas = mallocInt(nAreas);
	CIter<CArea> ia (&n->areas);
	i = 0;
	for (CArea *a = ia.First(); a; a = ia.Next(), i++)
		areas[i] = a->UID();
	nTees = n->tees.GetSize();
	tees = mallocInt(nTees);
	CIter<CTee> it (&n->tees);
	i = 0;
	for (CTee *t = it.First(); t; t = it.Next(), i++)
		tees[i] = t->UID();
	def_w = n->def_w;
	def_via_w = n->def_via_w;
	def_via_hole_w = n->def_via_hole_w;
	bVisible = n->bVisible;
}

void CUNet::CreateTarget()
	{ target = new CNet(m_doc, m_uid); }

void CUNet::FixTarget()
{
	CNet *n = target->ToNet();
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

void CUNet::AddToLists()
{
	CNet *n = target->ToNet();
	n->m_nlist->nets.Add( n );
}

/* CPT2 phased out as of r345

CUDre::CUDre( CDre *d )
	: CUndoItem (d->doc, d->UID())
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

void CUDre::CreateTarget()
	{ target = new CDre(m_doc, m_uid); }

void CUDre::FixTarget()
{
	CDre *d = target->ToDRE();
	d->index = index;
	d->type = type;
	d->str = str;
	d->item1 = item1==-1? NULL: CPcbItem::FindByUid(item1);
	d->item2 = item2==-1? NULL: CPcbItem::FindByUid(item2);
	d->x = x, d->y = y;
	d->w = w;
	d->layer = layer;
}

void CUDre::AddToLists()
{
	CDre *d = target->ToDRE();
	m_doc->m_drelist->dres.Add( d );
}
*/

CUPadstack::CUPadstack(CPadstack *ps)
	: CUndoItem (ps->doc, ps->UID())
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

void CUPadstack::CreateTarget()
	{ target = new CPadstack(m_doc, m_uid); }

void CUPadstack::FixTarget()
{
	CPadstack *ps = target->ToPadstack();
	ps->name = name;
	ps->hole_size = hole_size;
	ps->x_rel = x_rel; ps->y_rel = y_rel;
	ps->angle = angle;
	ps->top.CopyFromArray(top); ps->top_mask.CopyFromArray(top_mask); ps->top_paste.CopyFromArray(top_paste);
	ps->bottom.CopyFromArray(bottom); ps->bottom_mask.CopyFromArray(bottom_mask); ps->bottom_paste.CopyFromArray(bottom_paste);
	ps->inner.CopyFromArray(inner);
	ps->shape = ShapeFromUid(shape);
}


CUCentroid::CUCentroid(CCentroid *c)
	: CUndoItem (c->doc, c->UID())
{
	m_type = c->m_type;
	m_x = c->m_x; m_y = c->m_y;
	m_angle = c->m_angle;
	m_shape = c->m_shape? c->m_shape->UID(): -1;
}

void CUCentroid::CreateTarget()
	{ target = new CCentroid(m_doc, m_uid); }

void CUCentroid::FixTarget()
{
	CCentroid *c = target->ToCentroid();
	c->m_type = m_type;
	c->m_x = m_x; c->m_y = m_y;
	c->m_angle = m_angle;
	c->m_shape = ShapeFromUid( m_shape );
}


CUGlue::CUGlue(CGlue *g)
	: CUndoItem (g->doc, g->UID())
{
	type = g->type;
	w = g->w; 
	x = g->x; y = g->y;
	shape = g->shape? g->shape->UID(): -1;
}

void CUGlue::CreateTarget()
	{ target = new CGlue(m_doc, m_uid); }

void CUGlue::FixTarget()
{
	CGlue *g = target->ToGlue();
	g->type = type;
	g->w = w;
	g->x = x; g->y = y;
	g->shape = ShapeFromUid( shape );
}

CUShape::CUShape( CShape *s )
	: CUndoItem (s->doc, s->UID())
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
	CIter<CPadstack> ips (&s->m_padstacks);
	int i = 0;
	for (CPadstack *ps = ips.First(); ps; ps = ips.Next(), i++)
		m_padstacks[i] = ps->UID();
	m_nOutlines = s->m_outlines.GetSize();
	m_outlines = mallocInt(m_nOutlines);
	CIter<COutline> io (&s->m_outlines);
	i = 0;
	for (COutline *o = io.First(); o; o = io.Next(), i++)
		m_outlines[i] = o->UID();
	m_nTexts = s->m_tl->texts.GetSize();
	m_texts = mallocInt(m_nTexts);
	CIter<CText> it (&s->m_tl->texts);
	i = 0;
	for (CText *t = it.First(); t; t = it.Next(), i++)
		m_texts[i] = t->UID();
	m_nGlues = s->m_glues.GetSize();
	m_glues = mallocInt(m_nGlues);
	CIter<CGlue> ig (&s->m_glues);
	i = 0;
	for (CGlue *g = ig.First(); g; g = ig.Next(), i++)
		m_glues[i] = g->UID();
}

void CUShape::CreateTarget()
	{ target = new CShape(m_doc, m_uid); }

void CUShape::FixTarget()
{
	CShape *s = target->ToShape();
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

void CUShape::AddToLists()
{
	CShape *s = target->ToShape();
	m_doc->m_slist->shapes.Add( s );
}



CUndoRecord::CUndoRecord( CArray<CUndoItem*> *_items, CHeap<CPcbItem> *_sel )
{
	nItems = _items->GetSize();
	items = (CUndoItem**) malloc(nItems * sizeof(CUndoItem*));
	for (int i=0; i<nItems; i++)
		items[i] = (*_items)[i];
	moveOriginDx = moveOriginDy = 0;
	nSel = _sel->GetSize();
	sel = (int*) malloc(nSel * sizeof(int));
	CIter<CPcbItem> ii (_sel);
	int j = 0;
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
		sel[j++] = i->UID();
}

CUndoRecord::CUndoRecord( int _moveOriginDx, int _moveOriginDy, CHeap<CPcbItem> *_sel )
{
	nItems = 0;
	items = NULL;
	moveOriginDx = _moveOriginDx;
	moveOriginDy = _moveOriginDy;
	nSel = _sel->GetSize();
	sel = (int*) malloc(nSel * sizeof(int));
	CIter<CPcbItem> ii (_sel);
	int j = 0;
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
		sel[j++] = i->UID();
}

bool CUndoRecord::Execute( int op ) 
{
	// This is the biggie!  Performs an undo, redo, or undo-without-redo operation, depending on argument "op".
	// Returns true if the operation was a move-origin op.
	extern CFreePcbApp theApp;
	CFreePcbView *view = theApp.m_view;
	if (moveOriginDx || moveOriginDy)
	{
		view->MoveOrigin(-moveOriginDx, -moveOriginDy);
		moveOriginDx = -moveOriginDx;							// Negating these turns an undo record into a redo, and vice-versa.
		moveOriginDy = -moveOriginDy;
		return true;
	}

	// Step 1.  For each undo-item in the record, including the creations, find the corresponding existing pcb-item and fill the
	//			"target" field.  "target" may be an invalid object (if user removed it during the edit) or it may even be null
	//          (if user removed it AND it was garbage-collected).  In both cases we will have to recreate it.
	for (int i=0; i<nItems; i++)
		items[i]->target = CPcbItem::FindByUid( items[i]->m_uid );

	// Step 2.  Gather items for the redo record.  At the end, the current contents of "this" will be replaced with the redo items.  
	//  If op is OP_REDO, it's actually the same exact process:  the modified record on exit will be appropriate as an undo record if user
	//  resumes hitting ctrl-z.  If op is OP_UNDO_NO_REDO, we don't bother.
	CArray<CUndoItem*> redoItems;
	if (op != OP_UNDO_NO_REDO)
		for (int i=0; i<nItems; i++)
		{
			CPcbItem *t = items[i]->target;
			CFreePcbDoc *doc = items[i]->m_doc;
			int uid = items[i]->m_uid;
			if (t && t->IsOnPcb())
				redoItems.Add( t->MakeUndoItem() );
			else
				// From the point of view of the redo, undo will be creating the item with this uid from scratch;  therefore 
				// include a new undo_item with bWasCreated set.
				redoItems.Add( new CUndoItem (doc, uid, true) );
		}

	// Step 2a. Make sure all target objects are undrawn
	for (int i=0; i<nItems; i++)
	{
		CPcbItem *t = items[i]->target;
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
		if (!items[i]->target)
			// It's _just_ possible that CreateTarget failed to do anything in step 3, probably because we have a bWasCreated CUndoItem for an
			// object that was later deleted.  Just ignore these cases...
			;
		else if (items[i]->m_bWasCreated)
			items[i]->target->RemoveForUndo();
		else
			items[i]->FixTarget(),
			items[i]->target->MustRedraw();

	// Step 5.  Invoke AddToLists for all items, other than the bWasCreated ones which are now destroyed:
	for (int i=0; i<nItems; i++)
		if (!items[i]->m_bWasCreated)
			items[i]->AddToLists();

	// Step 6 (new in r345).  Reselect items.  For the sake of the redo, alter the sel member of this CUndoRecord to reflect the current selection.
	int nSel2 = view->m_sel.GetSize();
	int *sel2 = (int*) malloc(nSel2 * sizeof(int));
	CIter<CPcbItem> ii (&view->m_sel);
	int j = 0;
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
		sel2[j++] = i->UID();
	view->m_sel.RemoveAll();
	for (int i=0; i<nSel; i++)
	{
		CPcbItem *item = CPcbItem::FindByUid( sel[i] );
		if (item && item->IsOnPcb())
			view->m_sel.Add(item);
	}
	free(sel);
	nSel = nSel2;
	sel = sel2;

	// Step 7.  Clean up.  If appropriate, replace the current contents of the record with redo items.
	if (op == OP_UNDO_NO_REDO)
		return false;						// Caller will "delete this", which cleans everything out...
	for (int i=0; i<nItems; i++)
		delete items[i];
	for (int i=0; i<nItems; i++)
		items[i] = redoItems[i];
	return false;
}

