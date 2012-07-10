// PolyLine.cpp ... implementation of CPolyLine class

#include "stdafx.h"
#include "math.h"
#include "PolyLine.h"
//#include "FreePcb.h"
//#include "utility.h"
//#include "layers.h"
#include "php_polygon.h"
#include "php_polygon_vertex.h"

extern Cuid pcb_cuid;

#define pi  3.14159265359
#define DENOM 25400	// to use mils for php clipping
//#define DENOM 1			// to use nm for php clipping

//******************** CPolyCorner implementation **********************

CPolyCorner::CPolyCorner() 
{
	m_uid = pcb_cuid.GetNewUID();	
	x = 0; 
	y = 0; 
	end_contour = 0; 
	utility = 0;
	dl_corner_sel = NULL;
}

CPolyCorner::CPolyCorner( CPolyCorner& src ) 
{
	m_uid = pcb_cuid.GetNewUID();	
	x = src.x; 
	y = src.y; 
	end_contour = src.end_contour; 
	utility = src.utility;
	dl_corner_sel = NULL;
}

CPolyCorner::~CPolyCorner()
{
};

// assignment, copy everything except UID and display elements
CPolyCorner& CPolyCorner::operator=( const CPolyCorner& rhs )
{
	if( this != &rhs )
	{
		x = rhs.x;
		y = rhs.y;
		end_contour = rhs.end_contour;
		utility = rhs.utility;
	}
	return *this;
}

// assignment, copy everything except UID and display elements
CPolyCorner& CPolyCorner::operator=( CPolyCorner& rhs )
{
	if( this != &rhs )
	{
		x = rhs.x;
		y = rhs.y;
		end_contour = rhs.end_contour;
		utility = rhs.utility;
	}
	return *this;
}

//******************** CPolySide implementation **********************

CPolySide::CPolySide() 
{ 
	m_uid = pcb_cuid.GetNewUID();	
	m_style = CPolyLine::STRAIGHT;
	dl_side = NULL;
	dl_side_sel = NULL;
}

CPolySide::~CPolySide()
{
};

// assignment, copy everything except UID and display elements
CPolySide& CPolySide::operator=( const CPolySide& rhs )
{
	if( this != &rhs )
	{
		m_style = rhs.m_style;
	}
	return *this;
}

// assignment, copy everything except UID and display elements
CPolySide& CPolySide::operator=( CPolySide& rhs )
{
	if( this != &rhs )
	{
		m_style = rhs.m_style;
	}
	return *this;
}



//******************** CPolyLine constructor ***********************
// constructor:

CPolyLine::CPolyLine()
{ 
	m_uid = pcb_cuid.GetNewUID();
	m_dlist = NULL;
	m_ptr = 0;
	m_hatch = 0;
	m_sel_box = 0;
	m_gpc_poly = new gpc_polygon;
	m_gpc_poly->num_contours = 0;
	m_php_poly = new polygon;
	bDrawn = 0;
	m_parent_id.Clear();
}

// destructor, remove all elements
//
CPolyLine::~CPolyLine()
{
	Clear();
}

void CPolyLine::Clear()
{
	Undraw();
	corner.SetSize(0);
	side.SetSize(0);
	FreeGpcPoly();
	delete m_gpc_poly;
	delete m_php_poly;
}

int CPolyLine::SizeOfUndoRecord()
{
	return sizeof(undo_poly) + NumCorners()*sizeof(undo_corner);
}

// This sets an existing PolyLine from an undo record
//
void CPolyLine::SetFromUndo( undo_poly * un_poly )
{
	// clear out this poly
	Undraw();
	corner.SetSize(0);
	side.SetSize(0);
	FreeGpcPoly();
	// replace poly from undo record
	int nc = un_poly->ncorners;
	undo_corner * un_corner = (undo_corner*)((UINT)un_poly + sizeof(undo_poly));
	Start( un_poly->layer, un_poly->width, un_poly->sel_box, 
		un_corner[0].x, un_corner[0].y, un_poly->hatch, &un_poly->root_id, NULL );
	SetUID( un_poly->uid );
	SetCornerUID( 0, un_corner[0].uid ); 
	for( int ic=1; ic<nc; ic++ )
	{
		AppendCorner( un_corner[ic].x, un_corner[ic].y, un_corner[ic-1].side_style );
		SetCornerUID( ic, un_corner[ic].uid ); 
		SetSideUID( ic-1, un_corner[ic-1].side_uid ); 
	}
	Close( un_corner[nc-1].side_style );
	SetSideUID( nc-1, un_corner[nc-1].side_uid ); 
	Draw(); 
}

// create undo record for PolyLine;
//
void CPolyLine::CreatePolyUndoRecord( undo_poly * un_poly )
{
	int ncorners = NumCorners();

	un_poly->poly = this;
	un_poly->uid = m_uid;
	un_poly->layer = Layer();
	un_poly->width = W();
	un_poly->root_id = Id();
	un_poly->sel_box = m_sel_box;;
	un_poly->hatch = m_hatch;
	un_poly->ncorners = ncorners;
	undo_corner * un_corner = (undo_corner*)((UINT)un_poly + sizeof(undo_poly));
	for( int ic=0; ic<ncorners; ic++ )
	{
		un_corner[ic].uid = CornerUID( ic );
		un_corner[ic].x = X( ic );
		un_corner[ic].y = Y( ic );
		un_corner[ic].end_contour = EndContour( ic );
		un_corner[ic].side_uid = SideUID( ic );
		un_corner[ic].side_style = SideStyle( ic );
	}
	return;
}

// Use the General Polygon Clipping Library to clip contours
// If this results in new polygons, return them as CArray p
// If bRetainArcs == TRUE, try to retain arcs in polys
// Returns number of external contours, or -1 if error
//
int CPolyLine::NormalizeWithGpc( CArray<CPolyLine*> * pa, BOOL bRetainArcs )
{
	CArray<CArc> arc_array;

	if( bRetainArcs )
		MakeGpcPoly( -1, &arc_array );
	else
		MakeGpcPoly( -1, NULL );

	BOOL bWasDrawn = bDrawn;
	if( bDrawn )
		Undraw();

	// now, recreate poly
	// first, find outside contours and create new CPolyLines if necessary
	int n_ext_cont = 0;
	for( int ic=0; ic<m_gpc_poly->num_contours; ic++ )
	{
		if( !(m_gpc_poly->hole)[ic] )
		{
			if( n_ext_cont == 0 )
			{
				if( m_gpc_poly->contour[ic].num_vertices != NumCorners() )	//**
				{
					// first external contour, replace this poly if necessary
					corner.RemoveAll();
					side.RemoveAll();
					for( int i=0; i<m_gpc_poly->contour[ic].num_vertices; i++ )
					{
						int x = ((m_gpc_poly->contour)[ic].vertex)[i].x;
						int y = ((m_gpc_poly->contour)[ic].vertex)[i].y;
						if( i==0 )
							Start( m_layer, m_w, m_sel_box, x, y, m_hatch, &m_parent_id, m_ptr );
						else
							AppendCorner( x, y, STRAIGHT );
					}
					Close();
					n_ext_cont++;
				}
			}
			else if( pa )
			{
				// next external contour, create new poly
				CPolyLine * poly = new CPolyLine;
				pa->SetSize(n_ext_cont);	// put in array
				(*pa)[n_ext_cont-1] = poly;
				for( int i=0; i<m_gpc_poly->contour[ic].num_vertices; i++ )
				{
					int x = ((m_gpc_poly->contour)[ic].vertex)[i].x;
					int y = ((m_gpc_poly->contour)[ic].vertex)[i].y;
					if( i==0 )
						poly->Start( m_layer, m_w, m_sel_box, x, y, m_hatch, &m_parent_id, m_ptr, FALSE );
					else
						poly->AppendCorner( x, y, STRAIGHT );
				}
				poly->Close( STRAIGHT );
				n_ext_cont++;
			}
		}
	}


	// now add cutouts to the CPolyLine(s)
	for( int ic=0; ic<m_gpc_poly->num_contours; ic++ ) 
	{
		if( (m_gpc_poly->hole)[ic] )
		{
			CPolyLine * ext_poly = NULL;
			if( n_ext_cont == 1 )
			{
				ext_poly = this;
			}
			else
			{
				// find the polygon that contains this hole
				for( int i=0; i<m_gpc_poly->contour[ic].num_vertices; i++ )
				{
					int x = ((m_gpc_poly->contour)[ic].vertex)[i].x;
					int y = ((m_gpc_poly->contour)[ic].vertex)[i].y;
					if( TestPointInside( x, y ) )
						ext_poly = this;
					else
					{
						for( int ext_ic=0; ext_ic<n_ext_cont-1; ext_ic++ )
						{
							if( (*pa)[ext_ic]->TestPointInside( x, y ) )
							{
								ext_poly = (*pa)[ext_ic];
								break;
							}
						}
					}
					if( ext_poly )
						break;
				}
			}
			if( !ext_poly )
				ASSERT(0);
			for( int i=0; i<m_gpc_poly->contour[ic].num_vertices; i++ )
			{
				int x = ((m_gpc_poly->contour)[ic].vertex)[i].x;
				int y = ((m_gpc_poly->contour)[ic].vertex)[i].y;
				ext_poly->AppendCorner( x, y, STRAIGHT );
			}
			ext_poly->Close( STRAIGHT );
		}
	}
	if( bRetainArcs )
		RestoreArcs( &arc_array, pa );
	FreeGpcPoly();

	if( bWasDrawn )
		Draw();

	return n_ext_cont;
}

