#pragma once

#include <array>
#include <cstddef>

enum ELeaderSkill : int;

template <typename T, typename K, size_t N>
class CEnumIndexedArray : public std::array<T, N>
{
public:
    inline const T &operator[](K key) const;                                                                        
    inline T &operator[](K key);                                                                                    
};

#include "enum_indexed_array.tpp"
