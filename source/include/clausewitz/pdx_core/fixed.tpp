#pragma once
#include "fixed.h"

inline int fixed::GetUnderlyingValue() const
{
    return this->m_nVal;
}
inline void fixed::SetUnderlyingValue(int nValue)
{
    this->m_nVal = nValue;
}