// make a php_polygon from first contour
int CPolyLine::MakePhpPoly()
{
	FreePhpPoly();
	polygon test_poly;
	int nv = ContourEnd(0);
	for( int iv=0; iv<=nv; iv++ )
	{
		int x = X(iv)/DENOM;
		int y = Y(iv)/DENOM;
		m_php_poly->addv( x, y );
	}
	return 0;
}

void CPolyLine::FreePhpPoly()
{
	// delete all vertices
	while( m_php_poly->m_cnt > 1 )
	{
		vertex * fv = m_php_poly->getFirst();
		m_php_poly->del( fv->m_nextV );
	}
	delete m_php_poly->m_first;
	m_php_poly->m_first = NULL;
	m_php_poly->m_cnt = 0;
}

// Use the php clipping lib to clip this poly against poly
//
void CPolyLine::ClipPhpPolygon( int php_op, CPolyLine * poly )
{
	Undraw();
	poly->MakePhpPoly();
	MakePhpPoly();
	polygon * p = m_php_poly->boolean( poly->m_php_poly, php_op );
	poly->FreePhpPoly();
	FreePhpPoly();

	if( p )
	{
		// now screw with the PolyLine
		corner.RemoveAll();
		side.RemoveAll();
		do
		{
			vertex * v = p->getFirst();
			Start( m_layer, m_w, m_sel_box, v->X()*DENOM, v->Y()*DENOM, m_hatch, &m_parent_id, m_ptr );
			do
			{
				vertex * n = v->Next();
				AppendCorner( v->X()*DENOM, v->Y()*DENOM );
				v = n;
			}
			while( v->id() != p->getFirst()->id() );
			Close();
//			p = p->NextPoly();
			delete p;
			p = NULL;
		}
		while( p );
	}
	Draw();
}

// make a gpc_polygon for a closed polyline contour
// approximates arcs with multiple straight-line segments
// if icontour = -1, make polygon with all contours,
// combining intersecting contours if possible
// returns data on arcs in arc_array
//
int CPolyLine::MakeGpcPoly( int icontour, CArray<CArc> * arc_array )
{
	if( m_gpc_poly->num_contours )
		FreeGpcPoly();
	if( !Closed() && (icontour == (NumContours()-1) || icontour == -1))
		return 1;	// error

	// initialize m_gpc_poly
	m_gpc_poly->num_contours = 0;
	m_gpc_poly->hole = NULL;
	m_gpc_poly->contour = NULL;
	int n_arcs = 0;

	int first_contour = icontour;
	int last_contour = icontour;
	if( icontour == -1 )
	{
		first_contour = 0;
		last_contour = NumContours() - 1;
	}
	if( arc_array )
		arc_array->SetSize(0);
	int iarc = 0;
	for( int icont=first_contour; icont<=last_contour; icont++ )
	{
		// make gpc_polygon for this contour
		gpc_polygon * gpc = new gpc_polygon;
		gpc->num_contours = 0;
		gpc->hole = NULL;
		gpc->contour = NULL;

		// first, calculate number of vertices in contour
		int n_vertices = 0;
		int ic_st = ContourStart(icont);
		int ic_end = ContourEnd(icont);
		for( int ic=ic_st; ic<=ic_end; ic++ )
		{
			int style = side[ic].m_style;
			int x1 = corner[ic].x;
			int y1 = corner[ic].y;
			int x2, y2;
			if( ic < ic_end )
			{
				x2 = corner[ic+1].x;
				y2 = corner[ic+1].y;
			}
			else
			{
				x2 = corner[ic_st].x;
				y2 = corner[ic_st].y;
			}
			if( style == STRAIGHT )
				n_vertices++;
			else
			{
				// style is ARC_CW or ARC_CCW
				int n;	// number of steps for arcs
				n = (abs(x2-x1)+abs(y2-y1))/(CArc::MAX_STEP);
				n = max( n, CArc::MIN_STEPS );	// or at most 5 degrees of arc
				n_vertices += n;
				n_arcs++;
			}
		}
		// now create gcp_vertex_list for this contour
		gpc_vertex_list * g_v_list = new gpc_vertex_list;
		g_v_list->vertex = (gpc_vertex*)calloc( sizeof(gpc_vertex), n_vertices );
		g_v_list->num_vertices = n_vertices;
		int ivtx = 0;
		for( int ic=ic_st; ic<=ic_end; ic++ )
		{
			int style = side[ic].m_style;
			int x1 = corner[ic].x;
			int y1 = corner[ic].y;
			int x2, y2;
			if( ic < ic_end )
			{
				x2 = corner[ic+1].x;
				y2 = corner[ic+1].y;
			}
			else
			{
				x2 = corner[ic_st].x;
				y2 = corner[ic_st].y;
			}
			if( style == STRAIGHT )
			{
				g_v_list->vertex[ivtx].x = x1;
				g_v_list->vertex[ivtx].y = y1;
				ivtx++;
			}
			else
			{
				// style is arc_cw or arc_ccw
				int n;	// number of steps for arcs
				n = (abs(x2-x1)+abs(y2-y1))/(CArc::MAX_STEP);
				n = max( n, CArc::MIN_STEPS );	// or at most 5 degrees of arc
				double xo, yo, theta1, theta2, a, b;
				a = fabs( (double)(x1 - x2) );
				b = fabs( (double)(y1 - y2) );
				if( style == CPolyLine::ARC_CW )
				{
					// clockwise arc (ie.quadrant of ellipse)
					int i=0, j=0;
					if( x2 > x1 && y2 > y1 )
					{
						// first quadrant, draw second quadrant of ellipse
						xo = x2;	
						yo = y1;
						theta1 = pi;
						theta2 = pi/2.0;
					}
					else if( x2 < x1 && y2 > y1 )
					{
						// second quadrant, draw third quadrant of ellipse
						xo = x1;	
						yo = y2;
						theta1 = 3.0*pi/2.0;
						theta2 = pi;
					}
					else if( x2 < x1 && y2 < y1 )	
					{
						// third quadrant, draw fourth quadrant of ellipse
						xo = x2;	
						yo = y1;
						theta1 = 2.0*pi;
						theta2 = 3.0*pi/2.0;
					}
					else
					{
						xo = x1;	// fourth quadrant, draw first quadrant of ellipse
						yo = y2;
						theta1 = pi/2.0;
						theta2 = 0.0;
					}
				}
				else
				{
					// counter-clockwise arc
					int i=0, j=0;
					if( x2 > x1 && y2 > y1 )
					{
						xo = x1;	// first quadrant, draw fourth quadrant of ellipse
						yo = y2;
						theta1 = 3.0*pi/2.0;
						theta2 = 2.0*pi;
					}
					else if( x2 < x1 && y2 > y1 )
					{
						xo = x2;	// second quadrant
						yo = y1;
						theta1 = 0.0;
						theta2 = pi/2.0;
					}
					else if( x2 < x1 && y2 < y1 )	
					{
						xo = x1;	// third quadrant
						yo = y2;
						theta1 = pi/2.0;
						theta2 = pi;
					}
					else
					{
						xo = x2;	// fourth quadrant
						yo = y1;
						theta1 = pi;
						theta2 = 3.0*pi/2.0;
					}
				}
				// now write steps for arc
				if( arc_array )
				{
					arc_array->SetSize(iarc+1);
					(*arc_array)[iarc].style = style;
					(*arc_array)[iarc].n_steps = n;
					(*arc_array)[iarc].xi = x1;
					(*arc_array)[iarc].yi = y1;
					(*arc_array)[iarc].xf = x2;
					(*arc_array)[iarc].yf = y2;
					iarc++;
				}
				for( int is=0; is<n; is++ )
				{
					double theta = theta1 + ((theta2-theta1)*(double)is)/n;
					double x = xo + a*cos(theta);
					double y = yo + b*sin(theta);
					if( is == 0 )
					{
						x = x1;
						y = y1;
					}
					g_v_list->vertex[ivtx].x = x;
					g_v_list->vertex[ivtx].y = y;
					ivtx++;
				}
			}
		}
		if( n_vertices != ivtx )
			ASSERT(0);
		// add vertex_list to gpc
		gpc_add_contour( gpc, g_v_list, 0 );
		// now clip m_gpc_poly with gpc, put new poly into result
		gpc_polygon * result = new gpc_polygon;
		if( icontour == -1 && icont != 0 )
			gpc_polygon_clip( GPC_DIFF, m_gpc_poly, gpc, result );	// hole
		else
			gpc_polygon_clip( GPC_UNION, m_gpc_poly, gpc, result );	// outside
		// now copy result to m_gpc_poly
		gpc_free_polygon( m_gpc_poly );
		delete m_gpc_poly;
		m_gpc_poly = result;
		gpc_free_polygon( gpc );
		delete gpc;
		free( g_v_list->vertex );
		free( g_v_list );
	}
	return 0;
}

