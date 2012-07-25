// PcbItem.h:  Proposal for a new class hierarchy encompassing all the items that
//   appear on a PCB.  By having a single base class, cpcb_item, and using pointers to objects 
//   belonging to derived classes, it should be possible to supplant the old system of id's.
// Also included are new template classes, carray<T> and citer<T>;  we can rely mostly on them
//   instead of CArray<T>/CArray<T*> and the various CIter_XXX classes.  carray and citer have a number of
//   safety features, including: (1) if a member of a carray gets deleted, references to it 
//   in the carray will be removed automatically;  (2) if a member of a carray gets detached or
//   deleted in the midst of a loop, citer can handle the situation gracefully;  (3) if a carray as
//   a whole gets deleted in the midst of a loop, its members are detached and citer handles that too;  (4) removal of items 
//   from a carray, and detection of their presence in the carray, are quick.

#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "DisplayList.h"
// #include "PartList.h"
#include "PolyLine.h"
#include "UndoList.h"
// #include "LinkList.h"
// #include "Cuid.h"
// #include "PartListNew.h"		// CPT2

class cpcb_item;
class carray_link;
template <class T> class carray;  // NB "T" is expected to be a cpcb_item class
template <class T> class citer;	  // Ditto

class cvertex2;
class ctee;
class cseg2;
class cconnect2;
class cpin2;
class cpart2;
class ccorner;			// TODO: rename ccorner
class cside;			// TODO: rename cside
class ccontour;
class cpolyline;			// TODO: rename cpolyline
class carea2;
class csmcutout;
class cboard;
class coutline;
class cnet2;
class ctext;				// TODO: rename ctext
class creftext;
class cvaluetext;
class ccentroid;
class cglue;
class cdre;
class cpadstack;

class undo_con;
class undo_seg;
class undo_vtx;

class cnetlist;
class CFreePcbDoc;
class cpartlist;
class ctextlist;
class cpadstack;
class Shape;

struct stroke;

enum typebits {
	// Bit flags to represent the types of the various types of PCB items.  The main use for these will be in implementing selection masks.
	bitVia =			0x1,
	bitPinVtx =			0x2,
	bitTraceVtx =		0x4,
	bitTeeVtx =			0x8,
	bitTee =			0x10,			// Only returned by GetTypeBit() for non-vias (tees with vias produce a bitVia).
	bitSeg =			0x20,
	bitConnect =		0x40,
	bitPin =			0x80,
	bitPart =			0x100,
	bitAreaCorner =		0x200,
	bitAreaSide =		0x400,
	bitSmCorner =		0x800,
	bitSmSide =			0x1000,
	bitBoardCorner =	0x2000,
	bitBoardSide =		0x4000,
	bitOutlineCorner =	0x8000,			// Within the footprint editor only...
	bitOutlineSide =	0x10000,		// Within the footprint editor only...
	bitContour =		0x20000,
	bitArea =			0x40000,
	bitSmCutout =		0x80000,
	bitBoard =			0x100000,
	bitOutline =		0x200000,		// Fp editor only...
	bitNet =			0x400000,
	bitText =			0x800000,
	bitRefText =		0x1000000,
	bitValueText =		0x2000000,
	bitCentroid =		0x4000000,		// Fp editor only...
	bitGlue =		0x8000000,		// Fp editor only...
	bitDRE =			0x10000000,
	bitPadstack =		0x20000000,		// Fp editor only...
	bitOther =			0x80000000,
	bitsNetItem = bitVia | bitPinVtx | bitTraceVtx | bitTeeVtx | bitTee | bitSeg | bitConnect |
				  bitAreaCorner | bitAreaSide | bitArea,
	bitsPartItem = bitPin | bitPart,
	bitsFootItem = bitOutlineCorner | bitOutlineSide | bitOutline | bitText | bitRefText | bitValueText | 
				   bitCentroid | bitGlue | bitPadstack,
	bitsSelectableForGroup = bitVia | bitSeg | bitConnect | bitPart | bitText | bitAreaSide | bitSmSide | bitBoardSide
};

class cpcb_item 
{
	// THE BASE CLASS FOR EVERY ITEM ON A PCB.  
	// Best way I could figure out to give the various carray types access to cpcb_item's innards was to list 'em as friends 1 by 1.
	// An ugly PITA, but I suppose necessary:
	friend class carray<cpcb_item>;
	friend class carray<cvertex2>;
	friend class carray<ctee>;
	friend class carray<cseg2>;
	friend class carray<cconnect2>;
	friend class carray<cpin2>;
	friend class carray<cpart2>;
	friend class carray<ccorner>;
	friend class carray<cside>;
	friend class carray<ccontour>;
	friend class carray<cpolyline>;
	friend class carray<carea2>;
	friend class carray<csmcutout>;
	friend class carray<cboard>;
	friend class carray<coutline>;
	friend class carray<cnet2>;
	friend class carray<ctext>;
	friend class carray<creftext>;
	friend class carray<cvaluetext>;
	friend class carray<ccentroid>;
	friend class carray<cglue>;
	friend class carray<cdre>;
	friend class carray<cpadstack>;

	carray_link *carray_list;	// List of carray's into which this item has been added
	int m_uid;
	static int next_uid;
public:							// ?
	CFreePcbDoc *doc;
	dl_element *dl_el;
	dl_element *dl_sel;
	int utility;
	bool bDrawn;
	enum { NOERR = 0, NO_DLIST, NO_FOOTPRINT, ALREADY_DRAWN, EMPTY_TEXT };
	enum { STRAIGHT, ARC_CW, ARC_CCW };	// polyline side styles
	enum { NO_HATCH, DIAGONAL_FULL, DIAGONAL_EDGE }; // polyline hatch styles

protected:
	cpcb_item(CFreePcbDoc *_doc);				// Done in the .cpp.
	~cpcb_item();								// Done in cpp.  When an item is destroyed, references to it are automatically
												// removed from any carrays to which it belongs.
public:
	int UID() { return m_uid; }
	// void SetDoc(CFreePcbDoc *doc);			// Done in cpp.  CPT2 r317 maybe dump...
	void MustRedraw();							// CPT2 r313.  My latest-n-greatest new system for drawing/undrawing (see notes.txt). Done in cpp
	bool IsHit(int x, int y);					// Done in cpp.
	bool IsDrawn() { return bDrawn; }

	// Virtual functions:
	virtual	int Draw() { return NOERR; }
	virtual void Undraw() { }
	virtual void Highlight() { }
	virtual bool IsValid() { return false; }	// Or make it pure virtual.

	// Type identification functions.  All return false by default, but are overridden in specified derived classes.
	virtual bool IsVertex() { return false; }
	virtual bool IsVia() { return false; }
	virtual bool IsSlaveVtx() { return false; }
	virtual bool IsPinVtx() { return false; }
	virtual bool IsEndVtx() { return false; }
	virtual bool IsTraceVtx() { return false; }
	virtual bool IsTee() { return false; }
	virtual bool IsSeg() { return false; }
	virtual bool IsConnect() { return false; }
	virtual bool IsPin() { return false; }
	virtual bool IsPart() { return false; }
	virtual bool IsCorner() { return false; }
	virtual bool IsBoardCorner() { return false; }
	virtual bool IsAreaCorner() { return false; }
	virtual bool IsSmCorner() { return false; }
	virtual bool IsOutlineCorner() { return false; }
	virtual bool IsSide() { return false; }
	virtual bool IsAreaSide() { return false; }
	virtual bool IsBoardSide() { return false; }
	virtual bool IsSmSide() { return false; }
	virtual bool IsOutlineSide() { return false; }
	virtual bool IsContour() { return false; }
	virtual bool IsPolyline() { return false; }
	virtual bool IsArea() { return false; }
	virtual bool IsSmCutout() { return false; }
	virtual bool IsBoard() { return false; }
	virtual bool IsOutline() { return false; }
	virtual bool IsNet() { return false; }
	virtual bool IsText() { return false; }			// CPT2 true for ctext, creftext and cvaluetext
	virtual bool IsRefText() { return false; }
	virtual bool IsValueText() { return false; }
	virtual bool IsCentroid() { return false; }
	virtual bool IsGlue() { return false; }
	virtual bool IsDRE() { return false; }
	virtual bool IsPadstack() { return false; }

	virtual int GetTypeBit() { return 0; }				// See "enum typebits" above for return values from the various derived classes
	bool IsPartItem() { return (GetTypeBit() & bitsPartItem) != 0; }
	bool IsNetItem() { return (GetTypeBit() & bitsNetItem) != 0; }
	bool IsSelectableForGroup() { return (GetTypeBit() & bitsSelectableForGroup) != 0; }
	bool IsFootItem() { return (GetTypeBit() & bitsFootItem) != 0; }

	// Type casting functions.  All return null by default, but are overridden to return type-cast pointers in specified derived classes
	virtual cvertex2 *ToVertex() { return NULL; }
	virtual ctee *ToTee() { return NULL; }
	virtual cseg2 *ToSeg() { return NULL; }
	virtual cconnect2 *ToConnect() { return NULL; }
	virtual cpin2 *ToPin() { return NULL; }
	virtual cpart2 *ToPart() { return NULL; }
	virtual ccorner *ToCorner() { return NULL; }
	virtual cside *ToSide() { return NULL; }
	virtual ccontour *ToContour() { return NULL; }
	virtual cpolyline *ToPolyline() { return NULL; }
	virtual carea2 *ToArea() { return NULL; }
	virtual csmcutout *ToSmCutout() { return NULL; }
	virtual cboard *ToBoard() { return NULL; }
	virtual coutline *ToOutline() { return NULL; }
	virtual cnet2 *ToNet() { return NULL; }
	virtual ctext *ToText() { return NULL; }
	virtual creftext *ToRefText() { return NULL; }
	virtual cvaluetext *ToValueText() { return NULL; }
	virtual ccentroid *ToCentroid() { return NULL; }
	virtual cglue *ToGlue() { return NULL; }
	virtual cdre *ToDRE() { return NULL; }
	virtual cpadstack *ToPadstack() { return NULL; }

