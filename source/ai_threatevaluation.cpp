#include "ai_threatevaluation.h"
#include "adjacency.h"
#include "army.h"
#include "country.h"
#include "countrydiplomacy.h"
#include "gamestate.h"
#include "navy.h"
#include "provincetemplate.h"
#include "strength_factor_cache.h"
#include <cmath>
#include <queue>
#include <set>

void CAIThreatEvaluation::UpdateArmy(const CArmy *pArmy)
{
    if (!pArmy || !this->_bIsInitialized)
        return;

    CID cid = pArmy->GetID();
    std::pair<uint8_t, CEU3Date> &stamp = this->_ArmyVersions.try_emplace(cid, uint8_t{0}, CCurrentGameState::GetInstance()->GetCurrentDate()).first->second;
    stamp.first += 1;
    stamp.second = CCurrentGameState::GetInstance()->GetCurrentDate();

    if (pArmy->GetMission() != nullptr || pArmy->HasExileStatus())
        return;

    CPdxHybridArray<const CProvince *, 3, int> seeds{};
    seeds.Append(pArmy->GetLocation());
    if (pArmy->IsMovementLocked())
        seeds.Append(pArmy->GetNextTarget());
    if (pArmy->IsRelocating())
        seeds.Append(pArmy->GetRelocationLocation());

    int max_depth = 4;
    if (!pArmy->GetLocation()->GetTemplate().IsLand() ||
        pArmy->GetCountry().IsRebels())
        max_depth = 1;

    std::priority_queue<std::pair<NAiThreat::SThreatEntry, int>> queue;
    std::set<int> visited;

    int base_days = CCurrentGameState::GetInstance()->GetCurrentDate().GetTotalDays() - this->_CompareDate.GetTotalDays();
    NAiThreat::SThreatEntry seed;
    seed._date = static_cast<int16_t>(base_days);
    seed._nSteps = 0;
    seed._nArmyVersion = stamp.first;
    seed._UnitID = cid;
    seed._bCurrentlyBlocked = 0;
    queue.push({seed, pArmy->GetLocation()->GetProvinceID()});

    if (pArmy->IsMovementLocked())
    {
        CProvince *target = pArmy->GetNextTarget();
        if (target != nullptr)
        {
            CEU3Date arrival = pArmy->GetEstimatedArrival(nullptr, false);
            NAiThreat::SThreatEntry e;
            e._date = static_cast<int16_t>(arrival.GetTotalDays() - this->_CompareDate.GetTotalDays());
            e._nSteps = 0;
            e._nArmyVersion = stamp.first;
            e._UnitID = cid;
            e._bCurrentlyBlocked = 0;
            queue.push({e, target->GetProvinceID()});
        }
    }

    if (pArmy->IsRelocating())
    {
        NAiThreat::SThreatEntry e;
        e._date = static_cast<int16_t>(pArmy->GetRelocationDate().GetTotalDays() - this->_CompareDate.GetTotalDays());
        e._nSteps = 0;
        e._nArmyVersion = stamp.first;
        e._UnitID = cid;
        e._bCurrentlyBlocked = 0;
        queue.push({e, pArmy->GetRelocationLocation()->GetProvinceID()});
    }

    while (!queue.empty())
    {
        NAiThreat::SThreatEntry entry = queue.top().first;
        int province_id = queue.top().second;
        queue.pop();

        visited.insert(province_id);
        CArray<NAiThreat::SThreatEntry> &entries = this->_ArmyThreatArray[province_id];
        entries.Append(entry);

        if (entry._nSteps >= max_depth || entry._bCurrentlyBlocked != 0)
            continue;

        CProvince &cur = CCurrentGameState::GetInstance()->GetProvince(province_id);

        const SAdjacency *neighbours_begin = cur.GetTemplate().GetNeighbours().GetData();
        const SAdjacency *neighbours_end = cur.GetTemplate().GetNeighbours().GetData() + cur.GetTemplate().GetNeighbours().GetSize();

        CPdxArray<::CNeighborFinder::SResultType, int> results{};
        ::CNeighborFinder finder;

        for (auto it = neighbours_begin; it != neighbours_end; ++it)
        {
            CProvince &neighbour = it->AccessProvince();
            CPathFind::SFindSettings settings{};
            settings._bDirectIfAdjacent = true;
            settings._bEnableReturnToPreviousNoZocHax = true;
            settings._nNumRegimentsForLoadingOverride = -1;
            settings._HiredByCountryOverride = CCountryTag::FromRaw(0x2d2d2d);
            bool reachable = finder.IsValidTarget(pArmy, &neighbour, &cur,
                                                  &neighbour, *it, settings);
            uint8_t entering = 1;
            if (!reachable)
            {
                settings._bIgnoreAccess = 1;
                if (!finder.IsValidTarget(pArmy, &neighbour, &cur,
                                          &neighbour, *it, settings))
                    continue;
                entering = 0;
            }
            int travel_days = pArmy->GetTravelTime(&neighbour, &cur).GetUnderlyingValue() / 1000;
            results.Append({it->GetProvinceId(), static_cast<int16_t>(travel_days), entering != 0});
        }
        for (const ::CNeighborFinder::SResultType &res : results)
        {
            if (visited.count(res._nProvince))
                continue;
            NAiThreat::SThreatEntry next;
            next._date = static_cast<int16_t>(res._nDays + entry._date);
            next._nSteps = static_cast<uint8_t>(entry._nSteps + 1);
            next._nArmyVersion = stamp.first;
            next._UnitID = cid;
            next._bCurrentlyBlocked = static_cast<uint8_t>(res._bHasAccess ^ 1);
            queue.push({next, res._nProvince});
        }
    }
}