int CPolyLine::FreeGpcPoly()
{
	if( m_gpc_poly->num_contours )
	{
		delete m_gpc_poly->contour->vertex;
		delete m_gpc_poly->contour;
		delete m_gpc_poly->hole;
	}
	m_gpc_poly->num_contours = 0;
	return 0;
}


// Restore arcs to a polygon where they were replaced with steps
// If pa != NULL, also use polygons in pa array
//
int CPolyLine::RestoreArcs( CArray<CArc> * arc_array, CArray<CPolyLine*> * pa )
{
	// get poly info
	int n_polys = 1;
	if( pa )
		n_polys += pa->GetSize();
	CPolyLine * poly;

	// undraw polys and clear utility flag for all corners
	for( int ip=0; ip<n_polys; ip++ )
	{
		if( ip == 0 )
			poly = this;
		else
			poly = (*pa)[ip-1];
		poly->Undraw();
		for( int ic=0; ic<poly->NumCorners(); ic++ )
			poly->SetUtility( ic, 0 );	// clear utility flag
	}

	// find arcs and replace them
	BOOL bFound;
	int arc_start;
	int arc_end;
	for( int iarc=0; iarc<arc_array->GetSize(); iarc++ )
	{
		int arc_xi = (*arc_array)[iarc].xi;
		int arc_yi = (*arc_array)[iarc].yi;
		int arc_xf = (*arc_array)[iarc].xf;
		int arc_yf = (*arc_array)[iarc].yf;
		int n_steps = (*arc_array)[iarc].n_steps;
		int style = (*arc_array)[iarc].style;
		bFound = FALSE;
		// loop through polys
		for( int ip=0; ip<n_polys; ip++ )
		{
			if( ip == 0 )
				poly = this;
			else
				poly = (*pa)[ip-1];
			for( int icont=0; icont<poly->NumContours(); icont++ )
			{
				int ic_start = poly->ContourStart(icont);
				int ic_end = poly->ContourEnd(icont);
				if( (ic_end-ic_start) > n_steps )
				{
					for( int ic=ic_start; ic<=ic_end; ic++ )
					{
						int ic_next = ic+1;
						if( ic_next > ic_end )
							ic_next = ic_start;
						int xi = poly->X(ic);
						int yi = poly->Y(ic);
						if( xi == arc_xi && yi == arc_yi )
						{
							// test for forward arc
							int ic2 = ic + n_steps;
							if( ic2 > ic_end )
								ic2 = ic2 - ic_end + ic_start - 1;
							int xf = poly->X(ic2);
							int yf = poly->Y(ic2);
							if( xf == arc_xf && yf == arc_yf )
							{
								// arc from ic to ic2
								bFound = TRUE;
								arc_start = ic;
								arc_end = ic2;
							}
							else
							{
								// try reverse arc
								ic2 = ic - n_steps;
								if( ic2 < ic_start )
									ic2 = ic2 - ic_start + ic_end + 1;
								xf = poly->X(ic2);
								yf = poly->Y(ic2);
								if( xf == arc_xf && yf == arc_yf )
								{
									// arc from ic2 to ic
									bFound = TRUE; 
									arc_start = ic2;
									arc_end = ic;
									style = 3 - style;
								}
							}
							if( bFound )
							{
								poly->side[arc_start].m_style = style;
								// mark corners for deletion from arc_start+1 to arc_end-1
								for( int i=arc_start+1; i!=arc_end; )
								{
									if( i > ic_end )
										i = ic_start;
									poly->SetUtility( i, 1 );
									if( i == ic_end )
										i = ic_start;
									else
										i++;
								}
								break;
							}
						}
						if( bFound )
							break;
					}
				}
				if( bFound )
					break;
			}
		}
		if( bFound )
			(*arc_array)[iarc].bFound = TRUE;
	}

	// now delete all marked corners
	for( int ip=0; ip<n_polys; ip++ )
	{
		if( ip == 0 )
			poly = this;
		else
			poly = (*pa)[ip-1];
		for( int ic=poly->NumCorners()-1; ic>=0; ic-- )
		{
			if( poly->Utility(ic) )
				poly->DeleteCorner( ic );
		}
	}
	return 0;
}

// initialize new polyline
// set layer, width, selection box size, starting point, id and pointer
//
// if sel_box = 0, don't create selection elements at all
//
// if polyline is board outline, enter with:
//	id.t1 = ID_BOARD
//	id.SetT2( ID_OUTLINE
//	id.I2() = index to outline
//	ptr = NULL
//
// if polyline is copper area, enter with:
//	id.t1 = ID_NET;
//	id.SetT2( ID_AREA
//	id.I2() = index to area
//	ptr = pointer to net
//
void CPolyLine::Start( int layer, int w, int sel_box, int x, int y, 
					  int hatch, id * set_id, void * ptr, BOOL bDraw )
{
	m_layer = layer;
	m_w = w;
	m_sel_box = sel_box;

	// set id, using the one provided
	if( set_id )
		m_parent_id = *set_id;
	else
		m_parent_id.Clear();
	m_parent_id.SetU2( m_uid );	// use this poly's own uid

	m_ptr = ptr;
	m_hatch = hatch;
	corner.SetSize(1);
	side.SetSize(1);
	corner[0].x = x;
	corner[0].y = y;
	corner[0].end_contour = FALSE;
	if( m_sel_box && m_dlist && bDraw )
	{
		// create id for selection rect
		id sel_id = m_parent_id;
		sel_id.SetT3( ID_SEL_CORNER );
		sel_id.SetI3( 0 );					
		sel_id.SetU3( corner[0].m_uid );	
		corner[0].dl_corner_sel = m_dlist->AddSelector( sel_id, m_ptr, m_layer, DL_HOLLOW_RECT, 
			1, 0, 0, x-m_sel_box, y-m_sel_box, 
			x+m_sel_box, y+m_sel_box, 0, 0 );
	}
}

// add a corner to unclosed polyline
//
void CPolyLine::AppendCorner( int x, int y, int style )
{
	BOOL bWasDrawn = bDrawn;
	if( bDrawn )
		Undraw();

	// increase size of arrays
	int ncors = NumCorners();
	corner.SetSize( ncors+1 );
	side.SetSize( ncors+1 );

	// add entries for new corner and side
	corner[ncors].x = x;
	corner[ncors].y = y;
	corner[ncors].end_contour = FALSE;

	if( !corner[ncors-1].end_contour )
	{
		side[ncors-1].m_style = style;
	}
	int dl_type;
	if( style == CPolyLine::STRAIGHT )
		dl_type = DL_LINE;
	else if( style == CPolyLine::ARC_CW )
		dl_type = DL_ARC_CW;
	else if( style == CPolyLine::ARC_CCW )
		dl_type = DL_ARC_CCW;
	else
		ASSERT(0);
	if( bWasDrawn )
		Draw();
}

// close last polyline contour
//
void CPolyLine::Close( int style )
{
	BOOL bWasDrawn = bDrawn;
	if( bDrawn )
		Undraw();

	if( Closed() )
		ASSERT(0);
	Undraw();
	int ncors = NumCorners();
	side[ncors-1].m_style = style;
	corner[ncors-1].end_contour = TRUE;

	if( bWasDrawn )
		Draw();
}

