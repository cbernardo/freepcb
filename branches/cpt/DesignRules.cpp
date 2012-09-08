//  DesignRules.cpp.  Source file for classes related to design-rule checking, including CDre (which descends from CPcbItem), CDreList, and DesignRules.

#include "stdafx.h"


CDre::CDre(CFreePcbDoc *_doc, int _index, int _type, CString *_str, CPcbItem *_item1, CPcbItem *_item2, 
		   int _x, int _y, int _w, int _layer) 
	: CPcbItem(_doc)
{ 
	index = _index;
	type = _type;
	str = *_str;
	item1 = _item1? _item1->UID(): -1;
	item2 = _item2? _item2->UID(): -1;
	x = _x, y = _y;
	w = _w;
	layer = _layer;
}

CDre::CDre(CFreePcbDoc *_doc, int _uid):
	CPcbItem(_doc, _uid)
{ 
	item1 = item2 = NULL;
}

bool CDre::IsOnPcb()
{
	return doc->m_drelist->dres.Contains(this);
}

void CDre::Remove()
{
	/// As of r345 dre's are NOT incorporated into the undo system.  (NB CFreePcbDoc::FinishUndoRecord() has an explicit line making sure that
	// newly-created ones are also ignored.)
	Undraw();
	CDreList *list = doc->m_drelist;
	list->dres.Remove(this);
	if (prev)
		prev->next = next;
	else
		list->head = next;
	if (next)
		next->prev = prev;
	else
		list->tail = prev;
}

int CDre::Draw()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl )
		return NO_DLIST;
	if (bDrawn)
		return ALREADY_DRAWN;
	dl_el = dl->AddMain( this, LAY_DRC_ERROR, DL_HOLLOW_CIRC, 1, w, 0, 0, x, y, 0, 0, x, y ); 
	dl_sel = dl->AddSelector( this, LAY_DRC_ERROR, DL_HOLLOW_CIRC, 1, w, 0, x, y, 0, 0, x, y ); 
	bDrawn = true;
	return NOERR;
}

void CDre::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	if( !dl_sel ) return;
	dl->Highlight( DL_HOLLOW_CIRC, 
		dl->Get_x( dl_sel ), dl->Get_y( dl_sel ),
		dl->Get_xf( dl_sel ), dl->Get_yf( dl_sel ),
		dl->Get_w( dl_sel ) );
}

