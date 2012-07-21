// NetListNew.h: CPT2 sketches for the net-list class under the new architecture.
// Basically the new cnetlist is just a carray<cnet2>.
// A whole bunch of functions used to belong to this class that have now been farmed out as methods
//   in the cpcb_item classes.  The result is simpler argument lists and essentially no more
//   reliance on index values.

#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "DisplayList.h"
#include "PartList.h"
#include "PolyLine.h"
#include "UndoList.h"
#include "LinkList.h"
#include "Cuid.h"
#include "PcbItem.h"
#include "NetList.h"

extern int m_layer_by_file_layer[MAX_LAYERS];

class cnetlist;
class cvertex2;
class cconnect2;
class cnet2;
class CFreePcbDoc;					// CPT
template <class T> class carray;
template <class T> class citer;

#if 0								// CPT2 the following stuff duplicates what's in the old NetList.h, so disable it for now

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
	id m_id;
	// array of undo_pin structs start here
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
	BOOL modified;
	CArray<CString> ref_des;
	CArray<CString> pin_name;
};

// netlist_info is an array of net_info for each net
typedef CArray<net_info> netlist_info;

#endif

// CPT2: My modified version of CNetList (distinguished, during their period of coexistence, by being called cnetlist).
class cnetlist  
{
	friend class cnet2;
public:
	// enum{ MAX_ITERATORS=10 }; // CPT2
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

public:
	carray<cnet2> nets;	
	CFreePcbDoc *m_doc;							// CPT.  Added so that ReconcileVias() can figure out a good via width.  
												// CPT2.  Might be put to other uses as well.
	CDisplayList * m_dlist;
	cpartlist * m_plist;
	int m_layers;						// number of copper layers
	int m_def_w, m_def_via_w, m_def_via_hole_w;
	int m_annular_ring;
	BOOL m_bSMT_connect;


	cnetlist( CFreePcbDoc * _doc ) ;													// CPT2:  Changed param
	~cnetlist() { }																		// CPT2 TODO
	void SetNumCopperLayers( int layers ) { m_layers = layers; }
	int GetNumCopperLayers() { return m_layers; }
	void SetWidths( int w, int via_w, int via_hole_w ) 
		{ m_def_w = w, m_def_via_w = via_w, m_def_via_hole_w = via_hole_w; }
	void SetViaAnnularRing( int ring ) { m_annular_ring = ring; }
	void SetSMTconnect( BOOL bSMTconnect ) { m_bSMT_connect = bSMTconnect; }

	// functions for nets and pins
	void MarkAllNets( int utility )
	{
		citer<cnet2> in (&nets);
		for (cnet2 *n = in.First(); n; n = in.Next())
			n->utility = utility;
	}
	void MoveOrigin( int x_off, int y_off ) { }
	cnet2 * GetNetPtrByName( CString * name )
	{
		citer<cnet2> in (&nets);
		for (cnet2 *n = in.First(); n; n = in.Next())
			if (n->name == *name) return n;
		return NULL;
	}
	// cnet2 * GetNetPtrByUID( int uid ); // CPT2 Use nets.FindByUID().
	// cnet2 * AddNet( CString name, int def_width, int def_via_w, int def_via_hole_w ) // CPT2, just use cnet2::cnet2( cnetlist, ....) 
																						// NB old unused max_pins arg is gone for good.
	// void RemoveNet( cnet2 * net ); // CPT2 Use "nets.Remove(net)"
	void RemoveAllNets() { nets.RemoveAll(); }
	// void RemoveNetPin( cpart2 * part, CString * pin_name, BOOL bSetAreas=TRUE ); // CPT2 just use cnet2::RemovePin( pin, bSetAreas );
	// void DisconnectNetPin( cpart2 * part, CString * pin_name, BOOL bSetAreas=TRUE ); // CPT2 use cpin2::Disconnect( bSetAreas )
	// void DisconnectNetPin( cnet2 * net, CString * ref_des, CString * pin_name, BOOL bSetAreas=TRUE ); // CPT2 use cpin2::Disconnect( bSetAreas ) plus:
	cpin2 *GetPinByNames( CString *ref_des, CString *pin_name)
	{
		citer<cnet2> in (&nets);
		for (cnet2 *n = in.First(); n; n = in.Next())
		{
			citer<cpin2> ip (&n->pins);
			for (cpin2 *p = ip.First(); p; p = ip.Next())
				if (p->part->ref_des == *ref_des && p->pin_name == *pin_name)
					return p;
		}
		return NULL;
	}

