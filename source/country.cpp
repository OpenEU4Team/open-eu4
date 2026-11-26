#include "country.h"
#include "ai_agent.h"
#include "army.h"
#include "center_of_trade.h"
#include "automateddiplomatmanager.h"
#include "clausewitz/clausewitzlib/textutil.h"
#include "clausewitz/pdx_core/text.h"
#include "combat.h"
#include "construction.h"
#include "countrytag.h"
#include "defines.h"
#include "engine.h"
#include "enums.h"
#include "envoys.h"
#include "event.h"
#include "eventscope.h"
#include "flagship.h"
#include "gamestate.h"
#include "heir.h"
#include "leader.h"
#include "map.h"
#include "monarch.h"
#include "nativecountrydata.h"
#include "navy.h"
#include "province.h"
#include "queen.h"
#include "relation.h"
#include "rival.h"
#include "subjecttype.h"
#include "subunit.h"
#include "treatytag.h"
#include "unit.h"
#include "unitlist.h"
#include "unitmission.h"
#include "war.h"
#include "wargoal.h"
#include "wargoaltype.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

extern void UpdateScopeToTag(CEventScope &scope, CCountryTag NewTag, CCountryTag OldTag) asm("_ZN12_GLOBAL__N_116UpdateScopeToTagER11CEventScope11CCountryTagS2_");

