#include "ai_king.h"
#include "ai.h"
#include "ai_army.h"
#include "ai_region_plan.h"
#include "ai_strategy.h"
#include "ai_threatevaluation.h"
#include "areas.h"
#include "army.h"
#include "clausewitz/pdx_core/log.h"
#include "constructioncommands.h"
#include "country.h"
#include "countrydiplomacy.h"
#include "countrytag.h"
#include "defines.h"
#include "gamestate.h"
#include "navy.h"
#include "pathfind.h"
#include "province.h"
#include "provincetemplate.h"
#include "strength_factor_cache.h"
#include "subjecttype.h"
#include "unit.h"
#include "unitcommands.h"
#include "unitlist.h"
#include "values.h"
#include "war.h"
#include "wargoal.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>

bool CAIKing::AssignFleet(CNavy &Fleet)
{
    this->AcceptUnit(Fleet);
    if (this->IsActive() && !this->_IsPendingResource[5])
        this->_IsPendingResource[5] = 1;
    return true;
}

bool CAIKing::CalculateHomelandThreatened() const
{
    CCountryTag ourTag = this->GetCountryTag();
    CCountry *ourCountry = CCountryDataBase::AccessInstance()->GetCountry(ourTag);

    if (ourCountry->GetOwnedProvinces().GetSize() == 0)
        return false;

    const CMapArea *specialArea = ourCountry->GetCapitalLocation()->GetTemplate().GetMapArea();

    for (const int provinceId : ourCountry->GetOwnedProvinces())
    {
        CProvince *prov = &CCurrentGameState::GetInstance()->GetProvince(provinceId);

        bool besieged = prov->GetSiege() != nullptr;
        bool besiegedOurs = besieged &&
                            CCountryTag::SameUpperWord(prov->GetController(), ourTag);

        const CFixedPoint dev = prov->GetDevelopment(false, true, nullptr);
        int totalDev = ourCountry->GetCachedDevelopmentRaw().GetUnderlyingValue();
        int devShare;
        if (totalDev == 0)
            devShare = -1;
        else if (::IsValidRange(dev.GetUnderlyingValue()))
            devShare = (dev.GetUnderlyingValue() * 1000) / totalDev;
        else
            devShare = static_cast<int>(static_cast<long long>(dev.GetUnderlyingValue()) * 1000 / totalDev);

        if (!CCountryTag::SameUpperWord(prov->GetController(), ourTag))
            return true;

        if (besiegedOurs)
        {
            const CMapArea *areaVal = prov->GetTemplate().GetMapArea();
            if (areaVal == specialArea ||
                NDefines::NAI::IMPORANT_PROVINCE_THRESHOLD.GetUnderlyingValue() < devShare)
                return true;
            if (prov->HasFort())
                return true;
        }
    }
    return false;
}

void CAIKing::PlanArmyTypes()
{
    const int numRegions = CAIRegionMap::GetNumberOfRegions();
    char *regionHunterCount = static_cast<char *>(__builtin_alloca(numRegions));
    std::memset(regionHunterCount, 0, numRegions);

    int maxSizeSoFar = 0;
    int totalEffStrength = 0;
    int bestEffStrength = 0;
    CAIArmy *bestArmy = nullptr;

    for (CList<CAIArmy *>::Node *cell = this->GetArmyAgents().GetHeadNode(); cell; cell = cell->_pNext)
    {
        CAIArmy *army = cell->_Data;

        army->ReclaimArmiesUnderCreation();
        army->SetHunterKiller(false);

        if (const CAIRegion *assigned = army->GetAssignedRegion())
            regionHunterCount[assigned->GetIndex()] += 1;

        if (army->IsForExploration(nullptr))
            continue;
        if (army->IsCondottieri())
            continue;

        int unitCount = 0;
        int effStrength = 0;
        for (CUnit *unit : *army->GetArmyList())
        {
            int n1 = unit->GetNumSubUnits();
            int n2 = unit->GetNumSubUnits();
            CFixedPoint avgStrength = unit->GetAverageStrength();
            unitCount += n1;
            CFixedPoint eff(static_cast<int32_t>(static_cast<int64_t>(n2) * 1000 * avgStrength.GetUnderlyingValue() / 1000));
            effStrength += eff.GetRounded();
        }

        if (army->GetBaseProvince() != 0)
        {
            CProvince *assignedProv =
                &CCurrentGameState::GetInstance()->GetProvince(army->GetBaseProvince());
            if (!assignedProv->IsOverseas(this->GetCountryTag(), true))
            {
                if (army->GetMainArmy() != nullptr)
                {
                    bool wasHunter = army->IsHunterKiller();
                    int size = wasHunter
                                   ? static_cast<int>(static_cast<double>(unitCount) * kHunterKillerSizeScale)
                                   : unitCount;
                    if (maxSizeSoFar < size)
                    {
                        bestEffStrength = effStrength;
                        bestArmy = army;
                    }
                    if (maxSizeSoFar < size)
                        maxSizeSoFar = size;
                }
            }
        }

        totalEffStrength += effStrength;
    }

    if (bestArmy != nullptr &&
        bestArmy->GetInvasionTarget() == 0 &&
        kMinArmiesForElection <= this->GetArmyAgents().GetSize())
    {
        if (kMinBestStrength <= bestEffStrength &&
            (static_cast<double>(bestEffStrength) * kHunterKillerSizeScale < totalEffStrength ||
             kTotalLeadThreshold < totalEffStrength - bestEffStrength))
        {
            const CAIRegion *assigned = bestArmy->GetAssignedRegion();
            if (assigned == nullptr)
            {
                bestArmy->SetHunterKiller(true);
            }
            else
            {
                int count = regionHunterCount[assigned->GetIndex()];
                if (count <= 1 && assigned->GetNeighbors().GetSize() > 0)
                {
                    for (int i = assigned->GetNeighbors().GetSize(); i >= 1; i--)
                    {
                        const CAIRegion *nb = assigned->GetNeighbors()[i - 1];
                        count += regionHunterCount[nb->GetIndex()];
                        if (count > 1)
                            break;
                    }
                }
                if (kCrowdedRegionArmies <= count)
                    bestArmy->SetHunterKiller(true);
            }
        }
    }
}