CDre *CDreList::Add( int type, CString * str, CPcbItem *item1, CPcbItem *item2,
		int x1, int y1, int x2, int y2, int w, int layer )
{
	// CPT2 converted from DRErrorList function
	// find center of coords
	int x = x1;
	int y = y1;
	if( item2 )
	{
		x = (x1 + x2)/2;
		y = (y1 + y2)/2;
	}

	// first check for redundant error.  CPT: allow repeats of COPPERAREA_BROKEN errors
	if( type != CDre::COPPERAREA_BROKEN )	
	{
		CIter<CDre> id (&dres);
		for (CDre *dre = id.First(); dre; dre = id.Next())
		{
			CPcbItem *old1 = CPcbItem::FindByUid(dre->item1);
			CPcbItem *old2 = CPcbItem::FindByUid(dre->item2);
			// compare current error with error from list
			if( old1 == item1 && old2 == item2 )
				return NULL;
			if( old1 == item2 && old2 == item1 )
				return NULL;
			// if same traces at same point, don't add it
			if( item2 && old2 )
			{
				CConnect *oldC1 = old1->GetConnect(), *oldC2 = old2->GetConnect();
				CConnect *newC1 = item1->GetConnect(), *newC2 = item2->GetConnect();
				if (oldC1==newC1 && oldC2==newC2 && dre->x==x && dre->y==y)
					return NULL;
				if (oldC1==newC2 && oldC2==newC1 && dre->x==x && dre->y==y)
					return NULL;
			}

			// if RING_PAD error on same pad, don't add it
			if( type == CDre::RING_PAD && dre->type == CDre::RING_PAD )		// CPT2 second clause was dre->m_id.T3().  Check if I've translated right...
				if( item1 == old1 )
					return NULL;

			// if BOARDEDGE_PAD or BOARDEDGE_PADHOLE error on same pad, don't add it
			if( (type == CDre::BOARDEDGE_PAD || type == CDre::BOARDEDGE_PADHOLE)
				&& (dre->type == CDre::BOARDEDGE_PAD || dre->type == CDre::BOARDEDGE_PADHOLE) )
					if( item1 == old1 )
						return NULL;

			// if RING_VIA error on same via, don't add it
			if( type == CDre::RING_VIA && dre->type == CDre::RING_VIA )
				if( item1 == old1 )
					return NULL;

			// if BOARDEDGE_VIA or BOARDEDGE_VIAHOLE error on same via, don't add it
			if( (type == CDre::BOARDEDGE_VIA || type == CDre::BOARDEDGE_VIAHOLE)
				&& (dre->type == CDre::BOARDEDGE_VIA || dre->type == CDre::BOARDEDGE_VIAHOLE) )
					if( item1 == old1 )
						return NULL;

			// if BOARDEDGE_SEG on same trace at same point, don't add it
			if( type == CDre::BOARDEDGE_SEG && dre->type == CDre::BOARDEDGE_SEG )
				if( item1 == old1 )															// Sufficient check?
					return NULL;
			
			// if BOARDEDGE_COPPERAREA on same area at same point, don't add it
			if( type == CDre::BOARDEDGE_COPPERAREA && dre->type == CDre::BOARDEDGE_COPPERAREA )
				if( item1->GetPolyline() == old1->GetPolyline()								// CPT2 Are the "->GetPolyline()" invocations essential?
					&& x == dre->x  && y == dre->y ) 
					return NULL;
				
			if( type == CDre::COPPERAREA_COPPERAREA && dre->type == CDre::COPPERAREA_COPPERAREA )		
				if( x == dre->x && y == dre->y)
					if( item1 == old1 && item2 == old2 || 
					    item1 == old2 && item2 == old1) 
							return NULL;
		}
	}

	int d = 50*NM_PER_MIL;																		// Minimum width
	if( item2 )
		d = max( d, Distance( x1, y1, x2, y2 ) );
	if( w )
		d = max( d, w );

	CDre *dre = new CDre(doc, GetSize(), type, str, item1, item2, x, y, d, layer);
	dres.Add(dre);
	dre->MustRedraw();
	return dre;
}

void CDreList::Clear()
{
	// CPT2 new.  Undraw and then remove from "this" all design-rule-error objects.
	CIter<CDre> id (&dres);
	for (CDre* d = id.First(); d; d = id.Next())
		d->Undraw();
	dres.RemoveAll();
	head = tail = NULL;
}

void CDreList::MakeSolidCircles()
{
	CDisplayList *dl = doc->m_dlist;
	CIter<CDre> id (&dres);
	for (CDre *d = id.First(); d; d = id.Next())
	{
		if (!d->dl_el || d->dl_el->gtype != DL_HOLLOW_CIRC || d->dl_el->holew) 
			continue;																// Weird...
		dl->Set_holew( d->dl_el, 10000000 );						// CPT2 has the effect that the circle is drawn solid.  An ugly hack, I know...
		d->dl_el->radius = d->dl_el->w;								// A safe place to store the normal width (I guess)
		dl->Set_w( d->dl_el, 250*NM_PER_MIL );
	}
}

void CDreList::MakeHollowCircles()
{
	// Return dre symbol to normal (a hollow ring)
	CDisplayList *dl = doc->m_dlist;
	CIter<CDre> id (&dres);
	for (CDre *d = id.First(); d; d = id.Next())
	{
		if (!d->dl_el || d->dl_el->gtype != DL_HOLLOW_CIRC || !d->dl_el->holew) 
			continue;																// Weird...
		d->dl_el->w = d->dl_el->radius;
		dl->Set_holew( d->dl_el, 0 );
	}
}


