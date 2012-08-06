// TextListNew.h 
// CPT2 Eventual replacement for TextList.  Defines class ctextlist, the new rival to CTextList

#pragma once

#include <afxcoll.h>
#include <afxtempl.h>
#include "shape.h"
#include "DisplayList.h"
#include "PcbFont.h"
#include "smfontutil.h"
#include "UndoList.h"
#include "LinkList.h"

class CTextList;
struct stroke;

#ifndef CPT2													// For now use the definitions of the following items that already exist in TextList.h

// All info needed to recreate a modified or deleted CText
//
struct undo_text {
	int m_uid;
	int m_x, m_y;
	int m_layer;
	int m_angle;
	BOOL m_mirror;
	BOOL m_bNegative;
	int m_font_size;
	int m_stroke_width;
	CString m_str;
	CTextList * m_tlist;
};

// Class to represent a text string that can be drawn on a PCB
// If the string is part of a footprint or part, it's position parameters
// will be relative to the part position.
//

// CPT2 Can be dumped in favor of ctext:

class CText
{
public:
	// member functions
	CText();
	~CText();
	CText( CDisplayList * dlist, int x, int y, int angle, 
		int mirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, 
		unsigned int selType=ID_TEXT, unsigned int selSubtype=ID_SEL_TXT );
	void Init( CDisplayList * dlist, id tid, int x, int y, int angle, 
		int mirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr );
	void Draw( CDisplayList * dlist, SMFontUtil * smfontutil );
	void Draw();
	void Undraw();
	void Highlight();
	void StartDragging( CDC * pDC );
	void CancelDragging();
	void Move( int x, int y, int angle, BOOL mirror, BOOL negative, int layer, int size=-1, int w=-1 );
	void Move( int x, int y, int angle, int size=-1, int w=-1);											// CPT added.  Used when moving ref/value texts
	void GetBounds( CRect &br );
	void SetIDType( int type, int subtype );
	int UID(){ return m_uid; };

	// member variables
	id m_id;
	int m_uid;
	int m_x, m_y;
	int m_layer;
	int m_angle;
	int m_mirror;
	BOOL m_bNegative;
	int m_font_size;
	int m_stroke_width;
	int m_nchars;
	CString m_str;
	CArray<stroke> m_stroke;
	CDisplayList * m_dlist;
	dl_element * dl_sel;
	SMFontUtil * m_smfontutil;
};

#endif

// class to represent a list of CTexts
//
class ctextlist
{
public:
	enum {
		UNDO_TEXT_ADD = 1,
		UNDO_TEXT_MODIFY,
		UNDO_TEXT_DELETE
	};

	carray<ctext> texts;
	int m_text_type;												// CPT2 ???
	int m_text_subtype;												// CPT2 ???
	SMFontUtil * m_smfontutil;
	CDisplayList * m_dlist;
	CFreePcbDoc *m_doc;												// CPT2.  Added

	// member functions
	ctextlist( CFreePcbDoc *_doc );
	~ctextlist() { }
	
	ctext * AddText( int x, int y, int angle, bool bMirror, 
					bool bNegative,	int layer, 
					int font_size, int stroke_width, CString * str_ptr, cpart2 *part = NULL );
	void ReadTexts( CStdioFile * file );							// Done in cpp
	void WriteTexts( CStdioFile * file );							// Done in cpp
	void MoveOrigin( int dx, int dy );								// Done in cpp
	BOOL GetTextBoundaries( CRect * r );							// Done in cpp
	void ReassignCopperLayers( int n_new_layers, int * layer ) { }	// TODO
	static void TextUndoCallback( int type, void * ptr, BOOL undo ) { } // CPT2 TODO
};
