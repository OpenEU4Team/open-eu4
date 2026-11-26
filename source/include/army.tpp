#pragma once
#include "army.h"

inline CArmy *CArmy::GetArmy()
{
    return this;
}

inline int CArmy::GetAIDivisionOf() const
{
    return this->_nAIDivisionOf;
}
inline int CArmy::GetAIDivisionAt() const
{
    return this->_nAIDivisionAt;
}
inline const CEU3Date &CArmy::GetRelocationDate() const
{
    return this->_RelocationDate;
}
inline CProvince *CArmy::GetRelocationLocation() const
{
    return this->_pRelocationProvince;
}
inline bool CArmy::IsRelocating() const
{
    return this->_bIsRelocating;
}
inline int CArmy::GetTransportTarget()
{
    return this->_nTransTarget;
}
inline void CArmy::SetTransportTarget(int nTransTarget)
{
    this->_nTransTarget._Value = nTransTarget;
}
inline bool CArmy::IsAIDivisionExpectingSiege() const
{
    return this->_bAIDivisionExpectingSiege;
}
