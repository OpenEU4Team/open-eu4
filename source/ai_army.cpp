#include "ai.h"
#include "ai_sharedeval.h"
#include "country_modifiers.h"
#include "enums.h"
#include "provincetemplate.h"
#include "values.h"
#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "ai_admiral.h"
#include "ai_army.h"
#include "ai_king.h"
#include "ai_plan_transport.h"
#include "ai_region_plan.h"
#include "ai_threatevaluation.h"
#include "areas.h"
#include "army.h"
#include "clausewitz/clausewitzlib/gregoriandate.h"
#include "clausewitz/clausewitzlib/order.h"
#include "combat.h"
#include "country.h"
#include "countrytag.h"
#include "engine.h"
#include "gamestate.h"
#include "leader.h"
#include "map.h"
#include "modifier.h"
#include "navy.h"
#include "pathfind.h"
#include "port.h"
#include "province.h"
#include "rebelfaction.h"
#include "relation.h"
#include "strength_factor_cache.h"
#include "subunit.h"
#include "unit.h"
#include "unitcommands.h"
#include "unitlist.h"

static int GetLeastCapacityInProvince(CCountryTag tag, const CProvince *prov)
{
    int occupied = 0;
    for (CUnit *unit : prov->GetUnits())
    {
        if (unit->GetNavy() != nullptr)
            continue;
        if (unit->HasExileStatus() &&
            !CCountryTag::SameUpperWord(prov->GetController(), unit->GetCountry()) &&
            !CCountryTag::SameUpperWord(prov->GetOwner(), unit->GetCountry()))
            continue;
        occupied += unit->GetNumSubUnits();
    }
    return ::GetTargetRegimentCount(tag, prov) - occupied;
}

static bool IsEasyTargetProvince(const CProvince &prov, const CArmy *army,
                                  CStrengthFactorCache &cache)
{
    if (!prov.GetTemplate().IsLand())
        return false;
    const bool weAreHere = army->GetLocation() == &prov;
    if (prov.GetUnits().GetSize() > (weAreHere ? 1 : 0))
        return false;

    const CCountryTag allegiance = army->GetAllegiance();
    CCountry *controller = CCountryDataBase::AccessInstance()->GetCountry(prov.GetController());
    if (CCountryTag::SameUpperWord(controller->GetCountryTag(), allegiance))
        return false;
    if (!controller->IsAtWarWithCountry(allegiance) &&
        !allegiance.IsRebels() && !controller->GetCountryTag().IsRebels())
        return false;

    if (CAISiegeMap::GetInstance().GetFriendlyRegiments(army, prov.GetProvinceID(), false) > 0)
        return false;
    if (prov.HasHostileFortProtectingAgainst(allegiance))
        return false;
    if (prov.GetFortLevelAfterMothball() > 0)
        return false;
    CEU3Date horizon = CCurrentGameState::GetInstance()->GetCurrentDate();
    horizon.SetUniversalDate(horizon.GetUniversalDate() + kEasyTargetThreatHorizon);
    const CFixedPoint threat = CAIThreatEvaluation::GetInstance().CalcArmyThreatForCountry(
        prov.GetProvinceID(), horizon, allegiance, false, cache);
    return threat.GetUnderlyingValue() <= 0;
}

static CFixedPoint GetFriendlyForceStrength(const CArmy *army, bool includeSelf)
{
    CCountry *country = CCountryDataBase::AccessInstance()->GetCountry(army->GetCountry());
    const CProvince *location = army->GetLocation();

    bool combatIsOurs = false;
    if (army->AccessSiege() != nullptr && army->AccessSiege()->GetAttacker().GetParticipatingCountries().GetSize() > 0)
    {
        combatIsOurs = CCountryTag::SameUpperWord(
            army->AccessSiege()->GetAttacker().GetParticipatingCountries()[0].GetCountryTag(), army->GetCountry());
    }

    CFixedPoint total;
    CCountryTag enemyTag = CCountryTag::FromRaw(kEmptyCountryTag);
    CAISharedEval::DetermineEnemy(enemyTag, army, army->GetAllegiance(), location);

    for (CUnit *unit : location->GetUnits())
    {
        if (unit->GetNavy() != nullptr)
            continue;
        if (unit->InCombat() || unit->IsMoving())
            continue;
        if (unit->HasExileStatus())
        {
            if (!CCountryTag::SameUpperWord(location->GetController(), unit->GetCountry()) &&
                !CCountryTag::SameUpperWord(location->GetOwner(), unit->GetCountry()))
                continue;
        }
        if (unit->GetCountry().IsRebels())
        {
            const CRebelFaction *faction = unit->GetRebelFaction();
            if (faction != nullptr && faction->IsFriendly(army->GetCountry()))
                continue;
        }
        if (unit == army && !includeSelf)
            continue;
        CFixedPoint strength = unit->GetTotalStrength();
        if (!CCountryTag::SameUpperWord(unit->GetCountry(), army->GetCountry()))
        {
            if (!combatIsOurs && unit->InSiege())
            {
                total.SetUnderlyingValue(total.GetUnderlyingValue() + strength.GetUnderlyingValue());
                continue;
            }
            if (!country->IsWarFriend(unit->GetCountry(), enemyTag))
                continue;
        }
        total.SetUnderlyingValue(total.GetUnderlyingValue() + strength.GetUnderlyingValue());
    }
    return total;
}

static bool HasEnoughMenForDivision(const CArmy *army, int regimentCount)
{
    CFixedPoint total = army->GetTotalStrength();
    if (total.GetUnderlyingValue() <= regimentCount * 1000 + 1000)
        return false;
    CFixedPoint friendly = ::GetFriendlyForceStrength(army, true);
    return friendly.GetUnderlyingValue() > regimentCount * 1000;
}

CUnitList *CAIArmy::GetFleetList()
{
    return &this->_FleetList;
}

const CUnitList* CAIArmy::GetFleetList() const
{
    return &this->_FleetList;
}

bool CAIArmy::AssignFleet(CNavy &Fleet)
{
    bool active = this->IsActive();
    if (active)
        this->AcceptUnit(Fleet);
    return active;
}

void CAIArmy::DetachFleet(CNavy &Fleet)
{
    auto found = this->GetFleetList()->end();
    for (auto it = this->GetFleetList()->begin(); it != this->GetFleetList()->end(); ++it)
    {
        if (*it == &Fleet)
        {
            found = it;
            break;
        }
    }
    this->GetFleetList()->Remove(found);
}

void CAIArmy::SetInvasionTarget(int nProv, bool bForcedBySea)
{
    if (CCurrentGameState::GetInstance()->GetRuntimeSettings()._nAIInvasionTarget < 0)
    {
        nProv = 0;
        if (this->GetInvasionTarget() == 0)
            return;
        this->_bInvasionInterrupted._Value = false;
    }
    else if (nProv == 0)
    {
        nProv = 0;
        this->_bInvasionInterrupted._Value = false;
    }
    else if (this->GetInvasionTarget() != 0)
    {
    }
    else
    {
        this->_bInvasionInterrupted._Value = false;
    }

    this->_nInvasionTarget = nProv;
    this->_bInvadingBySea = bForcedBySea;

    if (CArmy *main = this->GetMainArmy())
        main->SetTransportTarget(nProv);

    if (this->GetInvasionTarget() == 0)
    {
        for (auto it = this->GetFleetList()->begin(); it != this->GetFleetList()->end();)
        {
            CNavy *fleet = static_cast<CNavy *>(*it);
            ++it;
            CAIKing *king = &this->GetKing();
            king->GetAdmiral()->AssignFleet(*fleet);
        }

        this->_BadLoadZoneList.RemoveAll();
        this->_InvasionDate.SetUniversalDate(static_cast<int>(kNullContainerMarker));
        return;
    }

    this->_nConsolidationTarget = 0;
    this->_bHunterKiller = 0;
    this->_InvasionDate = CCurrentGameState::GetInstance()->GetCurrentDate();
    this->_InvasionDate.AddMonths(kInvasionDeadlineMonths);
}

CProvince *CAIArmy::CalcInvasionTarget(CControllerArea *pArea,
                                       const CArmy *pOurArmy)
{
    if (pArea == nullptr)
        return nullptr;
    CArmy *main = this->GetMainArmy();
    if (main == nullptr)
        return nullptr;

    const CPdxArray<CProvince *, int> *searchList = &pArea->GetPorts();

    CCountryTag areaCtrl = pArea->GetOwner();
    if (areaCtrl.IsRebels() &&
        pArea->GetPorts().GetSize() == 0)
    {
        for (CList<CNeighborArea>::Node *cell = pArea->GetNeighborList().GetHeadNode(); cell;)
        {
            const CList<CNeighborArea>::Node *cur = cell;
            cell = cur->_pNext;
            if (CCountryTag::SameUpperWord(cur->_Data._Area.GetOwner(), this->GetCountryTag()))
            {
                searchList = &cur->_Data._Area.GetPorts();
                break;
            }
        }
    }

    const int count = searchList->GetSize();
    if (count == 0)
        return nullptr;
    CProvince *const *data = searchList->GetData();

    CProvince *bestProvince = nullptr;
    float best = kFLTMax;
    for (int i = 0; i < count; i++)
    {
        CProvince *prov = data[i];

        if (pOurArmy != nullptr)
        {
            int enemyStrength = prov->GetUnits().GetEnemyLandUnitStrength(this->GetCountryTag());
            CFixedPoint ourStrength = main->GetTotalAIStrength();
            if (enemyStrength >= ourStrength.GetUnderlyingValue() / 1000)
                continue;
        }

        float score = prov->GetDistance(main->GetLocation(), true).GetUnderlyingValue() / kMilli;
        bool fullyOurs = CCountryTag::SameUpperWord(prov->GetOwner(), this->GetCountryTag()) &&
                         CCountryTag::SameUpperWord(prov->GetController(), prov->GetOwner());
        if (fullyOurs)
        {
            score += kOwnedControlled;
        }
        else if (!CCountryTag::SameUpperWord(prov->GetOwner(), this->GetCountryTag()))
        {
            CCountry *ourCountry = CCountryDataBase::AccessInstance()
                                       ->GetCountry(this->GetCountryTag());
            if (!ourCountry->IsAtWarWithCountry(prov->GetOwner()) &&
                !prov->GetOwner().IsRebels() &&
                !this->GetCountryTag().IsRebels())
            {
                if (CCountryTag::SameUpperWord(prov->GetController(), ourCountry->GetCountryTag()))
                {
                    score += kNeutralOwner;
                }
                else if (!ourCountry->IsAtWarWithCountry(prov->GetController()) &&
                         !prov->GetController().IsRebels() &&
                         !this->GetCountryTag().IsRebels())
                {
                    score += kNeutralOwner;
                }
            }
        }
        if (best > score)
        {
            best = score;
            bestProvince = prov;
        }
    }
    return bestProvince;
}

