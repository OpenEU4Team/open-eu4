#pragma once

#include "countrydiplomacy.h"

inline bool CCountryDiplomacy::HasWar(CCountryTag Tag) const
{
    return this->_Wars.HasEntry(Tag);
}

inline const CDiplomacyStatus *CCountryDiplomacy::operator[](int nIndex) const
{
    return &this->_Statuses[nIndex];
}
inline CDiplomacyStatus *CCountryDiplomacy::operator[](int nIndex)
{
    return &this->_Statuses[nIndex];
}

template <typename R>
bool NDiplomacy::CRelationsContainerArray<R>::HasEntry(CCountryTag Tag) const
{
    const int num = Tag.GetIndex();
    for (int i = 0; i < this->_Entries.GetSize(); ++i)
        if (this->_Entries[i].first == num)
            return true;
    return false;
}