void CCountry::SetCountryTag(CCountryTag tag, bool bCancelConstructions)
{
    (void)bCancelConstructions;
    const CCurrentGameState &game = *CCurrentGameState::GetInstance();

    if (this->GetCountryTag().CalcIsValid() && !CCountryTag::SameUpperWord(this->GetCountryTag(), tag))
        this->AddPreviousCountryTag(this->GetCountryTag());

    for (CUnit *unit : this->GetUnits())
    {
        unit->SetCountry(tag);
        unit->SetLocation(unit->GetLocation(), true);

        if (unit->InCombat())
        {
            const CCombatant *attacker = &unit->GetCombat()->GetAttacker();
            bool bFound = false;
            for (CUnit *engaged : attacker->GetUnits())
                if (engaged == unit)
                {
                    bFound = true;
                    break;
                }
            unit->OnCombatStart(bFound);
        }

        if (unit->GetAgent() != nullptr)
        {
            if (unit->IsArmy())
                unit->GetAgent()->DetachArmy(*unit->GetArmy());
            else if (unit->IsNavy())
                unit->GetAgent()->DetachFleet(*unit->GetNavy());
            unit->SetAgent(nullptr);
        }
    }

    this->VisitFlagship(
        [&tag](const std::unique_ptr<CFlagship> &flagship, const CSubUnit *) -> void
        {
            flagship->SetOriginalOwner(tag);
        });

    for (CLeader *leader : this->GetActiveLeaders())
        leader->SetCountry(tag);

    if (this->GetCurrentMonarch()->IsValid())
        this->GetCurrentMonarch()->SetCountry(tag);
    if (this->GetCurrentHeir()->IsValid())
        this->GetCurrentHeir()->SetCountry(tag);
    if (this->GetCurrentQueen()->IsValid())
        this->GetCurrentQueen()->SetCountry(tag);

    CProvince *const provinces = &game.GetProvince(0);
    const size_t provinceCount = game.GetProvinces().GetSize();
    const CEU3Date today = game.GetCurrentDate();

    if (provinceCount > 0)
    {
        for (size_t i = 0; i < provinceCount; ++i)
        {
            CProvince &p = provinces[i];

            const bool bOldObserver = this->GetCountryTag().GetCountryType() == CountryType::_COUNTRY_OBSERVER_;
            const bool bOldNull = !this->GetCountryTag().CalcIsValid();
            const bool bNewObserver = tag.GetCountryType() == CountryType::_COUNTRY_OBSERVER_;
            const bool bNewNull = !tag.CalcIsValid();
            const bool bOpen = p.IsTerraIncognitaActive();
            const bool bOldKnown = p.IsDiscoveredBy(this->GetCountryTag());
            const bool bNewKnown = p.IsDiscoveredBy(tag);

            if (((bOldObserver || bOldNull || !bOpen) || bOldKnown) &&
                (!bNewObserver && bOpen && !bNewNull && !bNewKnown))
            {
                p.Discover(tag, today, false, false, true, false);
            }

            if ((!bOldObserver && !bOldNull && bOpen && !bOldKnown) &&
                (bNewObserver || bNewNull || bNewKnown))
            {
                p.Discover(this->GetCountryTag(), today, false, false, true, false);
            }
        }

        for (size_t i = 0; i < provinceCount; ++i)
        {
            CProvince &p = provinces[i];
            for (CConstruction *c : p.GetConstructions())
            {
                if (CCountryTag::SameUpperWord(c->GetCountry(), this->GetCountryTag()))
                    c->SetCountry(tag);
            }
        }
    }

    const auto updatePart1 = [&]() -> void
    {
        CPdxArray<int, int> changingProvinces{};
        changingProvinces.Append(this->GetOwnedProvinces());
        for (int k = 0; k < changingProvinces.GetSize(); ++k)
        {
            CProvince &p = game.GetProvince(changingProvinces[k]);

            CProvince::SSetOwnerParameters params(tag, p.FindLastOwnerChange(this->GetCountryTag()),
                                                  true, true, false);
            p.SetOwner(params);
        }
    };
    updatePart1();

    for (CColonyConstruction *construction : this->GetCurrentColonyConstructions())
        construction->SetCountry(tag);

    const auto updatePart2 = [&]() -> void
    {
        for (int i = 0; i < COUNTRY_ENVOY_COUNT; ++i)
            this->AccessEnvoys(static_cast<EnvoyType>(i))->SetTag(tag);
    };
    updatePart2();

    this->_pAutomaticDiplomats->SetCountryTag(tag);

    for (auto node = this->GetControlledProvinces().GetTail(); node != this->GetControlledProvinces().end();)
    {
        auto prev = node--;
        const int provinceId = *prev;
        CProvince &p = game.GetProvince(provinceId);
        const CEU3Date lastChange = p.FindLastControllerChange(&p.GetHistory(), this->GetCountryTag());
        p.SetController(tag, lastChange, true,
                        g_sSetControllerToken);
    }

    const auto updatePart3 = [&]() -> void
    {
        const CCountry &newCountry =
            *CCountryDataBase::AccessInstance()->GetCountry(tag);
        if (newCountry.GetOwnedProvinces().GetSize() != 0 ||
            newCountry.GetCountryTag().GetCountryType() == CountryType::_COUNTRY_OBSERVER_)
        {
            for (int provinceId : this->GetCoreProvinces())
            {
                CProvince &p = game.GetProvince(provinceId);
                p.AddAsCore(tag, p.FindLastOwnerChange(this->GetCountryTag()), true);
            }
            for (int provinceId : this->GetClaimProvinces())
            {
                CProvince &p = game.GetProvince(provinceId);
                const bool bPermanent = p.HasPermanentClaim(this->GetCountryTag());
                p.AddAsClaim(tag, true, bPermanent, true);
            }
        }
    };
    updatePart3();

    for (CRival &rival : this->AccessRivals())
        if (CCountryTag::SameUpperWord(rival.GetCountry(), this->GetCountryTag()))
            rival.SetCountry(tag);

    for (int enemyIndex = 0; enemyIndex < this->GetEnemies().GetSize(); ++enemyIndex)
    {
        const CCountryTag &e = this->GetEnemies()[enemyIndex];
        CCountry &other =
            *CCountryDataBase::AccessInstance()->GetCountry(e);
        other.ReplaceEnemy(this->GetCountryTag(), tag);
    }

    if (CNativeCountryData *const nativeData = this->AccessNativeDataOrNullptr())
        nativeData->ChangeTag(tag);

    const CCountryTag OldTag = this->GetCountryTag();
    this->_Tag = tag;

    CCountryDataBase::AccessInstance()->GetCountry(OldTag)->UpdateEstates(false, true);
    CCountryDataBase::AccessInstance()->GetCountry(this->GetCountryTag())->UpdateEstates(false, true);

    const auto updatePart4 = [&]() -> void
    {
        CPdxArray<int, int> changingProvinces{};
        changingProvinces.Append(this->GetOwnedProvinces());
        for (int k = 0; k < changingProvinces.GetSize(); ++k)
            ::UpdateCenterOfTrade(game.GetProvince(changingProvinces[k]),
                                  *CCountryDataBase::AccessInstance()->GetCountry(OldTag),
                                  0, true, CCountryTag::FromRaw(kEmptyCountryTag));
    };
    updatePart4();

    const auto updatePart5 = [&]() -> void
    {
        const size_t oldSize = this->_DelayedEvents.GetSize();
        for (size_t i = 0; i < oldSize; ++i)
        {
            CDelayedEvent *const pEntry = this->_DelayedEvents[i];
            CEventScope scope(pEntry->GetScope());
            ::UpdateScopeToTag(scope, this->GetCountryTag(), OldTag);
            this->AddDelayedEvent(pEntry->GetEvent(), scope, pEntry->GetRemainingDays());
        }
        this->_DelayedEvents.RemoveRange(0, oldSize);
    };
    updatePart5();

    this->AccessStrategy().SetTag(this->GetCountryTag());
}

