#ifndef _LINKLIST_H /* [ */
#define _LINKLIST_H

#include <stddef.h>
#include <assert.h>

// Circular doubly linked list
//
// CDLinkList may be used as a mix-in class.
// It actually represents an element of the linked list.
//
class CDLinkList
{
private:
	// remove element from the list (affects other elements)
	void _DLinkList_remove()
	{
		next->prev = prev;
		prev->next = next;
	}

public:
	CDLinkList * prev;
	CDLinkList * next;

public:
	CDLinkList()  { DLinkList_init(); }
	~CDLinkList() { _DLinkList_remove(); }

	// sets the unlinked state of the element
	void DLinkList_init()
	{
	  prev = next = this;	
	}

	// test for linked state
	int isLinked() const
	{
		return (this != next);
	}

	// insert element after another element
	void insert_after(CDLinkList *pDLL_Element)
	{
		ASSERT(!pDLL_Element->isLinked());

		next->prev = pDLL_Element;
		pDLL_Element->next = next;
		pDLL_Element->prev = this;
		next = pDLL_Element;
	}

	// insert element before another element
	void insert_before(CDLinkList *pDLL_Element)
	{
		ASSERT(!pDLL_Element->isLinked());

		prev->next = pDLL_Element;
		pDLL_Element->prev = prev;
		pDLL_Element->next = this;
		prev = pDLL_Element;
	}

	// move element to position after another one
	void move_after(CDLinkList *pDLL_Element)
	{
		pDLL_Element->DLinkList_remove();
		insert_after(pDLL_Element);
	}

	// move element to position before another one
	void move_before(CDLinkList *pDLL_Element)
	{
		pDLL_Element->DLinkList_remove();
		insert_before(pDLL_Element);
	}

	// remove from list and re-initialize to unlinked state
	void DLinkList_remove()
	{
		_DLinkList_remove();
		DLinkList_init();
	}

	// get number of elements in list
	int GetListSize()
	{
		int n = 0;
		CDLinkList * pElement;
		for( pElement = this->next; 
			pElement != this; 
			pElement = pElement->next )
		{
			n++;
		}
		return n;
	}
};

#endif /* !_LINKLIST_H ] */
