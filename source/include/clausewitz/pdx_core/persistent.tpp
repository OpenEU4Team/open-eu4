#pragma once
#include "engine.h"
#include "persistent.h"

inline CPersistent::CPersistent()
    : _TypeToken(PERSISTENT_CLASS_ID)
{
}

inline int CPersistent::GetToken() const
{
    return this->_TypeToken;
}
inline void CPersistent::SetToken(int nToken)
{
    this->_TypeToken = nToken;
}
