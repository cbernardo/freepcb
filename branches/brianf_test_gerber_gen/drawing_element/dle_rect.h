#ifndef _DLE_RECT_H /* [ */
#define _DLE_RECT_H

#include "DrawingElement.h"

// filled rectangle
class CDLE_RECT : public CDLE_Rectangular
{
protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;
};


// rectangle outline
class CDLE_HOLLOW_RECT : public CDLE_Rectangular
{
protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;
	virtual int  _isHit(CPoint const &point) const;
};


// rectangle outline with X
class CDLE_RECT_X : public CDLE_HOLLOW_RECT
{
protected:
	virtual void _Draw(CDrawInfo const &di) const;
};

#endif /* !_DLE_RECT_H ] */