int CAIArmy::RecalcInvasionTarget(CArmy *pOurArmy)
{
    CPdxHybridArray<CProvinceValue, 32, int> candidates;
    CAIKing *king = &this->GetKing();
    king->FindInvasionTargets(candidates);

    CCountry *country = CCountryDataBase::AccessInstance()
                            ->GetCountry(this->GetCountryTag());
    const CSubUnitDefinition *transportDef =
        country->GetSubunitTypePreferred(SubUnitType::TRANSPORT);
    if (transportDef != nullptr)
    {
        const CFixedPoint sailSpeed = transportDef->GetSailSpeed();
        if (sailSpeed.GetUnderlyingValue() > 0)
        {
            CStrengthFactorCache &cache = CStrengthFactorCacheManager::AccessCache();
            int bestProvince = 0;
            float best = 0.0f;

            for (const CProvinceValue &candidate : candidates)
            {
                const CProvince *prov =
                    &CCurrentGameState::GetInstance()->GetProvince(candidate.GetId());
                if (prov->GetControllerArea() == nullptr)
                    continue;
                for (CList<CProvince *>::Node *cell = prov->GetControllerArea()->GetProvinceList().GetHeadNode();
                     cell; cell = cell->_pNext)
                {
                    const CProvince *adj = cell->_Data;
                    if (adj->GetTemplate().GetSeaZoneForPort() == 0)
                        continue;
                    if (!pOurArmy->HaveAccess(adj, false, false, false, CCountryTag::FromRaw(kEmptyCountryTag), nullptr, false, nullptr))
                        continue;

                    CEU3Date horizon = CCurrentGameState::GetInstance()->GetCurrentDate();
                    horizon.AddMonths(kRecalcThreatMonths);
                    CFixedPoint threat = CAIThreatEvaluation::GetInstance().CalcThreatForArmy(
                        adj->GetProvinceID(), horizon, pOurArmy, false, cache);

                    CFixedPoint dist;
                    if (pOurArmy->GetLocation()->GetTemplate().GetNavalIndex() < 0)
                        dist.SetUnderlyingValue((adj->GetDistance(pOurArmy->GetLocation(), false)).GetUnderlyingValue());
                    else if (adj->GetTemplate().GetNavalIndex() < 0)
                        dist.SetUnderlyingValue((CMap::_pMap->GetNavalDistanceInfinity()).GetUnderlyingValue());
                    else
                        dist.SetUnderlyingValue((adj->GetNavalDistance(adj->GetProvinceID())).GetUnderlyingValue());
                    const CFixedPoint navalInfinity = CMap::_pMap->GetNavalDistanceInfinity();
                    if (dist.GetUnderlyingValue() < navalInfinity.GetUnderlyingValue())
                    {
                        float distF = dist.GetUnderlyingValue() / kMilli;
                        float eta = distF / (sailSpeed.GetUnderlyingValue() / kMilli);
                        if (!country->HaveAccess(adj, true, false, nullptr, nullptr))
                        {
                            const CProvince *seaZoneProv =
                                &CCurrentGameState::GetInstance()->GetProvince(
                                    adj->GetPort()->GetSeaZone());
                            CFixedPoint targetCost = pOurArmy->CalcTargetCost(adj, seaZoneProv, false);
                            CFixedPoint moveSpeed = pOurArmy->GetMovementSpeed(
                                CCountryDataBase::AccessInstance()
                                    ->GetCountry(pOurArmy->GetCountry())
                                    ->GetCachedData()
                                    .GetModifiers()
                                    .GetModifier(),
                                seaZoneProv, false);
                            eta += (targetCost.GetUnderlyingValue() / kMilli) /
                                   (moveSpeed.GetUnderlyingValue() / kMilli);
                        }
                        float score = ((threat.GetUnderlyingValue() / kRebelThreatDiv + kOne) *
                                       candidate.GetScore()) /
                                      (eta + kOne);
                        if (score > best)
                        {
                            bestProvince = adj->GetProvinceID();
                            best = score;
                        }
                    }
                }
            }
            return bestProvince;
        }
    }
    return 0;
}

bool CAIArmy::TryMergeAtLocation(const CProvince &Prov, CUnitList *pList,
                                 bool bDoValidityCheck)
{
    CList<CUnit *> &cand = *pList;

    if (Prov.GetUnits().GetSize() < 2)
        return false;

    bool sameArea = true;
    if (this->IsInvadingBySea() != 0 && this->IsInvasionInterrupted() == 0)
    {
        CArmy *main = this->GetMainArmy();
        if (!main->HasExileStatus())
        {
            int locId = main->GetTransportTarget();
            const CControllerArea *area =
                CCurrentGameState::GetInstance()->GetProvince(locId).GetControllerArea();
            if (area != nullptr)
            {
                const CControllerArea *own = main->GetLocation()->GetControllerArea();
                if (area != own)
                {
                    if (own == nullptr)
                        sameArea = false;
                    else
                        sameArea = own->IsAdjacent(*area);
                }
            }
            else
            {
                sameArea = false;
            }
        }
        else
        {
            sameArea = false;
        }
    }

    CArray<CUnit *> picked;
    CUnit *bestNonExiled = nullptr;
    CUnit *bestExiled = nullptr;
    for (CUnit *unit : cand)
    {
        if (unit->GetLocation() == &Prov &&
            (sameArea || unit != this->GetMainArmy()))
        {
            if (!unit->HasExileStatus())
            {
                if (bestNonExiled == nullptr ||
                    (!bestNonExiled->HasLeader() && unit->HasLeader()))
                {
                    bestNonExiled = unit;
                }
            }
            else if (bestExiled == nullptr ||
                     (!bestExiled->HasLeader() && unit->HasLeader()))
            {
                bestExiled = unit;
            }
        }
    }
    if (bestExiled != nullptr)
        picked.Append(bestExiled);
    if (bestNonExiled != nullptr)
        picked.Append(bestNonExiled);

    bool merged = false;
    for (int pickedIndex = 0; pickedIndex < picked.GetSize(); ++pickedIndex)
    {
        CUnit *unit = picked[pickedIndex];
        if (unit->GetMercenaryCompany() != nullptr)
            continue;
        CUnitList group;
        int unitStrength = unit->GetTotalStrength().GetUnderlyingValue();
        if (unit->HasLeader())
            unitStrength += unit->GetLeader()->GetManuever() * -1000;
        const CProvince *dest = unit->GetFinalDestination();
        if (dest == nullptr)
            dest = unit->GetLocation();
        CFixedPoint supplyLimit = dest->GetNominalOwnerSupplyLimit();
        for (CUnit *other : cand)
        {
            if (other != unit &&
                other->GetLocation() == unit->GetLocation())
            {
                bool otherInSiege = other->InSiege();
                bool unitInSiege = unit->InSiege();
                if (otherInSiege == unitInSiege &&
                    other->GetMercenaryCompany() == nullptr)
                {
                    bool otherExiled = other->HasExileStatus();
                    bool unitExiled = unit->HasExileStatus();
                    if (otherExiled == unitExiled &&
                        (!unit->IsMoving() ||
                         (unit->GetFinalDestination()->GetTemplate().IsLand() &&
                          unit->GetNextTarget()->GetTemplate().IsLand())))
                    {
                        if (!other->IsMoving() ||
                            (other->GetFinalDestination()->GetTemplate().IsLand() &&
                             other->GetNextTarget()->GetTemplate().IsLand()))
                        {
                            if (other->GetFinalDestination() == unit->GetFinalDestination() &&
                                (sameArea || other != this->GetMainArmy()))
                            {
                                int otherStrength = other->GetTotalStrength().GetUnderlyingValue();
                                if (unitStrength + otherStrength <= supplyLimit.GetUnderlyingValue() ||
                                    unit->IsNavy())
                                {
                                    group.Append(std::move(other));
                                    unitStrength += otherStrength;
                                }
                            }
                        }
                    }
                }
            }
        }
        if (1 <= group.GetSize())
        {
            merged = CAIAgent::MergeTheseUnits(*unit, &group, bDoValidityCheck);
        }
        if (merged)
            return true;
    }
    return merged;
}

