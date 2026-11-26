#include "unit.h"
#include "army.h"
#include "combat.h"
#include "country.h"
#include "countrytag.h"
#include "defines.h"
#include "enums.h"
#include "gamestate.h"
#include "leader.h"
#include "map.h"
#include "modifier_enums.h"
#include "navy.h"
#include "province.h"
#include "provincetemplate.h"
#include "subunit.h"
#include "unitmission.h"

#include <cstdint>

CFixedPointFPML CUnit::CalcArmyStrengthFactor(const CProvince *pCombatProvinceOrNullptr, bool bLowMoralePenalty,
                                              bool bAttached, int32_t &nLeaderPips) const
{
    CCountry *techCountry = CCountryDataBase::AccessInstance()->GetCountry(this->GetTechCountry());

    int64_t totalStrengthFP = 0;
    int32_t totalBaseStrength = 0;
    int32_t cavBaseStrength = 0;

    CEnumIndexedArray<std::pair<const CSubUnitDefinition *, CFixedPoint>, SubUnitType, SUBUNIT_LAND_COUNT> defCache;
    CEnumIndexedArray<std::pair<const CSubUnitDefinition *, CFixedPoint>, ESpecialUnitCategory, static_cast<unsigned long>(ESpecialUnitCategory::MAX)> catCache;

    CFixedPointFPML subFP;
    for (int subIndex = 0; subIndex < this->GetSubUnits().GetSize(); ++subIndex)
    {
        CSubUnit *sub = this->GetSubUnits()[subIndex];
        subFP = sub->CalcStrengthFactor(*techCountry, pCombatProvinceOrNullptr, bLowMoralePenalty,
                                        nullptr, defCache, catCache);
        totalStrengthFP += ::RawFPML(subFP);

        int32_t baseStr = sub->GetStrength().GetUnderlyingValue();
        totalBaseStrength += baseStr;

        if (sub->GetDefinition()->GetType() == SubUnitType::CAVALRY)
            cavBaseStrength += baseStr;
    }

    const int64_t totalBaseFP = static_cast<int64_t>(totalBaseStrength) << CFixedPointFPML::fractional_bit_count;
    if (totalBaseFP > 0 && !this->GetCountry().IsRebels())
    {
        int modifier = this->GetAnyModifierValue(ModifierType::_MODIFIER_CAV_TO_INF_RATIO_, false, nullptr).GetUnderlyingValue();
        int cavCap = 0;
        if (modifier >= 0)
        {
            cavCap = (modifier > kFixedPointResolution) ? kFixedPointResolution : modifier;
        }
        int cavRatio = 0;
        if (totalBaseStrength != 0)
        {
            cavBaseStrength = ::Div1000Checked(cavBaseStrength, totalBaseStrength);
            if (cavBaseStrength >= 0)
                cavRatio = (cavBaseStrength < kFixedPointResolution) ? cavBaseStrength : kFixedPointResolution;
        }
        if (cavRatio - cavCap != 0 && cavCap <= cavRatio)
        {
            int64_t penalty = static_cast<int64_t>(NDefines::NMilitary::INSUFFICIENT_SUPPORT.GetUnderlyingValue()) * (cavRatio - cavCap);
            int32_t factor = kFixedPointResolution + ::Div1000(penalty);
            const int64_t factorFP = static_cast<int64_t>(factor) << CFixedPointFPML::fractional_bit_count;
            totalStrengthFP = (totalStrengthFP * factorFP) >> CFixedPointFPML::fractional_bit_count;
        }
    }

    if (this->GetLeader() && this->GetLeader()->IsValid())
    {
        int pips = this->GetLeader()->GetFire() + this->GetLeader()->GetShock();
        if (pips > nLeaderPips)
            nLeaderPips = pips;
    }

    if (bAttached)
    {
        totalStrengthFP += ::RawFPML(this->GetAttachedArmyStrengthFactor(pCombatProvinceOrNullptr, bLowMoralePenalty, true, nLeaderPips));
    }

    return ::FPMLFromRaw(totalStrengthFP);
}

CFixedPointFPML CUnit::GetArmyStrengthFactor(const CProvince *pCombatProvinceOrNullptr, bool bLowMoralePenalty,
                                             bool bAttached, int32_t &nLeaderPips) const
{
    if (bAttached && this->GetAttachedTo() != nullptr &&
        this->GetAttachedTo()->GetLocation() == this->GetLocation())
        return ::FPMLFromRaw(0);
    return this->CalcArmyStrengthFactor(pCombatProvinceOrNullptr, bLowMoralePenalty, bAttached, nLeaderPips);
}

