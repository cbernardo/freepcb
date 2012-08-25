// Net.h --- header file for classes related most closely to nets, many of which are descendants of cpcb_item:
//  cvertex2, ctee, cseg2, cconnect2, cnet2, cnetlist.  Note that carea2 is dealt with in Polyline.h/.cpp.

#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "PcbItem.h"
#include "DisplayList.h"
#include "Undo.h"
#include "stdafx.h"
#include "Shape.h"
#include "smfontutil.h"
#include "DlgLog.h"
#include "DesignRules.h"

extern int m_layer_by_file_layer[MAX_LAYERS];
#define MAX_NET_NAME_SIZE 39


/**********************************************************************************************/
/*  RELATED TO cseg2/cvertex2/cconnect2                                                          */
/**********************************************************************************************/

// cvertex2: describes a vertex between segments in a connection
class cvertex2: public cpcb_item
{
public:
	enum {
		// Return values for GetViaConnectionStatus().
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
	int force_via_flag;			// force a via even if no layer change
	int via_w, via_hole_w;		// via width and hole width (via_w==0 means no via)
	CArray<dl_element*> dl_els;	// CPT2 renamed;  contains one dl_element per layer
	dl_element * dl_hole;		// hole in via
	dl_element * dl_thermal;	// CPT2 new.  

