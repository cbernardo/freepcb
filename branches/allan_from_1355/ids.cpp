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
	t1 = qt1; 
	u1 = qu1;
	t2 = qt2; 
	u2 = qu2;
	i2 = qi2; 
	t3 = qt3; 
	u3 = qu3;
	i3 = qi3; 
}

// == operator, allows wildcards
BOOL operator ==(id id1, id id2)
{ return (
		( id1.T1() == id2.t1 || id1.T1() == ID_NONE || id2.T1() == ID_NONE ) 
		&& ( id1.U1()  == id2.U1()  || id1.U1()  == -1 || id2.U1()  == -1 )
		&& ( id1.T2() == id2.t2 || id1.T2() == ID_NONE || id2.T2() == ID_NONE ) 
		&& ( id1.U2() == id2.U2() || id1.U2() == -1 || id2.U2() == -1 )
		&& ( id1.I2() == id2.I2() || id1.I2() == -1 || id2.I2() == -1 )
		&& ( id1.T3() == id2.t3 || id1.T3() == ID_NONE || id2.T3() == ID_NONE ) 
		&& ( id1.U3() == id2.U3() || id1.U3() == -1 || id2.U3() == -1 )
		&& ( id1.I3() == id2.I3() || id1.I3() == -1 || id2.I3() == -1 )

		&& (id1.t2==id2.t2) 
		&& (id1.t3==id2.t3) 
		&& id1.I2()==id2.I2() 
		&& id1.I3()==id2.I3() 
		&& id1.U1() ==id2.U1() 
		&& id1.U2()==id2.U2() 
		&& id1.U3()==id2.U3() 
		); 
}

// member functions

// clear all fields
//
void id::Clear() 
{ 
	t1 = 0; u1 = -1; 
	t2 = 0; u2 = -1; i2 = -1; 
	t3 = 0; u3 = -1; i3 = -1; 		
}

// if the id describes an indexed item, get the indexes
//
void id::SetIndexes()
{
	if( IsCon() )
	{
		i2 = Net()->ConIndexByUID( u2 );
	}
	else if( IsSeg() )
	{
		cnet * net = Net();
		cconnect * c = Con();
		i2 = net->ConIndexByUID( u2 );
		c->SegByUID( u3, &i3 );
	}
	else if( IsVtx() )
	{
		i2 = Net()->ConIndexByUID( u2 );
		Con()->VtxByUID( u3, &i3 );
	}
	else if( IsArea() )
	{
		Net()->AreaByUID( u2, &i2 );
	}
	else if( IsAreaCorner() )
	{
		carea * a = Net()->AreaByUID( u2, &i2 );
		i3 = a->poly->GetCornerIndexByUID( u3 );
	}
	else if( IsAreaSide() )
	{
		carea * a = Net()->AreaByUID( u2, &i2 );
		i3 = a->poly->GetSideIndexByUID( u3 );
	}
	else if( IsPin() )
	{
	}
	else if( IsBoard() )
	{
		pcb->GetBoardOutlineByUID( u2, &i2 );
	}
	else if( IsBoardCorner() )
	{
		CPolyLine * poly = pcb->GetBoardOutlineByUID( u2, &i2 );
		i3 = poly->GetCornerIndexByUID( u3 );
	}
	else if( IsBoardSide() )
	{
		CPolyLine * poly = pcb->GetBoardOutlineByUID( u2, &i2 );
		i3 = poly->GetSideIndexByUID( u3 );
	}
	else if( IsMask() )
	{
		pcb->GetMaskCutoutByUID( u2, &i2 );
	}
	else if( IsMaskCorner() )
	{
		CPolyLine * poly = pcb->GetMaskCutoutByUID( u2, &i2 );
		i3 = poly->GetCornerIndexByUID( u3 );
	}
	else if( IsMaskSide() )
	{
		CPolyLine * poly = pcb->GetMaskCutoutByUID( u2, &i2 );
		i3 = poly->GetSideIndexByUID( u3 );
	}
}

// get individual fields
//
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
void id::SetT1( int qt1 ){ t1 = qt1; }
void id::SetU1( int qu1 ){ u1 = qu1; }
void id::SetT2( int qt2 ){ t2 = qt2; }
void id::SetU2( int qu2 ){ u2 = qu2; }
void id::SetI2( int qi2 ){ i2 = qi2; }
void id::SetT3( int qt3 ){ t3 = qt3; }
void id::SetU3( int qu3 ){ u3 = qu3; }
void id::SetI3( int qi3 ){ i3 = qi3; }

