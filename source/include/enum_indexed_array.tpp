#pragma once
#include "enum_indexed_array.h"

template <typename T, typename K, size_t N>
inline const T &CEnumIndexedArray<T, K, N>::operator[](K key) const
{
    return std::array<T, N>::operator[](static_cast<size_t>(key));
}

template <typename T, typename K, size_t N>
inline T &CEnumIndexedArray<T, K, N>::operator[](K key)
{
    return std::array<T, N>::operator[](static_cast<size_t>(key));
}
