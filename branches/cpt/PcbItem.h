// PcbItem.h:  The base class for all the items that
//   appear on a PCB.  By having a single base class, cpcb_item, and using pointers to objects 
//   belonging to derived classes, it was possible to supplant the old system of id's.
// Also included are new template classes, carray<T> and citer<T>;  we can rely mostly on them
//   instead of CArray<T>/CArray<T*> and the various old CIter_XXX classes.  carray and citer have a number of
//   safety features, including: (1) if a member of a carray gets deleted, references to it 
//   in the carray will be removed automatically;  (2) if a member of a carray gets detached or
//   deleted in the midst of a loop, citer can handle the situation gracefully;  (3) if a carray as
//   a whole gets deleted in the midst of a loop, its members are detached and citer handles that too;  (4) removal of items 
//   from a carray, and detection of their presence in the carray, are quick.
// The descendant classes are declared in Part.h, Net.h, Polyline.h, Text.h, Shape.h

#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "DisplayList.h"
#include "UndoNew.h"
#include "DlgLog.h"
#include "gpc_232.h"

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
class ccorner;	
class cside;	
class ccontour;
class cpolyline;
class carea2;
class csmcutout;
class cboard;
class coutline;
class cnet2;
class ctext;	
class creftext;
class cvaluetext;
class ccentroid;
class cglue;
class cdre;
class cpadstack;

class undo_con;
class undo_seg;
class undo_vtx;
class undo_net;

class cnetlist;
class CFreePcbDoc;
class cpartlist;
class ctextlist;
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
	bitGlue =			0x8000000,		// Fp editor only...
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
	friend class cundo_record;

	carray_link *carray_list;	// List of carray's into which this item has been added
	int m_uid;
	static int next_uid;
	static cpcb_item **uid_hash;			// Hash table (with an incredibly simple hash function).  Used by FindByUid(), qv.
	static int uid_hash_sz;					// Size of uid_hash, initially 0x2000.  Must be a power of 2.
	cpcb_item *uid_hash_next;				// Linked lists for hash conflicts.
public:
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
	cpcb_item(CFreePcbDoc *_doc, int _uid);		// Done in cpp.  Used only during undo operations.
	~cpcb_item();								// Done in cpp.  When an item is destroyed, references to it are automatically
												// removed from any carrays to which it belongs.
public:
	int UID() { return m_uid; }
	static cpcb_item *FindByUid(int uid);		// Done in cpp.
	static int GetNextUid() { return next_uid; }
	bool IsHit(int x, int y);					// Done in cpp.
	bool IsDrawn() { return bDrawn; }
	void RemoveForUndo();						// Done in cpp.

	// Virtual functions:
	virtual	int Draw() { return NOERR; }
	virtual void Undraw();							// Default behavior:  done in cpp
	virtual void MustRedraw();						// CPT2 r313.  My latest-n-greatest new system for drawing/undrawing (see notes.txt). Done in cpp
													// Overridden in cpin2 [only?]
	virtual void Highlight() { }
	virtual bool IsValid() { return false; }		// Or make it pure virtual.
	virtual cundo_item *MakeUndoItem() { return NULL; }		
	virtual void SaveUndoInfo();							// Most derived classes inherit the base-class version of the func.

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
		// Add item to this carray.  But first check if the item already belongs to this, and return silently if so
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

	void TransferTo(carray<T> *arr)
	{
		// Move the contents of this into "arr", removing all from this carray afterwards.
		arr->RemoveAll();
		arr->Add(this);
		RemoveAll();
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




