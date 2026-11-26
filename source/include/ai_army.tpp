#pragma once
#include "ai_army.h"

inline CReachCostCallback::CReachCostCallback(const CUnit *pUnit, CCountryFlagCache<char> &AccessCache)
    : _pUnit(pUnit), _AccessCache(AccessCache)
{
}

inline int CAIArmy::GetBaseProvince() const
{
    return this->_nBaseProv;
}
inline int CAIArmy::GetInvasionTarget() const
{
    return this->_nInvasionTarget;
}
inline CEU3Date CAIArmy::GetInvasionDate() const
{
    return this->_InvasionDate;
}

#include "country.h"
#include "province.h"

inline CUnitList *CAIArmy::GetArmyList()
{
    return &this->_ArmyList;
}

HELPER const CUnitList *CAIArmy::GetArmyList() const
{
    return &this->_ArmyList;
}

inline int CAIArmy::GetConsolidationTarget() const
{
    return this->_nConsolidationTarget;
}

inline bool CAIArmy::IsInvadingBySea() const
{
    return this->_bInvadingBySea;
}

inline bool CAIArmy::IsInvasionInterrupted() const
{
    return this->_bInvasionInterrupted;
}