	cvertex2(cconnect2 *c, int _x=0, int _y=0);				// CPT2 Added args. Done in cpp
	cvertex2(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();											// Done in cpp
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
	void GetTypeStatusStr( CString * str );					// Done in cpp
	cundo_item *MakeUndoItem()
		{ return new cuvertex(this); }

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
	int GetViaConnectionStatus( int layer );									// Done in cpp, derived from CNetList func.
	void GetViaPadInfo( int layer, int *pad_w, int *pad_hole_w, 
		int *connect_status );													// Done in cpp, derived from CNetList func.
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
	ctee(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb() { return vtxs.GetSize()>0; }			// TODO Adequate?
	bool IsTee() { return true; }
	ctee *ToTee() { return this; }
	bool IsVia() { return via_w > 0; }
	int GetTypeBit() 
		{ return via_w? bitVia: bitTee; }
	cnet2 *GetNet() 
		{ return vtxs.GetSize()==0? NULL: vtxs.First()->m_net; }
	int GetLayer();													// Done in cpp
	void GetStatusStr( CString * str );								// Done in cpp
	cundo_item *MakeUndoItem()
		{ return new cutee(this); }
	void SaveUndoInfo();

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
	cnet2 * m_net;				// parent net
	cconnect2 * m_con;			// parent connection
	cvertex2 *preVtx, *postVtx;	// CPT2

	enum Curve { STRAIGHT = 0,
			CCW,
			CW
	};

	cseg2(cconnect2 *c, int _layer, int _width);							// CPT2 added args.  Replaces Initialize().  Done in cpp
	cseg2(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsSeg() { return true; }
	cseg2 *ToSeg() { return this; }
	int GetTypeBit() { return bitSeg; }
	cnet2 *GetNet() { return m_net; }
	cconnect2 *GetConnect() { return m_con; }
	int GetLayer() { return m_layer; }
	cundo_item *MakeUndoItem()
		{ return new cuseg(this); }

	void SetConnect(cconnect2 *c);
	void SetWidth( int w, int via_w, int via_hole_w );			// Done in cpp
	int SetLayer( int _layer );									// Done in cpp, derived from CNetList::ChangeSegmentLayer

	int Draw();													// Done in cpp
	void Highlight( bool bThin );								// Done in cpp (derived from CNetList::HighlightSegment)
	void Highlight()											// CPT2 This form of the function overrides the base-class virtual func.  (Best system?)
		{ Highlight(false); }

	void SetVisible( bool bVis );										// Done in cpp
	int Index();													// CPT2:  maybe, maybe not...
	void GetStatusStr( CString * str, int width );					// CPT added width param
	void GetStatusStr( CString *str ) { GetStatusStr(str, 0); }
	void GetBoundingRect( CRect *br );								// CPT2 new, helper for DRC
	char GetDirectionLabel();										// Done in cpp
	void Divide( cvertex2 *v, cseg2 *s, int dir );					// Done in cpp
	bool InsertSegment(int x, int y, int layer, int width, int dir );  // Done in cpp, derived from CNetList::InsertSegment()
	int Route( int layer, int width );								// Done in cpp
	void Unroute(int dx=1, int dy=1, int end=0);					// Done in cpp
	bool UnrouteWithoutMerge(int dx=1, int dy=1, int end=0);		// Done in cpp
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
public:
	enum Dir {
		ROUTE_FORWARD = 0,
		ROUTE_BACKWARD
	};
	enum {
		NO_END = -1				// Leftover from the old architecture, only used now when reading in nets from files 
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
	cconnect2(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsConnect() { return true; }
	cconnect2 *ToConnect() { return this; }
	int GetTypeBit() { return bitConnect; }			// Rarely used since connects don't have selector elements.
	cnet2 *GetNet() { return m_net; }
	cconnect2 *GetConnect() { return this; }
	void GetStatusStr( CString * str );
	cundo_item *MakeUndoItem()
		{ return new cuconnect(this); }
	void SaveUndoInfo();

	// get info about start/ending pins
	cpin2 * StartPin() { return head->pin; }
	cpin2 * EndPin() { return tail->pin; }
	BOOL TestHitOnEndPad( int x, int y, int layer, int dir );	// CPT2 TODO:  adapt from old CNetList::TestHitOnConnectionEndPad

	// get info about vertices
	int NumVtxs() { return vtxs.GetSize(); }
	cvertex2 *VtxByIndex( int iv );								// CPT2: maybe, maybe not...  changed from cvertex2& ret-val to cvertex2*
	int VtxIndexByPtr( cvertex2 * v );							// CPT2: maybe, maybe not...

	// get info about segments
	int NumSegs() { return segs.GetSize(); }
	cseg2 *FirstSeg() { return head->postSeg; }
	cseg2 *LastSeg() { return tail->preSeg; }
	cseg2 *SegByIndex( int is );									// CPT2: maybe, maybe not....
	int SegIndexByPtr( cseg2 * s );									// CPT2: ditto

	void SetWidth( int w, int via_w, int via_hole_w );				// Done in cpp

	// modify segments and vertices. 
	void Start( cvertex2 *v );										// CPT2 New.  Done in cpp.
	void Remove();													// Done in cpp
	void AppendSegAndVertex( cseg2 *s, cvertex2 *v, cvertex2 *after) ;			// Done in cpp
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


// these definitions are for use within cnet2::ImportRouting()  (part of ImportSessionFile()).

enum NODE_TYPE { NONE, NPIN, NVIA, NJUNCTION };

class cnode 
{
public:
	NODE_TYPE type;
	int x, y, layer, via_w;
	cpin2 *pin;
	ctee *tee;
	CArray<int> path_index;
	CArray<int> path_end;  // 0 or 1

	cnode()
		{ pin = NULL; tee = NULL; }
};

class cpath_pt 
{
public:
	int x, y, inode;
};

class cpath 
{
public:
	int layer, width;
	CArray<cpath_pt> pt;
	// int n_used;		// number of times used
	bool bUsed;

	int GetInode( int iend )
	{ 
		// return inode at end of path
		int last_pt = pt.GetSize()-1;
		if(iend)
			return pt[last_pt].inode; 
		else 
			return pt[0].inode; 
	};
};
//
// end definitions for ImportSessionFile()

// cnet2: describes a net
class cnet2: public cpcb_item
{
public:
	CString name;				// net name
	carray<cconnect2> connects;	// array of pointers to connections.  Type change
	carray<cpin2> pins;			// array of pins
	carray<carea2> areas;		// array of copper areas
	carray<ctee> tees;			// Used when reading files (and the like), also by Draw().  Used to be in cnetlist
	int def_w;					// default trace width
	int def_via_w;				// default via width
	int def_via_hole_w;			// default via hole width
	bool bVisible;				// FALSE to hide ratlines and make unselectable.  CPT2 TODO put in base class?
	cnetlist * m_nlist;			// parent netlist

	cnet2( cnetlist *_nlist, CString _name, int _def_w, int _def_via_w, int _def_via_hole_w );
	cnet2( CFreePcbDoc *_doc, int _uid ); 

	virtual bool IsOnPcb();
	bool IsNet() { return true; }
	cnet2 *ToNet() { return this; }
	cnet2 *GetNet() { return this; }
	int GetTypeBit() { return bitNet; }
	void GetStatusStr( CString * str );
	cundo_item *MakeUndoItem()
		{ return new cunet(this); }
	enum { SAVE_ALL, SAVE_CONNECTS, SAVE_AREAS, SAVE_NET_ONLY };		// Args for SaveUndoInfo()
	void SaveUndoInfo( int mode = SAVE_ALL );

	int Draw();														// CPT2 new:  draws all connects and areas.  Done in cpp
	void Undraw();													// CPT2 new, analogous.  Done in cpp
	void Highlight(cpcb_item *exclude);								// Done in cpp
	void Highlight() { Highlight(NULL); }							// Also overrides base-class func.
	bool GetVisible() { return bVisible; }
	void SetVisible( bool _bVis );									// Done in cpp

	// pins
	int NumPins() { return pins.GetSize(); }
	cvertex2 *TestHitOnVertex(int layer, int x, int y);				// Done in cpp, derived from old CNetList function
	// connections
	int NumCons() { return connects.GetSize(); }
	// copper areas
	int NumAreas() { return areas.GetSize(); }
	carea2 *NetAreaFromPoint( int x, int y, int layer );							// CPT2 replaces CNetList::TestPointInArea

	// methods that edit objects
	void MarkConstituents(int util);												// Set utility on all constituents (connects, vtxs, segs, tees, areas).
	void Remove();																		// Done in cpp.
	void MergeWith( cnet2 *n2 );														// Done in cpp.
	// pins
	cpin2 *AddPin( CString * ref_des, CString * pin_name );								// Done in cpp.  Removed bSetAreas param
	void AddPin( cpin2 *pin );															// Done in cpp
	void RemovePin( cpin2 *pin );														// CPT2
	void SetWidth( int w, int via_w, int via_hole_w );									// Done in cpp
	void GetWidth( int *w, int *via_w=NULL, int *via_hole_w=NULL);						// Done in cpp, derived from old CNetList::GetWidths()
	void CalcViaWidths(int w, int *via_w, int *via_hole_w);								// Done in cpp
	void SetThermals();																	// Done in cpp
	// connections
	void AddConnect( cconnect2 *c );													// Done in cpp
	void CleanUpConnections( CString * logstr=NULL );									// Done in cpp, derived from old CNetList func.
	void OptimizeConnections(BOOL bLimitPinCount=TRUE, BOOL bVisibleNetsOnly=TRUE );	// Done in cpp, derived from old CNetList func.
	void ImportRouting( CArray<cnode> * nodes, CArray<cpath> * paths,					// Done in cpp, derived from old CNetList::ImportNetRouting
							int tolerance, CDlgLog * log, bool bVerbose );

};



// net_info structure
// used as a temporary copy of net info for editing in dialogs
// or importing/exporting netlists
struct net_info {
	CString name;
	cnet2 * net;
	BOOL visible;
	int w;
	int v_w;
	int v_h_w;
	BOOL apply_trace_width;
	BOOL apply_via_width;
	BOOL deleted;
	int merge_into;					// CPT2 new, allows for combining nets from the DlgNetlist.
	BOOL modified;
	CArray<CString> ref_des;
	CArray<CString> pin_name;
};

// netlist_info is an array of net_info for each net
typedef CArray<net_info> netlist_info;

// Class cnetlist.  Basically a souped-up carray<cnet2>.
class cnetlist  
{
public:
	carray<cnet2> nets;	
	CFreePcbDoc *m_doc;
	CDisplayList * m_dlist;
	cpartlist * m_plist;
	int m_layers;											// number of copper layers.  CPT2 TODO probably dump (look at m_doc->m_num_copper_layers)
	int m_def_w, m_def_via_w, m_def_via_hole_w;
	int m_annular_ring;
	BOOL m_bSMT_connect;

	cnetlist( CFreePcbDoc * _doc ) ;													// CPT2:  Changed param

	void SetNumCopperLayers( int layers ) { m_layers = layers; }
	int GetNumCopperLayers() { return m_layers; }
	void SetWidths( int w, int via_w, int via_hole_w ) 
		{ m_def_w = w, m_def_via_w = via_w, m_def_via_hole_w = via_hole_w; }
	void SetViaAnnularRing( int ring ) { m_annular_ring = ring; }
	void SetSMTconnect( BOOL bSMTconnect ) { m_bSMT_connect = bSMTconnect; }

	void MarkAllNets( int utility );													// Done in cpp
	void MoveOrigin( int dx, int dy );													// Done in cpp
	cnet2 * GetNetPtrByName( CString * name )
	{
		citer<cnet2> in (&nets);
		for (cnet2 *n = in.First(); n; n = in.Next())
			if (n->name == *name) return n;
		return NULL;
	}
	int CheckNetlist( CString * logstr );
	int CheckConnectivity( CString * logstr );
	BOOL GetNetBoundaries( CRect * r );											// Done in cpp

	void OptimizeConnections( BOOL bLimitPinCount=TRUE, BOOL bVisibleNetsOnly=TRUE ); // Done in cpp
	void CleanUpAllConnections( CString * logstr=NULL );

	// functions related to parts. CPT2 TODO think these through more
	void SwapPins( cpart2 * part1, CString * pin_name1,
						cpart2 * part2, CString * pin_name2 );								// CPT2 TODO.  'S' key was disabled in allan_uids_new_drawing branch.
																							// Bring it back some time?
	void SetThermals();																		// CPT2 new.

	// I/O  functions
	void ReadNets( CStdioFile * pcb_file, double read_version, int * layers=NULL );			// Done in cpp.
	void WriteNets( CStdioFile * file );													// Done in cpp.
	void ExportNetListInfo( netlist_info * nli );
	void ImportNetListInfo( netlist_info * nli, int flags, CDlgLog * log,
		int def_w, int def_w_v, int def_w_v_h );
	void ReassignCopperLayers( int n_new_layers, int * layer );
};

