// Net.h: interface for the the following classes:
//	cpin - a pin in a net (eg. U1.6)
//	cconnect - a connection in a net, usually a trace between pins
//	cseg - a segment of a connection, either copper or a ratline
//	cvertex - a vertex in a connection (ie. a pin or a point between segments)
//	carea - a copper area in a net
//

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

class cconnect;
class CVertex;
class cvertex;
class cseg;
class cpin;
class carea;
class undo_con;
class undo_seg;
class undo_vtx;

class cpcb_item;
class carray_link;
template <class T> class carray;  // NB "T" is expected to be a cpcb_item class
template <class T> class citer;	  // Ditto

class cpcb_item 
{
	// Best way I could figure out to give the various carray types access to cpcb_item's innards was to list 'em as friends 1 by 1.
	// An ugly PITA, but I suppose necessary:
	friend class carray<cconnect>;
	friend class carray<cvertex>;
	friend class carray<cseg>;
	friend class carray<carea>;
	friend class carray<cpin>;
	friend class carray<carea>;

	carray_link *carray_list;
protected:
	cpcb_item() { carray_list = NULL; }
	~cpcb_item();								// Had to put this in Net.cpp.  When an item is destroyed, references to it are automatically
												// removed from any carrays to which it belongs.
};

/*
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

// cpin: describes a pin in a net
class cpin : public cpcb_item
{
public:
	cpin();
	cpin( cpin& src );
	~cpin();
	cpin& operator=( const cpin& rhs );
	cpin& operator=( cpin& rhs );

	void Initialize( cnet * net );
	int Index();				// index of pin in array
	int UID();					// unique identifier			
	part_pin * GetPartPin();	// pointer to this pin on part
	id GetPartPinId();			// id of part pin

	int m_uid;			// unique identifier
	CString ref_des;	// reference designator such as 'U1'
	CString pin_name;	// pin name such as "1" or "A23"
	cpart * part;		// pointer to part containing the pin
	cnet * m_net;		// parent net
	int utility;		// flag for various purposes
};

// carea: describes a copper area in a net
class carea : public CPolyLine
{
public:
	carea();
	carea( const carea& source );		// dummy copy constructor
	~carea();						
	carea &operator=( carea &a );		// dummy assignment operator
	carea &operator=( const carea &a );	// dummy assignment operator

	void Initialize( CDisplayList * dlist, cnet * net );
	int UID(){ return m_uid; };
	void SetUID( int uid ){ m_uid = uid; };
	id Id();

	int NumVertices();
	cvertex * VtxByIndex( int iv );
	int NumPins();
	cpin * PinByIndex( int ip );

	cnet * m_net;		// parent net
	int npins;			// number of thru-hole pins within area on same net
	CArray<int> pin;	// array of thru-hole pins
	CArray<dl_element*> dl_thermal;	// graphics for thermals on pins
	int nvias;			// number of via connections to area
	CArray<int> vcon;	// connections 
	CArray<int> vtx;	// vertices
	CArray<dl_element*> dl_via_thermal; // graphics for thermals on stubs
	int utility, utility2;

};

// cseg: describes a segment of a connection
class cseg
{
public:
	enum Curve { STRAIGHT = 0,
			CCW,
			CW
	};
	cseg();
	~cseg();
	cseg( cseg& src );		// copy constructor
	cseg& operator=( cseg& rhs );	// assignment
	cseg& operator=( const cseg& rhs );	// assignment

	void Initialize( cconnect * c );

	void Draw();
	void Undraw();
	BOOL IsDrawn();

	void SetVisibility( int vis );
	id Id();
	int UID(){ return m_uid; };
	int Index();
	cvertex& GetPreVtx();
	cvertex& GetPostVtx();
	void GetStatusStr( CString * str, int width = 0 );			// CPT added width param

	static int m_array_step;
	int m_uid;				// unique id
	int m_layer;			// copper layer
	int m_width;			// width
	int m_curve;
	int m_selected;			// 1 if selected for editing
//private:
	dl_element * dl_el;		// display element for segment
	dl_element * dl_sel;	// selection line
public:
	CDisplayList * m_dlist;
	cnet * m_net;			// parent net
	cconnect * m_con;		// parent connection
	int utility;
};

// cvertex: describes a vertex between segments 
// in a connection
class cvertex
{
public:
	// types of vertices
	enum VType { 
		V_ERR = 0,	// unknown
		V_PIN,		// pin at end of trace
		V_TRACE,	// vertex between trace segments
		V_END,		// end vertex of stub trace
		V_TEE,		// T-vertex (defined here)
		V_SLAVE		// T-vertex (defined in another trace)
	};	

	cvertex();
	cvertex( cvertex& src );
	~cvertex();
	cvertex &operator=( const cvertex &v ); // assignment	
	cvertex &operator=( cvertex &v );		// assignment
	void Initialize( cconnect * c );

	int UID();
	id Id();
	int Index();

	void Draw();
	void Undraw();
	void Highlight();

	VType GetType();
	cpin * GetNetPin();
	int GetNetPinIndex();
	void GetTypeStatusStr( CString * str );
	void GetStatusStr( CString * str );

	bool IsConnectedToArea( carea * a );
	int GetConnectedAreas( CArray<int> * a=NULL );

	int m_uid;					// unique id
	int x, y;					// coords
	int pad_layer;				// layer of pad if this is first or last vertex, otherwise 0
	int force_via_flag;			// force a via even if no layer change
	int via_w, via_hole_w;		// via width and hole width (via_w==0 means no via)
	int tee_ID;					// used to flag a t-connection point
	int utility, utility2;		// used for various functions
	CArray<dl_element*> dl_el;	// array of display elements for each layer
	dl_element * dl_sel;		// selection box
	dl_element * dl_hole;		// hole in via
	cconnect * m_con;			// parent connection
	CDisplayList * m_dlist;		// NULL if not drawable
};

// cconnect: describes a connection between two pins 
// or a stub trace with no end pin
class cconnect
{
	friend class CNetList;
	friend class cnet;
public:
	enum {
		NO_END = -1		// used for end_pin if stub trace
	};
	enum Dir {
		ROUTE_FORWARD = 0,
		ROUTE_BACKWARD
	};

// methods
public:
	// general
	cconnect( cnet * net );
	~cconnect();

	int UID(){ return m_uid; };
	void ClearArrays();
	void GetStatusStr( CString * str );
	id Id();
	int Index();

	// get info about start/ending pins
	cpin * StartPin();
	cpin * EndPin();

	// get info about vertices
	int NumVtxs(){ return vtx.GetSize(); };
	cvertex * FirstVtx();
	cvertex * LastVtx();
	cvertex * VtxByUID( int uid, int * index=NULL );
	cvertex& VtxByIndex( int iv );
	int VtxIndexByPtr( cvertex * v );
	int VtxUIDByPtr( cvertex * v );

	// get info about segments
	int NumSegs();
	void SetNumSegs( int n );
	cseg & FirstSeg();
	cseg & LastSeg();
	cseg * SegByUID( int uid, int * index=NULL );
	cseg & SegByIndex( int is );
	int SegIndexByPtr( cseg * s );
	int SegUIDByPtr( cseg * s );

	// modify segments and vertices
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

	// utilities
	int Utility(){ return utility; };
	void SetUtility( int i ){ utility = i; };

	// member variables
	int locked;					// 1 if locked (will not be optimized away)
	int start_pin, end_pin;		// indexes into net.pin array
private:
	CArray<cseg> seg;		// array of segments
	CArray<cvertex> vtx;	// array of vertices
public:
	int utility;	// used as a flag for various operations
public:
	// these params used only by DRC
	int min_x, max_x;			// bounding rect
	int min_y, max_y;
	BOOL vias_present;			// flag to indicate that vias are pesent
	int seg_layers;				// mask for all layers used by segments
	// new stuff
	int m_uid;					// unique id
	cnet * m_net;				// parent net
	BOOL m_bDrawn;				// drawn into display list or not
	CDisplayList * m_dlist;		
};

// cnet: describes a net
class cnet
{
	friend class CNetList;
	friend class CIterator_cconnect;
public:
	cnet( CDisplayList * dlist, CNetList * nlist );
	~cnet();

// methods that return data
	// general
	int UID(){ return m_uid; };
	id Id(){ return id(ID_NET, m_uid);};
	void GetStatusStr( CString * str );

	// pins
	int NumPins();
	cpin * PinByIndex( int ip );
	cpin * PinByUID( int uid, int * index );
	void SetVertexToPin( int ip, cvertex * v );

	// connections
	int NumCons();
	cconnect * ConByIndex( int ic );
	cconnect * ConByUID( int uid, int * index=NULL );
	int ConIndexByPtr( cconnect * c );

	// copper areas
	int NumAreas();
	carea * AreaByUID( int uid, int * index=NULL );
	carea * AreaByIndex( int ia );

// methods that edit object
	// pins
	void AddPin( CString * ref_des, CString * pin_name, BOOL set_areas=TRUE );
	void RemovePinByUID( int uid, BOOL bSetAreas=TRUE );
	void RemovePin( CString * ref_des, CString * pin_name, BOOL bSetAreas=TRUE );
	void RemovePin( int pin_index, BOOL bSetAreas=TRUE );

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
	// segments and vertices
	bool RemoveSegmentAdjustTees( cseg * s );
	bool RemoveSegAndVertexByIndex( cconnect * c, int is );
	bool RemoveVertexAndSegByIndex( cconnect * c, int is );
	bool RemoveVertex( cconnect * c, int iv );
	bool RemoveVertex( cvertex * v );
	bool MergeUnroutedSegments( cconnect * c );	// AMW r267 added
	// end AMW

// member variables
	id m_id;				// net id
	int m_uid;
	CString name;		// net name
private:
	CArray<cconnect*> connect;	// array of pointers to connections
public:
	CArray<cpin> pin;			// array of pins
	CArray<carea,carea> area;	// array of copper areas
	int def_w;					// default trace width
	int def_via_w;				// default via width
	int def_via_hole_w;			// default via hole width
	BOOL visible;				// FALSE to hide ratlines and make unselectable
	int utility;				// used to keep track of which nets have been optimized
	int utility2;				// used to keep track of which nets have been optimized
	CDisplayList * m_dlist;		// CDisplayList to use
	CNetList * m_nlist;			// parent netlist
	// new stuff, for testing
	CMap<int,int,CVertex*,CVertex*> m_vertex_map;
};