CFixedPoint CCountry::CalcDesperation(bool bIncludeSieges) const
{
    CFixedPoint out;
    if (!this->IsAtWar())
        return out;
    int numProvinces = this->GetOwnedProvinces().GetSize();
    if (numProvinces == 0)
        return out;

    const int *provinceIds = this->GetOwnedProvinces().begin();

    int occupiedValue = 0;
    int heldValue = 0;

    for (int i = 0; i < numProvinces; ++i)
    {
        CProvince *prov = &CMap::_pMap->GetProvince(provinceIds[i]);

        CFixedPoint warValueFP = prov->GetWarValue(true, nullptr, 0);
        int warValue = warValueFP.GetUnderlyingValue();

        const CArray<CCountryTag> &tags = prov->GetCores();
        bool weAreListed = std::find_if(tags.GetData(), tags.GetData() + tags.GetSize(),
                                        [this](const CCountryTag &t) -> bool
                                        { return t == this->GetCountryTag(); }) != tags.GetData() + tags.GetSize();
        warValue /= weAreListed ? kFixedPointResolution : 2 * kFixedPointResolution;

        if (prov->GetCountryState() == nullptr)
            continue;

        occupiedValue += warValue;

        if (!CCountryTag::SameUpperWord(prov->GetController(), prov->GetOwner()))
            continue;

        if (prov->GetSiege() == nullptr)
        {
            heldValue += warValue;
        }
        else if (!bIncludeSieges)
        {
            CFixedPoint statusFP = prov->GetSiegeStatus();
            int status = statusFP.GetUnderlyingValue() / kFixedPointResolution;
            int maxSiegeMemory = prov->GetSiege()->GetMaxSiegeMemory();
            heldValue += (warValue * status * kDesperationSiegeNumerator) / (maxSiegeMemory * kDesperationSiegeDenominator);
        }
    }

    if (occupiedValue > 0)
    {
        int heldPermille = ::Div1000Checked(static_cast<int64_t>(heldValue) * kFixedPointResolution,
                                            occupiedValue * kFixedPointResolution);
        out.SetUnderlyingValue(kFixedPointResolution - heldPermille);
    }
    return out;
}

