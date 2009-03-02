#ifndef _CLEARANCE_H /* [ */
#define _CLEARANCE_H

// Information for clearances
class CClearanceInfo
{
	// Parent points to self if it's not assigned or this is the end of the item list
	CClearanceInfo const *m_pParent;

public:
	enum EStatus
	{
		E_UNDEF = -1024,
		E_AUTO_CALC = -2,
		E_USE_PARENT = -1,
		E_USE_VAL = 0
	};

	struct Item
	{
		short int m_status;
		int m_val;

		Item() : m_status(E_UNDEF), m_val(0)
		{
		}

		// Set item from int:
		//   negative values map to EStatus
		//   positive values count as values
		void Set(int val_status)
		{
			if (val_status < 0)
			{
				if (val_status > E_UNDEF)
				{
					m_status = val_status;
					m_val = 0;
				}
				else
				{
					// Don't assign to <unassigned>
				}
			}
			else
			{
				m_status = E_USE_VAL;
				m_val = val_status;
			}
		}

		int Get_item_as_int(void) const
		{
			return (m_status < 0) ? m_status : m_val;
		}

		Item &operator = (Item const &from)
		{
			// only assign if the source status is DEFINED
			if (from.m_status > E_UNDEF)
			{
				m_status = from.m_status;
				m_val    = from.m_val;
			}

			return *this;
		}

		int operator > (Item const &comp) const
		{
			if (m_status < 0)
			{
				if (comp.m_status < 0) return m_status > comp.m_status;
				else                   return 0;
			}
			else
			{
				if (comp.m_status < 0) return 1;
				else                   return m_val > comp.m_val;
			}
		}

		int operator < (Item const &comp) const
		{
			if (m_status < 0)
			{
				if (comp.m_status < 0) return m_status < comp.m_status;
				else                   return 1;
			}
			else
			{
				if (comp.m_status < 0) return 0;
				else                   return m_val < comp.m_val;
			}
		}
	};

public:
	explicit CClearanceInfo(CClearanceInfo const &from) :
		m_pParent(from.m_pParent),
		m_ca_clearance(from.m_ca_clearance)
	{
	}

	explicit CClearanceInfo(int ca_clearance = E_UNDEF) :
		m_pParent(this)
	{
		m_ca_clearance.Set(ca_clearance);
	}

	CClearanceInfo &operator = (CClearanceInfo const &from)
	{
		// Copy parent only if current parent is uninitialized and from's parent is initialized
		if( !hasParent() && from.hasParent() ) m_pParent = from.m_pParent;

		// Copy Data
		m_ca_clearance = from.m_ca_clearance;

		return *this;
	}

public:
	void SetParent(CClearanceInfo const *pParent = NULL)
	{
		m_pParent = pParent ? pParent : this;
	}
	void SetParent(CClearanceInfo const &pParent) { SetParent(&pParent); }

	// Direct for non-updating version
	Item m_ca_clearance;

    // Update the current value of m_ca_clearance
	void Update_ca_clearance()
	{
        GetItem(m_ca_clearance);
	}

private:
	int hasParent(void) const { return m_pParent != this; }

    Item const *GetItem(int offset) const { return reinterpret_cast<Item const *>(reinterpret_cast<char const *>(this) + offset); }

    void GetItem(Item &item) const
    {
        int offset = (int)(reinterpret_cast<char const*>(&item) - reinterpret_cast<char const*>(this));

        CClearanceInfo const *ci = this;

		item.m_status = ci->GetItem(offset)->m_status;

		for (;;)
		{
   			switch (ci->GetItem(offset)->m_status)
   			{
   			case E_USE_VAL:
   				item.m_val = ci->GetItem(offset)->m_val;
           		return;

   			case E_USE_PARENT:
			{
				CClearanceInfo const *nci = ci->m_pParent;
                if (nci != ci)
                {
                    // Got a valid parent - loop to handle parent
					ci = nci;
                    break;
                }
			}
            // Fallthru to default

   			default:
   				item.m_status = E_UNDEF;
   				// Fallthru to E_UNDEF

   			case E_UNDEF:
   				item.m_val = 0;
           		return;
   			}
		}
    }
};

#endif /* !_CLEARANCE_H ] */
