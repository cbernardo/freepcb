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

	// CPT: reworked a bit:
	void Draw(CDrawInfo &di);
	virtual void DrawHiliteSegs (CDrawInfo &di) { }
	// LATER?  void _DrawClearance(CDrawInfo &di);
    void DrawThermalRelief(CDrawInfo &di);
	int isHit(CPoint  &point) ;
	int getBoundingRect(CRect &rect)  { return _getBoundingRect(rect); }

protected:
	virtual void _Draw(CDrawInfo &di, bool fHiliteSegs) {}
    virtual void _DrawThermalRelief(CDrawInfo &di) {}

	virtual int  _isHit(CPoint &point) { return 0; }
	virtual int  _getBoundingRect(CRect &rect)  { return 0; }

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
    int onScreen(void) ;
	virtual int  _getBoundingRect(CRect &rect) ;
};

// rectangular
class CDLE_Rectangular : public dl_element
{
protected:
    int onScreen(void) ;
	virtual int  _getBoundingRect(CRect &rect) ;
	virtual void _DrawThermalRelief(CDrawInfo  &di) ;
};


#endif /* !_DRAWINGELEMENT_H ] */
