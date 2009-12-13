#pragma once
#include <afxtempl.h>

// CArrayMngIdx< TYPE, ARG_TYPE = const TYPE& >
//
// Parameters: TYPE     = Data type in the area
//             ARG_TYPE = Argument type to pass data around (default = const TYPE& )
//
// Purpose: CArray with managed index updates.
//
// The CArrayMngIdx<> class is designed to contain objects which need to keep
// track of their location in the array.  The elements stored in the
// CArrayMngIdx<> class MUST derive from the managed-index template class,
//    CArrayMngIdx<TYPE,ARG_TYPE>::CElement
//
// The element class (TYPE) contains (via the base class above) the index into
// the array which can be retrieved using the get_CArrayMngIdx() function.
// The index will automatically be changed when the CArrayMngIdx<> is updated.
// Also, the element class (TYPE) may choose to receive update events when
// the index is changed by implementing the CArrayMngIdx_UpdateIndex() function.
//
template<class TYPE, class ARG_TYPE = const TYPE&>
class CArrayMngIdx : public CArray<TYPE, ARG_TYPE>
{
protected:
	void UpdateElementIndex(INT_PTR index)
	{
		TYPE *pElement = m_pData + index;

		pElement->__CArrayMngIdx = index;
		pElement->CArrayMngIdx_UpdateIndex(index);
	}

public:
	class CElement
	{
		friend class CArrayMngIdx;
		INT_PTR __CArrayMngIdx;

	public:
		CElement() : __CArrayMngIdx(-1)
		{
		}

		CElement &operator = ( CElement const &from )
		{
			// Don't change the index during assignment
			return *this;
		}

		INT_PTR get_CArrayMngIdx() const
		{
			return __CArrayMngIdx;
		}

		// Override in derived class
		void CArrayMngIdx_UpdateIndex(INT_PTR index)
		{
		}
	};

public: // Overrides of CArray
// Attributes
	void SetSize(INT_PTR nNewSize, INT_PTR nGrowBy = -1);

// Operations

	// Potentially growing the array
	void SetAtGrow(INT_PTR nIndex, ARG_TYPE newElement);
	INT_PTR Add(ARG_TYPE newElement);
	INT_PTR Append(const CArray& src);
	void Copy(const CArray& src);


	// Operations that move elements around
	void InsertAt(INT_PTR nIndex, ARG_TYPE newElement, INT_PTR nCount = 1);
	void InsertAt(INT_PTR nStartIndex, CArray* pNewArray);

	void RemoveAt(INT_PTR nIndex, INT_PTR nCount = 1);

public: // CArrayMngIdx specific
	void UpdateIndexing(void);
};


//-----------------------------------------------------------------------------
//
// Implementation
//

template<class TYPE, class ARG_TYPE>
void CArrayMngIdx<TYPE, ARG_TYPE>::SetSize(INT_PTR nNewSize, INT_PTR nGrowBy)
{
	INT_PTR idx = m_nSize;

	CArray<TYPE, ARG_TYPE>::SetSize(nNewSize, nGrowBy);

	// Update new elements' indexes
	for ( ; idx < nNewSize; idx++)
	{
		UpdateElementIndex(idx);
	}
}

template<class TYPE, class ARG_TYPE>
void CArrayMngIdx<TYPE, ARG_TYPE>::SetAtGrow(INT_PTR nIndex, ARG_TYPE newElement)
{
	INT_PTR idx = m_nSize;

	CArray<TYPE, ARG_TYPE>::SetAtGrow(nIndex, newElement);

	// Update new elements' indexes
	for ( ; idx < m_nSize; idx++)
	{
		UpdateElementIndex(idx);
	}
}

template<class TYPE, class ARG_TYPE>
INT_PTR CArrayMngIdx<TYPE, ARG_TYPE>::Add(ARG_TYPE newElement)
{
	CArray<TYPE, ARG_TYPE>::Add(newElement);

	// Update new element's index
	UpdateElementIndex( m_nSize - 1 );
}

template<class TYPE, class ARG_TYPE>
INT_PTR CArrayMngIdx<TYPE, ARG_TYPE>::Append(const CArray& src)
{
	INT_PTR idx = m_nSize;

	INT_PTR CArray<TYPE, ARG_TYPE>::Append(src);

	// Update new elements' indexes
	for ( ; idx < m_nSize; idx++)
	{
		UpdateElementIndex(idx);
	}
}

template<class TYPE, class ARG_TYPE>
void CArrayMngIdx<TYPE, ARG_TYPE>::Copy(const CArray& src)
{
	void CArray<TYPE, ARG_TYPE>::Copy(src);

	UpdateIndexing();
}

template<class TYPE, class ARG_TYPE>
void CArrayMngIdx<TYPE, ARG_TYPE>::InsertAt(INT_PTR nIndex, ARG_TYPE newElement, INT_PTR nCount)
{
	CArray<TYPE, ARG_TYPE>::InsertAt(nIndex, newElement, nCount);

	// Update elements' indexes
	for (nIndex; nIndex < m_nSize; nIndex++)
	{
		UpdateElementIndex(nIndex);
	}
}

template<class TYPE, class ARG_TYPE>
void CArrayMngIdx<TYPE, ARG_TYPE>::InsertAt(INT_PTR nStartIndex, CArray* pNewArray)
{
	CArray<TYPE, ARG_TYPE>::InsertAt(nStartIndex, pNewArray);

	// Update elements' indexes
	for (nStartIndex; nStartIndex < m_nSize; nStartIndex++)
	{
		UpdateElementIndex(nStartIndex);
	}
}

template<class TYPE, class ARG_TYPE>
void CArrayMngIdx<TYPE, ARG_TYPE>::RemoveAt(INT_PTR nIndex, INT_PTR nCount)
{
	CArray<TYPE, ARG_TYPE>::RemoveAt(nIndex, nCount);

	// Update elements' indexes
	for (nIndex; nIndex < m_nSize; nIndex++)
	{
		UpdateElementIndex(nIndex);
	}
}

template<class TYPE, class ARG_TYPE>
void CArrayMngIdx<TYPE, ARG_TYPE>::UpdateIndexing(void)
{
	// Update elements' indexes
	for (INT_PTR idx = 0; idx < m_nSize; idx++)
	{
		UpdateElementIndex(idx);
	}
}
