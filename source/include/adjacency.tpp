#pragma once

HELPER int SAdjacency::GetProvinceId() const
{
    return this->_nProvince;
}

HELPER EAdjacencyType SAdjacency::GetType() const
{
    return this->_Type;
}