	virtual cnet2 *GetNet() { return NULL; }					// Returns something for items that belong to a net.
	virtual cconnect2 *GetConnect() { return NULL; }			// Similar
	virtual cpolyline *GetPolyline() { return NULL; }
	carea2 *GetArea();											// In cpp
	virtual int GetLayer() { return -1; }
	virtual void GetStatusStr( CString *str ) { *str = ""; }
	virtual void ForceVia() { }									// Implemented in cvertex2 and ctee
	virtual void UnforceVia() { }								// Implemented in cvertex2 and ctee
};

class carray_link
{
	// Used in forging linked lists of carray<T> pointers (which are declared below as void pointers for simplicity).  When a cpcb_item is
	// added to a carray, one of these links is hooked to its carray_list.  In that way we know which carrays it belongs to (I don't foresee
	// many items belonging to more than 1 or 2 arrays in general).  We can also use the "off" member of this structure to determine what position
	// the item has in the indicated array.
	// Same dumb business with friends as in cpcb_item :(
	friend class cpcb_item;
	friend class carray<cpcb_item>;
	friend class carray<cvertex2>;
	friend class carray<ctee>;
	friend class carray<cseg2>;
	friend class carray<cconnect2>;
	friend class carray<cpin2>;
	friend class carray<cpart2>;
	friend class carray<ccorner>;
	friend class carray<cside>;
	friend class carray<ccontour>;
	friend class carray<cpolyline>;
	friend class carray<carea2>;
	friend class carray<csmcutout>;
	friend class carray<cboard>;
	friend class carray<coutline>;
	friend class carray<cnet2>;
	friend class carray<ctext>;
	friend class carray<creftext>;
	friend class carray<cvaluetext>;
	friend class carray<ccentroid>;
	friend class carray<cglue>;
	friend class carray<cdre>;
	friend class carray<cpadstack>;

	void *arr;									// Really a carray<T> pointer, for some T.
	int off;
	carray_link *next;
};


template <class T> class carray 
{
	// An array designed to hold items in classes derived from cpcb_item.  Note that entries in these arrays are pointers, not objects (so 
	// that carray<cpin2> is more like CArray<cpin2*> than CArray<cpin2>).  Each cpcb_item has an attached linked list of the carrays to which it
	// belongs, which allows various maintenance chores to be done automatically.  When an item is removed from a carray, the slot it occupied
	// is marked as free and linked to a list for later reuse.
	// For ease of reference and for a potential inline speed-boost, I put the code here in the .h
	friend class cpcb_item;
	friend class citer<T>;
	T **heap;					// Expandable array of data pointers.
	unsigned char *flags;		// Bitset, indicating which elements of "heap" contain valid entries.  A zero bit indicates a free slot.
								// Bits beyond position "maxOff" are undefined.
	int nHeap;					// Total number of slots in heap
	int nItems;					// Number of filled slots
	int free;					// The head of the free slot list (an index into "heap").  -1 => everything through maxOff is filled
	int maxOff;					// Maximum offset among the slots that are either filled, or hooked to the free list
	citer<T> *iters;			// Linked list of iterators that are actively looping through this.  Typically I don't expect more than 1 or 2...

public:
	carray(int nHeapInit = 16)
	{
		// Create the array, with the given number of initial entries in the heap.
		heap = (T**) HeapAlloc(GetProcessHeap(), 0, nHeapInit*sizeof(T**));
		flags = (unsigned char*) HeapAlloc(GetProcessHeap(), 0, nHeapInit/8 + 1);
		nHeap = nHeapInit;
		nItems = 0;
		maxOff = free = -1;
		iters = NULL;
	}

	~carray()
	{
		// Destructor must remove all references to this from its member items.  But it doesn't destroy the members.
		RemoveAll();
		HeapFree(GetProcessHeap(), 0, heap);
		HeapFree(GetProcessHeap(), 0, flags);
		// Blank out references to this from active iterators:
		for (citer<T> *iter = iters; iter; iter = iter->nextIter)
			iter->arr = NULL;
	}

	void Add(T* item)
	{
		// NB If item is null, we get a crash.  OK?
		// Add item to this carray.  But first check if the item already belongs to this, and return silently if so (?)
		for (carray_link *link = item->carray_list; link; link = link->next)
			if (link->arr==this) return;
		// See if heap needs to expand.  It grows by a factor of ~1.5 each time:
		if (nItems==nHeap)
		{
			int newNHeap = nHeap*3/2 + 1;
			T** newHeap = (T**) HeapAlloc(GetProcessHeap(), 0, newNHeap*sizeof(T**));
			unsigned char *newFlags = (unsigned char*) HeapAlloc(GetProcessHeap(), 0, (newNHeap/8) + 1);
			CopyMemory(newHeap, heap, nHeap*sizeof(T**));
			CopyMemory(newFlags, flags, nHeap/8 + 1);
			HeapFree(GetProcessHeap(), 0, heap);
			HeapFree(GetProcessHeap(), 0, flags);
			heap = newHeap;
			flags = newFlags;
			nHeap = newNHeap;
		}
		// Determine offset where item will be placed:
		int off; 
		if (free>=0)
			off = free,
			free = *(int*) (heap+off);
		else
			off = ++maxOff;
		// Add item!  Then create a carray_link that connects the item with this carray.
		heap[off] = item;
		flags[off>>3] |= 1<<(off&7);						// Set flag for "off"
		carray_link *link = new carray_link();
		link->off = off;
		link->arr = this;
		link->next = item->carray_list;
		item->carray_list = link;
		nItems++;
	}

	void Add(carray<T> *arr) 
	{
		// Append the members of "arr" onto this.
		for (int i=0; i<=arr->maxOff; i++) 
			if (arr->flags[i>>3] & (1<<(i&7)))
				Add(arr->heap[i]);
	}

	bool Contains(T* item)
	{
		// Return true if "item" is in the array.  NB if item is null, return false. (?)
		if (!item) return false;
		for (carray_link *link = item->carray_list; link; link = link->next)
			if (link->arr == this) return true;
		return false;
	}

	int OffsetOf(T* item)
	{
		for (carray_link *link = item->carray_list; link; link = link->next)
			if (link->arr == this) return link->off;
		return INT_MAX;
	}

	void Remove(T* item)
	{
		// Remove "item" from this.  If item isn't in the list, do an ASSERT (?).  If item is null, crash (?)
		carray_link *link, *prev = NULL;
		for (link = item->carray_list; link; prev = link, link = link->next)
			if (link->arr == this) break;
		ASSERT(link!=0);
		// Unhook and destroy "link" (after extracting its offset info)
		int off = link->off;
		if (!prev) item->carray_list = link->next;
		else       prev->next = link->next;
		delete link;
		// Remove item!
		ASSERT(heap[off]==item);			// For now...
		heap[off] = (T*) free;				// Add "off" to the free-offset list
		free = off;
		flags[off>>3] &= ~(1<<(off&7));		// Clear flag for "off"
		nItems--;
	}

	void RemoveAll()
	{
		// Remove all items from array (but don't destroy them).
		for (int i=0; i<=maxOff; i++) 
			if (flags[i>>3] & (1<<(i&7)))
				Remove(heap[i]);
		maxOff = free = -1;							// Ensures that if array fills up again, iteration through it will be efficient.
	}

	void DestroyAll()
	{
		// Destroy all items in array (and their destructors will also ensure that the array is empty at the end)
		for (int i=0; i<=maxOff; i++) 
			if (flags[i>>3] & (1<<(i&7)))
				delete heap[i];
		maxOff = free = -1;							// Ensures that if array fills up again, iteration through it will be efficient.
	}

	int GetSize() { return nItems; }
	bool IsEmpty() { return nItems==0; }
	
	T *FindByUID(int uid)
	{
		for (int i=0; i<=maxOff; i++)
			if (flags[i>>3] & (1<<(i&7)))
				if (heap[i]->UID() == uid)
					return heap[i];
		return NULL;
	}

	T *FindByUtility(int utility)
	{
		for (int i=0; i<=maxOff; i++)
			if (flags[i>>3] & (1<<(i&7)))
				if (heap[i]->utility == utility)
					return heap[i];
		return NULL;
	}

	void SetUtility(int utility)
	{
		// Mark utility value for all members of the carray
		for (int i=0; i<=maxOff; i++)
			if (flags[i>>3] & (1<<(i&7)))
				heap[i]->utility = utility;
	}

	T *First()
	{
		if (nItems < 1) return NULL;
		for (int i=0; i<=maxOff; i++)
			if (flags[i>>3] & (1<<(i&7)))
				return heap[i];
		ASSERT(0);
		return NULL;
	}

	bool ReadItems(T** out, int ct) 
	{
		// Read "ct" items from the carray into "out". Return false if ct is too large.  Maybe unneeded.
		if (ct>nItems) return false;
		int j = 0;
		for (int i=0; j<ct; i++)
			if (flags[i>>3] & (1<<(i&7)))
				out[j++] = heap[i];
		return true;
	}
};