void CAIThreatEvaluation::UpdateNavy(const CNavy *pNavy)
{
    if (!pNavy || !this->_bIsInitialized)
        return;

    CID cid = pNavy->GetID();
    std::pair<uint8_t, CEU3Date> &stamp = this->_ArmyVersions.try_emplace(cid, uint8_t{0}, CCurrentGameState::GetInstance()->GetCurrentDate()).first->second;
    stamp.first += 1;
    stamp.second = CCurrentGameState::GetInstance()->GetCurrentDate();

    if (pNavy->GetMission() != nullptr)
        return;

    CArray<CProvince *> seeds;
    seeds.Append(pNavy->GetLocation());
    if (pNavy->IsMovementLocked())
        seeds.Append(pNavy->GetNextTarget());

    constexpr int max_depth = 4;
    std::priority_queue<std::pair<NAiThreat::SThreatEntry, int>> queue;
    std::set<int> visited;

    int base_days = CCurrentGameState::GetInstance()->GetCurrentDate().GetTotalDays() - this->_CompareDate.GetTotalDays();
    NAiThreat::SThreatEntry seed;
    seed._date = static_cast<int16_t>(base_days);
    seed._nSteps = 0;
    seed._nArmyVersion = stamp.first;
    seed._UnitID = cid;
    seed._bCurrentlyBlocked = 0;
    queue.push({seed, pNavy->GetLocation()->GetProvinceID()});

    if (pNavy->IsMovementLocked())
    {
        CProvince *target = pNavy->GetNextTarget();
        if (target != nullptr)
        {
            CEU3Date arrival = pNavy->GetEstimatedArrival(nullptr, false);
            NAiThreat::SThreatEntry e;
            e._date = static_cast<int16_t>(arrival.GetTotalDays() - this->_CompareDate.GetTotalDays());
            e._nSteps = 0;
            e._nArmyVersion = stamp.first;
            e._UnitID = cid;
            e._bCurrentlyBlocked = 0;
            queue.push({e, target->GetProvinceID()});
        }
    }

    while (!queue.empty())
    {
        NAiThreat::SThreatEntry entry = queue.top().first;
        int province_id = queue.top().second;
        queue.pop();

        visited.insert(province_id);
        CArray<NAiThreat::SThreatEntry> &entries = this->_NavyThreatArray[province_id];
        entries.Append(entry);

        if (entry._nSteps >= max_depth || entry._bCurrentlyBlocked != 0)
            continue;

        CProvince &cur = CCurrentGameState::GetInstance()->GetProvince(province_id);
        const SAdjacency *neighbours_begin = cur.GetTemplate().GetNeighbours().GetData();
        const SAdjacency *neighbours_end = cur.GetTemplate().GetNeighbours().GetData() + cur.GetTemplate().GetNeighbours().GetSize();

        CPdxArray<::CNeighborFinder::SResultType, int> results{};
        ::CNeighborFinder finder;

        for (auto it = neighbours_begin; it != neighbours_end; ++it)
        {
            CProvince &neighbour = it->AccessProvince();
            CPathFind::SFindSettings settings{};
            settings._bDirectIfAdjacent = true;
            settings._bEnableReturnToPreviousNoZocHax = true;
            settings._nNumRegimentsForLoadingOverride = -1;
            settings._HiredByCountryOverride = CCountryTag::FromRaw(0x2d2d2d);
            if (!finder.IsValidTarget(pNavy, &neighbour, &cur, &neighbour, *it, settings))
                continue;
            int travel_days = pNavy->GetTravelTime(&neighbour, &cur).GetUnderlyingValue() / 1000;
            results.Append({it->GetProvinceId(), static_cast<int16_t>(travel_days), true});
        }

        for (const ::CNeighborFinder::SResultType &res : results)
        {
            if (visited.count(res._nProvince))
                continue;
            NAiThreat::SThreatEntry next;
            next._date = static_cast<int16_t>(res._nDays + entry._date);
            next._nSteps = static_cast<uint8_t>(entry._nSteps + 1);
            next._nArmyVersion = stamp.first;
            next._UnitID = cid;
            next._bCurrentlyBlocked = static_cast<uint8_t>(res._bHasAccess ^ 1);
            queue.push({next, res._nProvince});
        }
    }
}

