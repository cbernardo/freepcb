// PcbItem.cpp:  Source for the base class underlying all the items that appear on a PCB. 

#include "stdafx.h"
#include <math.h>
#include <stdlib.h>
#include "PcbItem.h"
#include "Net.h"
#include "FreePcbDoc.h"
#include "Part.h"
#include "Text.h"

class CFreePcbDoc;
class cpcb_item;

int cpcb_item::next_uid = 1;
int cpcb_item::uid_hash_sz = 0x2000;
cpcb_item **cpcb_item::uid_hash = NULL;

cpcb_item::cpcb_item(CFreePcbDoc *_doc)
{
	carray_list = NULL; 
	if (!uid_hash)
	{
		// First ever item creation.  Initialize uid_hash table.
		int sz = uid_hash_sz * sizeof(cpcb_item*);
		uid_hash = (cpcb_item**) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sz);
	}
	m_uid = next_uid++;
	doc = _doc;
	doc->items.Add(this);
	dl_el = dl_sel = NULL;
	utility = 0;
	bDrawn = false;
	// Enter item into uid_hash table.  It's an incredibly simple hash function:  just extract the lower few bits, depending on uid_hash_sz, which
	// must be a power of 2.  Given how objects are allocated and eventually die, this function should work as well as any.
	int hashVal = m_uid & (uid_hash_sz-1);
	uid_hash_next = uid_hash[hashVal];
	uid_hash[hashVal] = this;
}

cpcb_item::cpcb_item(CFreePcbDoc *_doc, int _uid)
{
	// Constructor used only during undo operations, for recreating items that user removed and that were subsequently garbage-collected.
	// Creates an empty object with the specified uid.  See the constructors cvertex2::cvertex2(_doc, _uid), etc.
	carray_list = NULL;
	m_uid = _uid;
	doc = _doc;
	doc->items.Add(this);
	dl_el = dl_sel = NULL;
	utility = 0;
	bDrawn = false;
	// Enter item into uid_hash table.
	int hashVal = m_uid & (uid_hash_sz-1);
	uid_hash_next = uid_hash[hashVal];
	uid_hash[hashVal] = this;
}

cpcb_item::~cpcb_item()
{
	// When an item is destroyed, references to it are automatically removed from any carrays to which it belongs:
	for (carray_link *link = carray_list, *next; link; link = next)
	{
		carray<cpcb_item> *arr = (carray<cpcb_item>*) link->arr;
		int off = link->off;
		ASSERT(arr->heap[off]==this);				// For now...
		arr->heap[off] = (cpcb_item*) arr->free;	// Add "off" to arr's free-offset list
		arr->free = off;
		arr->flags[off>>3] &= ~(1 << (off&7));
		arr->nItems--;
		next = link->next;
		delete link;
	}
	// Item is also removed from the uid-hash table.
	cpcb_item *i, *prev = NULL;
	int hashVal = m_uid & (uid_hash_sz-1);
	for (i = uid_hash[hashVal]; i; i = i->uid_hash_next)
	{
		if (i==this)
		{
			if (prev)
				prev->uid_hash_next = i->uid_hash_next;
			else
				uid_hash[hashVal] = i->uid_hash_next;
			break;
		}
		prev = i;
	}
}

cpcb_item *cpcb_item::FindByUid(int uid)
{
	// CPT2 new.  Static function.  Looks in static uid_hash[], using an extremely simple hash function (extract lower few bits).
	for (cpcb_item *i = uid_hash[uid & (uid_hash_sz-1)]; i; i = i->uid_hash_next)
		if (i->m_uid == uid)
			return i;
	return NULL;
}

void cpcb_item::MustRedraw()
{
	Undraw();
	if (doc)
		doc->redraw.Add(this);
}

void cpcb_item::Undraw()
{
	// Default behavior, often overridden
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	dl->Remove( dl_el );
	dl->Remove( dl_sel );
	dl_el = dl_sel = NULL;
	bDrawn = false;
}

void cpcb_item::SaveUndoInfo()
{
	// Default behavior, overridden in complex classes like cconnect2, cnet2, cpart2, and cshape
	doc->m_undo_items.Add( this->MakeUndoItem() );
}