void CAIArmy::SplitOff(CArmy *pArmy, int nWantedSize, int nAIDivisionAt, int nAIDivisionOf)
{
    if (nWantedSize == 0 ||
        pArmy->GetMercenaryCompany() != nullptr)
        return;

    CID newCid = CCurrentGameState::AccessInstance()->GenerateCID(EIDCategory::UNIT, false);
    CAIKing *king = &this->GetKing();
    CProvince *location = pArmy->GetLocation();
    bool exile = pArmy->HasExileStatus();
    const CProvince *previous = pArmy->GetPrevious();
    const int prevId = previous != nullptr ? previous->GetProvinceID() : 0;
    const CProvince *previousNoZoC = pArmy->GetPreviousNoZoC();
    const int prevNoZoCId = previousNoZoC != nullptr ? previousNoZoC->GetProvinceID() : 0;
    king->CreateArmy(newCid, location, nullptr, exile, prevId, prevNoZoCId,
                     nAIDivisionAt, 0, nAIDivisionOf);
    this->_OurArmies.Append(newCid);

    int total = pArmy->GetNumSubUnits();
    int remaining = total - nWantedSize;
    int infCount = 0, cavCount = 0, artCount = 0;
    const auto tally = [&]() -> void
    {
        const CArray<CSubUnit *> &subs = pArmy->GetSubUnits();
        for (int subIndex = 0; subIndex < subs.GetSize(); ++subIndex)
        {
            CSubUnit *sub = subs[subIndex];
            if (sub == nullptr)
                continue;
            const SubUnitType cat = sub->GetDefinition()->GetType();
            if (cat == SubUnitType::ARTILLERY)
                artCount++;
            else if (cat == SubUnitType::CAVALRY)
                cavCount++;
            else if (cat == SubUnitType::INFANTRY)
                infCount++;
        }
    };
    tally();
    if (total == 0)
        total = -1;
    else
    {
        int fraction = ::Div1000Checked(static_cast<int64_t>(remaining) * 1000, total * 1000);
        if (fraction == 0)
            return;
        int keepInf = ::Div1000(static_cast<int64_t>(infCount) * 1000 * fraction) / 1000;
        int keepCav = ::Div1000(static_cast<int64_t>(cavCount) * 1000 * fraction) / 1000;
        int keepArt = ::Div1000(static_cast<int64_t>(artCount) * 1000 * fraction) / 1000;
        int keepTotal = keepCav + keepInf;
        while (keepTotal + keepArt < remaining)
        {
            if (keepInf < infCount)
                keepInf++;
            else if (keepCav < cavCount)
                keepCav++;
            else
            {
                if (artCount <= keepArt)
                    return;
                keepArt++;
            }
            keepTotal = keepCav + keepInf;
        }
        if (0 < remaining)
        {
            const CArray<CSubUnit *> &subs = pArmy->GetSubUnits();
            for (size_t i = subs.GetSize(); i >= 1; i--)
            {
                CSubUnit *sub = subs[i - 1];
                const SubUnitType cat = sub->GetDefinition()->GetType();
                if (cat == SubUnitType::ARTILLERY)
                {
                    if (keepArt > 0)
                    {
                        keepArt--;
                        king->AssignSubUnitProperly(newCid, sub);
                        remaining--;
                    }
                }
                else if (cat == SubUnitType::CAVALRY)
                {
                    if (keepCav > 0)
                    {
                        keepCav--;
                        king->AssignSubUnitProperly(newCid, sub);
                        remaining--;
                    }
                }
                else if (cat == SubUnitType::INFANTRY && keepInf > 0)
                {
                    keepInf--;
                    king->AssignSubUnitProperly(newCid, sub);
                    remaining--;
                }
                if (i < 2)
                    break;
            }
        }
    }
    this->_nSplitOff = 3;
}

void CAIArmy::MergeDivisions(const std::map<std::pair<int, int>, CList<CUnit *>> &divisionMap)
{
    for (const auto &entry : divisionMap)
    {
        const CList<CUnit *> &group = entry.second;
        if (group.GetSize() > 1)
        {
            CMergeCommand *cmd = new CMergeCommand(group);
            ::PostAICommandOnlyIfValid(std::unique_ptr<CCommand>(cmd));
        }
    }
}

void CAIArmy::AssignToRegion(const CAIRegion *pRegion, CCountryFlagCache<char> *pAccessCache)
{
    this->_pAssignedRegion = pRegion;
    if (pRegion == nullptr)
        return;

    for (int provId : pRegion->GetProvinces())
        if (provId == this->GetBaseProvince())
            return;

    CCountry *country = &this->GetCountry();
    CArmy *main = this->GetMainArmy();
    int bestProvince = this->GetBaseProvince();
    float bestScore = 0.0f;

    for (int provId : pRegion->GetProvinces())
    {
        if (main == nullptr)
            break;
        CProvince *prov = &CCurrentGameState::GetInstance()->GetProvince(provId);
        if (!main->HaveAccess(prov, false, false, true, CCountryTag::FromRaw(kEmptyCountryTag), nullptr,
                              false, pAccessCache))
            continue;

        CFixedPoint supply = prov->GetSupplyLimit(country->GetCountryTag(), nullptr, false);
        float score = supply.GetUnderlyingValue() / kMilli;

        for (CCountryTag holderTag : {prov->GetOwner(), prov->GetController()})
        {
            if (CCountryTag::SameUpperWord(holderTag, country->GetCountryTag()))
                score += kOwnControlledSettle;
            else
            {
                const CDiplomacyStatus &dip = *country->GetDiplomacy()[holderTag.GetIndex()];
                if (dip.IsFightingWarTogether())
                    score += kWarTogether;
                if (dip.HasWar())
                    score += kAtWar;
            }
        }
        if (score > bestScore)
        {
            bestScore = score;
            bestProvince = provId;
        }
    }

    if (bestProvince != 0 && bestProvince != this->GetBaseProvince())
        this->_nBaseProv = bestProvince;
}

const CAIRegion *CAIArmy::GetAssignedRegion() const
{
    CCountry *country = &this->GetCountry();
    if (country->IsSubject() &&
        country->GetSubjectMilitaryFocus() != ESubjectMilitaryFocus::_FOCUS_NONE_)
        return nullptr;
    CArmy *main = this->GetMainArmy();
    if (main != nullptr && main->HasExileStatus())
        return nullptr;
    return this->_pAssignedRegion;
}

bool CAIArmy::CanGoByLand(CArmy *pArmy, CProvince *pTarget, CList<int> &Path,
                          CCountryFlagCache<char> &AccessCache, int nRegsOverride)
{
    if (pArmy == nullptr)
    {
        pArmy = this->GetMainArmy();
        if (pArmy == nullptr)
            return false;
    }

    CPathFind::SFindSettings settings{};
    settings._bDirectIfAdjacent = true;
    settings._bEnableReturnToPreviousNoZocHax = true;
    settings._nNumRegimentsForLoadingOverride = nRegsOverride;
    settings._HiredByCountryOverride = CCountryTag::FromRaw(kEmptyCountryTag);
    settings._pAccessCache = &AccessCache;
    settings._bPathLengthMap = 0;

    CPathFind finder;
    return finder.Find(pArmy, pArmy->GetLocation(), pTarget, &Path, settings, nullptr, nullptr);
}

bool CAIArmy::CanInvadeBySea(CArmy *pArmy, CProvince *pTarget, int &nNeededTPC,
                             int &nWantedTPC, CCountryFlagCache<char> &AccessCache,
                             bool bIgnoreStrengthChecks)
{
    if (pTarget->GetTemplate().GetSeaZoneForPort() == 0)
        return false;
    if (pArmy->IsCondottieri())
        return false;

    CAIKing *king = &this->GetKing();
    int divNeed = king->CalcInvasionAreaDivNeed(*pTarget, AccessCache);
    nWantedTPC = 0;
    nNeededTPC = 0;

    int transports = this->GetFleetList()->GetTPC();
    if (transports == 0)
    {
        if (!this->GetKing().MayRequestFleets(this))
            return false;
    }

    if (pArmy == nullptr &&
        (pArmy = this->GetMainArmy()) == nullptr)
        return false;

    CCountry *ourCountry = CCountryDataBase::AccessInstance()
                               ->GetCountry(this->GetCountryTag());
    transports = ourCountry->GetUnits().GetTotalNumOfTransports();
    if (transports <= 0)
        return false;

    int numSubUnits = pArmy->GetNumSubUnits();
    nWantedTPC = numSubUnits;

    if (!bIgnoreStrengthChecks)
    {
        if (transports < numSubUnits)
            numSubUnits = transports;
        CFixedPoint tactics = ourCountry->GetCountryMilitaryTactics(nullptr);
        CFixedPoint scaled(::Div1000(static_cast<int64_t>(numSubUnits * 1000) * tactics.GetUnderlyingValue()));
        int scaledSubUnits = scaled.GetRounded();
        if (scaledSubUnits * 2 < divNeed)
        {
            CControllerArea *homeArea =
                ourCountry->GetHomeControllerArea();
            CControllerArea *stagingArea = pTarget->GetControllerArea();
            if (homeArea != stagingArea)
            {
                if (homeArea == nullptr)
                    return false;
                if (stagingArea == nullptr)
                    return false;
                if (!stagingArea->IsAdjacent(*homeArea))
                    return false;
            }
        }
    }

    const CProvince *portProv =
        &CCurrentGameState::GetInstance()->GetProvince(pTarget->GetTemplate().GetSeaZoneForPort());

    int minUnits = nWantedTPC;
    if (transports <= nWantedTPC)
        minUnits = transports;
    nNeededTPC = minUnits;
    if (nWantedTPC < minUnits)
    {
        nNeededTPC = minUnits;
        minUnits = nWantedTPC;
    }

    int needScaled = ::Div1000(static_cast<int64_t>(divNeed * 1000) * 1250);
    int needUnits = needScaled / 1000;
    if (needScaled > 0 && needScaled != (needScaled / 1000) * 1000)
        needUnits++;
    if (needUnits < 1)
        needUnits = 1;
    if (minUnits <= needUnits)
        needUnits = minUnits;
    nNeededTPC = needUnits;
    if (transports < needUnits)
        return false;

    for (CUnit *unit : ourCountry->GetUnits())
    {
        if (!unit->IsNavy())
            continue;
        if (unit->GetTPC() <= 0)
            continue;

        CSafeAiPathFind finder;

        CPathFind::SFindSettings settings{};
        settings._bDirectIfAdjacent = true;
        settings._bEnableReturnToPreviousNoZocHax = true;
        settings._nNumRegimentsForLoadingOverride = -1;
        settings._HiredByCountryOverride = CCountryTag::FromRaw(kEmptyCountryTag);
        settings._pStrengthFactorCache = &CStrengthFactorCacheManager::AccessCache();
        settings._pAccessCache = &AccessCache;
        settings._bPathLengthMap = 0;

        CList<int> path;
        bool found = finder.Find(unit, unit->GetLocation(), portProv, &path, settings,
                                 nullptr, nullptr);
        if (found)
        {
            for (int wayId : path)
            {
                this->_UnsafeForTransportMemory.find(wayId);
            }
            return true;
        }
    }
    return false;
}

