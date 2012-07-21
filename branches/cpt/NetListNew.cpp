#include "stdafx.h"
#include "FreePcbDoc.h"
#include "NetListNew.h"

cnetlist::cnetlist( CFreePcbDoc * _doc )
	{ 
		m_doc = _doc; 
		m_dlist = m_doc->m_dlist;
		m_plist = m_doc->m_plist;
	}													// CPT2 TODO: finish


void cnetlist::SetThermals()
{
	// CPT2 new.  Call SetThermals() for all nets in the list.
	citer<cnet2> in (&nets);
	for (cnet2 *n = in.First(); n; n = in.Next())
		n->SetThermals();
}

BOOL cnetlist::GetNetBoundaries( CRect * r )
{
	// get bounding rectangle for all net elements
	BOOL bValid = FALSE;
	CRect br (INT_MAX, INT_MIN, INT_MIN, INT_MAX);
	citer<cnet2> in (&nets);
	for (cnet2 *net = in.First(); net; net = in.Next())
	{
		citer<cconnect2> ic (&net->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
		{
			citer<cvertex2> iv (&c->vtxs);
			for (cvertex2 *v = iv.First(); v; v = iv.Next())
			{
				br.bottom = min( br.bottom, v->y - v->via_w );
				br.top = max( br.top, v->y + v->via_w );
				br.left = min( br.left, v->x - v->via_w );
				br.right = max( br.right, v->x + v->via_w );
				bValid = TRUE;
			}
		}
		citer<carea2> ia (&net->areas);
		for (carea2 *a = ia.First(); a; a = ia.Next())
		{
			CRect r = a->GetBounds();
			br.bottom = min( br.bottom, r.bottom );
			br.top = max( br.top, r.top );
			br.left = min( br.left, r.left );
			br.right = max( br.right, r.right );
			bValid = TRUE;
		}
	}

	*r = br;
	return bValid;
}

void cnetlist::ReadNets( CStdioFile * pcb_file, double read_version, int * layer )
{
	int err, pos, np;
	CArray<CString> p;
	CString in_str, key_str;
	int * in_layer = layer;
	if( in_layer == NULL )
		in_layer = m_layer_by_file_layer;	// this is a translation table

	// find beginning of [nets] section
	do
	{
		if( !pcb_file->ReadString( in_str ))
		{
			// error reading pcb file
			CString mess ((LPCSTR) IDS_UnableToFindNetsSectionInFile);
			AfxMessageBox( mess );
			return;
		}
		in_str.Trim();
	}
	while( in_str != "[nets]" );

	// get each net in [nets] section
	while( 1 )
	{
		pos = (long)pcb_file->GetPosition();
		if (!pcb_file->ReadString( in_str ))
		{
			CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
			throw err_str;
		}
		in_str.Trim();
		if( in_str[0] == '[' && in_str != "[nets]" )
		{
			pcb_file->Seek( pos, CFile::begin );
			return;		// start of next section, reset position and exit
		}
		else if( in_str.Left(4) == "net:" )
		{
			np = ParseKeyString( &in_str, &key_str, &p );
			CString net_name = p[0].Left(MAX_NET_NAME_SIZE);
			net_name.Trim();
			int npins = my_atoi( &p[1] ); 
			int nconnects = my_atoi( &p[2] );
			int nareas = my_atoi( &p[3] );
			int def_width = my_atoi( &p[4] );
			int def_via_w = my_atoi( &p[5] );
			int def_via_hole_w = my_atoi( &p[6] );
			int visible = 1;
			if( np > 8 )
				visible = my_atoi( &p[7] );
			cnet2 *net = new cnet2( m_doc, net_name, def_width, def_via_w, def_via_hole_w );
			net->bVisible = visible;
			for( int ip=0; ip<npins; ip++ )
			{
				if (!pcb_file->ReadString( in_str ))
					throw new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "pin" || np < 3 )
					throw new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);
				CString pin_str = p[1].Left(CShape::MAX_PIN_NAME_SIZE);
				int dot_pos = pin_str.FindOneOf( "." );
				CString ref_str = pin_str.Left( dot_pos );
				CString pin_num_str = pin_str.Right( pin_str.GetLength()-dot_pos-1 );
				cpin2 * pin = net->AddPin( &ref_str, &pin_num_str );
				if (!pin)
				{
					CString s ((LPCSTR) IDS_FatalErrorInNetPinDoesntExist), *err_str = new CString();
					err_str->Format(s, net_name, pin_num_str, ref_str );	
					throw err_str;
				}
				else if( !pin->part )
				{
					CString s ((LPCSTR) IDS_FatalErrorInNetPartDoesntExist), *err_str = new CString();
					err_str->Format(s, net_name, ref_str);
					throw err_str;
				}
				else if( !pin->part->shape )
				{
					CString s ((LPCSTR) IDS_FatalErrorInNetPartDoesntHaveAFootprint), *err_str = new CString();
					err_str->Format(s, net_name, ref_str);
					throw err_str;
				}
				else
					pin->utility = ip;												// CPT2: Index value temporarily stored in pin, so that we can use it
																					// during the reading of connects
			}

			for( int ic=0; ic<nconnects; ic++ )
			{
				if (!pcb_file->ReadString( in_str ))
					throw new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "connect" || np < 6 )
					throw new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);

				// Create the connect with one vertex in it.  More precise data gets loaded later.
				cconnect2 *c = new cconnect2(net);
				cvertex2 *v0 = new cvertex2(c);
				c->Start(v0);
				int start_pin = my_atoi( &p[1] );
				int end_pin = my_atoi( &p[2] );
				cpin2 *pin0 = NULL, *pin1 = NULL;
				if (start_pin != cconnect::NO_END)
				{
					pin0 = net->pins.FindByUtility(start_pin);
					if (!pin0)
					{
						CString s ((LPCSTR) IDS_FatalErrorInNetConnectNonexistentPin), *err_str = new CString();
						err_str->Format(s, net_name, ic+1, start_pin );
						throw err_str;
					}
				}
				v0->pin = pin0;
				if (end_pin != cconnect::NO_END)
				{
					pin1 = net->pins.FindByUtility(end_pin);
					if (!pin1)
					{
						CString s ((LPCSTR) IDS_FatalErrorInNetConnectNonexistentPin), *err_str = new CString();
						err_str->Format(s, net_name, ic+1, end_pin );
						throw err_str;
					}
				}
				// Additional error checks that used to occur here have moved to the loop above...
				int nsegs = my_atoi( &p[3] );
				c->locked = my_atoi( &p[4] );
				
				// Get more info on first vertex
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
					throw err_str;
				}
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "vtx" || np < 8 )
				{
					CString * err_str = new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);
					throw err_str;
				}
				v0->x = my_atoi( &p[1] );
				v0->y = my_atoi( &p[2] );
				// int file_layer = my_atoi( &p[3] ); 
				// first_vtx->pad_layer = in_layer[file_layer];				// CPT2 TODO figure this out... looks bogus
				v0->force_via_flag = my_atoi( &p[4] ); 
				v0->via_w = my_atoi( &p[5] ); 
				v0->via_hole_w = my_atoi( &p[6] );
				v0->tee = NULL;
				int tee_ID = np==9? abs(my_atoi(&p[7])): 0;
				if( tee_ID )
				{
					ctee *tee = net->tees.FindByUtility(tee_ID);
					if (!tee) 
						tee = new ctee(net),				// Constructor adds tee to net->tees
						tee->utility = tee_ID;
					tee->vtxs.Add(v0);
					v0->tee = tee;
				}

				// now add all segments
				for( int is=0; is<nsegs; is++ )
				{
					// read segment data
					err = pcb_file->ReadString( in_str );
					if( !err )
					{
						CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
						throw err_str;
					}
					np = ParseKeyString( &in_str, &key_str, &p );
					if( key_str != "seg" || np < 6 )
					{
						CString * err_str = new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);
						throw err_str;
					}
					int file_layer = my_atoi( &p[1] ); 
					int layer = in_layer[file_layer]; 
					int seg_width = my_atoi( &p[2] );
					cseg2 *seg = new cseg2(c, layer, seg_width);

					// read following vertex data
					err = pcb_file->ReadString( in_str );
					if( !err )
					{
						CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
						throw err_str;
					}
					np = ParseKeyString( &in_str, &key_str, &p );
					if( key_str != "vtx" || np < 8 )
					{
						CString * err_str = new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);
						throw err_str;
					}

					cvertex2 *v = new cvertex2(c);
					v->x = my_atoi( &p[1] ); 
					v->y = my_atoi( &p[2] ); 
					// int file_layer = my_atoi( &p[3] ); 
					// int pad_layer = in_layer[file_layer];						// TODO: Figure this out (bogus?)
					v->force_via_flag = my_atoi( &p[4] ); 
					v->via_w = my_atoi( &p[5] ); 
					v->via_hole_w = my_atoi( &p[6] );
					int tee_ID = np==9? abs(my_atoi(&p[7])): 0;
					if( tee_ID )
					{
						ctee *tee = net->tees.FindByUtility(tee_ID);
						if (!tee) 
							tee = new ctee(net),									// Constructor adds tee to net->tees
							tee->utility = tee_ID;
						tee->vtxs.Add(v);
						v->tee = tee;
					}
					if (is==nsegs-1)
						v->pin = pin1;												// Assign end-pin (possibly NULL) to v if we're at the last vertex.

					c->AppendSegAndVertex(seg, v, c->tail);

					/** this code is for bug in versions before 1.313. CPT2 TODO:  Figure it out
					if( force_via_flag )
					{
						if( end_pin == cconnect::NO_END && is == nsegs-1 )
							ForceVia( net, ic, is+1 );
						else if( read_version > 1.312001 )	// i.e. 1.313 or greater
							ForceVia( net, ic, is+1 );
					}
					*/

