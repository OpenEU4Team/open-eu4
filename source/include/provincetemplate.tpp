#pragma once
#include "provincetemplate.h"

inline bool CProvinceTemplate::IsLand() const
{
    return this->_bIsLand;
}
inline const CArray<SAdjacency> &CProvinceTemplate::GetNeighbours() const
{
    return this->_Neighbours;
}
inline int CProvinceTemplate::GetSeaZoneForPort() const
{
    return this->_nSeaZone;
}
inline int CProvinceTemplate::GetNavalIndex() const
{
    return this->_nNavalIndex;
}
inline CMapArea *CProvinceTemplate::GetMapArea() const
{
    return this->_pMapArea;
}