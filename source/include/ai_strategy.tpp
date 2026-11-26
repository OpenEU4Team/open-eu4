#pragma once
#include "ai_strategy.h"

inline bool CAIStrategy::IsFixed() const
{
    return this->_bFixed;
}
inline bool CAIStrategy::IsInitialized() const
{
    return this->_bInitialized;
}
inline const CPdxArray<CAIStrategy::SArmyOrNavySizeObjective, int> &CAIStrategy::GetArmySizeObjectives() const
{
    return this->_ArmySizeTargets;
}
inline bool CAIStrategy::IsBudgetInitialized() const
{
    return this->_bInitializedBudget;
}
inline CFixedPoint CAIStrategy::GetBudget(AIBudgetType type) const
{
    return this->_Budget[type];
}
inline ESpecialUnitCategory CAIStrategy::GetNeedsShips() const
{
    return this->_NeedsShips;
}
inline ESpecialUnitCategory CAIStrategy::GetNeedsRegiments() const
{
    return this->_NeedsRegiments;
}
inline int CAIStrategy::GetDefendedHomeStrait() const
{
    return this->_nDefendedHomeStrait;
}

inline CCountryTag CAIStrategy::GetCountryTag() const
{
    return this->_CountryTag;
}