CAIArmy::EGetFleetsToLoadZoneResult CAIArmy::GetFleetsToLoadZoneAndDetachUnnecessary(
    CCountryFlagCache<char> &AccessCache, const CProvince *pLoadProv,
    const CProvince *pInvasionTarget, int nWantedTransports, int nNeededTransports, CFixedPointFPML vWantedStrength,
    CFixedPointFPML vNeededStrength)
{
    if (nNeededTransports <= 0 || this->GetFleetList()->GetSize() == 0)
        return this->CancelInvasion;

    std::vector<SFleetInfo> fleets;
    for (auto it = this->GetFleetList()->begin(); it != this->GetFleetList()->end();)
    {
        auto cur = it++;
        CUnit *fleet = *cur;
        if (fleet == nullptr)
        {
            this->GetFleetList()->Remove(cur);
        }
        else
        {
            bool useInCombat = fleet->InCombat();
            if (!useInCombat)
            {
                CFixedPoint avgStrength = fleet->GetAverageStrength();
                bool strong = avgStrength.GetUnderlyingValue() >= kMinAvgStrength;
                bool highMorale = false;
                if (strong)
                {
                    CFixedPoint morale = fleet->GetAverageMorale(false);
                    highMorale = morale.GetUnderlyingValue() >= kMinAvgStrength;
                }
                bool walkable = fleet->GetLocation()->GetTemplate().IsLand();
                bool normal = ((strong && highMorale) || !walkable) &&
                              !fleet->IsRetreating() && !fleet->IsExploring();
                useInCombat = !normal;
            }

            if (useInCombat)
            {
                CFixedPoint dist = fleet->GetLocation()->GetNavalDistance(pLoadProv->GetProvinceID());
                fleets.push_back({static_cast<CNavy *>(fleet), dist, AIMoveResult::_AI_MOVE_RESULT_CANTMOVE_});
            }
            else if (fleet->GetLocation()->GetProvinceID() == pLoadProv->GetProvinceID())
            {
                if (!fleet->IsMoving())
                {
                    fleets.push_back({static_cast<CNavy *>(fleet), CFixedPoint(0), AIMoveResult::_AI_MOVE_RESULT_NOTNEEDED_});
                }
                else
                {
                    this->HaltUnit(*fleet, EStopMissionReason::NeedToLoad);
                    if (!fleet->IsMovementLocked())
                    {
                        fleets.push_back({static_cast<CNavy *>(fleet), CFixedPoint(0), AIMoveResult::_AI_MOVE_RESULT_NOTNEEDED_});
                    }
                    else
                    {
                        CFixedPoint dist =
                            fleet->GetNextTarget()->GetNavalDistance(pLoadProv->GetProvinceID());
                        fleets.push_back({static_cast<CNavy *>(fleet), CFixedPoint(dist.GetUnderlyingValue() * 2), AIMoveResult::_AI_MOVE_RESULT_OK_});
                    }
                }
            }
            else
            {
                AIMoveResult mv = this->MoveFleet(*static_cast<CNavy *>(fleet), pLoadProv->GetProvinceID(),
                                         false, AccessCache);
                CFixedPoint dist = fleet->GetLocation()->GetNavalDistance(pLoadProv->GetProvinceID());
                fleets.push_back({static_cast<CNavy *>(fleet), dist, mv});
            }
        }
    }

    std::stable_sort(fleets.begin(), fleets.end());

    const int remainingFleets = nWantedTransports - 1;
    int64_t accumStrength = 0;
    int accumTPC = 0;
    int enRouteSeen = 0;
    const CUnit *prevFleet = fleets.empty() ? nullptr : fleets[0]._pNavy;
    int prevScore = 0;
    AIMoveResult prevKind = AIMoveResult::_AI_MOVE_RESULT_NOPATH_;

    for (size_t i = 0; i < fleets.size(); i++)
    {
        CUnit *fleet = fleets[i]._pNavy;
        const int navalDistance = fleets[i]._vDistance.GetUnderlyingValue();
        const AIMoveResult reachState = fleets[i]._MoveResult;

        CCountry *fleetCountry = CCountryDataBase::AccessInstance()
                                     ->GetCountry(fleet->GetCountry());
        CFixedPoint aux;
        CFixedPointFPML strength = fleet->GetNavyStrengthFactor(
            fleetCountry->GetCachedData().GetModifiers().GetModifier(), false, true,
            ESeaType::MAX, aux, nullptr,
            SubUnitType::MAX, nullptr);
        accumStrength += ::RawFPML(strength);
        accumTPC += fleet->GetTPC();

        bool keep;
        if ((prevKind == AIMoveResult::_AI_MOVE_RESULT_NOPATH_) == (reachState == AIMoveResult::_AI_MOVE_RESULT_NOPATH_))
        {
            if ((prevKind > AIMoveResult::_AI_MOVE_RESULT_NOTNEEDED_) == (reachState < AIMoveResult::_AI_MOVE_RESULT_NOPATH_))
            {
                keep = reachState <= AIMoveResult::_AI_MOVE_RESULT_NOTNEEDED_;
            }
            else if (navalDistance != prevScore)
            {
                keep = navalDistance < prevScore;
            }
            else
            {
                keep = fleet->GetID().GetID() < prevFleet->GetID().GetID();
            }
        }
        else
        {
            keep = reachState != AIMoveResult::_AI_MOVE_RESULT_NOPATH_;
        }

        if (!keep)
        {
            if (reachState == AIMoveResult::_AI_MOVE_RESULT_NOPATH_)
            {
                CList<int> &blocked = this->_BadLoadZoneList;
                bool present = false;
                for (int blockedId : blocked)
                    if (blockedId == pLoadProv->GetProvinceID())
                    {
                        present = true;
                        break;
                    }
                if (!present)
                    blocked.Append(pLoadProv->GetProvinceID());
            }
            this->DetachFleet(*static_cast<CNavy *>(fleet));
            continue;
        }

        if (accumTPC >= nNeededTransports && accumStrength >= ::RawFPML(vWantedStrength))
        {
            int fleetWeight = (remainingFleets + accumTPC) / accumTPC;
            CFixedPoint dist = pLoadProv->GetNavalDistance(pInvasionTarget->GetProvinceID());
            if (fleetWeight < 1)
                fleetWeight = 1;
            int scaled = ::Div1000(static_cast<int64_t>(dist.GetUnderlyingValue()) * (fleetWeight * 2000 - 1000));
            prevScore = ((scaled < 0 ? scaled + 3 : scaled) >> 2) + navalDistance;
            prevFleet = fleet;
            prevKind = reachState;
        }
        if (reachState == AIMoveResult::_AI_MOVE_RESULT_UNSAFE_)
            return CancelInvasion;
        enRouteSeen = navalDistance != 0 ? 1 : (enRouteSeen & 0xff);
    }

    if (accumTPC < nNeededTransports)
        return CancelInvasion;
    if (accumStrength < ::RawFPML(vNeededStrength))
        return CancelInvasion;
    if (this->GetFleetList()->GetSize() < 2)
        return (enRouteSeen & 1) != 0 ? Wait : GoGoGo;
    this->TryMergeAtLocation(*pLoadProv, this->GetFleetList(), true);
    return Wait;
}

bool CAIArmy::CanInvade(int targetProvince) const
{
    if (this->_CannotInvadeList.GetSize() == 0)
        return true;

    CControllerArea *targetArea =
        CCurrentGameState::GetInstance()->GetProvince(targetProvince).GetControllerArea();
    for (int provinceId : this->_CannotInvadeList)
    {
        CProvince *blocked = &CCurrentGameState::GetInstance()->GetProvince(provinceId);
        if (blocked->GetControllerArea() != nullptr &&
            blocked->GetControllerArea() == targetArea)
            return false;
    }
    return true;
}

bool CAIArmy::MayInvade() const
{
    CArmy *main = this->GetMainArmy();
    if (main == nullptr)
        return false;
    if (main->HasExileStatus())
        return true;
    if (this->IsNearHotSpot())
        return false;
    return this->_bIsIdleForInvasion != 0;
}

float CReachCostCallback::Cost(const CControllerArea &area)
{
    const CProvince *prov = *area.GetProvinceList().begin();
    return this->_pUnit->HaveAccess(prov, false, false, false, CCountryTag::FromRaw(kEmptyCountryTag),
                                    nullptr, false, &this->_AccessCache)
               ? 1.0f
               : -1.0f;
}

