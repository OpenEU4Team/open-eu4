#pragma once

#include "country_cached_data.h"

inline CFixedPoint CCountryCachedData::GetWarThreat() const
{
    return this->_vWarThreat;
}
