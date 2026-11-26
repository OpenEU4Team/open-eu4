#pragma once

#include "event.h"

inline int CDelayedEvent::GetRemainingDays() const
{
    return this->_nDays;
}
inline const CEventScope &CDelayedEvent::GetScope() const
{
    return this->_Scope;
}
inline CEvent *CDelayedEvent::GetEvent() const
{
    return this->_pEvent;
}
