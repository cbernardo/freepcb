// PcbItem.h:  The base class for all the items that
//   appear on a PCB.  By having a single base class, CPcbItem, and using pointers to objects 
//   belonging to derived classes, it was possible to supplant the old system of id's.
// Also included are new template classes, CHeap<T> and CIter<T>;  we can rely mostly on them
//   instead of CArray<T>/CArray<T*> and the various old CIter_XXX classes.  CHeap and CIter have a number of
//   safety features, including: (1) if a member of a CHeap gets deleted, references to it 
//   in the CHeap will be removed automatically;  (2) if a member of a CHeap gets detached or
//   deleted in the midst of a loop, CIter can handle the situation gracefully;  (3) if a CHeap as
//   a whole gets deleted in the midst of a loop, its members are detached and CIter handles that too;  (4) removal of items 
//   from a CHeap, and detection of their presence in the CHeap, are quick.
// The descendant classes are declared in Part.h, Net.h, Polyline.h, Text.h, Shape.h.

#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "DisplayList.h"
#include "DlgLog.h"
#include "gpc_232.h"

// A couple of enum's relating to CGlue and CCentroid (putting 'em here to appease the @$#% compiler)
enum GLUE_POS_TYPE
{
	GLUE_POS_CENTROID,	// at centroid
	GLUE_POS_DEFINED	// defined by user
};

// centroid types
enum CENTROID_TYPE
{
	CENTROID_DEFAULT = 0,	// center of pads
	CENTROID_DEFINED		// defined by user
};


class CPcbItem;
class CHeapLink;
template <class T> class CHeap;  // NB "T" is expected to be a CPcbItem class
template <class T> class CIter;	  // Ditto

class CVertex; 
class CTee;
class CSeg;
class CConnect;
class CPin;
class CPart;
class CCorner;	
class CSide;	
class CContour;
class CPolyline;
class CArea;
class CSmCutout;
class CBoard;
class COutline;
class CNet;
class CText;	
class CRefText;
class CValueText;
class CCentroid;
class CGlue;
class CDre;
class CPadstack;
class CShape;

class CFreePcbDoc;
class CPartList;
class CTextList;
class CNetList;

class CUndoItem;
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
	bitShape =          0x40000000,
	bitOther =			0x80000000,
	bitsNetItem = bitVia | bitPinVtx | bitTraceVtx | bitTeeVtx | bitTee | bitSeg | bitConnect |
				  bitAreaCorner | bitAreaSide | bitArea,
	bitsPartItem = bitPin | bitPart,
	bitsFootItem = bitOutlineCorner | bitOutlineSide | bitOutline | bitCentroid | bitGlue | bitPadstack | bitShape,
	bitsSelectableForGroup = bitVia | bitSeg | bitConnect | bitPart | bitText | bitAreaSide | bitSmSide | bitBoardSide
};

class CPcbItem 
{
	// THE BASE CLASS FOR EVERY ITEM ON A PCB.  
	// Best way I could figure out to give the various CHeap types access to CPcbItem's innards was to list 'em as friends 1 by 1.
	// An ugly PITA, but I suppose necessary:
	friend class CHeap<CPcbItem>;
	friend class CHeap<CVertex>;
	friend class CHeap<CTee>;
	friend class CHeap<CSeg>;
	friend class CHeap<CConnect>;
	friend class CHeap<CPin>;
	friend class CHeap<CPart>;
	friend class CHeap<CCorner>;
	friend class CHeap<CSide>;
	friend class CHeap<CContour>;
	friend class CHeap<CPolyline>;
	friend class CHeap<CArea>;
	friend class CHeap<CSmCutout>;
	friend class CHeap<CBoard>;
	friend class CHeap<COutline>;
	friend class CHeap<CNet>;
	friend class CHeap<CText>;
	friend class CHeap<CRefText>;
	friend class CHeap<CValueText>;
	friend class CHeap<CCentroid>;
	friend class CHeap<CGlue>;
	friend class CHeap<CDre>;
	friend class CHeap<CPadstack>;
	friend class CHeap<CShape>;
	friend class CUndoRecord;
	friend class CFreePcbDoc;

