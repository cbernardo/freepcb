#ifndef _DLE_DONUT_H /* [ */
#define _DLE_DONUT_H

#include "DrawingElement.h"

// annulus
class CDLE_DONUT : public CDLE_Symmetric
{
protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;
};

#endif /* !_DLE_DONUT_H ] */
