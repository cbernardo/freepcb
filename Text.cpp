// Text.cpp --- source file for classes related most closely to text, many of which are descendants of CPcbItem:
//  CText, CRefText, CValueText, CTextList

#include "stdafx.h"
#include <math.h>
#include <stdlib.h>
#include "Text.h"
#include "Net.h"
#include "Part.h"
#include "FreePcbDoc.h"


CText::CText( CFreePcbDoc *_doc, int _x, int _y, int _angle, 
	BOOL _bMirror, BOOL _bNegative, int _layer, int _font_size, 
	int _stroke_width, SMFontUtil *_smfontutil, CString * _str )			// CPT2 Removed selType/selSubtype args.  Will use derived CRefText and CValueText
	: CPcbItem (_doc)														// classes in place of this business.
{
	m_x = _x, m_y = _y;
	m_angle = _angle;
	m_bMirror = _bMirror; m_bNegative = _bNegative;
	m_layer = _layer;
	m_font_size = _font_size;
	m_stroke_width = _stroke_width;
	m_smfontutil = _smfontutil;
	m_str = *_str;
	m_bShown = true;
	m_part = NULL;
	m_shape = NULL;
}

CText::CText(CFreePcbDoc *_doc, int _uid):
	CPcbItem(_doc, _uid)
{ 
	m_smfontutil = NULL;
	m_part = NULL;
	m_shape = NULL;
}


bool CText::IsOnPcb() 
{
	if (m_shape)
		return m_shape->IsOnPcb() && m_shape->m_tl->texts.Contains(this);
	else if (m_part)
		return m_part->IsOnPcb() && m_part->m_tl->texts.Contains(this);
	else
		return doc->m_tlist->texts.Contains(this); 
}

int CText::GetLayer()
{
	// CPT2.  Now takes into account the possibility that there's a containing part on the bottom side.
	int layer = m_layer;
	if (m_part && m_part->side)
		if (layer==LAY_SILK_TOP) layer = LAY_SILK_BOTTOM;
		else if (layer==LAY_TOP_COPPER) layer = LAY_BOTTOM_COPPER;
		else if (layer==LAY_SILK_BOTTOM) layer = LAY_SILK_TOP;
		else if (layer==LAY_BOTTOM_COPPER) layer = LAY_TOP_COPPER;
	return layer;
}

void CText::Copy( CText *src )
{
	// CPT2 new.  Copy the contents of "src" into "this".  Includes the "m_part" and "m_shape" members (caller might want to override that)
	MustRedraw();
	m_x = src->m_x;
	m_y = src->m_y;
	m_angle = src->m_angle;
	m_bMirror = src->m_bMirror; 
	m_bNegative = src->m_bNegative;
	m_layer = src->m_layer;
	m_font_size = src->m_font_size;
	m_stroke_width = src->m_stroke_width;
	m_smfontutil = src->m_smfontutil;
	m_str = src->m_str;
	m_bShown = src->m_bShown;
	m_part = src->m_part;
	m_shape = src->m_shape;
	utility = src->utility;
}

void CText::Move( int x, int y, int angle, BOOL mirror, BOOL negative, int layer, int size, int w )
{
	MustRedraw();
	m_x = x;
	m_y = y;
	m_angle = angle;
	m_layer = layer;
	m_bMirror = mirror;
	m_bNegative = negative;
	if (size>=0) m_font_size = size;
	if (w>=0) m_stroke_width = w;
}

void CText::Move(int x, int y, int angle, int size, int w) 
	// CPT:  extra version of Move(); appropriate for ref and value-texts, where the layer, mirroring, and negation don't change.
	// NB DrawRelativeTo() will take care of automatic mirroring for ref and value-texts that wind up on the bottom layers.
	{ Move(x, y, angle, false, false, m_layer, size, w); }

void CText::Resize(int size, int w)
	{ Move (m_x, m_y, m_angle, m_bMirror, m_bNegative, m_layer, size, w); }

void CText::MoveRelative( int _x, int _y, int _angle, int _size, int _w )
{
	// CPT2 used for moving ref and value texts, whose coordinate values are relative to the parent part.
	// _x and _y are in absolute world coords.
	// angle is relative to part angle.  (Change? It's a bit confusing.)  "_angle", "_size", and "_w" are now -1 by default (=> no change)
	// get position of new text box origin relative to part
	CPart * part = GetPart();
	CPoint part_org, tb_org;
	tb_org.x = _x - part->x;
	tb_org.y = _y - part->y;
	
	// correct for rotation of part
	RotatePoint( &tb_org, 360-part->angle, zero );
	
	// correct for part on bottom of board (reverse relative x-axis)
	if( part->side == 1 )
		tb_org.x = -tb_org.x;
	
	// reset text parameters
	MustRedraw();
	m_x = tb_org.x;
	m_y = tb_org.y;
	if (_angle!=-1) 
		m_angle = _angle % 360;
	if (_size!=-1)
		m_font_size = _size;
	if (_w!=-1)
		m_stroke_width = _w;
}

