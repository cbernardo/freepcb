// TextList.cpp ... implementation of class CTextList
//
// class representing text elements for PCB (except reference designators for parts)
//
#include "stdafx.h"
#include "utility.h"
#include "textlist.h"
#include "PcbFont.h"
#include "shape.h"
#include "ids.h"
#include "smfontutil.h"
#include "file_io.h"

// global index for iterating through texts
int g_it;

// CText constructors
//	tid.type = ID_TEXT or
//	tid.type = ID_PART 
//		tid.st = ID_REF_TXT or 
//		tid.st = ID_VALUE_TXT

// tid.st = 

CText::CText()
{
	m_dlist = NULL;
}

CText::CText( CDisplayList * dlist, id tid, int x, int y, int angle, int mirror,
			BOOL bNegative, int layer, int font_size, int stroke_width, 
			SMFontUtil * smfontutil, CString * str_ptr )
{
	m_dlist = NULL;
	Init( dlist, tid, x, y, angle, mirror, bNegative, layer, font_size, stroke_width, 
			smfontutil, str_ptr );
}

// draws strokes into display list if dlist != 0
//
void CText::Init( CDisplayList * dlist, id tid, int x, int y, int angle, int mirror,
			BOOL bNegative, int layer, int font_size, int stroke_width, 
			SMFontUtil * smfontutil, CString * str_ptr )
{
	Undraw();
	m_guid = GUID_NULL;
	HRESULT hr = ::UuidCreate(&m_guid);
	m_id = tid;
	m_x = x;
	m_y = y;
	m_angle = angle;
	m_mirror = mirror;
	m_bNegative = bNegative;
	m_layer = layer;
	m_stroke_width = stroke_width;
	m_font_size = font_size;
	m_str = *str_ptr;
	m_nchars = str_ptr->GetLength();
	m_dlist = dlist;

	if( smfontutil )
	{
		Draw( dlist, smfontutil );
	}
}

// CText destructor
// removes all dlist elements
//
CText::~CText()
{
	Undraw();
}

// Draw text as a series of strokes
// If dlist == NULL, generate strokes but don't draw into display list
void CText::Draw( CDisplayList * dlist, SMFontUtil * smfontutil )
{
	if( smfontutil )
	{
		// draw text
		m_dlist = dlist;
		id tid = m_id;
		if( tid.type == ID_TEXT )
			tid.st = ID_STROKE;
		else
			tid.sst = ID_STROKE;
		m_stroke.SetSize( 1000 );

		// now draw strokes
		CPoint si, sf;
		double x_scale = (double)m_font_size/22.0;
		double y_scale = (double)m_font_size/22.0;
		double y_offset = 9.0*y_scale;
		int i = 0;
		double xc = 0.0;
		for( int ic=0; ic<m_nchars; ic++ )
		{
			// get stroke info for character
			int xi, yi, xf, yf;
			double coord[64][4];
			double min_x, min_y, max_x, max_y;
			int nstrokes;
			if( !m_mirror )
			{
				nstrokes = smfontutil->GetCharStrokes( m_str[ic], SIMPLEX, &min_x, &min_y, &max_x, &max_y,
					coord, 64 );
			}
			else
			{
				nstrokes = smfontutil->GetCharStrokes( m_str[m_nchars-ic-1], SIMPLEX, &min_x, &min_y, &max_x, &max_y,
					coord, 64 );
			}
			for( int is=0; is<nstrokes; is++ )
			{
				if( m_mirror )
				{
					xi = (max_x - coord[is][0])*x_scale;
					yi = coord[is][1]*y_scale + y_offset;
					xf = (max_x - coord[is][2])*x_scale;
					yf = coord[is][3]*y_scale + y_offset;
				}
				else
				{
					xi = (coord[is][0] - min_x)*x_scale;
					yi = coord[is][1]*y_scale + y_offset;
					xf = (coord[is][2] - min_x)*x_scale;
					yf = coord[is][3]*y_scale + y_offset;
				}

				// get stroke relative to x,y
				si.x = xi + xc;
				sf.x = xf + xc;
				si.y = yi;
				sf.y = yf;
				// rotate
				RotatePoint( &si, m_angle, zero );
				RotatePoint( &sf, m_angle, zero );
				// add x, y and draw
				tid.i = i;
				m_stroke[i].w = m_stroke_width;
				m_stroke[i].xi = m_x + si.x;
				m_stroke[i].yi = m_y + si.y;
				m_stroke[i].xf = m_x + sf.x;
				m_stroke[i].yf = m_y + sf.y;
				if( dlist )
					m_stroke[i].dl_el = dlist->Add( tid, this, 
					m_layer, DL_LINE, 1, m_stroke_width, 0, 
					m_x+si.x, m_y+si.y, m_x+sf.x, m_y+sf.y, 0, 0 );
				else
					m_stroke[i].dl_el = NULL;
				i++;
				if( i >= m_stroke.GetSize() )
					m_stroke.SetSize( i + 100 );
			}
			if( nstrokes > 0 )
				xc += (max_x - min_x + 8.0)*x_scale;
			else
				xc += 16.0*x_scale;
		}
		m_stroke.SetSize( i );
		m_smfontutil = smfontutil;

		if( dlist )
		{
			// create selection rectangle
			int width = xc - 8.0*x_scale;
			si.x = 0 - m_stroke_width/2;
			sf.x = width + m_stroke_width/2;
			si.y = 0 - m_stroke_width/2;
			sf.y = m_font_size;
			// rotate to angle
			RotatePoint( &si, m_angle, zero );
			RotatePoint( &sf, m_angle, zero );
			// draw it
			if( tid.type == ID_TEXT )
				tid.st = ID_SEL_TXT;
			else if( tid.type == ID_PART && tid.st == ID_VALUE_TXT )
				tid.st = ID_SEL_VALUE_TXT;
			else if( tid.type == ID_PART && tid.st == ID_REF_TXT )
				tid.st = ID_SEL_REF_TXT;
			tid.i = 0;
			dl_sel = dlist->AddSelector( tid, this, m_layer, DL_HOLLOW_RECT, 1,
				0, 0, m_x + si.x, m_y + si.y, m_x + sf.x, m_y + sf.y, m_x + si.x, m_y + si.y );
			m_dlist = dlist;
		}
	}
	else
	{
		ASSERT(0);
	}
}

