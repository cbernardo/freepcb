// PartListNew.h : CPT2 modified version of CPartList, called cpartlist (at least for now).
// Like cnetlist, this class is going to have many of its functions stripped away and reassigned (to cpart2 and cpin2)
// Original implementation was as a linked list, but I'm going to try a carray instead.

#pragma once
#include "stdafx.h"
#include "Shape.h"
#include "smfontutil.h"
#include "DlgLog.h"
#include "UndoList.h"
#include "Cuid.h"
#include "PcbItem.h"

#define MAX_REF_DES_SIZE 39

class cpart2;
class cpartlist;
class cnetlist;
class cnet2;
class carea2;
class CFreePcbDoc;
template <class T> class carray;

#include "DesignRules.h"

#if 0													// CPT2 the following overlaps with existing PartList.h code, so is suppressed for now

// clearance types for GetPadDrawInfo()
enum {
	CLEAR_NORMAL = 0,
	CLEAR_THERMAL,
	CLEAR_NONE
};

// struct to hold data to undo an operation on a part
//
struct undo_part {
	int size;				// size of this instance of the struct
	id m_id;				// instance id for this part
	BOOL visible;			// FALSE=hide part
	int x,y;				// position of part origin on board 
	int side;				// 0=top, 1=bottom
	int angle;				// orientation (degrees)
	BOOL glued;				// TRUE=glued in place
	BOOL m_ref_vis;			// TRUE = ref shown
	int m_ref_xi, m_ref_yi, m_ref_angle, m_ref_size, m_ref_w;	// ref text
	int m_ref_layer;
	BOOL m_value_vis;		// TRUE = value shown
	int m_value_xi, m_value_yi, m_value_angle, m_value_size, m_value_w;	// value text
	int m_value_layer;
	char ref_des[MAX_REF_DES_SIZE+1];	// ref designator such as "U3"
	char new_ref_des[MAX_REF_DES_SIZE+1];	// if ref designator will be changed
	char package[CShape::MAX_NAME_SIZE+1];		// package
	char value[CShape::MAX_VALUE_SIZE+1];		// package
	char shape_name[CShape::MAX_NAME_SIZE+1];	// name of shape
	CShape * shape;			// pointer to the footprint of the part, may be NULL
	CPartList * m_plist;	// parent cpartlist	
	// here goes array of char[npins][40] for attached net names
	// followed by array of int[npins] for pin UIDs
};