CPoint CText::GetAbsolutePoint()
{
	// CPT2.  Used for reftexts and valuetexts, whose absolute position is computed relative to the parent part.
	CPart *part = GetPart();
	CPoint pt;

	// move origin of text box to position relative to part
	pt.x = m_x;
	pt.y = m_y;
	// flip if part on bottom
	if( part->side )
		pt.x = -pt.x;
	// rotate with part about part origin
	RotatePoint( &pt, part->angle, zero );
	pt.x += part->x;
	pt.y += part->y;
	return pt;
}

CRect CText::GetRect()
{
	// CPT2 utility.  Get bounding rectangle.  If this is a reftext or valuetext, the result is relative to the parent part.
	GenerateStrokes();
	return m_br;
}

void CText::GenerateStrokes() {
	// CPT2 new.  Helper for Draw(), though it might also be called independently of drawing.
	// Generate strokes and put them in m_stroke.  Also setup the bounding rectangle member m_br.
	// TODO consider caching
	m_stroke.SetSize( 1000 );
	CPoint si, sf;
	double x_scale = (double)m_font_size/22.0;
	double y_scale = (double)m_font_size/22.0;
	double y_offset = 9.0*y_scale;
	int i = 0;
	double xc = 0.0;
	int xmin = INT_MAX;
	int xmax = INT_MIN;
	int ymin = INT_MAX;
	int ymax = INT_MIN;
	int nChars = m_str.GetLength();
	// CPT2.  Setup font (in case m_smfontutil is NULL).  This font business is a bit of a pain...
	SMFontUtil *smf = m_smfontutil;
	if (!smf)
		smf = ((CFreePcbApp*)AfxGetApp())->m_doc->m_smfontutil;

	for( int ic=0; ic<nChars; ic++ )
	{
		// get stroke info for character
		int xi, yi, xf, yf;
		double coord[64][4];
		double min_x, min_y, max_x, max_y;
		int nstrokes;
		if( !m_bMirror )
			nstrokes = smf->GetCharStrokes( m_str[ic], SIMPLEX, &min_x, &min_y, &max_x, &max_y,
				coord, 64 );
		else
			nstrokes = smf->GetCharStrokes( m_str[nChars-ic-1], SIMPLEX, &min_x, &min_y, &max_x, &max_y,
				coord, 64 );
		// loop through strokes and create stroke structures
		for( int is=0; is<nstrokes; is++ )
		{
			if( m_bMirror )
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
			// add m_x, m_y, and fill in stroke structure
			stroke * s = &m_stroke[i];
			s->w = m_stroke_width;
			s->xi = m_x + si.x;
			s->yi = m_y + si.y;
			s->xf = m_x + sf.x;
			s->yf = m_y + sf.y;
			s->layer = m_layer;
			s->type = DL_LINE;
			// update bounding rectangle
			ymin = min( ymin, s->yi - s->w );
			ymin = min( ymin, s->yf - s->w );
			ymax = max( ymax, s->yi + s->w );
			ymax = max( ymax, s->yf + s->w );
			xmin = min( xmin, s->xi - s->w );
			xmin = min( xmin, s->xf - s->w );
			xmax = max( xmax, s->xi + s->w );
			xmax = max( xmax, s->xf + s->w );
			// Next stroke...
			i++;
			if( i >= m_stroke.GetSize() )
				m_stroke.SetSize( i + 100 );
		}
		if( nstrokes > 0 )
			xc += (max_x - min_x + 8.0)*x_scale;
		else
			xc += 16.0*x_scale;
	}

	// Wrap up
	m_stroke.SetSize( i );
	m_br.left = xmin - m_stroke_width/2;
	m_br.right = xmax + m_stroke_width/2;
	m_br.bottom = ymin - m_stroke_width/2;
	m_br.top = ymax + m_stroke_width/2;
}

