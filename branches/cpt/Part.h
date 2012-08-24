// Part.h --- header file for classes related most closely to parts, many of which are descendants of cpcb_item:
//  cpart2, cpin2, cpartlist

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

#define MAX_REF_DES_SIZE 39

// clearance types for GetPadDrawInfo()
enum {
	CLEAR_NORMAL = 0,
	CLEAR_THERMAL,
	CLEAR_NONE
};


// struct used for DRC to store pin info
struct drc_pin {
	int hole_size;	// hole diameter or 0
	int min_x;		// bounding rect of padstack
	int max_x;
	int min_y;
	int max_y;
	int max_r;		// max. radius of padstack
	int layers;		// bit mask of layers with pads
};

// cpin2: describes a pin on the main editor screen, associated with a particular part and possibly assigned to a net.
class cpin2 : public cpcb_item
{
public:
	// CString ref_des;		// reference designator such as 'U1'.  CPT2 READ OFF of part->ref_des
	CString pin_name;		// pin name such as "1" or "A23"
	cpart2 * part;			// pointer to part containing the pin.
	int x, y;				// position on PCB
	cpadstack *ps;			// CPT2. Seems useful to have this here.  Constructor sets it up
	int pad_layer;			// CPT2. layer of pad on this pin (was in cvertex2).  
							// Constructor sets this based on ps. Possible values LAY_PAD_THRU, LAY_TOP_COPPER, LAY_BOTTOM_COPPER
	cnet2 * net;			// pointer to net, or NULL if not assigned.
	bool bNeedsThermal;		// CPT2 new.  Set to true if there's an areas from the same network that occupies this same point.
	dl_element * dl_hole;	// pointer to graphic element for hole
	CArray<dl_element*> dl_els;	// array of pointers to graphic elements for pads
	dl_element *dl_thermal; // CPT2 new.  The thermal drawn with this pin.
	drc_pin drc;			// drc info.

	enum {
		// Return vals for GetConnectionStatus().
		NOT_CONNECTED = 0,		// pin not attached to net
		ON_NET = 1,				// pin is attached to a net
		TRACE_CONNECT = 2,		// pin connects to trace on this layer
		AREA_CONNECT = 4		// pin connects to copper area on this layer
	};


	cpin2(cpart2 *_part, cpadstack *_ps, cnet2 *_net);					// CPT2. Added args. Done in cpp
	cpin2(CFreePcbDoc *_doc, int _uid);

	bool IsOnPcb();													// Done in cpp
	bool IsPin() { return true; }
	cpin2 *ToPin() { return this; }
	int GetTypeBit() { return bitPin; }
	cnet2 *GetNet() { return net; }
	cundo_item *MakeUndoItem()
		{ return new cupin(this); }
	void MustRedraw();												// Override the default behavior of cpcb_item::MustRedraw()
	int GetLayer() { return pad_layer; }
	int GetWidth();													// Done in cpp
	void GetVtxs(carray<cvertex2> *vtxs);							// Done in cpp, CPT2 new.

	void Highlight();												// Done in cpp, derived from old CPartList::HighlightPad

	bool TestHit(int x, int y, int layer);							// Done in cpp, derived from CPartList::TestHitOnPad
	void SetPosition();												// Done in cpp.  New, but related to CPartList::GetPinPoint
	bool SetNeedsThermal();											// Done in cpp.  New, but related to CNetList::SetAreaConnections
	// void SetThermalVisible(int layer, bool bVisible) { }			// CPT2.  TODO figure this out.
	void Disconnect();												// Done in cpp
	cseg2 *AddRatlineToPin( cpin2 *p2 );

	bool GetDrawInfo(int layer,	bool bUse_TH_thermals, bool bUse_SMT_thermals,		// Done in cpp, derived from CPartList::GetPadDrawInfo().  Used during DRC
		  int mask_clearance, int paste_mask_shrink,
		  int * type=0, int * w=0, int * l=0, int * r=0, int * hole=0,										// CPT2 got rid of "x" and "y" args
		  int * connection_status=0, int * pad_connect_flag=0, int * clearance_type=0 );
	int GetConnectionStatus( int layer );											// Done in cpp, derived from CPartList::GetPinConnectionStatus().
};


// class cpart2 represents a part
class cpart2: public cpcb_item
{
public:
	cpartlist * m_pl;	// parent partlist
	carray<cpin2> pins;	// array of all pins in part
	BOOL visible;		// 0 to hide part                                ?? TODO Put into base class?
	int x,y;			// position of part origin on board
	int side;			// 0=top, 1=bottom
	int angle;			// orientation, degrees CW
	BOOL glued;

	CString ref_des;	// ref designator such as "U3"
	creftext *m_ref;	// CPT2 added. It's convenient to have an object for the ref-text on which we can invoke ctext methods.
						// Note that m_ref->m_x, m_ref->m_y, etc. will be _relative_ positions
	CString value_text;
	cvaluetext *m_value;	// CPT2 added.  Analogous to m_ref
	ctextlist *m_tl;		// CPT2 added.  Objects for each of the texts derived from the footprint.

	CString package;						// package (from original imported netlist, may be "")
	cshape * shape;							// pointer to the footprint of the part, may be NULL.  CPT2 I have yet to see a case where it is null, though.
	CArray<stroke> m_outline_stroke;		// array of outline strokes

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


	cpart2( cpartlist * pl );										// Done in cpp.
	cpart2(CFreePcbDoc *_doc, int _uid);
	~cpart2();

	bool IsOnPcb();													// Done in cpp
	bool IsPart() { return true; }
	cpart2 *ToPart() { return this; }
	int GetTypeBit() { return bitPart; }
	void Remove(bool bEraseTraces, bool bErasePart=true);			// Done in cpp.
	cundo_item *MakeUndoItem()
		{ return new cupart(this); }
	void SaveUndoInfo(bool bSaveAttachedConnects = true);

