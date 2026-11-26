#include "ai.h"
#include "country_modifiers.h"
#include "land_maintenance.h"
#include "mercenary.h"
#include "modifier_enums.h"
#include "provincetemplate.h"
#include "subjecttype.h"
#include <algorithm>

#include "ai_economy.h"
#include "ai_king.h"
#include "ai_recruit.h"
#include "ai_threatevaluation.h"
#include "areas.h"
#include "clausewitz/pdx_core/text.h"
#include "construction.h"
#include "constructioncommands.h"
#include "country.h"
#include "countrycommands.h"
#include "countrytag.h"
#include "defines.h"
#include "flagship.h"
#include "gamestate.h"
#include "government_reform_stack.h"
#include "governmentinstance.h"
#include "modifier.h"
#include "pdx_dlc.h"
#include "playeraiprefs.h"
#include "province.h"
#include "rebelfaction.h"
#include "strength_factor_cache.h"
#include "subunit.h"
#include "unit.h"
#include "unitcommands.h"
#include "unitlist.h"

#include <vector>

extern unsigned char PotentialProvincesTL[0x60] asm("_ZN12_GLOBAL__N_120PotentialProvincesTLE");

extern void *table_lookup(void *pThreadSpecific, bool &bExists) asm("_ZN3tbb10interface68internal8ets_baseILNS_18ets_key_usage_typeE1EE12table_lookupERb"); 

static CFixedPoint GetUnitPower(const CSubUnitDefinition *pDef, CCountryTag Tag)
{
    const CSubUnitDefinition *def = pDef;
    int sum = def->GetOffensiveFire() + def->GetOffensiveShock() + def->GetOffensiveMorale() + def->GetDefensiveShock() + def->GetDefensiveFire() + def->GetDefensiveMorale();
    int scaled = sum * 1000 + 0x9c40;
    CFixedPoint impact = def->GetPowerImpact(Tag);
    return CFixedPoint(::Div1000(static_cast<int64_t>(scaled) * impact.GetUnderlyingValue()));
}

namespace
{
SDesiredTypeResult GetNextDesiredArmyType(const CCountry &Country, ESpecialUnitCategory Category,
                                                    const CProvince *pProvince, const CProvince *pGatheringProvince,
                                                    const CEnumIndexedArray<int, SubUnitType, 7> &CurrentRegiments,
                                                    CFixedPoint vWillingToSpend, CFixedPoint vLeftToSpend, int nRandom)
{
    CFixedPoint infShareFP, cavShareFP, artShareFP;
    int forceLimit = Country.GetLandForceLimit(nullptr).GetUnderlyingValue();
    int landMaint = Country.GetStrategy().GetBudget(AIBudgetType::ARMIES).GetUnderlyingValue();

    CFixedPoint baseCostScale;
    baseCostScale.SetUnderlyingValue(forceLimit);
    CFixedPoint budgetMaint;
    budgetMaint.SetUnderlyingValue(landMaint);
    CAIRecruitMinister::GetDesiredLandRegimentTypeFractions(Country, baseCostScale, budgetMaint,
                                                            infShareFP, cavShareFP,
                                                            artShareFP);
    int infShare = infShareFP.GetUnderlyingValue();
    int cavShare = cavShareFP.GetUnderlyingValue();
    int artShare = artShareFP.GetUnderlyingValue();
    int desiredRegiments = Country.GetStrategy().CalcDesiredAmountOfRegiments();
    int64_t scaled = static_cast<int64_t>(desiredRegiments) * 1000;

    int infTarget = ::Div1000(infShare * scaled) / 1000;
    int cavTarget = ::Div1000(cavShare * scaled) / 1000;
    int artTarget = ::Div1000(artShare * scaled) / 1000;

    bool onlyInfantryMercs = 0 < NDefines::NAI::ONLY_INFANTRY_MERCS;
    bool hasProvince = pProvince != nullptr;
    bool constrained = false;

    auto manpowerUsage = [&Country, Category]() -> int
    {
        const CFixedPoint usage = CSubUnit::GetManpowerOrSailorsUsagePercentage(Country,
                                                                                Category);
        return usage.GetUnderlyingValue();
    };
    if (hasProvince || onlyInfantryMercs)
    {
        if (manpowerUsage() < 1)
        {
            constrained = true;
        }
        else
        {
            int cap = ::Div1000(static_cast<int64_t>(Country.GetMaxManpower().GetUnderlyingValue()) * 330);
            if (cap <= Country.GetManpower().GetUnderlyingValue())
                constrained = true;
            else
                constrained = (CurrentRegiments[SubUnitType::INFANTRY] * 4 <= infTarget * 3);
        }
    }
    else
    {
        constrained = true;
    }

    bool infAvail = Country.GetSubunitTypePreferred(SubUnitType::INFANTRY)->IsValid();
    bool cavAvail = Country.GetSubunitTypePreferred(SubUnitType::CAVALRY)->IsValid();
    bool artAvail = Country.GetSubunitTypePreferred(SubUnitType::ARTILLERY)->IsValid();
    nRandom = nRandom % 3;

    int choice = 0;
    if (nRandom != 0 || !constrained)
    {
        if (nRandom == 2)
        {
            if (CurrentRegiments[SubUnitType::ARTILLERY] < artTarget)
                choice = artAvail ? 2 : 0;
        }
        else if (nRandom == 1)
        {
            choice = (CurrentRegiments[SubUnitType::CAVALRY] < cavTarget && cavAvail) ? 1 : 0;
        }
    }

    const auto refineChoice = [&]() -> bool
    {
        int fallback = choice;
        if (!constrained)
        {
            choice = 1;
            if ((cavTarget <= CurrentRegiments[SubUnitType::CAVALRY]) || !cavAvail)
            {
                if (CurrentRegiments[SubUnitType::ARTILLERY] < artTarget && artAvail)
                {
                    fallback = 2;
                }
                else
                {
                    fallback = choice;
                    if (!(CurrentRegiments[SubUnitType::CAVALRY] < cavTarget && cavAvail))
                    {
                        if (CurrentRegiments[SubUnitType::ARTILLERY] < artTarget && artAvail)
                            fallback = 2;
                        else if (!(CurrentRegiments[SubUnitType::CAVALRY] < cavTarget * 2 && cavAvail) &&
                                 (artTarget * 2 <= CurrentRegiments[SubUnitType::ARTILLERY] || !artAvail))
                        {
                            choice = 0;
                            return false;
                        }
                    }
                }
            }
            if (0 >= CurrentRegiments[SubUnitType::CAVALRY])
            {
                if (!constrained || (5 < CurrentRegiments[SubUnitType::INFANTRY] || !infAvail))
                {
                    choice = 1;
                    if (!cavAvail)
                        choice = fallback;
                    if (1 < CurrentRegiments[SubUnitType::CAVALRY])
                        choice = fallback;
                }
            }
        }
        else
        {
            if (!(infTarget <= CurrentRegiments[SubUnitType::INFANTRY]) && infAvail)
            {
                if (2 < CurrentRegiments[SubUnitType::INFANTRY])
                    return true;
            }
            else
            {
                choice = 1;
                return true;
            }
        }
        if ((0 < CurrentRegiments[SubUnitType::CAVALRY]) || !cavAvail)
        {
            if (!constrained || (5 < CurrentRegiments[SubUnitType::INFANTRY] || !infAvail))
            {
                choice = 1;
                if (!cavAvail)
                    choice = fallback;
                if (1 < CurrentRegiments[SubUnitType::CAVALRY])
                    choice = fallback;
            }
        }
        return true;
    };
    if (!refineChoice())
        return {SubUnitType::INFANTRY, false};

    int chosen = choice;
    const CSubUnitDefinition *chosenDef = Country.GetSubunitTypePreferred(
        static_cast<SubUnitType>(choice));
    int chosenMaint;
    const auto maintenanceCost1 = [&]() -> void
    {
        CFixedPoint size(1000);
      chosenMaint = Country.CalcActualLandMaintenanceCost(
          chosenDef, ESpecialUnitCategory::NORMAL, size, nullptr, nullptr).GetUnderlyingValue();
    };
    maintenanceCost1();
    chosen = choice;
    if (vWillingToSpend.GetUnderlyingValue() < chosenMaint)
    {
        bool freeA = (chosen == 0 || !constrained ||
                      (infTarget <= CurrentRegiments[SubUnitType::INFANTRY] &&
                       (Category == ESpecialUnitCategory::MERCENARY || CurrentRegiments[SubUnitType::CAVALRY] < 1)));
        bool freeB = ((chosen != 2 || cavTarget <= CurrentRegiments[SubUnitType::CAVALRY]) &&
                      Country.IsAtWar() &&
                      manpowerUsage() == 0);
        if (freeA && freeB)
            chosen = 0;
    }
    const CSubUnitDefinition *finalDef = Country.GetSubunitTypePreferred(
        static_cast<SubUnitType>(chosen));
    int finalMaint;
    const auto maintenanceCost2 = [&]() -> void
    {
        CFixedPoint size(1000);
      finalMaint = Country.CalcActualLandMaintenanceCost(
          finalDef, Category,
          size, nullptr, nullptr).GetUnderlyingValue();
    };
    maintenanceCost2();
    int buildCost;
    buildCost = Country.GetBuildCost(Category,
                                     static_cast<SubUnitType>(chosen),
                                     pProvince, 0, nullptr)
                    .GetUnderlyingValue();
    if (pGatheringProvince == nullptr && vWillingToSpend.GetUnderlyingValue() < finalMaint)
    {
        return {static_cast<SubUnitType>(chosen), Country.GetTreasury().GetUnderlyingValue() < buildCost};
    }

    bool costFails = false;
    int finalType = chosen;
    const auto refineCost = [&]() -> void
    {
        int cost;
        cost = Country.GetBuildCost(ESpecialUnitCategory::NORMAL, static_cast<SubUnitType>(chosen),
                                    pProvince, 0, nullptr)
                   .GetUnderlyingValue();
        if (vLeftToSpend.GetUnderlyingValue() < cost)
        {
            if (chosen != 0 && constrained &&
                CurrentRegiments[SubUnitType::INFANTRY] < infTarget)
            {
                int infCost;
                infCost = Country.GetBuildCost(ESpecialUnitCategory::NORMAL, SubUnitType::INFANTRY,
                                               pProvince, 0, nullptr)
                              .GetUnderlyingValue();
                if (infCost < vLeftToSpend.GetUnderlyingValue())
                    return;
            }
            finalType = chosen;
            if (chosen == 2)
            {
                if (CurrentRegiments[SubUnitType::CAVALRY] < cavTarget)
                {
                    int cavCost;
                    cavCost = Country.GetBuildCost(ESpecialUnitCategory::NORMAL, SubUnitType::CAVALRY,
                                                   pProvince, 0, nullptr)
                                  .GetUnderlyingValue();
                    finalType = 2 - (cavCost < vLeftToSpend.GetUnderlyingValue());
                }
                else
                {
                    finalType = 2;
                }
            }
        }
    };
    refineCost();
    int refinedType = finalType;
    if (hasProvince || onlyInfantryMercs)
        finalType = 0;
    if (Category != ESpecialUnitCategory::MERCENARY)
        finalType = refinedType;
    return {static_cast<SubUnitType>(finalType), costFails};
}
}


bool SLandDisbandEval::WillReallySoonFightRebels(int nMonths, bool bAbortOnNoncontrolled) const
{
    const CCountry *country = &this->Country;
    const auto *home = country->GetHomeOwnerArea();
    if (home == nullptr || this->vRatio.GetUnderlyingValue() >= 0x3e9)
        return false;
    home = country->GetHomeOwnerArea();

    std::vector<const CRebelFaction *> factions;
    factions.reserve(home->GetSize());

    for (CProvince *prov : home->GetProvinceList())
    {
        if (bAbortOnNoncontrolled &&
            (!CCountryTag::SameUpperWord(prov->GetController(), prov->GetOwner()) ||
             prov->GetSiege() != nullptr))
            return false;

        CRebelFaction *faction = prov->GetRevoltRebelFaction();
        if (faction != nullptr &&
            std::find(factions.begin(), factions.end(), faction) == factions.end())
        {
            factions.push_back(faction);
            CFixedPoint progress = faction->GetUprisingProgress();
            CFixedPoint monthly = faction->GetMonthlyProgressIncreaseChance();
            int scaled = ::Div1000(static_cast<int64_t>(progress.GetUnderlyingValue()) * (nMonths * 10000));
            if (99999 < scaled + monthly.GetUnderlyingValue())
                return true;
        }
    }
    return false;
}

