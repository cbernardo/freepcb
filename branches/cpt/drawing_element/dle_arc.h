#ifndef _DLE_ARC_H /* [ */
#define _DLE_ARC_H

#include "DrawingElement.h"

// arc base class
class CDLE_ARC : public CDLElement
{
protected:
    int onScreen(void);
	virtual int _IsHit(double x, double y, double &d);
};



// arc with clockwise curve
class CDLE_ARC_CW : public CDLE_ARC
{
protected:
	virtual void _Draw(CDrawInfo &di, bool);
};


// arc with counter-clockwise curve
class CDLE_ARC_CCW : public CDLE_ARC
{
protected:
	virtual void _Draw(CDrawInfo &di, bool);
};


#endif /* !_DLE_ARC_H ] */