bool CAIArmy::CanReach(const CArmy *pArmy, const CProvince *pTarget,
                       CCountryFlagCache<char> &AccessCache)
{
    const CProvince *location = pArmy->GetLocation();
    if (location == nullptr)
        return false;

    CPathFind::SFindSettings settings{};
    settings._bDirectIfAdjacent = true;
    settings._bEnableReturnToPreviousNoZocHax = true;
    settings._nNumRegimentsForLoadingOverride = -1;
    settings._HiredByCountryOverride = CCountryTag::FromRaw(kEmptyCountryTag);
    settings._pAccessCache = &AccessCache;
    settings._bPathLengthMap = 0;

    if (pArmy->HasExileStatus())
    {
        CCountry *ourCountry = CCountryDataBase::AccessInstance()
                                   ->GetCountry(this->GetCountryTag());
        bool sameContinent = pTarget->GetContinent() == location->GetContinent();
        bool noTransports = ourCountry->GetUnits().GetTPC() == 0;
        if (sameContinent || noTransports)
        {
            CPathFind finder;
            CList<int> path;
            bool found = finder.Find(pArmy, location, pTarget, &path, settings, nullptr, nullptr);
            if (found)
                return true;
        }
    }

    CControllerArea *locationArea = location->GetControllerArea();
    CControllerArea *targetArea = pTarget->GetControllerArea();
    if (locationArea == nullptr || targetArea == nullptr)
    {
        CPathFind finder;
        bool found = finder.Find(pArmy, location, pTarget, nullptr, settings, nullptr, nullptr);
        return found;
    }
    ::CReachCostCallback cb(pArmy, AccessCache);
    return CControllerArea::FindBestPathV2(*locationArea, *targetArea, &cb, nullptr);
}

void CAIArmy::SetConsolidationTarget(int nProv)
{
    this->_nConsolidationTarget = nProv;
    if (nProv == 0)
    {
        for (auto it = this->GetFleetList()->begin(); it != this->GetFleetList()->end();)
        {
            CUnit *fleet = *it;
            ++it;
            CAIKing *king = &this->GetKing();
            king->GetAdmiral()->AssignFleet(static_cast<CNavy &>(*fleet));
        }

        this->_BadLoadZoneList.RemoveAll();
        return;
    }

    std::vector<const CProvince *> locations;
    for (CUnit *unit : *this->GetArmyList())
    {
        const CProvince *location = unit->GetLocation();
        if (std::find(locations.begin(), locations.end(), location) == locations.end())
            locations.push_back(location);
    }
    for (const CProvince *prov : locations)
        this->TryMergeAtLocation(*prov, this->GetArmyList(), true);
}

void CAIArmy::SetProvinceUnsafeForTransport(int provinceId, int travelDays)
{
    const int deadline =
        CCurrentGameState::GetInstance()->GetCurrentDate().GetUniversalDate() + travelDays * kTravelDayHours;
    auto it = this->_UnsafeForTransportMemory.find(provinceId);
    if (it == this->_UnsafeForTransportMemory.end())
    {
        CEU3Date date = CCurrentGameState::GetInstance()->GetCurrentDate();
        date.SetUniversalDate(deadline);
        this->_UnsafeForTransportMemory.emplace(provinceId, date);
    }
    else if (deadline > it->second.GetUniversalDate())
    {
        it->second.SetUniversalDate(deadline);
    }
}

void CAIArmy::ProcessSplinters(CCountryFlagCache<char> &AccessCache)
{
    if (this->GetArmyList()->GetSize() < 2)
        return;

    CArmy *main = this->GetMainArmy();
    main->GetFinalDestination();                       
    const bool shouldGather = this->ShouldGather();

    std::map<std::pair<int, int>, CList<CUnit *>> divisionMap;
    int sameLocationExiles = 0;

    for (CUnit *unit : *this->GetArmyList())
    {
        CArmy *army = unit->GetArmy();
        if (army == main)
            continue;

        if (army->GetAttachedTo() != nullptr)
        {
            ::PostAICommand(new CDetachUnitCommand(army));
            continue;
        }

        if (army->IsForceMarch())
            ::PostAICommand(new CForceMarchCommand(army));
        if (army->IsRetreating())
            continue;
        if (army->IsExploring())
            break;
        if (army->InSiege() &&
            !army->AccessSiege()->CanLeaveCombat())
            continue;
        if (army->IsMovementLocked())
            continue;
        if (army->HasLowMorale())
            continue;

        if (army->GetAIDivisionOf() >= 1)
        {
            if (army->InCombat())
                continue;
            this->ProcessDivision(army, divisionMap, AccessCache);
            continue;
        }

        bool invasion = army->IsLoaded();
        if (!invasion)
        {
            if (army->IsMoving())
                invasion = !army->GetFinalDestination()->GetTemplate().IsLand();
        }
        if (invasion)
        {
            if (this->GetInvasionTarget() != 0 && this->GetConsolidationTarget() == 0 &&
                !main->IsLoaded())
            {
                auto head = this->GetArmyList()->begin();
                auto found = head;
                while (*found != army)
                    ++found;
                CUnit *tmp = *head;
                *head = *found;
                *found = tmp;
            }
            else
            {
                this->ProcessSplinterInvasion(army, AccessCache);
            }
            continue;
        }

        if (!this->ProcessSplinter(army, main, shouldGather, AccessCache))
            break;
        if (army->GetLocation() == main->GetLocation())
        {
            sameLocationExiles +=
                (main->HasExileStatus() ^ army->HasExileStatus() ^ 1) & 1;
        }
    }

    if (!divisionMap.empty())
        this->MergeDivisions(divisionMap);
    if (sameLocationExiles > 0 && this->GetInvasionTarget() == 0 && this->GetConsolidationTarget() == 0)
        CAIAgent::MergeArmy(*main);
}

void CAIArmy::ProcessDivision(CArmy *pArmy,
                              std::map<std::pair<int, int>, CList<CUnit *>> &ToMerge,
                              CCountryFlagCache<char> &AccessCache)
{
    if (pArmy->IsMovementLocked())
        return;

    const auto controllerHostile = [](const CCountryTag &selfTag, const CProvince *location) -> bool
    {
        const CCountry *country = CCountryDataBase::AccessInstance()->GetCountry(location->GetController());
        if (CCountryTag::SameUpperWord(selfTag, country->GetCountryTag()))
            return false;
        if (country->IsAtWarWithCountry(selfTag))
            return true;
        return selfTag.IsRebels() || country->GetCountryTag().IsRebels();
    };

    const auto process = [&]() -> bool
    {
        CArmy *main = this->GetMainArmy();
        if (main == nullptr || pArmy->GetLocation() == nullptr)
            return false;
        if (main->IsCondottieri())
            return false;

        const int target = pArmy->GetAIDivisionAt();
        bool viaControllerGate = false;
        bool checkFort = false;
        if (main->IsMoving())
        {
            viaControllerGate = target != 0;
            checkFort = target == 0;
        }
        else if (target != 0 && pArmy->GetAIDivisionOf() != main->GetLocation()->GetProvinceID())
        {
            viaControllerGate = true;
        }

        if (viaControllerGate)
        {
            if (!controllerHostile(this->GetCountryTag(), pArmy->GetLocation()))
                return false;
            checkFort = true;
        }
        if (checkFort && pArmy->GetLocation()->GetFortLevelAfterMothball() > 0)
            return false;
        if (this->_bNeedsHelp != 0)
            return false;

        if (pArmy->GetLocation() != main->GetLocation() &&
            !this->CheckDivisionSafety(pArmy))
            return false;

        bool focalFlag;
        if (this->GetFocalProvince(focalFlag) != nullptr)
            return false;

        CStrengthFactorCache &cache = CStrengthFactorCacheManager::AccessCache();
        if (::IsEasyTargetProvince(*pArmy->GetRealLocation(), pArmy, cache))
            return true;

        const int target2 = pArmy->GetAIDivisionAt();
        if (pArmy->GetLocation()->GetProvinceID() == target2)
        {
            if (pArmy->IsAIDivisionExpectingSiege() != 0 &&
                !controllerHostile(this->GetCountryTag(), pArmy->GetLocation()))
                return false;
            ToMerge[std::make_pair(pArmy->GetAIDivisionAt(), pArmy->GetAIDivisionOf())]
                .Append(pArmy);
            return true;
        }

        CSafeAiPathFind finder;

        CPathFind::SFindSettings settings{};
        settings._nNumRegimentsForLoadingOverride = -1;
        settings._HiredByCountryOverride = CCountryTag::FromRaw(kEmptyCountryTag);
        settings._pAccessCache = &AccessCache;
        settings._bDirectIfAdjacent = true;
        settings._bEnableReturnToPreviousNoZocHax = true;

        if (target2 != 0)
        {
            const AIMoveResult r = this->MoveArmy(*pArmy, target2, AccessCache, 0, false);
            if (r > AIMoveResult::_AI_MOVE_RESULT_NOTNEEDED_)
                this->AbortDivision(pArmy);
            return true;
        }

        const SAdjacency *begin = pArmy->GetLocation()->GetTemplate().GetNeighbours().GetData();
        const SAdjacency *end = (pArmy->GetLocation()->GetTemplate().GetNeighbours().GetData() + pArmy->GetLocation()->GetTemplate().GetNeighbours().GetSize());
        const int n = static_cast<int>(end - begin);
        if (n <= 0)
            return false;

        int idx = this->GetKing().GetRandom() % n;
        for (int i = 0; i < n; i++)
        {
            const SAdjacency *entry = begin + idx;
            const int id = entry->GetProvinceId();
            if (id != 0 && id != main->GetRealLocation()->GetProvinceID() &&
                (entry->GetType() | EAdjacencyType::_ADJACENCY_RIVER_) == EAdjacencyType::_ADJACENCY_RIVER_)
            {
                const CProvince *prov = &entry->GetProvince();
                if (::IsEasyTargetProvince(*prov, pArmy, cache) &&
                    finder.IsValidTarget(pArmy, prov, pArmy->GetLocation(), prov,
                                         *entry, settings))
                {
                    if (id == 0)
                        return false;
                    const AIMoveResult r = this->MoveArmy(*pArmy, id, AccessCache, 0, false);
                    if (r > AIMoveResult::_AI_MOVE_RESULT_NOTNEEDED_)
                        this->AbortDivision(pArmy);
                    return true;
                }
            }
            idx = (idx + 1 >= n) ? 0 : idx + 1;
        }

        return false;
    };
    if (!process())
        this->AbortDivision(pArmy);
}