bool CCountry::IsInImportantWar(bool bOnlyCritical, bool bIgnoreStrengthCheck, const CWar *pParticularWar) const
{
    if (!this->IsAtWar())
        return false;
    if (this->GetCurrentAtWarWith().GetSize() == 0)
        return false;

    bool important = false;

    for (CCountryTag enemyTag : this->GetCurrentAtWarWith())
    {

        const CWar *war = this->GetDiplomacy()[enemyTag.GetIndex()]->GetWar();

        if (pParticularWar != nullptr && war != pParticularWar)
            continue;
        if (war == nullptr)
            continue;
        if (this->DoesSubjectTypeProhibitWarWith(enemyTag, nullptr))
            continue;
        if (!(war->GetAttackers()->GetSize() > 0))
            continue;
        if (!(war->GetDefenders()->GetSize() > 0))
            continue;

        bool participant =
            CCountryTag::SameUpperWord(war->GetOriginalAttacker(), this->GetCountryTag()) ||
            CCountryTag::SameUpperWord((*war->GetAttackers())[0], this->GetCountryTag()) ||
            CCountryTag::SameUpperWord(war->GetOriginalDefender(), this->GetCountryTag()) ||
            CCountryTag::SameUpperWord((*war->GetDefenders())[0], this->GetCountryTag());

        important = participant && this->_IsFriend.test(enemyTag.GetIndex());

        if (!important && this->GetOverlord().CalcIsValid())
        {
            CFixedPoint libertyDesire = this->GetLibertyDesire(nullptr, 0, false);
            if (libertyDesire.GetUnderlyingValue() < NDefines::NCountry::HIGH_LIBERTY_DESIRE.GetUnderlyingValue() &&
                (CCountryTag::SameUpperWord(war->GetOriginalAttacker(), this->GetOverlord()) ||
                 CCountryTag::SameUpperWord((*war->GetAttackers())[0], this->GetOverlord()) ||
                 CCountryTag::SameUpperWord(war->GetOriginalDefender(), this->GetOverlord()) ||
                 CCountryTag::SameUpperWord((*war->GetDefenders())[0], this->GetOverlord())))
            {
                const CSubjectType *subjectType = this->GetSubjectType();
                if (subjectType != nullptr && subjectType->JoinsOverlordsWars())
                {
                    if (CCountryDataBase::AccessInstance()
                            ->GetCountry(enemyTag)
                            ->GetCapitalLocation()->GetColonialRegion() ==
                        this->GetCapitalLocation()->GetColonialRegion())
                    {
                        important = true;
                    }
                }
            }
        }

        if (!important &&
            CCountryTag::SameUpperWord(this->GetCoalitionTarget(), enemyTag) &&
            !this->GetOverlord().CalcIsValid())
        {
            important = true;
        }

        if (important && bOnlyCritical)
        {
            const CWarGoal *appliedGoal =
                war->GetAppliedWarGoal(enemyTag, this->GetCountryTag());
            const CWarGoalPeace *peace = &appliedGoal->GetType()->GetPeace(true);
            if (peace->AllowsPeaceOption(SPeaceTreatyTag(EStaticTreatyType::TREATY_TYPE_FORM_PERSONAL_UNION), nullptr))
            {
            }
            else if (peace->AllowsPeaceOption(SPeaceTreatyTag(EStaticTreatyType::TREATY_TYPE_BECOME_VASSAL), nullptr) ||
                     peace->AllowsPeaceOption(SPeaceTreatyTag(EStaticTreatyType::TREATY_TYPE_SUBJUGATE_VASSAL), nullptr) ||
                     peace->AllowsPeaceOption(SPeaceTreatyTag(EStaticTreatyType::TREATY_TYPE_CEDE_PROVINCE), nullptr))
            {
                important = this->IsSmallNation(enemyTag);
            }
            else
            {
                important = false;
            }
        }

        if (important)
            break;
    }

    if (!important)
        return false;

    if (this->GetCachedData().GetWarThreat().GetUnderlyingValue() > kImportantWarCounterHigh)
        return true;
    CFixedPoint desperation = this->CalcDesperation(false);
    if (desperation.GetUnderlyingValue() > kDesperationHigh)
        return true;
    if (bIgnoreStrengthCheck)
        return true;
    return false;
}

