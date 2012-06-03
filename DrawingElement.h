#ifndef _DRAWINGELEMENT_H /* [ */
#define _DRAWINGELEMENT_H

#include "afxwin.h"
#include "ids.h"
#include "layers.h"
#include "LinkList.h"

struct CDrawInfo;
class CDisplayList;

// this structure contains an element of the display list
// CPT:  no longer derived from CDLinkList.
class dl_element
{
	friend class CDisplayList;
	friend class CDisplayLayer;

public:
	int magic;
	id id;			// identifier (see ids.h)
	void * ptr;		// pointer to object drawing this element
	int gtype;		// type of primitive
	int visible;	// 0 to hide

	dl_element();

	void Draw             (CDrawInfo const &di) const;
	void DrawClearance    (CDrawInfo const &di) const { _DrawClearance(di); }
    void DrawThermalRelief(CDrawInfo const &di) const;

	int isHit(CPoint const &point) const;
	int getBoundingRect(CRect &rect) const { return _getBoundingRect(rect); }

protected:
	virtual void _Draw             (CDrawInfo const &di) const {}
	virtual void _DrawClearance    (CDrawInfo const &di) const {}
    virtual void _DrawThermalRelief(CDrawInfo const &di) const {}

	virtual int  _isHit(CPoint const &point) const { return 0; }
	virtual int  _getBoundingRect(CRect &rect) const { return 0; }

protected:
	CDisplayList * dlist;

	int sel_vert;     // for selection rectangles, 1 if part is vertical
	int w;            // width (for round or square shapes)
	int holew;        // hole width (for round holes)
	int clearancew;   // clearance width (for lines and pads)
	CPoint org;       // x origin (for rotation, reflection, etc.)
	CPoint i;         // starting or center position of element
	CPoint f;         // opposite corner (for rectangle or line)
	int radius;       // radius of corners for DL_RRECT
	int layer;        // layer to draw on.  CPT:  for elements in the selection layer, this is not necessarily LAY_SELECTION!
	int orig_layer;   // for elements on highlight layer,
	                  // the original layer, the highlight will
	                  // only be drawn if this layer is visible
	dl_element *prev, *next;			// CPT.  I'm phasing out references to CDLinkList, and am implementing the linked lists this way.
	CDisplayLayer *displayLayer;		// CPT
public:
	void Unhook();						// CPT.  Used to be called Remove(), but this name is more descriptive.

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


#endif /* !_DRAWINGELEMENT_H ] */