CProvince *CUnit::GetFinalDestination() const
{
    if (this->GetPath().GetSize() > 0)
        return &CMap::_pMap->GetProvince(*this->GetPath().GetTail());
    return nullptr;
}

CProvince *CUnit::GetNextTarget() const
{
    if (this->GetPath().GetSize() > 0)
        return &CMap::_pMap->GetProvince(*this->GetPath().begin());
    return nullptr;
}

CProvince *CUnit::GetFinalLocation() const
{
    if (this->GetPath().GetSize() == 0)
        return this->GetLocation();
    if (this->GetPath().GetSize() > 0)
        return &CMap::_pMap->GetProvince(*this->GetPath().GetTail());
    return nullptr;
}

CFixedPoint CUnit::MonthlyUpdate(bool bChangeMorale)
{
    int outAttrition = 0;

    if (this->GetCombat() == nullptr)
    {
        const CCurrentGameState *game = CCurrentGameState::GetInstance();
        if (game->HasGameStarted() != 0)
        {
            if (this->IsNavy())
                this->GetNavy()->UseSailorsMonthly();

            if (!(this->GetMission() != nullptr && this->AccessMission()->GetType() == EUnitMission::AM_DRILL_ARMY))
                this->MonthlyDecayDrill();

            outAttrition = this->ApplyAttrition().GetUnderlyingValue();

            if (this->IsNavy())
                this->GetNavy()->RemoveOverload(true);

            bool skipReset = this->IsRetreating();
            if (!skipReset && this->IsArmy() && this->GetArmy()->IsLoaded())
                skipReset = true;
            if (!skipReset && this->IsNavy() && this->GetNavy()->IsMothballing())
                skipReset = true;
            if (!skipReset)
            {
                CCountry *country = CCountryDataBase::AccessInstance()
                                        ->GetCountry(this->GetCountry());
                CFixedPoint men(country->GetManpower());
                CFixedPoint sailors(country->GetSailors());
                this->ReinforceAndRepair(men, sailors);
                country->SetManpower(men);
                country->SetSailors(sailors);
            }
        }

        if (bChangeMorale != 0)
        {
            if (this->AccessSiege() == nullptr ||
                this->AccessSiege()->CanLeaveCombat())
            {
                if (!this->IsNavy() || !this->IsRetreating())
                    this->AdjustMorale();
            }
        }
    }

    if (this->IsNavy())
    {
        CNavy *navy = this->GetNavy();
        CCountry *country = CCountryDataBase::AccessInstance()
                                ->GetCountry(this->GetCountry());
        if (navy->IsNavyAtDangerousSea(navy->GetLocation()) &&
            bChangeMorale != 0)
        {
            if (!(this->GetMission() != nullptr &&
                  this->AccessMission()->GetType() == EUnitMission::NM_EXPLORE))
            {
                if (!country->GetTechnologyStatus()->Allow(
                        TechFlags::_TECH_FLAGS_HAS_REDUCED_NAVAL_ATTRITION_))
                    navy->SetAtSea(navy->GetAtSea() + 1);
            }
        }
        if (navy->IsMothballing())
        {
            int sailorLoss = 0;
            auto it = this->GetSubUnits().GetData();
            while (it < this->GetSubUnits().GetData() + this->GetSubUnits().GetSize())
            {
                CSubUnit *sub = *it;
                ++it;
                int baseStrength = sub->GetStrength().GetUnderlyingValue();
                if (baseStrength > NDefines::NMilitary::MOTHBALLING_MINIMUM_STRENGTH.GetUnderlyingValue())
                {
                    int decrease = baseStrength - NDefines::NMilitary::MOTHBALLING_MONTHLY_DECREASE.GetUnderlyingValue();
                    if (decrease < NDefines::NMilitary::MOTHBALLING_MINIMUM_STRENGTH.GetUnderlyingValue())
                        decrease = NDefines::NMilitary::MOTHBALLING_MINIMUM_STRENGTH.GetUnderlyingValue();
                    int lost = baseStrength - decrease;
                    sailorLoss += ::Div1000(
                        static_cast<int64_t>(sub->GetDefinition()->GetDefaultMenCost().GetUnderlyingValue()) *
                        lost);
                    sub->LoseStrength(CFixedPoint(lost));
                }
            }
            navy->ReturnSailors(CFixedPoint(sailorLoss));
        }
    }

    const auto discardPeaceAttrition = [&]() -> void
    {
        CCountry *country = CCountryDataBase::AccessInstance()
                                ->GetCountry(this->GetCountry());
        if (!country->IsAtWar() &&
            CCountryTag::SameUpperWord(this->GetLocation()->GetOwner(), this->GetCountry()))
            outAttrition = 0;
    };
    discardPeaceAttrition();
    this->ProcessMissionMonthly();
    this->UpdateUnsafeMoves();
    return CFixedPoint(outAttrition);
}

