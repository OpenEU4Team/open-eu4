#pragma once

#include "clausewitz/pdx_core/pdxarray.h"
#include "engine.h"
#include <cstring>

template <typename T, typename SizeType>
T *CPdxArray<T, SizeType>::begin()
{
    return this->_pData;
}

HELPER long long RawFPML(const CFixedPointFPML &value)
{
    long long raw;
    std::memcpy(&raw, &value, sizeof(raw));
    return raw;
}
HELPER CFixedPointFPML FPMLFromRaw(long long raw)
{
    CFixedPointFPML value;
    std::memcpy(static_cast<void *>(&value), &raw, sizeof(raw));
    return value;
}
HELPER int Div1000(int64_t num)
{
    const __int128 prod = static_cast<__int128>(num) * kDiv1000Multiplier;
    return static_cast<int>((prod >> 71) + ((prod >> 127) & 1));
}
HELPER bool IsValidRange(int32_t num)
{
    return num + kFixedPointRangeBias < kFixedPointRangeSize;
}
HELPER int Div1000Checked(int64_t num, int32_t den)
{
    if (::IsValidRange(static_cast<int32_t>(num)))
        return static_cast<int32_t>(static_cast<uint32_t>(num) * static_cast<uint32_t>(kFixedPointResolution)) / den;
    return static_cast<int>((num * kFixedPointResolution) / den);
}