void CText::GenerateStrokesRelativeTo(CPart *part) {
	// CPT2 new.  Helper for DrawRelativeTo(), though it might also be called independently of drawing.
	// Somewhat descended from the old GenerateStrokesFromPartString() in PartList.cpp.
	// Used for texts (including reftexts and valuetexts) whose position is relative to "part".
	// Generate strokes and put them in m_stroke.  Also setup the bounding rectangle member m_br.
	// If "part" is null, then we're in the footprint editor.  The only way calling GenerateStrokes() differs 
	// from GenerateStrokesRelativeTo(NULL) is that in the latter we make sure that text on the bottom silk or bottom copper
	// gets mirrored.
	// TODO consider caching
	SMFontUtil *smf = doc->m_smfontutil;
	m_stroke.SetSize( 1000 );
	CPoint si, sf;
	double x_scale = (double)m_font_size/22.0;
	double y_scale = (double)m_font_size/22.0;
	double y_offset = 9.0*y_scale;
	int partX = part? part->x: 0;
	int partY = part? part->y: 0;
	int partAngle = part? part->angle: 0;
	// Adjust layer value if part is on bottom
	int layer = m_layer;
	int bMirror = m_bMirror, bOnBottom;
	if (part) 
		bOnBottom = layer==LAY_SILK_BOTTOM || layer==LAY_BOTTOM_COPPER;
	else
		bOnBottom = layer==LAY_FP_SILK_BOTTOM || layer==LAY_FP_BOTTOM_COPPER;
	if (bOnBottom)
		bMirror = !bMirror;
	if (part && part->side)
		if (layer==LAY_SILK_TOP) layer = LAY_SILK_BOTTOM;
		else if (layer==LAY_TOP_COPPER) layer = LAY_BOTTOM_COPPER;
		else if (layer==LAY_SILK_BOTTOM) layer = LAY_SILK_TOP;
		else if (layer==LAY_BOTTOM_COPPER) layer = LAY_TOP_COPPER;
	int i = 0;
	double xc = 0.0;
	int xmin = INT_MAX;
	int xmax = INT_MIN;
	int ymin = INT_MAX;
	int ymax = INT_MIN;
	int nChars = m_str.GetLength();

	for( int ic=0; ic<nChars; ic++ )
	{
		// get stroke info for character
		int xi, yi, xf, yf;
		double coord[64][4];
		double min_x, min_y, max_x, max_y;
		int nstrokes;
		if( !bMirror )
			nstrokes = smf->GetCharStrokes( m_str[ic], SIMPLEX, &min_x, &min_y, &max_x, &max_y,
				coord, 64 );
		else
			nstrokes = smf->GetCharStrokes( m_str[nChars-ic-1], SIMPLEX, &min_x, &min_y, &max_x, &max_y,
				coord, 64 );
		// loop through strokes and create stroke structures
		for( int is=0; is<nstrokes; is++ )
		{
			if( bMirror )
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
			// Get stroke points relative to text box origin
			si.x = xi + xc;
			sf.x = xf + xc;
			si.y = yi;
			sf.y = yf;
			// rotate about text box origin
			RotatePoint( &si, m_angle, zero );
			RotatePoint( &sf, m_angle, zero );
			// move origin of text box to position relative to part
			si.x += m_x;
			sf.x += m_x;
			si.y += m_y;
			sf.y += m_y;
			if (part && part->side)
				si.x = -si.x,
				sf.x = -sf.x;
			// rotate with part about part origin
			RotatePoint( &si, partAngle, zero );
			RotatePoint( &sf, partAngle, zero );
			// add part's (x,y), then add stroke to array
			stroke * s = &m_stroke[i];
			s->w = m_stroke_width;
			s->xi = partX + si.x;
			s->yi = partY + si.y;
			s->xf = partX + sf.x;
			s->yf = partY + sf.y;
			s->layer = layer;
			s->type = DL_LINE;
			// update bounding rectangle
			ymin = min( ymin, s->yi - s->w );
			ymin = min( ymin, s->yf - s->w );
			ymax = max( ymax, s->yi + s->w );
			ymax = max( ymax, s->yf + s->w );
			xmin = min( xmin, s->xi - s->w );
			xmin = min( xmin, s->xf - s->w );
			xmax = max( xmax, s->xi + s->w );
			xmax = max( xmax, s->xf + s->w );
			// Next stroke...
			i++;
			if( i >= m_stroke.GetSize() )
				m_stroke.SetSize( i + 100 );
		}
		if( nstrokes > 0 )
			xc += (max_x - min_x + 8.0)*x_scale;
		else
			xc += 16.0*x_scale;
	}

	// Wrap up
	m_stroke.SetSize( i );
	m_br.left = xmin - m_stroke_width/2;
	m_br.right = xmax + m_stroke_width/2;
	m_br.bottom = ymin - m_stroke_width/2;
	m_br.top = ymax + m_stroke_width/2;
}


