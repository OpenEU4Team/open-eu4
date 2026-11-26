#pragma once

#include "treatytag.h"

inline EStaticTreatyType SPeaceTreatyTag::GetStaticType() const
{
    return this->_StaticType;
}
inline const CScriptablePeaceTreatyDefinition *SPeaceTreatyTag::GetDynamicType() const
{
    return this->_pDynamicType;
}
