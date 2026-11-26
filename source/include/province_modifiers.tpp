#pragma once

#include "province_modifiers.h"

HELPER const CProvinceModifier &CProvinceModifiers::GetModifier() const
{
    return this->_Modifier;
}
