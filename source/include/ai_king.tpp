#pragma once
#include "ai_king.h"

inline CAIAdmiral *CAIKing::GetAdmiral()
{
    return this->_pAdmiral;
}
inline const CList<CAIArmy *> &CAIKing::GetArmyAgents() const
{
    return this->_ArmyAgents;
}