void CAIKing::FindInvasionTargets(CPdxHybridArray<CProvinceValue, 32, int> &PrioList)
{
    CCountry *ourCountry = CCountryDataBase::AccessInstance()
                               ->GetCountry(this->GetCountryTag());
    for (int enemyIndex = 0; enemyIndex < this->_EnemyList.GetSize(); ++enemyIndex)
    {
        const CCountryTag *tag = &this->_EnemyList[enemyIndex];
        if (CCountryTag::SameUpperWord(*tag, ourCountry->GetCountryTag()))
            continue;
        if (!ourCountry->GetDiplomacy().HasWar(*tag) &&
            !tag->IsRebelPirateNative() &&
            !ourCountry->GetCountryTag().IsRebelPirateNative())
            continue;

        const CWar *war = ourCountry->GetDiplomacy()[tag->GetIndex()]->GetWar();
        CCountryTag overlord = ourCountry->GetTopOverlord(true);
        if (war != nullptr)
            overlord = war->GetTopOverlordInWarTogether(this->GetCountryTag());
        if (ourCountry->IsSubject())
        {
            if (!ourCountry->IsLibertyDesireChainBelowLimit(
                    CFixedPoint(NDefines::NCountry::HIGH_LIBERTY_DESIRE), overlord))
                continue;
        }
        this->FindRemoteInvasionTargets(*tag, PrioList, war);
    }

    CList<std::unique_ptr<CControllerArea>>::Node *areaList = ourCountry->AccessControllerAreaList().GetHeadNode();
    for (CList<std::unique_ptr<CControllerArea>>::Node *entry = areaList; entry;)
    {
        const CControllerArea *area = entry->_Data.get();
        entry = entry->_pNext;
        const int areaCount = area->GetSize();
        if (areaCount == 0)
            continue;

        CList<CProvince *>::Node *provCell = area->GetProvinceList().GetHeadNode();
        CProvince *firstProv = provCell ? provCell->_Data : nullptr;
        bool valid = false;
        for (; provCell;)
        {
            CProvince *prov = provCell->_Data;
            if (!CCountryTag::SameUpperWord(prov->GetOwner(), prov->GetController()) ||
                prov->GetSiege() != nullptr)
            {
                valid = true;
                break;
            }
            provCell = provCell->_pNext;
            if (!CAIThreatEvaluation::GetInstance().ArmyHasNearbyThreat(
                    prov->GetProvinceID(), this->GetCountryTag(), 200, false))
            {
                continue;
            }
            valid = true;
            break;
        }
        if (!valid || firstProv == nullptr)
            continue;

        int count = std::max(kTargetCountMin, std::min(kTargetCountMax, areaCount));
        int value = count * kTargetPerArea + kHomeAreaBonus;
        if (ourCountry->GetCapitalLocation()->GetControllerArea() != area)
            value -= kHomeAreaBonus;
        if (!firstProv->IsOverseas(this->GetCountryTag(), true))
            value += kNotOverseasBonus;

        CProvinceValue v;
        v.SetId(firstProv->GetProvinceID());
        v.SetScore(value);

        PrioList.InsertSorted(v, CProvinceValueDescComp());
    }
}

