// PartListNew.h : CPT2 modified version of CPartList, called cpartlist (at least for now).
// Like cnetlist, this class is going to have many of its functions stripped away and reassigned (to cpart2 and cpin2)
// Original implementation was as a linked list, but I'm going to try a carray instead.

#pragma once
#include "stdafx.h"
#include "Shape.h"
#include "smfontutil.h"
#include "DlgLog.h"
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

// clearance types for GetPadDrawInfo()
enum {
	CLEAR_NORMAL = 0,
	CLEAR_THERMAL,
	CLEAR_NONE
};

// partlist_info is used to hold digest of CPartList 
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


class cpartlist
{
public:
	carray<cpart2> parts;
	CFreePcbDoc *m_doc;			// CPT2
	int m_layers;
	int m_annular_ring;
	cnetlist * m_nlist;
	CDisplayList * m_dlist;
	CMapStringToPtr * m_footprint_cache_map;

	cpartlist( CFreePcbDoc *doc );
	~cpartlist() { }

	void UseNetList( cnetlist * nlist ) { m_nlist = nlist; };
	void SetShapeCacheMap( CMapStringToPtr * shape_cache_map )
		{ m_footprint_cache_map = shape_cache_map; };

	cpart2 * GetPartByName( CString *ref_des );		// Done in cpp.  CPT2 changed arg to CString*
	cpin2 * GetPinByNames ( CString *ref_des, CString *pin_name);										// Done in cpp

	cpart2 * Add( CShape * shape, CString * ref_des, CString *value_text, CString * package, 
		int x, int y, int side, int angle, int visible, int glued );									// Done in cpp.  Removed uid and ref_vis args
	cpart2 * AddFromString( CString * str );															// Done in cpp
	void MarkAllParts( int util );
	void SetNumCopperLayers( int nlayers ) { m_layers = nlayers; }
	void MoveOrigin( int dx, int dy );													// Done in cpp
	static int FootprintLayer2Layer( int fp_layer );		 							// Done in cpp (identical to old version)
	// int RotatePoint( CPoint * p, int angle, CPoint org );
	// int RotateRect( CRect * r, int angle, CPoint p );
	void SetPinAnnularRing( int ring ) { m_annular_ring = ring; }
	int GetPartBoundaries( CRect * part_r );										// Done in cpp  
	void SetThermals();																// CPT2 new.  Updates the bNeedsThermal flags for all pins in all parts.
	
	int ReadParts( CStdioFile * file );													// Done in cpp
	int WriteParts( CStdioFile * file );												// Done in cpp
	int GetNumFootprintInstances( CShape * shape );										// Done in cpp
	int ExportPartListInfo( partlist_info * pli, cpart2 *part0 );						// Done in cpp.  Converted from old CPartList func.
	void ImportPartListInfo( partlist_info * pli, int flags, CDlgLog * log=NULL );		// Done in cpp.  Converted from old CPartList func.
	int CheckPartlist( CString * logstr );
	bool CheckForProblemFootprints();
};
 