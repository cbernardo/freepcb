#include "stdafx.h"
#include "PartListNew.h"


cpartlist::cpartlist( CFreePcbDoc *_doc )
{
	m_doc = _doc;
	m_dlist = m_doc->m_dlist;
}

void cpartlist::MarkAllParts( int mark )
{															
	citer<cpart2> ip (&parts);																		// CPT2 TODO. Add generic method to carray<T>?
	for (cpart2 *p = ip.First(); p; p = ip.Next())
		p->utility = mark;
}

void cpartlist::RemoveAllParts()
{
	citer<cpart2> ip (&parts);
	for (cpart2 *p = ip.First(); p; p = ip.Next())
		p->Remove();
}

cpart2 *cpartlist::GetPartByName( CString *ref_des )
{
	// Find part in partlist by ref-des
	citer<cpart2> ip (&parts);
	for (cpart2 *p = ip.First(); p; p = ip.Next())
		if (p->ref_des == *ref_des)
			return p;
	return NULL;
}

cpin2 * cpartlist::GetPinByNames ( CString *ref_des, CString *pin_name)
{
	cpart2 *part = GetPartByName(ref_des);
	if (!part) return NULL;
	citer<cpin2> ip (&part->pins);
	for (cpin2 *p = ip.First(); p; p = ip.Next())
		if (p->pin_name == *pin_name)
			return p;
	return NULL;
}


cpart2 * cpartlist::AddFromString( CString * str )
{
	// set defaults
	CShape * s = NULL;
	CString in_str, key_str;
	CArray<CString> p;
	int pos = 0;
	int len = str->GetLength();
	int np;
	CString ref_des;
	BOOL ref_vis = TRUE;
	int ref_size = 0;
	int ref_width = 0;
	int ref_angle = 0;
	int ref_xi = 0;
	int ref_yi = 0;
	int ref_layer = LAY_SILK_TOP;
	CString value;
	BOOL value_vis = TRUE;
	int value_size = 0;
	int value_width = 0;
	int value_angle = 0;
	int value_xi = 0;
	int value_yi = 0;
	int value_layer = LAY_SILK_TOP;

	// add part to partlist
	CString package;
	int x;
	int y;
	int side;
	int angle;
	int glued;
	
	// CPT2 TODO Consider removing the following?
	CDisplayList * old_dlist = m_dlist;
	m_dlist = NULL;		// cancel further drawing

	in_str = str->Tokenize( "\n", pos );
	while( in_str != "" )
	{
		np = ParseKeyString( &in_str, &key_str, &p );
		if( key_str == "ref" )
		{
			ref_des = in_str.Right( in_str.GetLength()-4 );
			ref_des.Trim();
			ref_des = ref_des.Left(MAX_REF_DES_SIZE);
		}
		else if( key_str == "part" )
		{
			ref_des = in_str.Right( in_str.GetLength()-5 );
			ref_des.Trim();
			ref_des = ref_des.Left(MAX_REF_DES_SIZE);
		}
		else if( np >= 6 && key_str == "ref_text" )
		{
			ref_size = my_atoi( &p[0] );
			ref_width = my_atoi( &p[1] );
			ref_angle = my_atoi( &p[2] );
			ref_xi = my_atoi( &p[3] );
			ref_yi = my_atoi( &p[4] );
			if( np > 6 )
				ref_vis = my_atoi( &p[5] );
			else
			{
				if( ref_size )
					ref_vis = TRUE;
				else
					ref_vis = FALSE;
			}
			if( np > 7 )
				ref_layer = my_atoi( &p[6] );
		}
		else if( np >= 7 && key_str == "value" )
		{
			value = p[0];
			value_size = my_atoi( &p[1] );
			value_width = my_atoi( &p[2] );
			value_angle = my_atoi( &p[3] );
			value_xi = my_atoi( &p[4] );
			value_yi = my_atoi( &p[5] );
			if( np > 7 )
				value_vis = my_atoi( &p[6] );
			else
			{
				if( value_size )
					value_vis = TRUE;
				else
					value_vis = FALSE;
			}
			if( np > 8 )
				value_layer = my_atoi( &p[7] );
		}
		else if( key_str == "package" )
		{
			if( np >= 2 )
				package = p[0];
			else
				package = "";
			package = package.Left(CShape::MAX_NAME_SIZE);
		}
		else if( np >= 2 && key_str == "shape" )
		{
			// lookup shape in cache
			s = NULL;
			void * ptr;
			CString name = p[0];
			name = name.Left(CShape::MAX_NAME_SIZE);
			if ( m_footprint_cache_map->Lookup( name, ptr ) )
				// found in cache
				s = (CShape*)ptr; 
		}
		else if( key_str == "pos" )
		{
			if( np >= 6 )
			{
				x = my_atoi( &p[0] );
				y = my_atoi( &p[1] );
				side = my_atoi( &p[2] );
				angle = my_atoi( &p[3] );
				glued = my_atoi( &p[4] );
			}
			else
			{
				x = 0;
				y = 0;
				side = 0;
				angle = 0;
				glued = 0;
			}
		}
		in_str = str->Tokenize( "\n", pos );
	}

	cpart2 * part = new cpart2(this);
	part->SetData( s, &ref_des, &package, x, y, side, angle, 1, glued, ref_vis );			// CPT2.  Also initializes pins.
	part->SetValue( &value, value_xi, value_yi, value_angle, value_size, value_width, 
		value_vis, value_layer );
	if( part->shape ) 
	{
		part->m_ref_xi = ref_xi;
		part->m_ref_yi = ref_yi;
		part->m_ref_angle = ref_angle;
		part->m_ref_layer = ref_layer;
		part->ResizeRefText( ref_size, ref_width, ref_vis );
	}
	m_dlist = old_dlist;
	part->Draw();
	return part;
}


int cpartlist::ReadParts( CStdioFile * pcb_file )
{
	int pos, err;
	CString in_str, key_str;
	CArray<CString> p;

	// find beginning of [parts] section
	do
	{
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			// error reading pcb file
			CString mess ((LPCSTR) IDS_UnableToFindPartsSectionInFile);
			AfxMessageBox( mess );
			return 0;
		}
		in_str.Trim();
	}
	while( in_str != "[parts]" );

	// get each part in [parts] section
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
		if( in_str[0] == '[' && in_str != "[parts]" )
		{
			pcb_file->Seek( pos, CFile::begin );
			break;		// next section, exit
		}
		else if( in_str.Left(4) == "ref:" || in_str.Left(5) == "part:" )
		{
			CString str;
			do
			{
				str.Append( in_str );
				str.Append( "\n" );
				pos = pcb_file->GetPosition();
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
					throw err_str;
				}
				in_str.Trim();
			} while( (in_str.Left(4) != "ref:" && in_str.Left(5) != "part:" )
						&& in_str[0] != '[' );
			pcb_file->Seek( pos, CFile::begin );

			// now add part to partlist
			cpart2 * part = AddFromString( &str );
		}
	}
	return 0;
}
