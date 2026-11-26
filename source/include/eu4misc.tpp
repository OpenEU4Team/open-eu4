#pragma once
#include "eu4misc.h"

template <typename T>
inline size_t TTrivialHasher<T>::operator()(T value) const
{
    return static_cast<size_t>(value);
}