#if 0	// AMW r266 not needed anymore
					// if older version of fpc file, negate tee_ID of end_vertex
					// of stub trace to make compatible with version 1.360 and higher
					if( end_pin == cconnect::NO_END 
						&& is == nsegs-1 
						&& read_version < 1.360 
						&& tee_ID > 0 )
					{
						net->connect[ic]->vtx[is+1].tee_ID = -tee_ID;
					}
#endif
					/* CPT2.  Don't get.  I'm just assigning the width values for v during this very iteration (above)
					if( is != 0 )
					{
						// set widths of preceding vertex
						net->connect[ic]->vtx[is].via_w = pre_via_w;
						net->connect[ic]->vtx[is].via_hole_w = pre_via_hole_w;
					}
					pre_via_w = via_w;
					pre_via_hole_w = via_hole_w;
					*/
				}

				/* CPT2 TODO
				// connection created
				// if older version of fpc file, split at tees if needed
				if( read_version < 1.360 )
				{
					cconnect * c = net->ConByIndex( ic );
					// iterate through vertices in reverse
					for( int iv=c->NumVtxs()-2; iv>0; iv-- )
					{
						cvertex * v = &c->VtxByIndex( iv );
						if( v->tee_ID )
						{
							// split into 2 connections
							net->SplitConnectAtVtx( v->Id() );
							nconnects++;
							ic++;
						}
					}
				}
				*/
			} // end for(ic)

			for( int ia=0; ia<nareas; ia++ )
			{
				if (!pcb_file->ReadString( in_str ))
					throw new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "area" || np < 4 )
					throw new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);
				int na = my_atoi( &p[0] );
				if( (na-1) != ia )
					throw new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);
				int ncorners = my_atoi( &p[1] );
				int file_layer = my_atoi( &p[2] );
				int layer = in_layer[file_layer]; 
				int hatch = 1;
				if( np == 5 )
					hatch = my_atoi( &p[3] );
				int last_side_style = CPolyLine::STRAIGHT;
				carea2 *a = new carea2(net, layer, hatch, 2*NM_PER_MIL, 10*NM_PER_MIL);
				ccontour *ctr = new ccontour(a, true);				// Adds ctr as a's main contour

				for( int icor=0; icor<ncorners; icor++ )
				{
					if (!pcb_file->ReadString( in_str ))
						throw new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
					np = ParseKeyString( &in_str, &key_str, &p );
					if( key_str != "corner" || np < 4 )
						throw new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);
					int ncor = my_atoi( &p[0] );
					if( (ncor-1) != icor )
						throw new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);

					int x = my_atoi( &p[1] );
					int y = my_atoi( &p[2] );
					last_side_style = np >= 5? my_atoi( &p[3] ): CPolyLine::STRAIGHT;
					int end_cont = np >= 6? my_atoi( &p[4] ): 0;
					bool bContourWasEmpty = ctr->corners.IsEmpty();
					ccorner *c = new ccorner(ctr, x, y);			// Constructor adds corner to ctr->corners (and may also set ctr->head/tail if
																	// it was previously empty)
					if (!bContourWasEmpty)
					{
						cside *s = new cside(ctr, last_side_style);
						ctr->AppendSideAndCorner(s, c, ctr->tail);
					}
					if( icor == (ncorners-1) || end_cont )
					{
						ctr->Close(last_side_style);
						if (icor<ncorners-1)
							ctr = new ccontour(a, false);		// Make a new secondary contour
					}
				}
			}

			// Delete bogus areas whose main contour has <3 corners
			citer<carea2> ia (&net->areas);
			for (carea2 *a = ia.First(); a; a = ia.Next())
				if (a->main->corners.GetSize() < 3)
					a->Remove();
			
			net->SetThermals();
			net->MustRedraw();
		}
	}
}

