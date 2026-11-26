#pragma once

#include "province.h"

inline const CProvinceModifiers &CCachedProvinceData::GetModifiers() const
{
    return this->_Modifiers;
}

inline const CProvinceModifier &CProvince::GetModifier() const
{
    return this->_CachedData.GetModifiers().GetModifier();
}

inline const CProvinceHistory &CProvince::GetHistory() const
{
    return this->_History;
}

inline CControllerArea *CProvince::GetControllerArea() const
{
    return this->_pControllerArea;
}
inline COwnerArea *CProvince::GetOwnerArea() const
{
    return this->_pOwnerArea;
}
inline const CUnitList &CProvince::GetUnits() const
{
    return this->_Units;
}
inline CFixedPoint CProvince::GetBlockadeProgress() const
{
    return this->_vBlockadeProgress;
}
inline CPort *CProvince::GetPort() const
{
    return this->_pPort;
}
inline CCombat *CProvince::GetCombat() const
{
    return this->_pCombat;
}
inline CContinent *CProvince::GetContinent() const
{
    return this->_pContinent;
}
inline CColonialRegion *CProvince::GetColonialRegion() const
{
    return this->_pColonialRegion;
}
inline const CList<CConstruction *> &CProvince::GetConstructions() const
{
    return this->_Constructions;
}
inline const CCountryState *CProvince::GetCountryState() const
{
    return this->_pCountryState;
}

inline const CCulture *CProvince::GetCulture() const
{
    return this->_pCulture;
}

HELPER bool CProvince::IsTerraIncognitaActive() const
{
    return this->_bTIActive;
}

template <unsigned N, typename T>
inline bool pdx::hybrid_bitset<N, T>::test(unsigned nIndex) const
{
    return (this->_Storage[nIndex / (sizeof(T) * 8)] >> (nIndex % (sizeof(T) * 8)) & 1) != 0;
}
HELPER bool CProvince::IsDiscoveredBy(CCountryTag tag) const
{
    return this->_Discovered.test(static_cast<unsigned>(tag.GetIndex()));
}

inline CProvince::SSetOwnerParameters::SSetOwnerParameters(CCountryTag newOwner, const CEU3Date &date, bool bAddHistory, bool bAddingToSelf, bool bDestroyBuildings)
    : _NewOwner(newOwner), _Date(date), _bAddHistory(bAddHistory), _bAddingToSelf(bAddingToSelf), _bDestroyBuildings(bDestroyBuildings), _bSetCountrytag(false), _bClearCountryIfLastProvince(true), _bMigrating(false), _bByForce(false)
{
}
