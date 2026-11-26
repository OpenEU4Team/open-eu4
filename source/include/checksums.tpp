#pragma once

inline CBasicChecksum::operator int() const
{
    return this->_nSum;
}