// move corner of polyline
// if bEnforceCircularArcs == TRUE, convert adjacent sides to STRAIGHT if angle not
// a multiple of 45 degrees and return TRUE, otherwise return FALSE
//
BOOL CPolyLine::MoveCorner( int ic, int x, int y, BOOL bEnforceCircularArcs )
{
	Undraw();
	corner[ic].x = x;
	corner[ic].y = y;
	BOOL bReturn = FALSE;
	if( bEnforceCircularArcs )
	{
		int icont = Contour( ic );
		int icont_start = ContourStart( icont );
		int icont_end = ContourEnd( icont );
		int ic_prev, ic_next;
		if( ic == icont_start )
		{
			// ic is first corner in contour
			ic_next = ic+1;
			ic_prev = icont_end;
		}
		else if( ic == icont_end )
		{
			// ic is last corner in contour
			ic_next = icont_start;
			ic_prev = ic - 1;
		}
		else
		{
			ic_next = ic+1;
			ic_prev = ic-1;
		}
		if( abs(X(ic)-X(ic_next)) != abs(Y(ic)-Y(ic_next)) )
		{
			// endpoints not at multiple of 45 degree angle
			SetSideStyle(ic, STRAIGHT );
			bReturn = TRUE;
		}
		if( abs(X(ic)-X(ic_prev)) != abs(Y(ic)-Y(ic_prev)) )
		{
			// endpoints not at multiple of 45 degree angle
			SetSideStyle(ic_prev, STRAIGHT );
			bReturn = TRUE;
		}
	}
	Draw();
	return bReturn;
}

// delete corner and adjust arrays
//
void CPolyLine::DeleteCorner( int ic )
{
	BOOL bWasDrawn = bDrawn;
	if( bWasDrawn )
		Undraw();

	int icont = Contour( ic );
	int istart = ContourStart( icont );
	int iend = ContourEnd( icont );
	BOOL bClosed = icont < NumContours()-1 || Closed();

	if( !bClosed )
	{
		// open contour, must be last contour
		corner.RemoveAt( ic ); 
		if( ic != istart )
			side.RemoveAt( ic-1 );
	}
	else
	{
		// closed contour
		corner.RemoveAt( ic ); 
		side.RemoveAt( ic );
		if( ic == iend )
			corner[ic-1].end_contour = TRUE;
	}
	if( bClosed && ContourSize(icont) < 3 )
	{
		// delete the entire contour
		RemoveContour( icont );
	}
	if( bWasDrawn )
		Draw();
}

void CPolyLine::RemoveContour( int icont )
{
	Undraw();
	int istart = ContourStart( icont );
	int iend = ContourEnd( icont );

	if( icont == 0 && NumContours() == 1 )
	{
		// remove the only contour
		ASSERT(0);
	}
	else if( icont == NumContours()-1 )
	{
		// remove last contour
		corner.SetSize( ContourStart(icont) );
		side.SetSize( ContourStart(icont) );
	}
	else
	{
		// remove closed contour
		for( int ic=iend; ic>=istart; ic-- )
		{
			corner.RemoveAt( ic );
			side.RemoveAt( ic );
		}
	}
	Draw();
}

// insert a new corner between two existing corners
//
void CPolyLine::InsertCorner( int ic, int x, int y )
{
	if( ic == 0 )
		ASSERT(0);
	BOOL bWasDrawn = bDrawn;
	if( bWasDrawn )
		Undraw();

	CPolyCorner new_corner;
	new_corner.x = x;
	new_corner.y = y;
	corner.InsertAt( ic, new_corner );

	side.InsertAt( ic, CPolySide() );	// copy old side into new side
	side[ic] = side[ic-1];
	if( ic )
	{
		if( corner[ic-1].end_contour )
		{
			corner[ic].end_contour = TRUE;
			corner[ic-1].end_contour = FALSE;
		}
	}

	if( bWasDrawn )
		Draw();
}

// undraw polyline by removing all graphic elements from display list
//
void CPolyLine::Undraw()
{
	if( m_dlist && bDrawn )
	{
		// remove display elements, if present
		for( int i=0; i<NumSides(); i++ )
		{
			m_dlist->Remove( side[i].dl_side );
			m_dlist->Remove( side[i].dl_side_sel );
			side[i].dl_side = NULL;
			side[i].dl_side_sel = NULL;
		}
		for( int i=0; i<NumCorners(); i++ )
		{
			m_dlist->Remove( corner[i].dl_corner_sel );
			corner[i].dl_corner_sel = NULL;
		}
		for( int i=0; i<dl_hatch.GetSize(); i++ )
			m_dlist->Remove( dl_hatch[i] );

		// remove pointers
		dl_hatch.RemoveAll();

		m_nhatch = 0;
	}
	bDrawn = FALSE;
}

// draw polyline by adding all graphics to display list
// if side style is ARC_CW or ARC_CCW but endpoints are not angled,
// convert to STRAIGHT
//
void CPolyLine::Draw(  CDisplayList * dl )
{
	int i_start_contour = 0;

	// first, undraw if necessary
	if( bDrawn )
		Undraw(); 

	// use new display list if provided
	if( dl )
		m_dlist = dl;

	if( m_dlist )
	{
		// draw elements
		for( int ic=0; ic<NumCorners(); ic++ )
		{
			id sel_id = m_parent_id;	// id for selection rects
			sel_id.SetSubSubType( ID_SEL_CORNER, corner[ic].m_uid, ic ); // id for corner

			int xi = corner[ic].x;
			int yi = corner[ic].y;
			int xf, yf;
			if( corner[ic].end_contour == FALSE && ic < NumCorners()-1 )
			{
				xf = corner[ic+1].x;
				yf = corner[ic+1].y;
			}
			else
			{
				xf = corner[i_start_contour].x;
				yf = corner[i_start_contour].y;
				i_start_contour = ic+1;
			}
			// draw
			if( m_sel_box )
			{
				corner[ic].dl_corner_sel = m_dlist->AddSelector( sel_id, m_ptr, m_layer, DL_HOLLOW_RECT, 
					1, 0, 0, xi-m_sel_box, yi-m_sel_box, 
					xi+m_sel_box, yi+m_sel_box, 0, 0 );
			}
			if( ic<(NumCorners()-1) || corner[ic].end_contour )
			{
				// draw side
				sel_id.SetSubSubType( ID_SIDE, side[ic].m_uid, ic ); // id for side
				if( xi == xf || yi == yf )
				{
					// if endpoints not angled, make side STRAIGHT
					side[ic].m_style = STRAIGHT;
				}
				int g_type = DL_LINE;
				if( side[ic].m_style == STRAIGHT )
					g_type = DL_LINE;
				else if( side[ic].m_style == ARC_CW )
					g_type = DL_ARC_CW;
				else if( side[ic].m_style == ARC_CCW )
					g_type = DL_ARC_CCW;
				side[ic].dl_side = m_dlist->Add( sel_id, m_ptr, m_layer, g_type, 
					1, m_w, 0, 0, xi, yi, xf, yf, 0, 0 );

				// draw selection rect
				if( m_sel_box )
				{
					sel_id.SetT3( ID_SEL_SIDE );
					side[ic].dl_side_sel = m_dlist->AddSelector( sel_id, m_ptr, m_layer, g_type, 
						1, m_w, 0, xi, yi, xf, yf, 0, 0 );
				}
			}
		}
		if( m_hatch )
			Hatch();
	}
	bDrawn = TRUE;
}

void CPolyLine::SetSideVisible( int is, int visible )
{
	if( m_dlist && side.GetSize() > is )
	{
		m_dlist->Set_visible( side[is].dl_side, visible );
	}
}

// start dragging new corner to be inserted into side, make side and hatching invisible
//
void CPolyLine::StartDraggingToInsertCorner( CDC * pDC, int ic, int x, int y, int crosshair )
{
	if( !m_dlist )
		ASSERT(0);

	int icont = Contour( ic );
	int istart = ContourStart( icont );
	int iend = ContourEnd( icont );
	int post_c;

	if( ic == iend )
		post_c = istart;
	else
		post_c = ic + 1;
	int xi = corner[ic].x;
	int yi = corner[ic].y;
	int xf = corner[post_c].x;
	int yf = corner[post_c].y;
	m_dlist->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, 
		LAY_SELECTION, LAY_SELECTION, 1, 1, DSS_STRAIGHT, DSS_STRAIGHT,
		0, 0, 0, 0, crosshair );
	m_dlist->CancelHighLight();
	m_dlist->Set_visible( side[ic].dl_side, 0 );
	for( int ih=0; ih<m_nhatch; ih++ )
		m_dlist->Set_visible( dl_hatch[ih], 0 );
}

