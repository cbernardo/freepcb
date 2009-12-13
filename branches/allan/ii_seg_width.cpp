#include "stdafx.h"

#include "FreePcb.h"
#include "ii_seg_width.h"


CSegWidthInfo &CSegWidthInfo::operator = (CInheritableInfo const &from)  
{
	CInheritableInfo::operator = (from);

	// Copy Data (only if present in 'from')
	m_seg_width    = from.GetItem(E_II_TRACE_WIDTH);
	m_ca_clearance = from.GetItem(E_II_CA_CLEARANCE);

	return *this;
}


void CSegWidthInfo::Update_seg_width()
{
	UpdateItem(E_II_TRACE_WIDTH, m_seg_width);
}

void CSegWidthInfo::Update_ca_clearance()
{
	UpdateItem(E_II_CA_CLEARANCE, m_ca_clearance);
}


CInheritableInfo::Item const &CSegWidthInfo::GetItem(int item_id) const
{
	switch( item_id )
	{
		case E_II_TRACE_WIDTH:  return m_seg_width;
		case E_II_CA_CLEARANCE: return m_ca_clearance;

		default:
			break;
	}

	return CInheritableInfo::GetItem(item_id);
}

// ----------------------------------------------------------------------------------

CConnectionWidthInfo &CConnectionWidthInfo::operator = (CInheritableInfo const &from)
{
	CSegWidthInfo::operator = (from);

	// Copy Data (only if present in 'from')
	m_via_width = from.GetItem(E_II_VIA_WIDTH);
	m_via_hole  = from.GetItem(E_II_VIA_HOLE);

	return *this;
}


void CConnectionWidthInfo::Update_via_width()
{
	UpdateItem(E_II_VIA_WIDTH, m_via_width);

	ApplyDefWidths(&m_via_width, NULL);
}

void CConnectionWidthInfo::Update_via_hole()
{
	UpdateItem(E_II_VIA_HOLE, m_via_hole);

	ApplyDefWidths(NULL, &m_via_hole);
}


void CConnectionWidthInfo::Update()
{
	CSegWidthInfo::Update();

	// Must update via AFTER segment width
	UpdateItem(E_II_VIA_WIDTH, m_via_width);
	UpdateItem(E_II_VIA_HOLE,  m_via_hole);

	ApplyDefWidths( &m_via_width, &m_via_hole );
}


void CConnectionWidthInfo::ApplyDefWidths(Item *p_via_width, Item *p_via_hole)
{
	// Only apply updates if the items have a status of E_USE_DEF_FROM_WIDTH
	if( p_via_width != NULL )
	{
		if( p_via_width->m_status != E_USE_DEF_FROM_WIDTH )
		{
			p_via_width = NULL;
		}
	}
	if( p_via_hole != NULL )
	{
		if( p_via_hole->m_status != E_USE_DEF_FROM_WIDTH )
		{
			p_via_hole = NULL;
		}
	}

	if( ( p_via_width != NULL ) || ( p_via_hole != NULL ) )
	{
		int new_w = m_seg_width.m_val;

		int new_v_w = 0;
		int new_v_h_w = 0;

		if( new_w >= 0 )
		{
			if( new_w == 0 )
			{
				new_v_w = 0;
				new_v_h_w = 0;
			}
			else
			{
				int n = theApp.m_Doc->m_w.GetSize();
				int i;
				for( i=0; i < n-1; i++ )
				{
					if( new_w <= theApp.m_Doc->m_w[i] )
					{
						break;
					}
				}
				new_v_w   = theApp.m_Doc->m_v_w[i];
				new_v_h_w = theApp.m_Doc->m_v_h_w[i];
			}
		}

		if( p_via_width != NULL )
		{
			p_via_width->m_val = new_v_w;
		}
		if( p_via_hole != NULL )
		{
			p_via_hole->m_val = new_v_h_w;
		}
	}
}


CInheritableInfo::Item const &CConnectionWidthInfo::GetItem(int item_id) const
{
	switch( item_id )
	{
		case E_II_VIA_WIDTH:   return m_via_width;
		case E_II_VIA_HOLE:    return m_via_hole;

		default:
			break;
	}

	return CSegWidthInfo::GetItem(item_id);
}

void CConnectionWidthInfo::GetItemExt(Item &item, Item const &src) const
{
	switch( src.m_status )
	{
	case E_USE_DEF_FROM_WIDTH:
		item.m_val = src.m_val;
		return;

	default:
		break;
	}

	CSegWidthInfo::GetItemExt( item, src );
}
