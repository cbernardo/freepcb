#ifndef _DLE_OVAL_H /* [ */
#define _DLE_OVAL_H

#include "DrawingElement.h"

// filled oval
class CDLE_OVAL : public CDLE_Rectangular
{
protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;
};

// oval outline
typedef CDLE_OVAL CDLE_HOLLOW_OVAL;

#endif /* !_DLE_OVAL_H ] */
