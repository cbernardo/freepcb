// NetList.h: interface for the CNetList class.
//
// This is a basically a map of nets
//	- each net is represented by an cnet object
//	- each cnet is mapped by its name
//	- each cnet contains arrays of pins, connections between pins, and copper areas
//	- each pin is represented by a cpin object
//	- each connection is represented by a cconnect object 
//  - each cconnect contains arrays of segments and vertices between segments 
//	- each segment is represented by a cseg object
//	- each vertex is represented by a cvertex object
//	- each copper area is represented by a carea object
//
////////////////////////////////////////////////////////////////////// 

#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "DisplayList.h"
#include "PartList.h"
#include "PolyLine.h"
#include "LinkList.h"
#include "Cuid.h"
#include "Net.h"

extern int m_layer_by_file_layer[MAX_LAYERS];

class CNetList;
class cvertex;
class cconnect;
class CIterator_cconnect;
class CFreePcbDoc;					// CPT

#define MAX_NET_NAME_SIZE 39

// these definitions are for ImportSessionFile()
//
enum NODE_TYPE { NONE, NPIN, NVIA, NJUNCTION };

typedef class {
public:
	NODE_TYPE type;
	int x, y, layer, via_w;
	BOOL bUsed;
	int pin_index;	// if type == NPIN
	CArray<int> path_index;
	CArray<int> path_end;  // 0 or 1
} cnode;

typedef class {
public:
	int x, y, inode;
} cpath_pt;

typedef class {
public:
	// return inode at end of path
	int GetInode( int iend )
	{ 
		int last_pt = pt.GetSize()-1;
		if(iend)
			return pt[last_pt].inode; 
		else 
			return pt[0].inode; 
	};
	// member variables
	int layer, width;
	CArray<cpath_pt> pt;
	int n_used;		// number of times used
} cpath;
//
// end definitions for ImportSessionFile()


// these structures are used for undoing 
struct undo_pin {
	char ref_des[MAX_REF_DES_SIZE+1];
	char pin_name[CShape::MAX_PIN_NAME_SIZE+1];
};

struct undo_area {
	int size;
	cnetlist * nlist;
	char net_name[MAX_NET_NAME_SIZE+1];
	int iarea;
	id m_id;
};

struct undo_seg {
	int uid;
	int layer;				// copper layer
	int width;				// width
	int via_w, via_hole_w;	// via width and hole width
};

struct undo_vtx {
	int uid;
	int x, y;				// coords
	int pad_layer;			// layer of pad if this is first or last vertex
	int force_via_flag;		// force a via even if no layer change
	int tee_ID;				// identifier for t-connection
	int via_w, via_hole_w;	// via width and hole width (via_w==0 means no via)
};

struct undo_con {
	int uid;
	int size;
	cnetlist * nlist;
	char net_name[MAX_NET_NAME_SIZE+1];
	int start_pin, end_pin;		// indexes into net.pin array
	int nsegs;					// # elements in seg array
	int locked;					// 1 if locked (will not be optimized away)
	int set_areas_flag;			// 1 to force setting of copper areas
	int seg_offset;				// offset to array of segments
	int vtx_offset;				// offset to array of vertices
	// array of undo_seg structs goes here
	// followed by array of undo_vtx structs
};

struct undo_net {
	int size;
	cnetlist * nlist;
	char name[MAX_NET_NAME_SIZE+1];
	int npins;
	int m_uid;
	// id m_id; // CPT2 removed
	// array of undo_pin structs start here
};

// net_info structure
// used as a temporary copy of net info for editing in dialogs
// or importing/exporting netlists
struct net_info {
	CString name;
	cnet2 * net;					// CPT2 changed to cnet2*
	BOOL visible;
	int w;
	int v_w;
	int v_h_w;
	BOOL apply_trace_width;
	BOOL apply_via_width;
	BOOL deleted;
	BOOL modified;
	CArray<CString> ref_des;
	CArray<CString> pin_name;
};

// netlist_info is an array of net_info for each net
typedef CArray<net_info> netlist_info;


