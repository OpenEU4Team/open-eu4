#include "enums.h"
#include "ai_region_plan.h"
#include "ai.h"
#include "ai_army.h"
#include "ai_king.h"
#include "army.h"
#include "combat.h"
#include "country.h"
#include "defines.h"
#include "engine.h"
#include "gamestate.h"
#include "map.h"
#include "pathfind.h"
#include "province.h"
#include "provincetemplate.h"
#include "rebelfaction.h"
#include "relation.h"
#include "unit.h"
#include "unitlist.h"
#include "war.h"
#include "wargoal.h"
#include <algorithm>
#include <cmath>

extern CPdxArray<CAIRegion> AIRegionMapRegions asm("_ZZN12_GLOBAL__N_124AccessAIRegionMapRegionsEvE7Regions");

extern CPdxArray<const CAIRegion *> AIRegionMapPerProvince asm("_ZZN12_GLOBAL__N_128AccessAiRegionMapPerProvinceEvE11PerProvince");

extern void InitializeAIRegionMapRegions() asm("_ZN12_GLOBAL__N_124AccessAIRegionMapRegionsEv");

extern void InitializeAIRegionMapPerProvince() asm("_ZN12_GLOBAL__N_128AccessAiRegionMapPerProvinceEv");

static HELPER CPdxArray<CAIRegion> &AccessAIRegionMapRegions()
{
    ::InitializeAIRegionMapRegions();
    return ::AIRegionMapRegions;
}
static HELPER CPdxArray<const CAIRegion *> &AccessAiRegionMapPerProvince()
{
    ::InitializeAIRegionMapPerProvince();
    return ::AIRegionMapPerProvince;
}

namespace NAIPlan
{
static void AddMyArmyStrengthToRegion(SRegionInfo &Region, CAIArmy *pArmy, const CAIRegion *pTheirRegion, bool bStatic, CRegionDistanceCacher &DistanceToRegion, bool bRemove);
}

int NAIPlan::CRegionDistanceCacher::operator()(const std::pair<const CAIArmy *, const CAIRegion *> &input)
{
    auto it = this->_CachedResult.find(input);
    if (it == this->_CachedResult.end())
    {
        CArray<int> regionList;
        regionList.Append(input.second->GetIndex());
        this->CalcAndCache(input.first, regionList);
        it = this->_CachedResult.find(input);
        if (it == this->_CachedResult.end())
            return kUnreachable;
    }
    return it->second;
}