template <class T> class citer
{
	// Use these classes to iterate through the members of a carray<T>.  That way we can handle it gracefully if an item is removed from the array 
	//  from within the loop, and even if the array as a whole is destroyed.
	// Sample usage:
	//		citer<cseg2> iseg (con->m_segs);
	//		for (cseg2 *seg = iseg.First(); seg; seg = iseg.Next())
	//			...
	friend class carray<T>;
	carray<T> *arr;
	int nextOff;								// Next offset at which Next() will seek a new entry.
	citer<T> *nextIter;							// Active iterators for a particular array are linked (singly) in a list.

public:
	citer(carray<T> *arr0)
	{
		arr = arr0;
		nextOff = 0;
		// Hook this to arr's list of iterators.
		nextIter = arr->iters;
		arr->iters = this;
	}
	citer(carray<T> *arr0, T *at) 
	{
		// Iterate through, starting at the position of "at". If "at" is not in the array, advance iterator past the end
		arr = arr0;
		nextOff = arr0->OffsetOf(at);
		nextIter = arr->iters;
		arr->iters = this;
	}

	~citer()
	{
		// Remove this iterator from its array's list of iterators.  Since the list is singly-linked, the process may be inefficient;  
		//   but I expect the list to be very short and the overall savings should outweight the penalty.
		if (!arr) return;								// Array has been destroyed!
		if (arr->iters==this)
			arr->iters = nextIter;
		else 
		{
			citer<T> *prev = NULL;
			for (citer<T> *iter = arr->iters; iter; iter = iter->nextIter)
				if (iter->nextIter==this) 
					{ prev = iter; break; }
			ASSERT(prev!=NULL);
			prev->nextIter = nextIter;
		}
	}

	T* Next() 
	{
		if (!arr) return NULL;							// Array has been destroyed!
		for ( ; nextOff <= arr->maxOff; nextOff++)
			if (arr->flags[nextOff>>3] & (1<<(nextOff&7)))
				return arr->heap[nextOff++];
		return NULL;
	}

	T* First()
		{ nextOff = 0; return Next(); }
};



/**********************************************************************************************/
/*  RELATED TO cseg2/cvertex2/cconnect2                                                          */
/**********************************************************************************************/

// cvertex2: describes a vertex between segments in a connection
class cvertex2: public cpcb_item
{
public:
	enum {
		VIA_NO_CONNECT = 0,
		VIA_TRACE = 1,
		VIA_AREA = 2
	};

	cconnect2 * m_con;			// parent connection
	cnet2 *m_net;				// CPT2.  Very likely useful.
	cseg2 *preSeg, *postSeg;		// Succeeding and following segs.  If either is null, this is an end seg.
	ctee *tee;					// Points to a tee structure at tee-junctions, null otherwise
	cpin2 *pin;					// If this vertex is at a pin, point to the said object
	int x, y;					// coords
	bool bNeedsThermal;			// CPT2 new.  If true, there's an area in this same net that intersects this vertex.  Therefore, if this is a via, we'll need
								// to draw a thermal as well.
	// CPT2: put into cpin2 (?) int pad_layer;				// layer of pad if this is first or last vertex, otherwise 0
	int force_via_flag;			// force a via even if no layer change
	int via_w, via_hole_w;		// via width and hole width (via_w==0 means no via)
	int utility2;				// used for various functions
	CArray<dl_element*> dl_els;	// CPT2 renamed;  contains one dl_element per layer
	dl_element * dl_hole;		// hole in via
	dl_element * dl_thermal;	// CPT2 new.  
	// CDisplayList * m_dlist;		// NULL if not drawable.  CPT2 use doc->m_dlist (?)

	cvertex2(cconnect2 *c, int _x=0, int _y=0);				// CPT2 Added args. Done in cpp
	~cvertex2()
		{ }

	bool IsValid();											// Done in cpp
	bool IsVertex() { return true; }
	bool IsVia();											// Done in cpp
	bool IsSlaveVtx() { return tee!=NULL; }
	bool IsPinVtx() { return pin!=NULL; }
	bool IsEndVtx() { return (!preSeg || !postSeg) && !pin && !tee; }
	bool IsTraceVtx() { return preSeg && postSeg; }
	cvertex2 *ToVertex() { return this; }
	int GetTypeBit()
	{ 
		if (IsVia()) return bitVia;
		if (pin) return bitPinVtx;
		if (tee) return bitTeeVtx;							// NB tee-vertices will typically NOT be selectable, but tees will.
		return bitTraceVtx;
	}
	cnet2 *GetNet() { return m_net; }
	cconnect2 *GetConnect() { return m_con; }
	int GetLayer();											// Done in cpp
	void GetStatusStr( CString * str );						// Done in cpp, derived from old cvertex func.
	void GetTypeStatusStr( CString * str );					// CPT2

	int Draw();												// Done in cpp
	void Undraw();											// Done in cpp
	void Highlight();										// Done in cpp, derived from cvertex::Highlight
	void SetVisible( bool bVis );							// Done in cpp, derived from CNetList::SetViaVisible

	bool IsLooseEnd();										// Done in cpp
	bool Remove();											// Done in cpp
	void SetConnect(cconnect2 *c);							// Done in cpp
	void ForceVia();										// Done in cpp
	void UnforceVia();										// Done in cpp
	bool IsViaNeeded();										// Done in cpp
	void SetViaWidth();										// Done in cpp
	void ReconcileVia();									// Done in cpp
	bool TestHit(int x, int y, int layer);					// New, done in cpp.  (Frequently supplants CNetList::TestHitOnVertex())
	bool IsConnectedToArea( carea2 * a );
	int GetConnectedAreas( carray<carea2> *a=NULL );		// CPT2. Arg change
	cconnect2 * SplitConnect();								// Done in cpp, derived from cnet::SplitConnectAtVertex
	cseg2 * AddRatlineToPin( cpin2 *pin );					// Done in cpp
	void StartDraggingStub( CDC * pDC, int x, int y, int layer1, int w,	
							int layer_no_via, int crosshair, int inflection_mode );		// Done in cpp

	void StartDragging( CDC * pDC, int x, int y, int crosshair = 1 );			// Done in cpp, derived from CNetList::StartDraggingVertex
	void CancelDragging();														// Done in cpp, derived from CNetList::CancelDraggingVertex
	void Move( int x, int y );													// Done in cpp, derived from CNetList::MoveVertex
	int GetViaConnectionStatus( int layer );									// Done in cpp
	bool SetNeedsThermal();														// Done in cpp.  New.  Sets the bNeedsThermal flag, depending on net areas
																				// that overlap this point.
};

class ctee: public cpcb_item 
{
	// CPT2 new.  Basically a carray<cvertex2>, indicating the vertices that together join up to form a single tee junction.
public:
	carray<cvertex2> vtxs;
	int via_w, via_hole_w;					// These will be the max of the values in the constituent vtxs.
	CArray<dl_element*> dl_els;				// Tee will be drawn if it's a via.
	dl_element * dl_hole;					// 
	dl_element * dl_thermal;				// CPT2 new.

	ctee(cnet2 *n);							// Done in cpp
	~ctee() 
	{
		// References to the tee from its constituent vertices must be erased.
		citer<cvertex2> iv (&vtxs);
		for (cvertex2 *v = iv.First(); v; v = iv.Next())
			v->tee = NULL;
		vtxs.RemoveAll();
	}

	bool IsValid() { return vtxs.GetSize()>0; }			// TODO Adequate?
	bool IsTee() { return true; }
	ctee *ToTee() { return this; }
	bool IsVia() { return via_w > 0; }
	int GetTypeBit() 
		{ return via_w? bitVia: bitTee; }
	cnet2 *GetNet() 
		{ return vtxs.GetSize()==0? NULL: vtxs.First()->m_net; }
	int GetLayer();													// Done in cpp
	void GetStatusStr( CString * str );								// Done in cpp

	int Draw();											// Done in cpp
	void Undraw();										// Done in cpp
	void Highlight();									// Done in cpp
	void SetVisible( bool bVis );						// Done in cpp, derived from cvertex2::SetVisible

	void Remove(bool bRemoveVtxs = false);								// Done in cpp
	void Add(cvertex2 *v);												// Done in cpp
	bool Adjust();														// Done in cpp
	void ForceVia();													// Done in cpp
	void UnforceVia();													// Done in cpp
	bool IsViaNeeded();													// Done in cpp
	void ReconcileVia();												// Done in cpp
	void Remove(cvertex2 *v, bool fAdjust = true);						// Done in cpp
	void Move(int x, int y);											// Done in cpp
	void StartDragging( CDC * pDC, int x, int y, int crosshair );		// Done in cpp
	void CancelDragging();												// Done in cpp
};


// cseg2: describes a segment of a connection 
class cseg2: public cpcb_item
{
public:
	// static int m_array_step;
	int m_layer;
	int m_width;
	int m_curve;
	bool m_selected;			// CPT2 TODO check (bogus?)
	// CDisplayList * m_dlist;	// CPT2 use doc->m_dlist (?)
	cnet2 * m_net;				// parent net
	cconnect2 * m_con;			// parent connection
	cvertex2 *preVtx, *postVtx;	// CPT2

	enum Curve { STRAIGHT = 0,
			CCW,
			CW
	};

	cseg2(cconnect2 *c, int _layer, int _width);							// CPT2 added args.  Replaces Initialize().  Done in cpp
	~cseg2() 
	{
		// Destructor is not responsible for unhooking seg from preVtx and postVtx.
		Undraw();
	}

	bool IsValid();												// Done in cpp
	bool IsSeg() { return true; }
	cseg2 *ToSeg() { return this; }
	int GetTypeBit() { return bitSeg; }
	cnet2 *GetNet() { return m_net; }
	cconnect2 *GetConnect() { return m_con; }
	int GetLayer() { return m_layer; }

