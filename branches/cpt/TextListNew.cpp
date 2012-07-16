#include "stdafx.h"
#include "FreePcbDoc.h"
#include "TextListNew.h"

ctextlist::ctextlist( CFreePcbDoc *_doc )
	{ 
		m_doc = _doc;
		if (_doc)
			m_dlist = m_doc->m_dlist,
			m_smfontutil = m_doc->m_smfontutil;
		else
			m_dlist = NULL,
			m_smfontutil = NULL;
	}	

void ctextlist::ReadTexts( CStdioFile * pcb_file )
{
	int pos, err, np;
	CString in_str, key_str;
	CArray<CString> p;

	// find beginning of [texts] section
	do
	{
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			CString mess ((LPCSTR) IDS_UnableToFindTextsSectionInFile);
			AfxMessageBox( mess );
			return;
		}
		in_str.Trim();
	}
	while( in_str != "[texts]" );

	// get all text strings
	while( 1 )
	{
		pos = pcb_file->GetPosition();
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
			throw err_str;
		}
		in_str.Trim();
		if( in_str == "[end]" )
			break;		// start of [texts] section, exit
		else if( in_str.Left(5) == "text:" )
		{
			np = ParseKeyString( &in_str, &key_str, &p );
			CString str = p[0];
			int x = my_atoi( &p[1] );
			int y = my_atoi( &p[2] );
			int file_layer = my_atoi( &p[3] );
			int layer = m_layer_by_file_layer[file_layer];
			int angle = my_atoi( &p[4] );
			BOOL mirror = my_atoi( &p[5] );
			int font_size = my_atoi( &p[6] );
			int stroke_width = my_atoi( &p[7] );
			BOOL negative = np>9? my_atoi( &p[8] ): false;
			ctext *text = new ctext(m_doc, x, y, angle, mirror, negative, layer, font_size, stroke_width, m_smfontutil, &str);
			texts.Add(text);
			text->MustRedraw();
		}
	}
}

ctext *ctextlist::AddText( int x, int y, int angle, bool bMirror, bool bNegative, int layer, 
					int font_size, int stroke_width, CString * str_ptr )
{
	ctext * text = new ctext(m_doc, x, y, angle, bMirror, bNegative, layer, font_size, stroke_width, m_smfontutil, str_ptr);
	texts.Add(text);
	return text;
}

BOOL ctextlist::GetTextBoundaries( CRect * r )
{
	BOOL bValid = FALSE;
	CRect br (INT_MAX, INT_MIN, INT_MIN, INT_MAX);
	citer<ctext> it (&texts);
	for (ctext *t = it.First(); t; t = it.Next())
	{
		t->GenerateStrokes();							// In case it's not been drawn
		br.bottom = min(br.bottom, t->m_br.bottom);
		br.top = max(br.top, t->m_br.top);
		br.left = min(br.left, t->m_br.left);
		br.right = max(br.right, t->m_br.right);
		bValid = TRUE;
		// end CPT
	}
	*r = br;
	return bValid;
}



