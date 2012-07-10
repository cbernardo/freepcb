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

int FpToPCBLayer( int fp_layer )
{
	switch( fp_layer )
	{
	case LAY_FP_SILK_TOP: return LAY_SILK_TOP; break;
	case LAY_FP_SILK_BOTTOM: return LAY_SILK_BOTTOM; break;
	case LAY_FP_TOP_COPPER: return LAY_TOP_COPPER; break;
	case LAY_FP_BOTTOM_COPPER: return LAY_BOTTOM_COPPER; break;
	default: ASSERT(0); return -1; break;
	}
}