// CPT2.  Thought it might be fun to cook up an algorithm for sorting DRE's, so that when users use arrow keys to move among them, they
// proceed in a sort of geometrical order.  The order is, roughly:  start with the DRE closest to the NW corner of the PCB space;  after that,
// on each step just find the nearest DRE that has not already been assigned to the sorted list.  To implement this I use helper struct
// quad, which represents the division of a variously-sized portion of the space into 4 quadrants, NW, NE, SW, and SE.  The quad structure
// assigns each of its quadrants to one of the following categories: 
//   (00) empty;
//   (01) a pointer to a smaller sub-quad structure
//   (10) a pointer to a cluster of DRE's --- DRE's that are all at the same position, relative to the algorithm's fundamental granularity.
//        That is to say, throughout all routines, DRE positions (which occupy the range from -2**31 to 2**31 nm along each axis) constantly 
//        get mapped into the range [0,0xfff] x [0,0xfff];  accordingly DRE's within 2**20 nm (~1 mm) of each other get assigned to the same cluster.
// Routine SetupQuads() establishes a root quad representing the whole space, from -2**31 to 2**31 nm along each axis.  From there we have a tree 
//   of subquadrants down to the aforementioned 2**20 nm granularity.  But note that leaf nodes may be larger than that if a DRE is in an uncrowded
//   region.  The size of a given quad does not have to be marked explicitly, as the routines can keep track themselves.

struct quad 
{
	unsigned char flags;			// Top 2 bits: category of NW quadrant;  next 2: category of NE quadrant; etc.
	void *ptrNW;					// Either a cluster*, or a quad*, or NULL
	void *ptrNE;
	void *ptrSW;
	void *ptrSE;
};

enum {
	// Values within quad::flags
	bitClstrNW  = 0x80,
	bitQuadNW   = 0x40,
	bitClstrNE  = 0x20,
	bitQuadNE   = 0x10,
	bitClstrSW  = 0x08,
	bitQuadSW   = 0x04,
	bitClstrSE  = 0x02,
	bitQuadSE   = 0x01
};

// Lookup tables for the above bits, indexed by quadrant numbers:
static unsigned char bitClstr[4] = { bitClstrNW, bitClstrNE, bitClstrSW, bitClstrSE };
static unsigned char bitQuad[4] = { bitQuadNW, bitQuadNE, bitQuadSW, bitQuadSE };

struct cluster 
{
	// Represents a cluster of DRE's that have the same position (relative to the usual 2**20 nm granularity).
	int x, y;						// These are position values for the cluster, as mapped into the [0,0xfff] x [0,0xfff] range
	int ct;							// Number of DRE's in the cluster
};

// The coordinate mapping mentioned above, from [-2**31, 2**31) to the range [0, 0xfff].
inline int mappedX(CDre *dre)
	{ return (dre->x>>20) + 0x800; }
inline int mappedY(CDre *dre)
	{ return (dre->y>>20) + 0x800; }

