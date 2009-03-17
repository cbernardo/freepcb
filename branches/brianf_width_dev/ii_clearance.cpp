#include "stdafx.h"

#include "FreePcb.h"
#include "ii_clearance.h"

CClearanceInfo &CClearanceInfo::operator = (CInheritableInfo const &from)
{
	CInheritableInfo::operator = (from);

	Item item;

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
