#include "stdafx.h"
#include "LinkList.h"

//******** Iterator for cnet (CIterator_cnet) *********
CDLinkList CIterator_cnet::m_LIST_Iterator;

// Constructor
CIterator_cnet::CIterator_cnet( CNetList const * netlist )
	: m_NetList(netlist)
	, m_pCurrentNet(NULL)
{
	m_LIST_Iterator.insert_after(this);
}

// Iterator operators: First/Next
cnet *CIterator_cnet::GetFirst()
{
	m_pCurrentNet = NULL;

	// test for no nets
	if( m_NetList->m_map.GetSize() != 0 )
	{
		// increment iterator and get first net
		m_CurrentPos = m_NetList->m_map.GetStartPosition();

		return GetNext();
	}

	return m_pCurrentNet;
}

cnet *CIterator_cnet::GetNext()
{
	if( m_CurrentPos != NULL )
	{
		CString name;
		void * ptr;

		m_NetList->m_map.GetNextAssoc( m_CurrentPos, name, ptr );

		m_pCurrentNet = (cnet*)ptr;
		ASSERT( m_pCurrentNet != NULL );
	}
	else
	{
		m_pCurrentNet = NULL;
	}

	return m_pCurrentNet;
}


// OnRemove() must be called when removing/deleting net
// to update any active iterators.
void CIterator_cnet::OnRemove( cnet const *net )
{
	// NOP
}

//******** Iterator for cconnect (CIterator_cconnect) *********
CDLinkList CIterator_cconnect::m_LIST_Iterator;

// Constructor
CIterator_cconnect::CIterator_cconnect( cnet * net )
//	: m_NetList(netlist)
//	, m_pCurrentNet(NULL)
{
	m_net = net;
//**	int n = m_LIST_Iterator.GetListSize();
	m_LIST_Iterator.insert_after(this);
}

// Destructor
CIterator_cconnect::~CIterator_cconnect()
{
//**	int n = m_LIST_Iterator.GetListSize();
}

// Iterator operators: First/Next
cconnect *CIterator_cconnect::GetFirst()
{
	m_CurrentPos = -1;	// -1 = position before first connection
	m_pCurrentConnection = NULL;

	// test for no connections
	if( m_net->NumCons() != 0 )
	{
		// increment iterator and get first net
		GetNext();
	}

	return m_pCurrentConnection;
}

cconnect *CIterator_cconnect::GetNext()
{
	m_CurrentPos++;
	if( m_CurrentPos < m_net->NumCons() )
	{
		m_pCurrentConnection = m_net->connect[m_CurrentPos];
	}
	else
	{
		m_pCurrentConnection = NULL;
	}

	return m_pCurrentConnection;
}

// OnRemove() must be called before removing/deleting cconnect
// to update any active iterators.
void CIterator_cconnect::OnRemove( int ic )
{
	// For every iterator, adjust the "current connection" if that 
	// connection or earlier is being removed
	for( CDLinkList *pElement = m_LIST_Iterator.next; pElement != &m_LIST_Iterator; pElement = pElement->next )
	{
		CIterator_cconnect *pIterator = static_cast<CIterator_cconnect *>(pElement);

		if( ic <= pIterator->m_CurrentPos )
		{
			// Make adjustment so that the next GetNext() moves to 
			// the connection after the one removed.
			pIterator->m_CurrentPos--;
		}
	}
}

void CIterator_cconnect::OnRemove( cconnect * con )
{
	// get index of connection being removed
	int remove_ic = -1;
	for( int ic=0; ic<m_net->NumCons(); ic++ )
	{
		if( con == m_net->connect[ic] )
		{
			remove_ic = ic;
			break;
		}
	}
	if( remove_ic == -1 )
		ASSERT(0);

	OnRemove( remove_ic );
}

//******** Iterator for cseg (CIterator_cseg) *********
CDLinkList CIterator_cseg::m_LIST_Iterator;

// Constructor
CIterator_cseg::CIterator_cseg( cconnect * con )
//	: m_NetList(netlist)
//	, m_pCurrentNet(NULL)
{
	m_cconnect = con;
	m_LIST_Iterator.insert_after(this);
}

// Iterator operators: First/Next
cseg *CIterator_cseg::GetFirst()
{
	// initialize position to before first segment
	m_CurrentPos = -1;	// -1 = position before first seg
	m_PreviousPos = -1;	// -1 = position before first seg
	m_NextPos = 0;		// 0 = index of first seg
	// get first segment
	return( GetNext() );
}

