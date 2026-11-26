#pragma once
#include "list.h"
#include <utility>

template <typename T>
void CList<T>::RemoveAll()
{
    Node *node = this->_pHead;
    while (node != nullptr)
    {
        Node *next = node->_pNext;
        node->_Data.~T();
        ::operator delete(node);
        node = next;
    }
    this->_pHead = nullptr;
    this->_pTail = nullptr;
    this->_nSize = 0;
}

template <typename T>
CList<T>::~CList()
{
    this->RemoveAll();
}

template <typename T>
void CList<T>::Append(T &&data)
{
    Node *old = this->_pTail;
    Node *node = new Node{std::move(data), old, nullptr, false};
    this->_pTail = node;
    int count = this->_nSize;
    this->_nSize = count + 1;
    if (count != 0)
        old->_pNext = node;
    else
        this->_pHead = node;
}

template <typename T>
template <typename U>
typename std::enable_if<std::is_copy_constructible<U>::value == true>::type CList<T>::Append(const U &data)
{
    Node *old = this->_pTail;
    Node *node = new Node{data, old, nullptr, false};
    this->_pTail = node;
    int count = this->_nSize;
    this->_nSize = count + 1;
    if (count != 0)
        old->_pNext = node;
    else
        this->_pHead = node;
}

template <typename T>
T &CList<T>::Iterator::operator*()
{
    return this->_pCurr->_Data;
}

template <typename T>
inline typename CList<T>::Iterator &CList<T>::Iterator::operator++()
{
    this->_pCurr = this->_pCurr->_pNext;
    return *this;
}

template <typename T>
typename CList<T>::Iterator CList<T>::Iterator::operator++(int)
{
    Iterator old = *this;
    this->_pCurr = this->_pCurr->_pNext;
    return old;
}

template <typename T>
inline bool CList<T>::Iterator::operator!=(Iterator i) const
{
    return this->_pCurr != i._pCurr;
}

template <typename T>
inline typename CList<T>::Iterator CList<T>::Iterator::operator--(int)
{
    Iterator old = *this;
    this->_pCurr = this->_pCurr->_pPrev;
    return old;
}

template <typename T>
inline bool CList<T>::Iterator::operator==(Iterator other) const
{
    return this->_pCurr == other._pCurr;
}

template <typename T>
inline typename CList<T>::Iterator CList<T>::begin() const
{
    return Iterator(this->_pHead);
}

template <typename T>
inline typename CList<T>::Iterator CList<T>::end() const
{
    return Iterator(nullptr);
}

template <typename T>
inline int CList<T>::GetSize() const
{
    return this->_nSize;
}

template <typename T>
inline typename CList<T>::Iterator CList<T>::GetTail() const
{
    return Iterator(this->_pTail);
}

template <typename T>
inline typename CList<T>::Node *CList<T>::GetHeadNode() const
{
    return this->_pHead;
}

template <typename T>
CList<T>::CList(const CList &Other)
    : CList()
{
    Node *node = Other.GetHeadNode();
    while (node != nullptr)
    {
        Node *next = node->_pNext;
        this->Append(node->_Data);
        node = next;
    }
}
