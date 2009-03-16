#ifndef _INHERITABLE_ITEM_H /* [ */
#define _INHERITABLE_ITEM_H

// Inheritable information
class CInheritableInfo
{
	// Parent points to self if there is no parent.  This occurs if:
	//	1) Parent not yet assigned
	//  2) This is the end of the item list
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
		int       m_val;

	public: // Constructor/Destructor
		Item()
		{
			Undef();
		}

		explicit Item(int val_status)
		{
			// Must clear before set in case val_status
			// is undef (no assignment is done)
			Undef();

			SetItemFromInt(val_status);
		}

		~Item()
		{
			Undef();
		}

	public: // operators
		Item &operator = (Item const &from)
		{
			// only assign if the source status is DEFINED
			if( from.isDefined() )
			{
				m_status = from.m_status;
				m_val    = from.m_val;
			}

			return *this;
		}

		// To/From int
		Item &operator = (int val_status)
		{
			SetItemFromInt(val_status);

			return *this;
		}

		operator int () const
		{
			return GetItemAsInt();
		}

		// Comparison
		int operator >  (Item const &comp) const;
		int operator == (Item const &comp) const;
		int operator <  (Item const &comp) const { return !operator >(comp) && !operator ==(comp); }

	public:
		// Set/Get as int:
		//   negative values map to EStatus
		//   zero & positive values count as values - i.e. actual values must be >= 0
		void SetItemFromInt(int val_status);

		int GetItemAsInt(void) const
		{
			return (m_status == E_USE_VAL) ? m_val : m_status;
		}

		int isDefined(void) const
		{
			return m_status != E_UNDEF;
		}

		void Undef(void)
		{
			m_status = E_UNDEF;
			m_val    = 0;
		}

	public: // UNDEF item
		static Item const UNDEF;
	};

public:	// construction and copying/assignment
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
		//don't use this - m_pParent = from.hasParent() ? from.m_pParent : this;

		return *this;
	}

public:
	void SetParent(CInheritableInfo const &pParent) { SetParent(&pParent); }
	void SetParent(CInheritableInfo const *pParent = NULL)
	{
		m_pParent = (pParent != NULL) ? pParent : this;
	}

	// Acual item is returned. "item" is updated with the status
	// of the initial item_id and value of the most derived item_id.
	Item const *UpdateItem(int item_id, Item &item) const;

	// Returns the item specified
	virtual Item const &GetItem(int item_id) const;

	int hasParent(void) const { return m_pParent != this; }

	static CString GetItemText(Item const &item);

protected:
	virtual void GetItemExt(Item &item, Item const &src) const;
};

#endif /* !_INHERITABLE_ITEM_H ] */
