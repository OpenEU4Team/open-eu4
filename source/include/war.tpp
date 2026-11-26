#pragma once
#include "war.h"

inline CCountryTag CWar::GetOriginalAttacker() const
{
    return this->_OriginalAttacker;
}
inline CCountryTag CWar::GetOriginalDefender() const
{
    return this->_OriginalDefender;
}
