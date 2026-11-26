#pragma once
#include "pdxarray.h"

#include "engine.h"
#include <algorithm>

template <typename T, typename SizeType>
T *CPdxArray<T, SizeType>::end()
{
    return this->_pData + this->_nSize;
}

template <typename T, typename SizeType>
inline const T *CPdxArray<T, SizeType>::begin() const
{
    return this->_pData;
}

template <typename T, typename SizeType>
inline const T *CPdxArray<T, SizeType>::end() const
{
    return this->_pData + this->_nSize;
}

template <typename T, typename SizeType>
inline SizeType CPdxArray<T, SizeType>::GetSize() const
{
    return this->_nSize;
}

template <typename T, typename SizeType>
inline const T *CPdxArray<T, SizeType>::GetData() const
{
    return this->_pData;
}

template <typename T, typename SizeType>
inline const T &CPdxArray<T, SizeType>::operator[](SizeType nIndex) const
{
    return this->_pData[nIndex];
}

template <typename T, typename SizeType>
inline T &CPdxArray<T, SizeType>::operator[](SizeType nIndex)
{
    return this->_pData[nIndex];
}

template <typename T, typename SizeType>
template <typename U, typename>
void CPdxArray<T, SizeType>::Append(const U &OtherArray)
{
    const SizeType nTotal = this->_nSize + static_cast<SizeType>(OtherArray.end() - OtherArray.begin());
    if (this->_nCapacity < nTotal)
        this->GrowCapacity(nTotal);
    for (const T &Data : OtherArray)
        this->SetSize(this->_nSize + 1, Data);
}

template <typename T, typename SizeType>
void CPdxArray<T, SizeType>::InsertAt(SizeType nIndex, const T *pData, SizeType nCount)
{
    const SizeType nOldSize = this->_nSize;
    this->SetSize(nOldSize + nCount, T());
    for (SizeType i = nOldSize; i > nIndex; --i)
        this->_pData[i + nCount - 1] = this->_pData[i - 1];
    for (SizeType i = 0; i < nCount; ++i)
        this->_pData[nIndex + i] = pData[i];
}

template <typename T, typename SizeType>
template <typename Comp>
int CPdxArray<T, SizeType>::InsertSorted(const T &Data, const Comp &Compare)
{
    const T *pPosition = std::upper_bound(this->_pData, this->_pData + this->_nSize, Data, Compare);
    const SizeType nIndex = static_cast<SizeType>(pPosition - this->_pData);
    this->InsertAt(nIndex, &Data, 1);
    return nIndex;
}

template <typename T, typename SizeType>
CPdxArray<T, SizeType>::~CPdxArray()
{
    this->_nSize = 0;
    delete[] this->_pData;
    this->_pData = nullptr;
}

template <typename T, typename SizeType>
void CPdxArray<T, SizeType>::DeleteDataPtr()
{
    delete[] this->_pData;
    this->_pData = nullptr;
}

template <typename T, typename SizeType>
void CPdxArray<T, SizeType>::GrowCapacity(SizeType nCapacity)
{
    const SizeType nNewCapacity = std::max(this->_nCapacity * 2, nCapacity);
    T *pNewData = new T[nNewCapacity];
    for (SizeType i = 0; i < this->_nSize; ++i)
        pNewData[i] = std::move(this->_pData[i]);
    this->DeleteDataPtr();
    this->_pData = pNewData;
    this->_nCapacity = nNewCapacity;
}

template <typename T, typename SizeType>
void CPdxArray<T, SizeType>::ShrinkToFit()
{
    if (this->_nSize != this->_nCapacity)
    {
        T *pNewData = nullptr;
        if (this->_nSize != 0)
        {
            pNewData = new T[this->_nSize];
            for (SizeType i = 0; i < this->_nSize; ++i)
                pNewData[i] = std::move(this->_pData[i]);
        }
        this->DeleteDataPtr();
        this->_pData = pNewData;
        this->_nCapacity = this->_nSize;
    }
}

template <typename T, typename SizeType>
void CPdxArray<T, SizeType>::SetSize(SizeType nSize, const T &Data)
{
    if (this->_nCapacity < nSize)
        this->GrowCapacity(nSize);
    for (SizeType i = this->_nSize; i < nSize; ++i)
        this->_pData[i] = Data;
    this->_nSize = nSize;
}

template <typename T, typename SizeType>
void CPdxArray<T, SizeType>::Append(const T &Data)
{
    this->SetSize(this->_nSize + 1, Data);
}

template <typename T, typename SizeType>
void CPdxArray<T, SizeType>::Append(T &&Data)
{
    if (this->_nCapacity <= this->_nSize)
        this->GrowCapacity(this->_nSize + 1);
    this->_pData[this->_nSize++] = std::move(Data);
}

template <typename T, typename SizeType>
void CPdxArray<T, SizeType>::Append(CPdxArray &&OtherArray)
{
    const SizeType nOtherSize = OtherArray._nSize;
    if (this->_nCapacity < this->_nSize + nOtherSize)
        this->GrowCapacity(this->_nSize + nOtherSize);
    for (SizeType i = 0; i < nOtherSize; ++i)
        this->_pData[this->_nSize + i] = std::move(OtherArray._pData[i]);
    this->_nSize += nOtherSize;
    OtherArray.DeleteDataPtr();
    OtherArray._nCapacity = 0;
    OtherArray._nSize = 0;
}

template <typename T, typename SizeType>
void CPdxArray<T, SizeType>::Remove(const T &Data)
{
    for (SizeType i = 0; i < this->_nSize; ++i)
    {
        if (this->_pData[i] == Data)
        {
            for (SizeType j = i; j < this->_nSize - 1; ++j)
                this->_pData[j] = std::move(this->_pData[j + 1]);
            --this->_nSize;
            return;
        }
    }
}

template <typename T, typename SizeType>
template <typename U>
bool CPdxArray<T, SizeType>::RemoveFirstOf(const U &Item)
{
    for (SizeType i = 0; i < this->_nSize; ++i)
    {
        if (this->_pData[i] == Item)
        {
            for (SizeType j = i; j < this->_nSize - 1; ++j)
                this->_pData[j] = std::move(this->_pData[j + 1]);
            --this->_nSize;
            return true;
        }
    }
    return false;
}
