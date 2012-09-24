// Part.h --- header file for classes related most closely to parts, many of which are descendants of CPcbItem:
//  CPart, CPin, CPartList

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


// CPin: describes a pin on the main editor screen, associated with a particular part and possibly assigned to a net.
class CPin : public CPcbItem
{
public:
	// CString ref_des;		// reference designator such as 'U1'.  CPT2 READ OFF of part->ref_des
	CString pin_name;		// pin name such as "1" or "A23"
	CPart * part;			// pointer to part containing the pin.
	int x, y;				// position on PCB
	CPadstack *ps;			// CPT2. Seems useful to have this here.  Constructor sets it up
	//** AMW3 since we now have ps, we don't really need to save the pad_layer
	//**	int pad_layer;	// CPT2. layer of pad on this pin (was in CVertex).  
							// Constructor sets this based on ps. Possible values LAY_PAD_THRU, LAY_TOP_COPPER, LAY_BOTTOM_COPPER
	CNet * net;				// pointer to net, or NULL if not assigned.
	bool bNeedsThermal;		// CPT2 new.  Set to true if there's an areas from the same network that occupies this same point.
	CDLElement * dl_hole;	// pointer to graphic element for hole
	CArray<CDLElement*> dl_els;	// array of pointers to graphic elements for pads
	CDLElement *dl_thermal; // CPT2 new.  The thermal drawn with this pin.
	drc_pin drc;			// drc info.

	enum {
		// Return vals for GetConnectionStatus().
		NOT_CONNECTED = 0,		// pin not attached to net
		ON_NET = 1,				// pin is attached to a net
		TRACE_CONNECT = 2,		// pin connects to trace on this layer
		AREA_CONNECT = 4		// pin connects to copper area on this layer
	};


	CPin(CPart *_part, CPadstack *_ps, CNet *_net);					// CPT2. Added args.
	CPin(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	int PinLayer();		//** AMW3 replaces m_pad_layer (see above)			
	bool IsPin() { return true; }
	CPin *ToPin() { return this; }
	int GetTypeBit() { return bitPin; }
	CNet *GetNet() { return net; }
	CUndoItem *MakeUndoItem()
		{ return new CUPin(this); }
	void MustRedraw();						// Overrides the default behavior of CPcbItem::MustRedraw()
	int GetLayer();		//** AMW3 changed
	//** { return pad_layer; }
	int GetWidth();	
	void GetVtxs(CHeap<CVertex> *vtxs);

	void Highlight();

	bool TestHit(int x, int y, int layer);
	void SetPosition();	
	bool SetNeedsThermal();
	void Disconnect();
	CSeg *AddRatlineToPin( CPin *p2 );

	bool GetDrawInfo(int layer,	bool bUse_TH_thermals, bool bUse_SMT_thermals,				// Used during DRC
		  int mask_clearance, int paste_mask_shrink,
		  int * type=0, int * w=0, int * l=0, int * r=0, int * hole=0,						// CPT2 got rid of "x" and "y" args
		  int * connection_status=0, int * pad_connect_flag=0, int * clearance_type=0 );
	int GetConnectionStatus( int layer );
};


// class CPart represents a part
class CPart: public CPcbItem
{
public:
	CPartList * m_pl;	// parent partlist
	CHeap<CPin> pins;	// array of all pins in part
	BOOL visible;		// 0 to hide part
	int x,y;			// position of part origin on board
	int side;			// 0=top, 1=bottom
	int angle;			// orientation, degrees CW
	BOOL glued;

	CString ref_des;		// ref designator such as "U3"
	CRefText *m_ref;		// CPT2 added. It's convenient to have an object for the ref-text on which we can invoke CText methods.
							// Note that m_ref->m_x, m_ref->m_y, etc. will be _relative_ positions
	CString value_text;
	CValueText *m_value;	// CPT2 added.  Analogous to m_ref
	CTextList *m_tl;		// CPT2 added.  Objects for each of the texts derived from the footprint.

