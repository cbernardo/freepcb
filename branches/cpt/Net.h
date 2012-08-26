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
	cseg2 *preSeg, *postSeg;	// Succeeding and following segs.  If either is null, this is an end seg.
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

	cvertex2(cconnect2 *c, int _x=0, int _y=0);				// CPT2 Added args
	cvertex2(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsVertex() { return true; }
	bool IsVia();
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
	int GetLayer();
	void GetStatusStr( CString * str );
	void GetTypeStatusStr( CString * str );
	cundo_item *MakeUndoItem()
		{ return new cuvertex(this); }

	int Draw();
	void Undraw();
	void Highlight();
	void SetVisible( bool bVis );

	bool IsLooseEnd();
	bool Remove();
	void SetConnect(cconnect2 *c);
	void ForceVia();
	void UnforceVia();
	bool IsViaNeeded();
	void SetViaWidth();
	void ReconcileVia();
	bool TestHit(int x, int y, int layer);
	bool IsConnectedToArea( carea2 * a );
	int GetConnectedAreas( carray<carea2> *a=NULL );
	cconnect2 * SplitConnect();
	cseg2 * AddRatlineToPin( cpin2 *pin );
	void StartDraggingStub( CDC * pDC, int x, int y, int layer1, int w,	
							int layer_no_via, int crosshair, int inflection_mode );

	void StartDragging( CDC * pDC, int x, int y, int crosshair = 1 );
	void CancelDragging();
	void Move( int x, int y );
	int GetViaConnectionStatus( int layer );
	void GetViaPadInfo( int layer, int *pad_w, int *pad_hole_w, 
		int *connect_status );
	bool SetNeedsThermal();														// CPT2 new.  Sets the bNeedsThermal flag, depending on net areas
																				// that overlap this point.
};

class ctee: public cpcb_item 
{
	// CPT2 new.  Basically a carray<cvertex2>, indicating the vertices that together join up to form a single tee junction.
public:
	carray<cvertex2> vtxs;
	int via_w, via_hole_w;					// These will be the max of the values in the constituent vtxs.
	CArray<dl_element*> dl_els;				// Tee will be drawn if it's a via.
	dl_element * dl_hole;
	dl_element * dl_thermal;				// CPT2 new.

	ctee(cnet2 *n);
	ctee(CFreePcbDoc *_doc, int _uid);

	bool IsTee() { return true; }
	ctee *ToTee() { return this; }
	bool IsVia() { return via_w > 0; }
	int GetTypeBit() 
		{ return via_w? bitVia: bitTee; }
	cnet2 *GetNet() 
		{ return vtxs.GetSize()==0? NULL: vtxs.First()->m_net; }
	virtual bool IsOnPcb();
	int GetLayer();
	void GetStatusStr( CString * str );
	cundo_item *MakeUndoItem()
		{ return new cutee(this); }
	void SaveUndoInfo();

	int Draw();
	void Undraw();
	void Highlight();
	void StartDragging( CDC * pDC, int x, int y, int crosshair );
	void CancelDragging();
	void SetVisible( bool bVis );

	void Remove(bool bRemoveVtxs = false);
	void Add(cvertex2 *v);
	bool Adjust();
	void ForceVia();
	void UnforceVia();
	bool IsViaNeeded();
	void ReconcileVia();
	void Remove(cvertex2 *v, bool fAdjust = true);
	void Move(int x, int y);
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

	cseg2(cconnect2 *c, int _layer, int _width);							// CPT2 added args.  Replaces Initialize()
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
	void SetWidth( int w, int via_w, int via_hole_w );
	int SetLayer( int _layer );

	int Draw();	
	void Highlight( bool bThin );
	void Highlight()											// CPT2 This form of the function overrides the base-class virtual func.  (Best system?)
		{ Highlight(false); }

	void SetVisible( bool bVis );
	void GetStatusStr( CString * str, int width );
	void GetStatusStr( CString *str ) { GetStatusStr(str, 0); }
	void GetBoundingRect( CRect *br );								// CPT2 new, helper for DRC
	char GetDirectionLabel();
	void Divide( cvertex2 *v, cseg2 *s, int dir );
	bool InsertSegment(int x, int y, int layer, int width, int dir );
	int Route( int layer, int width );
	void Unroute(int dx=1, int dy=1, int end=0);
	bool UnrouteWithoutMerge(int dx=1, int dy=1, int end=0);
	bool RemoveMerge(int end);
	bool RemoveBreak();

	void StartDragging( CDC * pDC, int x, int y, int layer1, int layer2, int w,
						int layer_no_via, int dir, int crosshair = 1 );
	void CancelDragging();
	void StartDraggingNewVertex( CDC * pDC, int x, int y, int layer, int w, int crosshair );
	void CancelDraggingNewVertex( );
	void StartMoving( CDC * pDC, int x, int y, int crosshair );
	void CancelMoving();
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