void CAIKing::FindRemoteInvasionTargets(CCountryTag EnemyTag, CPdxHybridArray<CProvinceValue, 32, int> &PrioList, const CWar *pWar) const
{
    CCountry *targetCountry = CCountryDataBase::AccessInstance()
                                  ->GetCountry(EnemyTag);

    CList<std::unique_ptr<CControllerArea>>::Node *areaList = targetCountry->AccessControllerAreaList().GetHeadNode();
    if (areaList == nullptr)
        return;

    CCountry *ourCountry = CCountryDataBase::AccessInstance()
                               ->GetCountry(this->GetCountryTag());

    const bool tagSpecialOrInvalid =
        EnemyTag.GetCountryType() == CountryType::_COUNTRY_OBSERVER_ || !EnemyTag.IsValid();
    const bool ourTagSpecialOrInvalid =
        this->GetCountryTag().GetCountryType() == CountryType::_COUNTRY_OBSERVER_ || !this->GetCountryTag().IsValid();

    for (CList<std::unique_ptr<CControllerArea>>::Node *cell = areaList; cell;)
    {
        CControllerArea *area = cell->_Data.get();
        cell = cell->_pNext;
        if (area->GetSize() == 0)
            continue;

        CList<CProvince *>::Node *provCell = area->GetProvinceList().GetHeadNode();
        CProvince *firstProv = provCell->_Data;

        if (tagSpecialOrInvalid || EnemyTag.IsRebels())
        {
            if (!CCountryTag::SameUpperWord(firstProv->GetOwner(), this->GetCountryTag()))
                continue;
        }
        if (provCell == nullptr)
            continue;

        bool isRebelTarget = EnemyTag.IsRebels();
        int maxConquerValue = 0;
        int localAc = 0;
        bool wargoalMatch = false;
        bool inSupply = false;

        while (provCell != nullptr)
        {
            CProvince *prov = provCell->_Data;
            provCell = provCell->_pNext;

            maxConquerValue = std::max(maxConquerValue, ourCountry->GetStrategy().GetConquerValue(prov->GetProvinceID()));

            const CCountryTag ownerTag = prov->GetOwner();
            if (CCountryTag::SameUpperWord(ownerTag, EnemyTag) ||
                ourCountry->IsFriend(ownerTag, false, false, nullptr))
            {
                localAc = prov->GetProvinceID();
            }
            else if (!CCountryTag::SameUpperWord(ownerTag, this->GetCountryTag()))
            {
                if (ourCountry->GetDiplomacy().HasWar(ownerTag) ||
                    ownerTag.IsRebelPirateNative() ||
                    ourCountry->GetCountryTag().IsRebelPirateNative())
                {
                    localAc = prov->GetProvinceID();
                }
                else if (CCountryDataBase::AccessInstance()->GetCountry(
                                                               ownerTag)
                             ->IsPrimitive() &&
                         !ourCountry->IsPrimitive())
                {
                    localAc = prov->GetProvinceID();
                }
            }

            if (ownerTag.IsValid() &&
                prov->GetPort() != nullptr)
            {
                CProvince *poolProv = &CCurrentGameState::GetInstance()->GetProvince(
                    prov->GetTemplate().GetSeaZoneForPort());

                if (!isRebelTarget)
                {
                    bool d1 = ourTagSpecialOrInvalid || !prov->IsTerraIncognitaActive() ||
                              prov->IsDiscoveredBy(this->GetCountryTag());
                    bool d2 = ourTagSpecialOrInvalid || !poolProv->IsTerraIncognitaActive() ||
                              poolProv->IsDiscoveredBy(this->GetCountryTag());
                    if (d1 && d2)
                        isRebelTarget = true;
                }

                CProvince *supplySource = nullptr;
                if (ourCountry->IsInSupplyRange(poolProv, &supplySource))
                {
                    if (supplySource == nullptr ||
                        CCountryTag::SameUpperWord(supplySource->GetOwner(), this->GetCountryTag()))
                    {
                        inSupply = true;
                    }
                    else
                    {
                        CCountry *sourceController = CCountryDataBase::AccessInstance()
                                                         ->GetCountry(supplySource->GetOwner());
                        if (CCountryTag::SameUpperWord(sourceController->GetTopOverlord(true),
                                                       this->GetCountryTag()))
                            inSupply = true;
                    }
                }
            }

            if (pWar != nullptr &&
                (CCountryTag::SameUpperWord(pWar->GetOriginalAttacker(), this->GetCountryTag()) ||
                 CCountryTag::SameUpperWord(pWar->GetOriginalDefender(), this->GetCountryTag())))
            {
                const CWarGoal *goal = pWar->GetWarGoal();
                if (goal->GetProvince() == prov->GetProvinceID())
                    wargoalMatch = true;
            }
        }

        if (!isRebelTarget || localAc == 0)
            continue;

        const CSubjectType *subjectType = ourCountry->GetSubjectType();
        if (subjectType != nullptr && subjectType->HasColonialParent() &&
            firstProv->IsOverseas(this->GetCountryTag(), true))
            continue;

        int areaCount = area->GetSize();
        if (areaCount == 1)
        {
            CProvince *conquerProv = &CCurrentGameState::GetInstance()->GetProvince(localAc);
            if (conquerProv->GetTemplate().IsIsland(true))
            {
                int unitCount = 0;
                for (CUnit *unit : conquerProv->GetUnits())
                {
                    if (!unit->IsArmy())
                        continue;
                    CCountryTag unitTag = unit->GetAllegiance();
                    if (CCountryTag::SameUpperWord(unitTag, this->GetCountryTag()) ||
                        ourCountry->IsInSameWarAgainst(unitTag, conquerProv->GetController()))
                    {
                        unitCount += unit->GetNumSubUnits();
                    }
                }

                for (CAIAgent *childAgent : this->_AgentList)
                {
                    CAIArmy *army = static_cast<CAIArmy *>(childAgent);
                    if (!army->IsArmy())
                        continue;
                    bool countThem;
                    if (army->GetInvasionTarget() == localAc)
                    {
                        countThem = true;
                    }
                    else
                    {
                        CArmy *main = army->GetMainArmy();
                        if (main != nullptr)
                        {
                            CArmy *main2 = army->GetMainArmy();
                            countThem = main2->GetFinalDestination() == conquerProv;
                        }
                        else
                        {
                            countThem = false;
                        }
                    }
                    if (countThem)
                    {
                        for (CUnit *unit : *army->GetArmyList())
                        {
                            if (unit->GetLocation()->GetProvinceID() != localAc)
                                unitCount += unit->GetNumSubUnits();
                        }
                    }
                }

                if (unitCount >= ::GetTargetRegimentCount(this->GetCountryTag(), conquerProv))
                    continue;
            }
        }

        int count = std::max(1, std::min(10, areaCount));
        int score = count * 10;

        CCountryTag areaCtlTag = area->GetOwner();
        bool isWarLeader = false;
        if (pWar != nullptr)
            isWarLeader = pWar->IsWarLeader(areaCtlTag);
        if (pWar == nullptr || !isWarLeader)
        {
            area->GetOwner();
            if (pWar != nullptr)
            {
                CCountry *ctlCountry = CCountryDataBase::AccessInstance()
                                           ->GetCountry(areaCtlTag);
                if (ctlCountry->GetOverlord().IsValid())
                {
                    area->GetOwner();
                    isWarLeader = pWar->IsWarLeader(ctlCountry->GetTopOverlord(true));
                }
            }
        }

        bool areaFlag = CCountryTag::SameUpperWord(areaCtlTag, this->GetCountryTag());
        if (targetCountry->GetCapitalLocation()->GetControllerArea() == area)
            score += 100;
        CControllerArea *ourHome = ourCountry->GetCapitalLocation()->GetControllerArea();
        if (ourHome == area)
        {
            score += 100;
            areaFlag = true;
        }
        else
        {
            areaFlag = areaFlag || (targetCountry->GetCapitalLocation()->GetControllerArea() == area);
            if (ourHome != nullptr && area->IsAdjacent(*ourHome))
            {
                score += 100;
                areaFlag = true;
            }
        }
        score += maxConquerValue;

        if (EnemyTag.IsRebels())
        {
            if (CCountryTag::SameUpperWord(firstProv->GetOwner(), this->GetCountryTag()))
            {
                areaFlag = true;
                score += 100;
            }
            CCountry *controllerCountry = CCountryDataBase::AccessInstance()
                                              ->GetCountry(firstProv->GetOwner());
            if (!CCountryTag::SameUpperWord(controllerCountry->GetCountryTag(), this->GetCountryTag()))
            {
                if (controllerCountry->GetDiplomacy().HasWar(this->GetCountryTag()) ||
                    this->GetCountryTag().IsRebelPirateNative() ||
                    controllerCountry->GetCountryTag().IsRebelPirateNative())
                {
                    score -= 100;
                    areaFlag = true;
                }
            }
        }

        if (wargoalMatch)
            score += 1000;
        if (!firstProv->IsOverseas(firstProv->GetOwner(), true))
            score += 50;

        if (!(inSupply || isWarLeader))
            continue;
        if (!(inSupply || areaFlag || wargoalMatch))
            continue;
        if (!(count > 3 || areaFlag || wargoalMatch ||
              (area->GetNeighborList().GetSize() != 0) ||
              targetCountry->GetCapitalLocation()->GetController().GetCategoryWord() != EnemyTag.GetCategoryWord()))
            continue;

        if (!CCountryTag::SameUpperWord(firstProv->GetOwner(), this->GetCountryTag()) &&
            firstProv->IsOverseas(firstProv->GetOwner(), true) && !wargoalMatch)
        {
            score = count;
        }

        CProvinceValue v;
        v.SetId(localAc);
        v.SetScore(score);

        PrioList.InsertSorted(v, CProvinceValueDescComp());
    }
}

