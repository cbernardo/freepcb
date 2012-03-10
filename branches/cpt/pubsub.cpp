#include "stdafx.h"
#include "pubsub.h"

CPublisher::CPublisher()
{
    m_curSub = &m_LIST_Sub;
}

CPublisher::~CPublisher()
{
    for( m_curSub = m_LIST_Sub.next; m_curSub != &m_LIST_Sub; m_curSub = m_curSub->next )
    {
        _Remove(*static_cast<CSubscriber *>( m_curSub ));
    }
}

void CPublisher::Update() const
{
    for( m_curSub = m_LIST_Sub.next; m_curSub != &m_LIST_Sub; m_curSub = m_curSub->next )
    {
        _Update(static_cast<CSubscriber *>( m_curSub ));
    }
}

void CPublisher::_Add(CSubscriber &sub)
{
    m_LIST_Sub.insert_after(&sub);
    _Update(&sub);
}

void CPublisher::_Remove(CSubscriber &sub)
{
    // Update the publisher's update iterator if necessary
    if (&sub == m_curSub)
    {
        m_curSub = m_curSub->prev;
    }

    sub.DLinkList_remove();
}
