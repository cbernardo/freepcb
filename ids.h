#pragma once

// definition of ID structure used by FreePCB
//
// struct id : this structure is used to identify PCB design elements
// such as instances of parts or nets, and their subelements
// Each element will have its own id.
// An id is attached to each item of the Display List so that it can
// be linked back to the PCB design element which drew it.
// These are mainly used to identify items selected by clicking the mouse
//
// In general: 
//		id.type	= type of PCB element (e.g. part, net, text)
//		id.st	= subelement type (e.g. part pad, net connection)
//		id.i	= subelement index (zero-based)
//		id.sst	= subelement of subelement (e.g. net connection segment)
//		id.ii	= subsubelement index (zero-based)
//		id.uid		= uid for element
//		id.st_uid	= uid for subelement
//		id.sst_uid	= uid for subsubelement
//
// For example, the id for segment 0 of connection 4 of a net would be
//	id = { ID_NET, ID_CONNECT, 4, ID_SEG, 0, 
//			UID of net, UID of connection, UID of segment };
//
//
#include "Cuid.h"

class id {
public:
	// constructor
	id( int qtype=0, int qst=0, int qi=0, int qsst=0, int qii=0,
		int quid=-1, int qst_uid=-1, int qsst_uid=-1 ) 
	{ 
		type = qtype; 
		st = qst; 
		i = qi; 
		sst = qsst; 
		ii = qii; 
		uid = quid;
		st_uid = qst_uid;
		sst_uid = qsst_uid;
	} 
	// operators
	friend BOOL operator ==(id id1, id id2)
	{ return (id1.type==id2.type 
			&& id1.st==id2.st 
			&& id1.sst==id2.sst 
			&& id1.i==id2.i 
			&& id1.ii==id2.ii 
			&& id1.uid==id2.uid 
			&& id1.st_uid==id2.st_uid 
			&& id1.sst_uid==id2.sst_uid 
			); 
	}
	// member functions
	void Clear() 
	{ 
		type = 0; st = 0; i = 0; sst = 0; ii = 0; 
		uid=-1; st_uid=-1; sst_uid=-1;
	} 
	void Set(int qtype=0, int qst=0, int qi=0, int qsst=0, int qii=0,
		int quid=-1, int qst_uid=-1, int qsst_uid=-1 ) 
	{ 
	{ 
		type = qtype; 
		st = qst; 
		i = qi; 
		sst = qsst; 
		ii = qii; 
		uid = quid;
		st_uid = qst_uid;
		sst_uid = qsst_uid;
	} 
	} 
	// member variables
	unsigned int type;	// type of element
	unsigned int st;	// type of subelement
	unsigned int i;		// index of subelement
	unsigned int sst;	// type of subsubelement
	unsigned int ii;	// index of subsubelement
	unsigned int uid;		// UID of element
	unsigned int st_uid;	// UID of subelement
	unsigned int sst_uid;	// UID of subsubelement
};


// these are constants used in ids
// root types
enum {
	ID_NONE = 0,	// an undefined type or st (or an error)
	ID_BOARD,		// board outline
	ID_PART,		// part
	ID_NET,			// net
	ID_TEXT,		// free-standing text
	ID_DRC,			// DRC error
	ID_SM_CUTOUT,	// cutout for solder mask
	ID_CENTROID,	// centroid of footprint
	ID_GLUE,		// adhesive spot
	ID_MULTI		// if multiple selections
};

// subtypes of ID_PART
enum {
	ID_PAD = 1,		// pad_stack in a part
	ID_SEL_PAD,		// selection rectangle for pad_stack in a part
	ID_OUTLINE,		// part outline
	ID_REF_TXT,		// text showing ref num for part
	ID_VALUE_TXT,	// text showing value for part
	ID_FP_TXT,		// free text in footprint
	ID_ORIG,		// part origin
	ID_SEL_RECT,	// selection rectangle for part
	ID_SEL_REF_TXT,		// selection rectangle for ref text
	ID_SEL_VALUE_TXT,	// selection rectangle for value text
	ID_SEL_FP_TEXT		// selection rectangle for text in FP
};

// subtypes of ID_TEXT
enum {
	ID_SEL_TXT = 1,		// selection rectangle
	ID_STROKE			// stroke for text
};

// subtypes of ID_NET
enum {
	ID_ENTIRE_NET = 0,
	ID_CONNECT,		// connection
	ID_AREA			// copper area
};

// subtypes of ID_BOARD
enum {
	ID_BOARD_OUTLINE = 1,
};

// subsubtypes of ID_NET.ID_CONNECT
enum {
	ID_ENTIRE_CONNECT = 0,
	ID_SEG,
	ID_SEL_SEG,	
	ID_VERTEX,	
	ID_SEL_VERTEX,
	ID_VIA
};

// subsubtypes of ID_NET.ID_AREA, ID_BOARD.ID_BOARD_OUTLINE, ID_SM_CUTOUT
enum {
	ID_SIDE = 1,
	ID_SEL_SIDE,
	ID_SEL_CORNER,
	ID_HATCH,
	ID_PIN_X,	// only used by ID_AREA
	ID_STUB_X	// only used by ID_AREA
};

// subtypes of ID_DRC
// for subsubtypes, use types in DesignRules.h
enum {
	ID_DRE = 1,
	ID_SEL_DRE
};

// subtypes of ID_CENTROID
enum {
	ID_CENT = 1,
	ID_SEL_CENT
};

// subtypes of ID_GLUE
enum {
	ID_SPOT = 1,
	ID_SEL_SPOT
};