	// these params used only by DRC
	int min_x, max_x;			// bounding rect
	int min_y, max_y;
	BOOL vias_present;			// flag to indicate that vias are present. 
	int seg_layers;				// mask for all layers used by segments.

// methods
public:
	// general
	cconnect2( cnet2 * _net );
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

	// drawing methods
	int Draw();
	void Undraw();
	void Highlight();

	// get info about vertices
	int NumVtxs() { return vtxs.GetSize(); }
	cpin2 * StartPin() { return head->pin; }
	cpin2 * EndPin() { return tail->pin; }

	// get info about segments
	int NumSegs() { return segs.GetSize(); }
	cseg2 *FirstSeg() { return head->postSeg; }
	cseg2 *LastSeg() { return tail->preSeg; }

	// modify segments and vertices. 
	void Start( cvertex2 *v );
	void Remove();
	void AppendSegAndVertex( cseg2 *s, cvertex2 *v, cvertex2 *after) ;
	void PrependVertexAndSeg( cvertex2 *v, cseg2 *s );
	void AppendSegment( int x, int y, int layer, int width );
	void PrependSegment( int x, int y, int layer, int width );
	void ReverseDirection();
	void CombineWith( cconnect2 *c2, cvertex2 *v1, cvertex2 *v2);
	void MergeUnroutedSegments();
	void SetWidth( int w, int via_w, int via_hole_w );
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

	int Draw();														// CPT2 new:  draws all connects and areas.
	void Undraw();													// CPT2 new, analogous.
	void Highlight(cpcb_item *exclude);
	void Highlight() { Highlight(NULL); }							// Also overrides base-class func.
	bool GetVisible() { return bVisible; }
	void SetVisible( bool _bVis );

	// pins
	int NumPins() { return pins.GetSize(); }
	cvertex2 *TestHitOnVertex(int layer, int x, int y);
	// connections
	int NumCons() { return connects.GetSize(); }
	// copper areas
	int NumAreas() { return areas.GetSize(); }
	carea2 *NetAreaFromPoint( int x, int y, int layer );

	// methods that edit objects
	void MarkConstituents(int util);								// Set utility on all constituents (connects, vtxs, segs, tees, areas).
	void Remove();
	void MergeWith( cnet2 *n2 );
	// pins
	cpin2 *AddPin( CString * ref_des, CString * pin_name );
	void AddPin( cpin2 *pin );
	void RemovePin( cpin2 *pin );
	void SetWidth( int w, int via_w, int via_hole_w );
	void GetWidth( int *w, int *via_w=NULL, int *via_hole_w=NULL);
	void CalcViaWidths(int w, int *via_w, int *via_hole_w);
	void SetThermals();
	// connections
	void AddConnect( cconnect2 *c );
	void CleanUpConnections( CString * logstr=NULL );
	void OptimizeConnections(BOOL bLimitPinCount=TRUE, BOOL bVisibleNetsOnly=TRUE );
	void ImportRouting( CArray<cnode> * nodes, CArray<cpath> * paths,
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
	int m_def_w, m_def_via_w, m_def_via_hole_w;
	BOOL m_bSMT_connect;

	cnetlist( CFreePcbDoc * _doc ) ;

	void SetWidths( int w, int via_w, int via_hole_w ) 
		{ m_def_w = w, m_def_via_w = via_w, m_def_via_hole_w = via_hole_w; }
	void SetSMTconnect( BOOL bSMTconnect ) { m_bSMT_connect = bSMTconnect; }

	void MarkAllNets( int utility );
	void MoveOrigin( int dx, int dy );
	cnet2 * GetNetPtrByName( CString * name )
	{
		citer<cnet2> in (&nets);
		for (cnet2 *n = in.First(); n; n = in.Next())
			if (n->name == *name) return n;
		return NULL;
	}
	int CheckNetlist( CString * logstr );
	int CheckConnectivity( CString * logstr );
	BOOL GetNetBoundaries( CRect * r );

	void OptimizeConnections( BOOL bLimitPinCount=TRUE, BOOL bVisibleNetsOnly=TRUE );
	void CleanUpAllConnections( CString * logstr=NULL );

	void SwapPins( cpart2 * part1, CString * pin_name1,
						cpart2 * part2, CString * pin_name2 );								// CPT2 TODO.  'S' key was disabled in allan_uids_new_drawing branch.
																							// Bring it back some time?
	void SetThermals();																		// CPT2 new.

	// I/O  functions
	void ReadNets( CStdioFile * pcb_file, double read_version, int * layers=NULL );
	void WriteNets( CStdioFile * file );
	void ExportNetListInfo( netlist_info * nli );
	void ImportNetListInfo( netlist_info * nli, int flags, CDlgLog * log,
		int def_w, int def_w_v, int def_w_v_h );
	void ReassignCopperLayers( int n_new_layers, int * layer );
};