int CText::Draw() 
{
	// CPT2 TODO.  Deal with drawing negative text.
	CDisplayList *dl = doc->m_dlist;
	if( !dl )
		return NO_DLIST;
	if (bDrawn)
		return ALREADY_DRAWN;
	if (!m_bShown)
		return NOERR;

	GenerateStrokes();													// Fills m_stroke and m_br
	// Now draw each stroke
	for( int is=0; is<m_stroke.GetSize(); is++ )
		m_stroke[is].dl_el = dl->AddMain( this, m_layer, 
			DL_LINE, 1, m_stroke[is].w, 0, 0,
			m_stroke[is].xi, m_stroke[is].yi, 
			m_stroke[is].xf, m_stroke[is].yf, 0, 0 );
	// draw selection rectangle for text
	dl_sel = dl->AddSelector( this, m_layer, DL_HOLLOW_RECT, 1,	
		0, 0, m_br.left, m_br.bottom, m_br.right, m_br.top, m_br.left, m_br.bottom );
	bDrawn = true;
	return NOERR;
}

int CText::DrawRelativeTo(CPart *part, bool bSelector) 
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl )
		return NO_DLIST;
	if (bDrawn)
		return ALREADY_DRAWN;
	if (m_str.GetLength()==0)
		return EMPTY_TEXT;
	if (!m_bShown)
		return NOERR;

	GenerateStrokesRelativeTo( part );													// Fills m_stroke and m_br
	// Now draw each stroke
	for( int is=0; is<m_stroke.GetSize(); is++ )
		m_stroke[is].dl_el = dl->AddMain( this, m_stroke[is].layer, 
			DL_LINE, 1, m_stroke[is].w, 0, 0,
			m_stroke[is].xi, m_stroke[is].yi, 
			m_stroke[is].xf, m_stroke[is].yf, 0, 0 );
	// draw selection rectangle for text
	if (bSelector)
		dl_sel = dl->AddSelector( this, m_layer, DL_HOLLOW_RECT, 1,	
									0, 0, m_br.left, m_br.bottom, m_br.right, m_br.top, m_br.left, m_br.bottom );
	bDrawn = true;
	return NOERR;
}

void CText::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl || !IsDrawn() ) return;

	dl->Remove( dl_sel );
	dl_sel = NULL;
	for( int i=0; i<m_stroke.GetSize(); i++ )
		dl->Remove( m_stroke[i].dl_el );
	m_stroke.RemoveAll();
	// m_smfontutil = NULL;							// indicate that strokes have been removed.  CPT2 Removed, caused a crash
	bDrawn = false;
}

void CText::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;
	dl->Highlight( DL_HOLLOW_RECT, 
		dl->Get_x(dl_sel), dl->Get_y(dl_sel),
		dl->Get_xf(dl_sel), dl->Get_yf(dl_sel), 1 );
}

void CText::SetVisible(bool bVis)
{
	for( int is=0; is<m_stroke.GetSize(); is++ )
		m_stroke[is].dl_el->visible = bVis;
}

void CText::StartDragging( CDC * pDC )
{
	// CPT2 derived from CTextList::StartDraggingText
	SetVisible(false);
	CDisplayList *dl = doc->m_dlist;
	dl->CancelHighlight();
	// CPT2 r346 bugfix:  draw the rectangle so that the box for rotated text will match what you get when you click
	int x = dl->Get_x(dl_sel), y = dl->Get_y(dl_sel);
	int w = dl->Get_xf(dl_sel) - x, h = dl->Get_yf(dl_sel) - y;
	int angle = m_angle;
	if (m_part)
		angle = (angle + m_part->angle) % 360;
	if (angle==90)
		h = -h;
	else if (angle==180)
		w = -w, h = -h;
	else if (angle==270)
		w = -w;
	if (m_part && m_part->side)
		w = -w;
	dl->StartDraggingRectangle( pDC, x, y, 0, 0, w, h, 0, LAY_SELECTION );
}

void CText::CancelDragging()
{
	// CPT2 derived from CTextList::CancelDraggingText
	doc->m_dlist->StopDragging();
	SetVisible(true);
}


CRefText::CRefText( CPart *part, int x, int y, int angle, 
	BOOL bMirror, BOOL bNegative, int layer, int font_size, 
	int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown ) :
		CText(part->doc, x, y, angle, bMirror, bNegative, layer, font_size,
			stroke_width, smfontutil, str_ptr) 
		{ m_part = part; m_bShown = bShown; }

CRefText::CRefText( CFreePcbDoc *doc, int x, int y, int angle, 
	BOOL bMirror, BOOL bNegative, int layer, int font_size, 
	int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown ) :
		CText(doc, x, y, angle, bMirror, bNegative, layer, font_size,
			stroke_width, smfontutil, str_ptr) 
		{ m_part = NULL; m_bShown = bShown; }