cseg *CIterator_cseg::GetNext()
{
	m_pCurrentSegment = NULL;
	int ns = m_cconnect->NumSegs();
	if( ns == 0 )
	{
		// no segments in array
		m_CurrentPos = -1;
		m_PreviousPos = -1;
		m_NextPos = -1;
	}
	else if( m_NextPos == -1 )
	{
		// next doesn't exist
		m_CurrentPos = -1;
		m_PreviousPos = m_CurrentPos;
		m_NextPos = -1;
	}
	else
	{
		// next exists
		if( m_NextPos >= ns )
		{
			// make sure legal
			ASSERT(0);		// should never happen
		}
		else
		{
			m_CurrentPos = m_NextPos;
			m_PreviousPos = m_CurrentPos - 1;
			m_NextPos = m_CurrentPos + 1;
		}
	}
	// make sure m_NextPos is legal
	if( m_NextPos >= ns )
	{
		m_NextPos = -1;
	}
	if (m_CurrentPos >= 0 )
	{
		// set pointer
		m_pCurrentSegment = &m_cconnect->SegByIndex( m_CurrentPos );
	}
	return m_pCurrentSegment; 
}

// OnRemove(is) must be called when removing a cseg
// to update any active iterators.
void CIterator_cseg::OnRemove( int is )
{
	// look at all iterators for this connection 
	for( CDLinkList *pElement = m_LIST_Iterator.next; pElement != &m_LIST_Iterator; pElement = pElement->next )
	{
		CIterator_cseg *pIterator = static_cast<CIterator_cseg *>(pElement);
		if( m_cconnect == pIterator->m_cconnect )
		{
			// same connection
			int new_num_segs = m_cconnect->NumSegs() - 1;  // number of segments after remove
			if( new_num_segs <= 0 )
			{
				// there will be no segments left, invalidate all
				pIterator->m_CurrentPos = -1;
				pIterator->m_NextPos = -1;
				pIterator->m_PreviousPos = -1;
			}

			if( is <= pIterator->m_CurrentPos && m_cconnect == pIterator->m_cconnect )
			{
				// Make adjustments for removal
				if( is < pIterator->m_CurrentPos )
				{
					// removing previous segment, shift all down
					pIterator->m_NextPos = pIterator->m_CurrentPos;
					pIterator->m_CurrentPos = pIterator->m_PreviousPos;
					pIterator->m_PreviousPos--;
				}
				else if( is == pIterator->m_CurrentPos )
				{
					// removing current segment, shift next down 
					pIterator->m_CurrentPos = -1;
					pIterator->m_NextPos--;
				}
			}
			// make sure next still legal
			if( pIterator->m_NextPos >= new_num_segs )
			{
				pIterator->m_NextPos = -1;
			}
		}
	}
}

// OnRemove(seg) must be called BEFORE removing a cseg
// to update any active iterators.
void CIterator_cseg::OnRemove( cseg * seg )
{
	// get index of segion being removed
	int remove_is = -1;
	for( int is=0; is<m_cconnect->NumSegs(); is++ )
	{
		if( seg == &m_cconnect->SegByIndex(is) )
		{
			remove_is = is;
			break;
		}
	}
	if( remove_is == -1 )
		ASSERT(0);

	OnRemove( remove_is );

}

//******** Iterator for cvertex (CIterator_cvertex) *********
CDLinkList CIterator_cvertex::m_LIST_Iterator;

// Constructor
CIterator_cvertex::CIterator_cvertex( cconnect * con )
//	: m_NetList(netlist)
//	, m_pCurrentNet(NULL)
{
	m_cconnect = con;
	m_LIST_Iterator.insert_after(this);
}

// Iterator operators: First/Next
cvertex *CIterator_cvertex::GetFirst()
{
	m_CurrentPos = -1;	// -1 = position before first vertex
	m_pCurrentVertex = NULL;

	// test for no vertices
	if( m_cconnect->NumSegs() != 0 )
	{
		// increment iterator and get first net
		GetNext();
	}

	return m_pCurrentVertex;
}

cvertex *CIterator_cvertex::GetNext()
{
	m_CurrentPos++;
	if( m_CurrentPos <= m_cconnect->NumSegs() )
	{
		m_pCurrentVertex = &m_cconnect->VtxByIndex(m_CurrentPos);
	}
	else
	{
		m_pCurrentVertex = NULL;
	}

	return m_pCurrentVertex;
}

// OnRemove(iv) must be called when removing/deleting cvertex
// to update any active iterators.
void CIterator_cvertex::OnRemove( int iv )
{
	// For every iterator, adjust the "current vertex" if that 
	// vertex or earlier is being removed
	for( CDLinkList *pElement = m_LIST_Iterator.next; pElement != &m_LIST_Iterator; pElement = pElement->next )
	{
		CIterator_cvertex *pIterator = static_cast<CIterator_cvertex *>(pElement);

		if( iv <= pIterator->m_CurrentPos && m_cconnect == pIterator->m_cconnect )
		{
			// Make adjustment so that the next GetNext() moves to 
			// the vertex after the one removed.
			pIterator->m_pCurrentVertex--;
		}
	}
}

