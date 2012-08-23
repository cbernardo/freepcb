// Text.h --- header file for classes related most closely to text, many of which are descendants of cpcb_item:
//  ctext, creftext, cvaluetext, ctextlist

#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "PcbItem.h"
#include "DisplayList.h"
#include "Undo.h"
#include "stdafx.h"
#include "Shape.h"
#include "PcbFont.h"					// CPT2 TODO used?
#include "smfontutil.h"
#include "DlgLog.h"
#include "DesignRules.h"

struct stroke;

class ctext: public cpcb_item
{
public:
	int m_x, m_y;
	int m_layer;
	int m_angle;
	bool m_bMirror;
	bool m_bNegative;
	int m_font_size;
	int m_stroke_width;
	CString m_str;
	CArray<stroke> m_stroke;
	CRect m_br;							// CPT2 added.  Bounding rectangle
	SMFontUtil * m_smfontutil;
	bool m_bShown;						// CPT2 added.  Normally true, but may be set false for reftext's and valuetext's that aren't visible
	cpart2 *m_part;						// CPT2 added.  Usually null, but for reftexts, valuetexts, and footprint text objects, it points to
										// the containing part.
	cshape *m_shape;					// CPT2 added.  Other texts are embedded within abstract footprint (cshape) objects

	ctext( CFreePcbDoc *_doc, int _x, int _y, int _angle, 
		BOOL _bMirror, BOOL _bNegative, int _layer, int _font_size, 
		int _stroke_width, SMFontUtil *_smfontutil, CString * _str );
	ctext(CFreePcbDoc *_doc, int _uid);

	bool IsOnPcb();																// Done in cpp
	bool IsText() { return true; }
	ctext *ToText() { return this; }
	int GetTypeBit() { return bitText; }
	int GetLayer();																// Done in cpp:  NB now takes into account the side of any containing part
	cundo_item *MakeUndoItem()
		{ return new cutext(this); }
	cpart2 *GetPart() { return m_part; }

	void Init( CDisplayList * dlist, int x, int y, int angle,					// TODO: rethink relationship with constructor. Removed tid arg.
		int mirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr );
	void Copy(ctext *src);																				// CPT2 new, done in cpp.
	void Move( int x, int y, int angle, BOOL mirror, BOOL negative, int layer, int size=-1, int w=-1 );		// Done in cpp
	void Move( int x, int y, int angle, int size=-1, int w=-1);										// Done in cpp
	void Resize( int size, int w );																	// Done in cpp
	void MoveRelative( int _x, int _y, int _angle=-1, int _size=-1, int _w=-1 );					// Used for reftexts and valuetexts. Done in cpp
	CPoint GetAbsolutePoint();																		// Done in cpp.  Gets absolute position for reftexts and valuetexts.
	CRect GetRect();																				// Done in cpp

	// void Draw( CDisplayList * dlist, SMFontUtil * smfontutil );				// CPT2.  Probably dispensible
	void GenerateStrokes();														// CPT2 new.  Helper for Draw().
	int Draw();																	// Done in cpp
	void GenerateStrokesRelativeTo( cpart2 *p );								// CPT2 new.  Helper for DrawRelativeTo().
	int DrawRelativeTo( cpart2 *p, bool bSelector=true );						// CPT2 new.  Done in cpp
	void Undraw();																// Done in cpp
	void Highlight();															// Done in cpp
	void SetVisible(bool bVis);													// Done in cpp
	void StartDragging( CDC * pDC );											// Done in cpp, derived from CTextList::StartDraggingText
	void CancelDragging();														// Done in cpp, derived from CTextList::CancelDraggingText
	// void GetBounds( CRect &br );												// CPT2.  Use new m_br
};

class creftext: public ctext
{
public:
	creftext( cpart2 *_part, int x, int y, int angle, 
		BOOL bMirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown );			// Done in cpp
	creftext( CFreePcbDoc *doc, int x, int y, int angle, 
		BOOL bMirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown );			// Done in cpp
	creftext(CFreePcbDoc *_doc, int _uid);

	bool IsOnPcb();																				// Done in cpp
	bool IsRefText() { return true; }
	creftext *ToRefText() { return this; }
	int GetTypeBit() { return bitRefText; }
	cundo_item *MakeUndoItem()
		{ return new cureftext(this); }

	int Draw() { return DrawRelativeTo(m_part); }
};

class cvaluetext: public ctext
{
public:
	cvaluetext( cpart2 *_part, int x, int y, int angle, 
		BOOL bMirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown );			// Done in cpp
	cvaluetext( CFreePcbDoc *doc, int x, int y, int angle, 
		BOOL bMirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown );
	cvaluetext(CFreePcbDoc *_doc, int _uid);

	bool IsOnPcb();																				// Done in cpp
	bool IsValueText() { return true; }
	cvaluetext *ToValueText() { return this; }
	int GetTypeBit() { return bitValueText; }
	cundo_item *MakeUndoItem() 
		{ return new cuvaluetext(this); }

	int Draw() { return DrawRelativeTo(m_part); }
};


// class to represent a list of ctexts,  basically a souped-up carray<ctext>
//
class ctextlist
{
public:
	carray<ctext> texts;
	SMFontUtil * m_smfontutil;
	CFreePcbDoc *m_doc;												// CPT2.  Added

	// member functions
	ctextlist( CFreePcbDoc *_doc );
	~ctextlist() { }
	
	ctext * AddText( int x, int y, int angle, bool bMirror, 
					bool bNegative,	int layer, 
					int font_size, int stroke_width, CString * str_ptr, cpart2 *part = NULL, cshape *shape = NULL );
	void ReadTexts( CStdioFile * file );							// Done in cpp
	void WriteTexts( CStdioFile * file );							// Done in cpp
	void MoveOrigin( int dx, int dy );								// Done in cpp
	BOOL GetTextBoundaries( CRect * r );							// Done in cpp
	void ReassignCopperLayers( int n_new_layers, int * layer );
};

