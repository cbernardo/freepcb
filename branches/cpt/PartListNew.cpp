#include "stdafx.h"
#include "PartListNew.h"


cpartlist::cpartlist( CFreePcbDoc *_doc )
{
	m_doc = _doc;
	m_dlist = m_doc->m_dlist;
	m_layers = m_doc->m_num_copper_layers;
	m_annular_ring = m_doc->m_annular_ring_pins;					// (CPT2 TODO check)
	m_nlist = NULL;
	m_footprint_cache_map = NULL;
}

void cpartlist::RemoveAllParts()
{
	citer<cpart2> ip (&parts);
	for (cpart2 *p = ip.First(); p; p = ip.Next())
		p->Remove(false);
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

// get bounding rectangle of parts
// return 0 if no parts found, else return 1
//
int cpartlist::GetPartBoundaries( CRect * part_r )
{
	int min_x = INT_MAX;
	int max_x = INT_MIN;
	int min_y = INT_MAX;
	int max_y = INT_MIN;
	int parts_found = 0;
	
	citer<cpart2> ip (&parts);
	for (cpart2 *part = ip.First(); part; part = ip.Next())
	{
		if( part->dl_sel )
		{
			// TODO the ugly Get_x(), Get_y()... functions might be rethought some day
			int x = m_dlist->Get_x( part->dl_sel );
			int y = m_dlist->Get_y( part->dl_sel );
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			x = m_dlist->Get_xf( part->dl_sel );
			y = m_dlist->Get_yf( part->dl_sel );
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			parts_found = 1;
		}
		if( dl_element *ref_sel = part->m_ref->dl_sel )
		{
			int x = m_dlist->Get_x( ref_sel );
			int y = m_dlist->Get_y( ref_sel );
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			x = m_dlist->Get_xf( ref_sel );
			y = m_dlist->Get_yf( ref_sel );
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			parts_found = 1;
		}
	}
	part_r->left = min_x;
	part_r->right = max_x;
	part_r->bottom = min_y;
	part_r->top = max_y;
	return parts_found;
}


void cpartlist::SetThermals()
{
	// CPT2 new.  Updates the bNeedsThermal flags for all pins in all parts.
	citer<cpart2> ip (&parts);
	for (cpart2 *p = ip.First(); p; p = ip.Next())
	{
		citer<cpin2> ipin (&p->pins);
		for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
			pin->SetNeedsThermal();
	}
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
	bool ref_specified = false;				// CPT2
	int ref_size = 0;
	int ref_width = 0;
	int ref_angle = 0;
	int ref_xi = 0;
	int ref_yi = 0;
	int ref_layer = LAY_SILK_TOP;
	CString value;
	BOOL value_vis = TRUE;
	bool value_specified = false;			// CPT2
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
			ref_specified = true;					// CPT2
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
			value_specified = true;					// CPT2
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
	part->SetData( s, &ref_des, &value, &package, x, y, side, angle, 1, glued );			// CPT2.  Also initializes pins and puts ref and value
																							// into the positions indicated by the shape
	part->m_ref_vis = ref_vis;
	if (ref_specified)
		part->m_ref->Move(ref_xi, ref_yi, ref_angle,
				false, false, ref_layer, ref_size, ref_width);
	part->m_value_vis = value_vis;
	if (value_specified)
		part->m_value->Move(value_xi, value_yi, value_angle, 
				false, false, value_layer, value_size, value_width );
	part->MustRedraw();																			// CPT2. TODO is this the place to do this?
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

			// now add part to partlist.  (AddFromString() also calls MustRedraw().)
			cpart2 * part = AddFromString( &str );
		}
	}
	return 0;
}

int cpartlist::FootprintLayer2Layer( int fp_layer )
{
	int layer;
	switch( fp_layer )
	{
	case LAY_FP_SILK_TOP: layer = LAY_SILK_TOP; break;
	case LAY_FP_SILK_BOTTOM: layer = LAY_SILK_BOTTOM; break;
	case LAY_FP_TOP_COPPER: layer = LAY_TOP_COPPER; break;
	case LAY_FP_BOTTOM_COPPER: layer = LAY_BOTTOM_COPPER; break;
	default: ASSERT(0); layer = -1; break;
	}
	return layer;
}

