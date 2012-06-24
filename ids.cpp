// implementation of id class
#include "stdafx.h"
#include "ids.h"
#include "Cuid.h"
#include "FreePcbDoc.h"

// global pointer to the document
CFreePcbDoc * pcb;

// constructor
id::id( int qt1, int qu1,
	int qt2, int qu2, int qi2, 
	int qt3, int qu3,int qi3 ) 
{ 
	pcb = ((CFreePcbApp*)AfxGetApp())->m_Doc;
	ptr = NULL;
	t1 = qt1; 
	u1 = qu1;
	t2 = qt2; 
	u2 = qu2;
	i2 = qi2; 
	t3 = qt3; 
	u3 = qu3;
	i3 = qi3;
}

// == operator, allows wildcards, except for T1
BOOL operator ==(const id &id1, const id &id2)
{ 
	BOOL test = (
		     id1.t1 == id2.t1 
		&& ( id1.u1 == id2.u1	|| id1.u1 == -1			|| id2.u1  == -1 )
		&& ( id1.t2 == id2.t2	|| id1.t2 == ID_NONE	|| id2.t2 == ID_NONE ) 
		&& ( id1.u2 == id2.u2	|| id1.u2 == -1			|| id2.u2 == -1 )
		&& ( id1.i2 == id2.i2	|| id1.i2 == -1			|| id2.i2 == -1 )
		&& ( id1.t3 == id2.t3	|| id1.t3 == ID_NONE	|| id2.t3 == ID_NONE ) 
		&& ( id1.u3 == id2.u3	|| id1.u3 == -1			|| id2.u3 == -1 )
		&& ( id1.i3 == id2.i3	|| id1.i3 == -1			|| id2.i3 == -1 )
	); 
	return test;
}

// member functions

// clear all fields
//
void id::Clear() 
{ 
	Set(); 
	SetPtr( NULL );
}