void CText::Undraw()
{
	if( m_dlist )
	{
		m_dlist->Remove( dl_sel );
		dl_sel = NULL;
		for( int i=0; i<m_stroke.GetSize(); i++ )
		{
			m_dlist->Remove( (dl_element*)m_stroke[i].dl_el );
		}
		m_dlist = NULL;		// indicated that it is not drawn
		m_stroke.RemoveAll();
	}
	m_smfontutil = NULL;	// indicate that strokes have been removed
}

// Select text for editing
//
void CText::Highlight()
{
	m_dlist->HighLight( DL_HOLLOW_RECT, 
		m_dlist->Get_x(dl_sel), 
		m_dlist->Get_y(dl_sel),
		m_dlist->Get_xf(dl_sel), 
		m_dlist->Get_yf(dl_sel), 
		1 );
}

// start dragging a rectangle representing the boundaries of text string
//
void CText::StartDragging( CDC * pDC )
{
	// make text strokes invisible
	for( int is=0; is<m_stroke.GetSize(); is++ )
	{
		m_stroke[is].dl_el->visible = 0;
	}
	// cancel selection 
	m_dlist->CancelHighLight();

	// drag
	m_dlist->StartDraggingRectangle( pDC, 
		m_dlist->Get_x(dl_sel), 
		m_dlist->Get_y(dl_sel),
		m_dlist->Get_x(dl_sel) - m_dlist->Get_x_org(dl_sel),
		m_dlist->Get_y(dl_sel) - m_dlist->Get_y_org(dl_sel),
		m_dlist->Get_xf(dl_sel) - m_dlist->Get_x_org(dl_sel),
		m_dlist->Get_yf(dl_sel) - m_dlist->Get_y_org(dl_sel), 
		0, LAY_SELECTION );
}

// stop dragging 
//
void CText::CancelDragging()
{
	m_dlist->StopDragging();
	for( int is=0; is<m_stroke.GetSize(); is++ )
	{
		m_stroke[is].dl_el->visible = 1;
	}
}

