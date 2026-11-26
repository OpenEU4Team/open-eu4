#pragma once
#include "ai_army.h"
#include "ai_sharedeval.h"
#include "ai_threatevaluation.h"
#include "army.h"
#include "combat.h"
#include "country.h"
#include "defines.h"
#include "engine.h"
#include "gamestate.h"
#include "leader.h"
#include "province.h"
#include "provincetemplate.h"
#include "rebelfaction.h"
#include "strength_factor_cache.h"
#include "technology.h"
#include <algorithm>
#include <cmath>

template <typename T>
bool CAISharedEval::IsBalanceAcceptable(const CProvince *pProv,
                                        const CArmy *pArmy,
                                        CCountryTag enemy,
                                        bool bWantsCombat,
                                        bool bEnemyArrivesBeforeUs,
                                        T vFriendlyStrength,
                                        T vExtraFriendlyStrength,
                                        T vExtraExtraFriendlyStrength,
                                        int nFriendlyForce,
                                        int nExtraFriendlyForce,
                                        int nExtraExtraFriendlyForce,
                                        T vEnemyStrength,
                                        int nEnemyForce,
                                        int nOurBestLeaderPips,
                                        int nTheirBestLeaderPips,
                                        int nOurTerrainAdvantage,
                                        T &vTotalBalance,
                                        CStrengthFactorCache &StrengthFactorCache,
                                        T vAcceptableBalanceBase,
                                        int nRebelForce,
                                        T vProvThreat,
                                        CString *pDesc)
{
    (void)vExtraExtraFriendlyStrength;
    (void)nExtraExtraFriendlyForce;
    (void)pDesc;

    if (vEnemyStrength <= 0.0f ||
        (vProvThreat == 0.0f && !std::isnan(vProvThreat)))
        return true;

    const CCountry *me = CCountryDataBase::AccessInstance()->GetCountry(pArmy->GetAllegiance());
    int best_enemy_pips = nTheirBestLeaderPips;
    for (CCountryTag enemyTag : me->GetCurrentAtWarWith())
    {
        CCountry *hostile = CCountryDataBase::AccessInstance()->GetCountry(enemyTag);
        const CLeader *leader = hostile->GetBestReassignableArmyLeader();
        if (!leader->IsValid())
            continue;

        CUnit *unit = leader->GetUnit();
        bool tag_match = enemyTag.GetIndex() == enemy.GetIndex();
        bool counted = false;
        if (tag_match && !bEnemyArrivesBeforeUs)
        {
            if (!unit)
            {
                counted = true;
            }
            else
            {
                CCountry *owner = CCountryDataBase::AccessInstance()->GetCountry(
                    unit->GetLocation()->GetController());
                if (owner->GetCountryTag().GetIndex() == enemy.GetIndex())
                {
                    counted = false;
                }
                else
                {
                    counted = true;
                    bool enemy_special = enemyTag.IsRebelPirateNative();
                    if (!enemy_special)
                    {
                        counted = !owner->GetCountryTag().IsRebelPirateNative();
                    }
                }
            }
        }
        bool near = false;
        if (unit)
        {
            near = unit->GetLocation()->GetDistance(pProv->GetProvinceID(), false).GetUnderlyingValue() < kBalanceNearDist;
        }
        if (counted || near)
        {
            int pips = leader->GetFire() + leader->GetShock();
            if (best_enemy_pips < pips)
                best_enemy_pips = pips;
        }
    }

    CFixedPoint vMyAverageMorale = pArmy->GetAverageMorale(false);

    int floor = 0;
    int64_t eff = RawFPML(StrengthFactorCache.GetStrengthFactor(pArmy, floor));
    int64_t raw2 = ((eff * vMyAverageMorale.GetUnderlyingValue() >> 15) * vMyAverageMorale.GetUnderlyingValue()) >> 15;
    float vMyModifiedStrengthFactor = static_cast<float>(raw2) * kStrengthToFloat;

    float vFriendlyTotalStrength = vMyModifiedStrengthFactor + vFriendlyStrength + vExtraFriendlyStrength;
    if (kHundredStrengthCmp * vFriendlyTotalStrength < vEnemyStrength)
        return false;

    float vDirectBalance = (vFriendlyTotalStrength + kDirectBalanceEpsilon) / (kDirectBalanceEpsilon + vEnemyStrength);

    T acc = vAcceptableBalanceBase;
    if (!bWantsCombat)
        acc *= kNotLookingForTrouble;

    int width = std::max(CCountryDataBase::AccessInstance()->GetCountry(pArmy->GetTechCountry())->GetCombatWidth().GetUnderlyingValue(), 20000);
    CFixedPoint totalStrength = pArmy->GetTotalStrength();
    CFixedPoint attachedStrength = pArmy->GetAttachedTotalStrength(false);
    int engaged = (nExtraFriendlyForce + nFriendlyForce) * 1000 + totalStrength.GetUnderlyingValue() + attachedStrength.GetUnderlyingValue();
    int engaged_10 = engaged + 10;
    int flank_10 = nEnemyForce * 1000 + 10;
    if (flank_10 < width || engaged_10 < width)
    {
        int total_cap = nEnemyForce * 1000 + kEngagedCapPad;
        if (engaged_10 <= total_cap)
            total_cap = engaged_10;
        if (width <= total_cap)
            total_cap = width;
        int engaged_cap = engaged + kEngagedCapPad;
        if (flank_10 <= engaged_cap)
            engaged_cap = flank_10;
        if (width <= engaged_cap)
            engaged_cap = width;
        if (engaged_cap == 0)
        {
            engaged_cap = -1;
        }
        else if (::IsValidRange(total_cap))
        {
            engaged_cap = (total_cap * 1000) / engaged_cap;
        }
        else
        {
            engaged_cap = static_cast<int>((static_cast<int64_t>(total_cap) * 1000) / engaged_cap);
        }
        acc *= kOne / (engaged_cap / kMilli);
    }

    int cav = pArmy->GetLocation()->GetTemplate().IsLand()
                  ? nOurTerrainAdvantage
                  : nOurTerrainAdvantage - 2;
    int pips_diff = nOurBestLeaderPips - best_enemy_pips;
    int d_high = 15;
    int d_low = 15 - pips_diff;
    if (pips_diff > 0)
    {
        d_high = pips_diff + 15;
        d_low = 15;
    }
    float vLeaderAndTerrainFactor = (d_high + cav * 2) / 15.0f / ((d_low + cav * -2) / 15.0f);
    acc /= vLeaderAndTerrainFactor;

    if (nRebelForce >= 0 && nRebelForce == nEnemyForce)
    {
        if (pProv->GetOwnerArea() == me->GetHomeOwnerArea())
            acc *= kAssignedShare;
    }

    float ThreatWeight = NDefines::NAI::ACCEPTABLE_BALANCE_THREAT_WEIGHT.GetUnderlyingValue() / kMilli;
    float vNecessaryBalance = (vProvThreat >= 0.0f)
                                  ? (ThreatWeight + kOne) / (ThreatWeight * vProvThreat + kOne / vDirectBalance)
                                  : vDirectBalance;
    vTotalBalance = vNecessaryBalance;

    return acc <= vNecessaryBalance;
}

