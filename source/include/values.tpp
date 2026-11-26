#pragma once
#include "values.h"

HELPER int32_t CProvinceValue::GetScore() const
{
    return this->val;
}
HELPER void CProvinceValue::SetScore(int32_t value)
{
    this->val = value;
}
HELPER int32_t CProvinceValue::GetId() const
{
    return this->id;
}
HELPER void CProvinceValue::SetId(int32_t value)
{
    this->id = value;
}
inline bool CProvinceValueDescComp::operator()(const CProvinceValue &a, const CProvinceValue &b) const
{
    return a.GetScore() > b.GetScore();
}
inline bool CProvinceValueDescComp::operator()(const CProvinceValue *a, const CProvinceValue *b) const
{
    return this->operator()(*a, *b);
}