// move text
//
void CText::Move( int x, int y, int angle, 
					BOOL mirror, BOOL negative, int layer )
{
	Undraw();
	m_x = x;
	m_y = y;
	m_angle = angle;
	m_layer = layer;
	m_mirror = mirror;
	m_bNegative = negative;
	Draw( m_dlist, m_smfontutil );
}


// CTextList constructor/destructors
//

// default constructor
//
CTextList::CTextList()
{
	m_dlist = NULL;
	m_smfontutil = NULL;
}

// normal constructor
//
CTextList::CTextList( CDisplayList * dlist, SMFontUtil * smfontutil )
{
	m_dlist = dlist;
	m_smfontutil = smfontutil;
}

// destructor
//
CTextList::~CTextList()
{
	// destroy all CTexts
	for( int i=0; i<text_ptr.GetSize(); i++ )
	{
		delete text_ptr[i];
	}
}

// AddText ... adds a new entry to TextList, returns pointer to entry
//
CText * CTextList::AddText( int x, int y, int angle, int mirror, BOOL bNegative, int layer, 
						   int font_size, int stroke_width, CString * str_ptr, BOOL draw_flag )
{
	// create new CText and put pointer into text_ptr[]
	id tid(ID_TEXT, 0, 0, 0, 0);
	if( draw_flag )
	{
		CText * text = new CText( m_dlist, tid, x, y, angle, mirror, bNegative, 
			layer, font_size, stroke_width, m_smfontutil, str_ptr );
		text_ptr.Add( text );
		return text;
	}
	else
	{
		CText * text = new CText( NULL, tid, x, y, angle, mirror, bNegative, 
			layer, font_size, stroke_width, NULL, str_ptr );
		text_ptr.Add( text );
		return text;
	}
}

// RemoveText ... removes an entry and destroys it
//	returns 0 if successful, 1 if unable to find text
//
int CTextList::RemoveText( CText * text )
{
	for( int i=0; i<text_ptr.GetSize(); i++ )
	{
		if( text_ptr[i] == text )
		{
			delete text_ptr[i];
			text_ptr.RemoveAt( i );
			return 0;
		}
	}
	return 1;
}

// remove all text entries
//	returns 0 if successful, 1 if unable to find text
//
void CTextList::RemoveAllTexts()
{
	int n = text_ptr.GetSize();
	for( int i=0; i<n; i++ )
	{
		RemoveText( text_ptr[0] );
	}
	return;
}

// Select text for editing
//
void CTextList::HighlightText( CText * text )
{
	m_dlist->HighLight( DL_HOLLOW_RECT, 
		m_dlist->Get_x(text->dl_sel), 
		m_dlist->Get_y(text->dl_sel),
		m_dlist->Get_xf(text->dl_sel), 
		m_dlist->Get_yf(text->dl_sel), 
		1 );
}

// start dragging a rectangle representing the boundaries of text string
//
void CTextList::StartDraggingText( CDC * pDC, CText * text )
{
	// make text strokes invisible
	for( int is=0; is<text->m_stroke.GetSize(); is++ )
	{
		((dl_element*)text->m_stroke[is].dl_el)->visible = 0;
	}
	// cancel selection 
	m_dlist->CancelHighLight();

	// drag
	m_dlist->StartDraggingRectangle( pDC, 
		m_dlist->Get_x(text->dl_sel), 
		m_dlist->Get_y(text->dl_sel),
		m_dlist->Get_x(text->dl_sel) - m_dlist->Get_x_org(text->dl_sel),
		m_dlist->Get_y(text->dl_sel) - m_dlist->Get_y_org(text->dl_sel),
		m_dlist->Get_xf(text->dl_sel) - m_dlist->Get_x_org(text->dl_sel),
		m_dlist->Get_yf(text->dl_sel) - m_dlist->Get_y_org(text->dl_sel), 
		0, LAY_SELECTION );
}

// stop dragging text 
//
void CTextList::CancelDraggingText( CText * text )
{
	m_dlist->StopDragging();
	for( int is=0; is<text->m_stroke.GetSize(); is++ )
	{
		((dl_element*)text->m_stroke[is].dl_el)->visible = 1;
	}
}

#if 0
// move text
//
CText * CTextList::MoveText( CText * text, int x, int y, int angle, int mirror, int layer )
{
	CText * new_text = AddText( x, y, angle, mirror, layer, text->m_font_size, text->m_stroke_width, &(text->m_str) );
	new_text->m_guid = text->m_guid;
	RemoveText( text );
	return new_text;
}
#endif