void CDreList::SetupQuads(quad *heap, cluster *clusters)
{
	// Performs the process of subdividing the entire PCB space (mapped to the [0,0xfff] x [0,0xfff] range) according to the distribution
	// of DRE's.  "heap" has been allocated to be large enough, and has been completely zeroed out.  "cluster" has also been alloc'ed by caller.
	// heap[0] will be the root node, covering the entire space.
	quad *nextQuad = heap+1;
	cluster *nextClstr = clusters;
	CIter<CDre> idre (&dres);
	for (CDre *dre = idre.First(); dre; dre = idre.Next())
	{
		// Add dre to the tree.  dre->utility will receive an offset value, indicating which cluster it's been assigned to.
		int x = mappedX(dre), y = mappedY(dre);
		quad *q = heap;			// The root node...
		for (int bit = 0x800; bit; bit >>= 1)
		{
			void **ptr;
			unsigned char bitClstr, bitQuad;
			if (x & bit)
				if (y & bit) 
					ptr = &q->ptrNE, bitClstr = bitClstrNE, bitQuad = bitQuadNE;
				else
					ptr = &q->ptrSE, bitClstr = bitClstrSE, bitQuad = bitQuadSE;
			else
				if (y & bit)
					ptr = &q->ptrNW, bitClstr = bitClstrNW, bitQuad = bitQuadNW;
				else
					ptr = &q->ptrSW, bitClstr = bitClstrSW, bitQuad = bitQuadSW;
			if (q->flags & bitQuad)
				// Recurse down to the subquad
				{ q = (quad*) *ptr; continue; }
			if (q->flags & bitClstr)
			{
				// See if dre should be added to the cluster
				cluster *c = (cluster*) *ptr;
				if (c->x == x && c->y == y)
				{
					// Yes...
					c->ct++;
					dre->utility = c - clusters;
					break;								// Next dre, please
				}
				// Must create a new cluster for dre, and put a new subquad into the appropriate quadrant of q.  May have to recursively create
				// subquads until we can separate the old cluster from the new.
				cluster *c2 = nextClstr++;
				c2->ct = 1;
				c2->x = x, c2->y = y;
				dre->utility = c2 - clusters;
				// Convert quadrant from a cluster type into a quad type: 
				quad *q2 = nextQuad++;
				*ptr = q2;
				q->flags ^= bitClstr | bitQuad;
				for (bit >>= 1; bit; bit >>= 1)
				{
					if ((c->x & bit) != (x & bit) || (c->y & bit) != (y & bit))
						// c and c2 will be assigned to different quadrants of q2!
						break;
					// Create a new shared subquad
					if (x & bit)
						if (y & bit) 
							ptr = &q2->ptrNE, bitQuad = bitQuadNE;
						else
							ptr = &q2->ptrSE, bitQuad = bitQuadSE;
					else
						if (y & bit)
							ptr = &q2->ptrNW, bitQuad = bitQuadNW;
						else
							ptr = &q2->ptrSW, bitQuad = bitQuadSW;
					q2->flags = bitQuad;
					q2 = nextQuad++;
					*ptr = q2;
				}
				// Now fill in q2 with the 2 distinct cluster entries
				if (x & bit)
					if (y & bit) 
						ptr = &q2->ptrNE, bitClstr = bitClstrNE;
					else
						ptr = &q2->ptrSE, bitClstr = bitClstrSE;
				else
					if (y & bit)
						ptr = &q2->ptrNW, bitClstr = bitClstrNW;
					else
						ptr = &q2->ptrSW, bitClstr = bitClstrSW;
				*ptr = c2;
				q2->flags = bitClstr;
				if (c->x & bit)
					if (c->y & bit) 
						ptr = &q2->ptrNE, bitClstr = bitClstrNE;
					else
						ptr = &q2->ptrSE, bitClstr = bitClstrSE;
				else
					if (c->y & bit)
						ptr = &q2->ptrNW, bitClstr = bitClstrNW;
					else
						ptr = &q2->ptrSW, bitClstr = bitClstrSW;
				*ptr = c;
				q2->flags |= bitClstr;
				break;						// Next dre, please
			}

			// If we get here, we've reached an empty quadrant within "q".  Fill it with a new cluster for "dre".
			cluster *c = nextClstr++;
			c->ct = 1;
			c->x = x, c->y = y;
			dre->utility = c - clusters;
			*ptr = c;
			q->flags |= bitClstr;
			break;							// Next dre, please
		}
	}
}

inline int MinDist2(int x, int y, int qx0, int qx1, int qy0, int qy1)
{
	// Helper function:  Get the minimum distance from point (x,y) to the quad with bounds [qx0,qx1] x [qy0,qy1].  
	// Actually, throughout these routines we always use squared distances to avoid an unnecessary sqrt op.
	int dx, dy;
	if (qy0<=y && y<=qy1)
	{
		if (qx0<=x && x<=qx1)
			return 0;							// (x,y) is within the quad, so the answer is 0...
		dx = x>=qx1? x-qx1: qx0-x;
		return dx*dx;
	}
	else if (qx0<=x && x<=qx1)
	{
		dy = y>=qy1? y-qy1: qy0-y;
		return dy*dy;
	}
	else 
	{
		dx = x>=qx1? x-qx1: qx0-x;
		dy = y>=qy1? y-qy1: qy0-y;
		return dx*dx + dy*dy;
	}
}