// Try to identify the item described by this id
// Set pointer to top level item
// If the id describes an indexed item, set the indexes
// Returns TRUE if successful
//
BOOL id::Resolve()
{
	i2 = -1;
	i3 = -1;
	ptr = NULL;
	if( IsCon() )
	{
		if( Net() == NULL )
			return FALSE;
		ptr = (void*)Net();
		cconnect * c = Net()->ConByUID( u2, &i2 );
		if( c == NULL )
			return FALSE;
	}
	else if( IsSeg() )
	{
		if( Net() == NULL )
			return FALSE;
		ptr = (void*)Net();
		cconnect * c = Net()->ConByUID( u2, &i2 );
		if( c == NULL )
			return FALSE;
		c->SegByUID( u3, &i3 );
		if( i3 == -1 )
			return FALSE;
	}
	else if( IsVtx() )
	{
		if( Net() == NULL )
			return FALSE;
		ptr = (void*)Net();
		cconnect * c = Net()->ConByUID( u2, &i2 );
		if( c == NULL )
			return FALSE;
		c->VtxByUID( u3, &i3 );
		if( i3 == -1 )
			return FALSE;
	}
	else if( IsArea() )
	{
		if( Net() == NULL )
			return FALSE;
		ptr = (void*)Net();
		Net()->AreaByUID( u2, &i2 );
		if( i2 == -1 )
			return FALSE;
	}
	else if( IsAreaCorner() )
	{
		if( Net() == NULL )
			return FALSE;
		carea * a = Net()->AreaByUID( u2, &i2 );
		if( a == NULL )
			return FALSE;
		i3 = a->CornerIndexByUID( u3 );
		if( i3 == -1 )
			return FALSE;
	}
	else if( IsAreaSide() )
	{
		if( Net() == NULL )
			return FALSE;
		ptr = (void*)Net();
		carea * a = Net()->AreaByUID( u2, &i2 );
		if( a == NULL )
			return FALSE;
		i3 = a->SideIndexByUID( u3 );
		if( i3 == -1 )
			return FALSE;
	}
	else if( IsPart() || IsRefText() || IsValueText() )
	{
		if( Part() == NULL )
			return FALSE;
		ptr = (void*)Part();
	}
	else if( IsPin() )
	{
		if( Part() == NULL )
			return FALSE;
		ptr = (void*)Part();
		part_pin * pin = Part()->PinByUID( u2, &i2 );
		if( i2 == -1 )
			return FALSE;
	}
	else if( IsBoard() )
	{
		CPolyLine * poly = pcb->GetBoardOutlineByUID( u2, &i2 );
		if( poly == NULL )
			return FALSE;
		ptr = (void*)poly;
	}
	else if( IsBoardCorner() )
	{
		CPolyLine * poly = pcb->GetBoardOutlineByUID( u2, &i2 );
		if( poly == NULL )
			return FALSE;
		ptr = (void*)poly;
		i3 = poly->CornerIndexByUID( u3 );
		if( i3 == -1 )
			return FALSE;
	}
	else if( IsBoardSide() )
	{
		CPolyLine * poly = pcb->GetBoardOutlineByUID( u2, &i2 );
		if( poly == NULL )
			return FALSE;
		ptr = (void*)poly;
		i3 = poly->SideIndexByUID( u3 );
		if( i2 == -1 || i3 == -1 )
			return FALSE;
	}
	else if( IsMask() )
	{
		CPolyLine * poly = pcb->GetMaskCutoutByUID( u2, &i2 );
		if( poly == NULL )
			return FALSE;
		ptr = (void*)poly;
	}
	else if( IsMaskCorner() )
	{
		CPolyLine * poly = pcb->GetMaskCutoutByUID( u2, &i2 );
		if( poly == NULL )
			return FALSE;
		ptr = (void*)poly;
		i3 = poly->CornerIndexByUID( u3 );
		if( i3 == -1 )
			return FALSE;
	}
	else if( IsMaskSide() )
	{
		CPolyLine * poly = pcb->GetMaskCutoutByUID( u2, &i2 );
		if( poly == NULL )
			return FALSE;
		i3 = poly->SideIndexByUID( u3 );
		ptr = (void*)poly;
		if( i3 == -1 )
			return FALSE;
	}
	else if( IsText() )
	{
		CText * text = pcb->m_tlist->GetText( u1 );
		if( text == NULL )
			return FALSE;
		ptr = (void*)text;
	}
	else
	{
		return FALSE;	// unidentified
	}
	return TRUE;
}

// get individual fields
//
void * id::Ptr(){ return ptr; };
int id::T1(){ return t1; };
int id::U1(){ return u1; };
int id::T2(){ return t2; };
int id::U2(){ return u2; };
int id::I2(){ return i2; };
int id::T3(){ return t3; };
int id::U3(){ return u3; };
int id::I3(){ return i3; };

// set individual fields
//
void id::SetPtr( void * p ){ ptr = p; };
void id::SetT1( int qt1 ){ t1 = qt1; ptr = NULL; }
void id::SetU1( int qu1 ){ u1 = qu1; ptr = NULL; }
void id::SetT2( int qt2 ){ t2 = qt2; ptr = NULL; }
void id::SetU2( int qu2 ){ u2 = qu2; ptr = NULL; }
void id::SetI2( int qi2 ){ i2 = qi2; ptr = NULL; }
void id::SetT3( int qt3 ){ t3 = qt3; ptr = NULL; }
void id::SetU3( int qu3 ){ u3 = qu3; ptr = NULL; }
void id::SetI3( int qi3 ){ i3 = qi3; ptr = NULL; }

// set all fields
//
void id::Set(int qt1, int qu1, 
			int qt2, int qu2, int qi2, 
			int qt3, int qu3, int qi3  )
{ 
	ptr = NULL;
	if( qt1 != NOP ) t1 = qt1; 
	if( qu1 != NOP ) u1 = qu1;
	if( qt2 != NOP ) t2 = qt2; 
	if( qu2 != NOP ) u2 = qu2;
	if( qi2 != NOP ) i2 = qi2; 
	if( qt3 != NOP ) t3 = qt3; 
	if( qu3 != NOP ) u3 = qu3;
	if( qi3 != NOP ) i3 = qi3; 
} 