int cpartlist::ExportPartListInfo( partlist_info * pli, cpart2 *part0 )
{
	// CPT2 converted.  
	// export part list data into partlist_info structure for editing in dialog.
	// Returns the index of part0's info within pli (-1 if not found)
	pli->SetSize( parts.GetSize() );
	int i = 0, ret = -1;
	citer<cpart2> ip (&parts);
	for (cpart2 *part = ip.First(); part; part = ip.Next())
	{
		part_info *pi = &(*pli)[i];
		pi->part = part;
		if (part==part0) ret = i;
		pi->shape = part->shape;
		pi->bShapeChanged = FALSE;
		pi->ref_des = part->ref_des;
		pi->ref_vis = part->m_ref_vis;
		pi->ref_layer = part->m_ref->m_layer;					// CPT2 added this for consistency
		pi->ref_size = part->m_ref->m_font_size;				// CPT2 TODO why do we have ref size/width in partlistinfo, but not value size/width?
		pi->ref_width = part->m_ref->m_stroke_width;
		pi->package = part->package;
		pi->value = part->value_text;
		pi->value_vis = part->m_value_vis;
		pi->value_layer = part->m_value->m_layer;
		pi->x = part->x;
		pi->y = part->y;
		pi->angle = part->angle;
		pi->side = part->side;
		pi->deleted = FALSE;
		pi->bOffBoard = FALSE;
		i++;
	}
	return ret;
}

