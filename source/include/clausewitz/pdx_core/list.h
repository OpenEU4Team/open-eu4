#pragma once

#include "clausewitz/clausewitzlib/id.h"
#include <cstdint>
#include <iterator>
#include <type_traits>

template <typename T>
class CList
{
public:
    struct Node
    {
        T _Data;                                     
        Node *_pPrev;                                  
        Node *_pNext;                              
        bool _bIsDeleted;                                           
    };

    class Iterator : public std::iterator<std::bidirectional_iterator_tag, T, int64_t, T *, T &>
    {
    public:
        explicit Iterator(Node *pNode)
            : _pCurr(pNode)
        {
        }

        inline Iterator()                                                    
            : _pCurr(nullptr)
        {
        }
        T &operator*();                                                                              
        inline Iterator &operator++();                                                                       
        Iterator operator++(int);                                                                    
        inline Iterator operator--(int);                                                                   
        inline bool operator!=(Iterator i) const;                                                            
        inline bool operator==(Iterator other) const;                                                          

    protected:
        Node *_pCurr;                             
    };

    ~CList();                                                               
    inline CList(const CList &Other);                                           
    bool Remove(Iterator iterator);                                                           
    bool Remove(const T &data);                                                   
    bool RemoveHead();                                                          
    bool RemoveTail();                                                          
    void RemoveAll();                                                                
    void Insert(const T &data);                                                      
    void Insert(T &&data);                                                          
    void Append(T &&data);                                                             

    template <typename U>
    typename std::enable_if<std::is_copy_constructible<U>::value == true>::type Append(const U &data);                                                                                                                           

    inline CList()                                       
        : _pHead(nullptr), _pTail(nullptr), _nSize(0), _IsLocked(false)
    {
    }

    inline Iterator begin() const;                                          
    inline Iterator end() const;                                          
    inline int GetSize() const;                                               
    inline Iterator GetTail() const;                                          
    inline Node *GetHeadNode() const;                                             

protected:
    Node *_pHead;                             
    Node *_pTail;                            
    int _nSize;                                     
    bool _IsLocked;                                 
};

#include "list.tpp"
