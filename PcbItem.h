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
#include "PartList.h"
#include "PolyLine.h"
#include "UndoList.h"
#include "LinkList.h"
#include "Cuid.h"
#include "NetList.h"

class cpcb_item;
class carray_link;
template <class T> class carray;  // NB "T" is expected to be a cpcb_item class
template <class T> class citer;	  // Ditto

class cvertex;
class ctee;
class cseg;
class cconnect;
class cpin;
class cpart;
class CPolyCorner;			// TODO: rename ccorner
class CPolySide;			// TODO: rename cside
class ccontour;
class CPolyLine;			// TODO: rename cpolyline
class carea;
class csmcutout;
class cboard;
class coutline;
class cnet;
class CText;				// TODO: rename ctext
class creftext;
class cvaluetext;
class ccentroid;
class cadhesive;
class cdrc;

class undo_con;
class undo_seg;
class undo_vtx;

enum typebits {
	// Bit flags to represent the types of the various types of PCB items.  The main use for these will be in implementing selection masks.
	bitVia = 0x4,
	bitPinVtx = 0x8,
	bitTraceVtx = 0x10,
	bitSeg = 0x20,
	bitConnect = 0x40,
	bitPin = 0x80,
	bitPart = 0x100,
	bitAreaCorner = 0x1,
	bitAreaSide = 0x2,
	bitSmCorner = 0x200,
	bitSmSide = 0x400,
	bitBoardCorner = 0x800,
	bitBoardSide = 0x1000,
	bitOutlineCorner = 0x2000,		// Within the footprint editor only...
	bitOutlineSide = 0x4000,		// Within the footprint editor only...
	bitContour = 0x8000,
	bitArea = 0x10000,
	bitSmCutout = 0x20000,
	bitBoard = 0x40000,
	bitOutline = 0x80000,			// Fp editor only...
	bitNet = 0x100000,
	bitText = 0x200000,
	bitRefText = 0x400000,
	bitValueText = 0x800000,
	bitCentroid = 0x1000000,		// Fp editor only...
	bitAdhesive = 0x2000000,		// Fp editor only...
	bitDRC = 0x4000000,
	bitOther = 0x80000000,
	bitsNetItem = bitVia | bitPinVtx | bitTraceVtx | bitSeg | bitConnect |
				  bitAreaCorner | bitAreaSide | bitArea,
	bitsPartItem = bitPin | bitPart
};

class cpcb_item 
{
	// THE BASE CLASS FOR EVERY ITEM ON A PCB.  
	// Best way I could figure out to give the various carray types access to cpcb_item's innards was to list 'em as friends 1 by 1.
	// An ugly PITA, but I suppose necessary:
	friend class carray<cvertex>;
	friend class carray<ctee>;
	friend class carray<cseg>;
	friend class carray<cconnect>;
	friend class carray<cpin>;
	friend class carray<cpart>;
	friend class carray<CPolyCorner>;
	friend class carray<CPolySide>;
	friend class carray<ccontour>;
	friend class carray<CPolyLine>;
	friend class carray<carea>;
	friend class carray<csmcutout>;
	friend class carray<cboard>;
	friend class carray<coutline>;
	friend class carray<cnet>;
	friend class carray<CText>;
	friend class carray<creftext>;
	friend class carray<cvaluetext>;
	friend class carray<ccentroid>;
	friend class carray<cadhesive>;
	friend class carray<cdrc>;

	carray_link *carray_list;	// List of carray's into which this item has been added
	int m_uid;
	static int next_uid;
	dl_element *dl_el;			// public?  Do enough of the derived classes want it to merit its inclusion?
	dl_element *dl_sel;			// Ditto...
public: // ?
	int utility;

protected:
	cpcb_item() 
		{ carray_list = NULL; m_uid = next_uid++; }
	~cpcb_item();								// Had to define this in the .cpp.  When an item is destroyed, references to it are automatically
												// removed from any carrays to which it belongs.
public:
	int UID() { return m_uid; }

	// Virtual functions:
	virtual	void Draw() { }
	virtual void Undraw() { }
	virtual BOOL IsDrawn() { return false; }
	virtual void Highlight() { }
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
	virtual bool IsSide() { return false; }
	virtual bool IsContour() { return false; }
	virtual bool IsPolyline() { return false; }
	virtual bool IsArea() { return false; }
	virtual bool IsSmCutout() { return false; }
	virtual bool IsBoard() { return false; }
	virtual bool IsOutline() { return false; }
	virtual bool IsNet() { return false; }
	virtual bool IsText() { return false; }			// Also false for creftext and cvaluetext
	virtual bool IsRefText() { return false; }
	virtual bool IsValueText() { return false; }
	virtual bool IsCentroid() { return false; }
	virtual bool IsAdhesive() { return false; }
	virtual bool IsDRC() { return false; }