bool CAIArmy::MakeDivision(CArmy *pMainArmy, int nTargetProv, CCountryFlagCache<char> &AccessCache)
{
    const CProvince *location = pMainArmy->GetLocation();
    const CProvinceTemplate *area = &location->GetTemplate();
    const int count = area->GetNeighbours().GetSize();

    CProvince *candidate = nullptr;
    const SAdjacency *candidateAdj = nullptr;
    float best = 1000000.0f;

    if (nTargetProv != 0)
    {
        candidate = &CCurrentGameState::GetInstance()->GetProvince(nTargetProv);
        for (int i = count - 1; i >= 0; i--)
        {
            const SAdjacency *entry = area->GetNeighbours().GetData() + i;
            if (entry->GetProvinceId() == nTargetProv)
            {
                candidateAdj = entry;
                break;
            }
        }
    }
    else
    {
        for (int i = count - 1; i >= 0; i--)
        {
            const SAdjacency *entry = area->GetNeighbours().GetData() + i;
            if ((entry->GetType() | EAdjacencyType::_ADJACENCY_RIVER_) != EAdjacencyType::_ADJACENCY_RIVER_)
                continue;
            CProvince *prov = &CCurrentGameState::GetInstance()->GetProvince(entry->GetProvinceId());
            if (!prov->GetTemplate().IsLand())
                continue;
            if (!pMainArmy->HaveAccess(prov, false, false, false, CCountryTag::FromRaw(kEmptyCountryTag),
                                       nullptr, false, nullptr))
                continue;
            if (pMainArmy->IsUnsafeMove(entry->GetProvinceId()))
                continue;
            if (::GetLeastCapacityInProvince(this->GetCountryTag(), prov) <= 0)
                continue;

            const bool lowMorale = pMainArmy->HasLowMorale();
            const float threat1 = this->GetThreat(pMainArmy, prov, lowMorale, false);
            CFixedPoint moralePt = pMainArmy->GetAverageMorale(false);
            const float morale = moralePt.GetUnderlyingValue() / kMilli;

            float vTotalBalance;
            bool bFriendsNeedHelp, bFriendsEnteringCombat, bFriendToAttachTo, bIndirectlyThreatened;
            const float score = this->EvaluateProvince(
                pMainArmy, morale, prov, vTotalBalance, bFriendsNeedHelp, bFriendsEnteringCombat, bFriendToAttachTo, false,
                0.0f, threat1, 0.0f, bIndirectlyThreatened, 1.0f, nullptr, nullptr, nullptr, nullptr);
            if (score < best)
            {
                best = score;
                candidate = prov;
                candidateAdj = entry;
            }
        }
    }

    if (candidateAdj == nullptr || candidate == nullptr)
        return false;

    int take = 0;
    const int targetRegiments = ::GetTargetRegimentCount(this->GetCountryTag(), location);
    if (nTargetProv == 0)
    {
        const CFixedPoint friendly = ::GetFriendlyForceStrength(pMainArmy, false);
        int diff = targetRegiments * 1000 - friendly.GetUnderlyingValue();
        if (diff < 1000)
            diff = 1000;
        const int subUnits = pMainArmy->GetNumSubUnits();
        int rounded = diff / 1000 + ((diff % 1000 != 0) ? 1 : 0);
        take = subUnits - rounded;
        const int capacity = ::GetLeastCapacityInProvince(this->GetCountryTag(), candidate);
        if (capacity < take)
            take = capacity;
        if (take < 1)
            return false;
    }
    else
    {
        CFixedPoint total = pMainArmy->GetTotalStrength();
        CFixedPoint maxStrength = pMainArmy->GetMaximumStrength();
        take = (total.GetUnderlyingValue() < maxStrength.GetUnderlyingValue()) ? 2 : 1;
        CFixedPoint total2 = pMainArmy->GetTotalStrength();
        if (take * 1000 + targetRegiments * 1000 < total2.GetUnderlyingValue())
            return false;
    }

    CSafeAiPathFind finder;

    CPathFind::SFindSettings settings{};
    settings._bDirectIfAdjacent = true;
    settings._bEnableReturnToPreviousNoZocHax = true;
    settings._nNumRegimentsForLoadingOverride = -1;
    settings._HiredByCountryOverride = CCountryTag::FromRaw(kEmptyCountryTag);
    settings._pAccessCache = &AccessCache;
    settings._bPathLengthMap = 0;

    if (!finder.IsValidTarget(pMainArmy, candidate, pMainArmy->GetLocation(), candidate,
                              *candidateAdj, settings))
    {
        ::PostAICommand(new CAddUnsafeMoveCommand(pMainArmy, candidate->GetProvinceID(), 1));
        return false;
    }

    const int subUnits = pMainArmy->GetNumSubUnits();
    const int a = (nTargetProv == 0) ? candidate->GetProvinceID() : 0;
    this->SplitOff(pMainArmy, subUnits - take, a, pMainArmy->GetLocation()->GetProvinceID());
    return true;
}

int CAIArmy::TakeFromList(CUnitList *pArmyList, int nNeeded)
{
    int sourceTotal = 0;
    int sourceCounts[3] = {0, 0, 0};
    for (CUnit *unit : *pArmyList)
    {
        if (!unit->IsArmy())
            continue;
        for (int subIndex = 0; subIndex < unit->GetSubUnits().GetSize(); ++subIndex)
        {
            CSubUnit *sub = unit->GetSubUnits()[subIndex];
            sourceTotal++;
            sourceCounts[static_cast<int>(sub->GetDefinition()->GetType())]++;
        }
    }

    int armyTotal = 0;
    int armyCounts[3] = {0, 0, 0};
    for (CUnit *unit : *this->GetArmyList())
    {
        if (!unit->IsArmy())
            continue;
        for (int subIndex = 0; subIndex < unit->GetSubUnits().GetSize(); ++subIndex)
        {
            CSubUnit *sub = unit->GetSubUnits()[subIndex];
            armyTotal++;
            armyCounts[static_cast<int>(sub->GetDefinition()->GetType())]++;
        }
    }

    int remaining = nNeeded;
    std::vector<CUnit *> taken;

    for (auto it = pArmyList->GetTail(); it != pArmyList->end() && remaining > 0; it--)
    {
        CUnit *unit = *it;
        if (unit->InCombat() || unit->IsRetreating() || unit->IsExploring() ||
            unit->GetArmy()->IsLoaded() || unit->GetNumSubUnits() == 0)
            continue;

        const int unitSubs = unit->GetNumSubUnits();
        const int willRemain = remaining - unitSubs;
        if (willRemain < 0 && unit->GetMercenaryCompany() == nullptr &&
            !unit->IsCondottieri())
        {
            const CID newCid = CCurrentGameState::AccessInstance()->GenerateCID(EIDCategory::UNIT, false);
            CAIKing *king = &this->GetKing();
            int prevId = 0;
            if (const CProvince *prev = unit->GetPrevious())
                prevId = prev->GetProvinceID();
            int prevNoZoCId = 0;
            if (const CProvince *prev = unit->GetPreviousNoZoC())
                prevNoZoCId = prev->GetProvinceID();
            king->CreateArmy(newCid, unit->GetLocation(), nullptr,
                             unit->HasExileStatus(), prevId, prevNoZoCId,
                             0, unit->GetWasPrevious() != 0, 0);
            this->_OurArmies.Append(newCid);

            const CArray<CSubUnit *> &subs = unit->GetSubUnits();
            size_t pos[3] = {0, 0, 0};
            while (remaining > 0)
            {
                const float ratio =
                    armyTotal > 0 ? static_cast<float>(sourceTotal) /
                                        static_cast<float>(armyTotal)
                                  : 0.0f;
                int chosen = 3;
                int chosenIndex = -1;
                float bestScore = 0.0f;
                for (int c = 0; c < 3; c++)
                {
                    size_t j = pos[c];
                    const size_t subCount = subs.GetSize();
                    while (j < subCount &&
                           subs[j]->GetDefinition()->GetType() != static_cast<SubUnitType>(c))
                        j++;
                    pos[c] = j;
                    if (j >= subCount)
                        continue;
                    float score = 1.0f;
                    if (armyTotal > 0 && sourceCounts[c] > 0)
                        score = static_cast<float>(armyCounts[c]) /
                                static_cast<float>(sourceCounts[c]) * ratio;
                    if (chosen == 3 || bestScore > score)
                    {
                        chosen = c;
                        chosenIndex = static_cast<int>(j);
                        bestScore = score;
                    }
                }
                if (chosen == 3)
                    break;
                CSubUnit *sub = subs[chosenIndex];
                pos[chosen] = chosenIndex + 1;
                this->GetKing().AssignSubUnitProperly(newCid, sub);
                armyCounts[chosen]++;
                sourceCounts[chosen]--;
                armyTotal++;
                sourceTotal--;
                remaining--;
            }
            remaining = nNeeded;
            break;
        }

        taken.push_back(unit);
        for (int subIndex = 0; subIndex < unit->GetSubUnits().GetSize(); ++subIndex)
        {
            CSubUnit *sub = unit->GetSubUnits()[subIndex];
            const int type = static_cast<int>(sub->GetDefinition()->GetType());
            armyCounts[type]++;
            armyTotal++;
            sourceCounts[type]--;
            sourceTotal--;
        }
        remaining = willRemain;
    }

    for (CUnit *unit : taken)
        this->AssignArmy(*unit->GetArmy());

    return nNeeded - remaining;
}

