#ifndef _DLE_RECT_H /* [ */
#define _DLE_RECT_H

#include "DrawingElement.h"

// filled rectangle
class CDLE_RECT : public CDLE_Rectangular
{
protected:
	virtual void _Draw(CDrawInfo &di, bool);
	// virtual void _DrawClearance(CDrawInfo &di);
};


// rectangle outline
class CDLE_HOLLOW_RECT : public CDLE_Rectangular
{
protected:
	virtual void _Draw(CDrawInfo &di, bool);
	// virtual void _DrawClearance(CDrawInfo  &di) ;
	virtual int _IsHit(double x, double y, double &d);
};


// rectangle outline with X
class CDLE_RECT_X : public CDLE_HOLLOW_RECT
{
protected:
	virtual void _Draw(CDrawInfo &di, bool);
};

#endif /* !_DLE_RECT_H ] */
