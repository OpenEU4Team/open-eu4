#pragma once
#include "array.h"

template <typename T>
void CArray<T>::Append(const T &data)
{
    this->_data.push_back(data);
}
template <typename T>
inline int CArray<T>::GetSize() const
{
    return static_cast<int>(this->_data.size());
}
template <typename T>
inline const T *CArray<T>::GetData() const
{
    return this->_data.data();
}
template <typename T>
inline T *CArray<T>::GetData()
{
    return this->_data.data();
}
template <typename T>
inline const T &CArray<T>::operator[](unsigned i) const
{
    return this->_data[i];
}
template <typename T>
inline T &CArray<T>::operator[](unsigned i)
{
    return this->_data[i];
}

template <typename T>
void CArray<T>::Append(const CArray &other)
{
    if (this->_data.empty())
        this->_data = other._data;
    else
        this->_data.insert(this->_data.end(), other._data.begin(), other._data.end());
}

template <typename T>
inline void CArray<T>::RemoveAll()
{
    this->_data.clear();
}

template <typename T>
inline void CArray<T>::RemoveRange(const int nStart, const int nCount)
{
    if (nCount != 0)
        this->_data.erase(this->_data.begin() + nStart, this->_data.begin() + nStart + nCount);
}
