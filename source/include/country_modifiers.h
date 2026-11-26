#pragma once

#include "modifier.h"
#include <cstdint>

#define HELPER inline

class CCountry;

class CCountryModifiers
{
public:
    CCountryModifiers(const CCountry *pParent);                                                     

    HELPER const CModifier &GetModifier() const;

protected:
    uint8_t m_pad0x0[0x8 - 0x0];             
    CModifier Modifier;                                                       
    uint8_t m_pad0xc0[0x6a90 - 0xc0];         
};

#include "country_modifiers.tpp"