	CString package;						// package (from original imported netlist, may be "")
	CShape * shape;							// pointer to the footprint of the part, may be NULL.  CPT2 I have yet to see a case where it is null, though.
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


	CPart( CPartList * pl );
	CPart(CFreePcbDoc *_doc, int _uid);
	~CPart();

	virtual bool IsOnPcb();
	bool IsPart() { return true; }
	CPart *ToPart() { return this; }
	int GetTypeBit() { return bitPart; }
	void Remove(bool bEraseConnects, bool bErasePart=true);
	CUndoItem *MakeUndoItem()
		{ return new CUPart(this); }
	void SaveUndoInfo(bool bSaveAttachedConnects = true);

	void Move( int x, int y, int angle = -1, int side = -1);
	void PartMoved( int dx=1, int dy=1, int d_angle=0, int d_side=0 );
	void SetData( CShape * shape, CString * ref_des, CString *value_txt, CString * package, 
	     		  int x, int y, int side, int angle, int visible, int glued );
	void SetValue( CString * value, int x, int y, int angle, int size, 
				  int w, BOOL vis, int layer );	
	void InitPins();
	void MarkConstituents(int util);

	int GetNumOutlineStrokes();
	void OptimizeConnections( BOOL bLimitPinCount=TRUE, BOOL bVisibleNetsOnly=TRUE );
	CPin *GetPinByName(CString *name);
	CPoint GetCentroidPoint();
	CPoint GetGluePoint( CGlue *g );
	int GetBoundingRect( CRect * part_r );
	CRect CalcSelectionRect();
	bool IsAttachedToConnects();
	void ChangeFootprint( CShape * shape );	

	int Draw();	
	void Undraw();
	void Highlight();
	void SetVisible(bool bVis);
	void StartDragging( CDC * pDC, BOOL bRatlines, BOOL bBelowPinCount, int pin_count );
	void CancelDragging();

	void MakeString( CString *str );
	void GetDRCInfo();					// CPT2 new.  Extracted some of the code formerly in DRC().
};


// partlist_info is used to hold digest of CPartList 
// for editing in dialogs, or importing from netlist file
// notes:
//	package may be "" if no package assigned
//	shape may be NULL if no footprint assigned
//	may have package but no footprint, but not the reverse
typedef struct {
	CPart * part;		// pointer to original part, or NULL if new part added.
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
	CShape * shape;		// pointer to shape (may be edited)
	bool bShapeChanged;	// Set if "part->shape" gets its internals modified.
	CString shape_name;			// CPT2 new.  Included separately from shape, in case we read from a netlist file and didn't find 
								// a shape to put into "shape"
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


class CPartList
{
public:
	CHeap<CPart> parts;
	CFreePcbDoc *m_doc;
	CNetList * m_nlist;
	CDisplayList * m_dlist;

	CPartList( CFreePcbDoc *doc );

	void UseNetList( CNetList * nlist ) { m_nlist = nlist; }
	CPart * GetPartByName( CString *ref_des );	
	CPin * GetPinByNames ( CString *ref_des, CString *pin_name );
	CPart * Add( CShape * shape, CString * ref_des, CString *value_text, CString * package, 
		int x, int y, int side, int angle, int visible, int glued );
	CPart * AddFromString( CString * str );
	void MarkAllParts( int util );
	void MoveOrigin( int dx, int dy );
	static int FootprintLayer2Layer( int fp_layer );
	int GetPartBoundaries( CRect * part_r );									// CPT2 TODO check, seems fishy.
	void SetThermals();															// CPT2 new.  Updates the bNeedsThermal flags for all pins in all parts.
	
	int ReadParts( CStdioFile * file );	
	int WriteParts( CStdioFile * file );
	int GetNumFootprintInstances( CShape * shape );
	int ExportPartListInfo( partlist_info * pli, CPart *part0 );
	void ImportPartListInfo( partlist_info * pli, int flags, CDlgLog * log=NULL );
	int CheckPartlist( CString * logstr );
	bool CheckForProblemFootprints();
};
