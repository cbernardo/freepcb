#ifndef _PUBSUB_H /* [ */
#define _PUBSUB_H

#include "linklist.h"

//=============================================================================
// Generic publisher and subscriber classes

class CSubscriber;

// Publisher --------------------------
class CPublisher
{
    CDLinkList *m_curSub;
    CDLinkList m_LIST_Sub;

public:
    CPublisher();
    virtual ~CPublisher();

    void Update();

protected: // Generic code which requires template wrapper
    void _Add   (CSubscriber &sub);
    void _Remove(CSubscriber &sub);

    virtual void _Update(CSubscriber *pSub) = 0;
};


// Subscriber --------------------------
//
// A subscriber can be bound to only one publisher at a time.
class CSubscriber : private CDLinkList
{
	friend class CPublisher;
};


//=============================================================================
// Published data and data subscriber class templates

// Data subscriber --------------------

// When the data subscriber is a member of a composition,
// use this macro to get a pointer to the composition object.
#define CDataSubscriber_GET_COMPOSITION(comp_t, comp_var_name) \
    reinterpret_cast<comp_t *> (                               \
        const_cast<char *>(                                    \
            reinterpret_cast<char const *>(this) - offsetof(comp_t, comp_var_name) \
        )                                                      \
    )

// Subscriber template class
template <typename DT>
class CDataSubscriber : public CSubscriber
{
public:
    typedef typename DT pub_t;

    virtual void Update(pub_t const &data) {}
};

// void version
template <>
class CDataSubscriber<void> : public CSubscriber
{
public:
    typedef void pub_t;

    virtual void Update(void) {}
};


// Data publisher ---------------------
//
// Usage:
//  class CMySub : public CDataSubscriber<int>
//  {
//     virtual void Update(pub_t const &data);
//  };
//
//  CPublishedData<int> data;
//  CMySub sub1, sub2;
//
//  data.AddSub(sub1);
//  data.AddSub(sub2);
//
//  data = 3;  Updates sub1 and sub2
//
template <typename DT>
class CPublishedData : public CPublisher
{
public:
    typedef typename CDataSubscriber<DT>::pub_t pub_t;

    // Ctor
    CPublishedData() {}
    explicit CPublishedData(pub_t const &from) : m_data(from) {}
    CPublishedData(CPublishedData const &from) : m_data(from.m_data) {}

    // Get
    operator pub_t() const { return m_data; }

	pub_t       &get_data()       { return m_data; }
	pub_t const &get_data() const { return m_data; }

    // Set
    CPublishedData &operator = (pub_t const &from)
    {
        m_data = from;
        Update();

		return *this;
    }

    CPublishedData &operator = (CPublishedData const &from)
	{
		m_data = from.m_data;
        Update();

		return *this;
    }

    void AddSub   (CDataSubscriber<DT> &sub) { _Add(sub);    }
    void RemoveSub(CDataSubscriber<DT> &sub) { _Remove(sub); }

protected:
    pub_t m_data;

    virtual void _Update(CSubscriber *pSub)
    {
        static_cast< CDataSubscriber<DT> * >(pSub)->Update(m_data);
    }
};

// void version
template <>
class CPublishedData<void> : public CPublisher
{
public:
    typedef CDataSubscriber<void>::pub_t pub_t;

    // Ctor
    CPublishedData() {}
    CPublishedData(CPublishedData const &from) {}

    CPublishedData &operator = (CPublishedData const &from)
	{
        Update();

		return *this;
    }

    void AddSub   (CDataSubscriber<void> &sub) { _Add(sub);    }
    void RemoveSub(CDataSubscriber<void> &sub) { _Remove(sub); }

protected:
    virtual void _Update(CSubscriber *pSub)
    {
        static_cast< CDataSubscriber<void> * >(pSub)->Update();
    }
};


#endif /* !_PUBSUB_H ] */
