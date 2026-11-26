#pragma once
#include "navy.h"

inline void CNavy::SetAIStraitTarget(int nTarget)
{
    this->_nAIStraitTarget._Value = nTarget;
}
inline void CNavy::SetAIBlockadeTarget(int nTarget)
{
    this->_nAIBlockadeTarget = nTarget;
}
inline const CArray<CProvince *> &CNavy::GetBlockadedProvinces() const
{
    return this->_BlockedProvinces;
}
inline CFixedPoint CNavy::GetCurrentBlockadeCapacity() const
{
    return this->_vCurrentBlockadeCapacity;
}
inline int CNavy::GetAIStraitTarget() const
{
    return this->_nAIStraitTarget;
}
inline int CNavy::GetAIBlockadeTarget() const
{
    return this->_nAIBlockadeTarget;
}
