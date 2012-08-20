// NetListNew.h: CPT2 sketches for the net-list class under the new architecture.
// Basically the new cnetlist is just a carray<cnet2>.
// A whole bunch of functions used to belong to this class that have now been farmed out as methods
//   in the cpcb_item classes.  The result is simpler argument lists and essentially no more
//   reliance on index values.

#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "DisplayList.h"
#include "PcbItem.h"

extern int m_layer_by_file_layer[MAX_LAYERS];

class cnetlist;
class cvertex2;
class cconnect2;
class cnet2;
class CFreePcbDoc;					// CPT
template <class T> class carray;
template <class T> class citer;

#define MAX_NET_NAME_SIZE 39

#if 0
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
#endif

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
	void MarkAllNets( int utility );													// Done in cpp
	void MoveOrigin( int dx, int dy );													// Done in cpp
	cnet2 * GetNetPtrByName( CString * name )
	{
		citer<cnet2> in (&nets);
		for (cnet2 *n = in.First(); n; n = in.Next())
			if (n->name == *name) return n;
		return NULL;
	}
	/* cpin2 *GetPinByNames( CString *ref_des, CString *pin_name)
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
	*/

	int CheckNetlist( CString * logstr );
	int CheckConnectivity( CString * logstr );
	BOOL GetNetBoundaries( CRect * r );											// Done in cpp

	// functions for connections
	void OptimizeConnections( BOOL bLimitPinCount=TRUE, BOOL bVisibleNetsOnly=TRUE ); // Done in cpp
	void CleanUpAllConnections( CString * logstr=NULL );					// CPT2 done in cpp

	// functions related to parts. CPT2 TODO think these through more
	int RehookPartsToNet( cnet2 * net );
	// void PartAdded( cpart2 * part );												
	// int PartMoved( cpart2 * part, int dx = 1, int dy = 1 );								// CPT2.  Use cpart2::PartMoved
	int PartFootprintChanged( cpart2 * part );
	// int PartDeleted( cpart2 * part, BOOL bSetAreas=TRUE );								// CPT2.  Use cpart2::Delete()
	// int PartDisconnected( cpart2 * part, BOOL bSetAreas=TRUE );							// CPT2. Use cpart2::Disconnect()
	void SwapPins( cpart2 * part1, CString * pin_name1,
						cpart2 * part2, CString * pin_name2 );								// CPT2 TODO.  'S' key was disabled in allan_uids_new_drawing branch.
																							// Bring it back some time?
	void SetThermals();																					// CPT2 new.

	// I/O  functions
	void ReadNets( CStdioFile * pcb_file, double read_version, int * layers=NULL );			// Done in cpp.
	void WriteNets( CStdioFile * file );													// Done in cpp.
	void ExportNetListInfo( netlist_info * nli );
	void ImportNetListInfo( netlist_info * nli, int flags, CDlgLog * log,
		int def_w, int def_w_v, int def_w_v_h );
	// void Copy( cnetlist * nli );
	// void RestoreConnectionsAndAreas( cnetlist * old_nl, int flags, CDlgLog * log=NULL );
	void ReassignCopperLayers( int n_new_layers, int * layer );
};

