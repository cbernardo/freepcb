#pragma once

#include "inheritable_item.h"

class CII_FreePcb : public CInheritableInfo
{
public:
	// This is a list of all inheritable item IDs used in the project
	enum EInheritableItemIds
	{
		E_II_TRACE_WIDTH,
		E_II_VIA_WIDTH,
		E_II_VIA_HOLE,
		E_II_CA_CLEARANCE
	};

	enum EStatus
	{
        // Remember, these are negative so normal enum
        // counting order won't work here.
		E_AUTO_CALC          = CInheritableInfo::E__STATUS_USER,
		E_USE_DEF_FROM_WIDTH = E_AUTO_CALC-1,
	};

public:
	CII_FreePcb() {}
	CII_FreePcb(CInheritableInfo const &from) : CInheritableInfo(from) {}
	CII_FreePcb(CII_FreePcb const &from) : CInheritableInfo(from) {}

public:
	static CString GetItemText(Item const &item);
};
