#include "stdafx.h"

#include "FreePcb.h"
#include "II_FreePcb.h"

CString CII_FreePcb::GetItemText(CInheritableInfo::Item const &item)
{
	CString str;

	switch( item.m_status )
	{
	case E_USE_VAL:
		str.Format( "%d", item.m_val / NM_PER_MIL );
		break;

	case E_USE_PARENT:
		str.Format( "Default" );
		break;

	case E_AUTO_CALC:
		str.Format( "Auto" );
		break;

	case E_USE_DEF_FROM_WIDTH:
		str.Format( "->Trace" );
		break;

	default:
		str.Format( "Undefined" );
		break;
	}

	return str;
}
