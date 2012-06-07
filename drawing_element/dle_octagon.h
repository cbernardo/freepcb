#ifndef _DLE_OCTAGON_H /* [ */
#define _DLE_OCTAGON_H

#include "DrawingElement.h"

// filled octagon
class CDLE_OCTAGON : public CDLE_Symmetric
{
protected:
	virtual void _Draw(CDrawInfo &di, bool);
	// virtual void _DrawClearance(CDrawInfo  &di);
	virtual void _DrawThermalRelief(CDrawInfo  &di);
};

// octagon outline
typedef CDLE_OCTAGON CDLE_HOLLOW_OCTAGON;

#endif /* !_DLE_OCTAGON_H ] */
