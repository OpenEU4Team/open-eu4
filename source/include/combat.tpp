#pragma once

#include "combat.h"

HELPER CCountryTag CCombatantCountry::GetCountryTag() const
{
    return this->_CountryTag;
}

inline int CSiegeCombat::GetCachedSiegeImpact() const
{
    return this->_nSiegeImpact;
}
inline const CCombatant &CCombat::GetAttacker() const
{
    return *this->_pAttacker;
}
inline const CCombatant &CCombat::GetDefender() const
{
    return *this->_pDefender;
}
inline const CUnitList &CCombatant::GetUnits() const
{
    return this->_Units;
}
inline const CCombatantCountryArray &CCombatant::GetParticipatingCountries() const
{
    return this->_ParticipatingCountries;
}
