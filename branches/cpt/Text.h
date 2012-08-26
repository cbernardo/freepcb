// Text.h --- header file for classes related most closely to text, many of which are descendants of CPcbItem:
//  CText, CRefText, CValueText, CTextList

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

class CText: public CPcbItem
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
	CPart *m_part;						// CPT2 added.  Usually null, but for reftexts, valuetexts, and footprint text objects, it points to
										// the containing part.
	CShape *m_shape;					// CPT2 added.  Other texts are embedded within abstract footprint (CShape) objects

	CText( CFreePcbDoc *_doc, int _x, int _y, int _angle, 
		BOOL _bMirror, BOOL _bNegative, int _layer, int _font_size, 
		int _stroke_width, SMFontUtil *_smfontutil, CString * _str );
	CText(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsText() { return true; }
	CText *ToText() { return this; }
	int GetTypeBit() { return bitText; }
	int GetLayer();																// NB now takes into account the side of any containing part
	CUndoItem *MakeUndoItem()
		{ return new CUText(this); }
	CPart *GetPart() { return m_part; }

	void Init( CDisplayList * dlist, int x, int y, int angle,
		int mirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr );
	void Copy(CText *src);
	void Move( int x, int y, int angle, BOOL mirror, BOOL negative, int layer, int size=-1, int w=-1 );
	void Move( int x, int y, int angle, int size=-1, int w=-1);
	void Resize( int size, int w );						
	void MoveRelative( int _x, int _y, int _angle=-1, int _size=-1, int _w=-1 );					// Used for reftexts and valuetexts
	CPoint GetAbsolutePoint();																		// Gets absolute position for reftexts and valuetexts.
	CRect GetRect();

	void GenerateStrokes();														// CPT2 new.  Helper for Draw().
	int Draw();
	void GenerateStrokesRelativeTo( CPart *p );								// CPT2 new.  Helper for DrawRelativeTo().
	int DrawRelativeTo( CPart *p, bool bSelector=true );						// CPT2 new.
	void Undraw();
	void Highlight();
	void SetVisible(bool bVis);
	void StartDragging( CDC * pDC );
	void CancelDragging();
};

class CRefText: public CText
{
public:
	CRefText( CPart *_part, int x, int y, int angle, 
		BOOL bMirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown );
	CRefText( CFreePcbDoc *doc, int x, int y, int angle, 
		BOOL bMirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown );
	CRefText(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsRefText() { return true; }
	CRefText *ToRefText() { return this; }
	int GetTypeBit() { return bitRefText; }
	CUndoItem *MakeUndoItem()
		{ return new CURefText(this); }

	int Draw() { return DrawRelativeTo(m_part); }
};

class CValueText: public CText
{
public:
	CValueText( CPart *_part, int x, int y, int angle, 
		BOOL bMirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown );
	CValueText( CFreePcbDoc *doc, int x, int y, int angle, 
		BOOL bMirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown );
	CValueText(CFreePcbDoc *_doc, int _uid);

	virtual bool IsOnPcb();
	bool IsValueText() { return true; }
	CValueText *ToValueText() { return this; }
	int GetTypeBit() { return bitValueText; }
	CUndoItem *MakeUndoItem() 
		{ return new CUValueText(this); }

	int Draw() { return DrawRelativeTo(m_part); }
};


// class to represent a list of ctexts,  basically a souped-up CHeap<CText>
//
class CTextList
{
public:
	CHeap<CText> texts;
	SMFontUtil * m_smfontutil;
	CFreePcbDoc *m_doc;												// CPT2.  Added

	// member functions
	CTextList( CFreePcbDoc *_doc );
	
	CText * AddText( int x, int y, int angle, bool bMirror, 
					bool bNegative,	int layer, 
					int font_size, int stroke_width, CString * str_ptr, CPart *part = NULL, CShape *shape = NULL );
	void ReadTexts( CStdioFile * file );
	void WriteTexts( CStdioFile * file );
	void MoveOrigin( int dx, int dy );
	BOOL GetTextBoundaries( CRect * r );
	void ReassignCopperLayers( int n_new_layers, int * layer );
};