const CSubUnitDefinition *CCountry::GetSubunitTypePreferred(SubUnitType type) const
{
    return this->_SubUnitSelection[type];
}

int CCountry::CalcNominalArmySizeNotForAI() const
{
    int regiments = this->GetUnits().GetTotalNumOfRegiments(CCountryTag::FromRaw(kEmptyCountryTag));
    if (regiments > kNominalArmyRegimentCap)
        regiments = kNominalArmyRegimentCap;

    regiments = ::Div1000Checked(static_cast<int64_t>(regiments) * kFixedPointResolution, kNominalArmyDivisor);

    const int supplyLimit = this->GetTechnologyStatus()->GetSupplyLimit().GetUnderlyingValue();
    const int64_t scaled = static_cast<int64_t>(regiments * kNominalArmyWeight + kNominalArmyBase) * (supplyLimit + kFixedPointResolution);
    return ::Div1000(::Div1000(scaled));
}

bool CCountry::CanMergeUnits(CList<CUnit *> &list, CString *pDesc, bool bIgnoreMoveLockBlock) const
{
    auto appendError = [pDesc](const char *key) -> void
    {
        pDesc->AddNewlineIfNeeded();
        CString localized = ::PdxLocalizeAndReplace(key, nullptr, 0);
        CString text = ::CreateGreenRedText(localized, false);
        *pDesc += std::move(text);
    };

    if (list.GetSize() < 2)
    {
        if (pDesc == nullptr)
            return false;
        appendError("NO_REORGANISE_SINGLE_UNIT");
        return false;
    }

    CUnit *first = *list.begin();
    const CAutomaticFleetTransport *firstMission = first->GetAutoTransportMission();

    int nonNavy = 0;
    int mothballed = 0;
    int mercenary = 0;
    int nonExiled = 0;
    int exiled = 0;

    for (CUnit *unit : list)
    {

        if (!CCountryTag::SameUpperWord(unit->GetCountry(), this->GetCountryTag()))
        {
            if (pDesc == nullptr)
                return false;
            appendError("NO_MERGE_UNITS_OF_DIFFERENT_COUNTRIES");
            return false;
        }

        const bool isExiled = unit->HasExileStatus();
        if (unit->IsNavy() && unit->GetNavy()->IsMothballing())
            mothballed++;
        else
            nonNavy++;

        if (unit->GetLocation() != first->GetLocation())
        {
            if (pDesc == nullptr)
                return false;
            appendError("NO_REORGANISE_UNITS_DIFFERENT_LOCATION");
            return false;
        }

        if (unit->GetMercenaryCompany() == nullptr)
        {
            if (unit->GetAutoTransportMission() != firstMission)
            {
                if (pDesc == nullptr)
                    return false;
                appendError("CANT_SPLIT_WHILE_TRANSFER");
                return false;
            }
            if (!unit->CanBeSplitAndMerged(bIgnoreMoveLockBlock, pDesc))
                return false;

            if (unit->IsOnMission() && unit->IsNavy())
            {
                for (CUnit *other : list)
                {
                    if (other != unit &&
                        !unit->GetMission()->CanMergeWith(
                            other->GetMission()))
                    {
                        if (pDesc == nullptr)
                            return false;
                        appendError("UNIT_VIEW_CREATE_NEW_TRADE");
                        return false;
                    }
                }
            }
        }
        else
        {
            mercenary++;
        }

        nonExiled += isExiled ? 0 : 1;
        exiled += isExiled ? 1 : 0;
    }

    if (mothballed > 0 && nonNavy > 0)
    {
        if (pDesc == nullptr)
            return false;
        appendError("MERGE_MOTHBALLED");
        return false;
    }

    if (nonExiled != 0 && exiled != 0)
    {
        if (pDesc == nullptr)
            return false;
        appendError("UNIT_EXILE_DESC");
        return false;
    }

    if (list.GetSize() - mercenary > 1)
        return true;
    if (pDesc == nullptr)
        return false;
    appendError("CANT_MERGE_MERCENARY_COMPANY");
    return false;
}
