// Shape.h: header for the cshape class.  As of r335 I'm experimenting with making that class part of the cpcb_item hierarchy.
// Also includes classes cpad, stroke, cpadstack, ccentroid and cglue (the last 3 descendants of cpcb_item)

#pragma once

#include "stdafx.h"
#include "DisplayList.h"
#include "SMFontUtil.h"

#define CENTROID_WIDTH 40*NM_PER_MIL	// width of centroid symbol
#define DEFAULT_GLUE_WIDTH 15*NM_PER_MIL	// width of default glue spot

// pad shapes
enum {
	PAD_NONE = 0,
	PAD_ROUND,
	PAD_SQUARE,
	PAD_RECT,
	PAD_RRECT,
	PAD_OVAL,
	PAD_OCTAGON,
	PAD_DEFAULT = 99
};

// pad area connect flags
enum {
	PAD_CONNECT_DEFAULT = 0,
	PAD_CONNECT_NEVER,
	PAD_CONNECT_THERMAL,
	PAD_CONNECT_NOTHERMAL
};

// error returns
enum
{
	PART_NOERR = 0,
	PART_ERR_TOO_MANY_PINS
};

// structure describing pad flags
struct flag
{
	unsigned int mask : 1;
	unsigned int area : 2;
};

// structure describing stroke (ie. line segment)
struct stroke
{
	int layer;				// PCB or footprint layer (if known)
	int w, xi, yi, xf, yf;	// thickness + endpoints
	int type;				// CDisplayList g_type
	dl_element * dl_el;		// pointer to graphic element for stroke;
};

// structure describing mounting hole
// only used during conversion of Ivex files
struct mtg_hole
{
	int pad_shape;		// used for pad on top and bottom
	int x, y, diam, pad_diam;
};

class cpad
{
public:
	int shape;	// see enum above
	int size_l, size_r, size_h, radius;
	int connect_flag;	// only for copper pads
	dl_element *dl_el;

	cpad();
	BOOL operator==(cpad p);
	void CopyToArray(int *p)
	{
		// CPT2 Stupid business required in order to copy pads over into undo-items.  
		p[0] = shape; 
		p[1] = size_l; p[2] = size_r; p[3] = size_h;
		p[4] = radius;
		p[5] = connect_flag;
	}
	void CopyFromArray(int *p)
	{
		// CPT2 similarly
		shape = p[0];
		size_l = p[1]; size_r = p[2]; size_h = p[3];
		radius = p[4];
		connect_flag = p[5];
	}
};

// A cpadstack is a set of pads and hole associated with a footprint/shape's pins
class cpadstack: public cpcb_item
{
public:
	CString name;		// identifier such as "1" or "B24"
	int hole_size;		// 0 = no hole (i.e SMT)
	int x_rel, y_rel;	// position relative to part origin
	int angle;			// orientation: 0=left, 90=top, 180=right, 270=bottom
	cpad top, top_mask, top_paste;
	cpad bottom, bottom_mask, bottom_paste;
	cpad inner;
	cshape *shape;		// CPT2 new
	BOOL exists;		// only used when converting Ivex footprints or editing

	cpadstack(cshape *_shape);
	cpadstack(cpadstack *src, cshape *_shape);
	cpadstack(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();								// CPT2 done in cpp.
	bool IsPadstack() { return true; }
	cpadstack *ToPadstack() { return this; }
	int GetTypeBit() { return bitPadstack; }
	cundo_item *MakeUndoItem()
		{ return new cupadstack(this); }

	bool SameAs(cpadstack *ps);
	CRect GetBounds();							// CPT2 done in cpp, derived from old CShape::GetPadBounds
	void Highlight();
	void Copy(cpadstack *src, bool bCopyName=true);
};

class ccentroid : public cpcb_item
{
public:
	// Represents centroids within footprints.
	CENTROID_TYPE m_type;
	int m_x, m_y;
	int m_angle;				// angle of centroid (CCW)
	cshape *m_shape;

	ccentroid(cshape *shape); 
	ccentroid(CFreePcbDoc *_doc, int _uid)
		: cpcb_item (_doc, _uid)
		{ m_shape = NULL; }
	virtual bool IsOnPcb();
	bool IsCentroid() { return true; }
	ccentroid *ToCentroid() { return this; }
	int GetTypeBit() { return bitCentroid; }
	cundo_item *MakeUndoItem()
		{ return new cucentroid(this); }

	int Draw();
	void Highlight();
	void StartDragging( CDC *pDC );
	void CancelDragging();
};

class cglue : public cpcb_item
{
public:
	// Represents adhesive dots within footprints.
	GLUE_POS_TYPE type;
	int w, x, y;
	cshape *shape;

	cglue(cshape *_shape, GLUE_POS_TYPE _type, int _w, int _x, int _y);
	cglue(CFreePcbDoc *_doc, int _uid)
		: cpcb_item (_doc, _uid)
		{ shape = NULL; }
	cglue(cglue *src, cshape *_shape);

