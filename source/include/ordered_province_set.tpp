#pragma once
#include "ordered_province_set.h"

inline int COrderedProvinceSet::GetSize() const
{
    return this->_Array.GetSize();
}
inline const int *COrderedProvinceSet::begin() const
{
    return this->_Array.begin();
}
inline const int *COrderedProvinceSet::end() const
{
    return this->_Array.end();
}
