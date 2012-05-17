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

// cpin: describes a pin in a net
class cpin
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
	id& Id();

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

