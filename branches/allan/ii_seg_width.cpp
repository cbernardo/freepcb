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
}

void CConnectionWidthInfo::Update_via_hole()
{
	UpdateItem(E_II_VIA_HOLE, m_via_hole);
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
