#pragma once
#include "date.h"

inline int CDate::GetUniversalDate() const
{
    return this->_nUniversalDate;
}
inline void CDate::SetUniversalDate(int nDate)
{
    this->_nUniversalDate = nDate;
}
