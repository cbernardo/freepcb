#ifndef _DLE_ARC_H /* [ */
#define _DLE_ARC_H

#include "DrawingElement.h"

// arc base class
class CDLE_ARC : public dl_element
{
protected:
    int onScreen(void) const;
	virtual int  _isHit(CPoint const &point) const;
};



// arc with clockwise curve
class CDLE_ARC_CW : public CDLE_ARC
{
protected:
	virtual void _Draw(CDrawInfo const &di) const;
};


// arc with counter-clockwise curve
class CDLE_ARC_CCW : public CDLE_ARC
{
protected:
	virtual void _Draw(CDrawInfo const &di) const;
};


#endif /* !_DLE_ARC_H ] */
