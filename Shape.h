// Shape.h: header for the CShape class.  As of r335 I'm experimenting with making that class part of the CPcbItem hierarchy.
// Also includes classes CPad, stroke, CPadstack, CCentroid and CGlue (the last 3 descendants of CPcbItem)

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

// structure describing stroke (ie. line segment)
struct stroke
{
	int layer;				// PCB or footprint layer (if known)
	int w, xi, yi, xf, yf;	// thickness + endpoints
	int type;				// CDisplayList g_type
	CDLElement * dl_el;		// pointer to graphic element for stroke;
};

// structure describing mounting hole
// only used during conversion of Ivex files
struct mtg_hole
{
	int pad_shape;		// used for pad on top and bottom
	int x, y, diam, pad_diam;
};

class CPad
{
public:
	int shape;	// see enum above
	int size_l, size_r, size_h, radius;
	int connect_flag;	// only for copper pads
	CDLElement *dl_el;

	CPad();
	BOOL operator==(CPad p);
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

// A CPadstack is a set of pads and hole associated with a footprint/shape's pins
class CPadstack: public CPcbItem
{
public:
	CString name;					// identifier such as "1" or "B24"
	int hole_size;					// 0 = no hole (i.e SMT)
	int x_rel, y_rel;				// position relative to part origin
	int angle;						// orientation: 0=left, 90=top, 180=right, 270=bottom
	CPad top, top_mask, top_paste;
	CPad bottom, bottom_mask, bottom_paste;
	CPad inner;
	CShape *shape;					// CPT2 new
	BOOL exists;					// only used when converting Ivex footprints or editing

	CPadstack(CShape *_shape);
	CPadstack(CPadstack *src, CShape *_shape);
	CPadstack(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsPadstack() { return true; }
	CPadstack *ToPadstack() { return this; }
	int GetTypeBit() { return bitPadstack; }
	CUndoItem *MakeUndoItem()
		{ return new CUPadstack(this); }

	bool SameAs(CPadstack *ps);
	CRect GetBounds();
	void Highlight();
	void Copy(CPadstack *src, bool bCopyName=true);
	void SetVisible(bool bVis);
};

class CCentroid : public CPcbItem
{
public:
	// Represents centroids within footprints.
	CENTROID_TYPE m_type;
	int m_x, m_y;
	int m_angle;				// angle of centroid (CCW)
	CShape *m_shape;

	CCentroid(CShape *shape); 
	CCentroid(CFreePcbDoc *_doc, int _uid)
		: CPcbItem (_doc, _uid)
		{ m_shape = NULL; }
	virtual bool IsOnPcb();
	bool IsCentroid() { return true; }
	CCentroid *ToCentroid() { return this; }
	int GetTypeBit() { return bitCentroid; }
	CUndoItem *MakeUndoItem()
		{ return new CUCentroid(this); }

	int Draw();
	void Highlight();
	void StartDragging( CDC *pDC );
	void CancelDragging();
	void SetVisible(bool bVis);
};

class CGlue : public CPcbItem
{
public:
	// Represents adhesive dots within footprints.
	GLUE_POS_TYPE type;
	int w, x, y;
	CShape *shape;

	CGlue(CShape *_shape, GLUE_POS_TYPE _type, int _w, int _x, int _y);
	CGlue(CFreePcbDoc *_doc, int _uid)
		: CPcbItem (_doc, _uid)
		{ shape = NULL; }
	CGlue(CGlue *src, CShape *_shape);

	virtual bool IsOnPcb();
	bool IsGlue() { return true; }
	CGlue *ToGlue() { return this; }
	int GetTypeBit() { return bitGlue; }
	CUndoItem *MakeUndoItem()
		{ return new CUGlue(this); }

	int Draw();
	void Highlight();
	void StartDragging( CDC *pDC );
	void CancelDragging();
	void SetVisible(bool bVis);
};


// The CShape class represents a footprint.  CPT2 As of r335, I'm merging it into the CPcbItem hierarchy.  This will make it easier to incorporate
// the changing cast of footprints into the existing garbage-collection and undo machinery.   I'm ditching the old distinction between CShape
// and CEditShape.
//
class CShape: public CPcbItem
{
	// if variables are added, remember to modify Copy!
public:
	enum { MAX_NAME_SIZE = 59 };
	enum { MAX_PIN_NAME_SIZE = 39 };
	enum { MAX_VALUE_SIZE = 39 };

