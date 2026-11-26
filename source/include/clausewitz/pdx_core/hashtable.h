#pragma once

#include <cstdint>

template <typename Key, typename Value>
class ClassicHashKeyTraits
{
public:
    inline static unsigned GetHashValue(const Key &key);                                                                     
    inline static const Key &GetKey(const Value &value);                                                               
};

template <typename Key, typename Value, typename Traits = ClassicHashKeyTraits<Key, Value>>
class CHashTable
{
public:
    struct Node
    {
        Value *_pData;                            
        Node *_pNext;                           
    };

    Value *Find(const Key &key);                                                                                            
    void DeleteAll();                                                                                                        

    void Add(Value *value);                                                                                      

protected:
    unsigned _nSize;                                     
    unsigned _nCapacity;                                 
    Node **_pArray;                                 
};

#include "hashtable.tpp"