// CNetlist
class CNetList  
{
	friend class cnet;
public:
	enum{ MAX_ITERATORS=10 };
	enum {
		VIA_NO_CONNECT = 0,
		VIA_TRACE = 1,
		VIA_AREA = 2
	};
	enum {						// used for UNDO records
		UNDO_CONNECT_MODIFY=1,	// undo modify connection
		UNDO_AREA_CLEAR_ALL,	// flag to remove all areas
		UNDO_AREA_ADD,			// undo add area (i.e. delete area)
		UNDO_AREA_MODIFY,		// undo modify area
		UNDO_AREA_DELETE,		// undo delete area (i.e. add area) 
		UNDO_NET_ADD,			// undo add net (i.e delete net)
		UNDO_NET_MODIFY,		// undo modify net
		UNDO_NET_OPTIMIZE		// flag to optimize net on undo
	};
	CNetList( CDisplayList * dlist, CPartList * plist, CFreePcbDoc * doc );				// CPT:  added doc parameter
	~CNetList();
	void SetNumCopperLayers( int layers ){ m_layers = layers;};
	int GetNumCopperLayers(){ return m_layers;};
	void SetWidths( int w, int via_w, int via_hole_w );
	void SetViaAnnularRing( int ring ){ m_annular_ring = ring; };
	void SetSMTconnect( BOOL bSMTconnect ){ m_bSMT_connect = bSMTconnect; };

	// functions for nets and pins
	void MarkAllNets( int utility );
	void MoveOrigin( int x_off, int y_off );
	cnet * GetNetPtrByName( CString * name );
	cnet * GetNetPtrByUID( int uid );
	cnet * AddNet( CString name, int max_pins, int def_width, int def_via_w, int def_via_hole_w );
	void RemoveNet( cnet * net );
	void RemoveAllNets();
	void RemoveNetPin( cpart * part, CString * pin_name, BOOL bSetAreas=TRUE );
	void DisconnectNetPin( cpart * part, CString * pin_name, BOOL bSetAreas=TRUE );
	void DisconnectNetPin( cnet * net, CString * ref_des, CString * pin_name, BOOL bSetAreas=TRUE );
	int GetNetPinIndex( cnet * net, CString * ref_des, CString * pin_name );
	int SetNetWidth( cnet * net, int w, int via_w, int via_hole_w );
	void SetNetVisibility( cnet * net, BOOL visible );
	BOOL GetNetVisibility( cnet * net );
	int CheckNetlist( CString * logstr );
	int CheckConnectivity( CString * logstr );
	void HighlightNetConnections( cnet * net, id * exclude_id=NULL );
	void HighlightNet( cnet * net, id * exclude_id=NULL );
	void GetWidths( cnet * net, int * w, int * via_w, int * via_hole_w );
	BOOL GetNetBoundaries( CRect * r );

	// functions for connections
	int RemoveNetConnect( cnet * net, int ic, BOOL set_areas=TRUE );
	int UnrouteNetConnect( cnet * net, int ic );
	int SetConnectionWidth( cnet * net, int ic, int w, int via_w, int via_hole_w );
	void OptimizeConnections( BOOL bBelowPinCount=FALSE, int pin_count=0, BOOL bVisibleNetsOnly=TRUE );
	int OptimizeConnections( cnet * net, int ic, BOOL bBelowPinCount, int pin_count, BOOL bVisibleNetsOnly=TRUE );
	void OptimizeConnections( cpart * part, BOOL bBelowPinCount, int pin_count, BOOL bVisibleNetsOnly=TRUE );
	BOOL TestHitOnConnectionEndPad( int x, int y, cnet * net, int ic, int layer, int dir );
	int TestHitOnAnyPadInNet( int x, int y, int layer, cnet * net );
	void ChangeConnectionPin( cnet * net, int ic, int end_flag, 
		cpart * part, CString * pin_name );
	void HighlightConnection( cnet * net, int ic, id * exclude_id=NULL );
	void CleanUpConnections( cnet * net, CString * logstr=NULL );
	void CleanUpAllConnections( CString * logstr=NULL );