	CString m_name;										// name of shape (e.g. "DIP20")
	CString m_author;
	CString m_source;
	CString m_desc;
	int m_units;										// units used for original definition (MM, NM or MIL)
	int m_sel_xi, m_sel_yi, m_sel_xf, m_sel_yf;			// selection rectangle
	// CPT2.  Reorganizing:  change m_ref and m_value to type CRefText*/CValueText*, and 
	// get rid of m_ref_size, m_ref_xi, etc. (use m_ref->xi, etc., instead)
	CRefText *m_ref;	
	CValueText *m_value;
	CCentroid *m_centroid;
	CHeap<CPadstack> m_padstacks;						// array of padstacks for shape.  CPT2: was CArray<padstack>.
	CHeap<COutline> m_outlines;						// CPT2: was CArray<CPolyLine>.
	CTextList *m_tl;
	CHeap<CGlue> m_glues;								// array of adhesive dots.  CPT2 converted from old type (CArray<glue>)

public:
	CShape( CFreePcbDoc *doc );
	CShape( CShape *src );										// Copy constructor (create a copy of "other" for editing)
	CShape( CFreePcbDoc *doc, CString *name );					// CPT2 r317 new.  Create a blank shape with the given name
	CShape( CFreePcbDoc *doc, int uid );						// For the sake of undo functionality
	~CShape();

	bool IsShape() { return true; }
	CShape *ToShape() { return this; }
	int GetTypeBit() { return bitShape; }
	virtual bool IsOnPcb();
	CUndoItem *MakeUndoItem()
		{ return new CUShape(this); }
	void SaveUndoInfo();

	void Clear();
	void MakeDummy();
	void MarkConstituents(int util);
	int MakeFromString( CString name, CString str );
	int MakeFromFile( CStdioFile * in_file, CString name, CString file_path, int pos );
	int WriteFootprint( CStdioFile * file );
	int GetNumPins();
	CPadstack *GetPadstackByName (CString *name);					// CPT2
	CRect GetBounds( BOOL bIncludeLineWidths=TRUE );
	CRect GetCornerBounds();
	CPoint GetDefaultCentroid();
	CRect GetAllPadBounds();
	void Copy( CShape * src );										// copy all data from shape.
	bool SameAs( CShape * shape );									// compare shapes, return true if same
	HENHMETAFILE CreateMetafile( CMetaFileDC * mfDC, CDC * pDC, CRect const &window, 
		CString ref = "REF", int bDrawSelectionRect=1 );
	void GenerateValueParams();	
	
	int Draw();														// CPT2 removed args.
	void Undraw();
	// void StartDraggingPadRow( CDC * pDC, CHeap<CPadstack> *row );
	// void CancelDraggingPadRow( CHeap<CPadstack> *row );			// CPT2 converted (new arg)
	void ShiftToInsertPadName( CString * astr, int n );				// CPT2 converted
	bool GenerateSelectionRectangle( CRect * r );
};

class CShapeList
{
	// CPT2 r335 new.  A list of cshapes, analogous to CPartList, CNetList, CTextList.  An object of this type will take the place
	// of CFreePcbDoc::m_footprint_cache_map, under the name m_slist.  For now the searching for footprints by name is probably less 
	// efficient than it was with
	// the old CMapStringToPtr, but I can't imagine it being a big bottleneck.  (If it's essential more efficient searching can be 
	// grafted into this class later.)
public:
	CHeap<CShape> shapes;
	CFreePcbDoc *m_doc;			// CPT2

	CShapeList( CFreePcbDoc *doc )
		{ m_doc = doc; }
	CShape *GetShapeByName( CString *name );
	void ReadShapes( CStdioFile * file, bool bFindSection = true );
	void WriteShapes( CStdioFile * file );
};

