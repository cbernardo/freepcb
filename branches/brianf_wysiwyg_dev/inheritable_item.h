#ifndef _INHERITABLE_ITEM_H /* [ */
#define _INHERITABLE_ITEM_H

// Inheritable information
class CInheritableInfo
{
	// Parent points to self if it's not assigned or this is the end of the item list
	CInheritableInfo const *m_pParent;

public:
	enum EStatus
	{
		E_USE_VAL    =  0,
		E_UNDEF      = -1,
		E_USE_PARENT = -2,

		// First available status for derived class
		E__STATUS_USER = -3
	};

	struct Item
	{
		short int m_status;
		int m_val;

		Item()
		{
			undef();
		}

		Item(int val_status)
		{
			// Must clear before set in case val_status
			// is undef (no assignment is done)
			undef();

			Set_item_from_int(val_status);
		}

		void undef(void)
		{
			m_status = E_UNDEF;
			m_val    = 0;
		}

	public: // operators
		Item &operator = (Item const &from)
		{
			// only assign if the source status is DEFINED
			if (from.m_status != E_UNDEF)
			{
				m_status = from.m_status;
				m_val    = from.m_val;
			}

			return *this;
		}

		Item &operator = (int val_status)
		{
			Set_item_from_int(val_status);

			return *this;
		}

		int operator > (Item const &comp) const;
		int operator < (Item const &comp) const;

	public: // Set/Get as int
		// Set item from int:
		//   negative values map to EStatus
		//   zero & positive values count as values
		void Set_item_from_int(int val_status);

		int Get_item_as_int(void) const
		{
			return (m_status == E_USE_VAL) ? m_val : m_status;
		}

	public: // UNDEF item
		static const Item UNDEF;
	};

public:
	CInheritableInfo() : m_pParent(this)
	{
	}

	CInheritableInfo(CInheritableInfo const &from)
	{
		m_pParent = from.hasParent() ? from.m_pParent : this;
	}


	CInheritableInfo &operator = (CInheritableInfo const &from)
	{
		// Copy parent only if current parent is uninitialized and from's parent is initialized
		if( !hasParent() && from.hasParent() ) m_pParent = from.m_pParent;

		return *this;
	}

public:
	void SetParent(CInheritableInfo const &pParent) { SetParent(&pParent); }
	void SetParent(CInheritableInfo const *pParent = NULL)
	{
		m_pParent = pParent ? pParent : this;
	}

	Item const *GetItem(int item_id, Item &item) const;

protected:
	int hasParent(void) const { return m_pParent != this; }

	virtual Item const &_GetItem(int item_id) const;
	virtual void _GetItemExt(Item &item, Item const &src) const;
};


#endif /* !_INHERITABLE_ITEM_H ] */
