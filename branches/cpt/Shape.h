// Shape.h : interface for the CShape class
//
#pragma once

#include "stdafx.h"
#include "PolyLine.h"
#include "DisplayList.h"
#include "SMFontUtil.h"
#include "TextList.h"
#include "PcbItem.h"
#include "TextListNew.h"

class ctextlist;

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

// centroid types
enum CENTROID_TYPE
{
	CENTROID_DEFAULT = 0,	// center of pads
	CENTROID_DEFINED		// defined by user
};

// glue spot position types 
enum GLUE_POS_TYPE
{
	GLUE_POS_CENTROID,	// at centroid
	GLUE_POS_DEFINED	// defined by user
};

// structure describing pad flags
struct flag
{
	unsigned int mask : 1;
	unsigned int area : 2;
};

// structure describing adhesive spot
struct glue
{
	GLUE_POS_TYPE type;
	int w, x_rel, y_rel;
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


// CPT2.  I am adjusting classes pad and padstack so that the latter in particular becomes part of the new class hierarchy.  This
// will be useful when converting the footprint editor UI;  instead of using cpin2's to represent pins, as happens in the main
// view, padstacks will represent the pins user sees.
// In honor of this change I'm renaming pad to cpad and padstack to cpadstack.
class cpad
{
public:
	int shape;	// see enum above
	int size_l, size_r, size_h, radius;
	int connect_flag;	// only for copper pads
	dl_element *dl_el;

	cpad();
	BOOL operator==(cpad p);
};

// cpadstack is pads and hole associated with a pin
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
	BOOL exists;		// only used when converting Ivex footprints or editing

	cpadstack(CFreePcbDoc *_doc);
	BOOL operator==(cpadstack p);
	CRect GetBounds();							// CPT2 done in cpp, derived from CShape::GetPadBounds
	void Highlight();
	bool IsPadstack() { return true; }
	cpadstack *ToPadstack() { return this; }
	int GetTypeBit() { return bitPadstack; }
};

// CShape class represents a footprint
//
class CShape
{
	// if variables are added, remember to modify Copy!
public:
	enum { MAX_NAME_SIZE = 59 };	// max. characters
	enum { MAX_PIN_NAME_SIZE = 39 };
	enum { MAX_VALUE_SIZE = 39 };

	CFreePcbDoc *m_doc;		// CPT2
	CString m_name;			// name of shape (e.g. "DIP20")
	CString m_author;
	CString m_source;
	CString m_desc;
	int m_units;			// units used for original definition (MM, NM or MIL)
	int m_sel_xi, m_sel_yi, m_sel_xf, m_sel_yf;			// selection rectangle
	// CPT2.  Reorganizing:  change m_ref and m_value to type ctext*, and get rid of m_ref_size, m_ref_xi, etc. (use m_ref->xi, etc., instead)
	ctext *m_ref;										// CPT:  New system! Use the CText machinery to process "REF" and "VALUE"
	// int m_ref_size, m_ref_xi, m_ref_yi, m_ref_angle;	// ref text params
	// int m_ref_w;										// thickness of stroke for ref text
	ctext *m_value;										// CPT:  New system!
	// int m_value_size, m_value_xi, m_value_yi, m_value_angle;	// value text
	// int m_value_w;												// thickness of stroke for value text
	CENTROID_TYPE m_centroid_type;		// type of centroid
	int m_centroid_x, m_centroid_y;		// position of centroid
	int m_centroid_angle;				// angle of centroid (CCW)
	carray<cpadstack> m_padstack;		// array of padstacks for shape.  CPT2: was CArray<padstack>.  TODO rename padstacks
	carray<cpolyline> m_outline_poly;	// CPT2: was CArray<CPolyLine>.  TODO Rename outline_polys
	ctextlist *m_tl;					// CPT2.  Used to be CTextList*
	CArray<glue> m_glue;				// array of adhesive dots

public:
	CShape(CFreePcbDoc *doc = NULL);
	~CShape();
	void Clear();
	int MakeFromString( CString name, CString str );
	int MakeFromFile( CStdioFile * in_file, CString name, CString file_path, int pos );
	int WriteFootprint( CStdioFile * file );
	int GetNumPins();
	// int GetPinIndexByName( LPCTSTR name );				// CPT2 deprecated
	cpadstack *GetPadstackByName (CString *name);			// CPT2
	// CString GetPinNameByIndex( int index );				// CPT2 deprecated
	CRect GetBounds( BOOL bIncludeLineWidths=TRUE );
	CRect GetCornerBounds();
	// CRect GetPadBounds( int i );							// CPT2 use cpadstack::GetBounds
	CRect GetPadRowBounds( int i, int num );				// CPT2 TODO figure out
	CPoint GetDefaultCentroid();
	CRect GetAllPadBounds();
	void Copy( CShape * shape );	// copy all data from shape
	BOOL Compare( CShape * shape );	// compare shapes, return true if same
	//	HENHMETAFILE CreateMetafile( CMetaFileDC * mfDC, CDC * pDC, int x_size, int y_size );
	HENHMETAFILE CreateMetafile( CMetaFileDC * mfDC, CDC * pDC, CRect const &window, 
		CString ref = "REF", int bDrawSelectionRect=1 );
	// CPT:  the following is apparently leftover scrap?  Cull it out?
	// HENHMETAFILE CreateWarningMetafile( CMetaFileDC * mfDC, CDC * pDC, int x_size, int y_size );
	void GenerateValueParams();	// CPT
};



// CEditShape class represents a footprint whose elements can be edited
//
class CEditShape : public CShape
{
public:
	CEditShape();
	~CEditShape();
	void Clear();
	void Draw( CDisplayList * dlist, SMFontUtil * fontutil );
	void Undraw();
	void Copy( CShape * shape );
	// void HighlightPad( int i );									// CPT2 use cpadstack::Highlight
	void StartDraggingPad( CDC * pDC, int i );
	void CancelDraggingPad( int i );
	void StartDraggingPadRow( CDC * pDC, int i, int num );
	void CancelDraggingPadRow( int i, int num );
	void SelectAdhesive( int idot );
	void StartDraggingAdhesive( CDC * pDC, int idot );
	void CancelDraggingAdhesive( int idot );
	void SelectCentroid();
	void StartDraggingCentroid( CDC * pDC );
	void CancelDraggingCentroid();
	void ShiftToInsertPadName( CString * astr, int n );
	BOOL GenerateSelectionRectangle( CRect * r );

public:
	CDisplayList * m_dlist;
	// CPT2 the following are replaced by drawing elements within individual cpadstack or cpad objects
	// CArray<dl_element*> m_hole_el;			// hole display element 
	// CArray<dl_element*> m_pad_top_el;		// top pad display element 
	// CArray<dl_element*> m_pad_inner_el;		// inner pad display element 
	// CArray<dl_element*> m_pad_bottom_el;	// bottom pad display element 
	// CArray<dl_element*> m_pad_top_mask_el;
	// CArray<dl_element*> m_pad_top_paste_el;
	// CArray<dl_element*> m_pad_bottom_mask_el;
	// CArray<dl_element*> m_pad_bottom_paste_el;
	// CArray<dl_element*> m_pad_sel;		// pad selector
	dl_element * m_centroid_el;			// centroid
	dl_element * m_centroid_sel;		// centroid selector
	CArray<dl_element*> m_dot_el;		// adhesive dots
	CArray<dl_element*> m_dot_sel;		// adhesive dot selectors
};