// cancel dragging inserted corner, make side and hatching visible again
//
void CPolyLine::CancelDraggingToInsertCorner( int ic )
{
	if( !m_dlist )
		ASSERT(0);

	int post_c;
	if( ic == (NumCorners()-1) )
		post_c = 0;
	else
		post_c = ic + 1;
	m_dlist->StopDragging();
	m_dlist->Set_visible( side[ic].dl_side, 1 );
	for( int ih=0; ih<m_nhatch; ih++ )
		m_dlist->Set_visible( dl_hatch[ih], 1 );
}

// start dragging corner to new position, make adjacent sides and hatching invisible
//
void CPolyLine::StartDraggingToMoveCorner( CDC * pDC, int ic, int x, int y, int crosshair )
{
	if( !m_dlist )
		ASSERT(0);

	// see if corner is the first or last corner of an open contour
	int icont = Contour( ic );
	int istart = ContourStart( icont );
	int iend = ContourEnd( icont );
	if( !Closed()
		&& icont == NumContours() - 1
		&& (ic == istart || ic == iend) )
	{
		// yes
		int style, xi, yi, iside;
		if( ic == istart )
		{
			// first corner
			iside = ic;
			xi = X( ic+1 );
			yi = Y( ic+1 );
			style = SideStyle( iside );
			// reverse arc since we are drawing from corner 1 to 0
			if( style == CPolyLine::ARC_CW )
				style = CPolyLine::ARC_CCW;
			else if( style == CPolyLine::ARC_CCW )
				style = CPolyLine::ARC_CW;
		}
		else
		{
			// last corner
			iside = ic - 1;
			xi = X( ic-1 );
			yi = Y( ic-1);
			style = SideStyle( iside );
		}		
		m_dlist->StartDraggingArc( pDC, style, X(ic), Y(ic), xi, yi, LAY_SELECTION, 1, crosshair );
		m_dlist->CancelHighLight();
		m_dlist->Set_visible( side[iside].dl_side, 0 );
		for( int ih=0; ih<m_nhatch; ih++ )
			m_dlist->Set_visible( dl_hatch[ih], 0 );
	}
	else
	{
		// no
		// get indexes for preceding and following corners
		int pre_c, post_c;
		int poly_side_style1, poly_side_style2;
		int style1, style2;
		if( ic == istart )
		{
			pre_c = iend;
			post_c = istart+1;
			poly_side_style1 = side[iend].m_style;
			poly_side_style2 = side[istart].m_style;
		}
		else if( ic == iend )
		{
			// last side
			pre_c = ic-1;
			post_c = istart;
			poly_side_style1 = side[ic-1].m_style;
			poly_side_style2 = side[ic].m_style;
		}
		else
		{
			pre_c = ic-1;
			post_c = ic+1;
			poly_side_style1 = side[ic-1].m_style;
			poly_side_style2 = side[ic].m_style;
		}
		if( poly_side_style1 == STRAIGHT )
			style1 = DSS_STRAIGHT;
		else if( poly_side_style1 == ARC_CW )
			style1 = DSS_ARC_CW;
		else if( poly_side_style1 == ARC_CCW )
			style1 = DSS_ARC_CCW;
		if( poly_side_style2 == STRAIGHT )
			style2 = DSS_STRAIGHT;
		else if( poly_side_style2 == ARC_CW )
			style2 = DSS_ARC_CW;
		else if( poly_side_style2 == ARC_CCW )
			style2 = DSS_ARC_CCW;
		int xi = corner[pre_c].x;
		int yi = corner[pre_c].y;
		int xf = corner[post_c].x;
		int yf = corner[post_c].y;
		m_dlist->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, 
			LAY_SELECTION, LAY_SELECTION, 1, 1, style1, style2, 
			0, 0, 0, 0, crosshair );
		m_dlist->CancelHighLight();
		m_dlist->Set_visible( side[pre_c].dl_side, 0 );
		m_dlist->Set_visible( side[ic].dl_side, 0 );
		for( int ih=0; ih<m_nhatch; ih++ )
			m_dlist->Set_visible( dl_hatch[ih], 0 );
	}
}

// cancel dragging corner to new position, make sides and hatching visible again
//
void CPolyLine::CancelDraggingToMoveCorner( int ic )
{
	if( !m_dlist )
		ASSERT(0);

	// get indexes for preceding and following sides
	int pre_c;
	if( ic == 0 )
	{
		pre_c = NumCorners()-1;
	}
	else
	{
		pre_c = ic-1;
	}
	m_dlist->StopDragging();
	m_dlist->Set_visible( side[pre_c].dl_side, 1 );
	m_dlist->Set_visible( side[ic].dl_side, 1 );
	for( int ih=0; ih<m_nhatch; ih++ )
		m_dlist->Set_visible( dl_hatch[ih], 1 );
}


// highlight side by drawing line over it
//
void CPolyLine::HighlightSide( int is )
{
	if( !m_dlist )
		ASSERT(0);
	if( Closed() && is >= NumCorners() )
		return;
	if( !Closed() && is >= (NumCorners()-1) )
		return;

	int style;
	if( side[is].m_style == CPolyLine::STRAIGHT )
		style = DL_LINE;
	else if( side[is].m_style == CPolyLine::ARC_CW )
		style = DL_ARC_CW;
	else if( side[is].m_style == CPolyLine::ARC_CCW )
		style = DL_ARC_CCW;
	m_dlist->HighLight( style, 
		m_dlist->Get_x( side[is].dl_side_sel ),
		m_dlist->Get_y( side[is].dl_side_sel ),
		m_dlist->Get_xf( side[is].dl_side_sel ),
		m_dlist->Get_yf( side[is].dl_side_sel ),
		m_dlist->Get_w( side[is].dl_side_sel ) );
}

// highlight corner by drawing box around it
//
void CPolyLine::HighlightCorner( int ic )

{
	if( !m_dlist )
		ASSERT(0);

	m_dlist->HighLight( DL_HOLLOW_RECT, 
		m_dlist->Get_x( corner[ic].dl_corner_sel ),
		m_dlist->Get_y( corner[ic].dl_corner_sel ),
		m_dlist->Get_xf( corner[ic].dl_corner_sel ),
		m_dlist->Get_yf( corner[ic].dl_corner_sel ),
		m_dlist->Get_w( corner[ic].dl_corner_sel ) );
}

id CPolyLine::Id() 
{	
	id poly_id = m_parent_id;
	poly_id.SetU2( m_uid );
	poly_id.SetSubSubType( -1 );
	return poly_id;
}

int CPolyLine::UID() 
{	
	return m_uid; 
}

int CPolyLine::CornerIndexByUID( int uid )
{
	for( int ic=0; ic<NumCorners(); ic++ )
	{
		if( uid == CornerUID(ic) )
			return ic;
	}
	return -1;
}

int CPolyLine::SideIndexByUID( int uid )
{
	for( int ic=0; ic<NumSides(); ic++ )
	{
		if( uid == SideUID(ic) )
			return ic;
	}
	return -1;
}


int CPolyLine::X( int ic ) 
{	
	return corner[ic].x; 
}

int CPolyLine::Y( int ic ) 
{	
	return corner[ic].y; 
}

int CPolyLine::EndContour( int ic ) 
{	
	return corner[ic].end_contour; 
}

void CPolyLine::MakeVisible( BOOL visible ) 
{	
	if( m_dlist )
	{
		int ns = NumCorners()-1;
		if( Closed() )
			ns = NumCorners();
		for( int is=0; is<ns; is++ )
			m_dlist->Set_visible( side[is].dl_side, visible ); 
		for( int ih=0; ih<m_nhatch; ih++ )
			m_dlist->Set_visible( dl_hatch[ih], visible ); 
	}
} 

CRect CPolyLine::GetBounds()
{
	CRect r = GetCornerBounds();
	r.left -= m_w/2;
	r.right += m_w/2;
	r.bottom -= m_w/2;
	r.top += m_w/2;
	return r;
}

CRect CPolyLine::GetCornerBounds()
{
	CRect r;
	r.left = r.bottom = INT_MAX;
	r.right = r.top = INT_MIN;
	for( int i=0; i<NumCorners(); i++ )
	{
		r.left = min( r.left, corner[i].x );
		r.right = max( r.right, corner[i].x );
		r.bottom = min( r.bottom, corner[i].y );
		r.top = max( r.top, corner[i].y );
	}
	return r;
}