void NAIPlan::CRegionDistanceCacher::CalcAndCache(const CAIArmy *pArmy, const CArray<int> &Regions)
{
    for (int l_index = 0; l_index < Regions.GetSize(); ++l_index)
    {
        int regionIdx = Regions[l_index];
        CAIRegion &region = ::AccessAIRegionMapRegions()[regionIdx];
        this->_CachedResult[{pArmy, &region}] = 6;
    }
    if (pArmy == nullptr)
        return;
    CArmy *main = pArmy->GetMainArmy();
    if (main == nullptr)
        return;

    CArray<const CProvince *> provinces;
    int regionsWithProvinces = 0;
    for (int l_index = 0; l_index < Regions.GetSize(); ++l_index)
    {
        int regionIdx = Regions[l_index];
        CAIRegion &region = ::AccessAIRegionMapRegions()[regionIdx];
        if (region.GetProvinces().GetSize() == 0)
            continue;
        ++regionsWithProvinces;
        for (int provId : region.GetProvinces())
            provinces.Append(&CMap::_pMap->GetProvince(provId));
    }

    CPathFind::SFindSettings settings{};
    settings._bDirectIfAdjacent = true;
    settings._bEnableReturnToPreviousNoZocHax = true;
    settings._nNumRegimentsForLoadingOverride = -1;
    settings._HiredByCountryOverride = CCountryTag::FromRaw(kEmptyCountryTag);
    settings._pAccessCache = this->_pAccessCache;
    settings._bPathLengthMap = true;

    CPathFind pathFinder;
    std::unordered_map<int, CList<int>> resultPass0 =
        pathFinder.FindAllPaths(main, main->GetLocation(), provinces, settings,
                                regionsWithProvinces < 2, false);
    settings._bBothLandAndSea = true;
    std::unordered_map<int, CList<int>> resultPass1 =
        pathFinder.FindAllPaths(main, main->GetLocation(), provinces, settings,
                                regionsWithProvinces < 2, false);

    std::unordered_map<int, CList<int>> *results[2] = {&resultPass0, &resultPass1};
    for (int pass = 0; pass < 2; pass++)
    {
        for (const std::pair<const int, CList<int>> &entry : *results[pass])
        {
            const int targetId = entry.first;
            int cost = *entry.second.begin();
            int scaled = cost * kCostScale;
            int ring = scaled / 1000;
            if (scaled > 0 && scaled % 1000 != 0)
                ring += 1;
            if (pass == 1)
                ring += 5;
            const CProvince *prov = &CMap::_pMap->GetProvince(targetId);
            const CAIRegion *provRegion = nullptr;
            if (prov->GetProvinceID() > 0)
                provRegion = ::AccessAiRegionMapPerProvince()[prov->GetProvinceID()];
            if (pArmy->GetAssignedRegion() != provRegion)
                ring += 1;
            ring = std::min(ring, kMaxDistanceValue);

            auto it = this->_CachedResult.find({pArmy, provRegion});
            if (it != this->_CachedResult.end() && ring < it->second)
                it->second = ring;
        }
    }
}

float NAIPlan::SRegionInfo::GetProvinceValue(const CProvince &Prov, CCountryTag MyTag) const
{
    int tax = Prov.GetCurrentTaxIncome(nullptr, false, CFixedPoint{}, CFixedPoint{}).GetUnderlyingValue();
    int production = Prov.GetProductionIncome(nullptr, false).GetUnderlyingValue();
    float value = float(tax + production) / kMilli + kOne;

    int fortLevel = Prov.GetFortLevel();
    value /= (float(fortLevel) + kOne);
    if (fortLevel == 0)
        value /= kFortDivisor;

    if (Prov.IsCapital())
    {
        float bonus = kCapitalBonus;
        if (CCountryTag::SameUpperWord(Prov.GetOwner(), MyTag))
            bonus = kCapitalOwnCtrl;
        value = (value + bonus) * bonus;
    }

    CCountry *ourCountry = CCountryDataBase::AccessInstance()->GetCountry(MyTag);
    const CWar *war = ourCountry->GetDiplomacy()[Prov.GetController().GetIndex()]->GetWar();

    if (war != nullptr)
    {
        const CWarGoal *goal = war->GetWarGoal();
        if (goal->GetProvince() == Prov.GetProvinceID())
            value = (value + kWargoalBonus) * kFortDivisor;
    }
    return value;
}

