#ifndef _DLE_HOLE_H /* [ */
#define _DLE_HOLE_H

#include "DrawingElement.h"

// hole, shown as circle
class CDLE_HOLE : public CDLE_Symmetric
{
protected:
	virtual void _Draw(CDrawInfo &di, bool);
	// virtual void _DrawClearance(CDrawInfo  &di);
	virtual int  _getBoundingRect(CRect &rect);
};

#endif /* !_DLE_HOLE_H ] */
