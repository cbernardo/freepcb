#pragma once

// definition of ID structure used by FreePCB
//
// struct id : this structure is used to identify PCB design elements
// such as instances of parts or nets, and their subelements
// Each element will have its own id.
// An id is attached to each item of the DisplayList so that it can
// be linked back to the PCB design element which drew it.
// These are mainly used to identify items selected from the
// DisplayList by clicking the mouse
//
// In general: 
//		id.t1	= top level type of PCB element (eg. ID_NET)
//		id.u1	= uid for top level type
//		id.t2	= sub-type (eg. ID_CONNECT, a connection in the net)
//		id.u2	= sub-type uid
//		id.i2	= sub-type index (for ordered sub-elements)
//		id.t3	= sub-sub-type (eg. ID_SEG, a segment of the connection)
//		id.u3	= sub-sub-type uid
//		id.i3	= sub-sub-type index (for ordered sub-sub-elements)
//
// For example, the id for segment 0 of connection 4 of a net would be
//	id = {  ID_NET, uid of net, 
//			ID_CONNECT, uid of con, 4, 
//			ID_SEG, uid of seg, 0 
//	};
//

class cpart;
class part_pin;
class cnet;
class cconnect;
class cseg;
class cvertex;
class carea;
class CText;
class CPolyLine;
class CPolySide;
class CPolyCorner;

class id {
public:
	enum { NOP = -2 };	// for Set();

	// constructor
	id( int qt1 = 0, int qu1 = -1,
		int qt2 = 0, int qu2 = -1, int qi2 = -1, 
		int qt3 = 0, int qu3 = -1, int qi3 = -1 ); 

	// == operator
	friend BOOL operator ==(const id &id1, const id &id2);

	// member functions
	// set id elements
	void Clear();
	BOOL Resolve();
	void Set(	int t1=0, int u1=-1, 
				int t2=0, int u2=-1, int i2=-1, 
				int t3=0, int u3=-1, int i3=-1  );
	void SetSubType( int t2, int u2=-1, int i2=-1 );
	void SetSubSubType( int t3, int u3=-1, int i3=-1 );
	void SetPtr( void * p );

	void SetT1( int t1 );
	void SetU1( int u1 );
	void SetT2( int t2 );
	void SetU2( int u2 );
	void SetI2( int i2 );
	void SetT3( int t3 );
	void SetU3( int u3 );
	void SetI3( int i3 );

	// get id elements
	void * Ptr();
	int T1();
	int U1();
	int T2();
	int U2();
	int I2();
	int T3();
	int U3();
	int I3();

	// what type of item ?
	int Type();			// lowest level type in id
	// tests for specific types
	BOOL IsClear();		
	BOOL IsDRC();
	BOOL IsPart();
	BOOL IsRefText();
	BOOL IsValueText();
	BOOL IsPin();
	BOOL IsNet();
	BOOL IsCon();
	BOOL IsSeg();
	BOOL IsVtx();
	BOOL IsArea();
	BOOL IsAreaCorner();
	BOOL IsAreaSide();
	BOOL IsBoard();
	BOOL IsBoardCorner();
	BOOL IsBoardSide();
	BOOL IsMask();
	BOOL IsMaskCorner();
	BOOL IsMaskSide();
	BOOL IsText();
	BOOL IsAnyFootItem();	// any item in a footprint
	BOOL IsCentroid();
	BOOL IsGlue();
	BOOL IsFootPad();
	BOOL IsFootText();
	BOOL IsFootPolySide();
	BOOL IsFootPolyCorner();
	BOOL IsFootRef();
	BOOL IsFootValue();

	// get pointer to item
	cpart * Part();
	part_pin * Pin();
	cnet * Net();
	cconnect * Con();
	cseg * Seg();
	cvertex * Vtx();
	carea * Area();
	CText * Text();
	CPolyLine * BoardOutline();
	CPolyLine * MaskOutline();

	// member variables
protected:
	void * ptr;	// pointer to top-level item
	int t1;		// type of element
	int u1;		// UID of element
	int t2;		// type of subelement
	int u2;		// UID of subelement
	int i2;		// index of subelement
	int t3;		// type of subsubelement
	int u3;		// UID of subsubelement
	int i3;		// index of subsubelement
};


// these are constants used in ids
// root types
enum {
	ID_NONE = 0,	// an undefined type (or an error)
	ID_BOARD,		// board outline
	ID_PART,		// part
	ID_NET,			// net
	ID_TEXT,		// free-standing text
	ID_DRC,			// DRC error
	ID_MASK,		// cutout for solder mask
	ID_FP,			// footprint (in footprint editor)
	ID_MULTI		// if multiple selections
};

// subtypes of ID_PART or ID_FP
enum {
	ID_PAD = 1,		// pad_stack in a part
	ID_SEL_PAD,		// selection rectangle for pad_stack in a part
	ID_POLYLINE,	// part outline
	ID_REF_TXT,		// text showing ref num for part
	ID_VALUE_TXT,	// text showing value for part
	ID_FP_TXT,		// free text in footprint
	ID_CENTROID,	// centroid
	ID_GLUE,		// adhesive spot
	ID_ORIG,		// part origin
	ID_SEL_RECT		// selection rectangle for part
//** AMW2 these shouldn't be used, all instances replaced with ID_SEL_TXT
//	ID_SEL_REF_TXT,		// selection rectangle for ref text
//	ID_SEL_VALUE_TXT	// selection rectangle for value text
};

// subtype of ID_TEXT is always ID_TEXT

// subsubtypes of ID_TEXT, ID_REF_TXT, etc.
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

// subtypes of ID_BOARD and ID_MASK
enum {
	ID_OUTLINE = 1,
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

// subsubtypes of ID_NET.ID_AREA, ID_BOARD.ID_OUTLINE, ID_MASK.ID_OUTLINE
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

// subsubtypes of ID_CENTROID
enum {
	ID_CENT = 1,
	ID_SEL_CENT
};

// subsubtypes of ID_GLUE
enum {
	ID_SPOT = 1,
	ID_SEL_SPOT
};

