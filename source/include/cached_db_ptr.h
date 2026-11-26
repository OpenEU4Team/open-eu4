#pragma once

template<class T, T Default, bool Mode>
struct TPdxDefVal
{
public:
    inline operator T() const;                                                     
    T _Value;                           
};

#include "cached_db_ptr.tpp"
