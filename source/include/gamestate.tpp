#pragma once

#include "gamestate.h"
#include "province.h"

inline bool CCurrentGameState::HasGameStarted() const
{
    return this->_bHasGameStarted;
}
inline const CArray<CProvince *> &CGameState::GetProvinces() const
{
    return this->_Provinces;
}
inline int CGameState::GetGamespeed() const
{
    return this->_nGameSpeed;
}

inline const CEU3Date &CGameState::GetCurrentDate() const
{
    return this->_CurrentDate;
}
inline const SGamestateRuntimeSettings &CCurrentGameState::GetRuntimeSettings() const
{
    return this->_RuntimeSettings;
}
HELPER CProvince &CGameState::GetProvince(SProvinceId l_var0x01) const
{
    return this->GetProvince(l_var0x01._Id);
}
inline CProvince &CGameState::GetProvince(int nProvince) const
{
    return this->_ProvincesView.begin()[nProvince];
}

inline CCurrentGameState *CCurrentGameState::AccessInstance()
{
    return _pInstance;
}
