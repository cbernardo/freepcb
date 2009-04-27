#include "stdafx.h"
#include "partlist.h"

//******** *terator for cnet (CIterator_cnet) *********
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