	// int GetNetPinIndex( cnet2 * net, CString * ref_des, CString * pin_name );  // CPT2 use GetPinByNames() above
	// int SetNetWidth( cnet2 * net, int w, int via_w, int via_hole_w ); // CPT2 use cnet2::SetWidth()
	// void SetNetVisibility( cnet2 * net, BOOL visible ); // CPT2 use cnet2::SetVisible
	// BOOL GetNetVisibility( cnet2 * net ); // CPT2 use cnet2::GetVisible
	int CheckNetlist( CString * logstr ) { return 0; }
	int CheckConnectivity( CString * logstr ) { return 0; }
	// void HighlightNetConnections( cnet2 * net, id * exclude_id=NULL );       // CPT2 use cnet2::HighlightConnections()
	// void HighlightNet( cnet2 * net, id * exclude_id=NULL );			        // CPT2 use cnet2::Highlight()
	// void GetWidths( cnet2 * net, int * w, int * via_w, int * via_hole_w );   // CPT2 use cnet2::GetWidths()
	BOOL GetNetBoundaries( CRect * r );											// Done in cpp

	// functions for connections
	// int RemoveNetConnect( cnet2 * net, int ic, BOOL set_areas=TRUE ); // CPT2 use cconnect2::Remove() 
	// int UnrouteNetConnect( cnet2 * net, int ic ); // CPT2 use cconnect2::Unroute()
	// int SetConnectionWidth( cnet2 * net, int ic, int w, int via_w, int via_hole_w ); // CPT2 use cconnect2::SetWidth()
	void OptimizeConnections( BOOL bBelowPinCount=FALSE, int pin_count=0, BOOL bVisibleNetsOnly=TRUE ) { } // CPT2 TODO. Slight changes
	// int OptimizeConnections( cnet2 * net, int ic, BOOL bBelowPinCount, int pin_count, BOOL bVisibleNetsOnly=TRUE );  // CPT2 use cnet2::OptimizeConnections
	// void OptimizeConnections( cpart2 * part, BOOL bBelowPinCount, int pin_count, BOOL bVisibleNetsOnly=TRUE ); // CPT2 use cpart2::OptimizeConnections
	// BOOL TestHitOnConnectionEndPad( int x, int y, cnet2 * net, int ic, int layer, int dir ); // CPT2 use cconnect2::TestHitOnEndPad() 
	// int TestHitOnAnyPadInNet( int x, int y, int layer, cnet2 * net ); // CPT2 use cnet2::TestHitOnAnyPad()
	// void ChangeConnectionPin( cnet2 * net, int ic, int end_flag, 
	//	cpart2 * part, CString * pin_name );								// CPT2 use cconnect2::ChangePin()
	// void HighlightConnection( cnet2 * net, int ic, id * exclude_id=NULL ); // CPT2 use cconnect2::Highlight().  TODO Can we ditch exclude_id param?
	// void CleanUpConnections( cnet2 * net, CString * logstr=NULL );		// CPT2 use cnet2::CleanUpConnections()
	void CleanUpAllConnections( CString * logstr=NULL ) { }

	// functions for segments.
	// CPT r295 note that via_w and via_hole_w params are NO LONGER USED.
	// int AppendSegment( cnet2 * net, int ic, int x, int y, int layer, int width );				// CPT2 use cconnect2::AppendSegment()
	// int InsertSegment( cnet2 * net, int ic, int iseg, int x, int y, int layer, int width, int dir );  // CPT2 use cseg2::InsertSegment()
	// id  UnrouteSegment( cnet2 * net, int ic, int iseg, int dx=1, int dy=1, int end=0 );				// CPT2 use cseg2::Unroute()
	// void UnrouteSegmentWithoutMerge( cnet2 * net, int ic, int iseg, double dx=1, double dy=1, int end=0 ); // CPT2 use cseg2::UnrouteWithoutMerge()
	// id MergeUnroutedSegments( cnet2 * net, int ic ); // CPT2 use cconnect2::MergeUnroutedSegments()
	// int RouteSegment( cnet2 * net, int ic, int iseg, int layer, int width ); // CPT2 use cseg2::Route().
	// void RemoveSegment( cnet2 * net, int ic, int iseg, BOOL bHandleTees=FALSE, BOOL bSetAreaConnections=TRUE ); // CPT2 use cseg2::RemoveMerge(),
																				// which will also cover cnet::RemoveSegAndVertexByIndex() and
																				// cnet::RemoveVertexAndSegByIndex().
																									
