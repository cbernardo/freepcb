#include "stdafx.h"

#include "FreePcb.h"
#include "ii_seg_width.h"

CSegWidthInfo &CSegWidthInfo::operator = (CInheritableInfo const &from)
{
	CInheritableInfo::operator = (from);

	Item item;

	// Copy Data only if present in 'from'
	from.GetItem(E_II_TRACE_WIDTH, item);
	m_seg_width = item;

	from.GetItem(E_II_VIA_WIDTH, item);
	m_via_width = item;

	from.GetItem(E_II_VIA_HOLE, item);
	m_via_hole  = item;

	return *this;
}


void CSegWidthInfo::Update_seg_width()
{
	GetItem(E_II_TRACE_WIDTH, m_seg_width);
}

void CSegWidthInfo::Update_via_width()
{
	GetItem(E_II_VIA_WIDTH, m_via_width);
}

void CSegWidthInfo::Update_via_hole()
{
	GetItem(E_II_VIA_HOLE, m_via_hole);
}


CInheritableInfo::Item const &CSegWidthInfo::_GetItem(int item_id) const
{
	switch( item_id )
	{
		case E_II_TRACE_WIDTH: return m_seg_width;
		case E_II_VIA_WIDTH:   return m_via_width;
		case E_II_VIA_HOLE:    return m_via_hole;

		default:
			break;
	}

	return CInheritableInfo::_GetItem(item_id);
}
