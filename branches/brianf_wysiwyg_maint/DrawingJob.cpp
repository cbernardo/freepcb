#include "stdafx.h"
#include "memdc.h"
#include "DrawingElement.h"

CDL_job::~CDL_job()
{
	// Remove all elements
	CDLinkList *pElement;
	for (;;)
	{
	    pElement = m_LIST_DLE.next;
	    if( pElement == &m_LIST_DLE ) break;

        m_dlist->Remove(static_cast<dl_element*>(pElement));
    }
}


void CDL_job::Add(dl_element *pDLE)
{
	m_LIST_DLE.insert_after(pDLE);
}


// test x,y for a hit on an item in the drawing job
// creates arrays with layer and id of each hit item
// assigns priority based on layer and id
// then returns pointer to item with highest priority
// If exclude_id != NULL, excludes item with
// id == exclude_id and ptr == exclude_ptr
// If include_id != NULL, only include items that match include_id[]
// where n_include_ids is size of array, and
// where 0's in include_id[] fields are treated as wildcards
//
int CDL_job_traces::TestForHit( CPoint const &point, HitInfo hitInfo[], int max_hits ) const
{
	int nhits = 0;

	if( max_hits > 0 )
	{
		// traverse the list, looking for selection shapes
		CDLinkList *pElement;
		for (pElement = m_LIST_DLE.next; pElement != &m_LIST_DLE; pElement = pElement->next )
		{
			dl_element * el = static_cast<dl_element*>(pElement);

			if( el->isHit(point) )
			{
				// OK, hit
				hitInfo->layer = el->layer;
				hitInfo->ID    = el->id;
				hitInfo->ptr   = el->ptr;

				hitInfo++;
				nhits++;

				if( nhits >= max_hits ) break;
			}
		}
	}
	return nhits;
}



void CDL_job_traces::Draw(CDrawInfo &di) const
{
	CPen * old_pen;
	CBrush * old_brush;

	// Create drawing objects
	{
		di.erase_pen.CreatePen( PS_SOLID, 1, di.layer_color[0] );
		di.erase_brush.CreateSolidBrush( di.layer_color[0] );
		di.line_pen.CreatePen( PS_SOLID, 1, di.layer_color[1] );
		di.fill_brush.CreateSolidBrush( di.layer_color[1] );

		old_pen   = di.DC->SelectObject( &di.line_pen );
		old_brush = di.DC->SelectObject( &di.fill_brush );
	}

	CDLinkList *pElement;
	for( pElement = m_LIST_DLE.next; pElement != &m_LIST_DLE; pElement = pElement->next)
	{
		static_cast<dl_element*>(pElement)->Draw(di);
	}

	// Restore original drawing objects
	{
		di.DC->SelectObject( old_pen );
		di.DC->SelectObject( old_brush );

		di.erase_pen.DeleteObject();
		di.erase_brush.DeleteObject();
		di.line_pen.DeleteObject();
		di.fill_brush.DeleteObject();
	}
}


void CDL_job_traces::UpdateLineWidths( int width, int layer )
{
	CDLinkList *pElement;
	for( pElement = m_LIST_DLE.next; pElement != &m_LIST_DLE; pElement = pElement->next)
	{
		dl_element * el = static_cast<dl_element*>(pElement);

		if( el->gtype == DL_LINE )
		{
			if( el->layer == layer )
			{
				el->w = width;
			}
		}
	}
}


int CDL_job_copper_area::TestForHit( CPoint const &_point, HitInfo hitInfo[], int max_hits ) const
{
	int nHits = 0;

	CPoint point(_point);

	m_dlist->Scale_wu_to_pcbu(point);

	if( my_poly->TestPointInside( point.x, point.y ) )
	{
		nHits++;

		cnet *net = reinterpret_cast<cnet*>( my_poly->GetPtr() );

		hitInfo->layer = my_poly->GetLayer();
		hitInfo->ptr   = net;

		hitInfo->ID     = my_poly->GetId();
		hitInfo->ID.sst = ID_ENTIRE_AREA;
	}

	return nHits;
}