	// functions for segments.
	// CPT r295 note that via_w and via_hole_w params are NO LONGER USED.
	int AppendSegment( cnet * net, int ic, int x, int y, int layer, int width );
	int InsertSegment( cnet * net, int ic, int iseg, int x, int y, int layer, int width, int dir );
	id  UnrouteSegment( cnet * net, int ic, int iseg, int dx=1, int dy=1, int end=0 );
	void UnrouteSegmentWithoutMerge( cnet * net, int ic, int iseg, double dx=1, double dy=1, int end=0 );
	id MergeUnroutedSegments( cnet * net, int ic );
	int RouteSegment( cnet * net, int ic, int iseg, int layer, int width );
	void RemoveSegment( cnet * net, int ic, int iseg, BOOL bHandleTees=FALSE, BOOL bSetAreaConnections=TRUE );							 
	int ChangeSegmentLayer( cnet * net, int ic, int iseg, int layer );							 
	int SetSegmentWidth( cnet * net, int ic, int is, int w, int via_w, int via_hole_w );
	void HighlightSegment( cnet * net, int ic, int iseg, bool bThin=FALSE );
	int StartMovingSegment( CDC * pDC, cnet * net, int ic, int ivtx,
								   int x, int y, int crosshair, int use_third_segment );
	// CPT r295: removed via_w and via_hole_w params
	int StartDraggingSegment( CDC * pDC, cnet * net, int ic, int iseg,
						int x, int y, int layer1, int layer2, int w, 
						int layer_no_via, int dir,
						int crosshair = 1 );
	int CancelDraggingSegment( cnet * net, int ic, int iseg );
	int StartDraggingSegmentNewVertex( CDC * pDC, cnet * net, int ic, int iseg,
								   int x, int y, int layer, int w, int crosshair );
	int CancelDraggingSegmentNewVertex( cnet * net, int ic, int iseg );
	void StartDraggingStub( CDC * pDC, cnet * net, int ic, int iseg,
						int x, int y, int layer1, int w, 
						int layer_no_via, int crosshair, int inflection_mode );			// CPT r295, removed via-width args
	void CancelDraggingStub( cnet * net, int ic, int iseg );
	int CancelMovingSegment( cnet * net, int ic, int ivtx );

	// functions for vias
	void CalcViaWidths(cnet *net, int w, int *via_w, int *via_hole_w);					// CPT r295
	int ReconcileVia( cnet * net, int ic, int ivtx, BOOL bDrawVertex=TRUE );
	int ForceVia( cnet * net, int ic, int ivtx, BOOL set_areas=TRUE );
	int UnforceVia( cnet * net, int ic, int ivtx, BOOL set_areas=TRUE );
	void DrawVertex( cnet * net, int ic, int iv );
	void UndrawVia( cnet * net, int ic, int iv );
	void SetViaVisible( cnet * net, int ic, int iv, BOOL visible );

	// functions for vertices
	void HighlightVertex( cnet * net, int ic, int ivtx );
	int StartDraggingVertex( CDC * pDC, cnet * net, int ic, int iseg,
						int x, int y, int cosshair = 1 );
	int CancelDraggingVertex( cnet * net, int ic, int ivtx );
	void MoveVertex( cnet * net, int ic, int ivtx, int x, int y );
	int GetViaConnectionStatus( cnet * net, int ic, int iv, int layer );
	void GetViaPadInfo( cnet * net, int ic, int iv, int layer,
		int * pad_w, int * hole_w, int * connect_status );
	BOOL TestHitOnVertex( cnet * net, int layer, int x, int y, 
		cnet ** hit_net, int * hit_ic, int * hit_iv );

	// functions related to parts
	int RehookPartsToNet( cnet * net );
	void PartAdded( cpart * part );
	int PartMoved( cpart * part, int dx = 1, int dy = 1 );			// CPT added optional params
//	int PartMoved( cpart * part );
	int PartFootprintChanged( cpart * part );
	int PartDeleted( cpart * part, BOOL bSetAreas=TRUE );
	int PartDisconnected( cpart * part, BOOL bSetAreas=TRUE );
	void SwapPins( cpart * part1, CString * pin_name1,
						cpart * part2, CString * pin_name2 );
	void PartRefChanged( CString * old_ref_des, CString * new_ref_des );