void cpcb_item::RemoveForUndo()
{
	// On a prior editing operation, user created this object, but now undo needs to get rid of it.  It should suffice to detach the object
	// from any carrays in which it exists (for instance, we remove a net from its netlist).  We can assume that any other pcb-items that 
	// reference this now-defunct object will be getting revised by this same undo operation.
	for (carray_link *link = carray_list, *next; link; link = next)
	{
		carray<cpcb_item> *arr = (carray<cpcb_item>*) link->arr;
		int off = link->off;
		ASSERT(arr->heap[off]==this);				// For now...
		arr->heap[off] = (cpcb_item*) arr->free;	// Add "off" to arr's free-offset list
		arr->free = off;
		arr->flags[off>>3] &= ~(1 << (off&7));
		arr->nItems--;
		next = link->next;
		delete link;
	}
	carray_list = NULL;
	// However, we may as well leave this item in doc->items.   There's a chance it might get recycled later (if user hits redo).  If not,
	// the garbage collector will clean it out eventually.
	doc->items.Add(this);
}

bool cpcb_item::IsHit(int x, int y)
{
	// CPT2 new.  Return true if this item's selector contains (x,y), which is in pcb units
	if (!dl_sel || !dl_sel->visible) 
		return false;
	int pcbu_per_wu = doc->m_dlist->m_pcbu_per_wu;
	double xx = double(x)/pcbu_per_wu;				// CPT (was int, which caused range issues)
	double yy = double(y)/pcbu_per_wu;				// CPT (was int)
	double d;
	return dl_sel->IsHit(xx, yy, d);
}

carea2 *cpcb_item::GetArea() 
{
	if (cpolyline *p = GetPolyline())
		return p->ToArea();
	return NULL;
}

bool cpcb_item::IsFootItem() 
{ 
	if (GetTypeBit() & bitsFootItem) return true;
	if (ctext *t = this->ToText())
		return t->m_shape != NULL;
	return false;
}

/*
void cpcb_item::GarbageCollect() {
	// Part of my proposed new architecture is to take some of the hassle out of memory management for pcb-items by using garbage collection.
	// Each time an item is constructed, it is added to the "items" array.  If an item goes out of use, one does not have to "delete" it;
	// just unhook it from its parent entity or array.  When garbage collection time comes, we'll first clear the utility bits on all members of
	// "items".  Then we'll scan through the doc's netlist (recursing through connects, segs, vtxs, tees, areas), partlist (recursing through pins), 
	// textlist, and otherlist, marking utility bits as we go.  At the end, items with clear utility bits can be deleted.
	citer<cpcb_item> ii (&items);
	for (cpcb_item *i = ii.First(); i; i = ii.Next())
		i->utility = 0;

	citer<cnet2> in (&m_nlist->nets);
	for (cnet2 *n = in.First(); n; n = in.Next()) {
		n->utility = 1;
		citer<cconnect2> ic (&n->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next()) {
			c->utility = 1;
			citer<cvertex2> iv (&c->vtxs);
			for (cvertex2 *v = iv.First(); v; v = iv.Next()) {
				v->utility = 1;
				if (v->tee) v->tee->utility = 1;
				}
			citer<cseg2> is (&c->segs);
			for (cseg2 *s = is.First(); s; s = is.Next())
				s->utility = 1;
			}
		citer<carea2> ia (&n->areas);
		for (carea2 *a = ia.First(); a; a = ia.Next())
			a->MarkConstituents();
		}
	citer<cpart2> ip (&m_plist->parts);
	for (cpart2 *p = ip.First(); p; p = ip.Next()) {
		p->utility = 1;
		citer<cpin2> ipin (&p->pins);
		for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
			pin->utility = 1;
		}
	citer<ctext> it (&m_tlist->texts);
	for (ctext *t = it.First(); t; t = it.Next()) 
		t->utility = 1;
	citer<cpcb_item> ii2 (&others);
	for (cpcb_item *i = ii2.First(); i; i = ii2.Next()) {
		if (cpolyline *p = i->ToPolyline())
			p->MarkConstituents();
		else
			p->utility = 1;

	// Do the deletions of unused items!
	for (cpcb_item *i = ii.First(); i; i = ii.Next())
		if (!i.utility)
			delete i;
	}

*/