int FindClosestCluster(int x, int y, quad *q, int qx0, int qx1, int qy0, int qy1, int d2max, cluster **result)
{
	// Look within quad "q" (which has bounds [qx0,qx1] x [qy0,qy1]), for the cluster closest to (x,y).  (x,y) may be either inside or 
	// outside of q's bounds.  Param d2max is included as an optimization: sometimes the routine can finish more quickly if it knows that
	// results beyond that distance are worthless.  The result cluster ptr is
	// put into "result" (may be null on failure).  Returns the distance squared on success, or INT_MAX otherwise.
	if (q->flags==0)
		{ *result = NULL; return INT_MAX; }
	// Determine the order in which we should examine q's quadrants.  This depends on (x,y)'s spatial relationship to the quadrants.  The
	// result, "order", is a hex code like e.g. 0x0123, which means look first in quadrant 0 (NW), then quadrant 1 (NE), then 2 (SW), then 3 (SE).
	int qxMid = (qx0+qx1)/2, qyMid = (qy0+qy1)/2;
	int order;
	if (x <= qx0)
		order = y < qyMid? 0x2031: 0x0213;
	else if (x >= qx1)
		order = y < qyMid? 0x3120: 0x1302;
	else if (y < qyMid)
		order = x < qxMid? 0x2301: 0x3210;
	else
		order = x < qxMid? 0x0123: 0x1032;
	// Get minimum distances from (x,y) to each of q's quadrants.
	int minDist2[4];
	minDist2[0] = MinDist2(x, y, qx0, qxMid, qyMid, qy1);
	minDist2[1] = MinDist2(x, y, qxMid, qx1, qyMid, qy1);
	minDist2[2] = MinDist2(x, y, qx0, qxMid, qy0, qyMid);
	minDist2[3] = MinDist2(x, y, qxMid, qx1, qy0, qyMid);
	// Now examine q's quadrants in order:
	int d2min = d2max;
	cluster *best = NULL, *tmp;
	for (int shift = 12; shift >= 0; shift -= 4)
	{
		int quadrant = (order>>shift) & 0xf;
		if (d2min < minDist2[quadrant])
			continue;								// No chance that this quadrant will contain anything better than our current best.
		void *ptr;
		int xMin, xMax, yMin, yMax;
		if (quadrant==0)
			ptr = q->ptrNW,
			xMin = qx0, xMax = qxMid,
			yMin = qyMid, yMax = qy1;
		else if (quadrant==1)
			ptr = q->ptrNE,
			xMin = qxMid, xMax = qx1,
			yMin = qyMid, yMax = qy1;
		else if (quadrant==2)
			ptr = q->ptrSW,
			xMin = qx0, xMax = qxMid,
			yMin = qy0, yMax = qyMid;
		else
			ptr = q->ptrSE,
			xMin = qxMid, xMax = qx1,
			yMin = qy0, yMax = qyMid;
		int d2;
		if (q->flags & bitQuad[quadrant])
			// Quadrant is a subquad, so must recurse
			d2 = FindClosestCluster(x, y, (quad*) ptr, xMin, xMax, yMin, yMax, d2min, &tmp);
		else if (q->flags & bitClstr[quadrant])
		{
			// Quadrant contains a cluster, so see how good that is:
			tmp = (cluster*) ptr;
			int dx = x - tmp->x, dy = y - tmp->y;
			d2 = dx*dx + dy*dy;
		}
		else
			continue;		// Empty quadrant!
		if (d2 < d2min)
			d2min = d2,
			best = tmp;
	}
	*result = best;
	return d2min;
}

void DetachCluster(quad *heap, cluster *c)
{
	// After a cluster has been placed in the ordering, we want to remove the reference to it from the tree.  Note that we don't actually bother
	// blanking out the ptr members of any quad structures, but just tinker with the flags.
	quad *ancestors[12];
	unsigned char bits[12];
	int depth = 0;
	// Locate c, leaving a trail of ancestral quads in "ancestors"; also put into "bits" info about which quadrant is involved in each case.
	quad *q = heap;			// The root node...
	for (int bit = 0x800; bit; bit >>= 1)
	{
		void *ptr;
		unsigned char bitClstr, bitQuad;
		if (c->x & bit)
			if (c->y & bit) 
				ptr = q->ptrNE, bitClstr = bitClstrNE, bitQuad = bitQuadNE;
			else
				ptr = q->ptrSE, bitClstr = bitClstrSE, bitQuad = bitQuadSE;
		else
			if (c->y & bit)
				ptr = q->ptrNW, bitClstr = bitClstrNW, bitQuad = bitQuadNW;
			else
				ptr = q->ptrSW, bitClstr = bitClstrSW, bitQuad = bitQuadSW;
		ancestors[depth] = q;
		bits[depth] = bitQuad | bitClstr;
		depth++;
		if (q->flags & bitQuad)
			// Recurse down to the subquad
			q = (quad*) ptr;
		else if (q->flags & bitClstr)
			{ ASSERT( (cluster*) ptr == c ); break; }
		else
			ASSERT(0);
	}
	for (depth--; depth>=0; depth--)
	{
		ancestors[depth]->flags &= ~bits[depth];
		if (ancestors[depth]->flags)
			// Ancestor isn't totally empty yet...
			break;
		// Ancestor has been cleaned out, so go up to its parent and remove the reference to it...
	}
}

