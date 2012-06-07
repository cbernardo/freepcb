#ifndef _DLE_DONUT_H /* [ */
#define _DLE_DONUT_H

#include "DrawingElement.h"

// annulus
class CDLE_DONUT : public CDLE_Symmetric
{
protected:
	virtual void _Draw (CDrawInfo &di, bool);
	// virtual void _DrawClearance(CDrawInfo &di);
};

#endif /* !_DLE_DONUT_H ] */