	void SetConnect(cconnect2 *c);
	void SetWidth( int w, int via_w, int via_hole_w );			// Done in cpp
	int SetLayer( int _layer );									// Done in cpp, derived from CNetList::ChangeSegmentLayer

	int Draw();													// Done in cpp
	int DrawWithEndpoints();									// CPT2 new, done in cpp
	void Undraw();												// Done in cpp
	void Highlight( bool bThin );								// Done in cpp (derived from CNetList::HighlightSegment)
	void Highlight()											// CPT2 This form of the function overrides the base-class virtual func.  (Best system?)
		{ Highlight(false); }

	void SetVisible( bool bVis );										// Done in cpp
	int Index();													// CPT2:  maybe, maybe not...
	void GetStatusStr( CString * str, int width );					// CPT added width param
	void GetStatusStr( CString *str ) { GetStatusStr(str, 0); }
	void Divide( cvertex2 *v, cseg2 *s, int dir );					// Done in cpp
	bool InsertSegment(int x, int y, int layer, int width, int dir );  // Done in cpp, derived from CNetList::InsertSegment()
	int Route( int layer, int width );								// Done in cpp
	void Unroute(int dx=1, int dy=1, int end=0);					// Done in cpp
	bool UnrouteWithoutMerge(int dx, int dy, int end);				// Done in cpp
	bool RemoveMerge(int end);										// Done in cpp
	bool RemoveBreak();												// Done in cpp

	void StartDragging( CDC * pDC, int x, int y, int layer1, int layer2, int w,			// Done in cpp, derived from CNetList::StartDraggingSegment
						int layer_no_via, int dir, int crosshair = 1 );
	void CancelDragging();																// Done in cpp, derived from CNetList::CancelDraggingSegment
	void StartDraggingNewVertex( CDC * pDC, int x, int y, int layer, int w, int crosshair );// Done in cpp, derived from CNetList::StartDraggingSegmentNewVertex()
	void CancelDraggingNewVertex( );														// Done in cpp, derived from CNetList::CancelDraggingSegmentNewVertex()
	void StartMoving( CDC * pDC, int x, int y, int crosshair );								// Done in cpp, derived from CNetList::StartMovingSegment
	void CancelMoving();																	// Done in cpp, derived from CNetList::CancelMovingSegment()
};

// cconnect2: describes a sequence of segments, running between two end vertices of arbitrary type (pin, tee, isolated end-vertex...)
class cconnect2: public cpcb_item
{
	// friend class CNetList;
	// friend class cnet2;
public:
	enum Dir {
		ROUTE_FORWARD = 0,
		ROUTE_BACKWARD
	};

	cnet2 * m_net;				// parent net
	carray<cseg2> segs;			// array of segments. NB not necessarily in geometrical order
	carray<cvertex2> vtxs;		// array of vertices. NB ditto
	cvertex2 *head, *tail;		// CPT2 for geometrical traversing.	

	bool locked;				// true if locked (will not be optimized away)
	// CDisplayList * m_dlist;	// CPT2 use doc->m_dlist (?)

	// these params used only by DRC
	int min_x, max_x;			// bounding rect
	int min_y, max_y;
	BOOL vias_present;			// flag to indicate that vias are present. 
	int seg_layers;				// mask for all layers used by segments.  CPT2 MUST STUDY

// methods
public:
	// general
	cconnect2( cnet2 * _net );	// Done in cpp
	~cconnect2()
	{
		// Destructor does not delete constituent segs and vtxs.  If you want to get rid of all those, call DestroyAll() before deleting.
		Undraw();
		// If constituent segs and vtxs point back to this, change their ptrs to NULL
		citer<cseg2> is (&segs);
		for (cseg2 *s = is.First(); s; s = is.Next())
			if (s->m_con == this) s->m_con = NULL;
		citer<cvertex2> iv (&vtxs);
		for (cvertex2 *v = iv.First(); v; v = iv.Next())
			if (v->m_con == this) v->m_con = NULL;
	}

	bool IsValid();
	bool IsConnect() { return true; }
	cconnect2 *ToConnect() { return this; }
	int GetTypeBit() { return bitConnect; }			// Rarely used since connects don't have selector elements.
	cnet2 *GetNet() { return m_net; }
	cconnect2 *GetConnect() { return this; }

	// void ClearArrays();							// CPT2 unused
	void GetStatusStr( CString * str );

	// get info about start/ending pins
	cpin2 * StartPin() { return head->pin; }
	cpin2 * EndPin() { return tail->pin; }
	BOOL TestHitOnEndPad( int x, int y, int layer, int dir );	// CPT2 TODO:  adapt from old CNetList::TestHitOnConnectionEndPad

	// get info about vertices
	int NumVtxs() { return vtxs.GetSize(); }
	// cvertex2 * FirstVtx();
	// cvertex2 * LastVtx();
	// cvertex2 * VtxByUID( int uid, int * index=NULL );        // Use vtxs.FindByUID() instead
	cvertex2 *VtxByIndex( int iv );								// CPT2: maybe, maybe not...  changed from cvertex2& ret-val to cvertex2*
	int VtxIndexByPtr( cvertex2 * v );							// CPT2: maybe, maybe not...

	// get info about segments
	int NumSegs() { return segs.GetSize(); }
	// void SetNumSegs( int n );
	cseg2 *FirstSeg() { return head->postSeg; }
	cseg2 *LastSeg() { return tail->preSeg; }
	// cseg2 *SegByUID( int uid, int * index=NULL );				// Use segs.FindByUID().
	cseg2 *SegByIndex( int is );									// CPT2: maybe, maybe not....
	int SegIndexByPtr( cseg2 * s );									// CPT2: ditto
	// int SegUIDByPtr( cseg2 * s );

	void SetWidth( int w, int via_w, int via_hole_w );				// Done in cpp

	// modify segments and vertices. 
	void Start( cvertex2 *v );										// CPT2 New.  Done in cpp.
	void Remove();													// Done in cpp
	// cvertex2& InsertVertexByIndex( int iv, const cvertex2& new_vtx );		// CPT2.  Currently called only with iv==0.  Not really compatible with
																				// the new system, hopefully dumpable
	// void InsertSegAndVtxByIndex( int is, int dir, const cseg2& new_seg, const cvertex2& new_vtx );	// CPT2.  Replace by:
	void AppendSegAndVertex( cseg2 *s, cvertex2 *v, cvertex2 *after) ;			// Done in cpp
	// void AppendSegAndVertex( const cseg2& new_seg, const cvertex2& new_vtx );  // CPT2.  Use AppendSegAndVertex( new_seg, new_vtx, tail)
	// void PrependVertex( const cvertex2& new_vtx );							// CPT2.  Probably able to live without.  
	void PrependVertexAndSeg( cvertex2 *v, cseg2 *s );					// Done in cpp
	void AppendSegment( int x, int y, int layer, int width );           // Done in cpp
	void PrependSegment( int x, int y, int layer, int width );			// Done in cpp

	void ReverseDirection();											// Done in cpp
	void CombineWith( cconnect2 *c2, cvertex2 *v1, cvertex2 *v2) ;		// Done in cpp
	void MergeUnroutedSegments();										// Done in cpp
	void ChangePin( int end_flag, cpin2 *newPin );						// CPT2 TODO adapt from CNetList::ChangeConnectionPin()

	// drawing methods
	int Draw();															// Done in cpp
	void Undraw();														// Done in cpp
	void Highlight();													// Done in cpp, derived from CNetList::HighlightConnection()
};


/**********************************************************************************************/
/*  RELATED TO ctext                                                                     */
/**********************************************************************************************/

class ctext: public cpcb_item
{
public:
	int m_x, m_y;
	int m_layer;
	int m_angle;
	bool m_bMirror;
	bool m_bNegative;
	int m_font_size;
	int m_stroke_width;
	// int m_nchars;					// CPT2 Ditched
	CString m_str;
	CArray<stroke> m_stroke;
	CRect m_br;							// CPT2 added.  Bounding rectangle
	SMFontUtil * m_smfontutil;
	bool m_bShown;						// CPT2 added.  Normally true, but may be set false for reftext's and valuetext's that aren't visible

	ctext( CFreePcbDoc *_doc, int _x, int _y, int _angle, 
		BOOL _bMirror, BOOL _bNegative, int _layer, int _font_size, 
		int _stroke_width, SMFontUtil *_smfontutil, CString * _str );
	~ctext() { }

	bool IsValid();																// Done in cpp
	bool IsText() { return true; }
	ctext *ToText() { return this; }
	int GetTypeBit() { return bitText; }
	int GetLayer() { return m_layer; }
	virtual cpart2 *GetPart() { return NULL; }

	void Init( CDisplayList * dlist, int x, int y, int angle,					// TODO: rethink relationship with constructor. Removed tid arg.
		int mirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr );
	void Copy(ctext *other);													// CPT2 new, done in cpp.
	void Move( int x, int y, int angle, BOOL mirror, BOOL negative, int layer, int size=-1, int w=-1 );		// Done in cpp
	void Move( int x, int y, int angle, int size=-1, int w=-1);										// Done in cpp
	void Resize( int size, int w );																	// Done in cpp
	void MoveRelative( int _x, int _y, int _angle=-1, int _size=-1, int _w=-1 );					// Used for reftexts and valuetexts. Done in cpp
	CPoint GetAbsolutePoint();																		// Done in cpp.  Gets absolute position for reftexts and valuetexts.
	CRect GetRect();																				// Done in cpp