CRect CPolyLine::GetCornerBounds( int icont )
{
	CRect r;
	r.left = r.bottom = INT_MAX;
	r.right = r.top = INT_MIN;
	int istart = ContourStart( icont );
	int iend = ContourEnd( icont );
	for( int i=istart; i<=iend; i++ )
	{
		r.left = min( r.left, corner[i].x );
		r.right = max( r.right, corner[i].x );
		r.bottom = min( r.bottom, corner[i].y );
		r.top = max( r.top, corner[i].y );
	}
	return r;
}

int CPolyLine::NumCorners() 
{	
	return corner.GetSize();
}

int CPolyLine::NumSides() 
{	
	if( Closed() )
		return corner.GetSize();	
	else
		return corner.GetSize()-1;	
}

int CPolyLine::Layer() 
{	
	return m_layer;	
}

int CPolyLine::W() 
{	
	return m_w;	
}

int CPolyLine::SelBoxSize() 
{	
	return m_sel_box;	
}

int CPolyLine::NumContours()
{
	int ncont = 0;
	int ncors = NumCorners();
	if( !ncors )
		return 0;

	for( int ic=0; ic<ncors; ic++ )
		if( corner[ic].end_contour )
			ncont++;
	if( !corner[ncors-1].end_contour )
		ncont++;
	return ncont;
}

int CPolyLine::Contour( int ic )
{
	int ncont = 0;
	for( int i=0; i<ic; i++ )
	{
		if( corner[i].end_contour )
			ncont++;
	}
	return ncont;
}

int CPolyLine::ContourStart( int icont )
{
	if( icont == 0 )
		return 0;

	int ncont = 0;
	for( int i=0; i<NumCorners(); i++ )
	{
		if( corner[i].end_contour )
		{
			ncont++;
			if( ncont == icont )
				return i+1;
		}
	}
	ASSERT(0);
	return 0;
}

int CPolyLine::ContourEnd( int icont )
{
	if( icont < 0 )
		return 0;

	if( icont == NumContours()-1 )
		return NumCorners()-1;

	int ncont = 0;
	for( int i=0; i<NumCorners(); i++ )
	{
		if( corner[i].end_contour )
		{
			if( ncont == icont )
				return i;
			ncont++;
		}
	}
	ASSERT(0);
	return 0;
}

int CPolyLine::ContourSize( int icont )
{
	return ContourEnd(icont) - ContourStart(icont) + 1;
}


void CPolyLine::SetSideStyle( int is, int style, BOOL bDraw ) 
{	
	Undraw();
	CPoint p1, p2;
	int icont = Contour( is );
	int istart = ContourStart( icont );
	int iend = ContourEnd( icont );
	p1.x = corner[is].x;
	p1.y = corner[is].y;
	if( is == iend )
	{
		p2.x = corner[istart].x;
		p2.y = corner[istart].y;
	}
	else
	{
		p2.x = corner[is+1].x;
		p2.y = corner[is+1].y;
	}
	if( p1.x == p2.x || p1.y == p2.y )
		side[is].m_style = STRAIGHT;
	else
		side[is].m_style = style;	
	Draw();
}

int CPolyLine::SideUID( int is ) 
{	
	return side[is].m_uid;	
}

int CPolyLine::SideStyle( int is ) 
{	
	return side[is].m_style;	
}

void CPolyLine::SetUID( int uid )
{
	Undraw();
	m_uid = uid;
	if( m_dlist )
	{
		Draw();
	}
}

// Since a CPolyLine is often part of something else, 
// you can set the top-level of the id (actually T1, U1, T2 and I2)
// Then for the id of the CPolyLine, U2 will be it's own UID
// For the id's of display elements, T3, U3 and I3 are set for each side and corner
//
void CPolyLine::SetParentId( id * id )
{
	Undraw();
	m_parent_id = *id;
	m_parent_id.SetU2( m_uid );
	if( m_dlist )
	{
		Draw();
	}
}

int CPolyLine::Closed() 
{	
	int ncors = NumCorners();
	if( ncors == 0 )
		return 0;
	else
		return corner[ncors-1].end_contour; 
}

// draw hatch lines
//
void CPolyLine::Hatch()
{
	if( m_hatch == NO_HATCH )
	{
		m_nhatch = 0;
		return;
	}

	if( m_dlist && Closed() )
	{
		enum {
			MAXPTS = 100,
			MAXLINES = 1000
		};
		dl_hatch.SetSize( MAXLINES, MAXLINES );
		int xx[MAXPTS], yy[MAXPTS];

		// define range for hatch lines
		int min_x = corner[0].x;
		int max_x = corner[0].x;
		int min_y = corner[0].y;
		int max_y = corner[0].y;
		for( int ic=1; ic<NumCorners(); ic++ )
		{
			if( corner[ic].x < min_x )
				min_x = corner[ic].x;
			if( corner[ic].x > max_x )
				max_x = corner[ic].x;
			if( corner[ic].y < min_y )
				min_y = corner[ic].y;
			if( corner[ic].y > max_y )
				max_y = corner[ic].y;
		}
		int slope_flag = 1 - 2*(m_layer%2);	// 1 or -1
		double slope = 0.707106*slope_flag;
		int spacing;
		if( m_hatch == DIAGONAL_EDGE )
			spacing = 10*PCBU_PER_MIL;
		else
			spacing = 50*PCBU_PER_MIL;
		int max_a, min_a;
		if( slope_flag == 1 )
		{
			max_a = (int)(max_y - slope*min_x);
			min_a = (int)(min_y - slope*max_x);
		}
		else
		{
			max_a = (int)(max_y - slope*max_x);
			min_a = (int)(min_y - slope*min_x);
		}
		min_a = (min_a/spacing)*spacing;
		int offset;
		if( m_layer < (LAY_TOP_COPPER+2) )
			offset = 0;
		else if( m_layer < (LAY_TOP_COPPER+4) )
			offset = spacing/2;
		else if( m_layer < (LAY_TOP_COPPER+6) )
			offset = spacing/4;
		else if( m_layer < (LAY_TOP_COPPER+8) )
			offset = 3*spacing/4;
		else if( m_layer < (LAY_TOP_COPPER+10) )
			offset = 1*spacing/8;
		else if( m_layer < (LAY_TOP_COPPER+12) )
			offset = 3*spacing/8;
		else if( m_layer < (LAY_TOP_COPPER+14) )
			offset = 5*spacing/8;
		else if( m_layer < (LAY_TOP_COPPER+16) )
			offset = 7*spacing/8;
		else
			ASSERT(0);
		min_a += offset;

		// now calculate and draw hatch lines
		int nc = NumCorners();
		int nhatch = 0;
		// loop through hatch lines
		for( int a=min_a; a<max_a; a+=spacing )
		{
			// get intersection points for this hatch line
			int nloops = 0;
			int npts;
			// make this a loop in case my homebrew hatching algorithm screws up
			do
			{
				npts = 0;
				int i_start_contour = 0;
				for( int ic=0; ic<nc; ic++ )
				{
					double x, y, x2, y2;
					int ok;
					if( corner[ic].end_contour )
					{
						ok = FindLineSegmentIntersection( a, slope, 
								corner[ic].x, corner[ic].y,
								corner[i_start_contour].x, corner[i_start_contour].y, 
								side[ic].m_style,
								&x, &y, &x2, &y2 );
						i_start_contour = ic + 1;
					}
					else
					{
						ok = FindLineSegmentIntersection( a, slope, 
								corner[ic].x, corner[ic].y, 
								corner[ic+1].x, corner[ic+1].y,
								side[ic].m_style,
								&x, &y, &x2, &y2 );
					}
					if( ok )
					{
						xx[npts] = (int)x;
						yy[npts] = (int)y;
						npts++;
						ASSERT( npts<MAXPTS );	// overflow
					}
					if( ok == 2 )
					{
						xx[npts] = (int)x2;
						yy[npts] = (int)y2;
						npts++;
						ASSERT( npts<MAXPTS );	// overflow
					}
				}
				nloops++;
				a += PCBU_PER_MIL/100;
			} while( npts%2 != 0 && nloops < 3 );
			ASSERT( npts%2==0 );	// odd number of intersection points, error

			// sort points in order of descending x (if more than 2)
			if( npts>2 )
			{
				for( int istart=0; istart<(npts-1); istart++ )
				{
					int max_x = INT_MIN;
					int imax;
					for( int i=istart; i<npts; i++ )
					{
						if( xx[i] > max_x )
						{
							max_x = xx[i];
							imax = i;
						}
					}
					int temp = xx[istart];
					xx[istart] = xx[imax];
					xx[imax] = temp;
					temp = yy[istart];
					yy[istart] = yy[imax];
					yy[imax] = temp;
				}
			}

			// draw lines
			for( int ip=0; ip<npts; ip+=2 )
			{
				id hatch_id = m_parent_id;
				hatch_id.SetT3( ID_HATCH );
				hatch_id.SetI3( nhatch );
				double dx = xx[ip+1] - xx[ip];
				if( m_hatch == DIAGONAL_FULL || fabs(dx) < 40*NM_PER_MIL )
				{
					dl_element * dl = m_dlist->Add( hatch_id, 0, m_layer, DL_LINE, 1, 0, 0, 0,
						xx[ip], yy[ip], xx[ip+1], yy[ip+1], 0, 0 );
					dl_hatch.SetAtGrow(nhatch, dl);
					nhatch++;
				}
				else
				{
					double dy = yy[ip+1] - yy[ip];	
					double slope = dy/dx;
					if( dx > 0 )
						dx = 20*NM_PER_MIL;
					else
						dx = -20*NM_PER_MIL;
					double x1 = xx[ip] + dx;
					double x2 = xx[ip+1] - dx;
					double y1 = yy[ip] + dx*slope;
					double y2 = yy[ip+1] - dx*slope;
					dl_element * dl = m_dlist->Add( hatch_id, 0, m_layer, DL_LINE, 1, 0, 0, 0,
						xx[ip], yy[ip], x1, y1, 0, 0 );
					dl_hatch.SetAtGrow(nhatch, dl);
					dl = m_dlist->Add( hatch_id, 0, m_layer, DL_LINE, 1, 0, 0, 0, 
						xx[ip+1], yy[ip+1], x2, y2, 0, 0 );
					dl_hatch.SetAtGrow(nhatch+1, dl);
					nhatch += 2;
				}
			}
		} // end for 
		m_nhatch = nhatch;
		dl_hatch.SetSize( m_nhatch );
	}
}

