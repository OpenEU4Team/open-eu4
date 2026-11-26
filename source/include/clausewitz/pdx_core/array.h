#pragma once

#include <vector>

template <typename T>
class CArray
{
public:
    void Append(const T &data);                                                             
    void Append(const CArray &other);                                                       

    inline void RemoveAll();                                                                                                 
    inline void RemoveRange(const int nStart, const int nCount);                                                      

    inline CArray() = default;                                                                 
    inline int GetSize() const;                                                        
    inline const T *GetData() const;                                                   
    inline T *GetData();                                                               
    inline const T &operator[](unsigned i) const;                                         
    inline T &operator[](unsigned i);                                                     

protected:
    std::vector<T> _data;                               
};

#include "array.tpp"