	// void Draw( CDisplayList * dlist, SMFontUtil * smfontutil );				// CPT2.  Probably dispensible
	void GenerateStrokes();														// CPT2 new.  Helper for Draw().
	int Draw();																	// Done in cpp
	void GenerateStrokesRelativeTo( cpart2 *p );								// CPT2 new.  Helper for DrawRelativeTo().
	int DrawRelativeTo( cpart2 *p, bool bSelector=true );						// CPT2 new.  Done in cpp
	void Undraw();																// Done in cpp
	void Highlight();															// Done in cpp
	void SetVisible(bool bVis);													// Done in cpp
	void StartDragging( CDC * pDC );											// Done in cpp, derived from CTextList::StartDraggingText
	void CancelDragging();														// Done in cpp, derived from CTextList::CancelDraggingText
	// void GetBounds( CRect &br );												// CPT2.  Use new m_br
};

class creftext: public ctext
{
public:
	cpart2 *part;																// Which part contains this reftext?

	creftext( cpart2 *_part, int x, int y, int angle, 
		BOOL bMirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown );			// Done in cpp
	creftext( CFreePcbDoc *doc, int x, int y, int angle, 
		BOOL bMirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown );			// Done in cpp
	bool IsValid();																				// Done in cpp
	bool IsRefText() { return true; }
	creftext *ToRefText() { return this; }
	int GetTypeBit() { return bitRefText; }
	cpart2 *GetPart() { return part; }

	int Draw() { return DrawRelativeTo(part); }
};

class cvaluetext: public ctext
{
public:
	cpart2 *part;

	cvaluetext( cpart2 *_part, int x, int y, int angle, 
		BOOL bMirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown );			// Done in cpp
	cvaluetext( CFreePcbDoc *doc, int x, int y, int angle, 
		BOOL bMirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown );
	bool IsValid();																				// Done in cpp
	bool IsValueText() { return true; }
	cvaluetext *ToValueText() { return this; }
	int GetTypeBit() { return bitValueText; }
	cpart2 *GetPart() { return part; }

	int Draw() { return DrawRelativeTo(part); }
};



/**********************************************************************************************/
/*  RELATED TO cpin2/cpart2                                                                     */
/**********************************************************************************************/

// cpin2: describes a pin in a net.  Folded the old part_pin into this.
class cpin2 : public cpcb_item
{
public:
	// CString ref_des;		// reference designator such as 'U1'.  CPT2 READ OFF of part->ref_des
	CString pin_name;		// pin name such as "1" or "A23"
	cpart2 * part;			// pointer to part containing the pin.
	int x, y;				// position on PCB
	cpadstack *ps;			// CPT2. Seems useful to have this here.  Constructor sets it up
	int pad_layer;			// CPT2. layer of pad on this pin (was in cvertex2).  
							// Constructor sets this based on ps. Possible values LAY_PAD_THRU, LAY_TOP_COPPER, LAY_BOTTOM_COPPER
	cnet2 * net;			// pointer to net, or NULL if not assigned.
	bool bNeedsThermal;		// CPT2 new.  Set to true if there's an areas from the same network that occupies this same point.
	// drc_pin drc;			// drc info.  CPT2 TODO.  Was causing problems with the #$*& preprocessor, put the issue off for another day...
	dl_element * dl_hole;	// pointer to graphic element for hole
	CArray<dl_element*> dl_els;	// array of pointers to graphic elements for pads
	dl_element *dl_thermal; // CPT2 new.  The thermal drawn with this pin.

	cpin2(cpart2 *_part, cpadstack *_ps, cnet2 *_net);					// CPT2. Added args. Done in cpp
	~cpin2();

	bool IsValid();													// Done in cpp
	bool IsPin() { return true; }
	cpin2 *ToPin() { return this; }
	int GetTypeBit() { return bitPin; }
	cnet2 *GetNet() { return net; }
	int GetLayer() { return pad_layer; }
	int GetWidth();													// Done in cpp
	void GetVtxs(carray<cvertex2> *vtxs);							// Done in cpp, CPT2 new.

	void Highlight();												// Done in cpp, derived from old CPartList::HighlightPad

	bool TestHit(int x, int y, int layer);							// Done in cpp, derived from CPartList::TestHitOnPad
	void SetPosition();												// Done in cpp.  New, but related to CPartList::GetPinPoint
	bool SetNeedsThermal();											// Done in cpp.  New, but related to CNetList::SetAreaConnections
	// void SetThermalVisible(int layer, bool bVisible) { }			// CPT2.  TODO figure this out.
	// void Initialize( cnet2 * net );  // Put in constructor
	void Disconnect() ;												// Done in cpp
	cseg2 *AddRatlineToPin( cpin2 *p2 );
};


// class cpart2 represents a part
class cpart2: public cpcb_item
{
public:
	// The partlist will be a carray, so no links required:
	// cpart2 * prev;		// link backward
	// cpart2 * next;		// link forward
	cpartlist * m_pl;	// parent partlist
	carray<cpin2> pins;	// array of all pins in part
	BOOL visible;		// 0 to hide part                                ?? TODO Put into base class?
	int x,y;			// position of part origin on board
	int side;			// 0=top, 1=bottom
	int angle;			// orientation, degrees CW
	BOOL glued;

	CString ref_des;	// ref designator such as "U3"
	// BOOL m_ref_vis;	// CPT2 use m_ref->bShown
	// int m_ref_xi;	// CPT2 Replaced by m_ref members..
	// int m_ref_yi;	
	// int m_ref_angle; 
	// int m_ref_size;
	// int m_ref_w;
	// int m_ref_layer;	// layer if part is on top	
	creftext *m_ref;	// CPT2 added. It's convenient to have an object for the ref-text on which we can invoke ctext methods.
						// Note that m_ref.m_x, m_ref.m_y, etc. will be _relative_ parameters
	CString value_text;
	// BOOL m_value_vis; // CPT2 use m_value->bShown
	// int m_value_xi;	// CPT2 Replaced by m_value members
	// int m_value_yi; 
	// int m_value_angle; 
	// int m_value_size; 
	// int m_value_w;
	// int m_value_layer;	// layer if part is on top
	cvaluetext *m_value;	// CPT2 added.  Analogous to m_ref
	ctextlist *m_tl;		// CPT2 added.  Objects for each of the texts derived from the footprint.

	// In base: dl_element * dl_sel;		// pointer to display list element for selection rect
	//dl_element * dl_ref_sel;				// pointer to selection rect for ref text // CPT2 use m_ref->dl_sel
	//dl_element * dl_value_sel;			// pointer to selection rect for value // CPT2 use m_value->dl_sel
	CString package;						// package (from original imported netlist, may be "")
	class CShape * shape;					// pointer to the footprint of the part, may be NULL
	// CArray<stroke> ref_text_stroke;		// strokes for ref text.  Use m_ref->m_stroke
	// CArray<stroke> value_stroke;			// strokes for value text.  Use m_value.m_stroke
	CArray<stroke> m_outline_stroke;		// array of outline strokes

	// drc info
	BOOL hole_flag;	// TRUE if holes present
	int min_x;		// bounding rect of pads
	int max_x;
	int min_y;
	int max_y;
	int max_r;		// max. radius of pads
	int layers;		// bit mask for layers with pads
	// flag used for importing
	BOOL bPreserve;	// preserve connections to this part

	cpart2( cpartlist * pl );										// Done in cpp.
	~cpart2()
		{ delete m_ref; delete m_value; delete m_tl; }

	bool IsValid();													// Done in cpp
	bool IsPart() { return true; }
	cpart2 *ToPart() { return this; }
	int GetTypeBit() { return bitPart; }
	void Remove(bool bEraseTraces, bool bErasePart=true);			// Done in cpp.

	void Move( int x, int y, int angle, int side );												// Done in cpp, derived from CPartList::Move
	void PartMoved( int dx, int dy );															// Done in cpp, derived from CNetList::PartMoved
	void SetData( CShape * shape, CString * ref_des, CString *value_txt, CString * package, 
	     		  int x, int y, int side, int angle, int visible, int glued );					// Done in cpp, Derived from CPartList::SetPartData
	void SetValue( CString * value, int x, int y, int angle, int size, 
				  int w, BOOL vis, int layer );													// Done in cpp, Derived from CPartList::SetValue
	// void MoveRefText( int _x, int _y, int _angle=-1, int _size=-1, int _w=-1 );					// Done in cpp, derived from CPartList::MoveRefText
	void ResizeRefText(int size, int width, BOOL vis );											// Done in cpp, derived from CPartList::ResizeRefText()
	// void MoveValueText( int _x, int _y, int _angle=-1, int _size=-1, int _w=-1 );				// Done in cpp, derived from CPartList func
	void InitPins();																			// Done in cpp. Basically all new

	// cpin2 * PinByUID( int uid );		// CPT2. Use pins.FindByUID().
	// int GetNumRefStrokes();			// CPT2. Use m_ref.m_stroke.GetSize()
	// int GetNumValueStrokes();		// CPT2. Use m_value.m_stroke.GetSize()
	int GetNumOutlineStrokes();
	void OptimizeConnections( BOOL bBelowPinCount, int pin_count, BOOL bVisibleNetsOnly=TRUE );	// Done in cpp, derived from old CNetList function
	cpin2 *GetPinByName(CString *name);	// Done in cpp, new
	CPoint GetCentroidPoint();			// CPT2 TODO derive from CPartList::GetCentroidPoint
	CPoint GetGluePoint( int iglue );	// CPT2 TODO derive from CPartList::GetGluePoint()
	// void SetAreaConnections();		// CPT2 PROBABLY SUPERSEDED
	// CPoint GetRefPoint();				// Done in cpp.  Derived from CPartList func.
	// CPoint GetValuePoint();				// Done in cpp.  Derived from CPartList func.
	// CRect GetValueRect();				// Done in cpp.  Derived from CPartList func.
	int GetBoundingRect( CRect * part_r );			// Done in cpp. Derived from CPartList::GetPartBoundingRect()
	void ChangeFootprint( CShape * shape );		// Done in cpp, loosely derived from CPartList::PartFootprintChanged() & CNetList::PartFootprintChanged()

