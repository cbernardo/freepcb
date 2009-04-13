#ifndef _DLE_CIRCLE_H /* [ */
#define _DLE_CIRCLE_H

#include "DrawingElement.h"

// filled circle
class CDLE_CIRC : public CDLE_Symmetric
{
protected:
	virtual void _Draw             (CDrawInfo const &di) const;
	virtual void _DrawClearance    (CDrawInfo const &di) const;
	virtual void _DrawThermalRelief(CDrawInfo const &di) const;

	virtual int  _isHit(CPoint const &point) const;
};


// circle outline
class CDLE_HOLLOW_CIRC : public CDLE_Symmetric
{
protected:
	virtual void _Draw(CDrawInfo const &di) const;

	virtual int  _isHit(CPoint const &point) const;
};

#endif /* !_DLE_CIRCLE_H ] */