void CDL_job_copper_area::Draw(CDrawInfo &di) const
{
	CDLinkList *pElement;

	CRect area_bounds;
	CSize area_size;
	{
		area_bounds = my_poly->GetBounds();
		m_dlist->Scale_pcbu_to_wu(area_bounds);
		area_bounds.NormalizeRect();

		CRect screen_bounds(m_dlist->m_org_x, m_dlist->m_org_y, m_dlist->m_max_x, m_dlist->m_max_y);

        // Clip the area based on the screen.  Otherwise, at high zoom levels, the
        // area rect gets (unnecessarily) huge and taxes the memory and performance.
		area_bounds.IntersectRect(area_bounds, screen_bounds);

		// Adjust so that the area_bounds map to the actual pixels in the area bitmap
		{
			di.DC->LPtoDP(area_bounds);
			// Allow for 1 extra pixel on the outer edge
			area_bounds.top++;
			area_bounds.right++;
			di.DC->DPtoLP(area_bounds);
		}

#if 0 // enable to show area bounds
		{
			CPen *op, pen( PS_SOLID, m_dlist->m_scale, RGB(244, 122, 12) );
			op = di.DC_Master->SelectObject( &pen );
			di.DC_Master->MoveTo( area_bounds.left,  area_bounds.top );
			di.DC_Master->LineTo( area_bounds.right, area_bounds.top );
			di.DC_Master->LineTo( area_bounds.right, area_bounds.bottom );
			di.DC_Master->LineTo( area_bounds.left,  area_bounds.bottom );
			di.DC_Master->LineTo( area_bounds.left,  area_bounds.top );
			di.DC_Master->SelectObject( op );
		}
#endif
	}
	area_size = CSize((area_bounds.right - area_bounds.left + 1), (area_bounds.bottom - area_bounds.top + 1));

	CDC *pDC_Save = di.DC;
	CRect bitmap_area(0,0, area_size.cx, area_size.cy);
	CMemDC dcMemory(pDC_Save, &bitmap_area);
	dcMemory.compatible();
	di.DC = &dcMemory;
	{
		CPen * old_pen;
		CBrush * old_brush;

		// Create drawing objects
		{
			di.erase_pen.CreatePen( PS_SOLID, 1, di.layer_color[0] );
			di.erase_brush.CreateSolidBrush( di.layer_color[0] );
			di.line_pen.CreatePen( PS_SOLID, 1, di.layer_color[1] );
			di.fill_brush.CreateSolidBrush( di.layer_color[1] );

			old_pen   = di.DC->SelectObject( &di.line_pen );
			old_brush = di.DC->SelectObject( &di.fill_brush );
		}

		CPoint org = di.DC->SetWindowOrg(area_bounds.left, area_bounds.bottom);

		// Draw the area
		for( pElement = m_LIST_DLE.next; pElement != &m_LIST_DLE; pElement = pElement->next)
		{
			static_cast<dl_element*>(pElement)->Draw(di);
		}
		di.DC->SelectObject( old_pen );
		di.DC->SelectObject( old_brush );

		// Scratch the clearances
		void *area_net = my_poly->GetPtr();

		// Scratch the trace/pin clearances
		ScratchClearances(di, my_poly->GetLayer(), area_bounds, area_net);

		// Scratch the hole clearances
		ScratchClearances(di, LAY_PAD_THRU, area_bounds, area_net);

		// Restore original drawing objects
		{
			di.DC->SelectObject( old_pen );
			di.DC->SelectObject( old_brush );

			di.erase_pen.DeleteObject();
			di.erase_brush.DeleteObject();
			di.line_pen.DeleteObject();
			di.fill_brush.DeleteObject();
		}
	}

	pDC_Save->BitBlt(area_bounds.left, area_bounds.top,
	                 area_size.cx, area_size.cy,
	                 di.DC,
	                 area_bounds.left, area_bounds.top,
	                 SRCPAINT);

	di.DC->DeleteDC();
	di.DC = pDC_Save;
}


void CDL_job_copper_area::ScratchClearances(CDrawInfo &di, int layer, CRect const &area_bounds, void * area_net) const
{
	CDL_job_traces *pJT = m_dlist->GetJob_traces(layer);

	CRect el_bounds;
	CRect test_intersect;
	CDLinkList *pElement;
	for( pElement = pJT->m_LIST_DLE.next; pElement != &pJT->m_LIST_DLE; pElement = pElement->next)
	{
		dl_element * el = static_cast<dl_element*>(pElement);

		if( !el->getBoundingRect(el_bounds) ) continue;

		// Normalized:
		//   top < bottom
		//   left < right
		el_bounds.NormalizeRect();

		{ // Check for intersection
			if (el_bounds.bottom < area_bounds.top)    continue;
			if (el_bounds.top    > area_bounds.bottom) continue;

			if (el_bounds.right  < area_bounds.left)   continue;
			if (el_bounds.left   > area_bounds.right)  continue;
		}

		{
#if 0 // enable to show element bounds
			{
				CPen *op, pen( PS_SOLID, m_dlist->m_scale, RGB(144, 122, 12) );
				op = di.DC_Master->SelectObject( &pen );
				di.DC_Master->MoveTo( el_bounds.TopLeft() );
				di.DC_Master->LineTo( el_bounds.right, el_bounds.top );
				di.DC_Master->LineTo( el_bounds.right, el_bounds.bottom );
				di.DC_Master->LineTo( el_bounds.left,  el_bounds.bottom );
				di.DC_Master->LineTo( el_bounds.left,  el_bounds.top );
				di.DC_Master->SelectObject( op );
			}
#endif
			void *net;

			switch (el->id.type)
			{
			case ID_TEXT:
			{
				el->DrawClearance(di);
			}
			break;

			case ID_NET:
			{
				net = el->ptr;

				if( area_net == net )
                {
					cnet *net = (cnet*)area_net;
					carea *area = &net->area[my_poly->GetId().i];
					ASSERT(area->poly == my_poly);

					CArray<int> &a_vtx = area->vtx;
					CArray<int> &a_con = area->vcon;
					for (int i = 0; i < a_vtx.GetSize(); i++)
					{
						if (( a_vtx[i] == el->id.ii ) && ( a_con[i] == el->id.i ) )
						{
	                        el->DrawThermalRelief(di);
							break;
						}
					}
                }
                else
                {
					el->DrawClearance(di);
				}
			}
			break;

			case ID_PART:
			{
				cpart *part = (cpart *)el->ptr;
				part_pin *pPartPin = &part->pin[ el->id.i ];

                net = pPartPin->net;
				if( area_net == net )
	            {
					cnet *net = (cnet*)area_net;
					carea *area = &net->area[ my_poly->GetId().i ];
					ASSERT(area->poly == my_poly);

					// Find the pin in the net
					CArray<int> &a_pin = area->pin;
					for (int i = 0; i < a_pin.GetSize(); i++)
					{
						cpin *pin = &net->pin[ a_pin[i] ]; 

						if ( part == pin->part )
						{
							if( part->shape )
							{
								int pin_index = part->shape->GetPinIndexByName( pin->pin_name );
								if( ( pin_index != -1 ) && ( pin_index == el->id.i ) )
								{
									el->DrawThermalRelief(di);
									break;
								}
							}
						}
					}
                }
                else
                {
					el->DrawClearance(di);
				}
			}
			break;

			default:
				break;
			}
		}
	}
}