// test to see if a point is inside polyline
//
BOOL CPolyLine::TestPointInside( int x, int y )
{
	enum { MAXPTS = 100 };
	if( !Closed() )
		ASSERT(0);

	// define line passing through (x,y), with slope = 2/3;
	// get intersection points
	double xx[MAXPTS], yy[MAXPTS];
	double slope = (double)2.0/3.0;
	double a = y - slope*x;
	int nloops = 0;
	int npts;
	// make this a loop so if my homebrew algorithm screws up, we try it again
	do
	{
		// now find all intersection points of line with polyline sides
		npts = 0;
		for( int icont=0; icont<NumContours(); icont++ )
		{
			int istart = ContourStart( icont );
			int iend = ContourEnd( icont );
			for( int ic=istart; ic<=iend; ic++ )
			{
				double x, y, x2, y2;
				int ok;
				if( ic == istart )
					ok = FindLineSegmentIntersection( a, slope, 
					corner[iend].x, corner[iend].y,
					corner[ic].x, corner[ic].y, 
					side[iend].m_style,
					&x, &y, &x2, &y2 );
				else
					ok = FindLineSegmentIntersection( a, slope, 
					corner[ic-1].x, corner[ic-1].y, 
					corner[ic].x, corner[ic].y,
					side[ic-1].m_style,
					&x, &y, &x2, &y2 );
				if( ok )
				{
					xx[npts] = (int)x;
					yy[npts] = (int)y;
					npts++;
					ASSERT( npts<MAXPTS );	// overflow
				}
				if( ok == 2 )
				{
					xx[npts] = (int)x2;
					yy[npts] = (int)y2;
					npts++;
					ASSERT( npts<MAXPTS );	// overflow
				}
			}
		}
		nloops++;
		a += PCBU_PER_MIL/100;
	} while( npts%2 != 0 && nloops < 3 );
	ASSERT( npts%2==0 );	// odd number of intersection points, error

	// count intersection points to right of (x,y), if odd (x,y) is inside polyline
	int ncount = 0;
	for( int ip=0; ip<npts; ip++ )
	{
		if( xx[ip] == x && yy[ip] == y )
			return FALSE;	// (x,y) is on a side, call it outside
		else if( xx[ip] > x )
			ncount++;
	}
	if( ncount%2 )
		return TRUE;
	else
		return FALSE;
}

// test to see if a point is inside polyline contour
//
BOOL CPolyLine::TestPointInsideContour( int icont, int x, int y )
{
	if( icont >= NumContours() )
		return FALSE;

	enum { MAXPTS = 100 };
	if( !Closed() )
		ASSERT(0);

	// define line passing through (x,y), with slope = 2/3;
	// get intersection points
	double xx[MAXPTS], yy[MAXPTS];
	double slope = (double)2.0/3.0;
	double a = y - slope*x;
	int nloops = 0;
	int npts;
	// make this a loop so if my homebrew algorithm screws up, we try it again
	do
	{
		// now find all intersection points of line with polyline sides
		npts = 0;
		int istart = ContourStart( icont );
		int iend = ContourEnd( icont );
		for( int ic=istart; ic<=iend; ic++ )
		{
			double x, y, x2, y2;
			int ok;
			if( ic == istart )
				ok = FindLineSegmentIntersection( a, slope, 
				corner[iend].x, corner[iend].y,
				corner[istart].x, corner[istart].y, 
				side[NumCorners()-1].m_style,
				&x, &y, &x2, &y2 );
			else
				ok = FindLineSegmentIntersection( a, slope, 
				corner[ic-1].x, corner[ic-1].y, 
				corner[ic].x, corner[ic].y,
				side[ic-1].m_style,
				&x, &y, &x2, &y2 );
			if( ok )
			{
				xx[npts] = (int)x;
				yy[npts] = (int)y;
				npts++;
				ASSERT( npts<MAXPTS );	// overflow
			}
			if( ok == 2 )
			{
				xx[npts] = (int)x2;
				yy[npts] = (int)y2;
				npts++;
				ASSERT( npts<MAXPTS );	// overflow
			}
		}
		nloops++;
		a += PCBU_PER_MIL/100;
	} while( npts%2 != 0 && nloops < 3 );
	ASSERT( npts%2==0 );	// odd number of intersection points, error

	// count intersection points to right of (x,y), if odd (x,y) is inside polyline
	int ncount = 0;
	for( int ip=0; ip<npts; ip++ )
	{
		if( xx[ip] == x && yy[ip] == y )
			return FALSE;	// (x,y) is on a side, call it outside
		else if( xx[ip] > x )
			ncount++;
	}
	if( ncount%2 )
		return TRUE;
	else
		return FALSE;
}

// Test for intersection of sides
//
int CPolyLine::TestIntersection( CPolyLine * poly )
{
	if( !Closed() )
		ASSERT(0);
	if( !poly->Closed() )
		ASSERT(0);
	for( int ic=0; ic<NumContours(); ic++ )
	{
		int istart = ContourStart(ic);
		int iend = ContourEnd(ic);
		for( int is=istart; is<=iend; is++ )
		{
			int xi = X(is);
			int yi = Y(is);
			int xf, yf;
			if( is < ContourEnd(ic) )
			{
				xf = X(is+1);
				yf = Y(is+1);
			}
			else
			{
				xf = X(istart);
				yf = Y(istart);
			}
			int style = SideStyle(is);
			for( int ic2=0; ic2<poly->NumContours(); ic2++ )
			{
				int istart2 = poly->ContourStart(ic2);
				int iend2 = poly->ContourEnd(ic2);
				for( int is2=istart2; is2<=iend2; is2++ )
				{
					int xi2 = poly->X(is2);
					int yi2 = poly->Y(is2);
					int xf2, yf2;
					if( is2 < poly->ContourEnd(ic2) )
					{
						xf2 = poly->X(is2+1);
						yf2 = poly->Y(is2+1);
					}
					else
					{
						xf2 = poly->X(istart2);
						yf2 = poly->Y(istart2);
					}
					int style2 = poly->SideStyle(is2);
					// test for intersection between side and side2
				}
			}
		}
	}
	return 0;
}

// set selection box size 
//
void CPolyLine::SetSelBoxSize( int sel_box )
{
//	Undraw();
	m_sel_box = sel_box;
//	Draw();
}

// set pointer to display list, and draw into display list
//
void CPolyLine::SetDisplayList( CDisplayList * dl )
{
	if( m_dlist )
		Undraw();
	m_dlist = dl;
	if( m_dlist )
		Draw();
}