float CAIKing::GetShareOfCurrentlyInvadingTroops()
{
    float invading = 0.0f;
    float total = 0.0f;

    for (CList<CAIArmy *>::Node *cell = this->GetArmyAgents().GetHeadNode(); cell; cell = cell->_pNext)
    {
        CAIArmy *army = cell->_Data;

        bool inTransit = false;
        if (army->GetInvasionTarget() != 0)
            inTransit = CCurrentGameState::GetInstance()->GetCurrentDate().GetUniversalDate() <
                        army->GetInvasionDate().GetUniversalDate();

        for (CUnit *unit : *army->GetArmyList())
        {
            CFixedPoint strength = unit->GetTotalStrength();
            float s = strength.GetUnderlyingValue() / kMilli;
            if (inTransit)
                invading += s;
            total += s;
        }
    }

    if (total < kFltEpsilon)
        return 0.0f;
    return invading / total;
}

int CAIKing::CalcInvasionAreaDivNeed(const CProvince &TargetProv,
                                     CCountryFlagCache<char> &AccessCache) const
{
    auto accumulateAreaUnits = [this](CControllerArea *area, CCountryTag enemy,
                                      const CProvince &invasionTarget, float &impact, float &total,
                                      int &maxGarrison) -> void
    {
        for (CList<CProvince *>::Node *cell = area->GetProvinceList().GetHeadNode(); cell; cell = cell->_pNext)
        {
            CProvince *prov = cell->_Data;
            for (CUnit *unit : prov->GetUnits())
                this->CalcDivNeedImpactForUnit(unit, enemy, invasionTarget, impact, total);
            int garrison = prov->GetGarrison().GetUnderlyingValue();
            if (maxGarrison < garrison)
            {
                prov->GetGarrison();                       
                maxGarrison = garrison;
            }
        }
    };

    auto areaControllerAllowsAccess = [this](CControllerArea *area,
                                             CCountryFlagCache<char> &accessCache) -> bool
    {
        CCountry *controller = CCountryDataBase::AccessInstance()
                                   ->GetCountry(area->GetOwner());
        if (controller->GetOwnedProvinces().GetSize() == 0 &&
            controller->GetCountryTag().GetCountryType() != CountryType::_COUNTRY_OBSERVER_)
            return false;
        CCountry *ourCountry = CCountryDataBase::AccessInstance()
                                   ->GetCountry(this->GetCountryTag());
        return ourCountry->HaveAccess(area->GetProvinceList().GetHeadNode()->_Data, false, false,
                                      nullptr, &accessCache);
    };

    float impact = 0.0f;
    float total = 0.0f;
    int maxGarrison = 0;
    int divNeed = 0;

    CControllerArea *area = TargetProv.GetControllerArea();
    if (area == nullptr)
        return 0;

    CCountry *ownerCountry = CCountryDataBase::AccessInstance()
                                 ->GetCountry(TargetProv.GetController());

    CCountryTag enemy = CCountryTag::FromRaw(kEmptyCountryTag);
    if (!CCountryTag::SameUpperWord(this->GetCountryTag(), ownerCountry->GetCountryTag()) &&
        (ownerCountry->GetDiplomacy().HasWar(this->GetCountryTag()) ||
         this->GetCountryTag().IsRebelPirateNative() ||
         ownerCountry->GetCountryTag().IsRebelPirateNative()))
    {
        enemy = TargetProv.GetController();
    }

    if (area->GetProvinceList().GetHeadNode() != nullptr)
        accumulateAreaUnits(area, enemy, TargetProv, impact, total, maxGarrison);

    CList<CNeighborArea>::Node *visitedHead = nullptr;
    CList<CNeighborArea>::Node *visitedTail = nullptr;
    int visitedCount = 0;

    auto isVisitedArea = [&visitedHead](const CList<CNeighborArea>::Node *cell) -> bool
    {
        for (const CList<CNeighborArea>::Node *node = visitedHead; node != nullptr; node = node->_pNext)
            if ((&node->_Data._Area) == (&cell->_Data._Area) &&
                node->_Data._nRefCount == cell->_Data._nRefCount)
                return true;
        return false;
    };
    auto visitArea = [&visitedHead, &visitedTail, &visitedCount](
                         const CList<CNeighborArea>::Node *cell) -> void
    {
        CList<CNeighborArea>::Node *node =
            new CList<CNeighborArea>::Node{{cell->_Data._Area, cell->_Data._nRefCount}, visitedTail, nullptr, 0};
        if (visitedCount != 0)
            visitedTail->_pNext = node;
        else
            visitedHead = node;
        visitedTail = node;
        visitedCount += 1;
    };

    for (CList<CNeighborArea>::Node *cell = area->GetNeighborList().GetHeadNode(); cell; cell = cell->_pNext)
    {
        if (isVisitedArea(cell) || (&cell->_Data._Area) == area)
            continue;
        visitArea(cell);

        if (areaControllerAllowsAccess((&cell->_Data._Area), AccessCache))
        {
            accumulateAreaUnits((&cell->_Data._Area), enemy, TargetProv, impact, total, maxGarrison);

            for (CList<CNeighborArea>::Node *cell2 = cell->_Data._Area.GetNeighborList().GetHeadNode(); cell2; cell2 = cell2->_pNext)
            {
                if (isVisitedArea(cell2) || (&cell2->_Data._Area) == area)
                    continue;
                visitArea(cell2);
                if (areaControllerAllowsAccess((&cell2->_Data._Area), AccessCache))
                    accumulateAreaUnits((&cell2->_Data._Area), enemy, TargetProv, impact, total, maxGarrison);
            }
        }
    }

    while (visitedHead != nullptr)
    {
        CList<CNeighborArea>::Node *next = visitedHead->_pNext;
        delete visitedHead;
        visitedHead = next;
    }

    impact = impact + impact;
    float slack = (total - impact) +
                  ((total - impact) < 0.0f ? kRebelSlackNeg : kRebelSlackPos);
    const int slackInt = static_cast<int>(slack);
    divNeed = slackInt + 1;
    if (slackInt < 2)
        divNeed = 2;
    CFixedPoint garrisonFP(maxGarrison / 1000);
    if (divNeed < garrisonFP.GetRounded())
        divNeed = garrisonFP.GetRounded();
    return divNeed;
}

