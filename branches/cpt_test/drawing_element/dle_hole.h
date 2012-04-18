#ifndef _DLE_HOLE_H /* [ */
#define _DLE_HOLE_H

#include "DrawingElement.h"

// hole, shown as circle
class CDLE_HOLE : public CDLE_Symmetric
{
protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;
	virtual int  _getBoundingRect(CRect &rect) const;
};

#endif /* !_DLE_HOLE_H ] */