	int Draw();							// Done in cpp
	void Undraw();						// Done in cpp
	void Highlight();					// Done in cpp, derived from CPartList::HighlightPart()
	void SetVisible(bool bVis);         // CPT2 TODO derive from CPartList::MakePartVisible()
	void StartDragging( CDC * pDC, BOOL bRatlines, BOOL bBelowPinCount, int pin_count );		// Done in cpp, derived from CPartList::StartDraggingPart
	void CancelDragging();

	void MakeString( CString *str );	// Done in cpp, derived from old CPartList::SetPartString().
};


/**********************************************************************************************/
/*  RELATED TO cpolyline/carea2/csmcutout                                                      */
/**********************************************************************************************/

class ccorner: public cpcb_item
{
public:
	int x, y;
	ccontour *contour;				// CPT2.
	// dl_element * dl_corner_sel;  // Use cpcb_item::dl_sel
	cside *preSide, *postSide;		// CPT2

	ccorner(ccontour *_contour, int _x, int _y);		// Done in cpp
	~ccorner();

	bool IsValid();										// Done in cpp
	bool IsCorner() { return true; }
	bool IsAreaCorner();								// Done in cpp
	bool IsBoardCorner();								// Done in cpp
	bool IsSmCorner(); 									// Done in cpp
	bool IsOutlineCorner(); 							// Done in cpp
	ccorner *ToCorner() { return this; }
	int GetTypeBit();									// Done in cpp
	cnet2 *GetNet();									// Done in cpp
	int GetLayer();										// Done in cpp
	cpolyline *GetPolyline();							// Done in cpp

	bool IsOnCutout();									// Done in cpp
	void Highlight();									// Done in cpp, derived from old CPolyLine::HighlightCorner().  See also CNetList::HighlightAreaCorner.
	bool Move( int x, int y, BOOL bEnforceCircularArcs=FALSE );			// Done in cpp, derived from CNetList::MoveAreaCorner/CPolyLine::MoveCorner
	void StartDragging( CDC * pDC, int x, int y, int crosshair = 1 );	// Done in cpp, derived from CPolyLine::StartDraggingToMoveCorner
	void CancelDragging();												// Done in cpp, derived from CPolyLine::CancelDraggingToMoveCorner
	void Remove();
};

class cside: public cpcb_item
{
public:
	int m_style;					// TODO change to style?
	ccontour *contour;				// CPT2
	// dl_element * dl_side;		// Use base dl_el
	// dl_element * dl_side_sel;	// Use base dl_sel
	ccorner *preCorner, *postCorner;

	cside(ccontour *_contour, int _style);
	~cside();

	bool IsValid();
	bool IsSide() { return true; }
	bool IsAreaSide();								// Done in cpp
	bool IsBoardSide();								// Done in cpp
	bool IsSmSide(); 								// Done in cpp
	bool IsOutlineSide(); 							// Done in cpp
	cside *ToSide() { return this; }
	int GetTypeBit();								// Done in cpp
	cnet2 *GetNet();								// Done in cpp
	int GetLayer();									// Done in cpp
	cpolyline *GetPolyline();						// Done in cpp
	bool IsOnCutout();								// Done in cpp

	void InsertCorner(int x, int y);				// Done in cpp
	bool Remove( carray<cpolyline> *arr );			// CPT2 new, done in cpp
	void Highlight();		// Done in cpp, derived from cpolyline::HighlightSide()
	void SetVisible();		// CPT2
	void SetStyle();
	void StartDraggingNewCorner( CDC * pDC, int x, int y, int crosshair );  // Done in cpp, derived from CPolyLine::StartDraggingToInsertCorner
	void CancelDraggingNewCorner( );										// Done in cpp, derived from CPolyLine::CancelDraggingToInsertCorner
};

class ccontour: public cpcb_item
{
public:
	// All CPT2.
	carray<ccorner> corners;	// Array of corners.  NB entries not necessarily in geometrical order
	carray<cside> sides;		// Array of sides
	ccorner *head, *tail;		// For when we need to trace thru geometrically.
	cpolyline *poly;			// The parent polyline

	ccontour(cpolyline *_poly, bool bMain);									// Done in cpp
	ccontour(cpolyline *_poly, ccontour *src);								// Copy constructor, done in cpp
	~ccontour();

	bool IsValid();															// Done in cpp
	bool IsContour() { return true; }
	ccontour *ToContour() { return this; }
	int GetTypeBit() { return bitContour; }
	int GetLayer();															// Done in cpp
	cnet2 *GetNet();														// Done in cpp
	cpolyline *GetPolyline() { return poly; }

	void Highlight()														// CPT2 TODO used?
	{
		citer<cside> is (&sides);
		for (cside *s = is.First(); s; s = is.Next())
			s->Highlight();
	}
	
	int NumCorners() { return corners.GetSize(); }
	void AppendSideAndCorner( cside *s, ccorner *c, ccorner *after );		// Done in cpp
	void AppendCorner(int x, int y, int style = STRAIGHT);					// Done in cpp
	void Close(int style = STRAIGHT);										// Done in cpp
	void Unclose();															// Done in cpp
	CRect GetCornerBounds();												// Done in cpp
	BOOL TestPointInside( int x, int y );									// CPT2 Needed?
	void SetPoly( cpolyline *_poly );										// Done in cpp
	void Remove();															// Done in cpp, derived from CPolyLine::RemoveContour
};

class cpolyline: public cpcb_item
{
public:
	// CDisplayList * m_dlist;		// display list // CPT2 use doc->m_dlist (?)

	cpcb_item *parent;			// CPT2.  TODO Probably obsolete...
	ccontour *main;				// CPT2
	carray<ccontour> contours;	// CPT2.  All contours, including main
	// id m_parent_id;			// id of the parent of the polyline
	// void * m_ptr;			// pointer to parent object (or NULL)
	int m_layer;				// layer to draw on
	int m_w;					// line width
	int m_sel_box;				// corner selection box width/2
	int m_hatch;				// hatch style, see enum above
	int m_nhatch;				// number of hatch lines
	bool m_bTemporary;			// CPT2 Set for CFreePcbView::m_tmp_poly.
	CArray <dl_element*>  dl_hatch;	// hatch lines.  Use CArray with dl-elements generally?
	int utility2;
	gpc_polygon * m_gpc_poly;	// polygon in gpc format
	polygon * m_php_poly;

public:
	// constructors/destructor
	cpolyline(CFreePcbDoc *_doc);													// Done in cpp
	cpolyline(cpolyline *src, bool bCopyContours=true, bool bTemporary=false);		// Done in cpp
	~cpolyline()
		{ }

	bool IsPolyline() { return true; }
	cpolyline *ToPolyline() { return this; }
	int GetLayer() { return m_layer; }
	cpolyline *GetPolyline() { return this; }
	bool IsValid() { return m_bTemporary; }											// We want CFreePcbDoc::Redraw always to render temporary polylines.

	// functions for creating and modifying polyline
	virtual void Remove() { }														// CPT2 new.  Check out the versions in derived classes.
	void MarkConstituents();														// CPT2. Done in cpp.
	// void Clear();																// CPT2 TODO maybe obsolete?
	// void Start( int layer, int w, int sel_box, int x, int y,
	//	int hatch, id * id, void * ptr, BOOL bDraw=TRUE );							// CPT2 TODO maybe obsolete?
	// void AppendCorner( int x, int y, int style = STRAIGHT );							// CPT2 TODO Probably move into ccontour.
	// void InsertCorner( ccorner *c, int x, int y, int style = STRAIGHT );				// CPT2 TODO Probably move into ccontour.  Insert prior to "c"
	// void DeleteCorner( ccorner *c );													// Moved into ccorner
	// BOOL MoveCorner( ccorner *c, int x, int y, BOOL bEnforceCircularArcs=FALSE );	// Moved into ccorner
	// void Close( int style = STRAIGHT );													// CPT2 TODO maybe obsolete?

	// void RemoveContour( ccontour *ctr );												// Moved into ccontour
	// drawing functions
	// void HighlightSide( int is );													// Change to cside::Highlight();
	// void HighlightCorner( int ic );													// sim.
	// void StartDraggingToInsertCorner( CDC * pDC, ccorner *c, int x, int y, int crosshair = 1 ); // Arg change
	// void StartDraggingToMoveCorner( CDC * pDC, ccorner *c, int x, int y, int crosshair = 1 );	// CPT2 Moved to ccorner
	// void CancelDraggingToInsertCorner( ccorner *c );											// Arg change
	// void CancelDraggingToMoveCorner( ccorner *c );												// CPT2 Moved to ccorner
	int Draw();																					// Done in cpp.  Old arg removed.
	void Undraw();																				// Done in cpp
	void Hatch();																				// Done in cpp
	void Highlight();																			// Done in cpp
	void MakeVisible( BOOL visible = TRUE );
	// void MoveOrigin( int x_off, int y_off );		// Same as Offset
	// void SetSideVisible( int is, int visible );  // Use cside::SetVisible()

	// misc. functions
	CRect GetBounds();								// Done in cpp
	CRect GetCornerBounds();						// Done in cpp
	// CRect GetCornerBounds( int icont );			// Use ccontour::GetCornerBounds()
	bool TestPointInside( int x, int y );			// Done in cpp
	// void AppendArc( int xi, int yi, int xf, int yf, int xc, int yc, int num );	// CPT2. TODO Needed?