CFixedPoint CUnit::GetAverageMorale(bool bRetActualValue) const
{
    CFixedPoint out(0);
    if (this->GetSubUnits().GetSize() == 0)
        return out;

    CFixedPoint maxFP = this->GetMaximumMorale(CUnit::EMoraleType::Simulation, nullptr);
    int maxMorale = maxFP.GetUnderlyingValue();

    int acc = 0;
    for (int subIndex = 0; subIndex < this->GetSubUnits().GetSize(); ++subIndex)
    {
        CSubUnit *sub = this->GetSubUnits()[subIndex];
        if (sub == nullptr)
            continue;
        int scaled = ::Div1000(static_cast<int64_t>(sub->GetStrength().GetUnderlyingValue()) * sub->GetMorale().GetUnderlyingValue());
        if (!bRetActualValue)
        {
            if (maxMorale == 0)
                scaled = -1;
            else
                scaled = ::Div1000Checked(scaled, maxMorale);
        }
        acc += scaled;
    }

    int totalBase = 0;
    for (int subIndex = 0; subIndex < this->GetSubUnits().GetSize(); ++subIndex)
        totalBase += this->GetSubUnits()[subIndex]->GetStrength().GetUnderlyingValue();

    if (totalBase == 0)
    {
        out.SetUnderlyingValue(0);
        return out;
    }
    out.SetUnderlyingValue((static_cast<int64_t>(acc) * kFixedPointResolution) / totalBase);
    return out;
}

bool CUnit::IsUnsafeMove(int nProv) const
{
    if (this->GetLocation()->GetProvinceID() == nProv)
        return false;

    const CCurrentGameState *const game = CCurrentGameState::GetInstance();
    const CProvince *const target = &game->GetProvince(nProv);
    const int today = game->GetCurrentDate().GetUniversalDate();

    const CRegion *const targetRegion = target->GetRegion();
    if (targetRegion != this->GetLocation()->GetRegion())
    {
        const CFixedPoint distance = this->GetLocation()->GetDistance(nProv, false);
        for (int i = this->_UnsafeRegions.GetSize() - 1; i >= 0; --i)
        {
            if (this->_UnsafeRegions[i].GetId() != targetRegion)
                continue;
            if (today <= this->_UnsafeRegions[i].GetDate().GetUniversalDate() &&
                distance.GetUnderlyingValue() >= this->_UnsafeRegions[i].GetDistance().GetUnderlyingValue())
                return true;
            break;
        }
    }

    const CMapArea *const ourAreaLink = this->GetLocation()->GetTemplate().GetMapArea();
    const CMapArea *const targetAreaLink = target->GetTemplate().GetMapArea();
    if (ourAreaLink != targetAreaLink)
    {
        const CFixedPoint distance = this->GetLocation()->GetDistance(nProv, false);
        for (int i = this->_UnsafeAreas.GetSize() - 1; i >= 0; --i)
        {
            if (this->_UnsafeAreas[i].GetId() != targetAreaLink)
                continue;
            if (today <= this->_UnsafeAreas[i].GetDate().GetUniversalDate() &&
                distance.GetUnderlyingValue() >= this->_UnsafeAreas[i].GetDistance().GetUnderlyingValue())
                return true;
            break;
        }
    }

    for (int i = this->_UnsafeProvs.GetSize() - 1; i >= 0; --i)
    {
        if (this->_UnsafeProvs[i].GetId() != nProv)
            continue;
        if (today > this->_UnsafeProvs[i].GetDate().GetUniversalDate())
            return false;
        return this->_UnsafeProvs[i].GetDistance().GetUnderlyingValue() <= 0;
    }
    return false;
}

void CUnit::WriteAIData(CWriter &)
{
}

CArray<CSubUnit *> &CUnit::GetSubUnits()
{
    return *this;
}
