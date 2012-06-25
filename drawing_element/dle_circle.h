#ifndef _DLE_CIRCLE_H /* [ */
#define _DLE_CIRCLE_H

#include "DrawingElement.h"

// filled circle
class CDLE_CIRC : public CDLE_Symmetric
{
protected:
	virtual void _Draw(CDrawInfo &di, bool);
	// virtual void _DrawClearance (CDrawInfo &di);
	virtual void _DrawThermalRelief(CDrawInfo &di);
	virtual int _IsHit(double x, double y, double &d);
};


// circle outline
class CDLE_HOLLOW_CIRC : public CDLE_Symmetric
{
protected:
	virtual void _Draw(CDrawInfo &di, bool);
	virtual int  _IsHit(CPoint &point);
};

#endif /* !_DLE_CIRCLE_H ] */
