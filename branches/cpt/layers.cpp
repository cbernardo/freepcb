#include "stdafx.h"

int FlipLayer( int side, int layer )
{
	int lay = layer;
	if( side )
	{
		switch( layer )
		{
		case LAY_SILK_TOP: lay = LAY_SILK_BOTTOM; break;
		case LAY_SILK_BOTTOM: lay = LAY_SILK_TOP; break;
		case LAY_TOP_COPPER: lay = LAY_BOTTOM_COPPER; break;
		case LAY_BOTTOM_COPPER: lay = LAY_TOP_COPPER; break;
		default: ASSERT(0); break;
		}
	}
	return lay;
}
