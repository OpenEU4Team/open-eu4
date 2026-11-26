#pragma once
#include "areas.h"

inline const CProvinceList &CControllerArea::GetProvinceList() const
{
    return this->_ProvList;
}
inline int CControllerArea::GetSize() const
{
    return this->_ProvList.GetSize();
}
inline const CPdxArray<CProvince *> &CControllerArea::GetPorts() const
{
    return this->_Ports;
}
inline const CNeighborAreaList &CControllerArea::GetNeighborList() const
{
    return this->_NeighborList;
}

inline int CControllerArea::GetDivs() const
{
    return this->_nDivs;
}
