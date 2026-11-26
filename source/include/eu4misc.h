#pragma once
#include <cstddef>

template <typename T>
struct TTrivialHasher
{
    inline size_t operator()(T value) const;                                                 
};

#include "eu4misc.tpp"
