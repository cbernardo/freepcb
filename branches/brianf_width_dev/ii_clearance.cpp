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

	return CInheritableInfo::GetItem(item_id);
}


void CClearanceInfo::GetItemExt(Item &item, Item const &src) const
{
    // Handles E_AUTO_CALC
}