void NAIPlan::SRegionInfo::InitRegion(const CAIRegion *pRegion, CCountryTag MyTag)
{
    this->_pRegion = pRegion;

    CCountry *ourCountry = CCountryDataBase::AccessInstance()->GetCountry(MyTag);
    if (ourCountry->GetAIAtHost()->GetKing()->CalculateHomelandThreatened())
        this->_bHomeThreatened = true;

    bool anySiegeNeedsMoreThanOne = false;
    for (int provId : this->_pRegion->GetProvinces())
    {
        CProvince *prov = &CCurrentGameState::GetInstance()->GetProvince(provId);

        COwnerArea *ownArea = prov->GetOwnerArea();
        if (ownArea == ourCountry->GetHomeOwnerArea() ||
            (CCountryTag::SameUpperWord(prov->GetOwner(), MyTag) && prov->IsCapital()))
        {
            this->_bHomeRegion = true;
            this->_bHomeAdjacent = true;
        }

        if (CCountryTag::SameUpperWord(prov->GetController(), MyTag))
        {
            this->_vValueToLose += this->GetProvinceValue(*prov, MyTag);
            this->_bWeHavePresence = true;
        }
        else if (ourCountry->GetDiplomacy()[prov->GetController().GetIndex()]
                     ->IsFightingWarTogether())
        {
            this->_vValueToLose += this->GetProvinceValue(*prov, MyTag) * kHalf;
        }
        else
        {
            CCountryTag controllerTag = prov->GetController();
            if (CCountryTag::SameUpperWord(prov->GetOwner(), MyTag))
            {
                this->_vValueToGain += this->GetProvinceValue(*prov, MyTag);
            }
            else if (controllerTag.IsRebels())
            {
                if (!ourCountry->IsFriend(prov->GetOwner(), false, false, nullptr))
                    continue;
                this->_vValueToGain += this->GetProvinceValue(*prov, MyTag) * kRebelFriend;
            }
            else if (!CCountryTag::SameUpperWord(controllerTag, MyTag) &&
                     (ourCountry->GetDiplomacy().HasWar(controllerTag) ||
                      controllerTag.IsRebelPirateNative() ||
                      MyTag.IsRebelPirateNative()))
            {
                this->_vValueToGain += this->GetProvinceValue(*prov, MyTag) * kHalf;
            }
            else
            {
                continue;
            }

            int needed = prov->GetRegimentsNeededForSiege();
            this->_nSiegeNeed += needed;
            if (needed > 1)
            {
                this->_nMinSiegeNeed = std::min(this->_nMinSiegeNeed, needed);
                anySiegeNeedsMoreThanOne = true;
            }
        }
    }
    if (!anySiegeNeedsMoreThanOne)
        this->_nMinSiegeNeed = 1;
}

static void NAIPlan::AddMyArmyStrengthToRegion(SRegionInfo &Region,
                                        CAIArmy *pArmy,
                                        const CAIRegion *pTheirRegion,
                                        bool bStatic,
                                        CRegionDistanceCacher &DistanceToRegion,
                                        bool bRemove)
{
    if (pArmy == nullptr || pTheirRegion == nullptr)
        return;
    CArmy *main = pArmy->GetMainArmy();
    if (main == nullptr)
        return;

    int dist = DistanceToRegion(std::make_pair(pArmy, pTheirRegion));
    if (dist < 0)
        return;

    float strength = 0.0f;
    int unitCount = 0;
    for (CUnit *unit : *pArmy->GetArmyList())
    {
        if (!unit->IsArmy())
            continue;
        int32_t outStrength = 0;
        auto s = unit->GetArmyStrengthFactor(nullptr, false, true, outStrength);
        strength += static_cast<float>(s);
        unitCount += unit->GetNumSubUnits();
    }

    if (bRemove)
    {
        strength = -strength;
        unitCount = -unitCount;
    }

    if (bStatic)
    {
        Region._vAlliedStrength += strength;
        Region._nAlliedSiegeForce += unitCount;
    }
    else
    {
        Region._AssignedStrength[dist] += strength;
        Region._AssignedSiegeForce[dist] += unitCount;
    }
}

