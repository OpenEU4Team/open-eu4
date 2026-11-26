#pragma once
#include "flagship.h"

inline void CFlagship::SetOriginalOwner(CCountryTag Tag)
{
    this->_OriginalOwner = Tag;
}
inline const CString &CFlagshipModification::GetTag() const
{
    return this->_Tag;
}
inline const CString CFlagshipModification::GetName() const
{
    return PdxLocalizeAndReplace(this->GetTag().GetCharPtr(), nullptr, 0);
}
inline const CStaticModifier &CFlagshipModification::GetModifier() const
{
    return this->_Modifier;
}
inline CCountryTag CFlagship::GetOriginalOwner() const
{
    return this->_OriginalOwner;
}
