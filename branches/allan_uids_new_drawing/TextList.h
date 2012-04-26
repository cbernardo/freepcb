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
#include "LinkList.h"

class CTextList;
struct stroke;

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
	void Undraw();
	void Highlight();
	void StartDragging( CDC * pDC );
	void CancelDragging();
	void Move( int x, int y, int angle, BOOL mirror, BOOL negative, int layer, int size=-1, int w=-1 );
	void GetBounds( CRect &br );
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

// class to represent a list of CTexts
//
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
	void MoveText( CText * text, int x, int y, int angle, 
		BOOL mirror, BOOL negative, int layer );
	void ReadTexts( CStdioFile * file );
	int WriteTexts( CStdioFile * file );
	void MoveOrigin( int x_off, int y_off );
	CText * GetText( int uid, int * index=NULL );
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
	CDLinkList m_iterator_list;
};

// Iterator for CTextList
// All iterators are added to CTextList::m_iterator_list
// If a CText item is removed from the CTextList,
// call OnRemove() to adjust all existing iterators
//
class CIterator_CText : protected CDLinkList
{
public:
	CIterator_CText( CTextList * tlist );
	~CIterator_CText();
	CText * GetFirst();		// reset position, get first CText
	CText * GetNext();		// get next CText
	int GetIndex();			// get index of current position
	void OnRemove( CText * text );	// call when CText removed from CTextList
	int GetNumIterators();			// number of active iterators

private:
	CTextList * m_tlist;		// the CTextList
	int m_CurrentPos;			// current index into array of CTexts
	CText * m_pCurrentText;		// current CText
};