	// int ChangeSegmentLayer( cnet2 * net, int ic, int iseg, int layer );					 // CPT2 use cseg2::SetLayer().
	// int SetSegmentWidth( cnet2 * net, int ic, int is, int w, int via_w, int via_hole_w ); // CPT2 use cseg2::SetWidth()
	// void HighlightSegment( cnet2 * net, int ic, int iseg, bool bThin=FALSE );			 // CPT2 use cseg2::Highlight()
	// int StartMovingSegment( CDC * pDC, cnet2 * net, int ic, int ivtx,					 // CPT2 use cseg2::StartMoving()
	//							   int x, int y, int crosshair, int use_third_segment );
	// CPT r295: removed via_w and via_hole_w params
	// int StartDraggingSegment( CDC * pDC, cnet2 * net, int ic, int iseg,					 // CPT2 use cseg2::StartDragging()
	//					int x, int y, int layer1, int layer2, int w, 
	//					int layer_no_via, int dir,
	//					int crosshair = 1 );
	// int CancelDraggingSegment( cnet2 * net, int ic, int iseg );							// CPT2 use cseg2::CancelDragging()
	// int StartDraggingSegmentNewVertex( CDC * pDC, cnet2 * net, int ic, int iseg,			// CPT2 use cseg2::StartDraggingNewVertex()
	//							   int x, int y, int layer, int w, int crosshair );
	// int CancelDraggingSegmentNewVertex( cnet2 * net, int ic, int iseg );					// CPT2 use cseg2::CancelDraggingNewVertex()
	// void StartDraggingStub( CDC * pDC, cnet2 * net, int ic, int iseg,
	//					int x, int y, int layer1, int w, 
	//					int layer_no_via, int crosshair, int inflection_mode );				// CPT2 use cvertex2::StartDraggingStub()
	// void CancelDraggingStub( cnet2 * net, int ic, int iseg );							// CPT2 pretty much reduced to a simple CDisplayList::StopDragging(),
																							// therefore eliminated
	// int CancelMovingSegment( cnet2 * net, int ic, int ivtx );							// use cseg2::CancelMoving()

	// functions for vias
	// void CalcViaWidths(cnet2 *net, int w, int *via_w, int *via_hole_w);					// CPT r295. CPT2 use cnet2::CalcViaWidths()
	// int ReconcileVia( cnet2 * net, int ic, int ivtx, BOOL bDrawVertex=TRUE );			// CPT2.  Use cvertex2::ReconcileVia()
	// int ForceVia( cnet2 * net, int ic, int ivtx, BOOL set_areas=TRUE );					// CPT2.  Use cvertex2::ForceVia()
	// int UnforceVia( cnet2 * net, int ic, int ivtx, BOOL set_areas=TRUE );				// CPT2.  Use cvertex2::UnforceVia()
	// void DrawVertex( cnet2 * net, int ic, int iv );										// CPT2.  Use cvertex2::Draw
	// void UndrawVia( cnet2 * net, int ic, int iv );										// CPT2.  Use cvertex2::Undraw()
	// void SetViaVisible( cnet2 * net, int ic, int iv, BOOL visible );						// CPT2. Use cvertex2::SetVisible()

	// functions for vertices
	// void HighlightVertex( cnet2 * net, int ic, int ivtx );								// CPT2. Use cvertex2::Highlight. 
	// int StartDraggingVertex( CDC * pDC, cnet2 * net, int ic, int iseg,					// CPT2. Use cvertex2::StartDragging
	//					int x, int y, int cosshair = 1 );
	// int CancelDraggingVertex( cnet2 * net, int ic, int ivtx );							// CPT2. Use cvertex2::CancelDragging
	// void MoveVertex( cnet2 * net, int ic, int ivtx, int x, int y );						// CPT2. Use cvertex2::Move
	// int GetViaConnectionStatus( cnet2 * net, int ic, int iv, int layer );				// CPT2. Use cvertex2::GetViaConnectionStatus
	// void GetViaPadInfo( cnet2 * net, int ic, int iv, int layer,
	//	int * pad_w, int * hole_w, int * connect_status );									// CPT2. Use cvertex2::GetViaPadInfo
	// BOOL TestHitOnVertex( cnet2 * net, int layer, int x, int y, 
	// 	cnet2 ** hit_net, int * hit_ic, int * hit_iv );										// CPT2. Use cnet2::TestHitOnVertex

