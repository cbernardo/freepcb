#include "stdafx.h"

#include "inheritable_item.h"

int CInheritableInfo::Item::operator > (Item const &comp) const
{
	if (m_status != E_USE_VAL)
	{
		if (comp.m_status != E_USE_VAL) return m_status > comp.m_status;
		else                            return 0;
	}
	else
	{
		if (comp.m_status != E_USE_VAL) return 1;
		else                            return m_val > comp.m_val;
	}
}

int CInheritableInfo::Item::operator < (Item const &comp) const
{
	if (m_status != E_USE_VAL)
	{
		if (comp.m_status != E_USE_VAL) return m_status < comp.m_status;
		else                            return 1;
	}
	else
	{
		if (comp.m_status != E_USE_VAL) return 0;
		else                            return m_val < comp.m_val;
	}
}


// Set item from int:
//   negative values map to EStatus
//   zero & positive values count as values
void CInheritableInfo::Item::Set_item_from_int(int val_status)
{
	if (val_status < 0)
	{
		if (val_status != E_UNDEF)
		{
			// Defined data - assign
			m_status = val_status;
			m_val    = 0;
		}
		else
		{
			// Don't assign to <unassigned>
		}
	}
	else
	{
		m_status = E_USE_VAL;
		m_val    = val_status;
	}
}


int CInheritableInfo::GetExtendedItem(Item &item, Item const &src) const
{
	item.m_status = E_UNDEF;
	item.m_val = 0;

	return 1;
}


void CInheritableInfo::GetItem(Item &item) const
{
	int offset = (int)(reinterpret_cast<char const*>(&item) - reinterpret_cast<char const*>(this));

	CInheritableInfo const *ci = this;
	Item const *pSrcItem = ci->GetItem(offset);

	item.m_status = pSrcItem->m_status;

	for (;;)
	{
		switch (pSrcItem->m_status)
		{
		case E_USE_VAL:
			item.m_val = pSrcItem->m_val;
			return;

		default:
			if( GetExtendedItem(item, *pSrcItem) )
			{
				return;
			}
			// Fallthru to E_USE_PARENT

		case E_USE_PARENT:
		{
			CInheritableInfo const *next_ci = ci->m_pParent;
			if (next_ci != ci)
			{
				// Got a valid parent - loop to handle parent
				ci = next_ci;

				pSrcItem = ci->GetItem(offset);

				break;
			}

			// Invalid parent
			item.m_status = E_UNDEF;
		}
		// Fallthru to E_UNDEF

		case E_UNDEF:
			item.m_val = 0;
			return;
		}
	}
}