void cpartlist::ImportPartListInfo( partlist_info * pl, int flags, CDlgLog * log )
{
	// CPT2 converted.  Routine relies on lower-level subroutines to call Undraw() or MustRedraw(), as appropriate, for individual parts and 
	// attached nets.
	CString mess; 

	// grid for positioning parts off-board
	int pos_x = 0;
	int pos_y = 0;
	enum { GRID_X = 100, GRID_Y = 50 };
	BOOL * grid = (BOOL*)calloc( GRID_X*GRID_Y, sizeof(BOOL) );
	int grid_num = 0;

	// first, look for parts in project whose ref_des has been changed
	/* CPT2 obsolete
	for( int i=0; i<pl->GetSize(); i++ )
	{
		part_info * pi = &(*pl)[i];
		if( pi->part )
		{
			if( pi->ref_des != pi->part->ref_des )
			{
				m_nlist->PartRefChanged( &pi->part->ref_des, &pi->ref_des );
				pi->part->ref_des = pi->ref_des;
			}
		}
	}
	*/

	// now find parts in project that are not in partlist_info
	// loop through all parts in project
	citer<cpart2> ip (&parts);
	for (cpart2 *part = ip.First(); part; part = ip.Next())
	{
		// loop through the partlist_info array
		BOOL bFound = FALSE;
		part->bPreserve = FALSE;
		for( int i=0; i<pl->GetSize(); i++ )
		{
			part_info * pi = &(*pl)[i];
			if( pi->part == part )
			{
				// part exists in partlist_info.  CPT2 TODO Before this check was based on comparing ref-designators, but I think this
				// check will work better in case we're changing the part's ref-des only.
				bFound = TRUE;
				break;
			}
		}
		if( !bFound )
		{
			// part in project but not in partlist_info
			if( flags & KEEP_PARTS_AND_CON )
			{
				// set flag to preserve this part
				part->bPreserve = TRUE;
				if( log )
				{
					CString s ((LPCSTR) IDS_KeepingPartAndConnections);
					mess.Format( s, part->ref_des );
					log->AddLine( mess );
				}
			}
			else if( flags & KEEP_PARTS_NO_CON )
			{
				// CPT2 I would have thought it's more useful to delete the part and keep the connections.  But anyway,
				// I'll make it do what it did before...  TODO provide the opposite option.
				// keep part but remove connections from netlist
				if( log )
				{
					CString s ((LPCSTR) IDS_KeepingPartButRemovingConnections);
					mess.Format( s, part->ref_des );
					log->AddLine( mess );
				}
				part->Remove(true, false);
			}
			else
			{
				// remove part
				if( log )
				{
					CString s ((LPCSTR) IDS_RemovingPart);
					mess.Format( s, part->ref_des );
					log->AddLine( mess );
				}
				part->Remove(false);			// CPT2 I guess...
			}
		}
	}

	// loop through partlist_info array, changing partlist as necessary
	for( int i=0; i<pl->GetSize(); i++ )
	{
		part_info * pi = &(*pl)[i];
		if( pi->part == 0 && pi->deleted )
			// new part was added but then deleted, ignore it
			continue;
		if( pi->part != 0 && pi->deleted )
		{
			// old part was deleted, remove it
			pi->part->Remove(true);
			continue;
		}

		if( pi->part && !pi->part->shape && (pi->shape || pi->bShapeChanged == TRUE) )
		{
			// old part exists, but it had a null footprint that has now changed.
			// Remove old part (we'll create the new one from scratch)
			pi->part->Remove(true);
			pi->part = NULL;
		}

		if( pi->part == 0 )
		{
			// the partlist_info does not include a pointer to an existing part
			// the part might not exist in the project, or we are importing a netlist file
			cpart2 * old_part = GetPartByName( &pi->ref_des );
			if( old_part && old_part->shape)
			{
				// an existing part has the same ref_des as the new part, and has a footprint
				// see if the incoming package name matches the old package or footprint
				if( (flags & KEEP_FP) 
					|| (pi->package == "") 
					|| (pi->package == old_part->package)
					|| (pi->package == old_part->shape->m_name) )
				{
					// use footprint and parameters from existing part
					pi->part = old_part;
					pi->ref_size = old_part->m_ref->m_font_size; 
					pi->ref_width = old_part->m_ref->m_stroke_width;
					pi->ref_vis = old_part->m_ref_vis;					// CPT
					pi->value = old_part->value_text;
					pi->value_vis = old_part->m_value_vis;
					pi->x = old_part->x; 
					pi->y = old_part->y;
					pi->angle = old_part->angle;
					pi->side = old_part->side;
					pi->shape = old_part->shape;
				}
				else if( pi->shape )
				{
					// use new footprint, but preserve position
					pi->ref_size = old_part->m_ref->m_font_size; 
					pi->ref_width = old_part->m_ref->m_stroke_width;
					pi->ref_vis = old_part->m_ref_vis;					// CPT
					pi->value = old_part->value_text;
					pi->value_vis = old_part->m_value_vis;
					pi->x = old_part->x; 
					pi->y = old_part->y;
					pi->angle = old_part->angle;
					pi->side = old_part->side;
					pi->part = old_part;
					pi->bShapeChanged = TRUE;
					if( log && old_part->shape->m_name != pi->package )
					{
						CString s ((LPCSTR) IDS_ChangingFootprintOfPart);
						mess.Format( s, old_part->ref_des, old_part->shape->m_name, pi->shape->m_name );
						log->AddLine( mess );
					}
				}
				else
				{
					// new part does not have footprint, remove old part
					if( log && old_part->shape->m_name != pi->package )
					{
						CString s ((LPCSTR) IDS_ChangingFootprintOfPart2);
						mess.Format( s, old_part->ref_des, old_part->shape->m_name, pi->package );
						log->AddLine( mess );
					}
					old_part->Remove(true);
				}
			}
			else if (old_part && !old_part->shape)
			{
				// remove old part (which did not have a footprint)
				if( log && old_part->package != pi->package )
				{
					CString s ((LPCSTR) IDS_ChangingFootprintOfPart);
					mess.Format( s, old_part->ref_des, old_part->package, pi->package );
					log->AddLine( mess );
				}
				old_part->Remove(true);
			}

			if( pi->shape && pi->bOffBoard )
			{
				// place new part offboard, using grid 
				int ix, iy;	// grid indices
				// find size of part in 100 mil units
				BOOL OK = FALSE;
				int w = abs( pi->shape->m_sel_xf - pi->shape->m_sel_xi )/(100*PCBU_PER_MIL)+2;
				int h = abs( pi->shape->m_sel_yf - pi->shape->m_sel_yi )/(100*PCBU_PER_MIL)+2;
				// now find space in grid for part
				for( ix=0; ix<GRID_X; ix++ )
					for (iy = 0; iy < (GRID_Y - h); iy++ )
					{
						if( !grid[ix+GRID_X*iy] )
						{
							// see if enough space
							OK = TRUE;
							for( int iix=ix; iix<(ix+w); iix++ )
								for( int iiy=iy; iiy<(iy+h); iiy++ )
									if( grid[iix+GRID_X*iiy] )
										OK = FALSE;
							if( OK )
								goto end_loop;
						}
					}

				end_loop:
				if( OK )
				{
					// place part
					pi->side = 0;
					pi->angle = 0;
					if( grid_num == 0 )
					{
						// first grid, to left and above origin
						pi->x = -(ix+w)*100*PCBU_PER_MIL;
						pi->y = iy*100*PCBU_PER_MIL;
					}
					else if( grid_num == 1 )
					{
						// second grid, to left and below origin
						pi->x = -(ix+w)*100*PCBU_PER_MIL;
						pi->y = -(iy+h)*100*PCBU_PER_MIL;
					}
					else if( grid_num == 2 )
					{
						// third grid, to right and below origin
						pi->x = ix*100*PCBU_PER_MIL;
						pi->y = -(iy+h)*100*PCBU_PER_MIL;
					}
					// remove space in grid
					for( int iix=ix; iix<(ix+w); iix++ )
						for( int iiy=iy; iiy<(iy+h); iiy++ )
							grid[iix+GRID_X*iiy] = TRUE;
				}
				else
				{
					// fail, go to next grid
					if( grid_num == 2 )
						ASSERT(0);		// ran out of grids
					else
					{
						// zero grid
						for( int j=0; j<GRID_Y; j++ )
							for( int i=0; i<GRID_X; i++ )
								grid[j*GRID_X+i] = FALSE;
						grid_num++;
					}
				}
				// now offset for part origin
				pi->x -= pi->shape->m_sel_xi;
				pi->y -= pi->shape->m_sel_yi;
			}
			
			// Create new part!
			cpart2 *part = new cpart2(this);
			CShape *shape = pi->shape;
			// The following line also positions ref and value according to "shape", if possible, and calls part->MustRedraw():
			part->SetData( shape, &pi->ref_des, &pi->value, &pi->package, pi->x, pi->y,
				pi->side, pi->angle, TRUE, FALSE );
			part->m_ref_vis = pi->ref_vis;
			part->m_value_vis = pi->value_vis;
			// CPT2 TODO.  The point of the following line is to allow "phantom pins" (those within a net that refer to deleted parts)
			// to be reattached when the part is reinstated.  Since I'm proposing that we drop phantom pins, I'm commenting it out.
			// m_nlist->PartAdded( part );
		}

		else 
		{
			// part existed before but may have been modified
			cpart2 *part = pi->part;
			part->MustRedraw();
			part->package = pi->package;
			if( part->shape != pi->shape || pi->bShapeChanged )
			{
				// footprint was changed
				ASSERT( part->shape != NULL );
				if( pi->shape && !(flags & KEEP_FP) )
					// change footprint to new one
					part->FootprintChanged( pi->shape );
			}
			part->m_ref_vis = pi->ref_vis;
			part->m_ref->m_layer = pi->ref_layer;
			part->m_ref->Resize( pi->ref_size, pi->ref_width );
			if (part->ref_des != pi->ref_des)
				part->ref_des = pi->ref_des,
				part->m_ref->m_str = pi->ref_des;
			part->m_value_vis = pi->value_vis;
			part->m_value->m_layer = pi->value_layer;
			if( part->value_text != pi->value )
				part->value_text = pi->value,
				part->m_value->m_str = pi->value;
			if( pi->x != part->x 
				|| pi->y != part->y
				|| pi->angle != part->angle
				|| pi->side != part->side )
			{
				// part was moved
				int dx = pi->x - part->x, dy = pi->y - part->y;
				if (pi->angle != part->angle || pi->side != part->side)
					// If angle/side has changed, we must ensure that attached traces are all unrouted by PartMoved():
					dx = dy = 1;
				part->Move( pi->x, pi->y, pi->angle, pi->side );
				part->PartMoved( dx, dy );
			}
		}
	}

	// PurgeFootprintCache();
	free( grid );
}

