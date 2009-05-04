#ifndef _DLE_SQUARE_F_H /* [ */
#define _DLE_SQUARE_F_H

#include "DrawingElement.h"

// filled square
class CDLE_SQUARE : public CDLE_Symmetric
{
protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;
	virtual void _DrawThermalRelief(CDrawInfo const &di) const;
};

#endif /* !_DLE_SQUARE_F_H ] */
