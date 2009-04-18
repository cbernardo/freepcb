#include "stdafx.h"
#include "partlist.h"

//******** *terator for cpart (CIterator_cpart) *********
CDLinkList CIterator_cpart::m_LIST_Iterator;

// Constructor
CIterator_cpart::CIterator_cpart( CPartList const * partlist )
{
	m_PartList = partlist;

	// Set the current part to the placeholder node in partlist.
	// The const cast is ok since this element will never be
	// changed.
	m_pCurrentPart = const_cast<CDLinkList *>(&m_PartList->m_LIST_part);

	m_LIST_Iterator.insert_after(this);
}

// Iterator operators: First/Next
cpart *CIterator_cpart::GetFirst()
{
	// Set the current part to the placeholder node in partlist.
	// The const cast is ok since this element will never be
	// changed.
	m_pCurrentPart = const_cast<CDLinkList *>(&m_PartList->m_LIST_part);

	return GetNext();
}

cpart *CIterator_cpart::GetNext()
{
	if( m_pCurrentPart->next == &m_PartList->m_LIST_part )
	{
		// Done
		return NULL;
	}
	else
	{
		m_pCurrentPart = m_pCurrentPart->next;

		return static_cast<cpart *>( m_pCurrentPart );
	}
}


// OnRemove() must be called when removing/deleting parts
// to update any active iterators.
void CIterator_cpart::OnRemove( cpart const *part )
{
	// For every iterator, adjust the "current part" if that part
	// is the one being removed.
	for( CDLinkList *pElement = m_LIST_Iterator.next; pElement != &m_LIST_Iterator; pElement = pElement->next )
	{
		CIterator_cpart *pIterator = static_cast<CIterator_cpart *>(pElement);

		if( part == pIterator->m_pCurrentPart )
		{
			// Make adjustment to previous part so that the next
			// GetNext() moves to the part after the one removed.
			pIterator->m_pCurrentPart = part->prev;
		}
	}
}