CAIArmy *CAIKing::FindInvasionArmy(const CProvince &Target, const CProvince **pBestTarget,
                                   CCountryFlagCache<char> &AccessCache)
{
    CControllerArea *targetArea = Target.GetControllerArea();
    if (targetArea == nullptr)
        return nullptr;

    const int today = CCurrentGameState::GetInstance()->GetCurrentDate().GetUniversalDate();
    int homeRegiments, awayRegiments, armyCount, minRegiments;
    this->CountArmiesPlacements(homeRegiments, awayRegiments, armyCount, minRegiments);
    (void)homeRegiments;
    (void)awayRegiments;
    (void)armyCount;
    (void)minRegiments;

    const float maxAwayRatio = NDefines::NAI::INVADING_MAX_AWAY_RATIO.GetUnderlyingValue() / kMilli;
    if (!(this->GetShareOfCurrentlyInvadingTroops() < maxAwayRatio))
        return nullptr;

    CStrengthFactorCache &cache = CStrengthFactorCacheManager::AccessCache();

    CCountry *ourCountry = CCountryDataBase::AccessInstance()
                               ->GetCountry(this->GetCountryTag());

    auto isHostile = [&ourCountry](CCountryTag other) -> bool
    {
        return ourCountry->GetDiplomacy().HasWar(other) ||
               other.IsRebelPirateNative() ||
               ourCountry->GetCountryTag().IsRebelPirateNative();
    };

    CAIArmy *bestArmy = nullptr;
    float best = kFLTMax;

    for (CList<CAIArmy *>::Node *cell = this->GetArmyAgents().GetHeadNode(); cell; cell = cell->_pNext)
    {
        CAIArmy *army = cell->_Data;

        if (army->IsForExploration(nullptr))
            continue;
        if (army->IsCondottieri())
            continue;
        CArmy *main = army->GetMainArmy();
        if (main == nullptr || main->GetLocation() == nullptr)
            continue;

        for (CUnit *unit : *army->GetArmyList())
            (void)unit->GetNumSubUnits();

        if (!(army->GetInvasionTarget() < 1))
            continue;
        if (!(static_cast<uint32_t>(army->GetInvasionDate().GetUniversalDate()) <= static_cast<uint32_t>(today)))
            continue;
        if (!army->CanInvade(Target.GetProvinceID()))

            continue;
        if (!(main->GetNumSubUnits() > 0))
            continue;
        if (main->HasExileStatus())
            continue;
        if (main->GetMission() != nullptr)
            continue;

        if (main->GetLocation()->GetControllerArea() != nullptr &&
            main->GetLocation()->GetControllerArea()->CalcIsWarZone(this->GetCountryTag(), false))
            continue;

        if (!CCountryTag::SameUpperWord(main->GetLocation()->GetController(), ourCountry->GetCountryTag()) &&
            !isHostile(main->GetLocation()->GetController()))
            continue;
        if (!CCountryTag::SameUpperWord(Target.GetAreaController(), ourCountry->GetCountryTag()) &&
            !isHostile(Target.GetAreaController()))
            continue;

        const auto threatAllowsInvasion = [&]() -> bool
        {
            CFixedPoint travelTime =
                CPathFind::GetRoughlyEstimatedTravelTime(main->GetLocation(), &Target);
            CEU3Date horizon = CCurrentGameState::GetInstance()->GetCurrentDate();
            horizon.SetUniversalDate(today + (travelTime.GetUnderlyingValue() / 1000) * kTravelDayHours + kTravelDateExtra);
            CFixedPoint threat = CAIThreatEvaluation::GetInstance().CalcThreatForArmy(
                Target.GetProvinceID(), horizon, main, false, cache);
            if (NDefines::NAI::INVADING_BRAVERY.GetUnderlyingValue() < threat.GetUnderlyingValue())
                return false;
            if (100 < threat.GetUnderlyingValue())
            {
                CFixedPoint avgStrength = main->GetAverageStrength();
                CFixedPoint avgMorale = main->CUnit::GetAverageMorale(false);
                if (avgStrength.GetUnderlyingValue() < kMinAvgStrength || avgMorale.GetUnderlyingValue() < kMinAvgStrength)
                    return false;
            }
            return true;
        };
        if (!threatAllowsInvasion())
            continue;

        int needed = main->GetNumSubUnits();
        bool viable;
        const CProvince *staging = &Target;
        if (main->GetLocation()->GetControllerArea() == targetArea ||
            main->GetLocation()->GetControllerArea()->IsAdjacent(*targetArea) ||
            !army->MayInvade())
        {
            viable = true;
        }
        else if (army->CanReach(main, &Target, AccessCache))
        {
            viable = true;
        }
        else
        {
            CProvince *invasionTarget = army->CalcInvasionTarget(targetArea, nullptr);
            staging = invasionTarget;
            if (staging == nullptr)
                continue;
            if (main->IsUnsafeMove(staging->GetProvinceID()))
                continue;

            CFixedPoint dist = main->GetLocation()->GetDistance(staging, false);
            if (kMinStagingDist < dist.GetUnderlyingValue() / kMilli)
            {
                CEU3Date horizon = CCurrentGameState::GetInstance()->GetCurrentDate();
                horizon.SetUniversalDate(today + kThreatHorizonDays);
                CFixedPoint threat = CAIThreatEvaluation::GetInstance().CalcArmyThreatForCountry(
                    Target.GetProvinceID(), horizon, this->GetCountryTag(), false, cache);
                if (static_cast<uint32_t>(threat.GetUnderlyingValue() - kBraveryLow) < kBraveryHigh)
                    continue;
            }

            int outA = 0, outB = 0;
            bool bySea = army->CanInvadeBySea(main, invasionTarget,
                                              outA, outB, AccessCache, false);
            if (!bySea)
            {
                CList<int> landPath;
                bool byLand = army->CanGoByLand(main, invasionTarget,
                                                landPath, AccessCache, -1);
                if (!byLand)
                    continue;
            }
            else
            {
                int transports = ourCountry->GetUnits().GetTotalNumOfTransports();
                needed = std::min(needed, transports);
                bool nearbyThreat = CAIThreatEvaluation::GetInstance().ArmyHasNearbyThreat(
                    staging->GetProvinceID(), this->GetCountryTag(),
                    kNearbyThreatRange, !ourCountry->IsAtWar());
                if (!nearbyThreat)
                    needed = main->GetNumSubUnits();
                if (army->IsHunterKiller())
                {
                    if (army->GetAssignedRegion() != CAIRegionMap::GetRegion(staging))
                        continue;
                }
            }

            CCountryTag enemy;
            army->DetermineEnemy(enemy, main, staging);
            if (enemy.IsRebels())
            {
                float total = 0.0f, impact = 0.0f;
                for (CUnit *unit : staging->GetUnits())
                    this->CalcDivNeedImpactForUnit(unit, enemy, *staging, impact, total);
                float slack = (total - impact) +
                              ((total - impact) < 0.0f ? kRebelSlackNeg : kRebelSlackPos);
                if (!(static_cast<int>(slack) < needed))
                    continue;
            }
            else
            {
                int spare = std::max(ourCountry->GetWarExhaustion().GetUnderlyingValue(), kMinSpareFloor);
                int divNeed = this->CalcInvasionAreaDivNeed(*staging, AccessCache);
                CFixedPoint tactics = ourCountry->GetCountryMilitaryTactics(nullptr);
                int64_t threshold = static_cast<int64_t>(tactics.GetUnderlyingValue()) * needed;
                threshold = (5000000 / spare) * threshold / 1000;
                const CWar *war = ourCountry->GetDiplomacy()[staging->GetOwner().GetIndex()]
                                      ->GetWar();
                if (war != nullptr &&
                    !CCountryTag::SameUpperWord(war->GetOriginalAttacker(), ourCountry->GetCountryTag()) &&
                    !CCountryTag::SameUpperWord(war->GetOriginalDefender(), ourCountry->GetCountryTag()))
                {
                    int manpowerCap = ourCountry->GetMaxManpower().GetUnderlyingValue();
                    int manpowerShare = 0;
                    if (manpowerCap != 0)
                    {
                        if (::IsValidRange(ourCountry->GetManpower().GetUnderlyingValue()))
                            manpowerShare = (ourCountry->GetManpower().GetUnderlyingValue() * 1000) / manpowerCap;
                        else
                            manpowerShare = static_cast<int>(static_cast<long long>(ourCountry->GetManpower().GetUnderlyingValue()) * 1000 /
                                                             manpowerCap);
                    }
                    if (kDevShareHigh < manpowerShare)
                    {
                        if (manpowerShare < kDevShareHalfMax)
                            threshold /= 2;
                    }
                    else
                    {
                        threshold = (threshold < 0 ? threshold + 3 : threshold) >> 2;
                    }
                }
                if (staging->GetOwnerArea() == ourCountry->GetCapitalLocation()->GetOwnerArea())
                    threshold = threshold * kHomeAreaBoost / 1000;
                if (!(divNeed <= threshold))
                    continue;
            }
        }
        (void)viable;

        CFixedPoint dist = main->GetLocation()->GetDistance(staging, false);
        float ratio = (dist.GetUnderlyingValue() / kMilli) / needed;
        if (ratio < best)
        {
            *pBestTarget = staging;
            bestArmy = army;
            best = ratio;
        }
    }
    return bestArmy;
}

