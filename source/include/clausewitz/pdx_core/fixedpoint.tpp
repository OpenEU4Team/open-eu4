#pragma once
#include "fixedpoint.h"

inline int CFixedPoint::GetUnderlyingValue() const
{
    return this->_Value.GetUnderlyingValue();
}
inline void CFixedPoint::SetUnderlyingValue(int nValue)
{
    this->_Value.SetUnderlyingValue(nValue);
}

inline CFixedPoint &CFixedPoint::operator=(const CFixedPoint &__that)
{
    this->_Value = __that._Value;
    return *this;
}