void cnetlist::WriteNets( CStdioFile * file )
{
	CString line;
	try
	{
		line.Format( "[nets]\n\n" );
		file->WriteString( line );
		citer<cnet2> in (&nets);
		for (cnet2 *net = in.First(); net; net = in.Next())
		{
			line.Format( "net: \"%s\" %d %d %d %d %d %d %d\n", 
							net->name, net->NumPins(), net->NumCons(), net->NumAreas(),
							net->def_w, net->def_via_w, net->def_via_hole_w,
							net->bVisible );
			file->WriteString( line );
			// CPT2.  First assign ID numbers to each tee (put 'em in the utility field)
			citer<ctee> it (&net->tees);
			int i = 1;
			for (ctee *t = it.First(); t; t = it.Next())
				t->utility = i++;

			// CPT2.  Now output net's pins.  Note that we put the pin's index number (0-based) into the utility field for use when outputting connects
			i = 0;
			citer<cpin2> ip (&net->pins);
			for (cpin2 *p = ip.First(); p; p = ip.Next(), i++)
			{
				p->utility = i;
				line.Format( "  pin: %d %s.%s\n", i+1, p->part->ref_des, p->pin_name );
				file->WriteString( line );
			}

			i = 0;
			citer<cconnect2> ic (&net->connects);
			for (cconnect2 *c = ic.First(); c; c = ic.Next(), i++)
			{
				int start_pin = c->head->pin? c->head->pin->utility: -1;
				int end_pin = c->tail->pin? c->tail->pin->utility: -1;
				line.Format( "  connect: %d %d %d %d %d\n", i+1, start_pin, end_pin, c->NumSegs(), c->locked );
				file->WriteString( line );
				int is = 0;
				for (cvertex2 *v = c->head; v!=c->tail; v = v->postSeg->postVtx, is++)
				{
					line.Format( "    vtx: %d %d %d %d %d %d %d %d\n", 
						is+1, v->x, v->y, v->pin? v->pin->pad_layer: 0, v->force_via_flag, 
						v->via_w, v->via_hole_w, v->tee? v->tee->utility: 0 );
					file->WriteString( line );
					cseg2 *s = v->postSeg;
					line.Format( "    seg: %d %d %d 0 0\n", is+1, s->m_layer, s->m_width );
					file->WriteString( line );
				}
				// last vertex
				cvertex2 *v = c->tail;
				line.Format( "    vtx: %d %d %d %d %d %d %d %d\n", 
					is+1, v->x, v->y, v->pin? v->pin->pad_layer: 0, v->force_via_flag, 
					v->via_w, v->via_hole_w, v->tee? v->tee->utility: 0 );
				file->WriteString( line );
			}

			i = 0;
			citer<carea2> ia (&net->areas);
			for (carea2 *a = ia.First(); a; a = ia.Next(), i++)
			{
				line.Format( "  area: %d %d %d %d\n", i+1, a->NumCorners(), 
					a->m_layer, a->m_hatch );
				file->WriteString( line );
				int icor = 1;
				citer<ccontour> ictr (&a->contours);
				for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
				{
					ccorner *c = ctr->head;
					line.Format( "  corner: %d %d %d %d\n", icor++, c->x, c->y, c->postSide->m_style);
					file->WriteString(line);
					for (c = c->postSide->postCorner; c!=ctr->head; c = c->postSide->postCorner)
					{
						line.Format( "  corner: %d %d %d %d %d\n", icor++, c->x, c->y, 
							c->postSide->m_style, c->postSide->postCorner==ctr->head);
						file->WriteString(line);
					}
				} 
			}
			file->WriteString( "\n" );
		}
	}

	catch( CFileException * e )
	{
		CString str, s ((LPCSTR) IDS_FileError1), s2 ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			str.Format( s, e->m_cause );
		else
			str.Format( s2, e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
	}
}
