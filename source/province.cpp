#include "province.h"

CFixedPoint CProvince::GetNominalOwnerSupplyLimit() const
{
    return this->GetSupplyLimit(this->GetOwner(), nullptr, true);
}

const CList<const SAdjacency *> &CProvince::GetStraits() const
{
    return this->_Straits;
}
