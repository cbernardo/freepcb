#ifndef _II_CLEARANCE_H /* [ */
#define _II_CLEARANCE_H

#include "ii_FreePcb.h"

// Information for clearances
class CClearanceInfo : public CII_FreePcb
{
public:
	explicit CClearanceInfo(CInheritableInfo const &from) :
		CII_FreePcb(from)
	{
		*this = from;
	}

	explicit CClearanceInfo(int ca_clearance = E_USE_PARENT)
	{
		m_ca_clearance.SetItemFromInt(ca_clearance);
	}

	CClearanceInfo &operator = (CInheritableInfo const &from);
	CClearanceInfo &operator = (CClearanceInfo const &from) { operator = (static_cast<CInheritableInfo const &>(from)); return *this; }

public:
	// Direct for non-updating version
	Item m_ca_clearance;

    // Update the current value of ...
	void Update_ca_clearance();

	// Update all
	void Update()
	{
		Update_ca_clearance();
	}

	// Undef all
	void Undef()
	{
		m_ca_clearance.Undef();
	}

	virtual Item const &GetItem(int item_id) const;

protected:
    virtual void GetItemExt(Item &item, Item const &src) const;
};

#endif /* !_II_CLEARANCE_H ] */