float NAIPlan::SRegionAssignment::CalcSinglePartialScore(const SRegionInfo &ri, int ring) const
{
    int availUnits = ri._nAlliedSiegeForce;
    float availStr = ri._vAlliedStrength * kAssignedShare;
    for (int i = 0; i <= ring; i++)
    {
        availUnits += ri._AssignedSiegeForce[i];
        availStr += ri._AssignedStrength[i];
    }

    float ratio = kHalf;
    if (kStrengthEpsilon < ri._vEnemyStrength)
        ratio = availStr / (ri._vEnemyStrength + availStr) + kRatioBias;

    float siegeBonus = 0.0f;
    if (availUnits >= ri._nMinSiegeNeed && ri._nSiegeNeed != 0)
    {
        int clamped = std::min(availUnits, ri._nSiegeNeed);
        siegeBonus = float((clamped * 3) / ri._nSiegeNeed + 1);
    }

    ratio = ratio / (std::fabs(ratio) + kScoreNormAdd);

    float defenseScore = 0.0f;
    float siegeScore = 0.0f;
    float priorityMul = kOne;
    const auto scorePriority = [&]() -> void
    {
        float shifted = (ratio + kOne) * kHalf;
        defenseScore = ri._vValueToLose * shifted * shifted * kDefenseScale;
    };
    if (ratio <= 0.0f)
    {
        if (!ri._bHomeThreatened)
            return 0.0f;
        if (ri._bHomeRegion)
            scorePriority();
    }
    else
    {
        siegeScore = siegeBonus * ri._vValueToGain * ratio;
        scorePriority();
        if (!ri._bHomeThreatened)
            return defenseScore + siegeScore;
    }
    if (ri._bHomeAdjacent)
        priorityMul = float(NDefines::NAI::REGION_PLANNING_HOMELAND_PRIORIZATION.GetUnderlyingValue()) / kMilli;
    return defenseScore * priorityMul + siegeScore;
}

void NAIPlan::SRegionAssignment::CalcScore(const CArray<SRegionInfo> &regions)
{
    float totalScore = 0.0f;
    float priorityWeight = 0.0f;
    float totalWeight = 0.0f;

    for (int l_index = 0; l_index < regions.GetSize(); ++l_index)
    {
        const SRegionInfo &ri = regions[l_index];
        float decay = kOne;
        for (int ring = 0; ring < kDistanceBuckets; ring++)
        {
            decay *= kHalf;
            totalScore += this->CalcSinglePartialScore(ri, ring) * decay;
        }

        float w = ri._vMyStrength;
        if (ri._bHomeRegion)
            priorityWeight += w + w;
        else if (ri._bHomeAdjacent)
            priorityWeight += w;
        totalWeight += w;
    }

    if (totalWeight <= 0.0f)
    {
        this->_vScore = 0.0f;
        return;
    }
    this->_vScore = (priorityWeight / totalWeight) * totalScore;
}

NAIPlan::SRegionAssignment
NAIPlan::GenerateBestArmyToRegionAssignment(const CArray<CAIArmy *> &ArmiesToAssign,
                                            const CArray<SAIRegionTask> &TasksToAssign)
{
    std::vector<const NAICoalition::CCoalitionTask *> coalitionTasks;
    for (int i = 0; i < TasksToAssign.GetSize(); ++i)
        coalitionTasks.push_back(&TasksToAssign[i]);
    std::vector<CAIArmy *> armyList;
    for (int i = 0; i < ArmiesToAssign.GetSize(); ++i)
        armyList.push_back(ArmiesToAssign[i]);

    NAICoalition::CCoalitionFormationGeneratorSequenced<CAIArmy, double> generator(&armyList, &coalitionTasks);
    NAICoalition::CCoalitionFormationGeneratorSequenced<CAIArmy, double>::SPartitionSearchResult result =
        generator.GenerateCoalitionFormation();

    SRegionAssignment out{};
    out._Assignments.rehash(ArmiesToAssign.GetSize());
    for (size_t i = 0; i < result._CollaborationStructure.size(); i++)
    {
        const CAIRegion *region = TasksToAssign[i]._pRegionInfo->_pRegion;
        for (CAIArmy *member : result._CollaborationStructure[i])
            out._Assignments[member] = region;
    }
    return out;
}

NAIPlan::SAIRegionTask::~SAIRegionTask()
{
}