void CDreList::Sort()
{
	CArray<CDre*> sorted;
	int cDres = dres.GetSize();
	sorted.SetSize( cDres );
	if (cDres==0) 
		{ head = tail = NULL; return; }

	// Setup the whole tree structure based on the current set of DRE's.
	quad *heap = (quad*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 6*cDres * sizeof(quad));			// Should be ample space
	cluster *clusters = (cluster*) HeapAlloc(GetProcessHeap(), 0, cDres* sizeof(cluster));
	SetupQuads(heap, clusters);

	// Locate our first cluster in the ordering, aiming as far to the NW as we can. 
	quad *q = heap;				// Root node.
	cluster *c = NULL;
	while (1)
	{
		if (q->flags & bitClstrNW)
			{ c = (cluster*) q->ptrNW; break; }
		if (q->flags & bitQuadNW)
			{ q = (quad*) q->ptrNW; continue; }
		// Second choice:  look at the NE quadrant.  After that, it's SW, then SE.
		if (q->flags & bitClstrNE)
			{ c = (cluster*) q->ptrNE; break; }
		if (q->flags & bitQuadNE)
			{ q = (quad*) q->ptrNE; continue; }
		if (q->flags & bitClstrSW)
			{ c = (cluster*) q->ptrSW; break; }
		if (q->flags & bitQuadSW)
			{ q = (quad*) q->ptrSW; continue; }
		if (q->flags & bitClstrSE)
			{ c = (cluster*) q->ptrSE; break; }
		if (q->flags & bitQuadSE)
			{ q = (quad*) q->ptrSE; continue; }
		ASSERT(0);
	}

	// Now we do the ordering work!  As clusters are ordered, their "ct" fields get changed into offset values, indicating where	
	// they will ultimately get positioned in the output array.
	int currOffset = 0;
	while (c) 
	{
		int ct = c->ct;
		c->ct = currOffset;
		currOffset += ct;
		DetachCluster( heap, c );
		// Among the clusters that remain, find the one closest to c's position:
		FindClosestCluster(c->x, c->y, heap, 0, 0x1000, 0, 0x1000, INT_MAX, &c);
	}

	// Now we can assign dre's to "sorted".
	CIter<CDre> idre (&dres);
	for (CDre *dre = idre.First(); dre; dre = idre.Next())
	{
		cluster *c = clusters + dre->utility;
		int pos = c->ct;
		c->ct++;						// So that other members of the cluster will get positioned later.
		sorted.SetAt(pos, dre);
		dre->index = pos;
		// Also fix up dre->str (prefix the index number + 1)
		CString tmp = dre->str;
		dre->str.Format("%d: %s", pos+1, tmp);
	}

	// Create the linked list of dre's to reflect the sort order.  Having a linked list lets us delete DRE's without harmful side effects.
	head = sorted[0];
	tail = sorted[cDres-1];
	for (int i=0; i<cDres; i++)
	{
		if (i==0)
			sorted[i]->prev = NULL;
		else
			sorted[i]->prev = sorted[i-1];
		if (i==cDres-1)
			sorted[i]->next = NULL;
		else
			sorted[i]->next = sorted[i+1];
	}

	HeapFree(GetProcessHeap(), 0, heap);
	HeapFree(GetProcessHeap(), 0, clusters);
}