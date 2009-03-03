#ifndef _CLEARANCE_H /* [ */
#define _CLEARANCE_H

#include "inheritable_item.h"

// Information for clearances
class CClearanceInfo : public CInheritableInfo
{
public:
	enum EStatus
	{
        // Remember, these are negative so normal enum
        // counting order won't work here.
		E_AUTO_CALC = CInheritableInfo::E__STATUS_USER
	};

public:
	CClearanceInfo(CClearanceInfo const &from) :
		CInheritableInfo(from),
		m_ca_clearance(from.m_ca_clearance)
	{
	}

	explicit CClearanceInfo(int ca_clearance = E_UNDEF)
	{
		m_ca_clearance.Set_item_from_int(ca_clearance);
	}

	CClearanceInfo &operator = (CClearanceInfo const &from)
	{
        CInheritableInfo::operator = (from);

		// Copy Data
		m_ca_clearance = from.m_ca_clearance;

		return *this;
	}

public:
	// Direct for non-updating version
	Item m_ca_clearance;

    // Update the current value of m_ca_clearance
	void Update_ca_clearance()
	{
        GetItem(m_ca_clearance);
	}

protected:
    virtual int GetExtendedItem(Item &item, Item const &src) const
    {
        // Handles E_AUTO_CALC
        return 1;
    }
};

#endif /* !_CLEARANCE_H ] */