	void Move( int x, int y, int angle = -1, int side = -1);									// Done in cpp, derived from CPartList::Move
	void PartMoved( int dx=1, int dy=1 );														// Done in cpp, derived from CNetList::PartMoved
	void SetData( cshape * shape, CString * ref_des, CString *value_txt, CString * package, 
	     		  int x, int y, int side, int angle, int visible, int glued );					// Done in cpp, Derived from CPartList::SetPartData
	void SetValue( CString * value, int x, int y, int angle, int size, 
				  int w, BOOL vis, int layer );													// Done in cpp, Derived from CPartList::SetValue
	void InitPins();																			// Done in cpp. Basically all new
	void MarkConstituents(int util);

	int GetNumOutlineStrokes();
	void OptimizeConnections( BOOL bLimitPinCount=TRUE, BOOL bVisibleNetsOnly=TRUE );	// Done in cpp, derived from old CNetList function
	cpin2 *GetPinByName(CString *name);	// Done in cpp, new
	CPoint GetCentroidPoint();			// Done in cpp, derived from CPartList::GetCentroidPoint
	CPoint GetGluePoint( cglue *g );	// Done in cpp, derived from CPartList::GetGluePoint()
	int GetBoundingRect( CRect * part_r );		// Done in cpp. Derived from CPartList::GetPartBoundingRect()
	CRect CalcSelectionRect();
	bool IsAttachedToConnects();				// Done in cpp
	void ChangeFootprint( cshape * shape );		// Done in cpp, loosely derived from CPartList::PartFootprintChanged() & CNetList::PartFootprintChanged()

	int Draw();							// Done in cpp
	void Undraw();						// Done in cpp
	void Highlight();					// Done in cpp, derived from CPartList::HighlightPart()
	void SetVisible(bool bVis);         // Done in cpp
	void StartDragging( CDC * pDC, BOOL bRatlines, BOOL bBelowPinCount, int pin_count );		// Done in cpp, derived from CPartList::StartDraggingPart
	void CancelDragging();

	void MakeString( CString *str );	// Done in cpp, derived from old CPartList::SetPartString().
	void GetDRCInfo();					// CPT2 new.  Extracted some of the code formerly in DRC().
};


// partlist_info is used to hold digest of cpartlist 
// for editing in dialogs, or importing from netlist file
// notes:
//	package may be "" if no package assigned
//	shape may be NULL if no footprint assigned
//	may have package but no footprint, but not the reverse
typedef struct {
	cpart2 * part;		// pointer to original part, or NULL if new part added.
	CString ref_des;	// ref designator string
	int ref_size;		// size of ref text characters
	int ref_width;		// stroke width of ref text characters
	int ref_layer;		// ref layer
	BOOL ref_vis;		// CPT: visibility of ref text
	CString value;		// value (from original imported netlist, don't edit)
	int value_size;		// CPT2 added for consistency
	int value_width;	// CPT2 Ditto
	int value_layer;	// value layer
	BOOL value_vis;		// visibility of value
	CString package;	// package (from original imported netlist, don't edit)
	cshape * shape;		// pointer to shape (may be edited)
	bool bShapeChanged;	// Set if "part->shape" gets its internals modified.
	CString shape_file_name;	// CPT2 new.  Indicates which library file the shape came from.  Most often "", signifying the local cache,
						// but if user goes to the DlgAddPart it may change.
	BOOL deleted;		// flag to indicate that part was deleted
	BOOL bOffBoard;		// flag to indicate that position has not been set
	int x, y;			// position (may be edited)
	int angle, side;	// angle and side (may be edited)
} part_info;


typedef CArray<part_info> partlist_info;

// error codes
enum
{
	PL_NOERR = 0,
	PL_NO_DLIST,
	PL_NO_FOOTPRINT,
	PL_ERR
};


class cpartlist
{
public:
	carray<cpart2> parts;
	CFreePcbDoc *m_doc;
	int m_layers;
	int m_annular_ring;
	cnetlist * m_nlist;
	CDisplayList * m_dlist;

	cpartlist( CFreePcbDoc *doc );

	void UseNetList( cnetlist * nlist ) { m_nlist = nlist; }
	cpart2 * GetPartByName( CString *ref_des );	
	cpin2 * GetPinByNames ( CString *ref_des, CString *pin_name );
	cpart2 * Add( cshape * shape, CString * ref_des, CString *value_text, CString * package, 
		int x, int y, int side, int angle, int visible, int glued );
	cpart2 * AddFromString( CString * str );
	void MarkAllParts( int util );
	void SetNumCopperLayers( int nlayers ) { m_layers = nlayers; }
	void MoveOrigin( int dx, int dy );
	static int FootprintLayer2Layer( int fp_layer );
	void SetPinAnnularRing( int ring ) { m_annular_ring = ring; }
	int GetPartBoundaries( CRect * part_r );										// Done in cpp  CPT2 TODO check, seems fishy.
	void SetThermals();																// CPT2 new.  Updates the bNeedsThermal flags for all pins in all parts.
	
	int ReadParts( CStdioFile * file );													// Done in cpp
	int WriteParts( CStdioFile * file );												// Done in cpp
	int GetNumFootprintInstances( cshape * shape );										// Done in cpp
	int ExportPartListInfo( partlist_info * pli, cpart2 *part0 );						// Done in cpp.  Converted from old CPartList func.
	void ImportPartListInfo( partlist_info * pli, int flags, CDlgLog * log=NULL );		// Done in cpp.  Converted from old CPartList func.
	int CheckPartlist( CString * logstr );
	bool CheckForProblemFootprints();
};