	// functions related to parts. CPT2 TODO think these through more
	int RehookPartsToNet( cnet2 * net );
	void PartAdded( cpart2 * part );														// Done in cpp, derived from CNetList func.
	// int PartMoved( cpart2 * part, int dx = 1, int dy = 1 );								// CPT2.  Use cpart2::PartMoved
	int PartFootprintChanged( cpart2 * part );
	// int PartDeleted( cpart2 * part, BOOL bSetAreas=TRUE );								// CPT2.  Use cpart2::Delete()
	// int PartDisconnected( cpart2 * part, BOOL bSetAreas=TRUE );							// CPT2. Use cpart2::Disconnect()
	void SwapPins( cpart2 * part1, CString * pin_name1,
						cpart2 * part2, CString * pin_name2 );
	// void PartRefChanged( CString * old_ref_des, CString * new_ref_des );					// CPT2 obsolete

	// functions for copper areas
	// int AddArea( cnet2 * net, int layer, int x, int y, int hatch, BOOL bDraw=TRUE );					// CPT2 Use cnet2::AddArea
	// void InsertArea( cnet2 * net, int iarea, int layer, int x, int y, int hatch, BOOL bDraw=TRUE );  // CPT2 TODO Surely unnecessary?
	// int AppendAreaCorner( cnet2 * net, int iarea, int x, int y, int style );							// CPT2 Use cpolyline::AppendCorner
	// int InsertAreaCorner( cnet2 * net, int iarea, int icorner,										// CPT2 Use cpolyline::InsertCorner
	//	int x, int y, int style );
	// void MoveAreaCorner( cnet2 * net, int iarea, int icorner, int x, int y );						// CPT2 use ccorner::Move
	// void HighlightAreaCorner( cnet2 * net, int iarea, int icorner );									// CPT2 use ccorner::Highlight
	// void HighlightAreaSides( cnet2 * net, int ia );													// CPT2 use cpolyline::HighlightSides
	// CPoint GetAreaCorner( cnet2 * net, int iarea, int icorner );										// CPT2 just use ccorner.x,y
	// int CompleteArea( cnet2 * net, int iarea, int style );											// CPT2 use carea2::Complete
	/* CPT2 disabled in favor of SetThermals()
	void SetAreaConnections()
	{
		citer<cnet2> in (&nets);
		for (cnet2 *n = in.First(); n; n = in.Next())
			n->SetAreaConnections();
	}
	*/
	// void SetAreaConnections( cnet2 * net, int iarea );	// CPT2 use carea2::SetAreaConnections();
	// void SetAreaConnections( cnet2 * net );				// CPT2 use cnet2::SetAreaConnections();
	// void SetAreaConnections( cpart2 * part );			// CPT2 use cpart2::SetAreaConnections() ?
	// BOOL TestPointInArea( cnet2 * net, int x, int y, int layer, int * iarea );	// CPT2 use cnet2::NetAreaFromPoint
	// int RemoveArea( cnet2 * net, int iarea );									// CPT2 use carea2::Remove()
	// void SelectAreaSide( cnet2 * net, int iarea, int iside );					// CPT2 use CFreePcbView::SelectItem
	// void SelectAreaCorner( cnet2 * net, int iarea, int icorner );				// CPT2 ditto
	// void SetAreaSideStyle( cnet2 * net, int iarea, int iside, int style );		// CPT2 use cside::SetStyle
	// int StartDraggingAreaCorner( CDC *pDC, cnet2 * net, int iarea, int icorner, int x, int y, int crosshair = 1 );	// CPT2 use ccorner::StartDragging
	// int CancelDraggingAreaCorner( cnet2 * net, int iarea, int icorner );												// CPT2 use ccorner::CancelDragging
	// int StartDraggingInsertedAreaCorner( CDC *pDC, cnet2 * net, int iarea, int icorner, int x, int y, int crosshair = 1 ); // CPT2 use ccorner or cside
	// int CancelDraggingInsertedAreaCorner( cnet2 * net, int iarea, int icorner );											  // functions TODO figure it out
	// void RenumberAreas( cnet2 * net );																// CPT2 obsolete
	// int TestAreaPolygon( cnet2 * net, int iarea );													// CPT2 use cpolyline::TestPolygon
	// int ClipAreaPolygon( cnet2 * net, int iarea, 
	// 	BOOL bMessageBoxArc, BOOL bMessageBoxInt, BOOL bRetainArcs=TRUE );								// CPT2 use carea2::ClipPolygon
	// int AreaPolygonModified( cnet2 * net, int iarea, BOOL bMessageBoxArc, BOOL bMessageBoxInt );		// CPT2 use carea2::PolygonModified
	// int CombineAllAreasInNet( cnet2 * net, BOOL bMessageBox, BOOL bUseUtility );						// CPT2 use cnet2::CombineAllAreas
	// int TestAreaIntersections( cnet2 * net, int ia );												// CPT2 use carea2::TestIntersections
	// int TestAreaIntersection( cnet2 * net, int ia1, int ia2 );										// CPT2 use cpolyline::TestIntersection
	// int CombineAreas( cnet2 * net, int ia1, int ia2 );												// CPT2 use cpolyline::CombinePolyline
	// void ApplyClearancesToArea( cnet2 * net, int ia, int flags,										// CPT2 will be a carea2 function, I imagine
	//		int fill_clearance, int min_silkscreen_stroke_wid, 
	//		int thermal_wid, int hole_clearance );
	void SetThermals();																					// CPT2 new.

