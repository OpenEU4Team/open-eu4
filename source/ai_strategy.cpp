#include "ai_strategy.h"
#include "adjacency.h"
#include "ai_threatevaluation.h"
#include "country.h"
#include "countrytag.h"
#include "defines.h"
#include "enums.h"
#include "gamestate.h"
#include "province.h"
#include "strength_factor_cache.h"
#include "subjecttype.h"
#include "subunit.h"

int CAIStrategy::CalcDesiredAmountOfRegiments() const
{
    const auto number = this->GetCountryTag().GetIndex();
    const CCountry *country = CCountryDataBase::AccessInstance()->GetDBCountryList()[number];
    return country->GetLandForceLimit(nullptr).GetUnderlyingValue() / 1000;
}

void CAIStrategy::CalcDesiredAmountOfShipsFromCurrent(int &nHeavies, int &nGalleys,
                                                      int &nLight, int &nTransports) const
{
    const auto divideMilli = [](int64_t value) -> int
    { return static_cast<int>(value / 1000); };
    if (!this->IsBudgetInitialized())
        return;
    const int number = this->GetCountryTag().GetIndex();
    const CCountry *country = CCountryDataBase::AccessInstance()->GetDBCountryList()[number];

    int currentTotal = nGalleys + nHeavies + nLight + nTransports;
    CFixedPoint navalForceLimitFP = country->GetNavalForceLimit(nullptr);
    int navalForceLimit = navalForceLimitFP.GetUnderlyingValue();
    int limitRegiments = navalForceLimit / 1000;

    bool overLimit = limitRegiments < currentTotal;
    if (navalForceLimit / 2000 < currentTotal &&
        country->GetUnits().GetCount(SubUnitType::GALLEY) > 0)
    {
        overLimit = (country->GetInlandSeaRatio().GetUnderlyingValue() <
                     NDefines::NCountry::GALLEY_INLAND_SEA_COAST_RATIO.GetUnderlyingValue()) ||
                    overLimit;
    }

    CFixedPoint navalMaint = country->GetNavalMaintenance(CFixedPoint(1000));
    int budgetFlag = 1;
    if (navalMaint.GetUnderlyingValue() <= divideMilli(static_cast<int64_t>(country->GetStrategy().GetBudget(NAVIES).GetUnderlyingValue()) * 1100))
        budgetFlag = overLimit ? 1 : 0;
    const auto adjustBudget = [&]() -> void
    {
        if (!country->GetStrategy().GetWarTarget().IsValid() && !country->IsAtWar())
        {
            if (country->GetRelativeRebelThreat().GetUnderlyingValue() < 200)
                return;
        }
        if (country->GetCurrentNumberOfBankLoans() <
            country->GetMaximumBankLoans() / 2)
            budgetFlag = 0;
    };
    adjustBudget();
    bool budgetTight = budgetFlag != 0;

    int transportDesired = 0;
    if (this->IsNavyUseful())
    {
        CFixedPoint landForceLimitFP = CCountryDataBase::AccessInstance()->GetDBCountryList()[this->GetCountryTag().GetIndex()]->GetLandForceLimit(nullptr);
        int transportsCap = CFixedPoint(landForceLimitFP.GetUnderlyingValue() / 2).GetRounded();
        int tf = divideMilli(static_cast<int64_t>(limitRegiments * 1000) * NDefines::NAI::TRANSPORT_FRACTION.GetUnderlyingValue()) / 1000;
        if (transportsCap < tf)
            tf = transportsCap + 1;
        int supplyLimit = CCountryDataBase::AccessInstance()->GetDBCountryList()[this->GetCountryTag().GetIndex()]->GetTechnologyStatus()->GetSupplyLimit().GetUnderlyingValue();
        int supplyCap = (supplyLimit * 20 + 20000) / 1000;
        if (tf <= supplyCap)
            supplyCap = tf;
        transportDesired = (0 < supplyCap) ? supplyCap : 1;
    }

    int bigFraction = NDefines::NAI::BIGSHIP_FRACTION.GetUnderlyingValue();
    const CSubjectType *subjectType = country->GetSubjectType();
    int minLimitForBigShips = 10;

    const bool subjectHalvesBigShips =
        subjectType != nullptr && subjectType->TransferTradePower();
    if (subjectHalvesBigShips)
    {
        bigFraction /= 2;
        minLimitForBigShips = 0x32;
    }
    if (limitRegiments < minLimitForBigShips)
        bigFraction = 0;

    int nonTransport = limitRegiments - transportDesired;
    if (country->HasCriticalHomeAreaStrait() != 0)
    {
        int extra = navalForceLimit / 7000;
        nonTransport += extra;
        int galleyShare = static_cast<unsigned int>(transportDesired) / 3;
        if (extra < galleyShare)
            galleyShare = extra;
        transportDesired -= galleyShare;
    }

    int bigDesired = divideMilli(static_cast<int64_t>(nonTransport * 1000) * bigFraction) / 1000;
    int lightRemaining = nonTransport - bigDesired;

    int galleyDesired;
    int galleySlot;
    const int inlandSeaRatio = country->GetInlandSeaRatio().GetUnderlyingValue();
    if (inlandSeaRatio < NDefines::NCountry::GALLEY_INLAND_SEA_COAST_RATIO.GetUnderlyingValue())
    {
        galleyDesired = bigDesired;
        galleySlot = 0;
    }
    else
    {
        int halfPermille;
        if (navalForceLimit < 50000)
        {
            const int half = ::IsValidRange(bigDesired * 1000)
                                 ? static_cast<int32_t>(static_cast<uint32_t>(bigDesired) * 1000000U) / 2000
                                 : bigDesired * 500;
            int64_t scaled = (static_cast<int64_t>(limitRegiments * 1000) * half) / 1000;
            int divided = ::IsValidRange(static_cast<int>(scaled))
                              ? static_cast<int>(scaled) / 50
                              : static_cast<int>(static_cast<int64_t>(static_cast<int>(scaled)) / 50);
            halfPermille = (divided / 1000) * 1000;
        }
        else
        {
            halfPermille = ::IsValidRange(bigDesired * 1000)
                               ? static_cast<int32_t>(static_cast<uint32_t>(bigDesired) * 1000000U) / 2000
                               : bigDesired * 500;
        }
        const int splitRatio = country->GetInlandSeaRatio().GetUnderlyingValue();
        int64_t denom = 1000 - static_cast<int64_t>(NDefines::NCountry::GALLEY_INLAND_SEA_COAST_RATIO.GetUnderlyingValue());

        int numer = divideMilli(static_cast<int64_t>(halfPermille - bigDesired * 1000) *
                                (splitRatio - NDefines::NCountry::GALLEY_INLAND_SEA_COAST_RATIO.GetUnderlyingValue()));
        int galleyPermille = (static_cast<int>(denom) == 0)
                                 ? -1
                             : ::IsValidRange(numer)
                                 ? (numer * 1000) / static_cast<int>(denom)
                                 : static_cast<int>((static_cast<int64_t>(numer) * 1000) / denom);
        int galleys = (galleyPermille + bigDesired * 1000) / 1000;
        galleyDesired = galleys;
        int remaining = bigDesired - galleys;
        if (!country->HasTradeInterest())
        {
            lightRemaining /= 2;
            galleySlot = remaining + lightRemaining;
        }
        else
        {
            galleySlot = remaining;
        }
    }

    int heavyCandidate;
    if (!this->IsNavyUseful())
    {
        heavyCandidate = galleyDesired;
    }
    else
    {
        bool rich = this->IsRich(CFixedPoint(0));
        CArray<CCountryTag> competitors;
        this->GetPotentialNavalCompetition(competitors, true);
        if (competitors.GetSize() == 0)
        {
            heavyCandidate = 0;
        }
        else
        {
            float capScale = rich ? kNavalCompCapRich : kNavalCompFactorAlt;
            if (budgetTight)
                capScale = kNavalCompCapRich;
            int wantedTotal = galleySlot + transportDesired + lightRemaining + galleyDesired;

            int room = limitRegiments - wantedTotal;
            float ourCapital = static_cast<float>((galleySlot / 2) + galleyDesired);

            float competitorCap = static_cast<float>(limitRegiments) * kNavalCompCapLimit;
            int totalCap = static_cast<int>(capScale * static_cast<float>(limitRegiments));
            int best = 0;
            for (int competitorIndex = 0; competitorIndex < competitors.GetSize(); ++competitorIndex)
            {
                const CCountryTag &tag = competitors[competitorIndex];
                const CCountry *c = CCountryDataBase::AccessInstance()->GetDBCountryList()[tag.GetIndex()];
                CFixedPoint theirLimitFP = c->GetNavalForceLimit(nullptr);
                if (!(static_cast<float>(theirLimitFP.GetUnderlyingValue() / 1000) > competitorCap))
                {
                    int heavies = c->GetUnits().GetCount(SubUnitType::HEAVY_SHIP);
                    int galleys = c->GetUnits().GetCount(SubUnitType::GALLEY);
                    float theirCapital = static_cast<float>(galleys / 2 + heavies);
                    int against15 = static_cast<int>(theirCapital * kNavalCompFactor - ourCapital);
                    int against12 = static_cast<int>(theirCapital * kNavalCompFactorAlt - ourCapital);
                    int candidate = (room <= against15) ? room : against15;
                    if (candidate < against12)
                        candidate = against12;
                    if (wantedTotal + candidate <= totalCap && best < candidate)
                        best = candidate;
                }
            }
            heavyCandidate = best;
        }
        heavyCandidate += galleyDesired;
    }

    int heavy = this->GetWantedSubUnits(SubUnitType::HEAVY_SHIP);
    if (heavy <= heavyCandidate)
        heavy = heavyCandidate;
    int galley = this->GetWantedSubUnits(SubUnitType::GALLEY);
    if (galley <= galleySlot)
        galley = galleySlot;
    int light = this->GetWantedSubUnits(SubUnitType::LIGHT_SHIP);
    if (light <= lightRemaining)
        light = lightRemaining;
    int transport = this->GetWantedSubUnits(SubUnitType::TRANSPORT);
    if (transport <= transportDesired)
        transport = transportDesired;
    if (budgetTight || currentTotal <= galley + heavy + light + transport)
    {
        nHeavies = heavy;
        nGalleys = galley;
        nLight = light;
        nTransports = transport;
    }
}

