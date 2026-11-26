#pragma once
#include "text.h"
#include <utility>

inline CString::CString(const CString &other)
    : _str(other.GetCharPtr())
{
}

inline CString::CString(CString &&Other)
    : _str(std::move(Other._str))
{
}

inline void CString::operator=(const CString &other)
{
    this->_str = other.GetCharPtr();
}

inline CString &CString::operator=(CString &&Other)
{
    if (this == &Other)
        return *this;
    this->_str = std::move(Other._str);
    return *this;
}

inline const char *CString::GetCharPtr() const
{
    return this->_str.c_str();
}

inline CString::~CString()
{
}
