// Net.h: interface for the cnet class.
//
// Describes a net
//

#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "ids.h"
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

// cnet: describes a net
class cnet
{
	friend class CNetList;
	friend class CIterator_cconnect;
public:
	cnet( CDisplayList * dlist, CNetList * nlist );
	~cnet();
	int NumCons();
	int NumPins();
	int NumAreas();
	void AddPin( CString * ref_des, CString * pin_name, BOOL set_areas=TRUE );
	void RemovePinByUID( int uid, BOOL bSetAreas=TRUE );
	void RemovePin( CString * ref_des, CString * pin_name, BOOL bSetAreas=TRUE );
	void RemovePin( int pin_index, BOOL bSetAreas=TRUE );
	cconnect * AddNewConnect();
	void RemoveConnect( cconnect * c );
	cconnect * GetConnectByIndex( int ic );
	cconnect * GetConnectByUID( int uid );
	int GetConnectIndexByUID( int uid );
	int GetConnectIndexByPtr( cconnect * c );
	void RecreateConnectFromUndo( undo_con * con, undo_seg * seg, undo_vtx * vtx );

	id id;				// net id
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

