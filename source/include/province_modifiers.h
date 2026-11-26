#pragma once

#include "modifier.h"

#define HELPER inline

class CProvinceModifiers
{
public:
    HELPER const CProvinceModifier &GetModifier() const;

private:
    uint8_t m_pad0x0[0x10];                  
    CProvinceModifier _Modifier;                                               
    uint8_t m_pad0xd0[0x2398 - 0xd0];         
};

#include "province_modifiers.tpp"
