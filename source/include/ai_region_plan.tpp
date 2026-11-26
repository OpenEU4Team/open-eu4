#pragma once
#include "ai_army.h"
#include "ai_region_plan.h"
#include "army.h"
#include "engine.h"
#include "province.h"
#include "unitlist.h"

inline int CAIRegion::GetIndex() const
{
    return this->_nIndex;
}
inline const CPdxArray<int> &CAIRegion::GetProvinces() const
{
    return this->_Provinces;
}
inline const CPdxArray<const CAIRegion *> &CAIRegion::GetNeighbors() const
{
    return this->_Neighbors;
}

inline size_t NAIPlan::CRegionDistanceCacher::LocalHasher::operator()(const std::pair<const CAIArmy *, const CAIRegion *> &input) const
{
    const CID &id = input.first->GetID();
    return (
        (id.GetType() << 16) + id.GetID() + static_cast<unsigned>(input.second->GetIndex()) * kRegionDistanceHashMultiplier);
}

template <typename UnitListT>
void NAIPlan::AddArmyStrengthsToRegions(const UnitListT &UnitList, CArray<NAIPlan::SRegionInfo> &Regions, bool bEnemy)
{
    for (const CUnit *unit : UnitList)
    {
        if (!unit->IsArmy() || unit->IsCondottieri() ||
            unit->GetArmy()->GetAttachedTo() != nullptr)
            continue;

        int32_t outStrength = 0;                           
        auto strength = unit->GetArmyStrengthFactor(nullptr, false, false, outStrength);
        float fStrength = static_cast<float>(strength);                                       

        CProvince *loc = unit->GetLocation();
        if (loc == nullptr)
            continue;
        const CAIRegion *region = CAIRegionMap::GetRegion(loc);
        if (region == nullptr)
            continue;

        SRegionInfo &ri = Regions[region->GetIndex()];
        (bEnemy ? ri._vEnemyStrength : ri._vAlliedStrength) += fStrength;

        for (int i = region->GetNeighbors().GetSize(); i-- > 0;)
        {
            const CAIRegion *nb = region->GetNeighbors()[i];
            (bEnemy ? Regions[nb->GetIndex()]._vEnemyStrength
                    : Regions[nb->GetIndex()]._vAlliedStrength) += fStrength;
        }
    }
}