CFixedPoint CAIRecruitMinister::GetDesiredCavToInfRatio(const CCountry &Country)
{
    int capMod[2];
    const CModifier *modifiers = &Country.GetCachedData().GetModifiers().GetModifier();
    capMod[0] = modifiers->GetValue(_MODIFIER_CAV_TO_INF_RATIO_).GetUnderlyingValue();
    int maxCav = NDefines::NAI::MAX_CAV_PERCENTAGE.GetUnderlyingValue() / 100;
    if (capMod[0] < maxCav)
        maxCav = capMod[0];

    if (!Country.GetReformStack().IsNomad())
    {
        int infBuildCost;
        int cavBuildCost;
        infBuildCost = Country.GetBuildCost(ESpecialUnitCategory::NORMAL, SubUnitType::INFANTRY, nullptr, 0, nullptr).GetUnderlyingValue();
        cavBuildCost = Country.GetBuildCost(ESpecialUnitCategory::NORMAL, SubUnitType::CAVALRY, nullptr, 0, nullptr).GetUnderlyingValue();
        const CSubUnitDefinition *infPreferred = Country.GetSubunitTypePreferred(SubUnitType::INFANTRY);
        const CSubUnitDefinition *cavPreferred = Country.GetSubunitTypePreferred(SubUnitType::CAVALRY);

        int combatWidth = Country.GetCombatWidth().GetUnderlyingValue();
        int forceLimit = Country.GetLandForceLimit(nullptr).GetUnderlyingValue();
        int nominalSize = Country.CalcNominalArmySizeNotForAI() * 1000;
        int effLimit = (forceLimit < nominalSize) ? forceLimit : nominalSize;

        int supplyFactor = -1;
        if (effLimit != 0)
            supplyFactor = ::Div1000Checked(combatWidth, effLimit);

        const int cavPower = ::GetUnitPower(cavPreferred, Country.GetCountryTag()).GetUnderlyingValue();
        const int infPower = ::GetUnitPower(infPreferred, Country.GetCountryTag()).GetUnderlyingValue();
        int ratio = -1;
        int denom = infPower * cavBuildCost / 1000;
        if (denom != 0)
        {
            int num = cavPower * infBuildCost / 1000;
            ratio = ::Div1000Checked(num, denom);
        }
        ratio = ::Div1000(ratio * 2500);
        int supplyFloor = (99 < supplyFactor) ? supplyFactor : 100;
        ratio = ::Div1000(ratio * supplyFloor);

        int minCav = NDefines::NAI::MIN_CAV_PERCENTAGE.GetUnderlyingValue() / 100;
        if (capMod[0] < minCav)
            minCav = capMod[0];
        int result = maxCav;
        if (ratio < maxCav)
            result = ratio;
        if (ratio < minCav)
            result = minCav;
        return CFixedPoint(result);
    }
    return CFixedPoint(maxCav);
}

void CAIRecruitMinister::GetDesiredLandRegimentTypeFractions(
    const CCountry &Country, CFixedPoint nForceLimit, CFixedPoint vTotalArmyBudget,
    CFixedPoint &vInfantry, CFixedPoint &vCavalry, CFixedPoint &vArtillery)
{
    const int cavRatio = CAIRecruitMinister::GetDesiredCavToInfRatio(Country).GetUnderlyingValue();

    vCavalry.SetUnderlyingValue(cavRatio - 20);
    vInfantry.SetUnderlyingValue(1020 - cavRatio);
    vArtillery.SetUnderlyingValue(0);

    const CSubUnitDefinition *artPreferred = Country.GetSubunitTypePreferred(SubUnitType::ARTILLERY);
    if (artPreferred->IsValid())
    {
        int artShare = NDefines::NAI::ARTILLERY_FRACTION.GetUnderlyingValue();
        int milLevel = Country.GetTechnologyStatus()->GetMilitaryLevel();
        if (NDefines::NAI::FORCE_COMPOSITION_CHANGE_TECH_LEVEL <= milLevel)
            artShare *= 2;
        artShare = (artShare < 0) ? 0 : artShare;
        artShare = (artShare > 1000) ? 1000 : artShare;
        vArtillery.SetUnderlyingValue(artShare);
        vInfantry.SetUnderlyingValue(::Div1000((1000 - artShare) * vInfantry.GetUnderlyingValue()));
        vCavalry.SetUnderlyingValue(::Div1000(vCavalry.GetUnderlyingValue() * (1000 - vArtillery.GetUnderlyingValue())));
    }

    const CSubUnitDefinition *cavPreferred = Country.GetSubunitTypePreferred(SubUnitType::CAVALRY);
    if (!cavPreferred->IsValid())
    {
        vInfantry.SetUnderlyingValue(vInfantry.GetUnderlyingValue() + vCavalry.GetUnderlyingValue());
        vCavalry.SetUnderlyingValue(0);
    }

    const CSubUnitDefinition *infPreferred = Country.GetSubunitTypePreferred(SubUnitType::INFANTRY);
    int infMaint;
    int cavMaint;
    int artMaint;
    const auto maintenanceCost3 = [&]() -> void
    {
        CFixedPoint size(1000);
      infMaint = Country.CalcActualLandMaintenanceCost(infPreferred, ESpecialUnitCategory::NORMAL, size, nullptr, nullptr).GetUnderlyingValue();
    };
    maintenanceCost3();
    const auto maintenanceCost4 = [&]() -> void
    {
        CFixedPoint size(1000);
      cavMaint = Country.CalcActualLandMaintenanceCost(cavPreferred, ESpecialUnitCategory::NORMAL, size, nullptr, nullptr).GetUnderlyingValue();
    };
    maintenanceCost4();
    const auto maintenanceCost5 = [&]() -> void
    {
        CFixedPoint size(1000);
      artMaint = Country.CalcActualLandMaintenanceCost(artPreferred, ESpecialUnitCategory::NORMAL, size, nullptr, nullptr).GetUnderlyingValue();
    };
    maintenanceCost5();

    int64_t weightedCost = ::Div1000(infMaint * vInfantry.GetUnderlyingValue()) + ::Div1000(cavMaint * vCavalry.GetUnderlyingValue()) + ::Div1000(artMaint * vArtillery.GetUnderlyingValue());
    int64_t totalCost = ::Div1000(weightedCost * nForceLimit.GetUnderlyingValue());
    int64_t costPlus10 = totalCost + 10;
    int available = -1;
    if (costPlus10 != 0)
        available = ::Div1000Checked(vTotalArmyBudget.GetUnderlyingValue() * 1000, static_cast<int32_t>(costPlus10));
    if (available >= 1000)
        return;

    int unaffordable = 1000 - available;
    int totalMaint = infMaint + cavMaint + artMaint;
    int infUnaff = -1, cavUnaff = -1, artUnaff = -1;
    if (totalMaint != 0)
    {
        infUnaff = ::Div1000Checked(::Div1000(infMaint * unaffordable), totalMaint);
        cavUnaff = ::Div1000Checked(::Div1000(cavMaint * unaffordable), totalMaint);
        artUnaff = ::Div1000Checked(::Div1000(artMaint * unaffordable), totalMaint);
    }
    int cavScaled = ::Div1000((1000 - cavUnaff) * vCavalry.GetUnderlyingValue());
    int infScaled = ::Div1000((1000 - infUnaff) * vInfantry.GetUnderlyingValue());
    int artScaled = ::Div1000((1000 - artUnaff) * vArtillery.GetUnderlyingValue());
    int total = cavScaled + infScaled + artScaled;
    if (total == 0)
    {
        vInfantry.SetUnderlyingValue(-1);
        vCavalry.SetUnderlyingValue(-1);
        vArtillery.SetUnderlyingValue(-1);
        return;
    }
    vInfantry.SetUnderlyingValue(::Div1000Checked(infScaled, total));
    vCavalry.SetUnderlyingValue(::Div1000Checked(cavScaled, total));
    vArtillery.SetUnderlyingValue(::Div1000Checked(artScaled, total));
}

CArray<ESpecialUnitCategory> CAIRecruitMinister::GetPreferredLandUnitCategories(
    const CCountry &Country, const SCategoryScoreMultiplier &ScoreMultipliers)
{
    CArray<ESpecialUnitCategory> out;

    bool atWar;
    int rebelGate;
    if (!Country.IsAtWar() &&
        Country.GetRebelThreat(false) < 1 &&
        !Country.IsPreparingForWar())
    {
        atWar = false;
        rebelGate = 0;
    }
    else
    {
        int warThreat = Country.GetRelativeWarThreat(false, false).GetUnderlyingValue();
        atWar = true;
        rebelGate = (500 < warThreat)
                        ? 1
                        : (799 < Country.GetRelativeRebelThreat().GetUnderlyingValue());
    }

    CEnumIndexedArray<CFixedPoint, ESpecialUnitCategory, 23> scores{};
    scores[ESpecialUnitCategory::NORMAL].SetUnderlyingValue(1000);
    scores[ESpecialUnitCategory::MERCENARY].SetUnderlyingValue(500);
    scores[ESpecialUnitCategory::STRELTSY].SetUnderlyingValue(1000);
    for (int i = static_cast<int>(ESpecialUnitCategory::RAJPUT); i <= static_cast<int>(ESpecialUnitCategory::MUSKETEER); ++i)
        scores[static_cast<ESpecialUnitCategory>(i)].SetUnderlyingValue(1000);
    scores[ESpecialUnitCategory::SAMURAI].SetUnderlyingValue(1000);
    scores[ESpecialUnitCategory::QIZILBASH].SetUnderlyingValue(1000);
    scores[ESpecialUnitCategory::MAMLUKS].SetUnderlyingValue(1000);

    if (const CGovernmentInstance *gov = Country.GetGovernmentInstance())
        gov->BuildAISpecialUnitPreferences(scores);
    scores[ESpecialUnitCategory::STRELTSY].SetUnderlyingValue(scores[ESpecialUnitCategory::STRELTSY].GetUnderlyingValue() * 3);

    const int cavRatio = CAIRecruitMinister::GetDesiredCavToInfRatio(Country).GetUnderlyingValue();
    int cavCount = Country.GetUnits().GetCount(SubUnitType::CAVALRY);
    int landLimit = Country.GetLandForceLimit(nullptr).GetUnderlyingValue();
    int limitRatio = (landLimit == 0)
                         ? -1
                         : ::Div1000Checked(cavCount * 1000 + 1000, landLimit);
    if (limitRatio <= cavRatio)
    {
        scores[ESpecialUnitCategory::COSSACKS].SetUnderlyingValue(scores[ESpecialUnitCategory::COSSACKS].GetUnderlyingValue() + 5000);
        scores[ESpecialUnitCategory::BANNER].SetUnderlyingValue(scores[ESpecialUnitCategory::BANNER].GetUnderlyingValue() + 5000);
    }
    int manpowerRatio = (Country.GetMaxManpower().GetUnderlyingValue() + 10 == 0)
                            ? -1
                            : ::Div1000Checked(Country.GetManpower().GetUnderlyingValue() + 10,
                                               Country.GetMaxManpower().GetUnderlyingValue() + 10);

    int mercMult = atWar ? (rebelGate ? 1000 : 600) : 250;

    for (int i = 0; i < 23; ++i)
    {
        ESpecialUnitCategory cat = static_cast<ESpecialUnitCategory>(i);
        scores[cat].SetUnderlyingValue(::Div1000(static_cast<int64_t>(scores[cat].GetUnderlyingValue()) * ScoreMultipliers[cat].GetUnderlyingValue()));
        if (CSubUnit::UseMercenaryReinforceCost(cat))
        {
            scores[cat].SetUnderlyingValue(::Div1000(static_cast<int64_t>(scores[cat].GetUnderlyingValue()) * mercMult));
        }
        if (rebelGate)
        {
            const CFixedPoint usage = CSubUnit::GetManpowerOrSailorsUsagePercentage(Country, cat);
            int scale = (0 < usage.GetUnderlyingValue())
                            ? manpowerRatio + 500
                            : 1500 - manpowerRatio;
            scores[cat].SetUnderlyingValue(::Div1000(static_cast<int64_t>(scores[cat].GetUnderlyingValue()) * scale));
        }
    }

    std::vector<ESpecialUnitCategory> ordered;
    for (int i = 0; i < 23; ++i)
    {
        const auto cat = static_cast<ESpecialUnitCategory>(i);
        if (scores[cat].GetUnderlyingValue() > 0)
            ordered.push_back(cat);
    }
    std::stable_sort(ordered.begin(), ordered.end(), [&scores](ESpecialUnitCategory a, ESpecialUnitCategory b) -> bool
                     { return scores[a].GetUnderlyingValue() > scores[b].GetUnderlyingValue(); });
    for (const auto cat : ordered)
        out.Append(cat);
    if (out.GetSize() == 0)
        out.Append(ESpecialUnitCategory::NORMAL);
    return out;
}

