#pragma once

#include "fixed.h"
#include <cstdint>
#include <fpml/fixed_point.h>
#include <type_traits>

using CFixedPointFPML = fpml::fixed_point<long long, 48, 15>;

class CFixedPoint
{
public:
    CFixedPoint()                                     
        : _Value(0)
    {
    }
    CFixedPoint(const CFixedPoint &__that)                                        
        : _Value(__that._Value)
    {
    }

    template <typename T>
    CFixedPoint(T nValue, typename std::enable_if<(sizeof(T) <= 4) && std::is_integral<T>::value>::type * = nullptr)                                                                                                                    
        : _Value(nValue)
    {
    }

    operator CFixedPointFPML() const;                                                                                          
    int GetRounded() const;                                                                             
    int GetTruncated() const;                                                                             
    CFixedPoint operator*(const CFixedPoint &rhs) const;                                         

    inline CFixedPoint &operator=(const CFixedPoint &__that);                                        

    inline int GetUnderlyingValue() const;                                                      
    inline void SetUnderlyingValue(int nValue);                                                 

private:
    fixed _Value;                                    
};

#include "fixedpoint.tpp"