CRefText::CRefText(CFreePcbDoc *_doc, int _uid):
	CText(_doc, _uid)
		{ m_part = NULL; }


bool CRefText::IsOnPcb() 
{
	if (m_shape)
		return m_shape->IsOnPcb() && m_shape->m_ref == this;
	return m_part && m_part->IsOnPcb() && m_part->m_ref == this; 
}

CValueText::CValueText( CPart *part, int x, int y, int angle, 
	BOOL bMirror, BOOL bNegative, int layer, int font_size, 
	int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown ) :
		CText(part->doc, x, y, angle, bMirror, bNegative, layer, font_size,
			stroke_width, smfontutil, str_ptr) 
		{ m_part = part; m_bShown = bShown; }

CValueText::CValueText( CFreePcbDoc *doc, int x, int y, int angle, 
	BOOL bMirror, BOOL bNegative, int layer, int font_size, 
	int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown ) :
		CText(doc, x, y, angle, bMirror, bNegative, layer, font_size,
			stroke_width, smfontutil, str_ptr) 
		{ m_part = NULL; m_bShown = bShown; }

CValueText::CValueText(CFreePcbDoc *_doc, int _uid):
	CText(_doc, _uid)
		{ m_part = NULL; }

bool CValueText::IsOnPcb() 
{ 
	if (m_shape)
		return m_shape->IsOnPcb() && m_shape->m_value == this;
	return m_part && m_part->IsOnPcb() && m_part->m_value == this; 
}


CTextList::CTextList( CFreePcbDoc *_doc )
	{ 
		m_doc = _doc;
		if (_doc)
			m_smfontutil = m_doc->m_smfontutil;
		else
			m_smfontutil = NULL;
	}	

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
			CText *text = new CText(m_doc, x, y, angle, mirror, negative, layer, font_size, stroke_width, m_smfontutil, &str);
			texts.Add(text);
			text->MustRedraw();
		}
	}
}

CText *CTextList::AddText( int x, int y, int angle, bool bMirror, bool bNegative, int layer, 
					int font_size, int stroke_width, CString * str_ptr, CPart *part, CShape *shape )
{
	CText * text = new CText(m_doc, x, y, angle, bMirror, bNegative, layer, font_size, stroke_width, m_smfontutil, str_ptr);
	text->m_part = part;
	text->m_shape = shape;
	texts.Add(text);
	return text;
}

void CTextList::WriteTexts( CStdioFile * file )
{
	try
	{
		// now write all text strings
		file->WriteString( "[texts]\n\n" );
		CIter<CText> it (&texts);
		for (CText *t = it.First(); t; t = it.Next())
		{
			CString line;
			line.Format( "text: \"%s\" %d %d %d %d %d %d %d %d\n\n", t->m_str,
				t->m_x, t->m_y, t->m_layer, t->m_angle, t->m_bMirror,
				t->m_font_size, t->m_stroke_width, t->m_bNegative );
			file->WriteString( line );
		}
		
	}
	catch( CFileException * e )
	{
		CString str, s ((LPCSTR) IDS_FileError1), s2 = ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			str.Format( s, e->m_cause );
		else
			str.Format( s2, e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
	}
}

void CTextList::MoveOrigin( int dx, int dy )
{
	CIter<CText> it (&texts);
	for (CText *t = it.First(); t; t = it.Next())
		t->MustRedraw(),
		t->m_x += dx,
		t->m_y += dy;
}

BOOL CTextList::GetTextBoundaries( CRect * r )
{
	BOOL bValid = FALSE;
	CRect br (INT_MAX, INT_MIN, INT_MIN, INT_MAX);
	CIter<CText> it (&texts);
	for (CText *t = it.First(); t; t = it.Next())
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


void CTextList::ReassignCopperLayers( int n_new_layers, int * layer )
{
	CIter<CText> it (&texts);
	for (CText *t = it.First(); t; t = it.Next())
	{
		int old_layer = t->m_layer;
		if( old_layer >= LAY_TOP_COPPER )
		{
			int index = old_layer - LAY_TOP_COPPER;
			int new_layer = layer[index];
			if( new_layer == old_layer )
				// do nothing
				;
			else if( new_layer == -1 )
			{
				// delete this text
				t->Undraw();
				texts.Remove(t);
			}
			else
			{
				// move to new layer
				t->MustRedraw();
				t->m_layer = new_layer + LAY_TOP_COPPER;
			}
		}
	}
}