CFixedPoint CAIThreatEvaluation::CalcThreatForArmy(
    int nProvinceID, const CEU3Date &maxDate,
    const CArmy *pArmy, bool bIncludeNeutral,
    CStrengthFactorCache &Cache) const
{
    int floor = 0;
    int64_t eff_raw = ::RawFPML(Cache.GetStrengthFactor(pArmy, floor));

    float eff = eff_raw >> 15;

    NAiThreat::SUnitEvaluatorForUnit evaluator = {
        pArmy,
        bIncludeNeutral,
        Cache,
        eff};
    return this->CalcThreatForUnit(this->_ArmyThreatArray, nProvinceID, maxDate,
                                   pArmy, bIncludeNeutral, Cache, evaluator);
}

CFixedPoint CAIThreatEvaluation::CalcThreatForArmy(
    int nProvinceID, const CEU3Date &maxDate,
    const CArmy *pArmy, bool bIncludeNeutral,
    CStrengthFactorCache &Cache, CFixedPoint eff) const
{
    NAiThreat::SUnitEvaluatorForUnit evaluator = {
        pArmy, bIncludeNeutral, Cache,
        eff.GetUnderlyingValue() / kMilli};
    return this->CalcThreatForUnit(this->_ArmyThreatArray, nProvinceID, maxDate,
                                   pArmy, bIncludeNeutral, Cache, evaluator);
}

bool NAiThreat::SUnitEvaluatorForUnit::ShouldAssumeLessDistance(const CUnit *pUnit) const
{
    CCountry *me = CCountryDataBase::AccessInstance()->GetCountry(this->_pOwnUnit->GetAllegiance());
    CCountryTag other = pUnit->GetAllegiance();

    if (!other.IsValid())
        return false;
    if (other.GetCountryType() == CountryType::_COUNTRY_OBSERVER_)
        return false;
    int idx = other.GetIndex();
    if (other.IsRebelPirateNative())
        return false;

    if (idx == me->GetCountryTag().GetIndex())
        return false;

    bool at_war = me->IsAtWarWithCountry(other);
    if (!at_war)
    {
        return me->GetCountryTag().IsRebelPirateNative();
    }
    return true;
}