	virtual int GetTypeBit() { return 0; }			// See "enum typebits" above for return values from the various derived classes
	bool IsPartItem() { return (GetTypeBit() & bitsNetItem) != 0; }
	bool IsNetItem() { return (GetTypeBit() & bitsPartItem) != 0; }

	// Type casting functions.  All return null by default, but are overridden to return type-cast pointers in specified derived classes
	virtual cvertex *ToVertex() { return NULL; }
	virtual ctee *ToTee() { return NULL; }
	virtual cseg *ToSeg() { return NULL; }
	virtual cconnect *ToConnect() { return NULL; }
	virtual cpin *ToPin() { return NULL; }
	virtual cpart *ToPart() { return NULL; }
	virtual CPolyCorner *ToCorner() { return NULL; }
	virtual CPolySide *ToSide() { return NULL; }
	virtual ccontour *ToContour() { return NULL; }
	virtual CPolyLine *ToPolyline() { return NULL; }
	virtual carea *ToArea() { return NULL; }
	virtual csmcutout *ToSmCutout() { return NULL; }
	virtual cboard *ToBoard() { return NULL; }
	virtual coutline *ToOutline() { return NULL; }
	virtual cnet *ToNet() { return NULL; }
	virtual CText *ToText() { return NULL; }
	virtual creftext *ToRefText() { return NULL; }
	virtual cvaluetext *ToValueText() { return NULL; }
	virtual ccentroid *ToCentroid() { return NULL; }
	virtual cadhesive *ToAdhesive() { return NULL; }
	virtual cdrc *ToDRC() { return NULL; }
};

/*
.cpp FILE STUFF:

int cpcb_item::next_uid = 1;

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
}
*/

class carray_link
{
	// Used in forging linked lists of carray<T> pointers (which are declared below as void pointers for simplicity).  When a cpcb_item is
	// added to a carray, one of these links is hooked to its carray_list.  In that way we know which carrays it belongs to (I don't foresee
	// many items belonging to more than 1 or 2 arrays in general).  We can also use the "off" member of this structure to determine what position
	// the item has in the indicated array.
	friend class cpcb_item;
	friend class carray<cpin>;
	void *arr;									// Really a carray<T> pointer, for some T.
	int off;
	carray_link *next;
};


