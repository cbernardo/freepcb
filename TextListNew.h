// TextListNew.h 
// CPT2 Eventual replacement for TextList.  Defines class ctextlist, the new rival to CTextList

#pragma once

#include <afxcoll.h>
#include <afxtempl.h>
#include "shape.h"
#include "DisplayList.h"
#include "PcbFont.h"
#include "smfontutil.h"

struct stroke;

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
	SMFontUtil * m_smfontutil;
	// CDisplayList * m_dlist;
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
	void ReassignCopperLayers( int n_new_layers, int * layer );
};
