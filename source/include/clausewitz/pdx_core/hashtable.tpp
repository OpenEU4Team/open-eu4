#pragma once
#include "hashtable.h"

template <typename Key, typename Value>
inline unsigned ClassicHashKeyTraits<Key, Value>::GetHashValue(const Key &key)
{
    return key.GetHashValue();
}

template <typename Key, typename Value>
inline const Key &ClassicHashKeyTraits<Key, Value>::GetKey(const Value &value)
{
    return value.GetKey();
}

template <typename Key, typename Value, typename Traits>
void CHashTable<Key, Value, Traits>::Add(Value *value)
{
    const unsigned bucket = Traits::GetHashValue(Traits::GetKey(*value)) % this->_nCapacity;
    Node *node = new Node{value, this->_pArray[bucket]};
    this->_pArray[bucket] = node;
    ++this->_nSize;
}
