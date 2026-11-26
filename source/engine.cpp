#include "ai_army.h"
#include "ai_plan_transport.h"
#include "army.h"
#include "navy.h"
#include "gamestate.h"
#include "map.h"
#include "port.h"
#include "province.h"
#include "provincetemplate.h"
#include "engine.h"
#include "ai_subscriber.h"
#include "unit.h"

bool CAIArmy::IsProvinceUnsafeForTransport(int provinceId) const
{
    auto it = this->_UnsafeForTransportMemory.find(provinceId);
    if (it == this->_UnsafeForTransportMemory.end())
        return false;
    return CCurrentGameState::GetInstance()->GetCurrentDate().GetUniversalDate() <= it->second.GetUniversalDate();
}

void CAITransportPlan::TryLoad()
{
    if (this->GetFleetList()->GetSize() <= 0)
        return;
    CUnit* fleet = *this->GetFleetList()->begin();
    if (fleet->GetLocation() != this->_pCoastalSeaZone)
        return;
    CUnit* unit = *this->GetArmyList()->begin();
    CNavy* navy = fleet->GetNavy();
    CUnit* armyUnit = unit->GetArmy();
    if (!navy->CanLoad(armyUnit, -1))
        return;
    if (!unit->IsMoving())
        unit->PerformAction(this->_pCoastalSeaZone, false);
}

void CAITransportPlan::TryUnload()
{
    CNavy* navy = (*this->GetFleetList()->begin())->GetNavy();
    if (!navy->CanUnLoad(this->_pCoastalSeaZone) ||
        this->GetArmyList()->GetSize() == 0) {
        if (this->_pCoastalSeaZone == nullptr) {
            this->Cancel();
            return;
        }
        int seaZone = this->_pCoastalSeaZone->GetPort()->GetSeaZone();
        if (seaZone != 0) {
            this->GetFleetList()->AddMovementTo(seaZone, true);
            return;
        }
        const CProvinceTemplate* area = &this->_pCoastalSeaZone->GetTemplate();
        int target = 0;
        for (const SAdjacency* it = area->GetNeighbours().GetData();
             it != (area->GetNeighbours().GetData() + area->GetNeighbours().GetSize()); ++it) {
            CProvince* prov = &CMap::_pMap->GetProvince(it->GetProvinceId());
            if (!prov->GetTemplate().IsLand())
                target = it->GetProvinceId();
        }
        if (target != 0)
            this->GetFleetList()->AddMovementTo(target, true);
        return;
    }

    CUnit* army = *this->GetArmyList()->begin();
    if (army->IsMoving())
        return;
    CList<int> &path = this->GetPath();
    if (path.begin() == path.end())
        return;
    const int destId = this->_pCoastalSeaZone->GetProvinceID();
    for (int provinceId : path) {
        if (provinceId == destId) {
            this->GetArmyList()->AddMovementTo(destId, true);
            return;
        }
    }
}


const CArray<CCountryTag> &CProvince::GetCores() const
{
    return this->_Core;
}


CNavy *CNavy::GetNavy()
{
    return this;
}


bool CUnit::ShouldGoHomeAtWar() const
{
    return this->_bGoHomeAtWar != 0;
}

bool CUnit::IsRetreating() const
{
    return this->_isRetreat != 0;
}

bool CUnit::IsRetreatingShattered() const
{
    return this->_isRetreatShattered != 0;
}

bool CUnit::IsParsedCorrectly() const
{
    return true;
}

CFixedPoint CUnit::CalcMaintenanceCost() const
{
    return CFixedPoint();
}

const CArray<CSubUnit *> &CUnit::GetSubUnits() const
{
    return *this;
}

bool CUnit::IsPendingCondottieri() const
{
    return false;
}

bool CAISubscriber::WantTicks()
{
    return true;
}
