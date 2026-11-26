#include "ai.h"
#include "provincetemplate.h"
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

#include "ai_agent.h"
#include "ai_threatevaluation.h"
#include "army.h"
#include "clausewitz/clausewitzlib/order.h"
#include "engine.h"
#include "navy.h"
#include "pathfind.h"
#include "port.h"
#include "province.h"
#include "unit.h"
#include "unitcommands.h"
#include "unitlist.h"

bool CAIAgent::MergeUnit(CUnit &Unit, CUnitList *pList)
{
    bool mergedAny = false;
    CProvince *location = Unit.GetLocation();
    if (pList == nullptr ||
        location->GetProvinceID() == 0 ||
        location->GetCombat() != nullptr ||
        Unit.IsRetreating())
        return false;

    int locationId = Unit.GetFinalLocation()->GetProvinceID();
    CList<CUnit *> merged;
    merged.Append(&Unit);

    const CList<CUnit *> *siblings;
    if (!Unit.IsNavy() ||
        !location->GetTemplate().IsLand())
    {
        siblings = &location->GetUnits();
    }
    else
    {
        siblings = location->GetPort();
    }

    for (CUnit *other : *siblings)
    {
        if (other == &Unit)
            continue;
        if (other->GetAgent() != this)
            continue;
        if (other->IsRetreating() ||
            other->HasExileStatus() ||
            other->GetFinalLocation()->GetProvinceID() != locationId)
            continue;
        bool otherIsArmy = other->IsArmy();
        bool unitIsArmy = Unit.IsArmy();
        if (otherIsArmy != unitIsArmy)
            continue;
        if (otherIsArmy &&
            other->GetArmy()->GetAIDivisionOf() >= 1)
            continue;
        if (other->GetMercenaryCompany() != nullptr)
            continue;

        auto found = pList->end();
        for (auto it = pList->begin(); it != pList->end(); ++it)
        if (*it == other)
        {
            found = it;
            break;
        }
        pList->Remove(found);
            merged.Append(std::move(other));
            mergedAny = true;
        }

    if (1 < merged.GetSize())
    {
        CMergeCommand *cmd = new CMergeCommand(merged);
        ::PostAICommandOnlyIfValid(std::unique_ptr<CCommand>(cmd));
    }
    return mergedAny;
}

bool CAIAgent::MergeArmy(CArmy &army)
{
    return this->MergeUnit(army, this->GetArmyList());
}

bool CAIAgent::MergeFleet(CNavy &fleet)
{
    return this->MergeUnit(fleet, this->GetFleetList());
}

bool CAIAgent::MergeTheseUnits(CUnit &unit, CUnitList *list, bool l_var0x01)
{
    (void)l_var0x01;

    if (list == nullptr ||
        unit.GetLocation()->GetProvinceID() == 0 ||
        unit.GetLocation()->GetCombat() != nullptr ||
        unit.IsRetreating())
        return false;

    CList<CUnit *> &group = *list;

    CList<CUnit *> merged;
    merged.Append(&unit);

    for (auto it = group.begin(); it != group.end();)
    {
        auto cur = it++;
        CUnit *other = *cur;
        if (other != &unit &&
            other->GetLocation() == unit.GetLocation() &&
            other->GetMercenaryCompany() == nullptr)
        {
            if (other->IsArmy())
                other->GetArmy();                       

            group.Remove(cur);
            merged.Append(std::move(other));
        }
    }
    if (1 < merged.GetSize())
    {
        CMergeCommand *cmd = new CMergeCommand(merged);
        ::PostAICommandOnlyIfValid(std::unique_ptr<CCommand>(cmd));
    }

    return 1 < merged.GetSize();
}

bool CAIAgent::AcceptAndMove(CUnit &unit, CProvince &dest, CList<int> &path, int trim)
{
    if (unit.GetLocation()->GetProvinceID() == dest.GetProvinceID())
    {
        CUnit *accepted = &this->TrySplitUnit(unit);
        if (this->IsCanceled() == 0 && this->IsFinished() == 0)
        {
            this->StopUnit(*accepted, EStopMissionReason::Arrived);
            this->AcceptUnit(*accepted);
            return true;
        }
        return false;
    }

    if (!unit.CanMove())
        return false;

    CPathFind finder;
    CPathFind::SFindSettings settings{};
    settings._bDirectIfAdjacent = true;
    settings._bEnableReturnToPreviousNoZocHax = true;
    settings._nNumRegimentsForLoadingOverride = -1;
    std::memcpy(static_cast<void *>(&settings._HiredByCountryOverride), &kEmptyCountryTag,
                sizeof(settings._HiredByCountryOverride));
    settings._bPathLengthMap = 0;
    if (!finder.Find(&unit, unit.GetLocation(), &dest, &path, settings, nullptr, nullptr))
        return false;

    CUnit *accepted = &this->TrySplitUnit(unit);
    if (this->IsCanceled() != 0 || this->IsFinished() != 0)
        return false;

    this->AcceptUnit(*accepted);
    while (trim != 0)
    {
        path.RemoveTail();
        trim--;
    }

    CArray<int> waypoints;
    for (int wayId : path)
        waypoints.Append(wayId);

    if (accepted->IsArmy())
    {
        CArmy *army = accepted->GetArmy();
        CAISiegeMap::AccessInstance().Move(army, dest.GetProvinceID());
    }

    CMoveCommand *cmd = new CMoveCommand(accepted, waypoints, true, false, false);
    ::PostAICommand(cmd);
    return true;
}

AIMoveResult CAIAgent::MoveUnitThroughOwned(CUnit &unit, CProvince &dest,
                                            CCountryFlagCache<char> &flagCache)
{
    CList<int> pathA;
    COwnedPathFind finderA;
    CPathFind::SFindSettings settings{};
    settings._bDirectIfAdjacent = true;
    settings._bEnableReturnToPreviousNoZocHax = true;
    settings._nNumRegimentsForLoadingOverride = -1;
    std::memcpy(static_cast<void *>(&settings._HiredByCountryOverride), &kEmptyCountryTag,
                sizeof(settings._HiredByCountryOverride));
    settings._bPathLengthMap = 0;

    AIMoveResult result;
    if (finderA.Find(&unit, unit.GetLocation(), &dest, &pathA, settings, nullptr, nullptr))
    {

        pathA.RemoveAll();
        result = this->MoveUnit(unit, dest, pathA, finderA, flagCache, 0);
    }
    else
    {
        CList<int> pathB;
        CSafeAiPathFind finderB;

        result = this->MoveUnit(unit, dest, pathB, finderB, flagCache, 0);
    }

    return result;
}
