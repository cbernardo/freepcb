// textlist.cpp ... definition of class CTextList
//
#pragma once

#include <afxcoll.h>
#include <afxtempl.h>
#include "shape.h"
#include "DisplayList.h"
#include "PcbFont.h"
#include "smfontutil.h"
#include "UndoList.h"

class CTextList;
struct stroke;

struct undo_text {
	GUID m_guid;
	int m_x, m_y;
	int m_layer;
	int m_angle;
	BOOL m_mirror;
	BOOL m_bNegative;
	int m_font_size;
	int m_stroke_width;
	CString m_str;
	int m_nstrokes;
	CTextList * m_tlist;
};

class CText
{
public:
	// member functions
	// CPT added selType and selSubtype members.  Most of the time, for normal text, these are 
	// ID_TEXT/ID_SEL_TXT, but in special cases like e.g. "REF" in the footprint editor, they can be ID_PART/ID_SEL_REF_TXT
	CText( CDisplayList * dlist, int x, int y, int angle, 
		int mirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, unsigned int selType=ID_TEXT, unsigned int selSubtype=ID_SEL_TXT );
	~CText();
	void Draw( CDisplayList * dlist, SMFontUtil * smfontutil );
	void Undraw();
	// CPT:  Move() was in CTextList; moved it here.  GetBounds() is new:
	void Move( int x, int y, int angle, BOOL mirror, BOOL negative, int layer, int size=-1, int w=-1 );
	void GetBounds( CRect &br );
	// member variables
	GUID m_guid;
	int m_x, m_y;
	int m_layer;
	int m_angle;
	int m_mirror;
	BOOL m_bNegative;
	int m_font_size;
	int m_stroke_width;
	CPcbFont * m_font;
	int m_nchars;
	CString m_str;
	CArray<stroke> m_stroke;
	CDisplayList * m_dlist;
	dl_element * dl_sel;
	SMFontUtil * m_smfontutil;
	unsigned int m_selType, m_selSubtype;			// CPT
};

class CTextList
{
public:
	enum {
		UNDO_TEXT_ADD = 1,
		UNDO_TEXT_MODIFY,
		UNDO_TEXT_DELETE
	};
	// member functions
	CTextList();
	CTextList( CDisplayList * dlist, SMFontUtil * smfontutil );
	~CTextList();
	CText * AddText( int x, int y, int angle, int mirror, 
					BOOL bNegative,	int layer, 
					int font_size, int stroke_width, 
					CString * str_ptr, BOOL draw_flag=TRUE );
	int RemoveText( CText * text );
	void  RemoveAllTexts();
	void HighlightText( CText * text );
	void StartDraggingText( CDC * pDC, CText * text );
	void CancelDraggingText( CText * text );
	void ReadTexts( CStdioFile * file );
	int WriteTexts( CStdioFile * file );
	void MoveOrigin( int x_off, int y_off );
	CText * GetText( GUID * guid );
	CText * GetFirstText();
	CText * GetNextText();
	int GetNumTexts(){ return text_ptr.GetSize();};
	BOOL GetTextBoundaries( CRect * r );
	BOOL GetTextRectOnPCB( CText * t, CRect * r );
	void ReassignCopperLayers( int n_new_layers, int * layer );
	undo_text * CreateUndoRecord( CText * text );
	static void TextUndoCallback( int type, void * ptr, BOOL undo );

	// member variables
	SMFontUtil * m_smfontutil;
	CDisplayList * m_dlist;
	CArray<CText*> text_ptr;
};