// set lower levels of hierarchy
//
void id::SetSubType( int qt2, int qu2, int qi2 )
{
	ptr = NULL;
	t2 = qt2; 
	u2 = qu2;
	i2 = qi2; 
}

void id::SetSubSubType( int qt3, int qu3, int qi3 )
{
	ptr = NULL;
	t3 = qt3; 
	u3 = qu3;
	i3 = qi3; 
}


// identify the lowest-level type of item selected
//
int id::Type()
{
	if( t1 > 0 && t2 > 0 && t3 > 0 ) 
		return t3;
	else if( t1 > 0 && t2 > 0 ) 
		return t2;
	else 
		return t1;
}

// tests for various types, doesn't verify UIDs
//
BOOL id::IsClear()		{ return t1 == ID_NONE	&& t2 == ID_NONE	&& t3 == ID_NONE;		}
BOOL id::IsDRC()		{ return t1 == ID_DRC	&& t2 == ID_SEL_DRE;						}
BOOL id::IsText()		{ return t1 == ID_TEXT	&& t2 == ID_TEXT;							}
BOOL id::IsPart()		{ return t1 == ID_PART	&& t2 == ID_SEL_RECT;						}
BOOL id::IsRefText()	{ return t1 == ID_PART	&& t2 == ID_REF_TXT;						}
BOOL id::IsValueText()	{ return t1 == ID_PART	&& t2 == ID_VALUE_TXT;						}
BOOL id::IsPin()		{ return t1 == ID_PART	&& t2 == ID_SEL_PAD;						}
BOOL id::IsNet()		{ return t1 == ID_NET	&& t2 == ID_NONE;							}
BOOL id::IsCon()		{ return t1 == ID_NET	&& t2 == ID_CONNECT	&& t3 == ID_NONE;		}
BOOL id::IsSeg()		{ return t1 == ID_NET	&& t2 == ID_CONNECT	&& t3 == ID_SEL_SEG;	}
BOOL id::IsVtx()		{ return t1 == ID_NET	&& t2 == ID_CONNECT	&& t3 == ID_SEL_VERTEX; }
BOOL id::IsArea()		{ return t1 == ID_NET	&& t2 == ID_AREA	&& t3 == ID_NONE;		}
BOOL id::IsAreaCorner()	{ return t1 == ID_NET	&& t2 == ID_AREA	&& t3 == ID_SEL_CORNER; }
BOOL id::IsAreaSide()	{ return t1 == ID_NET	&& t2 == ID_AREA	&& t3 == ID_SEL_SIDE;	}
BOOL id::IsBoard()		{ return t1 == ID_BOARD && t2 == ID_OUTLINE	&& t3 == ID_NONE;		}
BOOL id::IsBoardCorner(){ return t1 == ID_BOARD && t2 == ID_OUTLINE	&& t3 == ID_SEL_CORNER; }
BOOL id::IsBoardSide()	{ return t1 == ID_BOARD && t2 == ID_OUTLINE	&& t3 == ID_SEL_SIDE;	}
BOOL id::IsMask()		{ return t1 == ID_MASK	&& t2 == ID_OUTLINE	&& t3 == ID_NONE;		}
BOOL id::IsMaskCorner()	{ return t1 == ID_MASK	&& t2 == ID_OUTLINE	&& t3 == ID_SEL_CORNER; }
BOOL id::IsMaskSide()	{ return t1 == ID_MASK	&& t2 == ID_OUTLINE	&& t3 == ID_SEL_SIDE;	}
BOOL id::IsAnyFootItem() { return t1 == ID_FP;												}
BOOL id::IsCentroid()	{ return t1 == ID_FP	&& t2 == ID_CENTROID && t3 == ID_SEL_CENT;	}
BOOL id::IsGlue()		{ return t1 == ID_FP	&& t2 == ID_GLUE	 && t3 == ID_SEL_SPOT;	}
BOOL id::IsFootPad()	{ return t1 == ID_FP	&& t2 == ID_SEL_PAD;						}
BOOL id::IsFootText()	{ return t1 == ID_FP	&& t2 == ID_FP_TXT	 && t3 == ID_SEL_TXT;	}
BOOL id::IsFootPolySide()   { return t1 == ID_FP	&& t2 == ID_POLYLINE && t3 == ID_SEL_SIDE;	}					
BOOL id::IsFootPolyCorner() { return t1 == ID_FP	&& t2 == ID_POLYLINE && t3 == ID_SEL_CORNER; }					
BOOL id::IsFootRef()	{ return t1 == ID_FP	&& t2 == ID_REF_TXT; } // CPT
BOOL id::IsFootValue()	{ return t1 == ID_FP	&& t2 == ID_VALUE_TXT; } // CPT

