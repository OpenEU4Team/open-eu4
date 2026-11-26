#pragma once

#include "range.h"

template <class Iterator>
inline Iterator pdx::range<Iterator>::begin() const
{
    return this->_first;
}
template <class Iterator>
inline Iterator pdx::range<Iterator>::end() const
{
    return this->_last;
}