	CHeapLink *carray_list;	// List of CHeap's into which this item has been added
	int m_uid;
	static int next_uid;
	static CPcbItem **uid_hash;			// Hash table (with an incredibly simple hash function).  Used by FindByUid(), qv.
	static int uid_hash_sz;					// Size of uid_hash, initially 0x2000.  Must be a power of 2.
	CPcbItem *uid_hash_next;				// Linked lists for hash conflicts.
public:
	CFreePcbDoc *doc;
	CDLElement *dl_el;
	CDLElement *dl_sel;
	int utility;
	bool bDrawn;
	enum { NOERR = 0, NO_DLIST, NO_FOOTPRINT, ALREADY_DRAWN, EMPTY_TEXT };
	enum { STRAIGHT, ARC_CW, ARC_CCW };	// polyline side styles
	enum { NO_HATCH, DIAGONAL_FULL, DIAGONAL_EDGE }; // polyline hatch styles

protected:
	CPcbItem(CFreePcbDoc *_doc);
	CPcbItem(CFreePcbDoc *_doc, int _uid);		// Used only during undo operations.
	virtual ~CPcbItem();						// When an item is destroyed, references to it are automatically
												// removed from any carrays to which it belongs.
public:
	int UID() { return m_uid; }
	static CPcbItem *FindByUid(int uid);
	static int GetNextUid() { return next_uid; }
	bool IsHit(int x, int y);
	bool IsDrawn() { return bDrawn; }
	void RemoveForUndo();

	// Virtual functions:
	virtual	int Draw() { return NOERR; }
	virtual void Undraw();							// Default behavior:  done in cpp
	virtual void MustRedraw();						// CPT2 r313.  My latest-n-greatest new system for drawing/undrawing (see notes.txt). Done in cpp
													// Overridden in CPin [only?]
	virtual void Highlight() { }
	virtual bool IsOnPcb() { return false; }		// Used to be called IsValid(), but this name seemed more descriptive.  E.g. items
													// on the clipboard are perfectly "valid" (not subject to garbage-collection), but the
													// drawing and undo routines should ignore them anyway.  Check out particularly CShape::IsOnPcb().
	virtual CUndoItem *MakeUndoItem() { return NULL; }		
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
	virtual bool IsText() { return false; }			// CPT2 true for CText, CRefText and CValueText
	virtual bool IsRefText() { return false; }
	virtual bool IsValueText() { return false; }
	virtual bool IsCentroid() { return false; }
	virtual bool IsGlue() { return false; }
	virtual bool IsDRE() { return false; }
	virtual bool IsPadstack() { return false; }
	virtual bool IsShape() { return false; }

	virtual int GetTypeBit() { return 0; }				// See "enum typebits" above for return values from the various derived classes
	bool IsPartItem() { return (GetTypeBit() & bitsPartItem) != 0; }
	bool IsNetItem() { return (GetTypeBit() & bitsNetItem) != 0; }
	bool IsSelectableForGroup() { return (GetTypeBit() & bitsSelectableForGroup) != 0; }
	bool IsFootItem();

	// Type casting functions.  All return null by default, but are overridden to return type-cast pointers in specified derived classes
	virtual CVertex *ToVertex() { return NULL; }
	virtual CTee *ToTee() { return NULL; }
	virtual CSeg *ToSeg() { return NULL; }
	virtual CConnect *ToConnect() { return NULL; }
	virtual CPin *ToPin() { return NULL; }
	virtual CPart *ToPart() { return NULL; }
	virtual CCorner *ToCorner() { return NULL; }
	virtual CSide *ToSide() { return NULL; }
	virtual CContour *ToContour() { return NULL; }
	virtual CPolyline *ToPolyline() { return NULL; }
	virtual CArea *ToArea() { return NULL; }
	virtual CSmCutout *ToSmCutout() { return NULL; }
	virtual CBoard *ToBoard() { return NULL; }
	virtual COutline *ToOutline() { return NULL; }
	virtual CNet *ToNet() { return NULL; }
	virtual CText *ToText() { return NULL; }
	virtual CRefText *ToRefText() { return NULL; }
	virtual CValueText *ToValueText() { return NULL; }
	virtual CCentroid *ToCentroid() { return NULL; }
	virtual CGlue *ToGlue() { return NULL; }
	virtual CDre *ToDRE() { return NULL; }
	virtual CPadstack *ToPadstack() { return NULL; }
	virtual CShape *ToShape() { return NULL; }

