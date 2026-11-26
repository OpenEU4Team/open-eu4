#pragma once
#include "ai_threatevaluation.h"

inline size_t NAiThreat::CIDHash::operator()(CID id) const
{
    return static_cast<size_t>(id.GetType() << 16) + id.GetID();
}
inline bool NAiThreat::SThreatEntry::operator<(const SThreatEntry &other) const
{
    if (this->_bCurrentlyBlocked != other._bCurrentlyBlocked)
        return this->_bCurrentlyBlocked > other._bCurrentlyBlocked;
    if (this->_nSteps != other._nSteps)
        return this->_nSteps > other._nSteps;
    return this->_date > other._date;
}

#include "engine.h"
#include "unit.h"
#include <cmath>

inline CFixedPoint NAiThreat::SUnitEvaluatorForUnit::CalcScore(const std::pair<float, float> &Strength)
{
    int result = kThreatSentinel;
    if (this->_fOwnStrength > 0.0f)
    {
        float ratio = Strength.second / (this->_fOwnStrength + Strength.first);
        float scaled = kMilli * ratio + kHalf;
        result = static_cast<int>(ratio >= 0.0f ? scaled : std::floor(scaled));
    }
    return CFixedPoint(result);
}

template <typename Evaluator>
CFixedPoint CAIThreatEvaluation::CalcThreatForUnit(const CArray<CArray<NAiThreat::SThreatEntry>> &ThreatArray, int nProvinceID, const CEU3Date &maxDate, const CUnit *pUnit, bool bIncludeNeutral, CStrengthFactorCache &Cache, Evaluator Eval) const
{
    (void)bIncludeNeutral;

    if (!this->_bIsInitialized)
        return CFixedPoint(0);

    int dayDelta = maxDate.GetTotalDays() - this->_CompareDate.GetTotalDays();

    CFixedPoint threat = NAiThreat::CalcThreat(
        ThreatArray[nProvinceID], dayDelta, this->_ArmyVersions, Eval);

    CFixedPoint native = NAiThreat::GetNativeAndRebelThreat(nProvinceID, Cache, pUnit);
    return CFixedPoint(threat.GetUnderlyingValue() + native.GetUnderlyingValue());
}

template <typename Evaluator>
CFixedPoint NAiThreat::CalcThreat(const CArray<SThreatEntry> &Threats, int maxDays, const std::unordered_map<CID, std::pair<uint8_t, CEU3Date>, CIDHash> &ArmyVersions, Evaluator Eval)
{

    std::pair<float, float> acc;

    size_t count = Threats.GetSize();
    for (size_t i = 0; i < count; ++i)
    {
        const SThreatEntry &entry = Threats[i];

        auto it = ArmyVersions.find(entry._UnitID);
        if (it == ArmyVersions.end())
            continue;
        if (entry._nArmyVersion != it->second.first)
            continue;
        if (entry._date > maxDays)
            continue;

        CUnit *unit = entry.GetUnit();
        if (unit == nullptr)
            continue;

        float f12 = kHalf;
        if (!unit->GetCountry().IsRebels())
            f12 = unit->GetCountry().IsPirates() ? kThreatDecayPirate : kThreatDecayRebel;
        float decay_mult = kHalf;
        if (!unit->GetCountry().IsNatives())
            decay_mult = f12;

        uint8_t depth = entry._nSteps;
        float mult = 1.0f;
        if (Eval.ShouldAssumeLessDistance(unit) && depth != 0)
        {
            depth -= 1;
            mult = decay_mult * kHalf + kHalf;
        }
        while (depth != 0)
        {
            mult *= decay_mult;
            depth -= 1;
        }

        if (entry._bCurrentlyBlocked & 1)
            mult *= kThreatFlagQuarter;

        Eval.EvaluateUnit(unit, acc, mult);
    }

    return Eval.CalcScore(acc);
}
