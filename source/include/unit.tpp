#pragma once
#include "unit.h"

inline bool CUnit::GetWasPrevious() const
{
    return this->_bWasPrevious;
}

inline bool CUnit::HasExileStatus() const
{
    if (!this->IsArmy())
        return false;

    const uint64_t raw = this->GetCountry().GetRaw();
    if (!this->GetCountry().CalcIsValid())
        return false;
    if ((raw & kTagTypeMask) == kTagTypeObserver)
        return false;
    if (this->GetCountry().IsRebels())
        return false;
    return this->_bExiled != 0;
}

inline bool CUnit::CanExploreNonAdjacentTI() const
{
    return false;
}

inline CCountryTag CUnit::PendingCondottieri() const
{
    return CCountryTag::FromRaw(kEmptyCountryTag);
}

inline const CMercenaryCompany *CUnit::GetMercenaryCompany() const
{
    return this->_pMercenaryCompany;
}
inline CFixedPoint CUnit::GetCachedAttritionLevel() const
{
    return this->_vAttritionLevel;
}
inline CAIAgent *CUnit::GetAgent() const
{
    return this->_pAgent;
}
inline CProvince *CUnit::GetLocation() const
{
    return this->_pLocation;
}
inline CCountryTag CUnit::GetCountry() const
{
    return this->_Country;
}
inline const CCombat *CUnit::GetCombat() const
{
    return this->_pCombat;
}
inline CCombat *CUnit::AccessCombat() const
{
    return this->_pCombat;
}
inline CSiegeCombat *CUnit::AccessSiege() const
{
    return this->_pSiege;
}
inline CUnit *CUnit::GetAttachedTo() const
{
    return this->_pAttachedTo;
}
inline const CUnitMission *CUnit::GetMission() const
{
    return this->_pMission;
}
inline CUnitMission *CUnit::AccessMission() const
{
    return this->_pMission;
}
inline const CUnitList &CUnit::GetAttachedUnits() const
{
    return this->_AttachedUnits;
}
inline CUnitList &CUnit::GetAttachedUnits()
{
    return this->_AttachedUnits;
}
inline bool CUnit::IsInvading() const
{
    return this->_bIsInvading;
}
inline bool CUnit::IsMainArmy() const
{
    return this->_bMainArmy;
}
inline bool CUnit::IsHunterKiller() const
{
    return this->_bHunterKiller;
}
inline void CUnit::SetGoHomeAtWar(bool bGoHomeAtWar)
{
    this->_bGoHomeAtWar = bGoHomeAtWar;
}
inline int CUnit::GetCount(const CSubUnitDefinition *pDef) const
{
    return CUnit::GetCount(pDef, this->GetSubUnits());
}
inline int CUnit::GetCount(SubUnitType type) const
{
    return CUnit::GetCount(type, this->GetSubUnits());
}

template <typename T>
HELPER T TUnsafeMove<T>::GetId() const
{
    return this->_id;
}
template <typename T>
HELPER const CEU3Date &TUnsafeMove<T>::GetDate() const
{
    return this->_Date;
}
template <typename T>
HELPER CFixedPoint TUnsafeMove<T>::GetDistance() const
{
    return this->_vDist;
}
