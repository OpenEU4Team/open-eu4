#pragma once

#include "country.h"

inline CEnvoy *CCountry::AccessEnvoys(EnvoyType l_var0x0)
{
    return this->_Envoys[static_cast<int>(l_var0x0)];
}

HELPER CNativeCountryData *CCountry::AccessNativeDataOrNullptr()
{
    return this->_pNativeData.get();
}

inline CList<CRival> &CCountry::AccessRivals()
{
    return this->_Rivals;
}

inline const CList<CLeader *> &CCountry::GetActiveLeaders() const
{
    return this->_Leaders;
}

inline CMonarch *CCountry::GetCurrentMonarch() const
{
    return this->_pCurrentMonarch;
}

inline CQueen *CCountry::GetCurrentQueen() const
{
    return this->_pCurrentQueen;
}

inline const CList<int> &CCountry::GetCoreProvinces() const
{
    return this->_CoreProvinces;
}

inline const CList<int> &CCountry::GetClaimProvinces() const
{
    return this->_ClaimProvinces;
}

inline CCountryTag CCountry::GetCoalitionTarget() const
{
    return this->_CoalitionTarget;
}

inline const CPlayerAiPrefs &CCountry::GetPlayerAiPrefs() const
{
    return *this->_PlayerAiPrefs;
}

inline CAIStrategy &CCountry::AccessStrategy()
{
    return this->_AIStrategy;
}
inline const CLeader *CCountry::GetBestReassignableArmyLeader() const
{
    return this->_pBestReassignableArmyLeader;
}
inline const CAIStrategy &CCountry::GetStrategy() const
{
    return this->_AIStrategy;
}
inline const CUnitList &CCountry::GetUnits() const
{
    return this->_Units;
}
inline const COrderedProvinceSet &CCountry::GetOwnedProvinces() const
{
    return this->_OwnedProvinces;
}
inline CCountryTag CCountry::GetOverlord() const
{
    return this->_OverLord;
}
inline const CCountryCachedData &CCountry::GetCachedData() const
{
    return this->_CachedData;
}
inline CProvince *CCountry::GetCapitalLocation() const
{
    return this->_pCapitalLocation;
}
inline CFixedPoint CCountry::GetManpower() const
{
    return this->_vManpower;
}
inline CFixedPoint CCountry::GetMaxManpower() const
{
    return this->_vMaxManpower;
}
inline CTechnologyStatus *CCountry::GetTechnologyStatus() const
{
    return this->_pTechnology;
}
inline CFixedPoint CCountry::GetWarExhaustion() const
{
    return this->_vWarExhaustion;
}
inline CFixedPoint CCountry::GetCachedDevelopmentRaw() const
{
    return this->_vCachedDevelopmentRaw;
}
inline const CArray<CRebelFaction *> &CCountry::GetRebelFactions() const
{
    return this->_RebelFactions;
}
inline bool CCountry::HasUnconditionalSurrender() const
{
    return this->_bHasUnconditionalSurrender;
}
inline bool CCountry::HasCriticalHomeAreaStrait() const
{
    return this->_bHasHomeAreaStrait;
}
inline bool CCountry::IsAtWar() const
{
    return this->_AtWar;
}
inline const CList<int> &CCountry::GetControlledProvinces() const
{
    return this->_ControlledProvinces;
}
inline CFixedPoint CCountry::GetSailors() const
{
    return this->_vSailors;
}
inline CCountryTag CCountry::GetCountryTag() const
{
    return this->_Tag;
}
inline COrderedProvinceSet &CCountry::GetOwnedProvinces()
{
    return this->_OwnedProvinces;
}
inline CFixedPoint CCountry::GetTreasury() const
{
    return this->_vTreasury;
}
inline const CList<CCountryTag> &CCountry::GetCurrentAtWarWith() const
{
    return this->_CurrentAtWarWith;
}
inline const CList<CCountryTag> &CCountry::GetCurrentWarAllies() const
{
    return this->_CurrentWarAllies;
}
inline CList<std::unique_ptr<CControllerArea>> &CCountry::AccessControllerAreaList()
{
    return this->_ControllerAreaList;
}
inline const CList<std::unique_ptr<COwnerArea>> &CCountry::GetOwnerAreaList() const
{
    return this->_OwnerAreaList;
}
inline bool CCountry::IsSubject() const
{
    return this->GetOverlord().IsValid();
}
inline const CCountryDiplomacy &CCountry::GetDiplomacy() const
{
    return this->_Diplomacy;
}

inline const CCulture *CCountry::GetPrimaryCulture() const
{
    return this->_pPrimaryCulture;
}
