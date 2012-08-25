//  DesignRules.h.  Header file for classes related to design-rule checking, including cdre (which descends from cpcb_item), cdrelist, and DesignRules.

#pragma once
#include "PcbItem.h"
#include "Undo.h"

struct DesignRules
{
	BOOL bCheckUnrouted;
	int trace_width; 
	int pad_pad; 
	int pad_trace;
	int trace_trace; 
	int hole_copper; 
	int annular_ring_pins;
	int annular_ring_vias;
	int board_edge_copper;
	int board_edge_hole;
	int hole_hole;
	int copper_copper;
};

class cdre : public cpcb_item
{
public:
	// Represents Design Rule Check error items.  Incorporates old class DRError
	enum {				
		// subtypes of errors 
		PAD_PAD = 1,				
		PAD_PADHOLE,		
		PADHOLE_PADHOLE,	
		SEG_PAD,			
		SEG_PADHOLE,	
		VIA_PAD,			
		VIA_PADHOLE,		
		VIAHOLE_PAD,		
		VIAHOLE_PADHOLE,	
		SEG_SEG,		
		SEG_VIA,			
		SEG_VIAHOLE,	
		VIA_VIA,			
		VIA_VIAHOLE,		
		VIAHOLE_VIAHOLE,	
		TRACE_WIDTH,	
		RING_PAD,			
		RING_VIA,
		BOARDEDGE_PAD,			
		BOARDEDGE_PADHOLE,	
		BOARDEDGE_VIA,			
		BOARDEDGE_VIAHOLE,		
		BOARDEDGE_SEG,	
		BOARDEDGE_COPPERAREA,	
		COPPERGRAPHIC_PAD,			
		COPPERGRAPHIC_PADHOLE,	
		COPPERGRAPHIC_VIA,			
		COPPERGRAPHIC_VIAHOLE,		
		COPPERGRAPHIC_SEG,	
		COPPERGRAPHIC_COPPERAREA,	
		COPPERGRAPHIC_BOARDEDGE,	
		COPPERAREA_COPPERAREA,
		COPPERAREA_INSIDE_COPPERAREA,
		COPPERAREA_BROKEN,					// CPT
		UNROUTED
	};
	
	int index;					// CPT2 new
	int type;					// id, using subtypes above
	CString str;				// descriptive string
	cpcb_item *item1, *item2;	// items tested
	int x, y;					// position of error
	int w;						// width of circle (CPT2 new)
	int layer;					// layer (if pad error)

	cdre(CFreePcbDoc *_doc, int _index, int _type, CString *_str, cpcb_item *_item1, cpcb_item *_item2, 
		int _x, int _y, int _w, int _layer=0);
	cdre(CFreePcbDoc *_doc, int _uid);

	bool IsDRE() { return true; }
	cdre *ToDRE() { return this; }
	int GetTypeBit() { return bitDRE; }
	virtual bool IsOnPcb();
	cundo_item *MakeUndoItem()
		{ return new cudre(this); }

	int Draw();
	void Highlight();
};

class cdrelist
{
public:
	// Updated version of class DRErrorList.
	CFreePcbDoc *doc;
	carray<cdre> dres;

	cdrelist(CFreePcbDoc *_doc)
		{ doc = _doc; }
	~cdrelist() { }
	cdre * Add( int type, CString * str, cpcb_item *item1, cpcb_item *item2,
		int x1, int y1, int x2, int y2, int w, int layer );
	void Remove( cdre *dre );									// Undraw a single member, then remove it.
	void Clear();												// Undraw all members, then remove 'em.
	int GetSize() 
		{ return dres.GetSize(); }
	void MakeHollowCircles();
	void MakeSolidCircles();
};