	// I/O  functions
	void ReadNets( CStdioFile * pcb_file, double read_version, int * layers=NULL );			// Done in cpp.
	void WriteNets( CStdioFile * file );														// Done in cpp.
	void ExportNetListInfo( netlist_info * nl ) { }
	void ImportNetListInfo( netlist_info * nl, int flags, CDlgLog * log,
		int def_w, int def_w_v, int def_w_v_h );
	void Copy( cnetlist * nl );
	void RestoreConnectionsAndAreas( cnetlist * old_nl, int flags, CDlgLog * log=NULL );
	void ReassignCopperLayers( int n_new_layers, int * layer ) { }
	void ImportNetRouting( CString * name, CArray<cnode> * nodes, 
		CArray<cpath> * paths, int tolerance, CDlgLog * log=NULL, BOOL bVerbose=TRUE ) { }

	// undo functions
	undo_con * CreateConnectUndoRecord( cnet2 * net, int icon, BOOL set_areas=TRUE );
	undo_area * CreateAreaUndoRecord( cnet2 * net, int iarea, int type );
	undo_net * CreateNetUndoRecord( cnet2 * net );
	static void ConnectUndoCallback( int type, void * ptr, BOOL undo );
	static void AreaUndoCallback( int type, void * ptr, BOOL undo );
	static void NetUndoCallback( int type, void * ptr, BOOL undo );

	// functions for tee_IDs
	/* CPT2 All obsolete
	void ClearTeeIDs();
	int GetNewTeeID();
	int FindTeeID( int id );
	void RemoveTeeID( int id );
	void AddTeeID( int id );
	*/

	// functions for tees and branches
	// BOOL FindTeeVertexInNet( cnet2 * net, int id, int * ic=NULL, int * iv=NULL );// CPT2 Obsolete
	// BOOL FindTeeVertex( int id, cnet2 ** net, int * ic=NULL, int * iv=NULL ); // CPT2 Obsolete
	// int RemoveTee( cnet2 * net, int id );									// CPT2 Obsolete
	// int RemoveTeeIfNoBranches( cnet2 * net, int id );						// CPT2.  Disused
	// BOOL TeeViaNeeded( cnet2 * net, int id, cvertex2 ** v );					// Folded into cvertex2::IsViaNeeded().
	// BOOL RemoveOrphanBranches( cnet2 * net, int id, BOOL bRemoveSegs=FALSE ); // CPT2. Disused
};