// move text
//
void CTextList::MoveText( CText * text, int x, int y, int angle, 
						 BOOL mirror, BOOL negative, int layer )
{
	CDisplayList * dl = text->m_dlist;
	SMFontUtil * smf = text->m_smfontutil;
	text->Undraw();
	text->m_x = x;
	text->m_y = y;
	text->m_angle = angle;
	text->m_layer = layer;
	text->m_mirror = mirror;
	text->m_bNegative = negative;
	text->Draw( dl, smf );
}

// write text info to file
//
int CTextList::WriteTexts( CStdioFile * file )
{
	CString line;
	CText * t;
	try
	{
		// now write all text strings
		file->WriteString( "[texts]\n\n" );
		for( int it=0; it<text_ptr.GetSize(); it++ )
		{
			t = text_ptr[it];
			line.Format( "text: \"%s\" %d %d %d %d %d %d %d %d\n\n", t->m_str,
				t->m_x, t->m_y, t->m_layer, t->m_angle, t->m_mirror,
				t->m_font_size, t->m_stroke_width, t->m_bNegative );
			file->WriteString( line );
		}
		
	}
	catch( CFileException * e )
	{
		CString str;
		if( e->m_lOsError == -1 )
			str.Format( "File error: %d\n", e->m_cause );
		else
			str.Format( "File error: %d %ld (%s)\n", e->m_cause, e->m_lOsError,
			_sys_errlist[e->m_lOsError] );
		return 1;
	}

	return 0;
}

// read text strings from file
//
void CTextList::ReadTexts( CStdioFile * pcb_file )
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
			// error reading pcb file
			CString mess;
			mess.Format( "Unable to find [texts] section in file" );
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
			CString * err_str = new CString( "unexpected EOF in project file" );
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
			int mirror = my_atoi( &p[5] );
			int font_size = my_atoi( &p[6] );
			int stroke_width = my_atoi( &p[7] );
			BOOL m_bNegative = 0;
			if( np > 9)
				m_bNegative = my_atoi( &p[8] );
			AddText( x, y, angle, mirror, m_bNegative, layer, font_size, stroke_width, &str );
		}
	}
}

// create new undo_text object for undoing changes
//
undo_text * CTextList::CreateUndoRecord( CText * text )
{
	undo_text * undo = new undo_text;
	undo->m_guid = text->m_guid;
	undo->m_x = text->m_x;
	undo->m_y = text->m_y; 
	undo->m_layer = text->m_layer; 
	undo->m_angle = text->m_angle; 
	undo->m_mirror = text->m_mirror; 
	undo->m_bNegative = text->m_bNegative; 
	undo->m_font_size = text->m_font_size; 
	undo->m_stroke_width = text->m_stroke_width;
	undo->m_str = text->m_str;
	undo->m_tlist = this;
	return undo;
}

void CTextList::TextUndoCallback( int type, void * ptr, BOOL undo )
{
	int ifound;
	undo_text * un_t = (undo_text*)ptr;
	CText * text = 0;
	if( undo )
	{
		CTextList * tlist = un_t->m_tlist;
		if( type == CTextList::UNDO_TEXT_ADD || type == CTextList::UNDO_TEXT_MODIFY )
		{
			// find existing CText object
			ifound = -1;
			for( int it=0; it<tlist->text_ptr.GetSize(); it++ )
			{
				text = tlist->text_ptr[it];
				if( text->m_guid == un_t->m_guid )
				{
					ifound = it;
					break;
				}
			}
			if( ifound == -1 )
				ASSERT(0);	// text string not found
			if( type == CTextList::UNDO_TEXT_ADD )
			{
				// delete text
				tlist->RemoveText( text );
			}
			else if( type == CTextList::UNDO_TEXT_MODIFY )
			{
				// modify text back
				CDisplayList * dl = text->m_dlist;
				SMFontUtil * smf = text->m_smfontutil;
				text->Undraw();
				text->m_guid = un_t->m_guid;
				text->m_x = un_t->m_x;
				text->m_y = un_t->m_y;
				text->m_angle = un_t->m_angle;
				text->m_layer = un_t->m_layer;
				text->m_mirror = un_t->m_mirror;
				text->m_font_size = un_t->m_font_size;
				text->m_stroke_width = un_t->m_stroke_width;
				text->m_nchars = un_t->m_str.GetLength();
				text->m_str = un_t->m_str;
				text->Draw( dl, smf );
			}
		}
		else if( type == CTextList::UNDO_TEXT_DELETE )
		{
			// add deleted text back into list
			CText * new_text = tlist->AddText( un_t->m_x, un_t->m_y, un_t->m_angle, 
				un_t->m_mirror, un_t->m_bNegative,
				un_t->m_layer, un_t->m_font_size, un_t->m_stroke_width, &un_t->m_str );
			new_text->m_guid = un_t->m_guid;
		}
	}
	delete un_t;
}

