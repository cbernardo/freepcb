#ifndef _DLE_LINE_H /* [ */
#define _DLE_LINE_H

#include "DrawingElement.h"

// line segment with round end-caps
class CDLE_LINE : public dl_element
{
    void __Draw(CDrawInfo const &di, int mode) const;

protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;

	virtual int  _isHit(CPoint const &point) const;

	virtual int  _getBoundingRect(CRect &rect) const;
};

#endif /* !_DLE_LINE_H ] */
