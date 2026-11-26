#pragma once

#include "clausewitz/pdx_core/pdxarray.h"

class COrderedProvinceSet
{
public:
    void Append(int Province);                                                          
    bool Contains(int Province) const;                                                     

    inline int GetSize() const;                                                     
    inline const int *begin() const;                                              
    inline const int *end() const;                                              

protected:
    CPdxArray<int, int> _Array;                                           
};

#include "ordered_province_set.tpp"
