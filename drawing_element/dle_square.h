#ifndef _DLE_SQUARE_F_H /* [ */
#define _DLE_SQUARE_F_H

#include "DrawingElement.h"

// filled square
class CDLE_SQUARE : public CDLE_Symmetric
{
protected:
	virtual void _Draw(CDrawInfo &di, bool);
	// virtual void _DrawClearance(CDrawInfo &di);
	virtual void _DrawThermalRelief(CDrawInfo &di);
};

#endif /* !_DLE_SQUARE_F_H ] */
