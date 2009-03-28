#ifndef _DLE_OCTAGON_H /* [ */
#define _DLE_OCTAGON_H

#include "DrawingElement.h"

// filled octagon
class CDLE_OCTAGON : public CDLE_Symmetric
{
protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;
	virtual void _DrawThermalRelief(CDrawInfo const &di) const;
};

// octagon outline
typedef CDLE_OCTAGON CDLE_HOLLOW_OCTAGON;

#endif /* !_DLE_OCTAGON_H ] */