	// undo functions
	int SizeOfUndoRecord();
	void SetFromUndo( undo_poly * un_poly );
	void CreatePolyUndoRecord( undo_poly * un_poly );

	// access functions
	// id Id();
	// int UID();   in base class
	int NumCorners();														// Total corners in all contours.  Done in cpp.
	int NumSides();				
	void GetSidesInRect( CRect *r, carray<cpcb_item> *arr );				// CPT2 new, done in cpp.
	bool IsClosed()
		{ return main->head==main->tail; }
	int NumContours() { return contours.GetSize(); }
	// int Contour( int ic );				// Use ccorner::contour
	// int ContourStart( int icont );		// Similarly...
	// int ContourEnd( int icont );
	// int ContourSize( int icont );
	// int CornerIndexByUID( int uid );		// Unlikely to be useful
	// int SideIndexByUID( int uid );		// Ditto
	// int X( int ic );						// Use ccorner::x
	// int Y( int ic );
	// int EndContour( int ic );			// Use ctr->postSide==NULL
	// int Utility(){ return utility; }						// Just use the base class data member?
	// int Utility( int ic ){ return corner[ic].utility; };
	// int Layer() { return m_layer; }		// Renamed GetLayer()
	int W() { return m_w; }
	// int CornerUID( int ic ){ return corner[ic].m_uid; };
	// int SideUID( int is );
	// int SideStyle( int is );
	// void * Ptr(){ return m_ptr; };
	int SelBoxSize();
	int GetHatch() { return m_hatch; }

	gpc_polygon * GetGpcPoly() { return m_gpc_poly; }

	// void SetParentId( id * id );
	// void SetUID( int uid );					// Seems fishy...
	// void SetCornerUID( int ic, int uid );	// Put in ccorner or dump...
	// void SetSideUID( int is, int uid );
	// void SetClosed( BOOL bClosed );			// Use ccontour::Close() and ccontour::Unclose()
	// void SetX( int ic, int x );
	// void SetY( int ic, int y );
	// void SetEndContour( int ic, BOOL end_contour );
	// void SetUtility( int u ){ utility = u; };								// Am just making cpcb_item::utility public...
	// void SetUtility( int ic, int utility ){ corner[ic].utility = utility; }  // Use corner->utility = ...
	void SetLayer( int layer );
	void SetW( int w );
	// void SetSideStyle( int is, int style, BOOL bDraw=TRUE );
	void SetSelBoxSize( int sel_box ) { m_sel_box = sel_box; }
	void SetHatch( int hatch )
		{ Undraw(); m_hatch = hatch; Draw(); }
	bool SetClosed( bool bClosed );														// CPT2 descended from CPolyLine function
	// void SetDisplayList( CDisplayList * dl );
	void Offset(int dx, int dy);														// CPT2 TODO obsolete?

	// Functions for normalizing and combining intersecting polylines
	virtual void GetCompatiblePolylines( carray<cpolyline> *arr ) { }					// CPT2 new, done in cpp
	virtual cpolyline *CreateCompatible() { return NULL; }								// CPT2 new, done in cpp
	void MakeGpcPoly( ccontour *ctr = NULL, CArray<CArc> * arc_array=NULL );			// Done in cpp, derived from CPolyline function
	void FreeGpcPoly();																	// Done in cpp.
	void NormalizeWithGpc( bool bRetainArcs=false );									// Done in cpp.  First version was in class carea2, am now generalizing
	int TestPolygon();																	// Done in cpp, derived from CNetList::TestAreaPolygon()
	// BOOL TestPointInsideContour( int icont, int x, int y );							// Use ccontour fxn
	int TestIntersection( cpolyline * poly2, bool bTestArcIntersections=true );			// Done in cpp, derived from CNetList::TestAreaIntersection().
	bool TestIntersections();															// Done in cpp.  First version was in carea2, now generalizing;
																						//   covers the old CNetList::TestAreaIntersections().
	int CombinePolyline( cpolyline *poly2 );											// Done in cpp, derived from CNetList::CombineAreas
	void RestoreArcs( CArray<CArc> *arc_array, carray<cpolyline> *pa=NULL );			// Done in cpp.  Originally in CPolyLine
	int ClipPolygon( bool bMessageBoxArc, bool bMessageBoxInt, bool bRetainArcs=true);	// Done in cpp.  Generalization of old carea2 func.
	static void CombinePolylines( carray<cpolyline> *pa, BOOL bMessageBox, BOOL bUseUtility );
																						// Done in cpp, generalization of old CNetList::CombineAllAreasInNet()
	virtual bool PolygonModified( bool bMessageBoxArc, bool bMessageBoxInt );			// Done in cpp.  Generalization of old carea2 func.
																						// Virtual, because at least for now cboard is overriding it (to
																						// do nothing)
	// void ClipGpcPolygon( gpc_op op, cpolyline * poly );								// CPT2, apparently obsolete

	// PHP functions.  CPT2 TODO apparently obsolete.
	int MakePhpPoly();
	void FreePhpPoly();
	void ClipPhpPolygon( int php_op, cpolyline * poly );
};

// carea2: describes a copper area in a net.
class carea2 : public cpolyline
{
public:
	cnet2 * m_net;		// parent net
	// CPT2.  Try moving these into cvertex2 and cpin2.  
	// carray<cpin2> pins;	// CPT2: replaces npins, pin, PinByIndex() below.  Set up by SetConnections() below.
	// CArray<dl_element*> dl_thermal;		// graphics for thermals on pins.
	// CArray<dl_element*> dl_via_thermal; // graphics for thermals on stubs
	// carray<cvertex2> vtxs;				// CPT2: replaces the following three members
	// int nvias;						// number of via connections to area
	// carray<cconnect2> vcon;			// Changed from CArray<int>. 
	// carray<cvertex2> vtx;			// Ditto

	carea2(cnet2 *_net, int layer, int hatch, int w, int sel_box);	// Done in cpp
	~carea2();						

	bool IsValid();
	bool IsArea() { return true; }
	carea2 *ToArea() { return this; }
	int GetTypeBit() { return bitArea; }
	cnet2 *GetNet() { return m_net; }

	void GetCompatiblePolylines( carray<cpolyline> *arr );			// CPT2 new, done in cpp
	cpolyline *CreateCompatible();									// CPT2 new, done in cpp
	void Remove();													// Done in cpp
	// int Complete( int style );									// CPT2 TODO consider dumping
	void SetNet( cnet2 *net );										// Done in cpp
};


class csmcutout : public cpolyline
{
	// Represents solder-mask cutouts, which are always closed polylines
public:
	csmcutout(CFreePcbDoc *_doc, int layer, int hatch);									// Done in cpp
	~csmcutout() { }

	bool IsValid();																		// Done in cpp
	bool IsSmCutout() { return true; }
	csmcutout *ToSmCutout() { return this; }
	int GetTypeBit() { return bitSmCutout; }
	void Remove();																		// Done in cpp
	cpolyline *CreateCompatible();														// CPT2 new, done in cpp
	void GetCompatiblePolylines( carray<cpolyline> *arr );								// CPT2 new, done in cpp
};


class cboard : public cpolyline
{
public:
	// Represents board outlines.
	cboard(CFreePcbDoc *_doc);					// Done in cpp
	~cboard() { }

	bool IsValid();								// Done in cpp
	bool IsBoard() { return true; }
	cboard *ToBoard() { return this; }
	int GetTypeBit() { return bitBoard; }
	void Remove();								// Done in cpp
	void GetCompatiblePolylines( carray<cpolyline> *arr );								// Done in cpp.
	bool PolygonModified( bool bMessageBoxArc, bool bMessageBoxInt ) { return true; }	// Overriding virtual func in cpolyline.

};

class coutline : public cpolyline
{
public:
	// Represents outlines within footprints
	coutline(CFreePcbDoc *_doc, int layer, int w);		// Done in cpp
	coutline(coutline *src):
		cpolyline(src) { }
	~coutline() { }

	bool IsValid();										// Done in cpp
	bool IsOutline() { return true; }
	coutline *ToOutline() { return this; }
	int GetTypeBit() { return bitOutline; }
	cpolyline *CreateCompatible();						// CPT2 new, done in cpp

};

/**********************************************************************************************/
/*  RELATED TO cnet2                                                                           */
/**********************************************************************************************/

// cnet2: describes a net
class cnet2: public cpcb_item
{
	friend class CNetList;

public:
	carray<cconnect2> connects;	// array of pointers to connections.  Type change
	CString name;				// net name
	carray<cpin2> pins;			// array of pins
	carray<carea2> areas;		// array of copper areas
	carray<ctee> tees;			// Used when reading files (and the like), also by Draw().  Used to be in cnetlist
	int def_w;					// default trace width
	int def_via_w;				// default via width
	int def_via_hole_w;			// default via hole width
	bool bVisible;				// FALSE to hide ratlines and make unselectable.  CPT2 TODO put in base class?
	int utility2;				// used to keep track of which nets have been optimized
	// CDisplayList * m_dlist;		// CDisplayList to use	// CPT2 use doc->m_dlist (?)
	cnetlist * m_nlist;			// parent netlist

	cnet2( CFreePcbDoc *_doc, CString _name, int _def_w, int _def_via_w, int _def_via_hole_w );			// Done in cpp
	~cnet2()
	{
		citer<cpin2> ip (&pins);
		for (cpin2 *p = ip.First(); p; p = ip.Next())
			p->net = NULL;
	}

