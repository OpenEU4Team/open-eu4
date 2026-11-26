#pragma once

#include <utility>

template <typename T, typename SizeType = int>
class CPdxArray
{
public:
    inline CPdxArray()
        : _pData(nullptr), _nCapacity(0), _nSize(0)
    {
    }                                               

    virtual ~CPdxArray();                                                       
    virtual void ShrinkToFit();                                                            

    void SetSize(SizeType nSize, const T &Data);                                               
    void Append(const T &Data);                                                              
    void Append(T &&Data);                                                                  
    void Append(CPdxArray &&OtherArray);                                                      
    void Remove(const T &Data);                                                              
    T *begin();                                                                                                   
    T *end();                                                                                                   

    void InsertAt(SizeType nIndex, const T *pData, SizeType nCount);                                                                  

    template <typename U, typename = decltype(std::declval<const U &>().begin())>
    void Append(const U &OtherArray);                                                                                                                                         

    template <typename U>
    bool RemoveFirstOf(const U &Item);                                                          

    template <typename Comp>
    int InsertSorted(const T &Data, const Comp &Compare);                                                                                                    

    inline const T *begin() const;                                                               
    inline const T *end() const;                                                               
    inline SizeType GetSize() const;                                                               
    inline const T *GetData() const;                                                               
    inline const T &operator[](SizeType nIndex) const;                                                
    inline T &operator[](SizeType nIndex);                                                            

protected:
    void GrowCapacity(SizeType nCapacity);                                                  
    virtual void DeleteDataPtr();                                                                       

    T *_pData;                                              
    SizeType _nCapacity;                                       
    SizeType _nSize;                                        
};

#include "pdxarray.tpp"