// copy data from another poly, but don't draw it
// 
// don't copy the UID or the display list
// generate new uids for corners and sides
//
void CPolyLine::Copy( CPolyLine * src )
{
	Undraw();
//	m_dlist = src->m_dlist;
	m_parent_id = src->m_parent_id;
	m_parent_id.SetU2( m_uid );
	m_ptr = src->m_ptr;
	m_layer = src->m_layer;
	m_w = src->m_w;
	m_sel_box = src->m_sel_box;
	m_hatch = src->m_hatch;
	m_nhatch = src->m_nhatch;
	// copy corners
	corner.SetSize( src->NumCorners() );
	for( int i=0; i<src->NumCorners(); i++ )
	{
		corner[i] = src->corner[i];
	}
	// copy side styles
	int nsides = src->NumSides();
	side.SetSize(nsides);
	for( int i=0; i<nsides; i++ )
	{
		side[i] = src->side[i];
	}
	// don't copy the Gpc_poly, just clear the old one
	FreeGpcPoly();
}

void CPolyLine::MoveOrigin( int x_off, int y_off )
{
	Undraw();
	for( int ic=0; ic<NumCorners(); ic++ )
	{
		SetX( ic, X(ic) + x_off );
		SetY( ic, Y(ic) + y_off );
	}
	Draw();
}


// Set various parameters:
//   the calling function should Undraw() before calling them,
//   and Draw() after
//
void CPolyLine::SetCornerUID( int ic, int uid ){ corner[ic].m_uid = uid; };
void CPolyLine::SetSideUID( int is, int uid ){ side[is].m_uid = uid; };
void CPolyLine::SetClosed( BOOL bClosed ){ corner[NumCorners()-1].end_contour = bClosed; };
void CPolyLine::SetX( int ic, int x ) { corner[ic].x = x; }
void CPolyLine::SetY( int ic, int y ) { corner[ic].y = y; }
void CPolyLine::SetEndContour( int ic, BOOL end_contour ) { corner[ic].end_contour = end_contour; }
void CPolyLine::SetLayer( int layer ) { m_layer = layer; }
void CPolyLine::SetW( int w ) { m_w = w; }

#if 0
// Create CPolyLine for a pad
//
CPolyLine * CPolyLine::MakePolylineForPad( int type, int x, int y, int w, int l, int r, int angle )
{
	CPolyLine * poly = new CPolyLine;
	int dx = l/2;
	int dy = w/2;
	if( angle%180 == 90 )
	{
		dx = w/2;
		dy = l/2;
	}
	if( type == PAD_ROUND )
	{
		poly->Start( 0, 0, 0, x-dx, y, 0, NULL, NULL );
		poly->AppendCorner( x, y+dy, ARC_CW );
		poly->AppendCorner( x+dx, y, ARC_CW );
		poly->AppendCorner( x, y-dy, ARC_CW );
		poly->Close( ARC_CW );
	}
	return poly;
}

// Add cutout for a pad
// Convert arcs to multiple straight lines
// Do NOT draw or undraw
//
void CPolyLine::AddContourForPadClearance( int type, int x, int y, int w, 
						int l, int r, int angle, int fill_clearance,
						int hole_w, int hole_clearance, BOOL bThermal, int spoke_w )
{
	int dx = l/2;
	int dy = w/2;
	if( angle%180 == 90 )
	{
		dx = w/2;
		dy = l/2;
	}
	int x_clearance = max( fill_clearance, hole_clearance+hole_w/2-dx);		
	int y_clearance = max( fill_clearance, hole_clearance+hole_w/2-dy);
	dx += x_clearance;
	dy += y_clearance;
	if( !bThermal )
	{
		// normal clearance
		if( type == PAD_ROUND || (type == PAD_NONE && hole_w > 0) )
		{
			AppendCorner( x-dx, y, ARC_CW );
			AppendCorner( x, y+dy, ARC_CW );
			AppendCorner( x+dx, y, ARC_CW );
			AppendCorner( x, y-dy, ARC_CW );
			Close( ARC_CW ); 
		}
		else if( type == PAD_SQUARE || type == PAD_RECT 
			|| type == PAD_RRECT || type == PAD_OVAL )
		{
			AppendCorner( x-dx, y-dy, STRAIGHT, 0 );
			AppendCorner( x+dx, y-dy, STRAIGHT, 0 );
			AppendCorner( x+dx, y+dy, STRAIGHT, 0 );
			AppendCorner( x-dx, y+dy, STRAIGHT, 0 );
			Close( STRAIGHT ); 
		}
	}
	else
	{
		// thermal relief
		if( type == PAD_ROUND || (type == PAD_NONE && hole_w > 0) )
		{
			// draw 4 "wedges"
			double r = max(w/2 + fill_clearance, hole_w/2 + hole_clearance);
			double start_angle = asin( spoke_w/(2.0*r) );
			double th1, th2, corner_x, corner_y;
			for( int i=0; i<4; i++ )
			{
				if( i == 0 )
				{
					corner_x = spoke_w/2;
					corner_y = spoke_w/2;
					th1 = start_angle;
					th2 = pi/2.0 - start_angle;
				}
				else if( i == 1 )
				{
					corner_x = -spoke_w/2;
					corner_y = spoke_w/2;
					th1 = pi/2.0 + start_angle;
					th2 = pi - start_angle;
				}
				else if( i == 2 )
				{
					corner_x = -spoke_w/2;
					corner_y = -spoke_w/2;
					th1 = -pi + start_angle;
					th2 = -pi/2.0 - start_angle;
				}
				else if( i == 3 )
				{
					corner_x = spoke_w/2;
					corner_y = -spoke_w/2;
					th1 = -pi/2.0 + start_angle;
					th2 = -start_angle;
				}
				AppendCorner( x+corner_x, y+corner_y, STRAIGHT, 0 );
				AppendCorner( x+r*cos(th1), y+r*sin(th1),  STRAIGHT, 0 );
				AppendCorner( x+r*cos(th2), y+r*sin(th2),  ARC_CCW, 0 );
				Close( STRAIGHT );
			}
		}
		else if( type == PAD_SQUARE || type == PAD_RECT 
			|| type == PAD_RRECT || type == PAD_OVAL )
		{
			// draw 4 rectangles
			int xL = x - dx;
			int xR = x - spoke_w/2;
			int yB = y - dy;
			int yT = y - spoke_w/2;
			AppendCorner( xL, yB, STRAIGHT, 0 );
			AppendCorner( xR, yB, STRAIGHT, 0 );
			AppendCorner( xR, yT, STRAIGHT, 0 );
			AppendCorner( xL, yT, STRAIGHT, 0 );
			Close( STRAIGHT ); 
			xL = x + spoke_w/2;
			xR = x + dx;
			AppendCorner( xL, yB, STRAIGHT, 0 );
			AppendCorner( xR, yB, STRAIGHT, 0 );
			AppendCorner( xR, yT, STRAIGHT, 0 );
			AppendCorner( xL, yT, STRAIGHT, 0 );
			Close( STRAIGHT ); 
			xL = x - dx;
			xR = x - spoke_w/2;
			yB = y + spoke_w/2;
			yT = y + dy;
			AppendCorner( xL, yB, STRAIGHT, 0 );
			AppendCorner( xR, yB, STRAIGHT, 0 );
			AppendCorner( xR, yT, STRAIGHT, 0 );
			AppendCorner( xL, yT, STRAIGHT, 0 );
			Close( STRAIGHT ); 
			xL = x + spoke_w/2;
			xR = x + dx;
			AppendCorner( xL, yB, STRAIGHT, 0 );
			AppendCorner( xR, yB, STRAIGHT, 0 );
			AppendCorner( xR, yT, STRAIGHT, 0 );
			AppendCorner( xL, yT, STRAIGHT, 0 );
			Close( STRAIGHT ); 
		}
	}
	return;
}
#endif

void CPolyLine::AppendArc( int xi, int yi, int xf, int yf, int xc, int yc, int num )
{
	// get radius
	double r = sqrt( (double)(xi-xc)*(xi-xc) + (double)(yi-yc)*(yi-yc) );
	// get angles of start and finish
	double th_i = atan2( (double)yi-yc, (double)xi-xc );
	double th_f = atan2( (double)yf-yc, (double)xf-xc );
	double th_d = (th_f - th_i)/(num-1);
	double theta = th_i;
	// generate arc
	for( int ic=0; ic<num; ic++ )
	{
		int x = xc + r*cos(theta);
		int y = yc + r*sin(theta);
		AppendCorner( x, y, STRAIGHT );
		theta += th_d;
	}
	Close( STRAIGHT );
}


void CPolyLine::ClipGpcPolygon( gpc_op op, CPolyLine * clip_poly )
{
	gpc_polygon * result = new gpc_polygon;
	gpc_polygon_clip( op, m_gpc_poly, clip_poly->GetGpcPoly(), result );
	gpc_free_polygon( m_gpc_poly );
	delete m_gpc_poly;
	m_gpc_poly = result;
}