template <class T> class carray 
{
	// An array designed to hold items in classes derived from cpcb_item.  Note that entries in these arrays are pointers, not objects (so 
	// that carray<cpin> is more like CArray<cpin*> than CArray<cpin>).  Each cpcb_item has an attached linked list of the carrays to which it
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
			free = *(int*) heap[off];
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

	bool Contains(T* item)
	{
		// Return true if "item" is in the array.
		for (carray_link *link = item->carray_list; link; link = link->next)
			if (link->arr == this) return true;
		return false;
	}

	void Remove(T* item)
	{
		// Remove "item" from this.  If item isn't in the list, do an ASSERT (?)
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

	int GetNItems() { return nItems; }
	
	T *FindByUID(int uid)
	{
		for (int i=0; i<=maxOff; i++)
			if (flags[i>>3] & (1<<(i&7)))
				if (heap[i]->UID() == uid)
					return heap[i];
		return NULL;
	}
};


template <class T> class citer
{
	// Use these classes to iterate through the members of a carray<T>.  That way we can handle it gracefully if an item is removed from the array 
	//  from within the loop, and even if the array as a whole is destroyed.
	// Sample usage:
	//		citer<cseg> iseg (con->m_segs);
	//		for (cseg *seg = iseg.First(); seg; seg = iseg.Next())
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
/*  RELATED TO cseg/cvertex/cconnect                                                          */
/**********************************************************************************************/

// cvertex: describes a vertex between segments in a connection
class cvertex: public cpcb_item
{
public:
	/* types of vertices.  CPT:  can probably do without.
	enum VType { 
		V_ERR = 0,	// unknown
		V_PIN,		// pin at end of trace
		V_TRACE,	// vertex between trace segments
		V_END,		// end vertex of stub trace
		V_TEE,		// T-vertex (defined here)
		V_SLAVE		// T-vertex (defined in another trace)
	};	
	*/

	cconnect * m_con;			// parent connection
	cnet *m_net;				// CPT.  Very likely useful.
	cseg *preSeg, *postSeg;		// Succeeding and following segs.  If either is null, this is an end seg.
	ctee *tee;					// Points to a tee structure at tee-junctions, null otherwise
	cpin *pin;					// If this vertex is at a pin, point to the said object
	int x, y;					// coords
	int pad_layer;				// layer of pad if this is first or last vertex, otherwise 0
	int force_via_flag;			// force a via even if no layer change
	int via_w, via_hole_w;		// via width and hole width (via_w==0 means no via)
	int utility2;				// used for various functions
	CArray<dl_element*> dl_els;	// CPT renamed;  contains one dl_element per layer (CHECK INTO THIS)
	dl_element * dl_hole;		// hole in via
	CDisplayList * m_dlist;		// NULL if not drawable

	cvertex(cconnect *c);					// Added arg
	cvertex( cvertex& src );
	~cvertex();
	// cvertex &operator=( const cvertex &v );  // assignment	
	// cvertex &operator=( cvertex &v );		// assignment
	// void Initialize( cconnect * c );			// Move to constructor
	bool IsVertex() { return true; }
	bool IsVia() { return via_w>=0; }
	bool IsSlaveVtx() { return tee!=NULL; }
	bool IsPinVtx() { return pin!=NULL; }
	bool IsEndVtx() { return (!preSeg || !postSeg) && !pin && !tee; }
	bool IsTraceVtx() { return preSeg && postSeg; }
	cvertex *ToVertex() { return this; }
	int GetTypeBit()
	{ 
		if (via_w>=0) return bitVia;
		if (pin) return bitPinVtx;
		return bitTraceVtx;
	}

	void Draw();
	void Undraw();
	void Highlight();

	// VType GetType();
	void GetTypeStatusStr( CString * str );
	void GetStatusStr( CString * str );

	bool IsConnectedToArea( carea * a );
	int GetConnectedAreas( carray<carea> *a=NULL );		// Arg change
};

class ctee: public cpcb_item 
{
	// CPT new.  Basically a carray <cvertex>, indicating the vertices that together join up to form a single tee junction.
	carray<cvertex> vtx;

	ctee() { }
	~ctee() { }
	bool IsTee() { return true; }
	ctee *ToTee() { return this; }
	int GetTypeBit() { return bitOther; }				// tee objects won't have selector drawing-elements, so this is kind of irrelevant
};


// cseg: describes a segment of a connection
class cseg: public cpcb_item
{
	// static int m_array_step;
	int m_layer;
	int m_width;
	int m_curve;
	bool m_selected;
public:
	CDisplayList * m_dlist;
	cnet * m_net;				// parent net
	cconnect * m_con;			// parent connection
	cvertex *preVtx, *postVtx;

	enum Curve { STRAIGHT = 0,
			CCW,
			CW
	};

	cseg(cconnect *c);							// CPT added arg
	~cseg();
	cseg( cseg& src );							// copy constructor
	// cseg& operator=( cseg& rhs );			// assignment
	// cseg& operator=( const cseg& rhs );		// assignment
	bool IsSeg() { return true; }
	cseg *ToSeg() { return this; }
	int GetTypeBit() { return bitSeg; }

	// void Initialize( cconnect * c );			// Put in constructor
	void Draw();
	void Undraw();
	BOOL IsDrawn();

	void SetVisibility( int vis );
	int Index();
	void GetStatusStr( CString * str, int width = 0 );			// CPT added width param
};

// cconnect: describes a sequence of segments, running between two end vertices of arbitrary type (pin, tee, isolated end-vertex...)
class cconnect: public cpcb_item
{
	// friend class CNetList;
	// friend class cnet;
public:
	enum Dir {
		ROUTE_FORWARD = 0,
		ROUTE_BACKWARD
	};

	cnet * m_net;				// parent net
	carray<cseg> seg;			// array of segments. NB not necessarily in geometrical order
	carray<cvertex> vtx;		// array of vertices. NB ditto
	cvertex *head, *tail;		// CPT for geometrical traversing.	

	bool locked;				// true if locked (will not be optimized away)
	BOOL m_bDrawn;				// drawn into display list or not
	CDisplayList * m_dlist;		

	// these params used only by DRC
	int min_x, max_x;			// bounding rect
	int min_y, max_y;
	BOOL vias_present;			// flag to indicate that vias are present. 
	int seg_layers;				// mask for all layers used by segments.  CPT MUST STUDY

// methods
public:
	// general
	cconnect( cnet * net );
	~cconnect();
	bool IsConnect() { return true; }
	cconnect *ToConnect() { return this; }
	int GetTypeBit() { return bitConnect; }			// Rarely used since connects don't have selector elements.

	void ClearArrays();
	void GetStatusStr( CString * str );

	// get info about start/ending pins
	cpin * StartPin() { return head->pin; }
	cpin * EndPin() { return tail->pin; }

	// get info about vertices
	int NumVtxs() { return vtx.GetNItems(); }
	// cvertex * FirstVtx();
	// cvertex * LastVtx();
	// cvertex * VtxByUID( int uid, int * index=NULL );         // Use vtx.FindByUID() instead
	cvertex *VtxByIndex( int iv );								// CPT: maybe, maybe not...  changed from cvertex& ret-val to cvertex*
	int VtxIndexByPtr( cvertex * v );							// CPT: maybe, maybe not...

	// get info about segments
	int NumSegs() { return seg.GetNItems(); }
	// void SetNumSegs( int n );
	cseg *FirstSeg() { return head->postSeg; }
	cseg *LastSeg() { return tail->preSeg; }
	cseg *SegByUID( int uid, int * index=NULL );
	cseg *SegByIndex( int is );					// CPT: maybe, maybe not....
	int SegIndexByPtr( cseg * s );				// CPT: ditto
	// int SegUIDByPtr( cseg * s );

	// modify segments and vertices.  CPT: Still need to rethink these.  For consistency, get rid of cvertex& args in favor of cvertex*?
	// Less reliance on indices?
	cvertex& InsertVertexByIndex( int iv, const cvertex& new_vtx );
	void InsertSegAndVtxByIndex( int is, int dir, 
				const cseg& new_seg, const cvertex& new_vtx );
	void AppendSegAndVertex( const cseg& new_seg, const cvertex& new_vtx );
	void PrependVertex( const cvertex& new_vtx );
	void PrependVertexAndSeg( const cvertex& new_vtx, const cseg& new_seg );
	void ReverseDirection();

	// drawing methods
	void Draw();
	void Undraw();
	void HighLight();
};


/**********************************************************************************************/
/*  RELATED TO cpin/cpart                                                                     */
/**********************************************************************************************/

// cpin: describes a pin in a net.  Folded the old part_pin into this.
class cpin : public cpcb_item
{
public:
	// CString ref_des;	// reference designator such as 'U1'.  READ OFF of part->ref_des
	CString pin_name;	// pin name such as "1" or "A23"
	cpart * part;		// pointer to part containing the pin.
	cnet * m_net;		// parent net.    TODO change to "net" for consistency
	int x, y;				// position on PCB
	cnet * net;				// pointer to net, or NULL if not assigned
	drc_pin drc;			// drc info
	dl_element * dl_hole;	// pointer to graphic element for hole
	CArray<dl_element*> dl_els;	// array of pointers to graphic elements for pads

	cpin(cnet *net);					// Added arg
	cpin( cpin& src );
	~cpin();
	bool IsPin() { return true; }
	cpin *ToPin() { return this; }
	int GetTypeBit() { return bitPin; }

	// void Initialize( cnet * net );  // Put in constructor
};


// class cpart represents a part
class cpart: public cpcb_item
{
public:
	// The partlist will be a carray, so no links required:
	// cpart * prev;		// link backward
	// cpart * next;		// link forward
	CPartList * m_pl;	// parent partlist
	carray<cpin> pin;				// array of all pins in part
	BOOL drawn;			// TRUE if part has been drawn to display list.  ?? TODO Put into base class?
	BOOL visible;		// 0 to hide part                                ?? TODO Put into base class?
	int x,y;			// position of part origin on board
	int side;			// 0=top, 1=bottom
	int angle;			// orientation, degrees CW
	BOOL glued;			// 1=glued in place
	BOOL m_ref_vis;		// TRUE = ref shown
	int m_ref_xi;		// ref text params (relative to part)
	int m_ref_yi;	
	int m_ref_angle; 
	int m_ref_size;
	int m_ref_w;
	int m_ref_layer;	// layer if part is on top	
	BOOL m_value_vis;	// TRUE = value shown
	int m_value_xi;		// value text params (relative to part)
	int m_value_yi; 
	int m_value_angle; 
	int m_value_size; 
	int m_value_w;
	int m_value_layer;	// layer if part is on top
	// In base: dl_element * dl_sel;		// pointer to display list element for selection rect
	CString ref_des;			// ref designator such as "U3"
	dl_element * dl_ref_sel;	// pointer to selection rect for ref text 
	CString value;				// "value" string
	dl_element * dl_value_sel;	// pointer to selection rect for value 
	CString package;			// package (from original imported netlist, may be "")
	CShape * shape;				// pointer to the footprint of the part, may be NULL
	CArray<stroke> ref_text_stroke;		// strokes for ref. text // TODO CArray or carray?
	CArray<stroke> value_stroke;		// strokes for ref. text
	CArray<stroke> m_outline_stroke;	// array of outline strokes

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

	cpart();
	cpart( CPartList * pl );
	~cpart();
	bool IsPart() { return true; }
	cpart *ToPart() { return this; }
	int GetTypeBit() { return bitPart; }

	cpin * PinByUID( int uid, int * index=NULL );
	int GetNumRefStrokes();
	int GetNumValueStrokes();
	int GetNumOutlineStrokes();
};


/**********************************************************************************************/
/*  RELATED TO cpolyline/carea/csmcutout                                                      */
/**********************************************************************************************/

// TODO: Rename ccorner, cside, cpolyline

class CPolyCorner: public cpcb_item
{
public:
	int x, y;
	ccontour *contour;				// CPT.
	// dl_element * dl_corner_sel;  // Use cpcb_item::dl_sel
	CPolySide *preSide, *postSide;	// CPT

	CPolyCorner();
	CPolyCorner( CPolyCorner& src );	// copy constructor
	~CPolyCorner();
	// CPolyCorner& operator=( const CPolyCorner& rhs );	// assignment
	// CPolyCorner& operator=( CPolyCorner& rhs );	// assignment
	bool IsCorner() { return true; }
	CPolyCorner *ToCorner() { return this; }
	int GetTypeBit() 
	{														// Later:  put in .cpp file.  There wouldn't be this nonsense in Java...
		if (contour->poly->IsArea()) return bitAreaCorner;
		if (contour->poly->IsSmCutout()) return bitSmCorner;
		if (contour->poly->IsBoard()) return bitBoardCorner;
		return bitOther;
	}

	void Highlight();		// CPT
};

class CPolySide: public cpcb_item
{
public:
	int m_style;					// TODO change to style?
	ccontour *contour;				// CPT
	// dl_element * dl_side;		// Use base dl_el
	// dl_element * dl_side_sel;	// Use base dl_sel
	CPolyCorner *preCorner, *postCorner;

	CPolySide();
	CPolySide(ccontour *ctr, CPolyCorner *pre, CPolyCorner *post)
		{ contour = ctr; preCorner = pre; postCorner = post; pre->postSide = this; post->preSide = this; }
	CPolySide( CPolySide& src );
	~CPolySide();
	//	CPolySide& operator=( const CPolySide& rhs );	// assignment
	//	CPolySide& operator=( CPolySide& rhs );	// assignment
	bool IsSide() { return true; }
	CPolySide *ToSide() { return this; }
	int GetTypeBit() 
	{
		if (contour->poly->IsArea()) return bitAreaCorner;
		if (contour->poly->IsSmCutout()) return bitSmCorner;
		if (contour->poly->IsBoard()) return bitBoardCorner;
		return bitOther;
	}

	void Highlight();		// CPT
	void SetVisible();		// CPT
	void IsOnCutout()
		{ return contour->poly->main!=contour; }
};

class ccontour: public cpcb_item
{
public:
	// All CPT.
	carray<CPolyCorner> corner;	// Array of corners.  NB entries not necessarily in geometrical order
	carray<CPolySide> side;		// Array of sides
	CPolyCorner *head, *tail;	// For when we need to trace thru geometrically.
	CPolyLine *poly;			// The parent polyline

	ccontour();
	~ccontour();
	bool IsContour() { return true; }
	ccontour *ToContour() { return this; }
	int GetTypeBit() { return bitContour; }

	CRect GetCornerBounds();
	BOOL TestPointInside( int x, int y );
	void Close()
	{
		if (head==tail) return;
		side.Add(new CPolySide(this, tail, head));
		tail = head;
	}
	void Unclose();				// TODO.  Write something analogous...
};

class CPolyLine: public cpcb_item
{
public:
	enum { STRAIGHT, ARC_CW, ARC_CCW };	// side styles
	enum { NO_HATCH, DIAGONAL_FULL, DIAGONAL_EDGE }; // hatch styles

protected:
	CDisplayList * m_dlist;		// display list 

	cpcb_item *parent;			// CPT.  Probably obsolete...
	ccontour *main;				// CPT
	carray<ccontour> contours;	// CPT.  Secondary contours
	// id m_parent_id;			// id of the parent of the polyline
	// void * m_ptr;			// pointer to parent object (or NULL)
	int m_layer;				// layer to draw on
	int m_w;					// line width
	int m_sel_box;				// corner selection box width/2
	int m_hatch;				// hatch style, see enum above
	int m_nhatch;				// number of hatch lines
	CArray <dl_element*>  dl_hatch;	// hatch lines.  Use CArray with dl-elements generally?
	gpc_polygon * m_gpc_poly;	// polygon in gpc format
	polygon * m_php_poly;
	BOOL bDrawn;

public:
	// constructors/destructor
	CPolyLine();
	~CPolyLine();
	bool IsPolyline() { return true; }
	CPolyLine *ToPolyline() { return this; }

	// functions for creating and modifying polyline
	void Clear();
	void Start( int layer, int w, int sel_box, int x, int y,
		int hatch, id * id, void * ptr, BOOL bDraw=TRUE );
	void AppendCorner( int x, int y, int style = STRAIGHT );
	void InsertCorner( CPolyCorner *c, int x, int y );		// Arg change.  Insert prior to "corner"
	void DeleteCorner( CPolyCorner *c );					// Arg change.
	BOOL MoveCorner( CPolyCorner *c, int x, int y, BOOL bEnforceCircularArcs=FALSE );	// Put into CPolyCorner?
	void Close( int style = STRAIGHT );
	void RemoveContour( ccontour *ctr );					// Arg change

	// drawing functions
	// void HighlightSide( int is );		// Change to CPolySide::Highlight();
	// void HighlightCorner( int ic );		// sim.
	void StartDraggingToInsertCorner( CDC * pDC, CPolyCorner *c, int x, int y, int crosshair = 1 ); // Arg change
	void StartDraggingToMoveCorner( CDC * pDC, CPolyCorner *c, int x, int y, int crosshair = 1 );	// Arg change
	void CancelDraggingToInsertCorner( CPolyCorner *c );											// Arg change
	void CancelDraggingToMoveCorner( CPolyCorner *c );												// Arg change
	void Undraw();
	void Draw( CDisplayList * dl = NULL );
	void Hatch();
	void MakeVisible( BOOL visible = TRUE );
	void MoveOrigin( int x_off, int y_off );
	// void SetSideVisible( int is, int visible );  // Use CPolySide::SetVisible()

	// misc. functions
	CRect GetBounds();
	CRect GetCornerBounds();
	// CRect GetCornerBounds( int icont );			// Use ccontour::GetCornerBounds()
	void Copy( CPolyLine * src );
	BOOL TestPointInside( int x, int y );
	// BOOL TestPointInsideContour( int icont, int x, int y ); // Use ccontour fxn
	int TestIntersection( CPolyLine * poly );
	void AppendArc( int xi, int yi, int xf, int yf, int xc, int yc, int num );	// Should we put this in ccontour?

	// undo functions
	int SizeOfUndoRecord();
	void SetFromUndo( undo_poly * un_poly );
	void CreatePolyUndoRecord( undo_poly * un_poly );

	// access functions
	// id Id();
	// int UID();   in base class
	int NumCorners();				// Also in ccontour?
	int NumSides();					// Ditto
	bool IsClosed()
		{ return main->head==main->tail; }
	int NumContours();
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
	int Layer() { return m_layer; }							// Put in the header...
	int W() { return m_w; }
	// int CornerUID( int ic ){ return corner[ic].m_uid; };
	// int SideUID( int is );
	// int SideStyle( int is );
	// void * Ptr(){ return m_ptr; };
	int SelBoxSize();
	CDisplayList * DisplayList() { return m_dlist; }		// Hard to get excited about these...
	int GetHatch() { return m_hatch; }
	BOOL Drawn() { return bDrawn; };
	gpc_polygon * GetGpcPoly(){ return m_gpc_poly; }

	// void SetParentId( id * id );
	// void SetUID( int uid );					// Seems fishy...
	// void SetCornerUID( int ic, int uid );	// Put in CPolyCorner or dump...
	// void SetSideUID( int is, int uid );
	// void SetClosed( BOOL bClosed );			// Use ccontour::Close() and ccontour::Unclose()
	// void SetX( int ic, int x );
	// void SetY( int ic, int y );
	void SetEndContour( int ic, BOOL end_contour );
	// void SetUtility( int u ){ utility = u; };								// Maybe just make cpcb_item::utility public...
	// void SetUtility( int ic, int utility ){ corner[ic].utility = utility; }  // Use corner->utility = ...
	void SetLayer( int layer );
	void SetW( int w );
	void SetSideStyle( int is, int style, BOOL bDraw=TRUE );
	void SetSelBoxSize( int sel_box );
	void SetHatch( int hatch )
		{ Undraw(); m_hatch = hatch; Draw(); };
	void SetDisplayList( CDisplayList * dl );
	void SetDrawn( BOOL drawn )
		{ bDrawn = drawn; }

	// GPC functions
	int MakeGpcPoly( int icontour=0, CArray<CArc> * arc_array=NULL );
	int FreeGpcPoly();
	int NormalizeWithGpc( CArray<CPolyLine*> * pa=NULL, BOOL bRetainArcs=FALSE );
	int RestoreArcs( CArray<CArc> * arc_array, CArray<CPolyLine*> * pa=NULL );
//	CPolyLine * MakePolylineForPad( int type, int x, int y, int w, int l, int r, int angle );
//	void AddContourForPadClearance( int type, int x, int y, int w, 
//						int l, int r, int angle, int fill_clearance,
//						int hole_w, int hole_clearance, BOOL bThermal=FALSE, int spoke_w=0 );
	void ClipGpcPolygon( gpc_op op, CPolyLine * poly );

	// PHP functions
	int MakePhpPoly();
	void FreePhpPoly();
	void ClipPhpPolygon( int php_op, CPolyLine * poly );
};

// carea: describes a copper area in a net.
class carea : public CPolyLine
{
public:
	cnet * m_net;		// parent net
	carray<cpin> pin;	// CPT: replaces npins, pin, PinByIndex() below.  HOW DOES THIS GET SET UP?
	CArray<dl_element*> dl_thermal;		// graphics for thermals on pins
	CArray<dl_element*> dl_via_thermal; // graphics for thermals on stubs
	int nvias;							// number of via connections to area
	carray<cconnect> vcon;				// Changed from CArray<int>.  DON'T YET UNDERSTAND USAGE
	carray<cvertex> vtx;				// Ditto
	int utility2;

	carea();
	// carea( const carea& source);		// dummy copy constructor
	~carea();						
	// carea &operator=( carea &a );		// dummy assignment operator
	// carea &operator=( const carea &a );	// dummy assignment operator

	void Initialize( CDisplayList * dlist, cnet * net );		// TODO Rethink relationship to constructor??
	bool IsArea() { return true; }
	carea *ToArea() { return this; }
	int GetTypeBit() { return bitArea; }

	// The following supplanted by using pin, vcon, and vtx members above.
	// int NumVertices();
	// cvertex * VtxByIndex( int iv );
	// int NumPins();
	//	cpin * PinByIndex( int ip );
	//	int npins;			// number of thru-hole pins within area on same net
	//	CArray<int> pin;	// array of thru-hole pins.                           CPT: HOW IS THIS SET UP?
	//	cpin * PinByIndex( int ip );
	// CArray<int> vcon;	// connections 
	// CArray<int> vtx;	// vertices
};


class csmcutout : public CPolyLine
{
	// Represents solder-mask cutouts, which are always closed polylines
public:
	csmcutout() { }
	~csmcutout() { }
	bool IsSmCutout() { return true; }
	csmcutout *ToSmCutout() { return this; }
	int GetTypeBit() { return bitSmCutout; }
}


class cboard : public CPolyLine
{
public:
	// Represents board outlines.
	cboard() { }
	~cboard() { }
	bool IsBoard() { return true; }
	cboard *ToBoard() { return this; }
	int GetTypeBit() { return bitBoard; }
}

class coutline : public CPolyLine
{
public:
	// Represents outlines within footprints
	coutline() { }
	~coutline() { }
	bool IsOutline() { return true; }
	coutline *ToOutline() { return this; }
	int GetTypeBit() { return bitOutline; }
}

/**********************************************************************************************/
/*  RELATED TO cnet                                                                           */
/**********************************************************************************************/

// cnet: describes a net
class cnet: public cpcb_item
{
	friend class CNetList;

	CString name;		// net name
private:
	carray<cconnect> connect;	// array of pointers to connections.  Type change
public:
	carray<cpin> pin;			// array of pins
	carray<carea> area;			// array of copper areas
	int def_w;					// default trace width
	int def_via_w;				// default via width
	int def_via_hole_w;			// default via hole width
	BOOL visible;				// FALSE to hide ratlines and make unselectable.  TODO put in base class?
	int utility2;				// used to keep track of which nets have been optimized
	CDisplayList * m_dlist;		// CDisplayList to use
	CNetList * m_nlist;			// parent netlist


public:
	cnet( CDisplayList * dlist, CNetList * nlist );
	~cnet();
	bool IsNet() { return true; }
	cnet *ToNet() { return this; }
	int GetTypeBit() { return bitNet; }

	// general
	void GetStatusStr( CString * str );
	// pins
	int NumPins();
	// cpin * PinByIndex( int ip );
	// ? cpin * PinByUID( int uid, int * index );
	void SetVertexToPin( cpin *p, cvertex * v );  // Arg change

	// connections
	int NumCons() { return connect.GetNItems(); }
	// cconnect * ConByIndex( int ic );
	// cconnect * ConByUID( int uid, int * index=NULL );
	// int ConIndexByPtr( cconnect * c );

	// copper areas
	int NumAreas() { return area.GetNItems(); }
	// carea * AreaByUID( int uid, int * index=NULL );
	// carea * AreaByIndex( int ia );

// methods that edit object
	// pins
	void AddPin( CString * ref_des, CString * pin_name, BOOL bSetAreas=TRUE );
	void RemovePin( cpin *pin, BOOL bSetAreas=TRUE );											// CPT
	// ? void RemovePinByUID( int uid, BOOL bSetAreas=TRUE );
	void RemovePin( CString * ref_des, CString * pin_name, BOOL bSetAreas=TRUE );
	// void RemovePin( int pin_index, BOOL bSetAreas=TRUE );

	// connections
	cconnect * AddConnect( int * ic=NULL );
	cconnect * AddConnectFromPin( int p1, int * ic=NULL );
	cconnect * AddConnectFromPinToPin( int p1, int p2, int * ic=NULL );
	cconnect * AddConnectFromTraceVtx( id& vtx_id, int * ic=NULL );
	cconnect * AddRatlineFromVtxToPin( id vtx_id, int pin_index );
	void RemoveConnect( cconnect * c );
	void RemoveConnectAdjustTees( cconnect * c );
	cconnect * SplitConnectAtVtx( id vtx_id );
	void MergeConnections( cconnect * c1, cconnect * c2 );
	void RecreateConnectFromUndo( undo_con * con, undo_seg * seg, undo_vtx * vtx );
	void AdjustTees( int tee_ID );

	// AMW r267 added return values to indicate connection deleted
	bool RemoveSegmentAdjustTees( cseg * s );
	bool RemoveSegAndVertexByIndex( cseg *s );		// Changed arg.  Removes s and its postVtx
	bool RemoveVertexAndSegByIndex( cseg *s );		// Changed arg.  Removes s and its preVtx
	// bool RemoveVertex( cconnect * c, int iv );
	bool RemoveVertex( cvertex * v );
	bool MergeUnroutedSegments( cconnect * c );	// AMW r267 added
	// end AMW
};


/**********************************************************************************************/
/*  OTHERS: ctext, cadhesive, ccentroid                                                       */
/**********************************************************************************************/


// TODO: rename as ctext
class CText: public cpcb_item
{
public:
	// member variables
	int m_x, m_y;
	int m_layer;
	int m_angle;
	int m_mirror;
	BOOL m_bNegative;
	int m_font_size;
	int m_stroke_width;
	int m_nchars;
	CString m_str;
	CArray<stroke> m_stroke;
	CDisplayList * m_dlist;
	dl_element * dl_sel;
	SMFontUtil * m_smfontutil;

	// member functions
	CText();
	~CText();
	CText( CDisplayList * dlist, int x, int y, int angle, 
		BOOL bMirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr );			// Removed selType/selSubtype args.  Will use derived creftext and cvaluetext
																				// classes in place of this business.
	bool IsText() { return true; }
	CText *ToText() { return this; }
	int GetTypeBit() { return bitText; }

	void Init( CDisplayList * dlist, int x, int y, int angle,					// TODO: rethink relationship with constructor. Removed tid arg.
		int mirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr );
	void Draw( CDisplayList * dlist, SMFontUtil * smfontutil );
	void Draw();
	void Undraw();
	void Highlight();
	void StartDragging( CDC * pDC );
	void CancelDragging();
	void Move( int x, int y, int angle, BOOL mirror, BOOL negative, int layer, int size=-1, int w=-1 );
	void Move( int x, int y, int angle, int size=-1, int w=-1);											// CPT added.  Used when moving ref/value texts
	void GetBounds( CRect &br );
};

class creftext: public CText
{
public:
	creftext( CDisplayList * dlist, int x, int y, int angle, 
		BOOL bMirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr ) :
			CText(dlist, x, y, angle, bMirror, bNegative, layer, font_size,
				stroke_width, smfontutil, str_ptr) 
			{ }
	bool IsText() { return false; }
	bool IsRefText() { return true; }
	CText *ToText() { return NULL; }
	creftext *ToRefText() { return this; }
	int GetTypeBit() { return bitRefText; }
};

class cvaluetext: public CText
{
public:
	cvaluetext( CDisplayList * dlist, int x, int y, int angle, 
		BOOL bMirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr ) :
			CText(dlist, x, y, angle, bMirror, bNegative, layer, font_size,
				stroke_width, smfontutil, str_ptr) 
			{ }
	bool IsText() { return false; }
	bool IsValueText() { return true; }
	CText *ToText() { return NULL; }
	cvaluetext *ToValueText() { return this; }
	int GetTypeBit() { return bitValueText; }
};

class ccentroid : public cpcb_item
{
public:
	// Represents centroids within footprints.
	ccentroid() { }
	~ccentroid() { }
	bool IsCentroid() { return true; }
	ccentroid *ToCentroid() { return this; }
	int GetTypeBit() { return bitCentroid; }
}

class cadhesive : public cpcb_item
{
public:
	// Represents adhesive dots within footprints.
	cadhesive() { }
	~cadhesive() { }
	bool IsAdhesive() { return true; }
	cadhesive *ToAdhesive() { return this; }
	int GetTypeBit() { return bitAdhesive; }
}

class cdrc : public cpcb_item
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

	cdrc() { }
	~cdrc() { }
	bool IsDRC() { return true; }
	cdrc *ToDRC() { return this; }
	int GetTypeBit() { return bitDRC; }
}