void CAIRecruitMinister::Finish()
{
    CAIAgent::Finish();
}

void CAIRecruitMinister::NotifyTick(CCountryFlagCache<char> &AccessCache)
{
    (void)AccessCache;
    this->Tick(this->GetRandom() % 3 + 1);

    CSpendingList armySpending;
    CSpendingList fleetSpending;

    if (this->IsActive())
    {
        const CCountry *country = &this->GetCountry();
        if (country->GetStrategy().IsInitialized() || country->GetStrategy().IsFixed())
        {
            CCountryTag tag = this->GetCountryTag();

            if (country->AllowedToBuildShips())
            {
                int tick = this->_nCount % 2;
                if (tick == 1)
                {
                    bool flag = false;
                    auto needs = this->MaintainTroopLevels(armySpending, flag);
                    CCountry *c = CCountryDataBase::AccessInstance()->GetCountry(
                        tag);
                    if (c->GetStrategy().GetNeedsRegiments() != needs)
                    {
                        auto *cmd = new CSetNeedsRegimentsCommand(
                            tag, needs, flag);
                        ::PostAICommand(cmd);
                    }
                }
                else if (tick == 0)
                {
                    bool flag = false;
                    auto needs = this->MaintainFleet(fleetSpending, flag);
                    CCountry *c = CCountryDataBase::AccessInstance()->GetCountry(
                        tag);
                    if (c->GetStrategy().GetNeedsShips() != needs)
                    {
                        auto *cmd = new CSetNeedsShipsCommand(
                            tag, needs, false);
                        ::PostAICommand(cmd);
                    }
                }
                this->_nCount++;
            }
            else
            {
                bool flag = false;
                auto needs = this->MaintainTroopLevels(armySpending, flag);
                CCountry *c = CCountryDataBase::AccessInstance()->GetCountry(
                    tag);
                if (c->GetStrategy().GetNeedsRegiments() != needs)
                {
                    auto *cmd = new CSetNeedsRegimentsCommand(
                        tag, needs, flag);
                    ::PostAICommand(cmd);
                }
                c = CCountryDataBase::AccessInstance()->GetCountry(
                    tag);
                if (static_cast<int>(c->GetStrategy().GetNeedsShips()) != -1)
                {
                    auto *cmd = new CSetNeedsShipsCommand(
                        tag, ESpecialUnitCategory::NO_REGIMENT, false);
                    ::PostAICommand(cmd);
                }
            }

            const auto disband = [&]() -> void
            {
                if (this->WouldDisbandLand(0, 0, false) && this->DisbandMercenaryUnit())
                    return;
                if (this->WouldDisbandLand(0, 0, false))
                    this->DisbandLandUnit();
            };
            disband();
        }
    }

    CCountry *c = CCountryDataBase::AccessInstance()->GetCountry(
        this->GetCountryTag());
    const CAIKing *king = &this->GetKing();
    const CFocalPoint *fp = king->GetAllianceFocalPoint();
    int armyMoney;
    if (fp != nullptr && static_cast<int>(fp->GetState()) != 0)
    {
        armyMoney = c->GetTreasury().GetUnderlyingValue();
    }
    else
    {
        armyMoney = c->GetStrategy().GetArmyMoney().GetUnderlyingValue();
    }
    armySpending.DoSpending(CFixedPoint(armyMoney));
    int navyMoney = c->GetStrategy().GetNavyMoney().GetUnderlyingValue();
    fleetSpending.DoSpending(CFixedPoint(navyMoney));
    this->RecruitLeaders();
}

