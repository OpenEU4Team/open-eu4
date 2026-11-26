#pragma once
#include "random.h"

inline bool CRandom::IsLoggingHotJoin()
{
    return CRandom::_bLoggingHotJoin;
}

inline CForbidRandomInScope::CForbidRandomInScope(bool bForbid)
    : _bStoredForbid(CRandom::IsForbidden())
{
    CRandom::SetForBidden(bForbid);
}

inline CForbidRandomInScope::~CForbidRandomInScope()
{
    CRandom::SetForBidden(this->_bStoredForbid);
}
