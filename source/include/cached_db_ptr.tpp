#pragma once
#include "cached_db_ptr.h"

template <class T, T Default, bool Mode>
inline TPdxDefVal<T, Default, Mode>::operator T() const
{
    return this->_Value;
}