	// functions for copper areas
	int AddArea( cnet * net, int layer, int x, int y, int hatch, BOOL bDraw=TRUE );
	void InsertArea( cnet * net, int iarea, int layer, int x, int y, int hatch, BOOL bDraw=TRUE );
	int AppendAreaCorner( cnet * net, int iarea, int x, int y, int style );
	int InsertAreaCorner( cnet * net, int iarea, int icorner, 
		int x, int y, int style );
	void MoveAreaCorner( cnet * net, int iarea, int icorner, int x, int y );
	void HighlightAreaCorner( cnet * net, int iarea, int icorner );
	void HighlightAreaSides( cnet * net, int ia );
	CPoint GetAreaCorner( cnet * net, int iarea, int icorner );
	int CompleteArea( cnet * net, int iarea, int style );
	void SetAreaConnections();
	void SetAreaConnections( cnet * net, int iarea );
	void SetAreaConnections( cnet * net );
	void SetAreaConnections( cpart * part );
	BOOL TestPointInArea( cnet * net, int x, int y, int layer, int * iarea );
	int RemoveArea( cnet * net, int iarea );
	void SelectAreaSide( cnet * net, int iarea, int iside );
	void SelectAreaCorner( cnet * net, int iarea, int icorner );
	void SetAreaSideStyle( cnet * net, int iarea, int iside, int style );
	int StartDraggingAreaCorner( CDC *pDC, cnet * net, int iarea, int icorner, int x, int y, int crosshair = 1 );
	int CancelDraggingAreaCorner( cnet * net, int iarea, int icorner );
	int StartDraggingInsertedAreaCorner( CDC *pDC, cnet * net, int iarea, int icorner, int x, int y, int crosshair = 1 );
	int CancelDraggingInsertedAreaCorner( cnet * net, int iarea, int icorner );
	void RenumberAreas( cnet * net );
	int TestAreaPolygon( cnet * net, int iarea );
	int ClipAreaPolygon( cnet * net, int iarea, 
		BOOL bMessageBoxArc, BOOL bMessageBoxInt, BOOL bRetainArcs=TRUE );
	int AreaPolygonModified( cnet * net, int iarea, BOOL bMessageBoxArc, BOOL bMessageBoxInt );
	int CombineAllAreasInNet( cnet * net, BOOL bMessageBox, BOOL bUseUtility );
	int TestAreaIntersections( cnet * net, int ia );
	int TestAreaIntersection( cnet * net, int ia1, int ia2 );
	int CombineAreas( cnet * net, int ia1, int ia2 );
	void ApplyClearancesToArea( cnet * net, int ia, int flags,
			int fill_clearance, int min_silkscreen_stroke_wid, 
			int thermal_wid, int hole_clearance );

	// I/O  functions
	int WriteNets( CStdioFile * file );
	void ReadNets( CStdioFile * pcb_file, double read_version, int * layers=NULL );
	void ExportNetListInfo( netlist_info * nl );
	void ImportNetListInfo( netlist_info * nl, int flags, CDlgLog * log,
		int def_w, int def_w_v, int def_w_v_h );
	void Copy( CNetList * nl );
	void RestoreConnectionsAndAreas( CNetList * old_nl, int flags, CDlgLog * log=NULL );
	void ReassignCopperLayers( int n_new_layers, int * layer );
	void ImportNetRouting( CString * name, CArray<cnode> * nodes, 
		CArray<cpath> * paths, int tolerance, CDlgLog * log=NULL, BOOL bVerbose=TRUE );

	// undo functions
	undo_con * CreateConnectUndoRecord( cnet * net, int icon, BOOL set_areas=TRUE );
	undo_area * CreateAreaUndoRecord( cnet * net, int iarea, int type );
	undo_net * CreateNetUndoRecord( cnet * net );
	static void ConnectUndoCallback( int type, void * ptr, BOOL undo );
	static void AreaUndoCallback( int type, void * ptr, BOOL undo );
	static void NetUndoCallback( int type, void * ptr, BOOL undo );

	// functions for tee_IDs
	void ClearTeeIDs();
	int GetNewTeeID();
	int FindTeeID( int id );
	void RemoveTeeID( int id );
	void AddTeeID( int id );

	// functions for tees and branches
	BOOL FindTeeVertexInNet( cnet * net, int id, int * ic=NULL, int * iv=NULL );
	BOOL FindTeeVertex( int id, cnet ** net, int * ic=NULL, int * iv=NULL );
	int RemoveTee( cnet * net, int id );
	int RemoveTeeIfNoBranches( cnet * net, int id );
	BOOL TeeViaNeeded( cnet * net, int id, cvertex ** v );
	BOOL RemoveOrphanBranches( cnet * net, int id, BOOL bRemoveSegs=FALSE );

private:
	CDisplayList * m_dlist;
	CPartList * m_plist;
	int m_layers;	// number of copper layers
	int m_def_w, m_def_via_w, m_def_via_hole_w;
	int m_pos_i;	// index for iterators
	POSITION m_pos[MAX_ITERATORS];	// iterators for nets
	CArray<int> m_tee;
	BOOL m_bSMT_connect;

public:
	CMapStringToPtr m_map;	// map net names to pointers
	CMap<int,int,cnet*,cnet*> m_uid_map;
	int m_annular_ring;
	CFreePcbDoc *m_doc;			// CPT.  Added so that OptimizeConnections() can call CFreePcbDoc::ProjectModified()
};