// OnRemove(vtx) must be called BEFORE removing/deleting cvertex
// to update any active iterators.
void CIterator_cvertex::OnRemove( cvertex * vtx )
{
	// get index of vertex being removed
	int remove_iv = -1;
	for( int iv=0; iv<=m_cconnect->NumSegs(); iv++ )
	{
		if( vtx == &m_cconnect->VtxByIndex(iv) )
		{
			remove_iv = iv;
			break;
		}
	}
	if( remove_iv == -1 )
		ASSERT(0);

	OnRemove( remove_iv );
}

//******** Iterator for cpin (CIterator_cpin) *********
CDLinkList CIterator_cpin::m_LIST_Iterator;

// Constructor
CIterator_cpin::CIterator_cpin( cnet * net )
//	: m_NetList(netlist)
//	, m_pCurrentNet(NULL)
{
	m_net = net;
//**	int n = m_LIST_Iterator.GetListSize();
	m_LIST_Iterator.insert_after(this);
}

// Destructor
CIterator_cpin::~CIterator_cpin()
{
//**	int n = m_LIST_Iterator.GetListSize();
}

// Iterator operators: First/Next
cpin *CIterator_cpin::GetFirst()
{
	m_CurrentPos = -1;	// -1 = position before first connection
	m_pCurrentConnection = NULL;

	// test for no connections
	if( m_net->NumPins() != 0 )
	{
		// increment iterator and get first net
		GetNext();
	}

	return m_pCurrentConnection;
}

cpin *CIterator_cpin::GetNext()
{
	m_CurrentPos++;
	if( m_CurrentPos < m_net->NumPins() )
	{
		m_pCurrentConnection = &m_net->pin[m_CurrentPos];
	}
	else
	{
		m_pCurrentConnection = NULL;
	}

	return m_pCurrentConnection;
}

// OnRemove() must be called before removing/deleting cpin
// to update any active iterators.
void CIterator_cpin::OnRemove( int ip )
{
	// For every iterator, adjust the "current connection" if that 
	// connection or earlier is being removed
	for( CDLinkList *pElement = m_LIST_Iterator.next; pElement != &m_LIST_Iterator; pElement = pElement->next )
	{
		CIterator_cpin *pIterator = static_cast<CIterator_cpin *>(pElement);

		if( ip <= pIterator->m_CurrentPos )
		{
			// Make adjustment so that the next GetNext() moves to 
			// the connection after the one removed.
			pIterator->m_CurrentPos--;
		}
	}
}

void CIterator_cpin::OnRemove( cpin * pin )
{
	// get index of connection being removed
	int remove_ip = -1;
	for( int ip=0; ip<m_net->NumPins(); ip++ )
	{
		if( pin == &m_net->pin[ip] )
		{
			remove_ip = ip;
			break;
		}
	}
	if( remove_ip == -1 )
		ASSERT(0);

	OnRemove( remove_ip );
}

//******** Iterator for carea (CIterator_carea) *********
CDLinkList CIterator_carea::m_LIST_Iterator;

// Constructor
CIterator_carea::CIterator_carea( cnet * net )
//	: m_NetList(netlist)
//	, m_pCurrentNet(NULL)
{
	m_net = net;
//**	int n = m_LIST_Iterator.GetListSize();
	m_LIST_Iterator.insert_after(this);
}

// Destructor
CIterator_carea::~CIterator_carea()
{
//**	int n = m_LIST_Iterator.GetListSize();
}

// Iterator operators: First/Next
carea *CIterator_carea::GetFirst()
{
	m_CurrentPos = -1;	// -1 = position before first connection
	m_pCurrentConnection = NULL;

	// test for no connections
	if( m_net->NumAreas() != 0 )
	{
		// increment iterator and get first net
		GetNext();
	}

	return m_pCurrentConnection;
}

carea *CIterator_carea::GetNext()
{
	m_CurrentPos++;
	if( m_CurrentPos < m_net->NumAreas() )
	{
		m_pCurrentConnection = &m_net->area[m_CurrentPos];
	}
	else
	{
		m_pCurrentConnection = NULL;
	}

	return m_pCurrentConnection;
}

// OnRemove() must be called before removing/deleting carea
// to update any active iterators.
void CIterator_carea::OnRemove( int ia )
{
	// For every iterator, adjust the "current connection" if that 
	// connection or earlier is being removed
	for( CDLinkList *pElement = m_LIST_Iterator.next; pElement != &m_LIST_Iterator; pElement = pElement->next )
	{
		CIterator_carea *pIterator = static_cast<CIterator_carea *>(pElement);

		if( ia <= pIterator->m_CurrentPos )
		{
			// Make adjustment so that the next GetNext() moves to 
			// the connection after the one removed.
			pIterator->m_CurrentPos--;
		}
	}
}

void CIterator_carea::OnRemove( carea * area )
{
	// get index of connection being removed
	int remove_ia = -1;
	for( int ia=0; ia<m_net->NumAreas(); ia++ )
	{
		if( area == &m_net->area[ia] )
		{
			remove_ia = ia;
			break;
		}
	}
	if( remove_ia == -1 )
		ASSERT(0);

	OnRemove( remove_ia );
}