	virtual CNet *GetNet() { return NULL; }					// Returns something for items that belong to a net.
	virtual CConnect *GetConnect() { return NULL; }			// Similar
	virtual CPolyline *GetPolyline() { return NULL; }
	CArea *GetArea();											// In cpp
	virtual int GetLayer() { return -1; }
	virtual void GetStatusStr( CString *str ) { *str = ""; }
	virtual void ForceVia() { }									// Implemented in CVertex and CTee
	virtual void UnforceVia() { }								// Implemented in CVertex and CTee
};

class CHeapLink
{
	// Used in forging linked lists of CHeap<T> pointers (which are declared below as void pointers for simplicity).  When a CPcbItem is
	// added to a CHeap, one of these links is hooked to its carray_list.  In that way we know which carrays it belongs to (I don't foresee
	// many items belonging to more than 1 or 2 arrays in general).  We can also use the "off" member of this structure to determine what position
	// the item has in the indicated array.
	// Same dumb business with friends as in CPcbItem :(
	friend class CPcbItem;
	friend class CHeap<CPcbItem>;
	friend class CHeap<CVertex>;
	friend class CHeap<CTee>;
	friend class CHeap<CSeg>;
	friend class CHeap<CConnect>;
	friend class CHeap<CPin>;
	friend class CHeap<CPart>;
	friend class CHeap<CCorner>;
	friend class CHeap<CSide>;
	friend class CHeap<CContour>;
	friend class CHeap<CPolyline>;
	friend class CHeap<CArea>;
	friend class CHeap<CSmCutout>;
	friend class CHeap<CBoard>;
	friend class CHeap<COutline>;
	friend class CHeap<CNet>;
	friend class CHeap<CText>;
	friend class CHeap<CRefText>;
	friend class CHeap<CValueText>;
	friend class CHeap<CCentroid>;
	friend class CHeap<CGlue>;
	friend class CHeap<CDre>;
	friend class CHeap<CPadstack>;
	friend class CHeap<CShape>;

	void *arr;									// Really a CHeap<T> pointer, for some T.
	int off;
	CHeapLink *next;
};