ESpecialUnitCategory CAIRecruitMinister::MaintainTroopLevels(CSpendingList &Proxy, bool &bNeedsMoney)
{
    bNeedsMoney = false;
    const CCountry &Country = this->GetCountry();
    const CCountryTag MyTag = this->GetCountryTag();
    const CCurrentGameState *pGameState = CCurrentGameState::GetInstance();

    CEnumIndexedArray<int, SubUnitType, 7> nCurrent{};
    for (int nSubUnitIndex = static_cast<int>(SubUnitType::FIRST_LAND);
         nSubUnitIndex < static_cast<int>(SubUnitType::MAX); ++nSubUnitIndex)
    {
        const SubUnitType Type = static_cast<SubUnitType>(nSubUnitIndex);
        nCurrent[Type] = Country.GetUnits().GetCount(Type);
    }
    const int nCurrentAmount = Country.CalcCurrentAmountOfRegiments();

    const CFocalPoint *pFocalPoint = this->GetKing().GetAllianceFocalPoint();
    const CProvince *pAllianceGathering = nullptr;
    if (pFocalPoint != nullptr && pFocalPoint->GetState() != EFocalPointState::Inactive)
        pAllianceGathering = pFocalPoint->GetGatheringSpot();

    CList<CCountryTag> EnemyCountries = Country.GetCurrentAtWarWith();
    const CCountryTag WarTarget = Country.GetStrategy().GetWarTarget();
    if (WarTarget.IsValid())
        EnemyCountries.Append(WarTarget);

    int nMinRegimentsNeededForSiege = 1;
    const CProvince *pSiegeProv = nullptr;
    int64_t nEnemyArmyPower = 0;
    bool bHumanEnemy = false;
    for (const CCountryTag EnemyTag : EnemyCountries)
    {
        const CCountry &Enemy = *CCountryDataBase::AccessInstance()->GetCountry(EnemyTag);
        if (!Enemy.GetDiplomacy()[MyTag.GetIndex()]->HasWar() &&
            !CCountryTag::SameUpperWord(EnemyTag, Country.GetStrategy().GetWarTarget()))
        {
            continue;
        }
        nEnemyArmyPower += ::RawFPML(Enemy.GetCachedData().GetArmyPower());
        bHumanEnemy = bHumanEnemy ||
                      pGameState->GetControllerType(Enemy.GetCountryTag()) != _CONTROLLER_AI_;
        for (const int nProvince : Enemy.GetOwnedProvinces())
        {
            const CProvince &Prov = pGameState->GetProvince(nProvince);
            if (!CCountryTag::SameUpperWord(Prov.GetController(), EnemyTag))
                continue;
            const int nNeededForThis = Prov.GetRegimentsNeededForSiege();
            if (nMinRegimentsNeededForSiege == 1 ||
                (nNeededForThis > 1 && nNeededForThis < nMinRegimentsNeededForSiege))
            {
                nMinRegimentsNeededForSiege = nNeededForThis;
                pSiegeProv = &Prov;
            }
        }
    }

    bool exists = false;
    std::vector<const CProvince *> &PotentialProvinces =
        *static_cast<std::vector<const CProvince *> *>(::table_lookup(::PotentialProvincesTL, exists));
    PotentialProvinces.clear();
    for (const int nProvince : Country.GetControlledProvinces())
    {
        const CProvince *pProv = &pGameState->GetProvince(nProvince);
        if (pProv->GetSiege() == nullptr)
            PotentialProvinces.push_back(pProv);
    }
    const CArray<CCountryTag> &Subjects = Country.GetSubjects();
    for (int nSubject = 0; nSubject < Subjects.GetSize(); ++nSubject)
    {
        const CCountryTag SubjectTag = Subjects[nSubject];
        const CCountry &Subject = *CCountryDataBase::AccessInstance()->GetCountry(SubjectTag);
        for (const int nProvince : Subject.GetControlledProvinces())
        {
            const CProvince *pProv = &pGameState->GetProvince(nProvince);
            if (pProv->GetSiege() == nullptr)
                PotentialProvinces.push_back(pProv);
        }
    }

    std::vector<int> CurrentlyRecruitingIn;
    int nInf = 0;
    int nCav = 0;
    int nArt = 0;
    for (const CProvince *pProv : PotentialProvinces)
    {
        if (pProv->GetSiege() != nullptr || !pProv->HasConstruction())
            continue;
        for (const CConstruction *pC : pProv->GetConstructions())
        {
            const EConstructionType ConstructionType = pC->GetConstructionType();
            if (ConstructionType != ARMY && ConstructionType != NAVY)
                continue;
            if (!CCountryTag::SameUpperWord(pC->GetCountry(), MyTag))
                continue;
            const CSubUnitDefinition *pSub = static_cast<const CMilitaryConstruction *>(pC)->GetSubUnit();
            if (!pSub->IsRegiment())
                continue;
            CurrentlyRecruitingIn.push_back(pProv->GetProvinceID());
            switch (pSub->GetType())
            {
            case SubUnitType::INFANTRY:
                ++nInf;
                ++nCurrent[SubUnitType::INFANTRY];
                break;
            case SubUnitType::CAVALRY:
                ++nCav;
                ++nCurrent[SubUnitType::CAVALRY];
                break;
            case SubUnitType::ARTILLERY:
                ++nArt;
                ++nCurrent[SubUnitType::ARTILLERY];
                break;
            default:
                break;
            }
        }
    }
    int nTotalBuilt = nInf + nCav + nArt;

    const int nManpowerDivisor = Country.GetMaxManpower().GetUnderlyingValue() + kTroopManpowerBias;
    const int nManpowerRatio = nManpowerDivisor == 0
        ? -1
        : ::Div1000Checked(Country.GetManpower().GetUnderlyingValue() + kTroopManpowerBias,
                           nManpowerDivisor);
    const int nForceLimit = Country.GetLandForceLimit(nullptr).GetUnderlyingValue() / kFixedPointResolution;
    const CFixedPoint vRegularInfantryMaintenance = Country.CalcActualLandMaintenanceCost(
        Country.GetSubunitTypePreferred(SubUnitType::INFANTRY), ESpecialUnitCategory::NORMAL,
        CFixedPoint(kFixedPointResolution), nullptr, nullptr);
    const SLandMaintenance LandMaintenance = Country.GetLandMaintenance(
        CFixedPoint(kFixedPointResolution), CCountry::ELandMaintenanceMode::IgnoreReinforcementsForPerformance,
        CCountry::ELandMaintenanceCacheMode::UseCachedCore);
    const int nCurrentMaintenance =
        LandMaintenance._BaseCost._ArmyCost.GetUnderlyingValue() +
        LandMaintenance._BaseCost._OldMercsCost.GetUnderlyingValue() +
        LandMaintenance._BaseCost._MercenaryCompaniesCost.GetUnderlyingValue() +
        LandMaintenance._BaseCost._CondottieriCost.GetUnderlyingValue() +
        LandMaintenance._BaseCost._DrillCost.GetUnderlyingValue() +
        LandMaintenance._ReinforcementCost._ArmyCost.GetUnderlyingValue() +
        LandMaintenance._ReinforcementCost._OldMercsCost.GetUnderlyingValue() +
        LandMaintenance._ReinforcementCost._MercenaryCompaniesCost.GetUnderlyingValue() +
        LandMaintenance._ReinforcementCost._CondottieriCost.GetUnderlyingValue() +
        LandMaintenance._ReinforcementCost._DrillCost.GetUnderlyingValue() +
        LandMaintenance._ExtraCost._ArmyCost.GetUnderlyingValue() +
        LandMaintenance._ExtraCost._OldMercsCost.GetUnderlyingValue() +
        LandMaintenance._ExtraCost._MercenaryCompaniesCost.GetUnderlyingValue() +
        LandMaintenance._ExtraCost._CondottieriCost.GetUnderlyingValue() +
        LandMaintenance._ExtraCost._DrillCost.GetUnderlyingValue() +
        vRegularInfantryMaintenance.GetUnderlyingValue();
    const int nForceLimitPercent = nForceLimit != 0
        ? ::Div1000Checked(nTotalBuilt * kFixedPointResolution, nForceLimit * kFixedPointResolution) : -1;

    int nMax = nMinRegimentsNeededForSiege;
    if (nForceLimit < nMinRegimentsNeededForSiege)
    {
        const bool bImportantWar = Country.IsInImportantWar(true, true, nullptr);
        if (nMinRegimentsNeededForSiege == 1 || !bImportantWar)
            nMax = 0;
    }
    else if (nMinRegimentsNeededForSiege == 1)
    {
        nMax = 0;
    }

    const int nTotalWithBuilt = nCurrentAmount + nTotalBuilt;
    const int nSiegeShortfall = nMax - nTotalWithBuilt;
    const CProvince *pGatherProv = nSiegeShortfall > 0 ? pSiegeProv : pAllianceGathering;
    if (pAllianceGathering != nullptr)
        pGatherProv = pAllianceGathering;
    int nExtraNeededForSiegeOrMissions = std::max(nSiegeShortfall, 0);

    const CFixedPoint vMercenaryMaintenance = Country.CalcActualLandMaintenanceCost(
        Country.GetSubunitTypePreferred(SubUnitType::INFANTRY), ESpecialUnitCategory::MERCENARY,
        CFixedPoint(kFixedPointResolution), nullptr, nullptr);
    int nMaxDaily = Country.GetStrategy().GetBudget(AIBudgetType::ARMIES).GetUnderlyingValue();
    if (nSiegeShortfall > 0)
    {
        nMaxDaily = std::max(nMaxDaily, nCurrentMaintenance + ::Div1000(
            static_cast<int64_t>(nExtraNeededForSiegeOrMissions) * kFixedPointResolution *
            vMercenaryMaintenance.GetUnderlyingValue()));
    }
    for (const CAIStrategy::SArmyOrNavySizeObjective &Objective : Country.GetStrategy().GetArmySizeObjectives())
    {
        if (Objective._vImportance.GetUnderlyingValue() <= 0)
            continue;
        const int nObjectiveDeficit = Objective._nSize - nTotalWithBuilt;
        const int nAdditionalNeed = nObjectiveDeficit - nExtraNeededForSiegeOrMissions;
        if (nAdditionalNeed == 0 || nExtraNeededForSiegeOrMissions > nObjectiveDeficit)
            continue;
        const int nAdditionalCost = ::Div1000(static_cast<int64_t>(nAdditionalNeed) * kFixedPointResolution *
                                              vMercenaryMaintenance.GetUnderlyingValue());
        const int nImportanceCost = ::Div1000(
            static_cast<int64_t>(Objective._vImportance.GetUnderlyingValue()) * nMaxDaily) / 100;
        if (nAdditionalCost <= nImportanceCost)
        {
            nExtraNeededForSiegeOrMissions = nObjectiveDeficit;
            nMaxDaily = std::max(nMaxDaily, nCurrentMaintenance + ::Div1000(
                static_cast<int64_t>(nObjectiveDeficit) * kFixedPointResolution *
                vMercenaryMaintenance.GetUnderlyingValue()));
        }
    }

    int nTotalTarget = nForceLimit;
    const bool bForceLimitExceeded =
        nExtraNeededForSiegeOrMissions > 0 &&
        nTotalTarget < nExtraNeededForSiegeOrMissions + nTotalWithBuilt;
    if (bForceLimitExceeded)
        nTotalTarget = nExtraNeededForSiegeOrMissions + nTotalWithBuilt;
    const int nRegularShortfall = nTotalTarget - nTotalWithBuilt;
    if (nRegularShortfall <= 0)
    {
        if (nTotalBuilt > 5 || nForceLimitPercent > 100)
            return ESpecialUnitCategory::NO_REGIMENT;
        const int64_t nPowerScale = bHumanEnemy ? kTroopHumanPowerScale : kTroopAIPowerScale;
        const int64_t nOwnArmyPower = ::RawFPML(Country.GetCachedData().GetArmyPower());
        if ((nEnemyArmyPower * nPowerScale >> 15) < nOwnArmyPower)
            return ESpecialUnitCategory::NO_REGIMENT;
    }

    const int nRegularMargin = nMaxDaily - nCurrentMaintenance;
    if (pGatherProv == nullptr && nExtraNeededForSiegeOrMissions == 0 &&
        nRegularMargin < vRegularInfantryMaintenance.GetUnderlyingValue())
    {
        return ESpecialUnitCategory::NO_REGIMENT;
    }

    SCategoryScoreMultiplier CategoryMultipliers{};
    for (int i = 0; i < static_cast<int>(ESpecialUnitCategory::MAX); ++i)
        CategoryMultipliers[static_cast<ESpecialUnitCategory>(i)].SetUnderlyingValue(kFixedPointResolution);
    CategoryMultipliers[ESpecialUnitCategory::BANNER].SetUnderlyingValue(0);
    CategoryMultipliers[ESpecialUnitCategory::COSSACKS].SetUnderlyingValue(0);
    CategoryMultipliers[ESpecialUnitCategory::RAJPUT].SetUnderlyingValue(0);

    if (!Country.IsInImportantWar(false, false, nullptr))
    {
        if (nMaxDaily <= nCurrentMaintenance)
            return ESpecialUnitCategory::NO_REGIMENT;
    }
    else if (::Div1000(static_cast<int64_t>(nMaxDaily) * 1250) <= nCurrentMaintenance)
    {
        const CFixedPoint vBuildCost = Country.GetBuildCost(
            ESpecialUnitCategory::NORMAL, SubUnitType::INFANTRY, nullptr, 0, nullptr);
        bNeedsMoney = Country.GetTreasury().GetUnderlyingValue() < vBuildCost.GetUnderlyingValue();
        if (nTotalTarget <= nTotalWithBuilt || Country.GetCurrentNumberOfBankLoans() != 0 ||
            Country.GetTreasury().GetUnderlyingValue() - nMaxDaily * 24 < 10001)
        {
            return ESpecialUnitCategory::NO_REGIMENT;
        }
        CArray<ESpecialUnitCategory> PreferredUnitCategories =
            CAIRecruitMinister::GetPreferredLandUnitCategories(Country, CategoryMultipliers);
        return PreferredUnitCategories[0];
    }

    const bool bNoGathering = pGatherProv == nullptr;
    const bool bNoExtraNeeded = nExtraNeededForSiegeOrMissions < 1;
    const int nDivision = (bNoGathering && bNoExtraNeeded) ? 5 : 2;
    int nMaxDailyRegiments = std::max(nTotalTarget / nDivision, 1);
    nExtraNeededForSiegeOrMissions = std::max(nExtraNeededForSiegeOrMissions, nRegularShortfall);
    nExtraNeededForSiegeOrMissions = std::min(nExtraNeededForSiegeOrMissions, nMaxDailyRegiments);

    const bool bIsWar = Country.IsAtWar() || Country.GetRebelThreat(true) > 0;
    bool bIsMajorWar = false;
    if (bIsWar)
    {
        bIsMajorWar = Country.GetRelativeWarThreat(false, false).GetUnderlyingValue() >= 500;
        if (!bIsMajorWar)
            bIsMajorWar = Country.GetRelativeRebelThreat().GetUnderlyingValue() > 799;
    }
    const int nCurrentTotalLandRegiments = nCurrent[SubUnitType::INFANTRY] +
        nCurrent[SubUnitType::CAVALRY] + nCurrent[SubUnitType::ARTILLERY];
    const bool bOnlyRecruitInfantry = NDefines::NAI::ONLY_INFANTRY_MERCS > 0 ||
        pGatherProv != nullptr || bForceLimitExceeded;
    if (bOnlyRecruitInfantry)
    {
        if (nManpowerRatio > 150 && Country.GetManpower().GetUnderlyingValue() >= 2000 &&
            nCurrentTotalLandRegiments > 0 && nCurrent[SubUnitType::ARTILLERY] > 0 &&
            nCurrent[SubUnitType::INFANTRY] * kFixedPointResolution >
                ::Div1000(static_cast<int64_t>(nCurrentTotalLandRegiments) * 600000))
        {
            CategoryMultipliers[ESpecialUnitCategory::MERCENARY].SetUnderlyingValue(
                CategoryMultipliers[ESpecialUnitCategory::MERCENARY].GetUnderlyingValue() / 10);
        }
        if (bIsWar && nManpowerRatio < 800)
        {
            const CFixedPoint vNeeded = Country.CalcManpowerReinforcementsNeeded(true);
            if (Country.GetManpower().GetUnderlyingValue() < vNeeded.GetUnderlyingValue())
            {
                bool bAdjustNormal = nCurrent[SubUnitType::INFANTRY] * kFixedPointResolution <
                    ::Div1000(static_cast<int64_t>(nCurrent[SubUnitType::ARTILLERY]) * 1150000);
                if (!bAdjustNormal)
                {
                    const CFixedPoint vCavRatio = CAIRecruitMinister::GetDesiredCavToInfRatio(Country);
                    const int nCavTarget = ::Div1000(
                        ::Div1000(static_cast<int64_t>(vCavRatio.GetUnderlyingValue()) * 1100) *
                        static_cast<int64_t>(nCurrent[SubUnitType::INFANTRY]) * kFixedPointResolution);
                    bAdjustNormal = nCurrent[SubUnitType::CAVALRY] * kFixedPointResolution > nCavTarget;
                }
                if (bAdjustNormal)
                {
                    CategoryMultipliers[ESpecialUnitCategory::MERCENARY].SetUnderlyingValue(
                        CategoryMultipliers[ESpecialUnitCategory::MERCENARY].GetUnderlyingValue() * kFixedPointResolution /
                        (nManpowerRatio + 200));
                }
            }
        }
    }
    if (nTotalTarget < nCurrent[SubUnitType::INFANTRY] * 2)
    {
        const int nMercenaryScale = ::Div1000Checked(
            nTotalTarget * kFixedPointResolution, (nCurrent[SubUnitType::INFANTRY] * 2 | 1) * kFixedPointResolution);
        CategoryMultipliers[ESpecialUnitCategory::STRELTSY].SetUnderlyingValue(::Div1000(
            static_cast<int64_t>(nMercenaryScale) *
            CategoryMultipliers[ESpecialUnitCategory::STRELTSY].GetUnderlyingValue()));
    }
    if (!bNoGathering || !bNoExtraNeeded)
    {
        CategoryMultipliers[ESpecialUnitCategory::MERCENARY].SetUnderlyingValue(
            CategoryMultipliers[ESpecialUnitCategory::MERCENARY].GetUnderlyingValue() * 2);
    }

    CArray<ESpecialUnitCategory> PreferredUnitCategories =
        CAIRecruitMinister::GetPreferredLandUnitCategories(Country, CategoryMultipliers);
    const ESpecialUnitCategory PreferredUnitCategory = PreferredUnitCategories[0];
    CFixedPoint vArmyMoney = (bNoGathering && bNoExtraNeeded)
        ? Country.GetStrategy().GetArmyMoney() : Country.GetTreasury();
    CFixedPoint vMargin(nRegularMargin);
    const SDesiredTypeResult WantInfantryResult = ::GetNextDesiredArmyType(
        Country, PreferredUnitCategory, nullptr, pSiegeProv, nCurrent,
        vMargin, vArmyMoney, this->GetRandom());
    (void)WantInfantryResult;                       

    std::vector<int> Checked;
    std::vector<signed char> InWarZoneCache(PotentialProvinces.size(), -1);
    bool bRestrictToCores = PreferredUnitCategory != ESpecialUnitCategory::MERCENARY;
    const float fNeighbourFactor = bIsMajorWar ? kTroopHighThreatFactor : kTroopLowThreatFactor;
    int nMercs = 0;
    nTotalBuilt = 0;
    bRestrictToCores = bRestrictToCores || bNoGathering;
    while (nExtraNeededForSiegeOrMissions > 0)
    {
        CStrengthFactorCache &StrengthCache = CStrengthFactorCacheManager::AccessCache();
        CEU3Date MaxDate = pGameState->GetCurrentDate();
        MaxDate.SetUniversalDate(MaxDate.GetUniversalDate() + kTroopThreatHorizon);
        const CProvince *pBest = nullptr;
        float fBestScore = 0.0f;

        for (size_t nProvinceIndex = 0; nProvinceIndex < PotentialProvinces.size(); ++nProvinceIndex)
        {
            const CProvince *pProv = PotentialProvinces[nProvinceIndex];
            if (pProv->GetCombat() != nullptr)
                continue;
            if (bRestrictToCores)
            {
                const CArray<CCountryTag> &Cores = pProv->GetCores();
                bool bOwnerHasCore = false;
                for (int nCore = 0; nCore < Cores.GetSize(); ++nCore)
                    bOwnerHasCore = bOwnerHasCore || Cores[nCore] == pProv->GetOwner();
                if (!bOwnerHasCore)
                    continue;
            }

            int nCount = 0;
            for (const CConstruction *pC : pProv->GetConstructions())
            {
                if (pC == nullptr)
                    continue;
                const EConstructionType Type = pC->GetConstructionType();
                if ((Type == ARMY || Type == NAVY) &&
                    CCountryTag::SameUpperWord(pC->GetCountry(), MyTag))
                {
                    ++nCount;
                }
            }

            const CCountry &Controller = *CCountryDataBase::AccessInstance()->GetCountry(pProv->GetOwner());
            bool bLibertyOk = false;
            if (Controller.GetOverlord().IsValid() && !Controller.GetSubjectType()->IsVoluntary() &&
                CCountryTag::SameUpperWord(Controller.GetOverlord(), MyTag))
            {
                bLibertyOk = Controller.GetLibertyDesire(nullptr, 0, false).GetUnderlyingValue() <=
                    NDefines::NEconomy::MAX_LD_FOR_CONSTRUCT_IN_SUBJECT.GetUnderlyingValue();
            }
            if (PreferredUnitCategory == ESpecialUnitCategory::NORMAL)
            {
                if (nCount > 5 ||
                    (!CCountryTag::SameUpperWord(pProv->GetOwner(), MyTag) && !bLibertyOk) ||
                    !pProv->HasBuildableUnits())
                {
                    continue;
                }
            }

            bool bHasMilitaryConstruction = false;
            bool bHasNavyConstruction = false;
            for (const CConstruction *pC : pProv->GetConstructions())
            {
                const EConstructionType Type = pC->GetConstructionType();
                if (Type == NAVY)
                {
                    bHasNavyConstruction = true;
                    break;
                }
                if (Type == ARMY)
                    bHasMilitaryConstruction = true;
            }
            if (bHasNavyConstruction)
                continue;

            const int nLocalCost = pProv->GetModifier().GetValue(_MODIFIER_LOCAL_REGIMENT_COST_).GetUnderlyingValue();
            const float fModifiedCost = nLocalCost / 1000.0f * kTroopCostScoreFactor;
            float fScore = 100.0f / (10.0f - std::min(kTroopCostScoreCap, fModifiedCost));
            if (InWarZoneCache[nProvinceIndex] == 1)
            {
                fScore *= 2.0f;
            }
            else if (InWarZoneCache[nProvinceIndex] == -1)
            {
                const CControllerArea *pArea = pProv->GetControllerArea();
                if (pArea != nullptr && pArea->CalcIsWarZone(pArea->GetOwner(), false))
                {
                    InWarZoneCache[nProvinceIndex] = 1;
                    fScore *= 2.0f;
                }
                else
                {
                    InWarZoneCache[nProvinceIndex] = 0;
                }
            }
            if (pProv->IsOverseas(MyTag, true))
                fScore *= kTroopOverseasFactor;

            if (Country.IsAtWar())
            {
                bool bHostileNeighbour = false;
                const CArray<SAdjacency> &Neighbours = pProv->GetTemplate().GetNeighbours();
                for (int i = Neighbours.GetSize() - 1; i >= 0; --i)
                {
                    const SAdjacency &Adjacency = Neighbours[i];
                    if (Adjacency.GetType() != _ADJACENCY_NORMAL_ && Adjacency.GetType() != _ADJACENCY_RIVER_)
                        continue;
                    if (bHostileNeighbour)
                        continue;
                    const CCountryTag NeighbourTag = Adjacency.GetProvince().GetController();
                    if (CCountryTag::SameUpperWord(NeighbourTag, MyTag))
                        continue;
                    const CCountry &Neighbour = *CCountryDataBase::AccessInstance()->GetCountry(NeighbourTag);
                    if (Neighbour.GetDiplomacy().HasWar(MyTag) || MyTag.IsRebelPirateNative() ||
                        NeighbourTag.IsRebelPirateNative())
                    {
                        bHostileNeighbour = true;
                    }
                }
                if (bHostileNeighbour)
                    fScore *= fNeighbourFactor;
            }

            const CFixedPoint vThreat = CAIThreatEvaluation::GetInstance().CalcArmyThreatForCountry(
                pProv->GetProvinceID(), MaxDate, MyTag, false, StrengthCache);
            const float fThreat = vThreat.GetUnderlyingValue() / 1000.0f;
            fScore /= fThreat * kTroopThreatFactor + 1.0f;
            if (fThreat <= 0.0f)
                fScore *= 2.0f;
            if (pGatherProv != nullptr)
            {
                const float fDistance = pProv->GetDistance(pGatherProv->GetProvinceID(), false).GetUnderlyingValue() / 1000.0f;
                fScore /= fDistance * kTroopDistanceFactor + 1.0f;
            }
            if (std::find(CurrentlyRecruitingIn.begin(), CurrentlyRecruitingIn.end(), pProv->GetProvinceID()) !=
                    CurrentlyRecruitingIn.end() ||
                (bNoGathering && bHasMilitaryConstruction))
            {
                fScore /= 10.0f;
            }
            if (nCount > 0)
                fScore /= static_cast<float>(nCount * nCount) + 2.0f;
            if (pProv->GetUnrest().GetUnderlyingValue() > 0)
                fScore /= 5.0f;

            const CCulture *pCulture = pProv->GetCulture();
            const CCulture *pPrimaryCulture = Country.GetPrimaryCulture();
            if (pPrimaryCulture == pCulture)
                fScore *= 2.0f;
            else if (!Country.IsAcceptedCultureByAllRules(pCulture))
                fScore *= 0.5f;
            if (std::find(Checked.begin(), Checked.end(), pProv->GetProvinceID()) != Checked.end())
                fScore *= 0.5f;

            const CControllerArea *pArea = pProv->GetControllerArea();
            if (pArea != nullptr)
            {
                const float fSubjectAdjusted = bLibertyOk ? fScore / kTroopSubjectFactor : fScore;
                const int nDivs = pArea->GetDivs();
                fScore = pArea->GetSize() /
                    (nDivs * kTroopAreaDivisionFactor + 1.0f) * fSubjectAdjusted;
                if (fScore > fBestScore)
                {
                    fBestScore = fScore;
                    pBest = pProv;
                }
            }
        }

        if (pBest == nullptr)
            break;
        Checked.push_back(pBest->GetProvinceID());
        const SDesiredTypeResult DesiredTypeResult = ::GetNextDesiredArmyType(
            Country, PreferredUnitCategory, pBest, pGatherProv, nCurrent,
            vMargin, vArmyMoney, this->GetRandom());
        if (DesiredTypeResult._bNeedsMoneyBeforeBuildingNext)
        {
            bNeedsMoney = true;
            return PreferredUnitCategory;
        }
        SubUnitType DesiredType = DesiredTypeResult._Type;
        if (bForceLimitExceeded)
            DesiredType = SubUnitType::INFANTRY;
        if (Country.GetManpower().GetUnderlyingValue() == 0 &&
            CSubUnit::GetManpowerOrSailorsUsagePercentage(Country, PreferredUnitCategory).GetUnderlyingValue() > 0)
        {
            return PreferredUnitCategory;
        }

        if (PreferredUnitCategory == ESpecialUnitCategory::MERCENARY)
        {
            CPdxArray<const CMercenaryCompany *, int> PotentialCompanies;
            this->CalculatePotentialMercenaryCompanies(pBest, PotentialCompanies);
            if (PotentialCompanies.GetSize() == 0)
                return PreferredUnitCategory;
            for (const CMercenaryCompany *pMercenaryCompany : PotentialCompanies)
            {
                const CFixedPoint vHiringCost = pMercenaryCompany->CalcHiringCostFor(Country, nullptr);
                const CMercenaryComposition Composition = pMercenaryCompany->GetComposition(Country);
                const CFixedPoint vMaintenance = pMercenaryCompany->CalcMaintenanceFor(Country, nullptr);
                if (vMargin.GetUnderlyingValue() <= vMaintenance.GetUnderlyingValue() ||
                    vArmyMoney.GetUnderlyingValue() <= vHiringCost.GetUnderlyingValue() ||
                    this->WouldDisbandLand(0, Composition._nTotal + nMercs, true))
                {
                    continue;
                }

                std::unique_ptr<CCommand> pCommand(
                    new CConstructUnitCommand(MyTag, nullptr, pBest->GetProvinceID(), pMercenaryCompany));
                if (!pCommand->IsValid())
                    continue;
                vArmyMoney.SetUnderlyingValue(vArmyMoney.GetUnderlyingValue() - vHiringCost.GetUnderlyingValue());
                vMargin.SetUnderlyingValue(vMargin.GetUnderlyingValue() - vMaintenance.GetUnderlyingValue());
                Proxy.AddItem(std::move(pCommand), vHiringCost, CFixedPoint(kTroopPriority));
                nMercs += Composition._nTotal;
                break;
            }
            --nExtraNeededForSiegeOrMissions;
            continue;
        }
        const CFixedPoint vBuildCost = Country.GetBuildCost(
            ESpecialUnitCategory::NORMAL, DesiredType, pBest, 0, nullptr);
        bNeedsMoney = Country.GetTreasury().GetUnderlyingValue() < vBuildCost.GetUnderlyingValue();
        if (vArmyMoney.GetUnderlyingValue() < vBuildCost.GetUnderlyingValue())
            return PreferredUnitCategory;
        const int nNextTotalBuilt = nTotalBuilt + 1;
        if (this->WouldDisbandLand(nNextTotalBuilt, 0, true))
            return PreferredUnitCategory;

        CList<const CSubUnitDefinition *> BuildableUnits = pBest->GetBuildableUnits();
        bool bCanBuild = Country.GetManpower().GetUnderlyingValue() >= kFixedPointResolution && BuildableUnits.GetSize() > 0;
        if (bCanBuild && !CCountryTag::SameUpperWord(pBest->GetOwner(), MyTag))
        {
            const CCountry &Controller = *CCountryDataBase::AccessInstance()->GetCountry(pBest->GetOwner());
            bCanBuild = CCountryTag::SameUpperWord(Controller.GetOverlord(), MyTag);
        }
        if (bCanBuild)
        {
            const CSubUnitDefinition *pMaybe = nullptr;
            for (const CSubUnitDefinition *pDefinition : BuildableUnits)
            {
                if (pDefinition->GetType() == DesiredType)
                {
                    pMaybe = pDefinition;
                    break;
                }
            }
            if (pMaybe != nullptr)
            {
                if (Country.GetManpower().GetUnderlyingValue() < pMaybe->GetDefaultMenCost().GetUnderlyingValue())
                {
                    bNeedsMoney = Country.GetTreasury().GetUnderlyingValue() < vBuildCost.GetUnderlyingValue();
                    return PreferredUnitCategory;
                }

                std::unique_ptr<CCommand> pCommand(
                    new CConstructUnitCommand(MyTag, pMaybe, pBest->GetProvinceID(), nullptr));
                if (!pCommand->IsValid())
                    return PreferredUnitCategory;

                vArmyMoney.SetUnderlyingValue(vArmyMoney.GetUnderlyingValue() - vBuildCost.GetUnderlyingValue());
                const CFixedPoint vUnitMaintenance = Country.CalcActualLandMaintenanceCost(
                    pMaybe, ESpecialUnitCategory::NORMAL, CFixedPoint(kFixedPointResolution), nullptr, nullptr);
                vMargin.SetUnderlyingValue(vMargin.GetUnderlyingValue() - vUnitMaintenance.GetUnderlyingValue());
                CurrentlyRecruitingIn.push_back(pBest->GetProvinceID());
                Proxy.AddItem(std::move(pCommand), vBuildCost, CFixedPoint(kTroopPriority));
                nTotalBuilt = nNextTotalBuilt;
            }
        }
        --nExtraNeededForSiegeOrMissions;
    }

    const CFixedPoint vBuildCost = Country.GetBuildCost(
        ESpecialUnitCategory::NORMAL, SubUnitType::INFANTRY, nullptr, 0, nullptr);
    bNeedsMoney = Country.GetTreasury().GetUnderlyingValue() < vBuildCost.GetUnderlyingValue();
    return PreferredUnitCategory;
}

