#ifndef _II_SEG_WIDTH_H /* [ */
#define _II_SEG_WIDTH_H

#include "inheritable_item.h"

// Width information for segments
class CSegWidthInfo : public CInheritableInfo
{
public:
	CSegWidthInfo( CInheritableInfo const &from ) :
		CInheritableInfo(from)
	{
		*this = from;
	}

	explicit CSegWidthInfo( int seg_width = E_USE_PARENT )
	{
		FileToItem(seg_width, m_seg_width);
	}

	CSegWidthInfo &operator = ( CInheritableInfo const &from );
	CSegWidthInfo &operator = ( CSegWidthInfo const &from ) { operator = (static_cast<CInheritableInfo const &>(from)); return *this; }

public:
	// Direct for non-updating version
	Item m_seg_width;

	// For backwards compatibility with .fpc files, value of 0 means "use_parent".
	static int  ItemToFile(Item const &from)   { int to = from.GetItemAsInt(); if (to == E_USE_PARENT) to = 0; return to; }
	static void FileToItem(int from, Item &to) { if (from == 0) from = E_USE_PARENT; to.SetItemFromInt(from); }

    // Update the current value of ...
	void Update_seg_width();

	// Update all
	void Update()
	{
		Update_seg_width();
	}

	virtual Item const &GetItem( int item_id ) const;
};


class CConnectionWidthInfo : public CSegWidthInfo
{
public:
	CConnectionWidthInfo() :
		m_via_width(E_USE_PARENT),
		m_via_hole (E_USE_PARENT)
	{
	}

	CConnectionWidthInfo( CInheritableInfo const &from ) :
		CSegWidthInfo( from )
	{
		*this = from;
	}

	CConnectionWidthInfo( int seg_width, int via_width, int via_hole ) :
		CSegWidthInfo( seg_width )
	{
		FileToItem(via_width, m_via_width);
		FileToItem(via_hole,  m_via_hole);
	}

	CConnectionWidthInfo &operator = ( CInheritableInfo const &from );
	CConnectionWidthInfo &operator = ( CConnectionWidthInfo const &from ) { operator = (static_cast<CInheritableInfo const &>(from)); return *this; }

public:
	// Direct for non-updating version
	Item m_via_width;
	Item m_via_hole;

    // Update the current value of ...
	void Update_via_width();
	void Update_via_hole();

	// Update all
	void Update()
	{
		CSegWidthInfo::Update();

		Update_via_width();
		Update_via_hole();
	}

	virtual Item const &GetItem( int item_id ) const;
};


class CNetWidthInfo : public CConnectionWidthInfo
{
public:
	CNetWidthInfo() : 
		m_ca_clearance(E_USE_PARENT)
	{
	}

	CNetWidthInfo( CInheritableInfo const &from ) :
		CConnectionWidthInfo( from )	
	{
		*this = from;
	}

	CNetWidthInfo &operator = ( CInheritableInfo const &from );
	CNetWidthInfo &operator = ( CNetWidthInfo const &from ) { operator = (static_cast<CInheritableInfo const &>(from)); return *this; }

public:
	// Direct for non-updating version
	Item m_ca_clearance;

    // Update the current value of ...
	void Update_ca_clearance();

	// Update all
	void Update()
	{
		CConnectionWidthInfo::Update();

		Update_ca_clearance();
	}

	virtual Item const &GetItem( int item_id ) const;
};


#endif /* !_II_SEG_WIDTH_H ] */