double NAIPlan::SAIRegionTask::EvaluateCoalition(const std::vector<CAIArmy *> &ArmiesToAssign) const
{
    std::unordered_map<CAIArmy *, const CAIRegion *> scratch;
    scratch.rehash(ArmiesToAssign.size());

    const CAIRegion *region = this->_pRegionInfo->_pRegion;
    for (CAIArmy *army : ArmiesToAssign)
    {
        scratch[army] = region;
        NAIPlan::AddMyArmyStrengthToRegion(*this->_pRegionInfo, army, region, false, *this->_pDistanceToRegionCache, false);
    }

    const SRegionAssignment scorer{};
    float score = 0.0f;
    float decay = kOne;
    for (int ring = 0; ring < kDistanceBuckets; ring++)
    {
        decay *= kHalf;
        score = scorer.CalcSinglePartialScore(*this->_pRegionInfo, ring) * decay + score;
    }

    for (CAIArmy *army : ArmiesToAssign)
        NAIPlan::AddMyArmyStrengthToRegion(*this->_pRegionInfo, army, region, false, *this->_pDistanceToRegionCache, true);

    return static_cast<double>(score);
}

void NAIPlan::PlanArmyRegions(CAIKing *pKing, CCountryFlagCache<char> *pAccessCache)
{
    CCountry *country = CCountryDataBase::AccessInstance()->GetCountry(pKing->GetCountryTag());

    bool noReasonToPlan =
        (country->GetRebelThreat(false) == 0 &&
         country->GetNumOfRebelControlledProvinces() == 0 &&
         country->GetNumOfRebelArmies() == 0 &&
         !country->IsAtWar()) ||
        (country->IsSubject() &&
         country->GetSubjectMilitaryFocus() != ESubjectMilitaryFocus::_FOCUS_NONE_);

    if (noReasonToPlan)
    {
        for (CAIArmy *army : pKing->GetArmyAgents())
        {
            army->AssignToRegion(nullptr, pAccessCache);
        }
        return;
    }

    CRegionDistanceCacher distanceCacher(pAccessCache);
    auto &regions = ::AccessAIRegionMapRegions();
    CArray<SRegionInfo> regionInfos;
    for (int i = 0; i < regions.GetSize(); ++i)
        regionInfos.Append(SRegionInfo{});

    for (int i = 0; i < regions.GetSize(); i++)
        regionInfos[i].InitRegion(&regions[i], country->GetCountryTag());

    for (int i = 0; i < regionInfos.GetSize(); ++i)
    {
        SRegionInfo &ri = regionInfos[i];
        if (!ri._bHomeRegion)
            continue;
        for (const CAIRegion *nb : ri._pRegion->GetNeighbors())
            regionInfos[nb->GetIndex()]._bHomeAdjacent = true;
    }

    for (CCountryTag tag : country->GetCurrentAtWarWith())
    {
        CCountry *other = CCountryDataBase::AccessInstance()
                              ->GetCountry(tag);
        NAIPlan::AddArmyStrengthsToRegions(other->GetUnits(), regionInfos, true);
    }
    for (CCountryTag tag : country->GetCurrentWarAllies())
    {
        CCountry *other = CCountryDataBase::AccessInstance()
                              ->GetCountry(tag);
        NAIPlan::AddArmyStrengthsToRegions(other->GetUnits(), regionInfos, false);
    }
    if (!country->IsInImportantWar(false, false, nullptr))
    {
        for (int l_index = 0; l_index < country->GetRebelFactions().GetSize(); ++l_index)
        {
            auto armies = country->GetRebelFactions()[l_index]->GetArmiesArray();
            NAIPlan::AddArmyStrengthsToRegions(armies, regionInfos, true);
        }
    }

    CFixedPoint forceLimit = country->GetLandForceLimit(nullptr);
    int minSubUnits = std::max(5, CFixedPoint(forceLimit.GetUnderlyingValue() *
                                              NDefines::NAI::MIN_FORCE_LIMIT_SHARE_REGION_ASSIGN.GetUnderlyingValue() / 1000)
                                      .GetRounded());

    std::vector<CAIArmy *> unassignedArmies;
    for (CAIArmy *army : pKing->GetArmyAgents())
    {
        CArmy *main = army->GetMainArmy();
        if (main == nullptr)
            continue;
        if (main->GetNumSubUnits() < minSubUnits)
            continue;
        if (army->IsForExploration(nullptr))
            continue;
        if (army->IsCondottieri())
            continue;

        const CAIRegion *assigned = army->GetAssignedRegion();
        bool validRegion = false;
        if (assigned == nullptr && main->GetLocation() != nullptr)
        {
            int provId = main->GetLocation()->GetProvinceID();
            if (provId > 0)
                assigned = ::AccessAiRegionMapPerProvince()[provId];
        }
        if (assigned != nullptr)
        {
            regionInfos[assigned->GetIndex()]._bWeHavePresence = true;
            validRegion = true;
        }

        const CSiegeCombat *siege = main->AccessSiege();
        if (siege == nullptr || siege->GetCachedSiegeImpact() < 1)
        {
            unassignedArmies.push_back(army);
        }
        else if (validRegion)
        {
            NAIPlan::AddMyArmyStrengthToRegion(regionInfos[assigned->GetIndex()], army, assigned, true, distanceCacher, false);
        }
    }

    if (unassignedArmies.empty())
    {
        for (CAIArmy *army : pKing->GetArmyAgents())
        {
            army->AssignToRegion(nullptr, pAccessCache);
        }
        return;
    }

    CArray<int> candidateRegions;
    for (int i = 0; i < regionInfos.GetSize(); i++)
    {
        SRegionInfo &ri = regionInfos[i];
        const CAIRegion *region = ri._pRegion;
        if (region == nullptr || region->GetProvinces().GetSize() == 0)
            continue;

        CProvince *first = &CMap::_pMap->GetProvince(region->GetProvinces()[0]);
        if (!first->GetTemplate().IsLand())
            continue;

        if (ri._vValueToLose <= 0.0f || ri._vEnemyStrength <= 0.0f)
        {
            if (ri._vValueToGain > 0.0f)
            {
                if (ri._bWeHavePresence || ri._vAlliedStrength > 0.0f)
                {
                    candidateRegions.Append(i);
                }
                else
                {
                    for (const CAIRegion *nb : region->GetNeighbors())
                    {
                        if (regionInfos[nb->GetIndex()]._bWeHavePresence)
                        {
                            candidateRegions.Append(i);
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            candidateRegions.Append(i);
        }
    }

    if (candidateRegions.GetSize() == 0)
    {
        for (CAIArmy *army : pKing->GetArmyAgents())
        {
            army->AssignToRegion(nullptr, pAccessCache);
        }
        return;
    }

    for (CAIArmy *army : unassignedArmies)
        distanceCacher.CalcAndCache(army, candidateRegions);

    if (candidateRegions.GetSize() > NDefines::NAI::MAX_TASKS_NEW_REGION_ASSIGN_ALGORITHM ||
        unassignedArmies.size() > static_cast<size_t>(NDefines::NAI::MAX_ARMIES_NEW_REGION_ASSIGN_ALGORITHM))
    {
        std::vector<std::pair<SRegionAssignment, CArray<SRegionInfo>>>
            candidates;

        CApproximateReachCostCallback costCallback(*country);

        for (int attempt = 0; attempt < kRandomRestarts; attempt++)
        {
            std::pair<SRegionAssignment, CArray<SRegionInfo>> cand;
            cand.second = regionInfos;
            const size_t candidateCount = candidateRegions.GetSize();

            for (size_t j = unassignedArmies.size(); j-- > 0;)
            {
                CAIArmy *army = unassignedArmies[j];
                CArmy *main = army->GetMainArmy();
                COwnerArea *armyArea = main->GetLocation()->GetOwnerArea();

                int regionIdx = -1;
                bool reachable = false;
                if (armyArea != nullptr)
                {
                    for (int try_ = 0; try_ < kPathFindTries && !reachable; try_++)
                    {
                        const SRegionInfo &ri =
                            cand.second[candidateRegions[pKing->GetRandom() % static_cast<int>(candidateCount)]];
                        regionIdx = ri._pRegion->GetIndex();
                        CProvince *prov = &CMap::_pMap->GetProvince(
                            ri._pRegion->GetProvinces()[pKing->GetRandom() % ri._pRegion->GetProvinces().GetSize()]);
                        COwnerArea *provArea = prov->GetOwnerArea();
                        if (provArea != nullptr &&
                            COwnerArea::FindBestPathV2(*provArea, *armyArea, &costCallback, nullptr))
                            reachable = true;
                    }
                }
                if (!reachable && main->GetLocation() != nullptr)
                {
                    int provId = main->GetLocation()->GetProvinceID();
                    if (provId > 0)
                    {
                        const CAIRegion *cur = ::AccessAiRegionMapPerProvince()[provId];
                        if (cur != nullptr)
                            regionIdx = cur->GetIndex();
                    }
                }

                if (regionIdx >= 0)
                {
                    const CAIRegion *region = cand.second[regionIdx]._pRegion;
                    cand.first._Assignments[army] = region;
                    if (region != nullptr)
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[regionIdx], army, region,
                                                           false, distanceCacher, false);
                }
            }

            cand.first.CalcScore(cand.second);
            candidates.push_back(std::move(cand));
        }

        float bestScore = 0.0f;
        for (const auto &cand : candidates)
            bestScore = std::max(bestScore, cand.first._vScore);

        for (auto &cand : candidates)
        {
            if (bestScore > cand.first._vScore * 2.0f)
                continue;

            for (int round = 0; round < kMaxImprovementRounds; round++)
            {
                bool improved = false;

                for (CAIArmy *army : unassignedArmies)
                {
                    auto it = cand.first._Assignments.find(army);
                    if (it == cand.first._Assignments.end() || it->second == nullptr)
                        continue;
                    const CAIRegion *curRegion = it->second;

                    float moveBaseline = cand.first._vScore;
                    float swapBaseline = cand.first._vScore;

                    const CAIRegion *bestTarget = nullptr;
                    for (int l_index = 0; l_index < candidateRegions.GetSize(); ++l_index)
                    {
                        int regionIdx = candidateRegions[l_index];
                        const CAIRegion *other = cand.second[regionIdx]._pRegion;
                        if (other == curRegion)
                            continue;
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[curRegion->GetIndex()],
                                                           army, curRegion, false, distanceCacher, true);
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[regionIdx], army, other,
                                                           false, distanceCacher, false);
                        float prevScore = cand.first._vScore;
                        cand.first.CalcScore(cand.second);
                        float newScore = cand.first._vScore;
                        if (newScore > moveBaseline * kImprovementMin)
                        {
                            bestTarget = other;
                            moveBaseline = newScore;
                        }
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[curRegion->GetIndex()],
                                                           army, curRegion, false, distanceCacher, false);
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[regionIdx], army, other,
                                                           false, distanceCacher, true);
                        cand.first._vScore = prevScore;
                    }
                    if (bestTarget != nullptr)
                    {
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[curRegion->GetIndex()],
                                                           army, curRegion, false, distanceCacher, true);
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[bestTarget->GetIndex()],
                                                           army, bestTarget, false, distanceCacher, false);
                        it->second = bestTarget;
                        cand.first.CalcScore(cand.second);
                        improved = true;
                        continue;
                    }

                    const CAIRegion *bestSwapRegion = nullptr;
                    CAIArmy *bestSwapPartner = nullptr;
                    int checked = 0;
                    for (auto rit = unassignedArmies.rbegin();
                         rit != unassignedArmies.rend() && checked < kMaxSwapCandidates;
                         ++rit)
                    {
                        CAIArmy *other = *rit;
                        checked++;
                        auto it2 = cand.first._Assignments.find(other);
                        if (other == army || it2 == cand.first._Assignments.end() ||
                            it2->second == nullptr || it2->second == curRegion)
                            continue;
                        const CAIRegion *otherRegion = it2->second;
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[curRegion->GetIndex()],
                                                           army, curRegion, false, distanceCacher, true);
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[curRegion->GetIndex()],
                                                           other, curRegion, false, distanceCacher, false);
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[otherRegion->GetIndex()],
                                                           army, otherRegion, false, distanceCacher, false);
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[otherRegion->GetIndex()],
                                                           other, otherRegion, false, distanceCacher, true);
                        float prevScore = cand.first._vScore;
                        cand.first.CalcScore(cand.second);
                        if (cand.first._vScore > swapBaseline * kImprovementMin)
                        {
                            bestSwapPartner = other;
                            bestSwapRegion = otherRegion;
                            swapBaseline = cand.first._vScore;
                            improved = true;
                        }
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[curRegion->GetIndex()],
                                                           army, curRegion, false, distanceCacher, false);
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[curRegion->GetIndex()],
                                                           other, curRegion, false, distanceCacher, true);
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[otherRegion->GetIndex()],
                                                           army, otherRegion, false, distanceCacher, true);
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[otherRegion->GetIndex()],
                                                           other, otherRegion, false, distanceCacher, false);
                        cand.first._vScore = prevScore;
                    }
                    if (bestSwapPartner != nullptr)
                    {
                        auto pit = cand.first._Assignments.find(bestSwapPartner);
                        std::swap(it->second, pit->second);
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[curRegion->GetIndex()],
                                                           army, curRegion, false, distanceCacher, true);
                        NAIPlan::AddMyArmyStrengthToRegion(cand.second[curRegion->GetIndex()],
                                                           bestSwapPartner, curRegion, false,
                                                           distanceCacher, false);
                        if (bestSwapRegion != nullptr)
                        {
                            NAIPlan::AddMyArmyStrengthToRegion(cand.second[bestSwapRegion->GetIndex()],
                                                               army, bestSwapRegion, false,
                                                               distanceCacher, false);
                            NAIPlan::AddMyArmyStrengthToRegion(cand.second[bestSwapRegion->GetIndex()],
                                                               bestSwapPartner, bestSwapRegion, false,
                                                               distanceCacher, true);
                        }
                        cand.first.CalcScore(cand.second);
                    }
                }
                if (!improved)
                    break;
            }
        }

        const std::pair<SRegionAssignment, CArray<SRegionInfo>> *best = nullptr;
        for (const auto &cand : candidates)
            if (best == nullptr || cand.first._vScore > best->first._vScore)
                best = &cand;
        if (best != nullptr)
            for (const auto &[army, region] : best->first._Assignments)
                army->AssignToRegion(region, pAccessCache);
    }
    else
    {
        CArray<SAIRegionTask> tasks;
        for (int l_index = 0; l_index < candidateRegions.GetSize(); ++l_index)
        {
            int regionIdx = candidateRegions[l_index];
            SAIRegionTask task;
            task._pDistanceToRegionCache = &distanceCacher;
            task._pRegionInfo = &regionInfos[regionIdx];
            tasks.Append(task);
        }

        CArray<CAIArmy *> armiesToAssign;
        for (CAIArmy *army : unassignedArmies)
            armiesToAssign.Append(army);
        auto assignments = NAIPlan::GenerateBestArmyToRegionAssignment(armiesToAssign, tasks);

        for (const auto &[army, region] : assignments._Assignments)
            army->AssignToRegion(region, pAccessCache);
    }
}

float NAIPlan::CApproximateReachCostCallback::Cost(const CControllerArea &goal)
{
    return this->_Country.HaveAccess(*goal.GetProvinceList().begin(),
                                     false, false, nullptr, nullptr)
               ? 1.0f
               : -1.0f;
}