template <typename T>
bool CAISharedEval::IsSafeMove(const CArmy *pArmy, const CProvince *pTarget,
                               CStrengthFactorCache &StrengthFactorCache, const SServerOnlyThreatData &ServerOnlyThreatData,
                               T vAcceptableBalanceBase)
{
    if (pArmy->HasExileStatus() && !pArmy->WillClearExile(pTarget, nullptr))
        return true;

    int nMinEnemies = (CCountryDataBase::AccessInstance()->GetCountry(pArmy->GetTechCountry())->GetTechnologyStatus()->GetSupplyLimit().GetUnderlyingValue() * 20 + 20000) / 5000;
    if (nMinEnemies < 1)
        nMinEnemies = 1;

    CCountryTag enemy;
    CAISharedEval::DetermineEnemy(enemy, pArmy, pArmy->GetAllegiance(), pTarget);
    int enemyEta = CAISharedEval::FindEnemyETA(pArmy, pTarget, nMinEnemies);

    CPdxArray<CUnit *, int> list_a, list_b;
    bool enemySpotted = false;
    bool hostileEngaged = false;
    bool inCombat = false;
    bool subjectContext = false;
    bool biggerFriendlyArmy = false;
    int sameSideUnits = 0;
    int besiegingStrength = 0;
    int foreignBesieging = 0;
    int friendlyUnits = 0;
    float friendlyStrength = 0;
    int ourPips = 0;
    int enemyPipsFloor = 0;
    int flankingStrength = 0;
    int engagedUnits = 0;
    int extraEngagedUnits = 0;
    float extraFriendlyStrength = 0;
    float extraExtraFriendlyStrength = 0;
    float enemyStrengthTotal = 0;
    int nRebelForce = 0;
    float vRebelStrength = 0;
    int siegeExtraUnits = 0;
    bool outHostileAdjacent = false;
    bool outOwnOrWarAdjacent = false;
    bool outAdjacentFort = false;
    bool outUnalliedOwner = false;
    bool outMothballedAdjacent = false;
    bool movingRebelsNatives = false;
    bool outMovingDependentRebels = false;
    int nNextToSiegeForce;

    CAISharedEval::EvaluateUnitsInProvince(pTarget, pArmy, enemy, list_a, list_b,
                                           enemySpotted, inCombat, hostileEngaged, biggerFriendlyArmy, subjectContext,
                                           sameSideUnits, besiegingStrength, foreignBesieging,
                                           friendlyUnits, friendlyStrength,
                                           ourPips, enemyPipsFloor, nRebelForce,
                                           flankingStrength, vRebelStrength, enemyStrengthTotal,
                                           enemyEta, StrengthFactorCache);
    CAISharedEval::EvaluateUnitsMovingIntoProvince(pTarget, pArmy, enemy, list_a, list_b,
                                                   false, enemySpotted,
                                                   ourPips, enemyPipsFloor, nRebelForce, flankingStrength,
                                                   enemyStrengthTotal, nMinEnemies,
                                                   engagedUnits, extraEngagedUnits,
                                                   extraFriendlyStrength, extraExtraFriendlyStrength,
                                                   siegeExtraUnits,
                                                   outHostileAdjacent, outOwnOrWarAdjacent, outAdjacentFort,
                                                   outUnalliedOwner, outMothballedAdjacent, movingRebelsNatives,
                                                   outMovingDependentRebels, nNextToSiegeForce,
                                                   inCombat, hostileEngaged, friendlyUnits, StrengthFactorCache);

    int terrainAdvantage = CAISharedEval::GetOurTerrainAdvantage(pArmy, pTarget, false, false,
                                                                 movingRebelsNatives || enemyStrengthTotal > 0.0f, enemy);

    if (CAISharedEval::ShouldAvoidStrongNatives(pTarget, pArmy))
        return false;

    float threat = 0.0f;
    if (ServerOnlyThreatData._bEnabled)
    {
        int floor = 0;
        int64_t eff_raw = RawFPML(StrengthFactorCache.GetStrengthFactor(pArmy, floor));
        float eff = static_cast<float>(eff_raw) * kStrengthToFloat;

        if (ServerOnlyThreatData._pPrecalculatedThreat)
        {
            threat = *ServerOnlyThreatData._pPrecalculatedThreat;
        }
        else
        {
            float scaled = kMilli * eff + kHalf;
            int effMilli = static_cast<int>(eff >= 0.0f ? scaled : std::floor(scaled));
            CEU3Date maxDate = CCurrentGameState::GetInstance()->GetCurrentDate();
            maxDate.SetUniversalDate(maxDate.GetUniversalDate() + kThreatHorizonHours);
            CFixedPoint armyStrength;
            armyStrength.SetUnderlyingValue(effMilli);
            CFixedPoint t = CAIThreatEvaluation::GetInstance().CalcThreatForArmy(
                pTarget->GetProvinceID(),
                maxDate,
                pArmy, false, StrengthFactorCache, armyStrength);
            threat = t.GetUnderlyingValue() / kMilli;
        }

        if (ServerOnlyThreatData._vCurrentThreat < threat &&
            CAISharedEval::CalcAcceptableThreat(pTarget, pArmy, eff, friendlyStrength, enemyStrengthTotal, flankingStrength, besiegingStrength, enemySpotted, hostileEngaged, subjectContext, enemy) < threat)
            return false;
    }

    T outBalance = kOne;
    T acc = vAcceptableBalanceBase;
    if (hostileEngaged || inCombat)
        acc *= NDefines::NAI::ACCEPTABLE_BALANCE_MULT_FRIEND_IN_COMBAT.GetUnderlyingValue() / kMilli;
    return CAISharedEval::IsBalanceAcceptable(pTarget, pArmy, enemy,
                                              false,
                                              enemySpotted,
                                              friendlyStrength,
                                              extraFriendlyStrength,
                                              extraExtraFriendlyStrength,
                                              friendlyUnits,
                                              engagedUnits,
                                              extraEngagedUnits,
                                              enemyStrengthTotal,
                                              flankingStrength,
                                              ourPips,
                                              enemyPipsFloor,
                                              terrainAdvantage,
                                              outBalance, StrengthFactorCache, acc, -1,
                                              threat,
                                              nullptr);
}
template <typename T>
void CAISharedEval::EvaluateUnitsInProvince(const CProvince *pProv, const CArmy *pArmy,
                                            CCountryTag enemy,
                                            CPdxArray<CUnit *, int> &Counted, CPdxArray<CUnit *, int> &CountedFriendlyUnits,
                                            bool &bEnemyArrivesBeforeUs, bool &bFriendInCombat, bool &bFriendBeingAttacked, bool &bMySplinterHere,
                                            bool &bOverlordHere,
                                            int &nMyForce, int &nSiegeForce,
                                            int &nSiegeForceNotMe, int &nFriendlyForce,
                                            T &vFriendlyStrength,
                                            int &nOurBestLeaderPips, int &nTheirBestLeaderPips, int &nRebelForce,
                                            int &nEnemyForce, T &vRebelStrength,
                                            T &vEnemyStrength,
                                            int nEnemyETA, CStrengthFactorCache &StrengthFactorCache)
{
    const CCountry *me = CCountryDataBase::AccessInstance()->GetCountry(pArmy->GetAllegiance());

    int eta_days = 100000;
    if (pArmy->GetLocation() != nullptr)
    {
        for (auto it = pArmy->GetLocation()->GetTemplate().GetNeighbours().GetData();
             it != pArmy->GetLocation()->GetTemplate().GetNeighbours().GetData() + pArmy->GetLocation()->GetTemplate().GetNeighbours().GetSize(); ++it)
        {
            if (it->GetProvinceId() == pProv->GetProvinceID())
            {
                CEU3Date arrival = pArmy->GetEstimatedArrival(pProv, false);
                eta_days = CGregorianDate(arrival).GetTotalDays() - CGregorianDate(CCurrentGameState::GetInstance()->GetCurrentDate()).GetTotalDays();
                break;
            }
        }
    }

    const bool no_follow_focus =
        NSubjectBehavior::WantsToFollowFocus(pArmy) == 0;

    for (CUnit *unit : pProv->GetUnits())
    {
        if (unit->IsNavy() || unit == pArmy)
            continue;
        if (unit->IsRetreating() && unit->GetFinalDestination() != pProv)
            continue;
        if (unit->HasExileStatus() &&
            !CCountryTag::SameUpperWord(pProv->GetController(), unit->GetAllegiance()) &&
            !CCountryTag::SameUpperWord(pProv->GetOwner(), unit->GetAllegiance()))
            continue;

        if (unit->GetCountry().IsRebels())
        {
            CRebelFaction *faction = unit->GetRebelFaction();
            if (faction != nullptr && faction->IsFriendly(pArmy->GetAllegiance()))
                continue;
        }

        if (unit->InSiege())
        {
            int n = unit->GetNumSubUnits();
            CFixedPoint avg = unit->GetAverageStrength();
            nSiegeForce += ::Div1000(static_cast<int64_t>(n) * 1000 * avg.GetUnderlyingValue()) / 1000;
            if (!CCountryTag::SameUpperWord(unit->GetCountry(), pArmy->GetCountry()))
                nSiegeForceNotMe += ::Div1000(static_cast<int64_t>(n) * 1000 * avg.GetUnderlyingValue()) / 1000;
        }

        CCountryTag unit_tag = unit->GetAllegiance();
        if (me->IsWarFriend(unit_tag, enemy))
        {
            int arrival = 100000000;
            if (unit->IsMoving())
            {
                CEU3Date a = unit->GetEstimatedArrival(nullptr, false);
                arrival = CGregorianDate(a).GetTotalDays() - CGregorianDate(CCurrentGameState::GetInstance()->GetCurrentDate()).GetTotalDays();
            }
            if (nEnemyETA < arrival || unit->InCombat())
            {
                const CCountry *unit_country = CCountryDataBase::AccessInstance()->GetCountry(
                    unit->GetCountry());
                if (CCountryTag::SameUpperWord(pArmy->GetCountry(), unit->GetCountry()) ||
                    CCountryTag::SameUpperWord(unit_country->GetOverlord(), pArmy->GetCountry()))
                    nMyForce += unit->GetNumSubUnits();
                nFriendlyForce += unit->GetNumSubUnits();
                if (CCountryTag::SameUpperWord(unit->GetCountry(), pArmy->GetCountry()) &&
                    unit->GetAgent() == pArmy->GetAgent())
                {
                    if (pArmy->GetNumSubUnits() <= unit->GetNumSubUnits())
                    {
                        CUnit *army_of_unit = unit->GetArmy();
                        if (army_of_unit &&
                            static_cast<CArmy *>(army_of_unit)->GetAIDivisionOf() < 1)
                            bMySplinterHere = true;
                    }
                }
                int fl = 0;
                int64_t st = RawFPML(StrengthFactorCache.GetStrengthFactor(unit, fl));
                nOurBestLeaderPips = fl;
                vFriendlyStrength += static_cast<float>(st) * kStrengthToFloat;
                CountedFriendlyUnits.SetSize(CountedFriendlyUnits.GetSize() + 1, unit);
            }
            if (unit->InCombat())
            {
                bFriendInCombat = true;
                if (unit->GetCombat() != nullptr)
                {
                    const CCombatant *defender = &unit->GetCombat()->GetDefender();
                    if (defender != nullptr)
                    {
                        for (CUnit *engaged : defender->GetUnits())
                            if (engaged == unit)
                            {
                                bFriendBeingAttacked = true;
                                break;
                            }
                    }
                }
            }
            if (!bOverlordHere && !no_follow_focus &&
                CCountryTag::SameUpperWord(me->GetOverlord(), unit->GetAllegiance()))
                bOverlordHere = true;
        }
        else
        {
            CCountryTag ut = unit->GetAllegiance();
            const bool special_u = unit->GetCountry().IsRebelPirateNative();
            const bool special_m = me->GetCountryTag().IsRebelPirateNative();
            if (!CCountryTag::SameUpperWord(ut, me->GetCountryTag()) &&
                (me->GetDiplomacy().HasWar(ut) || special_u || special_m) &&
                !unit->GetCountry().IsNatives())
            {
                if (!unit->InCombat() && (unit->IsMoving() || unit->IsRetreating()))
                {
                    if (!unit->IsMoving())
                        continue;
                    if (unit->IsMovementLocked())
                    {
                        CEU3Date a = unit->GetEstimatedArrival(nullptr, false);
                        if (CGregorianDate(a).GetTotalDays() - CGregorianDate(CCurrentGameState::GetInstance()->GetCurrentDate()).GetTotalDays() < eta_days)
                            continue;
                    }
                }
                bool dup = false;
                for (int i = 0; i < Counted.GetSize(); ++i)
                    if (Counted[i] == unit)
                    {
                        dup = true;
                        break;
                    }
                if (dup)
                    continue;
                if (unit->GetCountry().IsRebels())
                {
                    nRebelForce += unit->GetNumSubUnits();
                    int fl = 0;
                    int64_t st = RawFPML(StrengthFactorCache.GetStrengthFactor(unit, fl));
                    nTheirBestLeaderPips = fl;
                    vRebelStrength += static_cast<float>(st) * kStrengthToFloat;
                }
                nEnemyForce += unit->GetNumSubUnits();
                int fl2 = 0;
                int64_t st2 = RawFPML(StrengthFactorCache.GetStrengthFactor(unit, fl2));
                nTheirBestLeaderPips = fl2;
                vEnemyStrength += static_cast<float>(st2) * kStrengthToFloat;
                bEnemyArrivesBeforeUs = true;
                Counted.SetSize(Counted.GetSize() + 1, unit);
            }
        }
    }
}

