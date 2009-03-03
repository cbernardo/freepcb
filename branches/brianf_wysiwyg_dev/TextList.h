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
#include "pubsub.h"

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
	class C_DS_ThermalClearance : public CDataSubscriber<int> 
	{
	    virtual void Update(pub_t const &data);		
	} m_DS_ThermalClearance;
	friend class C_DS_ThermalClearance;
	void Update_ThermalClearance(int clearance);

public:
	// member functions
	CText( CDisplayList * dlist, int x, int y, int angle, 
		int mirror, BOOL bNegative, int layer, int font_size, 
		int stroke_width, SMFontUtil * smfontutil, CString * str_ptr );
	~CText();
	void Draw( CDisplayList * dlist, SMFontUtil * smfontutil );
	void Undraw();
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
	void MoveText( CText * text, int x, int y, int angle, 
		BOOL mirror, BOOL negative, int layer );
	void ReadTexts( CStdioFile * file );
	int WriteTexts( CStdioFile * file );
	void MoveOrigin( int x_off, int y_off );
	CText * GetText( GUID * guid );
	CText * GetFirstText();
	CText * GetNextText();
	int GetNumTexts(){ return text_ptr.GetSize();};
	BOOL GetTextBoundaries( CRect * r );
	BOOL GetTextRectOnPCB( CText * t, CRect * r );
	undo_text * CreateUndoRecord( CText * text );
	static void TextUndoCallback( int type, void * ptr, BOOL undo );

	// member variables
	SMFontUtil * m_smfontutil;
	CDisplayList * m_dlist;
	CArray<CText*> text_ptr;
};

