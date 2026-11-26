#pragma once
#include "selectable.h"

inline bool CSelectable::IsSelected() const
{
    return this->_IsSelected;
}
inline void CSelectable::SetSelected(bool bSelected)
{
    this->_IsSelected = bSelected;
}