template <class T> class CHeap 
{
	// An array designed to hold items in classes derived from CPcbItem.  Note that entries in these arrays are pointers, not objects (so 
	// that CHeap<CPin> is more like CArray<CPin*> than CArray<CPin>).  Each CPcbItem has an attached linked list of the carrays to which it
	// belongs, which allows various maintenance chores to be done automatically.  When an item is removed from a CHeap, the slot it occupied
	// is marked as free and linked to a list for later reuse.
	// For ease of reference and for a potential inline speed-boost, I put the code here in the .h
	friend class CPcbItem;
	friend class CIter<T>;
	T **heap;					// Expandable array of data pointers.
	unsigned char *flags;		// Bitset, indicating which elements of "heap" contain valid entries.  A zero bit indicates a free slot.
								// Bits beyond position "maxOff" are undefined.
	int nHeap;					// Total number of slots in heap
	int nItems;					// Number of filled slots
	int free;					// The head of the free slot list (an index into "heap").  -1 => everything through maxOff is filled
	int maxOff;					// Maximum offset among the slots that are either filled, or hooked to the free list
	CIter<T> *iters;			// Linked list of iterators that are actively looping through this.  Typically I don't expect more than 1 or 2...

public:
	CHeap(int nHeapInit = 16)
	{
		// Create the array, with the given number of initial entries in the heap.
		heap = (T**) HeapAlloc(GetProcessHeap(), 0, nHeapInit*sizeof(T**));
		flags = (unsigned char*) HeapAlloc(GetProcessHeap(), 0, nHeapInit/8 + 1);
		nHeap = nHeapInit;
		nItems = 0;
		maxOff = free = -1;
		iters = NULL;
	}

	~CHeap()
	{
		// Frees up heap and flags, and safely removes all references to this from its member items.  But it doesn't destroy the members.
		// NB check out how this destructor is invoked within ~CPart and the like...
		RemoveAll();
		HeapFree(GetProcessHeap(), 0, heap);
		HeapFree(GetProcessHeap(), 0, flags);
		// Blank out references to this from active iterators:
		for (CIter<T> *iter = iters; iter; iter = iter->nextIter)
			iter->arr = NULL;
	}
	

	void Add(T* item)
	{
		// NB If item is null, we get a crash.  OK?
		// Add item to this CHeap.  But first check if the item already belongs to this, and return silently if so
		for (CHeapLink *link = item->carray_list; link; link = link->next)
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
		// Add item!  Then create a CHeapLink that connects the item with this CHeap.
		heap[off] = item;
		flags[off>>3] |= 1<<(off&7);						// Set flag for "off"
		CHeapLink *link = new CHeapLink();
		link->off = off;
		link->arr = this;
		link->next = item->carray_list;
		item->carray_list = link;
		nItems++;
	}

	void Add(CHeap<T> *arr) 
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
		for (CHeapLink *link = item->carray_list; link; link = link->next)
			if (link->arr == this) return true;
		return false;
	}

	int OffsetOf(T* item)
	{
		for (CHeapLink *link = item->carray_list; link; link = link->next)
			if (link->arr == this) return link->off;
		return INT_MAX;
	}

	void Remove(T* item)
	{
		// Remove "item" from this.  If item isn't in the list, do an ASSERT (?).  If item is null, crash (?)
		CHeapLink *link, *prev = NULL;
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

	void TransferTo(CHeap<T> *arr)
	{
		// Move the contents of this into "arr", removing all from this CHeap afterwards.
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
		// Mark utility value for all members of the CHeap
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
		// Read "ct" items from the CHeap into "out". Return false if ct is too large.  Maybe unneeded.
		if (ct>nItems) return false;
		int j = 0;
		for (int i=0; j<ct; i++)
			if (flags[i>>3] & (1<<(i&7)))
				out[j++] = heap[i];
		return true;
	}
};


template <class T> class CIter
{
	// Use these classes to iterate through the members of a CHeap<T>.  That way we can handle it gracefully if an item is removed from the array 
	//  from within the loop, and even if the array as a whole is destroyed.
	// Sample usage:
	//		CIter<CSeg> iseg (con->segs);
	//		for (CSeg *seg = iseg.First(); seg; seg = iseg.Next())
	//			...
	friend class CHeap<T>;
	CHeap<T> *arr;
	int nextOff;								// Next offset at which Next() will seek a new entry.
	CIter<T> *nextIter;							// Active iterators for a particular array are linked (singly) in a list.

public:
	CIter(CHeap<T> *arr0)
	{
		arr = arr0;
		nextOff = 0;
		// Hook this to arr's list of iterators.
		nextIter = arr->iters;
		arr->iters = this;
	}
	CIter(CHeap<T> *arr0, T *at) 
	{
		// Iterate through, starting at the position of "at". If "at" is not in the array, advance iterator past the end
		arr = arr0;
		nextOff = arr0->OffsetOf(at);
		nextIter = arr->iters;
		arr->iters = this;
	}

	~CIter()
	{
		// Remove this iterator from its array's list of iterators.  Since the list is singly-linked, the process may be inefficient;  
		//   but I expect the list to be very short and the overall savings should outweight the penalty.
		if (!arr) return;								// Array has been destroyed!
		if (arr->iters==this)
			arr->iters = nextIter;
		else 
		{
			CIter<T> *prev = NULL;
			for (CIter<T> *iter = arr->iters; iter; iter = iter->nextIter)
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