// set all fields
//
void id::Set(int qt1, int qu1, 
			int qt2, int qu2, int qi2, 
			int qt3, int qu3, int qi3  )
{ 
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
void id::SetLevel2( int qt2, int qu2, int qi2 )
{
	t2 = qt2; 
	u2 = qu2;
	i2 = qi2; 
}

void id::SetLevel3( int qt3, int qu3, int qi3 )
{
	t3 = qt3; 
	u3 = qu3;
	i3 = qi3; 
}


// identify the lowest-level type of item selected
//
int id::Type()
{
	if( t3 > 0 ) 
		return t3;
	else if( t2 > 0 ) 
		return t2;
	else 
		return t1;
}

// get lowest level type
BOOL id::IsText(){ return t2 == ID_TEXT && t2 == ID_TEXT; }
BOOL id::IsPart(){ return t1 == ID_PART && t2 == ID_NONE; }
BOOL id::IsRefText(){ return t2 == ID_REF_TXT && t3 == ID_SEL_TXT; }
BOOL id::IsValueText(){ return t2 == ID_REF_TXT && t3 == ID_SEL_TXT; }
BOOL id::IsPin(){ return Type() == ID_SEL_PAD; }
BOOL id::IsNet(){ return t1 == ID_NET && t2 == ID_NONE; }
BOOL id::IsCon(){ return t2 == ID_CONNECT && t3 == ID_NONE; }
BOOL id::IsSeg(){ return t3 == ID_SEL_SEG; }
BOOL id::IsVtx(){ return t3 == ID_SEL_SEG; }
BOOL id::IsArea(){ return t2 == ID_REF_TXT && t3 == ID_NONE; }
BOOL id::IsAreaCorner(){ return t2 == ID_AREA && t3 == ID_SEL_CORNER; }
BOOL id::IsAreaSide(){ return t2 == ID_AREA && t3 == ID_SEL_SIDE; }
BOOL id::IsBoard(){ return ID_BOARD_OUTLINE && t3 == ID_NONE; }
BOOL id::IsBoardCorner(){ return t2 == ID_BOARD_OUTLINE && t3 == ID_SEL_CORNER; }
BOOL id::IsBoardSide(){ return  t2 == ID_BOARD_OUTLINE && t3 == ID_SEL_SIDE; }
BOOL id::IsMask(){ return t2 == ID_MASK_OUTLINE && t3 == ID_NONE; }
BOOL id::IsMaskCorner(){ return t2 == ID_MASK_OUTLINE && t3 == ID_SEL_CORNER; }
BOOL id::IsMaskSide(){ return t2 == ID_MASK_OUTLINE && t3 == ID_SEL_SIDE; }

// return pointer to pcb element identified by id
//
cpart * id::Part()
{
	if( t1 == ID_PART )
	{
		return pcb->m_plist->GetPartByUID( u1 );
	}
	return NULL;
}

part_pin * id::Pin()
{
	if( t1 == ID_PART && t2 == ID_SEL_PAD )
	{
		cpart * part =  pcb->m_plist->GetPartByUID( u1 );
		return part->PinByUID( u2 );
	}
	return NULL;
}

cnet * id::Net()
{
	if( t1 == ID_NET )
	{
		cnet * net =  pcb->m_nlist->GetNetPtrByUID( u1 );
		return net;
	}
	return NULL;
}

cconnect * id::Con()
{
	if( t1 == ID_NET && t2 == ID_CONNECT )
	{
		cnet * net =  pcb->m_nlist->GetNetPtrByUID( u1 );
		return net->ConByUID( u2 );
	}
	return NULL;
}

cseg * id::Seg()
{
	if( t1 == ID_NET && t2 == ID_CONNECT && t3 == ID_SEL_SEG )
	{
		cnet * net =  pcb->m_nlist->GetNetPtrByUID( u1 );
		cconnect * c = net->ConByUID( u2 );
		return c->SegByUID( u3 );
	}
	return NULL;
}

cvertex * id::Vtx()
{
	if( t1 == ID_NET && t2 == ID_CONNECT && t3 == ID_SEL_VERTEX )
	{
		cnet * net =  pcb->m_nlist->GetNetPtrByUID( u1 );
		cconnect * c = net->ConByUID( u2 );
		return c->VtxByUID( u3 );
	}
	return NULL;
}

carea * id::Area()
{
	if( t1 == ID_NET && t2 == ID_AREA )
	{
		cnet * net =  pcb->m_nlist->GetNetPtrByUID( u1 );
		return net->AreaByUID( u2 );
	}
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
	if( t1 == ID_BOARD && t2 == ID_BOARD_OUTLINE )
	{
		return pcb->GetBoardOutlineByUID( u2 );
	}
	return NULL;
}

CPolyLine * id::MaskOutline()
{
	if( t1 == ID_MASK && t2 == ID_MASK_OUTLINE )
	{
		return pcb->GetMaskCutoutByUID( u2 );
	}
	return NULL;
}