CProvince *CAIArmy::ShouldReturnToBaseWithBoats(CCountryFlagCache<char> &AccessCache)
{
    CArmy *main = this->GetMainArmy();
    if (main == nullptr || main->IsMoving())
        return nullptr;

    const CCurrentGameState *gameState = CCurrentGameState::GetInstance();
    const CCountryTag allegiance = main->GetAllegiance();
    const CProvince *assigned = &gameState->GetProvince(this->GetBaseProvince());
    CStrengthFactorCache &cache = CStrengthFactorCacheManager::AccessCache();

    CEU3Date horizon = gameState->GetCurrentDate();
    horizon.SetUniversalDate(horizon.GetUniversalDate() + kThreatHorizonHours);
    const float threatAssigned = CAIThreatEvaluation::GetInstance()
                                     .CalcThreatForArmy(this->GetBaseProvince(), horizon, main, true, cache)
                                     .GetUnderlyingValue() /
                                 kMilli;
    const float threatLocation = CAIThreatEvaluation::GetInstance()
                                     .CalcThreatForArmy(main->GetLocation()->GetProvinceID(), horizon, main, true, cache)
                                     .GetUnderlyingValue() /
                                 kMilli;

    CFixedPoint morale = main->GetAverageMorale(false);
    const float moraleF = morale.GetUnderlyingValue() / kMilli;

    bool bFriendsNeedHelp, bFriendsEnteringCombat, bFriendToAttachTo, bIndirectlyThreatened;
    float vTotalBalance;
    const float scoreAssigned = this->EvaluateProvince(
        main, moraleF, assigned, vTotalBalance, bFriendsNeedHelp, bFriendsEnteringCombat, bFriendToAttachTo, false,
        0.0f, threatAssigned, 0.0f, bIndirectlyThreatened, 0.5f, nullptr, nullptr, nullptr, &AccessCache);
    const float scoreLocation = this->EvaluateProvince(
        main, moraleF, main->GetLocation(), vTotalBalance, bFriendsNeedHelp, bFriendsEnteringCombat, bFriendToAttachTo, false,
        0.0f, threatLocation, 0.0f, bIndirectlyThreatened, 0.5f, nullptr, nullptr, nullptr, &AccessCache);

    if (scoreAssigned * 0.9f > scoreLocation)
        return nullptr;

    const CCountry *ourCountry = &this->GetCountry();
    if (CCountryTag::SameUpperWord(allegiance, assigned->GetController()) &&
        this->CanReach(main, assigned, AccessCache))
        return nullptr;
    if (ourCountry->GetUnits().GetTPC() == 0)
        return nullptr;

    CProvince *invasionTarget = this->CalcInvasionTarget(
        gameState->GetProvince(this->GetBaseProvince()).GetControllerArea(), main);

    const CProvince *location = main->GetLocation();
    if (location == nullptr)
        return invasionTarget;
    if (ourCountry->HasUnconditionalSurrender() != 0)
        return invasionTarget;

    const CProvince *gate = location;
    if (location->GetControllerArea() == nullptr)
    {
        const CProvinceTemplate *area = &location->GetTemplate();
        const int count = area->GetNeighbours().GetSize();
        bool found = false;
        for (int i = 0; i < count; i++)
        {
            const SAdjacency *entry = area->GetNeighbours().GetData() + i;
            if (entry->GetType() == EAdjacencyType::_ADJACENCY_IMPASSABLE_)
                continue;
            CProvince *prov = &gameState->GetProvince(entry->GetProvinceId());
            if (prov->GetControllerArea() == nullptr)
                continue;
            if (main->HaveAccess(prov, false, false, false, CCountryTag::FromRaw(kEmptyCountryTag),
                                 nullptr, false, nullptr))
            {
                gate = prov;
                found = true;
                break;
            }
        }
        if (!found)
            gate = location;
    }

    if (!gate->GetOwner().IsValid())
        return invasionTarget;
    CControllerArea *gateArea = gate->GetControllerArea();
    if (gateArea == nullptr)
        return invasionTarget;
    if (invasionTarget == nullptr)
        return nullptr;
    if (gateArea->CalcIsWarZone(allegiance, true))
        return nullptr;
    if (main->InCombat() || main->InSiege())
        return nullptr;

    CControllerArea *assignedArea =
        gameState->GetProvince(this->GetBaseProvince()).GetControllerArea();
    if (assignedArea == nullptr)
        return nullptr;
    if (assignedArea == gateArea)
        return nullptr;
    if (assignedArea->IsAdjacent(*gateArea))
        return nullptr;

    for (CList<CNeighborArea>::Node *cell = gateArea->GetNeighborList().GetHeadNode();
         cell != nullptr; cell = cell->_pNext)
    {
        if (cell->_Data._Area.GetOwner().GetIndex() ==
            ourCountry->GetStrategy().GetWarTarget().GetIndex())
            return nullptr;
    }

    const auto canReturn = [&]() -> bool
    {
        int outLandingForce = 0, outNumSubUnits = 0;
        if (this->CanInvadeBySea(main, invasionTarget, outLandingForce, outNumSubUnits, AccessCache, false))
            return true;
        CList<int> path;
        return this->CanGoByLand(main, invasionTarget, path, AccessCache, -1);
    };
    if (canReturn())
        return invasionTarget;
    return nullptr;
}

int CAIArmy::MainLoop(CCountryFlagCache<char> &AccessCache)
{
    CArmy *main = this->GetMainArmy();
    if (main == nullptr)
    {
        this->UnassignFleets();
        return this->GetRandom() % kMainLoopWakeStride + kMainLoopWakeMin;
    }

    if (this->_bIsExploration != 0)
    {
        this->_StopExplorationReason = EStopMissionReason::CarryOn;
        const CLeader *leader = main->GetLeader();
        if (leader == nullptr)
            this->_StopExplorationReason = EStopMissionReason::NoLeader;
        leader = main->GetLeader();
        if (!leader->CanExplore())                       
            this->_StopExplorationReason = EStopMissionReason::LeaderCannotExplore;
        if (main->HasExileStatus())
        {
            this->_StopExplorationReason = EStopMissionReason::Exiled;
            this->_bIsExploration = 0;
        }
        else if (this->_StopExplorationReason != EStopMissionReason::CarryOn)
        {
            this->_bIsExploration = 0;
        }
    }

    if (!this->IsActive())
        return 1;
    const CCurrentGameState *gameState = CCurrentGameState::GetInstance();
    if (gameState->GetRuntimeSettings()._bArmyAI == 0)
        return 1;

    const int days = gameState->GetCurrentDate().GetTotalDays();
    uint32_t x = static_cast<uint32_t>(days) * 0xeeeeeeefu + 0x8888888u;
    x = (x >> 1) | ((x & 1u) << 31);
    if (x <= 0x8888888u)
        this->DecayUnsafety();

    if (main->GetAttachedTo() == nullptr && this->IsHunterKiller())
    {
        int outMaxPips = 0;
        const int64_t sf = main->GetArmyStrengthFactor(main->GetLocation(), false, true, outMaxPips);
        const CFixedPointFPML strengthFactor = ::FPMLFromRaw(sf);
        const CFixedPoint minStrength(1000);
        if (!this->IsSafeForSiegeForce(main->GetLocation(), main, strengthFactor, minStrength,
                                       AccessCache, true, false))
        {
            this->_bNeedsHelp = 1;
            this->_bWillWaitForHelp = 1;
            this->_nHelpTicks = 7;
        }
        else
        {
            this->_bWillWaitForHelp = 0;
            if (this->_nHelpTicks > 0)
                this->_nHelpTicks--;
            else
                this->_bNeedsHelp = 0;
        }
    }
    else
    {
        this->_bNeedsHelp = 0;
        this->_bWillWaitForHelp = 0;
        this->_nHelpTicks = 0;
    }

    int anyMoved = 0;
    if (this->_nSplitOff < 1)
    {
        std::vector<const CProvince *> locations;
        for (CUnit *unit : *this->GetArmyList())
        {
            if (unit->GetLocation() == nullptr)
                continue;
            CArmy *army = unit->GetArmy();
            if (army->IsLoaded() || unit->HasExileStatus())
                continue;
            bool present = false;
            for (const CProvince *loc : locations)
                if (loc == unit->GetLocation())
                {
                    present = true;
                    break;
                }
            if (present)
                continue;
            if (unit->IsMoving())
            {
                if (!unit->GetFinalDestination()->GetTemplate().IsLand())
                    continue;
                if (!unit->GetNextTarget()->GetTemplate().IsLand())
                    continue;
            }
            locations.push_back(unit->GetLocation());
            if ((anyMoved & 1) == 0)
                anyMoved = this->TryMergeAtLocation(*unit->GetLocation(), this->GetArmyList(), true) ? 1 : 0;
            else
                anyMoved = 1;
        }
    }
    else
    {
        this->_nSplitOff--;
    }

    for (CUnit *unit : *this->GetArmyList())
    {
        const bool hunter = this->IsHunterKiller();
        bool post = false;
        bool isInvasionMain = false;
        if (unit == main)
        {
            isInvasionMain = this->GetInvasionTarget() > 0;
            if (unit->IsMainArmy() == 0)
                post = true;
        }
        else if (unit->IsMainArmy() != 0)
        {
            post = true;
        }
        if (!post)
        {
            const bool unitHunter = (unit->IsHunterKiller() != 0);
            if (hunter != unitHunter)
            {
                post = true;
            }
            else
            {
                const bool unitInvasionMain = (unit->IsInvading() != 0);
                if ((unitInvasionMain ^ isInvasionMain))
                    post = true;
                else
                    continue;
            }
        }
        ::PostAICommand(new CMainArmyCommand(unit, unit == main, hunter, isInvasionMain));
    }

    if (this->GetConsolidationTarget() > 0 && this->GetArmyList()->GetSize() <= 1)
        this->SetConsolidationTarget(0);

    if (!(anyMoved & 1) && this->GetInvasionTarget() == 0 && this->GetConsolidationTarget() == 0 &&
        this->GetFleetList()->GetSize() > 0)
        this->UnassignFleets();

    CCountry *country = &this->GetCountry();
    if (!country->IsAtWar() && this->_CannotInvadeList.GetSize() > 0)
        this->_CannotInvadeList.RemoveAll();

    if (!(anyMoved & 1))
    {
        if (this->GetInvasionTarget() > 0)
        {
            this->ProcessInvasion(AccessCache);
            if (this->IsInvasionInterrupted() != 0)
                this->ProcessMainArmy(AccessCache);
        }
        else
        {
            this->ProcessMainArmy(AccessCache);
        }
        this->ProcessSplinters(AccessCache);
        this->CheckShouldEndSplinterInvasion();
    }

    if (country->IsAtWar())
        return 1;
    if (main->IsCondottieri())
        return 1;
    return this->GetRandom() % kMainLoopWakeStride + kMainLoopWakeMin;
}