void CTextList::MoveOrigin( int x_off, int y_off )
{
	for( int it=0; it<text_ptr.GetSize(); it++ )
	{
		CText * t = text_ptr[it];
		t->Undraw();
		t->m_x += x_off;
		t->m_y += y_off;
		t->Draw( m_dlist, m_smfontutil );
	}
}

// return text that matches guid
//
CText * CTextList::GetText( GUID * guid )
{
	CText * text = GetFirstText();
	while( text )
	{
		if( text->m_guid == *guid )
			return text;
		text = GetNextText();
	}
	return NULL;
}

// return first text in CTextList (or NULL if none)
//
CText * CTextList::GetFirstText()
{
	g_it = 0;
	if( text_ptr.GetSize() > 0 )
		return text_ptr[0];
	else
		return NULL;
}

// return next text in CTextList, or NULL if at end of list
//
CText * CTextList::GetNextText()
{
	g_it++;
	if( text_ptr.GetSize() > g_it )
		return text_ptr[g_it];
	else
		return NULL;
}

// get bounding rectangle for all text strings
// return FALSE if no text strings
//
BOOL CTextList::GetTextBoundaries( CRect * r )
{
	BOOL bValid = FALSE;
	CRect br;
	br.bottom = INT_MAX;
	br.left = INT_MAX;
	br.top = INT_MIN;
	br.right = INT_MIN;
	CText * t = GetFirstText();
	while( t )
	{
		for( int is=0; is<t->m_stroke.GetSize(); is++ )
		{
			stroke * s = &t->m_stroke[is];
			br.bottom = min( br.bottom, s->yi - s->w );
			br.bottom = min( br.bottom, s->yf - s->w );
			br.top = max( br.top, s->yi + s->w );
			br.top = max( br.top, s->yf + s->w );
			br.left = min( br.left, s->xi - s->w );
			br.left = min( br.left, s->xf - s->w );
			br.right = max( br.right, s->xi + s->w );
			br.right = max( br.right, s->xf + s->w );
			bValid = TRUE;
		}
		t = GetNextText();
	}
	*r = br;
	return bValid;
}

// get bounding rectangle for text string
// return FALSE if no text strings
//
BOOL CTextList::GetTextRectOnPCB( CText * t, CRect * r )
{
	BOOL bValid = FALSE;
	CRect br;
	br.left = m_dlist->Get_x( t->dl_sel );
	br.right = m_dlist->Get_xf( t->dl_sel );;
	br.bottom = m_dlist->Get_y( t->dl_sel );
	br.top = m_dlist->Get_yf( t->dl_sel );
	*r = br;
	return bValid;
}

// reassign copper text to new layers
// enter with layer[] = table of new copper layers for each old copper layer
//
void CTextList::ReassignCopperLayers( int n_new_layers, int * layer )
{
	CText * t = GetFirstText();
	while( t )
	{
		int old_layer = t->m_layer;
		if( old_layer >= LAY_TOP_COPPER )
		{
					int index = old_layer - LAY_TOP_COPPER;
					int new_layer = layer[index];
					if( new_layer == old_layer )
					{
						// do nothing
					}
					else if( new_layer == -1 )
					{
						// delete this text
						t->Undraw();
						RemoveText( t );
					}
					else
					{
						// move to new layer
						t->Undraw();
						t->m_layer = new_layer + LAY_TOP_COPPER;
						t->Draw( m_dlist, m_smfontutil ); 
					}
		}
		t = GetNextText();
	}
}