void CAIKing::CreateArmy(CID id, CProvince *pLocation, CArmy *pAgentArmy,
                         bool bExileStatus, int nPrev, int nPrevNoZoC,
                         int nAIDivisionAt, bool bWasPrevious, int nAIDivisionOf)
{
    if (pAgentArmy != nullptr)
    {
        SUnitToUnit *s2u = new SUnitToUnit(id, pAgentArmy->GetID());
        this->_UnitAssignments.Add(s2u);
    }

    if (!pLocation->GetTemplate().IsLand())
    {
        CLogStream logStream(CLogger::AccessInstance(),
                             "/mnt/gsg/eu4/eu4/eu4/source/ai_king.cpp", 0x1011, 0x10003);
        logStream << "AI attempting to create fleet with CreateArmy: ";
        static_cast<std::ostream &>(logStream) << pLocation->GetProvinceID();
        static_cast<std::ostream &>(logStream) << " ";
        const CCountryTag tag = this->GetCountryTag();
        static_cast<std::ostream &>(logStream)
            << reinterpret_cast<const char *>(&tag);
    }

    CCreateUnitCommand *cmd = new CCreateUnitCommand(
        this->GetCountryTag(), id, pLocation->GetProvinceID(),
        false, false, bExileStatus, false, false,
        nPrev, nPrevNoZoC, nAIDivisionAt, bWasPrevious, nAIDivisionOf,
        false, nullptr, nullptr);
    ::PostAICommand(cmd);
}

void CAIKing::AssignSubUnitProperly(CID id, CSubUnit *pSub)
{
    ::PostAICommand(std::unique_ptr<CCommand>(new CTransferSubUnitCommand(id, pSub)));
}
