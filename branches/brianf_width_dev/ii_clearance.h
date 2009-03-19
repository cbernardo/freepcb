#ifndef _II_CLEARANCE_H /* [ */
#define _II_CLEARANCE_H

#include "ii_FreePcb.h"

// Information for clearances
class CClearanceInfo : public CII_FreePcb
{
protected:
    virtual void GetItemExt(Item &item, Item const &src) const;

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
};


class CViaWidthInfo : public CClearanceInfo
{
public:
	class CViaItem : public CClearanceInfo::Item
	{
	protected:
		virtual void assign_from(Item const &from);

	public:
		CViaItem() {}
		CViaItem(Item const &from) : Item(from) {}
		explicit CViaItem(int val_status) : Item(val_status) {}

	public:
		CViaItem &operator = (int val_status)       { Item::operator = (val_status); return *this; }
		CViaItem &operator = (Item const &from)     { assign_from(from); return *this; }
		CViaItem &operator = (CViaItem const &from) { assign_from(from); return *this; }
	};

public:
	CViaWidthInfo()
		: CClearanceInfo( CClearanceInfo::E_AUTO_CALC )
		, m_via_width(0)
		, m_via_hole(0)
	{
	}

	CViaWidthInfo &operator = (CInheritableInfo const &from);
	CViaWidthInfo &operator = (CViaWidthInfo const &from) { operator = (static_cast<CInheritableInfo const &>(from)); return *this; }

public:
	CViaItem m_via_width;
	CViaItem m_via_hole;

    // Update the current value of ...
	void Update_via_width();
	void Update_via_hole();

	// Update all
	void Update()
	{
		CClearanceInfo::Update();

		Update_via_width();
		Update_via_hole();
	}

	// Undef all
	void Undef()
	{
		CClearanceInfo::Undef();

		m_via_width.Undef();
		m_via_hole.Undef();
	}

protected:
	virtual Item const &GetItem(int item_id) const;
};

#endif /* !_II_CLEARANCE_H ] */
