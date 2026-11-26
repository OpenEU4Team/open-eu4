#pragma once

#include "subjecttype.h"

inline bool CSubjectType::JoinsOverlordsWars() const
{
    return this->_CopyableMembers._bJoinsOverlordsWars;
}
inline bool CSubjectType::IsVoluntary() const
{
    return this->_CopyableMembers._bIsVoluntary;
}
inline bool CSubjectType::TransferTradePower() const
{
    return this->_CopyableMembers._bTransferTradePower;
}
inline bool CSubjectType::HasColonialParent() const
{
    return this->_CopyableMembers._bHasColonialParent;
}
inline bool CSubjectType::EatsOverlordsColonies() const
{
    return this->_CopyableMembers._bEatsOverlordsColonies;
}
inline bool CSubjectType::UsesMilitaryfocus() const
{
    return this->_CopyableMembers._bUsesMilitaryFocus;
}
