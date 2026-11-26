#pragma once

#include "leader.h"

inline CCountryTag CLeader::GetCountry() const
{
    return this->_CountryTag;
}
inline void CLeader::SetCountry(CCountryTag tag)
{
    this->_CountryTag = tag;
}
