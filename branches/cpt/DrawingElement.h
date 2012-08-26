#ifndef _DRAWINGELEMENT_H /* [ */
#define _DRAWINGELEMENT_H

#include "afxwin.h"
#include "layers.h"
// #include "LinkList.h"

struct CDrawInfo;
class CDisplayList;
class CPcbItem;

// this structure contains an element of the display list
// CPT:  no longer derived from CDLinkList.
class CDLElement
{
	friend class CDisplayList;
	friend class CDisplayLayer;

public:
	int magic;
	CPcbItem *item;												// CPT2.
	int usage;														// CPT2. A given pcb-item may have various types of drawing-elements associated
																	// with it (e.g. most have a main element and a selector;  parts have ref and value 
																	// elements, etc.)
																	// Currently id's are used to show an element's role, but I'm trying this method. Not
																	// sure if it's really useful.
	enum { DL_MAIN, DL_SEL, DL_REF, DL_REF_SEL, 
		DL_VALUE, DL_VALUE_SEL, DL_HATCH, DL_OTHER };				// Values for "usage" 
	int gtype;		// type of primitive
	int visible;	// 0 to hide
public:										// CPT2: was protected, causing pains in my tail.  Maybe change later...
	CDisplayList * dlist;

	int sel_vert;     // for selection rectangles, 1 if part is vertical.  CPT2 TODO did a search and it appears totally unused;  eliminate
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
	CDLElement *prev, *next;			// CPT.  I'm phasing out references to CDLinkList, and am implementing the linked lists this way.
	CDisplayLayer *displayLayer;		// CPT


	CDLElement();

	// CPT: reworked the following 6 declarations a bit:
	void Draw(CDrawInfo &di);
	virtual void DrawHiliteSeg (CDrawInfo &di) { }						// CPT: Does nothing except with CDLE_LINE objects.
	// LATER?  void _DrawClearance(CDrawInfo &di);
    void DrawThermalRelief(CDrawInfo &di);
	int IsHit(double x, double y, double &d) ;							// CPT: changed args.  Renamed with capital letter (consistency rules!)
	int GetBoundingRect(CRect &rect)  { return _GetBoundingRect(rect); }

protected:
	virtual void _Draw(CDrawInfo &di, bool fHiliteSegs) {}
    virtual void _DrawThermalRelief(CDrawInfo &di) {}

	virtual int _IsHit(double x, double y, double &d) { return 0; }
	virtual int _GetBoundingRect(CRect &rect)  { return 0; }

	void Unhook();						// CPT.  Used to be called Remove(), but this name is more descriptive.

};

// n-sided with equal length sides
class CDLE_Symmetric : public CDLElement
{
protected:
    int onScreen(void) ;
	virtual int  _GetBoundingRect(CRect &rect) ;
};

// rectangular
class CDLE_Rectangular : public CDLElement
{
protected:
    int onScreen(void) ;
	virtual int  _GetBoundingRect(CRect &rect) ;
	virtual void _DrawThermalRelief(CDrawInfo  &di) ;
};


#endif /* !_DRAWINGELEMENT_H ] */
