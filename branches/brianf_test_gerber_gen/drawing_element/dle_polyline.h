#ifndef _DLE_POLYLINE_H /* [ */
#define _DLE_POLYLINE_H

#include "DrawingElement.h"

// class CDLE_POLYLINE is a dl_element used to draw a 
// closed CPolyLine using a GDI polygon
// where dl_element::ptr points to the parent CPolyLine,
// which can't be destoyed for the life of the CDLE_POLYLINE
class CDLE_POLYLINE : public dl_element
{
protected:
	virtual void _Draw(CDrawInfo const &di) const;
    int onScreen(void) const;
};

#endif /* !_DLE_POLYLINE_H ] */
