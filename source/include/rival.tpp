#pragma once

#include "rival.h"

inline CCountryTag CRival::GetCountry() const
{
    return this->_Country;
}
inline void CRival::SetCountry(CCountryTag tag)
{
    this->_Country = tag;
}