// return pointer to pcb element identified by id
//
cpart * id::Part()
{
#ifndef CPT2
	if( t1 == ID_PART )
	{
		return pcb->m_plist->GetPartByUID( u1 );
	}
#endif
	return NULL;
}

part_pin * id::Pin()
{
#ifndef CPT2
	if( t1 == ID_PART && t2 == ID_SEL_PAD )
	{
		cpart * part =  pcb->m_plist->GetPartByUID( u1 );
		return part->PinByUID( u2 );
	}
#endif
	return NULL;
}

cnet * id::Net()
{
#ifndef CPT2
	if( t1 == ID_NET )
	{
		cnet * net =  pcb->m_nlist->GetNetPtrByUID( u1 );
		return net;
	}
	else if( IsPin() )
	{
		return Pin()->net;
	}
#endif
	return NULL;
}

cconnect * id::Con()
{
#ifndef CPT2
	if( t1 == ID_NET && t2 == ID_CONNECT )
	{
		cnet * net =  pcb->m_nlist->GetNetPtrByUID( u1 );
		return net->ConByUID( u2 );
	}
#endif
	return NULL;
}

cseg * id::Seg()
{
#ifndef CPT2

	if( t1 == ID_NET && t2 == ID_CONNECT && t3 == ID_SEL_SEG )
	{
		cnet * net =  pcb->m_nlist->GetNetPtrByUID( u1 );
		cconnect * c = net->ConByUID( u2 );
		return c->SegByUID( u3 );
	}
#endif
	return NULL;
}

cvertex * id::Vtx()
{
#ifndef CPT2

	if( t1 == ID_NET && t2 == ID_CONNECT && t3 == ID_SEL_VERTEX )
	{
		cnet * net =  pcb->m_nlist->GetNetPtrByUID( u1 );
		cconnect * c = net->ConByUID( u2 );
		return c->VtxByUID( u3 );
	}
#endif
	return NULL;
}

carea * id::Area()
{
#ifndef CPT2
	if( t1 == ID_NET && t2 == ID_AREA )
	{
		cnet * net =  pcb->m_nlist->GetNetPtrByUID( u1 );
		return net->AreaByUID( u2 );
	}
#endif
	return NULL;
}

CText * id::Text()
{
	if( t1 == ID_TEXT )
	{
		return pcb->m_tlist->GetText( u1 );
	}
	return NULL;
}

CPolyLine * id::BoardOutline()
{
	if( t1 == ID_BOARD && t2 == ID_OUTLINE )
	{
		return pcb->GetBoardOutlineByUID( u2 );
	}
	return NULL;
}

CPolyLine * id::MaskOutline()
{
	if( t1 == ID_MASK && t2 == ID_OUTLINE )
	{
		return pcb->GetMaskCutoutByUID( u2 );
	}
	return NULL;
}

