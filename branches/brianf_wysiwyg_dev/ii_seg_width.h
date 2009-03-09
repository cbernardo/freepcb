#ifndef _II_SEG_WIDTH_H /* [ */
#define _II_SEG_WIDTH_H

#include "inheritable_item.h"

// Information for clearances
class CSegWidthInfo : public CInheritableInfo
{
public:
	CSegWidthInfo() :
		m_seg_width(E_USE_PARENT),
		m_via_width(E_USE_PARENT),
		m_via_hole (E_USE_PARENT)
	{
	}

	CSegWidthInfo(CInheritableInfo const &from) :
		CInheritableInfo(from)
	{
		*this = from;
	}

	CSegWidthInfo(int seg_width, int via_width, int via_hole_width)
	{
		m_seg_width.Set_item_from_int(seg_width);
		m_via_width.Set_item_from_int(via_width);
		m_via_hole .Set_item_from_int(via_hole_width);
	}

	CSegWidthInfo &operator = (CInheritableInfo const &from);

public:
	// Direct for non-updating version
	Item m_seg_width;
	Item m_via_width;
	Item m_via_hole;

    // Update the current value of ...
	void Update_seg_width();
	void Update_via_width();
	void Update_via_hole();

	// Update all
	void Update()
	{
		Update_seg_width();
		Update_via_width();
		Update_via_hole();
	}

protected:
	virtual Item const &_GetItem(int item_id) const;
};

#endif /* !_II_SEG_WIDTH_H ] */
