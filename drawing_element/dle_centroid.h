#ifndef _DLE_CENTROID_H /* [ */
#define _DLE_CENTROID_H

#include "DrawingElement.h"

// circle and X
class CDLE_CENTROID : public CDLE_Symmetric
{
protected:
	virtual void _Draw(CDrawInfo &di, bool);
};

#endif /* !_DLE_CENTROID_H ] */