	virtual bool IsOnPcb();
	bool IsGlue() { return true; }
	cglue *ToGlue() { return this; }
	int GetTypeBit() { return bitGlue; }
	cundo_item *MakeUndoItem()
		{ return new cuglue(this); }

	int Draw();
	void Highlight();
	void StartDragging( CDC *pDC );
	void CancelDragging();
};


// The cshape class represents a footprint.  As of r335, I'm merging it into the cpcb_item hierarchy.  This will make it easier to incorporate
// the changing cast of footprints into the existing garbage-collection and undo machinery.   I'm ditching the old distinction between CShape
// and CEditShape.
//
class cshape: public cpcb_item
{
	// if variables are added, remember to modify Copy!
public:
	enum { MAX_NAME_SIZE = 59 };	// max. characters
	enum { MAX_PIN_NAME_SIZE = 39 };
	enum { MAX_VALUE_SIZE = 39 };

	CString m_name;			// name of shape (e.g. "DIP20")
	CString m_author;
	CString m_source;
	CString m_desc;
	int m_units;			// units used for original definition (MM, NM or MIL)
	int m_sel_xi, m_sel_yi, m_sel_xf, m_sel_yf;			// selection rectangle
	// CPT2.  Reorganizing:  change m_ref and m_value to type ctext*, and get rid of m_ref_size, m_ref_xi, etc. (use m_ref->xi, etc., instead)
	creftext *m_ref;	
	cvaluetext *m_value;
	ccentroid *m_centroid;
	carray<cpadstack> m_padstacks;		// array of padstacks for shape.  CPT2: was CArray<padstack>.
	carray<coutline> m_outlines;		// CPT2: was CArray<CPolyLine>.
	ctextlist *m_tl;					// CPT2.  Used to be CTextList*
	carray<cglue> m_glues;				// array of adhesive dots.  CPT2 converted from old type (CArray<glue>)

public:
	cshape( CFreePcbDoc *doc );
	cshape( cshape *src );										// Copy constructor (create a copy of "other" for editing)
	cshape( CFreePcbDoc *doc, CString *name );					// CPT2 r317 new.  Create a blank shape with the given name
	cshape( CFreePcbDoc *doc, int uid );						// For the sake of undo functionality
	~cshape();

	bool IsShape() { return true; }
	cshape *ToShape() { return this; }
	int GetTypeBit() { return bitShape; }
	virtual bool IsOnPcb();
	cundo_item *MakeUndoItem()
		{ return new cushape(this); }
	void SaveUndoInfo();

	void Clear();
	void MarkConstituents(int util);
	int MakeFromString( CString name, CString str );
	int MakeFromFile( CStdioFile * in_file, CString name, CString file_path, int pos );
	int WriteFootprint( CStdioFile * file );
	int GetNumPins();
	cpadstack *GetPadstackByName (CString *name);			// CPT2
	CRect GetBounds( BOOL bIncludeLineWidths=TRUE );
	CRect GetCornerBounds();
	// CRect GetPadRowBounds( int i, int num );				// CPT2 TODO figure out
	CPoint GetDefaultCentroid();
	CRect GetAllPadBounds();
	void Copy( cshape * src );	// copy all data from shape.
	bool SameAs( cshape * shape );	// compare shapes, return true if same
	HENHMETAFILE CreateMetafile( CMetaFileDC * mfDC, CDC * pDC, CRect const &window, 
		CString ref = "REF", int bDrawSelectionRect=1 );
	void GenerateValueParams();	
	
	int Draw();													// CPT2 removed args.
	void Undraw();
	void StartDraggingPadRow( CDC * pDC, carray<cpadstack> *row );	// CPT2 converted (arg change)
	void CancelDraggingPadRow( carray<cpadstack> *row );			// CPT2 converted (new arg)
	void ShiftToInsertPadName( CString * astr, int n );				// CPT2 converted
	bool GenerateSelectionRectangle( CRect * r );					// Done in cpp
};

class cshapelist
{
	// CPT2 r335 new.  A list of cshapes, analogous to cpartlist, cnetlist, ctextlist.  An object of this type will take the place
	// of CFreePcbDoc::m_footprint_cache_map.  For now the searching for footprints by name is probably less efficient than it was with
	// the old CMapStringToPtr, but I can't imagine it being a big bottleneck.  (If it's essential more efficient searching can be 
	// grafted into this class later.)
public:
	carray<cshape> shapes;
	CFreePcbDoc *m_doc;			// CPT2

	cshapelist( CFreePcbDoc *doc )
		{ m_doc = doc; }
	cshape *GetShapeByName( CString *name );
	void ReadShapes( CStdioFile * file, bool bFindSection = true );
	void WriteShapes( CStdioFile * file );
};

