#pragma once

#include "countrytag.h"

inline bool CCountryTag::IsDynamic() const
{
    return this->GetType() != 0;
}

inline bool operator==(CCountryTag lhs, CCountryTag rhs)
{
    return lhs.GetIndex() == rhs.GetIndex();
}

inline int CCountryTag::GetIndex() const
{
    return this->_nIndex;
}
inline bool CCountryTag::IsPirates() const
{
    return this->GetIndex() == this->_nPirateIndex;
}
inline bool CCountryTag::IsNatives() const
{
    return this->GetIndex() == this->_nNativesIndex;
}
inline bool CCountryTag::IsRebelPirateNative() const
{
    return (
        this->IsNatives() ||
        this->IsPirates() ||
        this->IsRebels());
}
inline bool CCountryTag::IsValid() const
{
    return this->_bIsValid != 0;
}
inline bool CCountryTag::CalcIsValid() const
{
    return this->_nIndex != 0;
}
inline size_t std::hash<CCountryTag>::operator()(CCountryTag tag) const
{
    return static_cast<size_t>(tag.GetIndex());
}
HELPER uint64_t CCountryTag::GetRaw() const
{
    return uint64_t(static_cast<uint32_t>(static_cast<uint8_t>(this->_Tag[0])) | static_cast<uint32_t>(static_cast<uint8_t>(this->_Tag[1])) << 8 | static_cast<uint32_t>(static_cast<uint8_t>(this->_Tag[2])) << 16 | static_cast<uint32_t>(static_cast<uint8_t>(this->_Tag[3])) << 24) | (uint64_t(static_cast<uint16_t>(this->_nIndex)) << 32) | (uint64_t(this->_Type) << 48) | (uint64_t(static_cast<uint8_t>(this->_bIsValid)) << 56);
}
HELPER int CCountryTag::GetType() const
{
    return this->_Type;
}
HELPER CountryType CCountryTag::GetCountryType() const
{
    return static_cast<CountryType>(this->GetType() >> 4);
}
HELPER uint32_t CCountryTag::GetCategoryWord() const
{
    return static_cast<uint32_t>(static_cast<uint16_t>(this->_nIndex)) << 16;
}
inline const CCountryDataBase *CCountryDataBase::GetInstance()
{
    return CCountryDataBase::_pInstance;
}
inline CCountryDataBase *CCountryDataBase::AccessInstance()
{
    return CCountryDataBase::_pInstance;
}
inline CPdxArray<CCountry *, int> &CCountryDataBase::GetDBCountryList()
{
    return this->_CountryPtrs;
}
HELPER bool CCountryTag::SameUpperWord(CCountryTag a, CCountryTag b)
{
    return a.GetCategoryWord() == b.GetCategoryWord();
}

HELPER CCountryTag CCountryTag::FromRaw(uint64_t value)
{
    CCountryTag tag;
    tag._Tag[0] = static_cast<char>(value);
    tag._Tag[1] = static_cast<char>(value >> 8);
    tag._Tag[2] = static_cast<char>(value >> 16);
    tag._Tag[3] = static_cast<char>(value >> 24);
    tag._nIndex = static_cast<int16_t>(value >> 32);
    tag._Type = static_cast<uint8_t>(value >> 48);
    tag._bIsValid = static_cast<bool>(value >> 56);
    return tag;
}