	bool IsValid();
	bool IsNet() { return true; }
	cnet2 *ToNet() { return this; }
	int GetTypeBit() { return bitNet; }
	void GetStatusStr( CString * str );

	int Draw();														// CPT2 new:  draws all connects and areas.  Done in cpp
	void Undraw();													// CPT2 new, analogous.  Done in cpp
	void Highlight(cpcb_item *exclude);								// Done in cpp
	void Highlight() { Highlight(NULL); }							// Also overrides base-class func.
	bool GetVisible() { return bVisible; }
	void SetVisible( bool _bVis );								// Done in cpp

	// pins
	int NumPins() { return pins.GetSize(); }
	// cpin2 * PinByIndex( int ip );
	// ? cpin2 * PinByUID( int uid, int * index );
	void SetVertexToPin( cvertex2 *v, cpin2 *p );					// Arg change
	int TestHitOnAnyPad( int x, int y, int layer );					// CPT2 TODO adapt from CNetList function
	cvertex2 *TestHitOnVertex(int layer, int x, int y);				// Done in cpp, derived from old CNetList function

	// connections
	int NumCons() { return connects.GetSize(); }
	// cconnect2 * ConByIndex( int ic );
	// cconnect2 * ConByUID( int uid, int * index=NULL );
	// int ConIndexByPtr( cconnect2 * c );

	// copper areas
	int NumAreas() { return areas.GetSize(); }
	// carea2 * AreaByUID( int uid, int * index=NULL );
	// carea2 * AreaByIndex( int ia );
	/* CPT2 phasing out in favor of SetThermals().
	void SetAreaConnections()
	{
		citer<carea2> ia (&areas);
		for (carea2* a = ia.First(); a; a = ia.Next())
			a->SetConnections();
	}
	*/
	int AddArea( int layer, int x, int y, int hatch, BOOL bDraw=TRUE );					// CPT2 TODO. Derive from CNetList::AddArea
	void DrawAreas();																	// Done in cpp, new
	void UndrawAreas();																	// Done in cpp, new

	// methods that edit objects
	// pins
	cpin2 *AddPin( CString * ref_des, CString * pin_name );										// Done in cpp.  Removed bSetAreas param
	void AddPin( cpin2 *pin );																	// Done in cpp
	void RemovePin( cpin2 *pin );																// CPT2
	// ? void RemovePinByUID( int uid, BOOL bSetAreas=TRUE );
	// void RemovePin( CString * ref_des, CString * pin_name, BOOL bSetAreas=TRUE );			// CPT2 Use above RemovePin, plus:
	// void RemovePin( int pin_index, BOOL bSetAreas=TRUE );

	void SetWidth( int w, int via_w, int via_hole_w )
	{
		citer<cconnect2> ic (&connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
			c->SetWidth(w, via_w, via_hole_w);
	}
	void GetWidth( int * w, int *via_w=NULL, int *via_hole_w=NULL);			// Done in cpp, derived from old CNetList::GetWidths()
	void CalcViaWidths(int w, int *via_w, int *via_hole_w);					// Done in cpp
	void SetThermals();														// Done in cpp

	// connections
	cconnect2 * AddConnect( int * ic=NULL );
	cconnect2 * AddConnectFromPin( int p1, int * ic=NULL );
	cconnect2 * AddConnectFromPinToPin( int p1, int p2, int * ic=NULL );
	// cconnect2 * AddConnectFromTraceVtx( id& vtx_id, int * ic=NULL );				// Just doing it by hand within CFreePcbView::OnVertexStartTrace
	// cseg2 * AddRatlineFromVtxToPin( cvertex2 *v, cpin2 *pin );					// Use cvertex2::AddRatlineToPin
	// void RemoveConnect( cconnect2 * c );											// CPT2 use cconnect::Remove().	Adjusting tees will always happen (why not?)
	// void RemoveConnectAdjustTees( cconnect2 * c );								// CPT2 use cconnect::Remove().
	// cconnect2 * SplitConnectAtVtx( id vtx_id );									// CPT2 use cvertex2::SplitConnect()
	void MergeConnections( cconnect2 * c1, cconnect2 * c2 );
	void RecreateConnectFromUndo( undo_con * con, undo_seg * seg, undo_vtx * vtx );
	// void AdjustTees( int tee_ID ); // CPT2 use ctee:Adjust().

	// bool RemoveSegmentAdjustTees( cseg2 * s );		// CPT2 use cseg2::RemoveBreak()
	// bool RemoveSegAndVertexByIndex( cseg2 *s );		// CPT2 use cseg2::RemoveMerge()
	// bool RemoveVertexAndSegByIndex( cseg2 *s );		// CPT2 use cseg2::RemoveMerge()
	// bool RemoveVertex( cconnect2 * c, int iv );		// CPT2 use cvertex2::Remove()
	// bool RemoveVertex( cvertex2 * v );				// Ditto
	// bool MergeUnroutedSegments( cconnect2 * c );		// CPT2 use cconnect::MergeUnroutedSegments()

	void CleanUpConnections( CString * logstr=NULL );				// CPT2 TODO adapt from CNetList::CleanUpConnections(cnet*,CString*)
	void OptimizeConnections(BOOL bBelowPinCount, int pin_count, BOOL bVisibleNetsOnly=TRUE );  // CPT2 TODO:  derive from old CNetList functions
	carea2 *NetAreaFromPoint( int x, int y, int layer )								// CPT2 replaces CNetList::TestPointInArea
	{
		citer<carea2> ia (&areas);
		for (carea2 *a = ia.First(); a; a = ia.Next())
			if ((a->GetLayer()==layer || layer==LAY_PAD_THRU) && a->TestPointInside(x, y))
				return a;
		return NULL;
	}

};


/**********************************************************************************************/
/*  OTHERS: cglue, ccentroid                                                       */
/**********************************************************************************************/

// centroid types
enum CENTROID_TYPE
{
	CENTROID_DEFAULT = 0,	// center of pads
	CENTROID_DEFINED		// defined by user
};

class ccentroid : public cpcb_item
{
public:
	// Represents centroids within footprints.
	CENTROID_TYPE m_type;
	int m_x, m_y;
	int m_angle;				// angle of centroid (CCW)

	ccentroid(CFreePcbDoc *_doc) 
		: cpcb_item (_doc)
		{ }
	~ccentroid() { }
	bool IsValid();
	bool IsCentroid() { return true; }
	ccentroid *ToCentroid() { return this; }
	int GetTypeBit() { return bitCentroid; }
	int Draw();
	void Undraw();
	void Highlight();
	void StartDragging( CDC *pDC );
	void CancelDragging();
};

// glue spot position types 
enum GLUE_POS_TYPE
{
	GLUE_POS_CENTROID,	// at centroid
	GLUE_POS_DEFINED	// defined by user
};


class cglue : public cpcb_item
{
public:
	// Represents adhesive dots within footprints.  Incorporates the old struct "glue"
	GLUE_POS_TYPE type;
	int w, x, y;

	cglue(CFreePcbDoc *_doc, GLUE_POS_TYPE _type, int _w, int _x, int _y) 
		: cpcb_item (_doc)
		{ type = _type; w = _w; x = _x; y = _y; }
	cglue(cglue *src)
		: cpcb_item (src->doc)
		{ type = src->type; w = src->w; x = src->x; y = src->y; }
	~cglue() { }

	bool IsValid();
	bool IsGlue() { return true; }
	cglue *ToGlue() { return this; }
	int GetTypeBit() { return bitGlue; }
	int Draw();
	void Undraw();
	void Highlight();
	void StartDragging( CDC *pDC );
	void CancelDragging();
};

class cdre : public cpcb_item
{
public:
	// Represents Design Rule Check error items.  Incorporates old class DRError
	enum {				
		// subtypes of errors 
		PAD_PAD = 1,				
		PAD_PADHOLE,		
		PADHOLE_PADHOLE,	
		SEG_PAD,			
		SEG_PADHOLE,	
		VIA_PAD,			
		VIA_PADHOLE,		
		VIAHOLE_PAD,		
		VIAHOLE_PADHOLE,	
		SEG_SEG,		
		SEG_VIA,			
		SEG_VIAHOLE,	
		VIA_VIA,			
		VIA_VIAHOLE,		
		VIAHOLE_VIAHOLE,	
		TRACE_WIDTH,	
		RING_PAD,			
		RING_VIA,
		BOARDEDGE_PAD,			
		BOARDEDGE_PADHOLE,	
		BOARDEDGE_VIA,			
		BOARDEDGE_VIAHOLE,		
		BOARDEDGE_SEG,	
		BOARDEDGE_COPPERAREA,	
		COPPERGRAPHIC_PAD,			
		COPPERGRAPHIC_PADHOLE,	
		COPPERGRAPHIC_VIA,			
		COPPERGRAPHIC_VIAHOLE,		
		COPPERGRAPHIC_SEG,	
		COPPERGRAPHIC_COPPERAREA,	
		COPPERGRAPHIC_BOARDEDGE,	
		COPPERAREA_COPPERAREA,
		COPPERAREA_INSIDE_COPPERAREA,
		COPPERAREA_BROKEN,					// CPT
		UNROUTED
	};

	int layer;					// layer (if pad error)
	int type;					// id, using subtypes above
	CString str;				// descriptive string
	CString name1, name2;		// names of nets or parts tested
	cpcb_item *item1, *item2;	// ids of items tested
	int x, y;					// position of error

	cdre(CFreePcbDoc *_doc) 
		: cpcb_item(_doc)
		{ }
	~cdre() { }
	bool IsDRE() { return true; }
	cdre *ToDRE() { return this; }
	int GetTypeBit() { return bitDRE; }
};