void NAiThreat::SUnitEvaluatorForUnit::EvaluateUnit(const CUnit *pUnit, std::pair<float, float> &Strength, float fMultiplier)
{
    if (this->_pOwnUnit == pUnit)
        return;

    int floor = 0;
    int64_t unit_eff = ::RawFPML(this->_Cache.GetStrengthFactor(pUnit, floor));
    fMultiplier = (unit_eff >> 15) * fMultiplier;

    CCountry *me = CCountryDataBase::AccessInstance()->GetCountry(this->_pOwnUnit->GetAllegiance());
    CCountryTag other = pUnit->GetAllegiance();
    int idx = other.GetIndex();

    bool special_pair = idx == me->GetCountryTag().GetIndex();
    if (!special_pair)
    {
        bool at_war = me->IsAtWarWithCountry(other);
        if (!at_war)
        {
            bool other_special = other.IsRebelPirateNative();
            bool me_special = me->GetCountryTag().IsRebelPirateNative();
            special_pair = !other_special && !me_special;
        }
    }

    if (special_pair)
    {
        bool co_fighting = this->_pOwnUnit->GetAllegiance().GetIndex() == other.GetIndex() ||
                           me->GetDiplomacy()[idx]->IsFightingWarTogether();
        if (co_fighting)
        {
            Strength.first += fMultiplier;
            return;
        }
        if (!this->_bIncludeNeutral)
            return;
        if (NAiThreat::CanTrustCountry(*me, *CCountryDataBase::AccessInstance()->GetCountry(other)))
            return;
        fMultiplier *= 0.25f;
    }
    Strength.second += fMultiplier;
}

void NAiThreat::SUnitEvaluatorForCountry::EvaluateUnit(const CUnit *pUnit,
                                                       std::pair<float, float> &Strength, float fMultiplier)
{
    int floor = 0;
    int64_t unit_eff = ::RawFPML(this->_Cache.GetStrengthFactor(pUnit, floor));
    fMultiplier = (unit_eff >> 15) * 0.01f * fMultiplier;

    CCountry *me = CCountryDataBase::AccessInstance()->GetCountry(this->_Tag);
    CCountryTag other = pUnit->GetAllegiance();
    int idx = other.GetIndex();

    bool special_pair = idx == me->GetCountryTag().GetIndex();
    if (!special_pair)
    {
        bool at_war = me->IsAtWarWithCountry(other);
        if (!at_war)
        {
            bool other_special = other.IsRebelPirateNative();
            bool me_special = me->GetCountryTag().IsRebelPirateNative();
            special_pair = !other_special && !me_special;
        }
    }

    if (special_pair)
    {
        if (!this->_bIncludeNeutral)
            return;
        if (NAiThreat::CanTrustCountry(*me, *CCountryDataBase::AccessInstance()->GetCountry(other)))
            return;
        fMultiplier *= 0.25f;
    }
    Strength.second += fMultiplier;
}

bool NAiThreat::CanTrustCountry(const CCountry &Me, const CCountry &Them)
{
    if (Me.HasDefensivePact(Them.GetCountryTag()))
        return true;

    const CDiplomacyStatus *status = Them.GetDiplomacy()[Me.GetCountryTag().GetIndex()];
    if (status->HasRoyalMarriage())
        return true;

    if (status->HasTruce())
    {
        int today = CCurrentGameState::GetInstance()->GetCurrentDate().GetUniversalDate();
        int expiry = status->GetLastExpireTruceDate().GetUniversalDate();
        if (today + 0x10e0 < expiry)
            return true;
    }

    if (CCountryTag::SameUpperWord(Me.GetOverlord(), Them.GetOverlord()))
    {
        if (Them.DoesSubjectTypeProhibitWarWith(Me.GetCountryTag(), nullptr))
            return true;
    }
    return false;
}
CUnit *NAiThreat::SThreatEntry::GetUnit() const
{
    CReferenceObject *object = CReferenceObject::GetObjectFromID(this->_UnitID);
    return object == nullptr ? nullptr : static_cast<CUnit *>(object);
}
