#pragma once
#include "id.h"

inline bool CID::operator==(const CID &Id) const
{
    return this->_Type == Id._Type && this->_ID == Id._ID;
}
inline unsigned CID::GetType() const
{
    return this->_Type;
}
inline unsigned CID::GetID() const
{
    return this->_ID;
}
inline unsigned CID::GetHashValue() const
{
    return (this->_Type << 16) + this->_ID;
}
inline const CID &CReferenceObject::GetID() const
{
    return this->_ID;
}
inline bool CReferenceObject::IsCreated() const
{
    return this->_bIsCreated;
}
