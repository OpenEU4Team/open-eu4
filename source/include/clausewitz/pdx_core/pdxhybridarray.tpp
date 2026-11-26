#pragma once
#include "pdxhybridarray.h"

template <typename T, unsigned N, typename SizeType>
CPdxHybridArray<T, N, SizeType>::~CPdxHybridArray()
{
    this->_nSize = 0;
    this->DeleteDataPtr();
}

template <typename T, unsigned N, typename SizeType>
void CPdxHybridArray<T, N, SizeType>::DeleteDataPtr()
{
    if (this->_pData != reinterpret_cast<T *>(this->_StaticData))
        CPdxArray<T, SizeType>::DeleteDataPtr();
    this->_pData = nullptr;
}