template <typename T>
void CAISharedEval::EvaluateUnitsMovingIntoProvince(const CProvince *pProv, const CArmy *pArmy,
                                                    CCountryTag enemy,
                                                    CPdxArray<CUnit *, int> &Counted, CPdxArray<CUnit *, int> &CountedFriendlyUnits,
                                                    bool bIsEvaluate, bool &bEnemyArrivesBeforeUs,
                                                    int &nOurBestLeaderPips, int &nTheirBestLeaderPips, int &             , int &nEnemyForce,
                                                    T &vEnemyStrength,
                                                    int nMinEnemies,
                                                    int &nExtraFriendlyForce, int &nExtraExtraFriendlyForce,
                                                    T &vExtraFriendlyStrength,
                                                    T &vExtraExtraFriendlyStrength,
                                                    int &nExtraEnemyForce,
                                                    bool &bBorderPrio, bool &bEnemyBorder, bool &bEnemyFortBorder,
                                                    bool &bBorder, bool &bNextToFriendly, bool &bNextToEnemyForce,
                                                    bool &bNextToRebels, int &nNextToSiegeForce,
                                                    bool &bFriendInCombat, bool &bFriendBeingAttacked,
                                                    int nFriendlyForce, CStrengthFactorCache &StrengthFactorCache)
{
    (void)nTheirBestLeaderPips;

    const CCountry *me = CCountryDataBase::AccessInstance()->GetCountry(pArmy->GetAllegiance());
    const bool friendOfOwner =
        me->IsFriend(pProv->GetController(), true, false, nullptr);
    const bool inverted = bIsEvaluate == false;

    int eta_days = 0;
    if (pArmy->GetLocation() != pProv && pArmy->GetLocation() != nullptr)
    {
        eta_days = 100000;
        for (auto it = pArmy->GetLocation()->GetTemplate().GetNeighbours().GetData();
             it != pArmy->GetLocation()->GetTemplate().GetNeighbours().GetData() + pArmy->GetLocation()->GetTemplate().GetNeighbours().GetSize(); ++it)
        {
            if (it->GetProvinceId() == pProv->GetProvinceID())
            {
                CEU3Date arrival = pArmy->GetEstimatedArrival(pProv, false);
                eta_days = CGregorianDate(arrival).GetTotalDays() - CGregorianDate(CCurrentGameState::GetInstance()->GetCurrentDate()).GetTotalDays();
                break;
            }
        }
    }

    auto neighbours_begin = pProv->GetTemplate().GetNeighbours().GetData();
    auto neighbours_end = pProv->GetTemplate().GetNeighbours().GetData() + pProv->GetTemplate().GetNeighbours().GetSize();
    if (!(neighbours_begin < neighbours_end))
        return;

    bool neighbour_moved = false;

    for (auto it = neighbours_begin; it != neighbours_end; ++it)
    {
        if (it->GetType() == EAdjacencyType::_ADJACENCY_IMPASSABLE_)
            continue;
        const CProvince &neighbour = it->GetProvince();

        CCountryTag owner = neighbour.GetController();
        const bool special_u = owner.IsRebelPirateNative();
        const bool special_m = me->GetCountryTag().IsRebelPirateNative();
        if (CCountryTag::SameUpperWord(neighbour.GetController(), pArmy->GetCountry()) &&
            !CCountryTag::SameUpperWord(pProv->GetController(), me->GetCountryTag()) &&
            (me->GetDiplomacy().HasWar(neighbour.GetController()) || special_u || special_m))
            bBorderPrio = true;

        if (friendOfOwner && neighbour.GetOwnerArea() != nullptr &&
            neighbour.GetTemplate().IsLand() && it->GetType() != EAdjacencyType::_ADJACENCY_STRAIT_)
        {
            bNextToFriendly = true;
            const CDiplomacyStatus *ctrl_status =
                me->GetDiplomacy()[neighbour.GetOwner().GetIndex()];
            if (CCountryTag::SameUpperWord(pProv->GetController(), pArmy->GetCountry()) &&
                !me->IsFriend(neighbour.GetOwner(), false, false, nullptr) &&
                !ctrl_status->HasAlliance())
                bBorder = true;
            if (!bEnemyFortBorder && !owner.IsRebelPirateNative())
            {
                if (owner.GetIndex() == me->GetStrategy().GetWarTarget().GetIndex())
                {
                    bEnemyBorder = true;
                    if (neighbour.GetFortLevel() > 0)
                        bEnemyFortBorder = true;
                }
                if (!CCountryTag::SameUpperWord(neighbour.GetController(), me->GetCountryTag()) &&
                    (me->GetDiplomacy().HasWar(neighbour.GetController()) || special_u || special_m))
                {
                    bEnemyBorder = true;
                    if (neighbour.GetFortLevel() > 0)
                        bEnemyFortBorder = true;
                }
            }
        }

        for (CUnit *unit : neighbour.GetUnits())
        {
            if (unit == pArmy)
                continue;
            if (unit->IsNavy())
                continue;
            if (unit->InCombat() ||
                (unit->GetAgent() != nullptr && unit->GetAttachedTo() == pArmy))
                continue;
            if (unit->IsRetreating() && unit->GetFinalDestination() != pProv)
                continue;
            if (unit->HasExileStatus() &&
                !CCountryTag::SameUpperWord(pProv->GetController(), unit->GetAllegiance()) &&
                !CCountryTag::SameUpperWord(pProv->GetOwner(), unit->GetAllegiance()))
                continue;
            if (unit->GetCountry().IsRebels())
            {
                CRebelFaction *faction = unit->GetRebelFaction();
                if (faction != nullptr && faction->IsFriendly(pArmy->GetAllegiance()))
                    continue;
            }

            CFixedPoint avg_morale = unit->GetAverageMorale(false);
            if (bIsEvaluate || avg_morale.GetUnderlyingValue() > 499)
            {
                CCountryTag unit_tag = unit->GetAllegiance();
                if (!CCountryTag::SameUpperWord(unit->GetCountry(), pArmy->GetCountry()) ||
                    unit->GetAgent() != pArmy->GetAgent() ||
                    unit->IsMainArmy())
                {
                    if (me->IsWarFriend(unit_tag, neighbour.GetController()) &&
                        unit->InSiege() && it->GetType() != EAdjacencyType::_ADJACENCY_STRAIT_)
                        nNextToSiegeForce += unit->GetNumSubUnits();
                }

                if (me->IsWarFriend(unit_tag, enemy) && unit->IsMoving() &&
                    unit->GetNextTarget() == pProv)
                {
                    int needed = nMinEnemies;
                    if (!unit->InSiege())
                    {
                        if (pProv->GetSiege() != nullptr &&
                            me->IsWarFriend(unit_tag, pProv->GetController()))
                            needed = pProv->GetRegimentsNeededForSiege();
                    }
                    else
                    {
                        int task_count = unit->AccessSiege() != nullptr ? unit->AccessSiege()->GetCachedSiegeImpact() : 0;
                        needed = (task_count / 2 + 1) * nMinEnemies;
                    }
                    int cap = nEnemyForce;
                    if (nEnemyForce <= nExtraEnemyForce)
                        cap = nExtraEnemyForce;
                    if (needed <= cap)
                    {
                        CEU3Date arrival = unit->GetEstimatedArrival(pProv, false);
                        int numSubUnit = unit->GetNumSubUnits();
                        int fl = 0;
                        int64_t st = RawFPML(StrengthFactorCache.GetStrengthFactor(unit, fl));
                        nOurBestLeaderPips = fl;
                        float str = static_cast<float>(st) * kStrengthToFloat;
                        if (eta_days < CGregorianDate(arrival).GetTotalDays() - CGregorianDate(CCurrentGameState::GetInstance()->GetCurrentDate()).GetTotalDays())
                        {
                            nExtraExtraFriendlyForce += numSubUnit;
                            vExtraExtraFriendlyStrength += str;
                        }
                        else
                        {
                            nExtraFriendlyForce += numSubUnit;
                            vExtraFriendlyStrength += str;
                        }
                        CountedFriendlyUnits.SetSize(CountedFriendlyUnits.GetSize() + 1, unit);
                    }
                }

                CCountryTag ut = unit->GetAllegiance();
                const bool special = unit->GetCountry().IsRebelPirateNative();
                if (!CCountryTag::SameUpperWord(ut, me->GetCountryTag()) &&
                    (me->GetDiplomacy().HasWar(ut) || special || special_m))
                {
                    bool dup = false;
                    for (int i = 0; i < Counted.GetSize(); ++i)
                        if (Counted[i] == unit)
                        {
                            dup = true;
                            break;
                        }
                    if (dup)
                        continue;
                    if (unit->GetCountry().IsRebels())
                    {
                        const CDiplomacyStatus *dep_status =
                            me->GetDiplomacy()[pProv->GetOwner().GetIndex()];
                        if (CCountryTag::SameUpperWord(pProv->GetOwner(), pArmy->GetCountry()) ||
                            dep_status->HasDependency())
                        {
                            bNextToEnemyForce = true;
                            bNextToRebels = true;
                        }
                    }
                    else
                    {
                        bNextToEnemyForce = true;
                    }
                    if (unit->IsMoving() && unit->GetNextTarget() == pProv)
                    {
                        if (!unit->IsMovementLocked())
                        {
                            CEU3Date a1 = unit->GetEstimatedArrival(pProv, false);
                            CEU3Date a2 = pArmy->GetEstimatedArrival(pProv, false);
                            if (CGregorianDate(a2).GetTotalDays() < CGregorianDate(a1).GetTotalDays() ||
                                (CGregorianDate(a2).GetTotalDays() == CGregorianDate(a1).GetTotalDays() &&
                                 pArmy->GetCountry().GetRaw() < unit->GetCountry().GetRaw()))
                                continue;
                        }
                        if (0 < nFriendlyForce && unit->IsMovementLocked())
                            neighbour_moved = true;
                        if (bFriendInCombat || (unit->IsMovementLocked() ? true : inverted))
                        {
                            int n = unit->GetNumSubUnits();
                            nEnemyForce += n;
                            nExtraEnemyForce += n;
                            int fl = 0;
                            int64_t st = RawFPML(StrengthFactorCache.GetStrengthFactor(unit, fl));
                            vEnemyStrength += static_cast<float>(st) * kStrengthToFloat;
                        }
                        if (!bEnemyArrivesBeforeUs)
                        {
                            CEU3Date a = unit->GetEstimatedArrival(nullptr, false);
                            if (CGregorianDate(a).GetTotalDays() - CGregorianDate(CCurrentGameState::GetInstance()->GetCurrentDate()).GetTotalDays() < eta_days)
                                bEnemyArrivesBeforeUs = true;
                        }
                        Counted.SetSize(Counted.GetSize() + 1, unit);
                    }
                }
            }
        }
    }
    if (neighbour_moved && !bFriendInCombat)
    {
        bFriendBeingAttacked = true;
        bFriendInCombat = true;
    }
}