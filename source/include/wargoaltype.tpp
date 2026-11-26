#pragma once

#include "wargoaltype.h"

inline const CWarGoalPeace &CWarGoalType::GetPeace(bool bAttacker) const
{
    return bAttacker ? this->_AttackerPeace : this->_DefenderPeace;
}