bool CAIRecruitMinister::WouldDisbandLand(int nAdditionalRegulars, int nAdditionalMercs, bool bOnlyCheckedType) const
{
    const CCountry *country = &this->GetCountry();
    if (country->GetStrategy().IsBudgetInitialized() == 0)
        return false;

    bool rebelThreatActive = true;
    if (!country->GetStrategy().IsPreparingWar() && !country->IsAtWar())
    {
        CFixedPoint rebelThreat = country->GetRelativeRebelThreat();
        rebelThreatActive = rebelThreat.GetUnderlyingValue() > 0;
    }

    CFixedPoint forceLimit = country->GetLandForceLimit(nullptr);
    CFixedPoint totalMen = country->GetTotalMen();

    int currentRegiments = totalMen.GetUnderlyingValue() / 1000;
    int projectedMen = nAdditionalRegulars + nAdditionalMercs + currentRegiments;

    int forceLimitPercent;
    const auto forceLimitRatio6 = [&]() -> void
    {
        int64_t limit = static_cast<int64_t>(forceLimit.GetUnderlyingValue());
        if (limit == 0)
            forceLimitPercent = -1;
        else if (::IsValidRange(projectedMen * 1000))
            forceLimitPercent = projectedMen * 1000000 / static_cast<int>(limit);
        else
            forceLimitPercent = static_cast<int>(static_cast<int64_t>(projectedMen) * 1000 * 1000 / limit);
    };
    forceLimitRatio6();

    SLandDisbandEval eval{bOnlyCheckedType, currentRegiments, nAdditionalRegulars, nAdditionalMercs,
                          CFixedPoint(forceLimitPercent), *country};

    bool siegeNeedsCovered = false;
    if (country->IsInImportantWar(false, true, nullptr))
    {
        if (!country->IsInImportantWar(false, false, nullptr))
        {
            int maxSiege = 0;
            for (CCountryTag enemy : country->GetCurrentAtWarWith())
            {
                CCountry *subject = CCountryDataBase::AccessInstance()->GetCountry(
                    enemy);
                for (int provinceId : subject->GetControlledProvinces())
                {
                    CProvince *prov = &CCurrentGameState::GetInstance()->GetProvince(
                        provinceId);
                    maxSiege = std::max(maxSiege, prov->GetRegimentsNeededForSiege());
                }
            }
            siegeNeedsCovered = projectedMen <= maxSiege;
        }
    }

    int loanRatio;
    const auto loanFraction7 = [&]() -> void
    {
        int current = country->GetCurrentNumberOfBankLoans();
        int maximum = country->GetMaximumBankLoans();
        if (maximum == 0)
            loanRatio = -1;
        else if (::IsValidRange(current * 1000))
            loanRatio = current * 1000000 / (maximum * 1000);
        else
            loanRatio = static_cast<int>(
                static_cast<int64_t>(current) * 1000 * 1000 / (maximum * 1000));
    };
    loanFraction7();

    if (rebelThreatActive)
    {
        if (loanRatio < 0xfb)
            return false;
        if (siegeNeedsCovered && loanRatio < 0x321)
            return false;
    }

    const CAIKing *king = &this->GetKing();
    const CFocalPoint *focal = king->GetAllianceFocalPoint();
    if (focal != nullptr)
    {
        if (static_cast<int>(focal->GetState()) != 0 || focal->GetGatheringSpot() != nullptr)
            return false;
    }
    if (eval.WillReallySoonFightRebels(6, true))
        return false;

    const auto meetsSizeObjective8 = [&]() -> bool
    {
        const CAIStrategy::SArmyOrNavySizeObjective *pairs = country->GetStrategy().GetArmySizeObjectives().GetData();
        const int count = country->GetStrategy().GetArmySizeObjectives().GetSize();
        if (count != 0)
        {
            for (const CAIStrategy::SArmyOrNavySizeObjective *it = pairs; it != pairs + count; ++it)
            {
                if (projectedMen <= it->_nSize && 0 < it->_vImportance.GetUnderlyingValue())
                    return true;
            }
        }

        return false;
    };
    if (meetsSizeObjective8())
        return false;

    int sum;
    const auto landMaintenance9 = [&]() -> void
    {
        CFixedPoint cavBase = country->CalcBaseLandMaintenanceCost(
            SubUnitType::CAVALRY, ESpecialUnitCategory::NORMAL, false, nullptr);
        sum = ::Div1000(static_cast<int64_t>(cavBase.GetUnderlyingValue()) * (nAdditionalRegulars * 1000));
        CFixedPoint cavBase2 = country->CalcBaseLandMaintenanceCost(
            SubUnitType::CAVALRY, ESpecialUnitCategory::MERCENARY, true, nullptr);
        sum += ::Div1000(static_cast<int64_t>(cavBase2.GetUnderlyingValue()) * (nAdditionalMercs * 1000));

        SLandMaintenance detail =
            country->GetLandMaintenance(CFixedPoint(1000),
                                        CCountry::ELandMaintenanceMode::IgnoreReinforcementsForPerformance,
                                        CCountry::ELandMaintenanceCacheMode::UseCachedCore);
        sum += detail._BaseCost._ArmyCost.GetUnderlyingValue() + detail._BaseCost._OldMercsCost.GetUnderlyingValue() +
               detail._BaseCost._MercenaryCompaniesCost.GetUnderlyingValue() + detail._BaseCost._CondottieriCost.GetUnderlyingValue() +
               detail._BaseCost._DrillCost.GetUnderlyingValue() +
               detail._ReinforcementCost._ArmyCost.GetUnderlyingValue() + detail._ReinforcementCost._OldMercsCost.GetUnderlyingValue() +
               detail._ReinforcementCost._MercenaryCompaniesCost.GetUnderlyingValue() + detail._ReinforcementCost._CondottieriCost.GetUnderlyingValue() +
               detail._ReinforcementCost._DrillCost.GetUnderlyingValue() +
               detail._ExtraCost._ArmyCost.GetUnderlyingValue() + detail._ExtraCost._OldMercsCost.GetUnderlyingValue() +
               detail._ExtraCost._MercenaryCompaniesCost.GetUnderlyingValue() + detail._ExtraCost._CondottieriCost.GetUnderlyingValue() +
               detail._ExtraCost._DrillCost.GetUnderlyingValue();
    };
    landMaintenance9();

    int threshold = country->GetStrategy().GetBudget(AIBudgetType::ARMIES).GetUnderlyingValue();
    if (nAdditionalRegulars == 0 && nAdditionalMercs == 0)
        threshold = ::Div1000(static_cast<int64_t>(threshold) * 1100);
    if (sum > threshold)
        return true;

    if (eval.WillReallySoonFightRebels(0x12, false))
        return false;

    if (nAdditionalMercs > 0 || !bOnlyCheckedType)
    {
        int64_t ratio;
        const auto manpowerFraction10 = [&]() -> void
        {
            int64_t maxMan10 = static_cast<int64_t>(country->GetMaxManpower().GetUnderlyingValue()) + 10;
            if (maxMan10 == 0)
                ratio = -1;
            else
            {
                int64_t man10 = static_cast<int64_t>(country->GetManpower().GetUnderlyingValue()) + 10;
                if (::IsValidRange(man10))
                    ratio = static_cast<int64_t>(int32_t(man10) * 1000 / static_cast<int32_t>(maxMan10));
                else
                    ratio = man10 * 1000 / maxMan10;
            }
        };
        manpowerFraction10();
        if (!rebelThreatActive)
        {
            int seaCount = country->GetNumOfSubUnits(ESpecialUnitCategory::MERCENARY);
            if (seaCount + nAdditionalMercs > 0)
            {
                CAIRecruitMinister::SCategoryScoreMultiplier mult;
                for (int i = 0; i < 23; i++)
                    mult[static_cast<ESpecialUnitCategory>(i)].SetUnderlyingValue(1000);
                mult[ESpecialUnitCategory::BANNER].SetUnderlyingValue(0);
                mult[ESpecialUnitCategory::COSSACKS].SetUnderlyingValue(0);
                mult[ESpecialUnitCategory::RAJPUT].SetUnderlyingValue(0);
                CArray<ESpecialUnitCategory> preferred =
                    CAIRecruitMinister::GetPreferredLandUnitCategories(*country, mult);
                int firstPreferred = static_cast<int>(preferred[0]);

                if (firstPreferred != 1)
                {
                    int landMaintFactor = NDefines::NEconomy::LAND_MAINTENANCE_FACTOR.GetUnderlyingValue();
                    CFixedPoint cavBuildCost = country->GetBuildCost(
                        ESpecialUnitCategory::NORMAL, SubUnitType::CAVALRY, nullptr, 0, nullptr);
                    if (static_cast<int>(ratio) >= 250)
                    {
                        int factor12 = landMaintFactor / 12;
                        int extra = ::Div1000(static_cast<int64_t>(factor12) * cavBuildCost.GetUnderlyingValue());
                        if (sum + extra >= threshold)
                            return true;
                        if (forceLimitPercent > 499)
                            return true;
                    }
                    if (country->GetCurrentNumberOfBankLoans() > 0)
                    {
                        CFixedPoint income = country->GetCurrentIncome(false);
                        CFixedPoint expenses = country->GetCurrentExpenses();
                        if (income.GetUnderlyingValue() < expenses.GetUnderlyingValue())
                            return true;
                    }
                }
            }
        }
    }
    return 1000 < forceLimitPercent;
}