CArmy *CAIArmy::GetMainArmy() const
{
    if (this->GetArmyList()->GetSize() < 1)
        return nullptr;

    for (CUnit *unit : *this->GetArmyList())
    {
        CArmy *army = unit->GetArmy();
        if (army->GetMercenaryCompany() == nullptr)
        {
            if (this->_bIsExploration == 0)
                return army;
            const CLeader *leader = army->GetLeader();
            if (leader != nullptr && leader->CanExplore())
                return army;
        }
    }
    return (*this->GetArmyList()->begin())->GetArmy();
}

bool CAIArmy::ShouldGather() const
{
    if (this->GetArmyList()->begin() == this->GetArmyList()->end())
        return false;

    int nonTask = 0;
    for (CUnit *unit : *this->GetArmyList())
        if (unit->GetMercenaryCompany() == nullptr)
            nonTask++;
    if (nonTask < 2)
        return false;

    CArmy *main = this->GetMainArmy();
    if (main == nullptr)
        return false;
    if (main->IsRetreating() || main->IsExploring())
        return false;

    int totalSubUnits = main->GetNumSubUnits();
    int mainSubUnits = main->GetNumSubUnits();
    for (CUnit *unit : *this->GetArmyList())
    {
        if (!unit->IsArmy())
            continue;
        CArmy *unitArmy = unit->GetArmy();
        if (unitArmy == main)
            continue;
        if (unitArmy->GetAIDivisionOf() >= 1)
            continue;
        if (unit->InSiege() || unit->InCombat())
            continue;
        if (unit->IsRetreating() || unit->IsExploring())
            continue;
        if (unit->GetLocation()->GetControllerArea() != main->GetLocation()->GetControllerArea() &&
            unit->GetFinalDestination() != main->GetLocation())
            continue;
        totalSubUnits += unit->GetNumSubUnits();
        if (unit->GetMercenaryCompany() != nullptr)
            mainSubUnits += unit->GetNumSubUnits();
    }

    if (totalSubUnits <= 0)
        totalSubUnits = 1;
    return static_cast<float>(mainSubUnits) / static_cast<float>(totalSubUnits) <
           0.33f;
}

int CAIArmy::CalcGatherProvince() const
{
    CArmy *main = this->GetMainArmy();
    if (main != nullptr && !main->HasExileStatus())
    {
        CCountry *country = CCountryDataBase::AccessInstance()
                                ->GetCountry(this->GetCountryTag());
        if (country->IsAtWar())
        {
            const float threat = const_cast<CAIArmy *>(this)->GetThreat(
                main, main->GetLocation(), main->HasLowMorale(), false);
            if (threat > 0.4f)
                return this->GetBaseProvince();
        }
        CProvince *pick = main->GetLocation();
        if (main->IsMoving())
            pick = main->GetNextTarget();
        return pick->GetProvinceID();
    }
    return this->GetBaseProvince();
}

void CAIArmy::UnassignFleets()
{
    for (auto it = this->GetFleetList()->begin(); it != this->GetFleetList()->end();)
    {
        CUnit *fleet = *it;
        ++it;
        CAIKing *king = &this->GetKing();
        king->GetAdmiral()->AssignFleet(static_cast<CNavy &>(*fleet));
    }
}

void CAIArmy::CheckShouldEndSplinterInvasion()
{
    if (this->GetConsolidationTarget() == 0 ||
        this->GetArmyList()->begin() == this->GetArmyList()->end())
        return;
    const CProvince *target =
        &CCurrentGameState::GetInstance()->GetProvince(this->GetConsolidationTarget());
    bool sawOther = false;
    CArmy *main = this->GetMainArmy();
    for (CUnit *unit : *this->GetArmyList())
    {
        CArmy *army = unit->GetArmy();
        if (army != main)
        {
            sawOther = true;
            if (army->GetAttachedTo() == nullptr &&
                army->GetLocation() != target &&
                army->GetLocation()->GetControllerArea() != target->GetControllerArea())
                return;
        }
    }
    if (sawOther)
        this->SetConsolidationTarget(0);
}

bool CAIArmy::CheckDivisionSafety(const CArmy *army) const
{
    CStrengthFactorCache &cache = CStrengthFactorCacheManager::AccessCache();
    CEU3Date horizon = CCurrentGameState::GetInstance()->GetCurrentDate();
    horizon.SetUniversalDate(horizon.GetUniversalDate() + kThreatHorizonHours);
    const CFixedPoint threat = CAIThreatEvaluation::GetInstance().CalcThreatForArmy(
        army->GetLocation()->GetProvinceID(), horizon, army, true, cache);
    return threat.GetUnderlyingValue() < kMinAvgStrength;
}

void CAIArmy::AbortDivision(CArmy *army)
{
    army->SetAIDivision(0, 0, false);
    if (army->IsMoving())
        this->HaltUnit(*army, EStopMissionReason::Generic);
}

bool CAIArmy::WantsToTakeProvinceEasily(const CArmy *army, const CProvince &prov)
{
    if (!::HasEnoughMenForDivision(army, 2))
        return false;
    CStrengthFactorCache &cache = CStrengthFactorCacheManager::AccessCache();
    return ::IsEasyTargetProvince(prov, army, cache);
}

bool CAIArmy::WantsDivision(const CArmy *pMainArmy, int &nTargetProv)
{
    nTargetProv = 0;
    if (this->_bNeedsHelp != 0 ||
        pMainArmy->IsCondottieri() ||
        pMainArmy->HasExileStatus())
        return false;

    CCountry *country = &this->GetCountry();
    if (country->IsSubject() &&
        country->GetSubjectMilitaryFocus() != ESubjectMilitaryFocus::_FOCUS_NONE_)
        return false;

    CStrengthFactorCache &cache = CStrengthFactorCacheManager::AccessCache();
    const CProvince *location = pMainArmy->GetLocation();
    CEU3Date horizon = CCurrentGameState::GetInstance()->GetCurrentDate();
    horizon.SetUniversalDate(horizon.GetUniversalDate() + kThreatHorizonHours);
    const CFixedPoint threat = CAIThreatEvaluation::GetInstance().CalcThreatForArmy(
        location->GetProvinceID(), horizon, pMainArmy, false, cache);
    if (threat.GetUnderlyingValue() / kMilli > 0.4f)
        return false;

    bool focalFlag;
    if (this->GetFocalProvince(focalFlag) != nullptr)
        return false;

    const CProvinceTemplate *area = &location->GetTemplate();
    const int neighbourCount = area->GetNeighbours().GetSize();
    if (neighbourCount != 0)
    {
        const int idx = this->GetKing().GetRandom() % neighbourCount;
        const SAdjacency *entry = area->GetNeighbours().GetData() + idx;
        if ((entry->GetType() | EAdjacencyType::_ADJACENCY_RIVER_) == EAdjacencyType::_ADJACENCY_RIVER_)
        {
            const CProvince *prov = &entry->GetProvince();
            if (this->WantsToTakeProvinceEasily(pMainArmy, *prov))
            {
                nTargetProv = entry->GetProvinceId();
                return true;
            }
        }
    }

    const int attritionLevel = pMainArmy->GetCachedAttritionLevel().GetUnderlyingValue();
    if (attritionLevel <= 0)
        return false;
    if (pMainArmy->AccessSiege() == nullptr)
    {
        if (this->GetKing().GetRandom() % kDivisionCheckChance != 0)
            return false;
    }

    int target = ::GetTargetRegimentCount(this->GetCountryTag(), location);
    if (pMainArmy->AccessSiege() != nullptr)
    {
        const CCombatant *combat = &pMainArmy->AccessSiege()->GetAttacker();
        if (combat != nullptr &&
            combat->GetParticipatingCountries().GetSize() > 0 &&
            CCountryTag::SameUpperWord(combat->GetParticipatingCountries()[0].GetCountryTag(), this->GetCountryTag()))
        {
            const int raised = static_cast<int>(
                (static_cast<int64_t>(target) * 1000) *
                (static_cast<uint32_t>(attritionLevel) / kAttritionTargetDivisor) / 1000);
            target = raised / 1000 + target +
                     ((raised > 0 && raised % 1000 != 0) ? 1 : 0);
        }
    }

    const int needed = (target > 0) ? target : 1;
    return ::HasEnoughMenForDivision(pMainArmy, needed);
}

bool SFleetInfo::operator<(const SFleetInfo &Other) const
{
    const AIMoveResult state = this->_MoveResult;
    const AIMoveResult otherState = Other._MoveResult;
    if ((state == AIMoveResult::_AI_MOVE_RESULT_NOPATH_) != (otherState == AIMoveResult::_AI_MOVE_RESULT_NOPATH_))
        return state != AIMoveResult::_AI_MOVE_RESULT_NOPATH_;
    const bool thisAtZone = state < AIMoveResult::_AI_MOVE_RESULT_NOPATH_;
    const bool otherAwayOrDetached = otherState > AIMoveResult::_AI_MOVE_RESULT_NOTNEEDED_;
    if (thisAtZone == otherAwayOrDetached)
        return thisAtZone;
    if (this->_vDistance.GetUnderlyingValue() != Other._vDistance.GetUnderlyingValue())
        return this->_vDistance.GetUnderlyingValue() < Other._vDistance.GetUnderlyingValue();
    return this->_pNavy->GetID().GetID() < Other._pNavy->GetID().GetID();
}
