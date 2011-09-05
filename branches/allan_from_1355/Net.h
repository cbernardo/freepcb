// Net.h: interface for the cnet class.
//
// Describes a net
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
	~cpin();
	void Initialize( cnet * net );
//**	int m_uid;
	CString ref_des;	// reference designator such as 'U1'
	CString pin_name;	// pin name such as "1" or "A23"
	cpart * part;		// pointer to part containing the pin
	cnet * m_net;		// parent net
	int utility;
};

// carea: describes a copper area in a net
class carea : public CPolyLine
{
public:
	carea();
	carea( const carea& source );	// dummy copy constructor
	~carea();						
	carea &operator=( carea &a );	// dummy assignment operator
	carea &operator=( const carea &a );	// dummy assignment operator
	void Initialize( CDisplayList * dlist, cnet * net );
	int UID(){ return m_uid; };
	void SetUID( int uid ){ m_uid = uid; };

	cnet * m_net;		// parent net
	id m_id;			// id
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
	int UID(){ return m_uid; };
	void ReplaceUID( int uid );
	int GetIndex();
	cvertex& GetPreVtx();
	cvertex& GetPostVtx();

	int m_uid;				// unique id
	int layer;				// copper layer
	int width;				// width
	int curve;
	int selected;			// 1 if selected for editing
	dl_element * dl_el;		// display element for segment
	dl_element * dl_sel;	// selection line
	CDisplayList * m_dlist;
	cnet * m_net;			// parent net
	cconnect * m_con;		// parent connection
	int utility;
};

// CVertex: new class for vertices
class CVertex
{
	enum Type { V_PIN, V_TRACE, V_END };	// types of vertices
public:
	CVertex( cnet * net );
	~CVertex();

	int m_uid;		// UID
	Type m_type;	// type of vertex
	int m_pin_uid;	// if type == V_PIN, UID of pin in net
	cnet * m_net;				// parent net
	CDisplayList * m_dlist;
};

// cvertex: describes a vertex between segments 
// in a connection
class cvertex
{
public:
	enum Type { V_PIN, V_TRACE, V_END };	// types of vertices

	cvertex();
	cvertex( cvertex& src );
	~cvertex();
	cvertex &operator=( const cvertex &v ); // assignment	
	cvertex &operator=( cvertex &v );	
	void Initialize( cconnect * c );
	int UID(){ return m_uid; };
	void ReplaceUID( int uid );
	void Undraw();
	Type GetType();
	cpin& Pin();	

	int m_uid;					// unique id
	Type m_type;				// type of vertex
	int m_pin_uid;				// if type == V_PIN, UID of pin in net
	int x, y;					// coords
	int pad_layer;				// layer of pad if this is first or last vertex, otherwise 0
	int force_via_flag;			// force a via even if no layer change
	int via_w, via_hole_w;		// via width and hole width (via_w==0 means no via)
//	BOOL m_bDrawingEnabled;		// TRUE if may be drawn
//	BOOL m_bDrawn;				// TRUE if drawn into CDisplayList
	CArray<dl_element*> dl_el;	// array of display elements for each layer
	dl_element * dl_sel;		// selection box
	dl_element * dl_hole;		// hole in via
	int tee_ID;					// used to flag a t-connection point
	int utility, utility2;		// used for various functions
	CDisplayList * m_dlist;		// 
	cnet * m_net;				// parent net
	cconnect * m_con;			// parent connection
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
	void ReplaceUID( int uid );
	void ClearArrays();

	// pins
	cpin * StartPin();
	cpin * EndPin();

	// trace vertices
	cvertex * FirstVtx();
	cvertex * LastVtx();
	cvertex * VtxByUID( int uid, int * index=NULL );
	cvertex& VtxByIndex( int iv );
	int VtxIndexByPtr( cvertex * v );
	int VtxUIDByPtr( cvertex * v );
	cvertex& InsertVertexByIndex( int iv, const cvertex& new_vtx );

	// trace segments
	int NumSegs();
	void SetNumSegs( int n );
	cseg * SegByUID( int uid, int * index=NULL );
	cseg& SegByIndex( int is );
	int SegIndexByPtr( cseg * s );
	int SegUIDByPtr( cseg * s );

	// segments and vertices
	void InsertSegAndVtxByIndex( int is, int dir, 
				const cseg& new_seg, const cvertex& new_vtx );
	void AppendSegAndVertex( const cseg& new_seg, const cvertex& new_vtx );
	void PrependVertexAndSeg( const cvertex& new_vtx, const cseg& new_seg );
	void RemoveSegAndVertexByIndex( int is, int dir );

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
	// pins
	int NumPins();
	cpin * PinByIndex( int ip );
	cpin * PinByUID( int uid );
	// connections
	int NumCons();
	cconnect * ConByIndex( int ic );
	cconnect * ConByUID( int uid, int * index=NULL );
	int ConIndexByUID( int uid );
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
	cconnect * AddNewConnect();
	void RemoveConnect( cconnect * c );
	void RecreateConnectFromUndo( undo_con * con, undo_seg * seg, undo_vtx * vtx );

// member variables
	id id;				// net id
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