bool CAIRecruitMinister::DisbandLandUnit()
{
    CCountry *country = CCountryDataBase::AccessInstance()->GetCountry(
        this->GetCountryTag());
    const CPlayerAiPrefs *prefs = &country->GetPlayerAiPrefs();
    if (prefs->IsEnabled() && !prefs->IsDisbandUnits())
        return false;

    bool threatened = true;
    if (!country->GetStrategy().IsPreparingWar() && !country->IsAtWar())
    {
        CFixedPoint rebelThreat = country->GetRelativeRebelThreat();
        threatened = rebelThreat.GetUnderlyingValue() > 0;
    }

    int minCost = 0xf4240;
    CSubUnit *best = nullptr;
    for (CUnit *unit : country->GetUnits())
    {
        if (!unit->IsArmy())
            continue;
        if (unit->InCombat())
            continue;
        if (unit->InSiege())
            continue;

        const CArray<CSubUnit *> &subs = unit->GetSubUnits();
        for (int i = 0; i < subs.GetSize(); ++i)
        {
            CSubUnit *sub = subs[i];
            int cost = sub->GetStrength().GetUnderlyingValue() + (this->GetRandom() % 0x19) * 10;

            switch (sub->GetCategory())
            {
            case ESpecialUnitCategory::MERCENARY:                                   
                continue;
            case ESpecialUnitCategory::BANNER:
                cost = cost * 100 + 50000;
                break;
            case ESpecialUnitCategory::STRELTSY:
                cost = ::Div1000(static_cast<int64_t>(cost) * 1500);
                break;
            case ESpecialUnitCategory::COSSACKS:
            case ESpecialUnitCategory::RAJPUT:
            case ESpecialUnitCategory::CAWA:
            case ESpecialUnitCategory::CAROLEAN:
            case ESpecialUnitCategory::HUSSARS:
            case ESpecialUnitCategory::MUSKETEER:
            case ESpecialUnitCategory::QIZILBASH:
            case ESpecialUnitCategory::MAMLUKS:
                cost = cost * 10;
                break;
            case ESpecialUnitCategory::JANISSARIES:
                cost = cost * 100 + 100000;
                break;
            case ESpecialUnitCategory::REVOLUTIONARY_GUARD:
                cost = cost * 2;
                break;
            case ESpecialUnitCategory::TERCIO:
                cost = cost * 50 + 25000;
                break;
            default:
                break;
            }
            if (!threatened)
            {
                if (CSubUnit::UseMercenaryReinforceCost(
                        sub->GetCategory()))
                    cost = cost / 10;
            }
            if (cost < minCost)
            {
                minCost = cost;
                best = sub;
            }
        }
    }
    if (best == nullptr)
        return false;
    ::PostAICommand(new CDisbandSubUnitCommand(best));
    return true;
}

