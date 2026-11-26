#pragma once

#include "pdxarray.h"
#include <type_traits>

template <typename T, unsigned N, typename SizeType = int>
class CPdxHybridArray : public CPdxArray<T, SizeType>
{
public:
    inline ~CPdxHybridArray() override;                                                            
    void ShrinkToFit() override;                                                                              

protected:
    inline void DeleteDataPtr() override;                                                                         

    using Storage = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
    Storage _StaticData[N];                                       
};

#include "pdxhybridarray.tpp"
