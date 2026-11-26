#pragma once
#include "order.h"

inline bool CCommand::IsTargetedAsynchronous() const
{
    return this->_bTargetedAsynchronous;
}
