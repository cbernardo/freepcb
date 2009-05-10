#ifndef _II_CLEARANCE_H /* [ */
#define _II_CLEARANCE_H

#include "ii_FreePcb.h"

// Information for clearances
class CClearanceInfo : public CII_FreePcb
{
protected:
    virtual void GetItemExt(Item &item, Item const &src) const;
	virtual void OnRemoveParent(CInheritableInfo const *pOldParent)
	{
		m_ca_clearance.OnRemoveParent();

		CII_FreePcb::OnRemoveParent(pOldParent);
	}

public:
	explicit CClearanceInfo(CInheritableInfo const &from)
		: CII_FreePcb(from)
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
protected:
	virtual void OnRemoveParent(CInheritableInfo const *pOldParent)
	{
		m_via_width.OnRemoveParent();
		m_via_hole .OnRemoveParent();

		CClearanceInfo::OnRemoveParent(pOldParent);
	}

public:
	class CViaItem : public CClearanceInfo::Item
	{
	protected:
		virtual void assign_from(Item const &from);

	public:
		CViaItem() {}
		CViaItem(Item const &from)        : Item(from) {}
		explicit CViaItem(int val_status) : Item(val_status) {}

	public:
		CViaItem &operator = (int val_status)       { Item::operator = (val_status); return *this; }
		CViaItem &operator = (Item const &from)     { Item::operator = (from);       return *this; }
		CViaItem &operator = (CViaItem const &from) { Item::operator = (from);       return *this; }
	};

public:
	// Initial values:
	// 1) No via (size = 0)
	// 2) Auto calc the clearance
	CViaWidthInfo()
		: CClearanceInfo( CClearanceInfo::E_USE_PARENT )
		, m_via_width(0)
		, m_via_hole(0)
	{
	}

	explicit CViaWidthInfo( CInheritableInfo const &from )
		: CClearanceInfo(from)
	{
		*this = from;
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

	void SetNoVia()
	{ 
		m_via_width = m_via_hole = 0; 
	}

protected:
	virtual Item const &GetItem(int item_id) const;
};

#endif /* !_II_CLEARANCE_H ] */