bool CAIStrategy::CalcIsDefendedHomeStraitUnderAttack()
{
    if (this->GetDefendedHomeStrait() <= 0)
        return false;

    CStrengthFactorCache &cache = CStrengthFactorCacheManager::AccessCache();
    const int horizonRaw = CCurrentGameState::GetInstance()->GetCurrentDate().GetUniversalDate() + 0x960;
    CEU3Date horizon = CCurrentGameState::GetInstance()->GetCurrentDate();
    horizon.SetUniversalDate(horizonRaw);
    const CProvince *home =
        &CCurrentGameState::GetInstance()->GetProvince(this->GetDefendedHomeStrait());

    const auto &straits = home->GetStraits();
    auto it = straits.begin();
    while (it != straits.end())
    {
        const SAdjacency *strait = *it;
        ++it;
        const CAIThreatEvaluation &fromEvaluation = CAIThreatEvaluation::GetInstance();
        CFixedPoint fromThreat =
            fromEvaluation.CalcArmyThreatForCountry(
                strait->GetFrom().GetProvinceID(), horizon, this->GetCountryTag(), false, cache);
        if (fromThreat.GetUnderlyingValue() > 0)
            return true;

        const CAIThreatEvaluation &toEvaluation = CAIThreatEvaluation::GetInstance();
        CFixedPoint toThreat =
            toEvaluation.CalcArmyThreatForCountry(
                strait->GetProvince().GetProvinceID(), horizon, this->GetCountryTag(), false, cache);
        if (toThreat.GetUnderlyingValue() > 0)
            return true;
    }
    return false;
}

CCountryTag CAIStrategy::GetWarTarget() const
{
    return this->_WarTarget;
}