ESpecialUnitCategory CAIRecruitMinister::MaintainFleet(CSpendingList &Proxy, bool &bNeedsMoney)
{
    bNeedsMoney = false;
    CCountry *country = CCountryDataBase::AccessInstance()->GetCountry(
        this->GetCountryTag());

    CEnumIndexedArray<STypeInfo, SubUnitType, 7> typeInfo;
    typeInfo = this->GetShipTypeInfo();

    SCategoryScoreMultiplier mult;
    for (int i = 0; i < 23; i++)
        mult[static_cast<ESpecialUnitCategory>(i)].SetUnderlyingValue(1000);
    mult[ESpecialUnitCategory::BANNER].SetUnderlyingValue(0);
    mult[ESpecialUnitCategory::COSSACKS].SetUnderlyingValue(0);
    mult[ESpecialUnitCategory::RAJPUT].SetUnderlyingValue(0);

    CArray<ESpecialUnitCategory> seaCategories =
        CAIRecruitMinister::GetPreferredSeaUnitCategories(*country, mult);
    const ESpecialUnitCategory seaCategory = seaCategories[0];

    bool hasOutdated = this->HasOutdatedShips(typeInfo);
    if (hasOutdated && this->UpgradeOutdatedShips())
        return seaCategory;

    CPdxArray<int, int> buildProvinces;
    for (int provId : country->GetOwnedProvinces())
    {
        CProvince *prov = &CCurrentGameState::GetInstance()->GetProvince(provId);
        if (prov->GetSiege() == nullptr &&
            prov->GetOwner().IsValid() &&
            prov->GetPort() != nullptr)
        {
            buildProvinces.SetSize(buildProvinces.GetSize() + 1, provId);
        }
    }
    const auto subjectProvinces11 = [&]() -> void
    {
        const CArray<CCountryTag> *subjects = &country->GetSubjects();
        const CCountryTag *begin = subjects->GetData();
        const CCountryTag *end = subjects->GetData() + subjects->GetSize();
        for (const CCountryTag *tag = begin; tag != end; ++tag)
        {
            CCountry *subject = CCountryDataBase::AccessInstance()->GetCountry(
                *tag);

            for (int provId : subject->GetOwnedProvinces())
            {
                CProvince *prov = &CCurrentGameState::GetInstance()->GetProvince(provId);
                if (prov->GetSiege() == nullptr &&
                    prov->GetOwner().IsValid() &&
                    prov->GetPort() != nullptr)
                {
                    buildProvinces.SetSize(buildProvinces.GetSize() + 1, provId);
                }
            }
        }
    };
    subjectProvinces11();
    this->IncludeShipsBeingBuilt(typeInfo, buildProvinces);

    int heavyDesired = typeInfo[SubUnitType::HEAVY_SHIP].nCurrent;
    int galleyDesired = typeInfo[SubUnitType::GALLEY].nCurrent;
    int lightDesired = typeInfo[SubUnitType::LIGHT_SHIP].nCurrent;
    int transportDesired = typeInfo[SubUnitType::TRANSPORT].nCurrent;
    country->GetStrategy().CalcDesiredAmountOfShipsFromCurrent(
        heavyDesired, galleyDesired, lightDesired, transportDesired);

    int desiredTotal = galleyDesired + heavyDesired + lightDesired + transportDesired;
    int currentTotal = typeInfo[SubUnitType::LIGHT_SHIP].nCurrent +
                       typeInfo[SubUnitType::HEAVY_SHIP].nCurrent +
                       typeInfo[SubUnitType::GALLEY].nCurrent +
                       typeInfo[SubUnitType::TRANSPORT].nCurrent;

    CFixedPoint navalForceLimit = country->GetNavalForceLimit(nullptr);
    const int forceLimitMilli = navalForceLimit.GetUnderlyingValue();

    const bool upgradeBlocked = hasOutdated ? !NDLCMechanics::CanUpgradeShips() : false;

    if (currentTotal > desiredTotal ||
        (forceLimitMilli / 1000 <= currentTotal && hasOutdated))
    {
        int narrowest = 7;
        int minDeficit = 0x7fffffff;
        int widest = 7;
        int maxDeficit = -0x7fffffff - 1;
        for (int t = 0; t < 7; t++)
        {
            if (static_cast<unsigned>(t - 3) < 4)
            {
                int deficit = typeInfo[static_cast<SubUnitType>(t)].nWanted -
                              typeInfo[static_cast<SubUnitType>(t)].nCurrent;
                if (deficit == 0)
                    continue;
                if (deficit < minDeficit)
                {
                    narrowest = t;
                    minDeficit = deficit;
                }
                else if (deficit > maxDeficit)
                {
                    widest = t;
                    maxDeficit = deficit;
                }
            }
        }
        if (widest != narrowest && static_cast<unsigned>(narrowest - 3) < 4)
        {
            int type = upgradeBlocked ? 7 : narrowest;
            CSubUnit *ship = this->FindShipToDisband(static_cast<SubUnitType>(type),
                                                     hasOutdated);
            if (ship != nullptr)
            {
                ::PostAICommand(new CDisbandSubUnitCommand(ship));
                return ESpecialUnitCategory::NO_REGIMENT;
            }
        }
    }
    else if (upgradeBlocked)
    {
        CSubUnit *ship = this->FindShipToDisband(SubUnitType::MAX, true);
        if (ship != nullptr)
        {
            ::PostAICommand(new CDisbandSubUnitCommand(ship));
            return ESpecialUnitCategory::NO_REGIMENT;
        }
    }
    if (currentTotal >= desiredTotal)
        return ESpecialUnitCategory::NO_REGIMENT;

    if (country->GetNumOfCorePorts() <= 0)
        return ESpecialUnitCategory::NO_REGIMENT;
    if (typeInfo[SubUnitType::INFANTRY].nCurrent +
            typeInfo[SubUnitType::CAVALRY].nCurrent +
            typeInfo[SubUnitType::ARTILLERY].nCurrent <=
        0)
        return ESpecialUnitCategory::NO_REGIMENT;

    const bool criticalHomeStrait = country->HasCriticalHomeAreaStrait() != 0;
    const int navalMaintenance = country->GetStrategy().GetBudget(AIBudgetType::NAVIES).GetUnderlyingValue();
    const CSubUnitDefinition *transportDef =
        country->GetSubunitTypePreferred(SubUnitType::GALLEY);
    if (!transportDef->IsValid())
        transportDef = country->GetSubunitTypePreferred(SubUnitType::TRANSPORT);
    CFixedPoint transportMaint = country->CalcActualNavalMaintenanceCost(
        transportDef->GetType(), ESpecialUnitCategory::NORMAL,
        CFixedPoint(1000), nullptr);
    CFixedPoint navalMaintTotal = country->GetNavalMaintenance(CFixedPoint(1000));
    const int maintSlack = (navalMaintenance - transportMaint.GetUnderlyingValue()) -
                           navalMaintTotal.GetUnderlyingValue();
    if (maintSlack < transportMaint.GetUnderlyingValue())
        return ESpecialUnitCategory::NO_REGIMENT;

    SubUnitType pickedType;
    const auto initialShipType12 = [&]() -> void
    {
        int roll = this->GetRandom() % 7;
        if (roll == 0)
        {
            if (heavyDesired > typeInfo[SubUnitType::HEAVY_SHIP].nCurrent &&
                typeInfo[SubUnitType::HEAVY_SHIP].pDefinition->IsValid())
                pickedType = SubUnitType::HEAVY_SHIP;
            else
                pickedType = SubUnitType::LIGHT_SHIP;
        }
        else if (roll - 1U < 2)
        {
            if (lightDesired > typeInfo[SubUnitType::LIGHT_SHIP].nCurrent &&
                typeInfo[SubUnitType::LIGHT_SHIP].pDefinition->IsValid())
                pickedType = SubUnitType::LIGHT_SHIP;
            else
                pickedType = SubUnitType::LIGHT_SHIP;
        }
        else if (roll - 3U < 2)
        {
            if (galleyDesired > typeInfo[SubUnitType::GALLEY].nCurrent &&
                typeInfo[SubUnitType::GALLEY].pDefinition->IsValid())
                pickedType = SubUnitType::GALLEY;
            else
                pickedType = SubUnitType::LIGHT_SHIP;
        }
        else
        {
            if (transportDesired > typeInfo[SubUnitType::TRANSPORT].nCurrent &&
                typeInfo[SubUnitType::TRANSPORT].pDefinition->IsValid())
                pickedType = SubUnitType::TRANSPORT;
            else
                pickedType = SubUnitType::LIGHT_SHIP;
        }
        if (criticalHomeStrait && galleyDesired / 2 > typeInfo[SubUnitType::GALLEY].nCurrent &&
            typeInfo[SubUnitType::GALLEY].pDefinition->IsValid())
            pickedType = SubUnitType::GALLEY;
    };
    initialShipType12();

    auto avail = [&typeInfo](SubUnitType t) -> bool
    { return typeInfo[t].pDefinition->IsValid(); };

    const auto prioritizeShipType13 = [&]() -> void
    {
        if (transportDesired / 2 > typeInfo[SubUnitType::TRANSPORT].nCurrent &&
            avail(SubUnitType::TRANSPORT))
            pickedType = SubUnitType::TRANSPORT;
        else if (lightDesired / 2 > typeInfo[SubUnitType::LIGHT_SHIP].nCurrent &&
                 avail(SubUnitType::LIGHT_SHIP))
            pickedType = SubUnitType::LIGHT_SHIP;
        else if (!criticalHomeStrait && galleyDesired / 2 > typeInfo[SubUnitType::GALLEY].nCurrent &&
                 avail(SubUnitType::GALLEY))
            pickedType = SubUnitType::GALLEY;
        else if (criticalHomeStrait && galleyDesired > typeInfo[SubUnitType::GALLEY].nCurrent &&
                 avail(SubUnitType::GALLEY))
            pickedType = SubUnitType::GALLEY;
        else if (transportDesired > typeInfo[SubUnitType::TRANSPORT].nCurrent &&
                 avail(SubUnitType::TRANSPORT))
            pickedType = SubUnitType::TRANSPORT;
        else if (lightDesired > typeInfo[SubUnitType::LIGHT_SHIP].nCurrent &&
                 avail(SubUnitType::LIGHT_SHIP))
            pickedType = SubUnitType::LIGHT_SHIP;
        else if (heavyDesired > typeInfo[SubUnitType::HEAVY_SHIP].nCurrent &&
                 avail(SubUnitType::HEAVY_SHIP))
            pickedType = SubUnitType::HEAVY_SHIP;
        else if (!criticalHomeStrait && galleyDesired > typeInfo[SubUnitType::GALLEY].nCurrent &&
                 avail(SubUnitType::GALLEY))
            pickedType = SubUnitType::GALLEY;
        else if (typeInfo[SubUnitType::GALLEY].nCurrent * kShipSizeScale < galleyDesired &&
                 avail(SubUnitType::GALLEY))
            pickedType = SubUnitType::GALLEY;
        else if (typeInfo[SubUnitType::LIGHT_SHIP].nCurrent * kShipSizeScale < lightDesired &&
                 avail(SubUnitType::LIGHT_SHIP))
            pickedType = SubUnitType::LIGHT_SHIP;
        else if (typeInfo[SubUnitType::GALLEY].nCurrent * 2 < galleyDesired &&
                 avail(SubUnitType::GALLEY))
            pickedType = SubUnitType::GALLEY;
        else if (typeInfo[SubUnitType::LIGHT_SHIP].nCurrent * 2 < lightDesired &&
                 avail(SubUnitType::LIGHT_SHIP))
            pickedType = SubUnitType::LIGHT_SHIP;
    };
    prioritizeShipType13();

    const auto fallbackShipType14 = [&]() -> void
    {
        if (lightDesired > 0 && pickedType == SubUnitType::LIGHT_SHIP &&
            typeInfo[SubUnitType::HEAVY_SHIP].nCurrent < heavyDesired)
        {
            int heavyPer = ::Div1000Checked(typeInfo[SubUnitType::HEAVY_SHIP].nCurrent * 1000,
                                            heavyDesired * 1000);
            int lightPer = ::Div1000Checked(typeInfo[SubUnitType::LIGHT_SHIP].nCurrent * 1000,
                                            lightDesired * 1000);
            if (lightPer > 0 &&
                ::Div1000Checked(heavyPer, lightPer) < 0x14a)
            {
                pickedType = typeInfo[SubUnitType::HEAVY_SHIP].pDefinition->IsValid()
                                 ? SubUnitType::HEAVY_SHIP
                                 : SubUnitType::LIGHT_SHIP;
            }
        }
        else
        {
            if (pickedType == SubUnitType::HEAVY_SHIP)
            {
                if (avail(SubUnitType::HEAVY_SHIP))
                    pickedType = SubUnitType::HEAVY_SHIP;
                else
                    pickedType = avail(SubUnitType::LIGHT_SHIP) ? SubUnitType::LIGHT_SHIP : (avail(SubUnitType::GALLEY) ? SubUnitType::GALLEY : SubUnitType::NO_SUB_UNIT_TYPE);
            }
            else if (pickedType == SubUnitType::LIGHT_SHIP)
            {
                pickedType = avail(SubUnitType::LIGHT_SHIP) ? SubUnitType::LIGHT_SHIP : (avail(SubUnitType::GALLEY) ? SubUnitType::GALLEY : SubUnitType::NO_SUB_UNIT_TYPE);
            }
            else if (pickedType == SubUnitType::TRANSPORT)
            {
                if (avail(SubUnitType::TRANSPORT))
                    pickedType = SubUnitType::TRANSPORT;
                else
                    pickedType = avail(SubUnitType::HEAVY_SHIP) ? SubUnitType::HEAVY_SHIP : (avail(SubUnitType::LIGHT_SHIP) ? SubUnitType::LIGHT_SHIP : (avail(SubUnitType::GALLEY) ? SubUnitType::GALLEY : SubUnitType::NO_SUB_UNIT_TYPE));
            }
            else
            {
                pickedType = avail(SubUnitType::GALLEY) ? pickedType : SubUnitType::NO_SUB_UNIT_TYPE;
            }
        }
    };
    fallbackShipType14();

    CFixedPoint pickedMaint = country->CalcActualNavalMaintenanceCost(
        country->GetSubunitTypePreferred(
                   pickedType)
            ->GetType(),
        static_cast<ESpecialUnitCategory>(seaCategory),
        CFixedPoint(1000), nullptr);
    if (maintSlack < pickedMaint.GetUnderlyingValue())
    {
        if (pickedType == SubUnitType::HEAVY_SHIP)
        {
            if (lightDesired > typeInfo[SubUnitType::LIGHT_SHIP].nCurrent)
                pickedType = SubUnitType::LIGHT_SHIP;
            else if (galleyDesired > typeInfo[SubUnitType::GALLEY].nCurrent)
                pickedType = SubUnitType::GALLEY;
            else if (transportDesired > typeInfo[SubUnitType::TRANSPORT].nCurrent)
                pickedType = SubUnitType::TRANSPORT;
        }
        else if (pickedType == SubUnitType::LIGHT_SHIP)
        {
            if (galleyDesired > typeInfo[SubUnitType::GALLEY].nCurrent)
                pickedType = SubUnitType::GALLEY;
            else if (transportDesired > typeInfo[SubUnitType::TRANSPORT].nCurrent)
                pickedType = SubUnitType::TRANSPORT;
        }
        else if (pickedType == SubUnitType::GALLEY)
        {
            if (transportDesired > typeInfo[SubUnitType::TRANSPORT].nCurrent)
                pickedType = SubUnitType::TRANSPORT;
        }
    }

    CFixedPoint flagshipDefMaint = country->CalcActualNavalMaintenanceCost(
        country->GetSubunitTypePreferred(pickedType)->GetType(),
        static_cast<ESpecialUnitCategory>(seaCategory),
        CFixedPoint(1000), nullptr);

    CFixedPoint buildCost = country->GetBuildCost(
        static_cast<ESpecialUnitCategory>(seaCategory),
        pickedType, nullptr, 0, nullptr);
    if (buildCost.GetUnderlyingValue() > country->GetTreasury().GetUnderlyingValue() &&
        maintSlack < flagshipDefMaint.GetUnderlyingValue())
    {
        bNeedsMoney = true;
        return seaCategory;
    }

    if (pickedType >= SubUnitType::HEAVY_SHIP && pickedType <= SubUnitType::TRANSPORT)
    {
        typeInfo[pickedType].vCost.SetUnderlyingValue(typeInfo[pickedType].vCost.GetUnderlyingValue() + 1000);
    }

    int maintSum;
    const auto navalMaintenance15 = [&]() -> void
    {
        CFixedPoint heavyBase = country->CalcBaseNavalMaintenanceCost(
            SubUnitType::HEAVY_SHIP, static_cast<ESpecialUnitCategory>(seaCategory), nullptr);
        CFixedPoint lightBase = country->CalcBaseNavalMaintenanceCost(
            SubUnitType::LIGHT_SHIP, static_cast<ESpecialUnitCategory>(seaCategory), nullptr);
        CFixedPoint galleyBase = country->CalcBaseNavalMaintenanceCost(
            SubUnitType::GALLEY, static_cast<ESpecialUnitCategory>(seaCategory), nullptr);
        CFixedPoint transportBase = country->CalcBaseNavalMaintenanceCost(
            SubUnitType::TRANSPORT, static_cast<ESpecialUnitCategory>(seaCategory), nullptr);
        maintSum = navalMaintTotal.GetUnderlyingValue() +
                   ::Div1000(static_cast<int64_t>(typeInfo[SubUnitType::HEAVY_SHIP].vCost.GetUnderlyingValue()) *
                             heavyBase.GetUnderlyingValue()) +
                   ::Div1000(static_cast<int64_t>(typeInfo[SubUnitType::LIGHT_SHIP].vCost.GetUnderlyingValue()) *
                             lightBase.GetUnderlyingValue()) +
                   ::Div1000(static_cast<int64_t>(typeInfo[SubUnitType::GALLEY].vCost.GetUnderlyingValue()) *
                             galleyBase.GetUnderlyingValue()) +
                   ::Div1000(static_cast<int64_t>(typeInfo[SubUnitType::TRANSPORT].vCost.GetUnderlyingValue()) *
                             transportBase.GetUnderlyingValue());
    };
    navalMaintenance15();

    if (maintSum < forceLimitMilli)
    {
        SAIDesiredFlagship flagship = CFlagshipModificationDatabase::GetInstance()
                                          ->GetDesiredFlagshipForAI(this->GetCountryTag());
        bool flagshipFlag = false;
        if (flagship._Type == pickedType)
        {
            flagshipFlag = ::FlagshipPrerequisitesFulfilled(*country, true,
                                                            pickedType, nullptr);
            if (flagshipFlag)
            {
                int flagSum = 0;
                for (int i = 0; i < flagship._Modifications.GetSize(); i++)
                {
                    const CFlagshipModification *mod = flagship._Modifications[i];
                    flagSum += mod->GetModifier().GetValue(_MODIFIER_NAVAL_MAINTENANCE_FLAGSHIP_).GetUnderlyingValue();
                }
                flagshipFlag = flagSum + maintSum < forceLimitMilli;
            }
        }

        CFixedPoint buildCost2 = country->GetBuildCost(
            static_cast<ESpecialUnitCategory>(seaCategory),
            SubUnitType::HEAVY_SHIP, nullptr, flagshipFlag ? 1 : 0, nullptr);
        if (pickedType != SubUnitType::HEAVY_SHIP &&
            heavyDesired <= typeInfo[SubUnitType::HEAVY_SHIP].nCurrent)
        {
            buildCost2.SetUnderlyingValue(country->GetBuildCost(
                                                     static_cast<ESpecialUnitCategory>(seaCategory),
                                                     SubUnitType::LIGHT_SHIP, nullptr, 0, nullptr)
                                              .GetUnderlyingValue());
        }
        CFixedPoint navyMoney = country->GetStrategy().GetNavyMoney();
        if (navyMoney.GetUnderlyingValue() < buildCost2.GetUnderlyingValue())
        {
            bNeedsMoney = true;
            return ESpecialUnitCategory::NO_REGIMENT;
        }

        const CModifier *cachedMods = &country->GetCachedData().GetModifiers().GetModifier();
        const bool mayExplore = cachedMods->GetBoolean(_MODIFIER_MAY_EXPLORE_);

        if (buildProvinces.GetSize() > 0)
        {
            float bestScore = 0.0f;
            CProvince *bestProvince = nullptr;
            for (int i = 0; i < buildProvinces.GetSize(); i++)
            {
                int provId = buildProvinces[i];
                CProvince *prov = &CCurrentGameState::GetInstance()->GetProvince(provId);
                CCountry *controller = CCountryDataBase::AccessInstance()->GetCountry(
                    prov->GetController());

                bool libertyOk = false;
                if (controller->GetOverlord().IsValid())
                {
                    const CSubjectType *subjectType = controller->GetSubjectType();
                    if (!subjectType->IsVoluntary())
                    {
                        if (controller->GetOverlord().GetCategoryWord() ==
                            static_cast<uint32_t>(this->GetCountryTag().GetIndex()) << 16)
                        {
                            CFixedPoint libertyDesire = controller->GetLibertyDesire(nullptr, 0, false);
                            libertyOk = libertyDesire.GetUnderlyingValue() <=
                                        NDefines::NEconomy::MAX_LD_FOR_CONSTRUCT_IN_SUBJECT.GetUnderlyingValue();
                        }
                    }
                }

                if (!CCountryTag::SameUpperWord(prov->GetController(), this->GetCountryTag()) && !libertyOk)
                    continue;
                CCountryTag ownerTag = prov->GetController();
                if (std::find_if(prov->GetCores().GetData(), prov->GetCores().GetData() + prov->GetCores().GetSize(),
                                 [&ownerTag](const CCountryTag &t) -> bool
                                 {
                                     return t.GetRaw() == ownerTag.GetRaw();
                                 }) == prov->GetCores().GetData() + prov->GetCores().GetSize())
                    continue;

                if (prov->GetSiege() != nullptr || prov->GetCombat() != nullptr)
                    continue;
                if (!prov->GetOwner().IsValid() ||
                    prov->GetPort() == nullptr)
                    continue;

                if (flagshipFlag &&
                    !::CanBuildFlagshipInProvince(*country, prov, true,
                                                  pickedType, nullptr))
                    continue;

                int modValue = prov->GetModifier().GetValue(_MODIFIER_LOCAL_SHIP_COST_).GetUnderlyingValue();
                float score = (modValue / 1000.0f) * -9.0f;
                const bool adjacentEnemy = prov->HasAdjacentEnemy(this->GetCountryTag());
                float slack = std::min(9.0f, score);
                float full = 100.0f / (10.0f - slack);
                if (adjacentEnemy)
                    score = full / 10.0f;
                else
                    score = full;
                if (prov->HasMilitaryConstruction())
                {
                    float constructions = prov->GetConstructions().GetSize();
                    score = score / (constructions + constructions);
                }
                if (libertyOk)
                    score = score / 1.5f;
                if (!mayExplore)
                {
                    const auto isPenalizedPortSeaZone = [](int portSeaZone) -> bool
                    {
                        const unsigned slot = static_cast<unsigned>(portSeaZone) - kPortSeaZoneIdBase;
                        return slot < kPortSeaZoneSlotCount && ((kPenalizedPortSeaZoneMask >> slot) & 1U) != 0;
                    };
                    if (isPenalizedPortSeaZone(prov->GetTemplate().GetSeaZoneForPort()))
                        score = score / kPortSeaZoneScoreDivisor;
                }
                if (score > bestScore)
                {
                    bestScore = score;
                    bestProvince = prov;
                }
            }

            if (bestProvince != nullptr)
            {
                CList<const CSubUnitDefinition *> buildable = bestProvince->GetBuildableShips();
                const CSubUnitDefinition *def = nullptr;
                for (const CSubUnitDefinition *entry : buildable)
                {
                    const CSubUnitDefinition *d = entry;
                    if (d->GetType() == pickedType)
                    {
                        def = d;
                        break;
                    }
                }
                if (def != nullptr)
                {
                    CCommand *cmd;
                    if (flagshipFlag)
                    {
                        CPdxArray<CString, int> names;
                        for (int i = 0; i < flagship._Modifications.GetSize(); i++)
                        {
                            const CFlagshipModification *mod = flagship._Modifications[i];
                            names.SetSize(i + 1, mod->GetName());
                        }
                        CString generatedName = country->GenerateSubUnitName(def, bestProvince,
                                                                             static_cast<ESpecialUnitCategory>(seaCategory), nullptr, nullptr, false);
                        cmd = ::BuildFlagshipConstructionCommandForProvince(*country, bestProvince, def,
                                                                            names, generatedName);
                    }
                    else
                    {
                        cmd = new CConstructUnitCommand(this->GetCountryTag(), def,
                                                        bestProvince->GetProvinceID(),
                                                        static_cast<ESpecialUnitCategory>(seaCategory));
                    }
                    CFixedPoint cost = country->GetBuildCost(ESpecialUnitCategory::NORMAL,
                                                             def->GetType(), bestProvince,
                                                             flagshipFlag ? 1 : 0, nullptr);
                    Proxy.AddItem(std::unique_ptr<CCommand>(cmd), cost, CFixedPoint(0x2710));
                }
            }
        }
        CFixedPoint transportCost = country->GetBuildCost(ESpecialUnitCategory::NORMAL, SubUnitType::TRANSPORT,
                                                          nullptr, 0, nullptr);
        bNeedsMoney = transportCost.GetUnderlyingValue() > country->GetTreasury().GetUnderlyingValue();
    }
    else
    {
        bNeedsMoney = true;
        return hasOutdated ? seaCategory : ESpecialUnitCategory::NO_REGIMENT;
    }
    return ESpecialUnitCategory::NO_REGIMENT;
}
