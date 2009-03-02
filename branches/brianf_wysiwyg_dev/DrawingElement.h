#ifndef _DRAWINGELEMENT_H /* [ */
#define _DRAWINGELEMENT_H

#include "afxwin.h"
#include "ids.h"
#include "layers.h"
#include "LinkList.h"

struct CDrawInfo;
class CDisplayList;

// this structure contains an element of the display list
class dl_element : public CDLinkList
{
	friend class CDisplayList;

public:
	int magic;
	id id;			// identifier (see ids.h)
	void * ptr;		// pointer to object drawing this element
	int gtype;		// type of primitive
	int visible;	// 0 to hide

	dl_element();

	void Draw(CDrawInfo const &di) const;
	void DrawClearance(CDrawInfo const &di) const { _DrawClearance(di); }
    void DrawThermalRelief(CDrawInfo const &di) const;

	int isHit(CPoint const &point) const;
	int getBoundingRect(CRect &rect) const { return _getBoundingRect(rect); }

	void Remove(void);

protected:
	virtual void _Draw         (CDrawInfo const &di) const {}
	virtual void _DrawClearance(CDrawInfo const &di) const {}
    virtual void _DrawThermalRelief(CDrawInfo const &di) const {}

	virtual int  _isHit(CPoint const &point) const { return 0; }
	virtual int _getBoundingRect(CRect &rect) const { return 0; }

protected:
	friend class CDL_job;
	friend class CDL_job_traces;

	CDisplayList * dlist;

	int sel_vert;     // for selection rectangles, 1 if part is vertical
	int w;            // width (for round or square shapes)
	int holew;        // hole width (for round holes)
	int clearancew;   // clearance width (for lines and pads)
	CPoint org;       // x origin (for rotation, reflection, etc.)
	CPoint i;         // starting or center position of element
	CPoint f;         // opposite corner (for rectangle or line)
	int radius;       // radius of corners for DL_RRECT
	int layer;        // layer to draw on
	int orig_layer;   // for elements on highlight layer,
	                  // the original layer, the highlight will
	                  // only be drawn if this layer is visible
};

// n-sided with equal length sides
class CDLE_Symmetric : public dl_element
{
protected:
    int onScreen(void) const;
	virtual int  _getBoundingRect(CRect &rect) const;
};

// rectangular
class CDLE_Rectangular : public dl_element
{
protected:
    int onScreen(void) const;
	virtual int  _getBoundingRect(CRect &rect) const;
	virtual void _DrawThermalRelief(CDrawInfo const &di) const;
};


// line segment with round end-caps
class CDLE_LINE : public dl_element
{
    void __Draw(CDrawInfo const &di, int mode) const;

protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;

	virtual int  _isHit(CPoint const &point) const;

	virtual int  _getBoundingRect(CRect &rect) const;
};

// filled circle
class CDLE_CIRC : public CDLE_Symmetric
{
protected:
	virtual void _Draw             (CDrawInfo const &di) const;
	virtual void _DrawClearance    (CDrawInfo const &di) const;
	virtual void _DrawThermalRelief(CDrawInfo const &di) const;

	virtual int  _isHit(CPoint const &point) const;
};

// annulus
class CDLE_DONUT : public CDLE_Symmetric
{
protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;
};

// filled square
class CDLE_SQUARE : public CDLE_Symmetric
{
protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;
	virtual void _DrawThermalRelief(CDrawInfo const &di) const;
};

// filled rectangle
class CDLE_RECT : public CDLE_Rectangular
{
protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;
};

// filled rounded rectangle
class CDLE_RRECT : public CDLE_Rectangular
{
protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;
};

// rounded rectangle outline
typedef CDLE_RRECT CDLE_HOLLOW_RRECT;


// filled oval
class CDLE_OVAL : public CDLE_Rectangular
{
protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;
};

// oval outline
typedef CDLE_OVAL CDLE_HOLLOW_OVAL;


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

// hole, shown as circle
class CDLE_HOLE : public CDLE_Symmetric
{
protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;
	virtual int  _getBoundingRect(CRect &rect) const;
};

// circle outline
class CDLE_HOLLOW_CIRC : public CDLE_Symmetric
{
protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;
	virtual int  _isHit(CPoint const &point) const;
};

// rectangle outline
class CDLE_HOLLOW_RECT : public CDLE_Rectangular
{
protected:
	virtual void _Draw         (CDrawInfo const &di) const;
	virtual void _DrawClearance(CDrawInfo const &di) const;
	virtual int  _isHit(CPoint const &point) const;
};


// rectangle outline with X
class CDLE_RECT_X : public CDLE_Rectangular
{
protected:
	virtual void _Draw(CDrawInfo const &di) const;
	virtual int  _isHit(CPoint const &point) const;
};

// shape to highlight a point
class CDLE_POINT : public dl_element
{
protected:
	virtual void _Draw(CDrawInfo const &di) const;
};

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

// circle and X
class CDLE_CENTROID : public CDLE_Symmetric
{
protected:
	virtual void _Draw(CDrawInfo const &di) const;
};

// X
class CDLE_X : public dl_element
{
protected:
	virtual void _Draw(CDrawInfo const &di) const;
};

#endif /* !_DRAWINGELEMENT_H ] */
