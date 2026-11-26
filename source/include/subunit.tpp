#pragma once
#include "subunit.h"

inline SubUnitType CSubUnitDefinition::GetType() const
{
    return this->_Type;
}
inline const CFixedPoint &CSubUnitDefinition::GetDefaultMenCost() const
{
    return this->_vDefaultMenCost;
}
inline CFixedPoint CSubUnit::GetStrength() const
{
    return this->_vStrength;
}
inline const CSubUnitDefinition *CSubUnit::GetDefinition() const
{
    return this->_pDefinition;
}
inline CFixedPoint CSubUnit::GetMorale() const
{
    return this->_vMorale;
}