// partlist_info is used to hold digest of CPartList 
// for editing in dialogs, or importing from netlist file
// notes:
//	package may be "" if no package assigned
//	shape may be NULL if no footprint assigned
//	may have package but no footprint, but not the reverse
typedef struct {
	cpart2 * part;		// pointer to original part, or NULL if new part added.  CPT2 changed cpart to cpart2
	CString ref_des;	// ref designator string
	int ref_size;		// size of ref text characters
	int ref_width;		// stroke width of ref text characters
	int ref_layer;		// ref layer
	CString package;	// package (from original imported netlist, don't edit)
	CString value;		// value (from original imported netlist, don't edit)
	BOOL value_vis;		// visibility of value
	BOOL ref_vis;		// CPT: visibility of ref text
	int value_layer;	// value layer
	CShape * shape;		// pointer to shape (may be edited)
	BOOL deleted;		// flag to indicate that part was deleted
	BOOL bShapeChanged;	// flag to indicate that the shape has changed
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

#endif

// this is the partlist class.  CPT2 version has new name (cpartlist not CPartList)
class cpartlist
{
public:
	enum {
		NOT_CONNECTED = 0,		// pin not attached to net
		ON_NET = 1,				// pin is attached to a net
		TRACE_CONNECT = 2,		// pin connects to trace on this layer
		AREA_CONNECT = 4		// pin connects to copper area on this layer
	};
	carray<cpart2> parts;
	CFreePcbDoc *m_doc;			// CPT2
	int m_layers;
	int m_annular_ring;
	cnetlist * m_nlist;
	CDisplayList * m_dlist;
	CMapStringToPtr * m_footprint_cache_map;

	enum { 
		UNDO_PART_DELETE=1, 
		UNDO_PART_MODIFY, 
		UNDO_PART_ADD };	// undo types

	cpartlist( CFreePcbDoc *doc );
	~cpartlist() { }

	void UseNetList( cnetlist * nlist ) { m_nlist = nlist; };
	void SetShapeCacheMap( CMapStringToPtr * shape_cache_map )
		{ m_footprint_cache_map = shape_cache_map; };

	// int GetNumParts(){ return m_size; }			// CPT2 use parts.GetSize()
	cpart2 * GetPartByName( CString *ref_des );		// Done in cpp.  CPT2 changed arg to CString*
	// cpart * GetFirstPart();						// CPT2 use citer<cpart2>
	// cpart * GetNextPart( cpart * part );
	// cpart * GetPartByUID( int uid );				// CPT2 use parts.FindByUID()
	cpin2 * GetPinByNames ( CString *ref_des, CString *pin_name);										// Done in cpp

	// cpart * Add( int uid=-1 );					// CPT2 ditch?
	cpart2 * Add( CShape * shape, CString * ref_des, CString *value_text, CString * package, 
		int x, int y, int side, int angle, int visible, int glued );									// Done in cpp.  Removed uid and ref_vis args
	cpart2 * AddFromString( CString * str );															// Done in cpp
	// int SetPartData( cpart * part, CShape * shape, CString * ref_des, CString * package, 
	//				int x, int y, int side, int angle, int visible, int glued, bool ref_vis  );			// CPT2.  Use cpart2::SetData
	void MarkAllParts( int util );
	// int Remove( cpart * element );								// CPT2.  Use cpart2::Remove().
	// void RemoveAllParts();										// CPT2.  Use parts.RemoveAll().  Assuming that nets will be getting axed too.
	// int HighlightPart( cpart * part );							// CPT2. Use cpart2::Highlight()
	// void MakePartVisible( cpart * part, BOOL bVisible );			// CPT2. Use cpart2::SetVisible()

	void SetNumCopperLayers( int nlayers ) { m_layers = nlayers; }
	// int SelectPart( cpart * part );													// CPT2 Obsolete
	// int SelectRefText( cpart * part );												// CPT2 Obsolete
	// int SelectValueText( cpart * part );												// CPT2 Obsolete
	// int HighlightPad( cpart * part, int i );											// CPT2 Use cpin2::Highlight
	void HighlightAllPadsOnNet( cnet * net );
	// BOOL TestHitOnPad( cpart * part, CString * pin_name, int x, int y, int layer );	// CPT2 Use cpin2::TestHit
	void MoveOrigin( int dx, int dy );													// Done in cpp
	// int Move( cpart * part, int x, int y, int angle, int side );						// CPT2 Use cpart2::Move
	// int MoveRefText( cpart * part, int x, int y, int angle, int size, int w );		// CPT2 Use cpart2::MoveRefText
	// int MoveValueText( cpart * part, int x, int y, int angle, int size, int w );		// CPT2 Use cPart2::MoveValueText
	// void ResizeRefText( cpart * part, int size, int width, BOOL vis=TRUE );			// CPT2 Use cpart2::ResizeRefText
	// void ResizeValueText( cpart * part, int size, int width, BOOL vis=TRUE );		// CPT2 sim.
	// void SetValue( cpart * part, CString * value, int x, int y, int angle, int size, 
	//	int w, BOOL vis, int layer );													// CPT2. Use cpart2::SetValue
	// int DrawPart( cpart * el );														// CPT2. Use cpart2::Draw
	// int UndrawPart( cpart * el );													// CPT2. Use cpart2::Undraw
	static int FootprintLayer2Layer( int fp_layer );		 							// Done in cpp (identical to old version)
	// void PartFootprintChanged( cpart * part, CShape * shape );						// CPT2. Use cpart2::ChangeFootprint
	// void ChangeFootprint( CShape * shape );
	// void RefTextSizeChanged( CShape * shape );
	int RotatePoint( CPoint * p, int angle, CPoint org );
	int RotateRect( CRect * r, int angle, CPoint p );
	// int GetSide( cpart * part );								// CPT2 Use cpart2 functions for the following 9
	// int GetAngle( cpart * part );
	// int GetRefAngle( cpart * part );
	// int GetValueAngle( cpart * part );
	// CPoint GetRefPoint( cpart * part );
	// CPoint GetValuePoint( cpart * part );
	// CRect GetValueRect( cpart * part );
	// int GetValuePCBLayer( cpart * part );
	// int GetRefPCBLayer( cpart * part );
	// CPoint GetPinPoint(  cpart * part, LPCTSTR pin_name );	// CPT2 Just use pin->x,y
	// CPoint GetPinPoint(  cpart * part, int pin_index );
	// CPoint GetCentroidPoint(  cpart * part );				// CPT2 use cpart2::GetCentroidPoint()
	// CPoint GetGluePoint(  cpart * part, int iglue );			// CPT2 use cpart2::GetGluePoint()
	// int GetPinLayer( cpart * part, CString * pin_name );		// CPT2 Just use pin->pad_layer
	// int GetPinLayer( cpart * part, int pin_index );
	// cnet * GetPinNet( cpart * part, CString * pin_name );	// CPT2 Just use pin->net
	// cnet * GetPinNet( cpart * part, int pin_index );
	// int GetPinWidth( cpart * part, CString * pin_name );		// CPT2 Use cpin2::GetWidth()
	void SetPinAnnularRing( int ring ) { m_annular_ring = ring; }
	// int GetPartBoundingRect( cpart * part, CRect * part_r );			// CPT2 This is barely used.  Anyway, move it to cpart2...
	int GetPartBoundaries( CRect * part_r );							// Done in cpp  
	// int GetPinConnectionStatus( cpart * part, CString * pin_name, int layer );	// CPT2 probably defunct
	/* int GetPadDrawInfo( cpart * part, int ipin, int layer,						// Ditto
							  BOOL bUse_TH_thermals, BOOL bUse_SMT_thermals,
							  int mask_clearance, int paste_mask_shrink,
							  int * type=0, int * x=0, int * y=0, int * w=0, int * l=0, int * r=0, int * hole=0,
							  int * angle=0, cnet ** net=0, 
							  int * connection_status=0, int * pad_connect_flag=0, 
							  int * clearance_type=0 );
	*/
	// int StartDraggingPart( CDC * pDC, cpart * part, BOOL bRatlines,  // CPT2 use part2::StartDragging()
	//							 BOOL bBelowPinCount, int pin_count );
	// int StartDraggingRefText( CDC * pDC, cpart * part );
	// int StartDraggingValue( CDC * pDC, cpart * part ) { return 0; }
	// int CancelDraggingPart( cpart * part );
	// int CancelDraggingRefText( cpart * part );
	// int CancelDraggingValue( cpart * part );
	// int StopDragging();
	void SetThermals();																// CPT2 new.  Updates the bNeedsThermal flags for all pins in all parts.
	
	int ReadParts( CStdioFile * file );													// Done in cpp
	int WriteParts( CStdioFile * file );												// Done in cpp
	int GetNumFootprintInstances( CShape * shape );										// Done in cpp
	// void PurgeFootprintCache();														// CPT2 obsolete?
	int ExportPartListInfo( partlist_info * pli, cpart2 *part0 );						// Done in cpp.  Converted from old CPartList func.
	void ImportPartListInfo( partlist_info * pli, int flags, CDlgLog * log=NULL );		// Done in cpp.  Converted from old CPartList func.
	int SetPartString( cpart * part, CString * str );
	int CheckPartlist( CString * logstr ) { return 0; }
	BOOL CheckForProblemFootprints() { return false; }
	void DRC( CDlgLog * log, int copper_layers, 
		int units, BOOL check_unrouted,
		CArray<CPolyLine> * board_outline,
		DesignRules * dr, DRErrorList * DRElist ) { }
	// CPT new helper for DRC():
	void CheckBrokenArea(carea *a, cnet *net, CDlgLog * log, int units, DRErrorList * drelist, long &nerrors);
	// end CPT

	undo_part * CreatePartUndoRecord( cpart * part, CString * new_ref_des ) { return NULL; }
	static void PartUndoCallback( int type, void * ptr, BOOL undo ) { }
};
 