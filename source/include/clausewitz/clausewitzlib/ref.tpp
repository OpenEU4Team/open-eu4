#pragma once

#include "ref.h"

template <typename T>
inline CID CRef<T>::GetId() const
{
    return this->_id;
}
