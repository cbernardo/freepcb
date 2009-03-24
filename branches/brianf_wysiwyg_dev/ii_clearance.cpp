#include "stdafx.h"

#include "FreePcb.h"
#include "ii_clearance.h"

CClearanceInfo &CClearanceInfo::operator = (CInheritableInfo const &from)
{
	CInheritableInfo::operator = (from);

	// Copy Data (only if present in 'from')
	m_ca_clearance = from.GetItem(E_II_CA_CLEARANCE);

	return *this;
}


void CClearanceInfo::Update_ca_clearance()
{
	UpdateItem(E_II_CA_CLEARANCE, m_ca_clearance);
}


CInheritableInfo::Item const &CClearanceInfo::GetItem(int item_id) const
{
	switch( item_id )
	{
		case E_II_CA_CLEARANCE: return m_ca_clearance;

		default:
			break;
	}

	return CII_FreePcb::GetItem(item_id);
}


void CClearanceInfo::GetItemExt(Item &item, Item const &src) const
{
	switch( src.m_status )
	{
	case E_AUTO_CALC:
		item.m_val = src.m_val;
		return;

	default:
		break;
	}

	CII_FreePcb::GetItemExt( item, src );
}


void CViaWidthInfo::CViaItem::assign_from(Item const &from)
{
	Item::assign_from(from);

	// Do not allow E_USE_DEF_FROM_WIDTH to get into a 
	// CViaItem as no trace width info is present in 
	// the vertex to resolve the size.
	if( m_status == E_USE_DEF_FROM_WIDTH ) m_status = E_USE_VAL;
}

CViaWidthInfo &CViaWidthInfo::operator = (CInheritableInfo const &from)
{
	CClearanceInfo::operator = (from);

	// Copy Data (only if present in 'from')
	m_via_width = from.GetItem(E_II_VIA_WIDTH);
	m_via_hole  = from.GetItem(E_II_VIA_HOLE);

	return *this;
}

void CViaWidthInfo::Update_via_width()
{
	UpdateItem(E_II_VIA_WIDTH, m_via_width);
}

void CViaWidthInfo::Update_via_hole()
{
	UpdateItem(E_II_VIA_HOLE, m_via_hole);
}

CInheritableInfo::Item const &CViaWidthInfo::GetItem(int item_id) const
{
	switch( item_id )
	{
		case E_II_VIA_WIDTH:   return m_via_width;
		case E_II_VIA_HOLE:    return m_via_hole;

		default:
			break;
	}

	return CClearanceInfo::GetItem(item_id);
}
