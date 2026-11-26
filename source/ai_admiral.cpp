#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>
#include <cstring>

#include "ai.h"
#include "unitmission.h"
#include "ai_strategy.h"
#include "countrydiplomacy.h"
#include "provincetemplate.h"
#include "ai_admiral.h"
#include "ai_army.h"
#include "ai_sharedeval.h"
#include "ai_king.h"
#include "ai_threatevaluation.h"
#include "unitcommands.h"
#include "country.h"
#include "country_cached_data.h"
#include "country_modifiers.h"
#include "countrytag.h"
#include "combat.h"
#include "gamestate.h"
#include "clausewitz/clausewitzlib/order.h"
#include "modifier.h"
#include "navy.h"
#include "pathfind.h"
#include "playeraiprefs.h"
#include "province.h"
#include "strength_factor_cache.h"
#include "subunit.h"
#include "relation.h"
#include "technology.h"
#include "unit.h"
#include "unitlist.h"
#include "war.h"
#include "engine.h"
#include "defines.h"

extern const CProvince *DetermineNextTarget(const CGameState &GameState, const CAIAdmiral &AIAdmiral, CNavy &Navy, const CCountry &Country, CCountryFlagCache<char> &AccessCache) asm("_ZN12_GLOBAL__N_119DetermineNextTargetERK10CGameStateRK10CAIAdmiralR5CNavyRK8CCountryR17CCountryFlagCacheIcE");

bool CAIAdmiral::AssignFleet(CNavy& Fleet)
{
    bool active = this->IsActive();
    if (active) {
        Fleet.SetAIBlockadeTarget(0);
        this->AcceptUnit(Fleet);
    }
    return active;
}

void CAIAdmiral::DetachFleet(CNavy& Fleet)
{
    auto found = this->_FleetList.end();
    for (auto it = this->_FleetList.begin(); it != this->_FleetList.end(); ++it) {
        if (*it == &Fleet) {
            found = it;
            break;
        }
    }

    this->_FleetList.Remove(found);

    Fleet.SetAIStraitTarget(0);
    Fleet.SetAIBlockadeTarget(0);
}

bool CAIAdmiral::IsTransportFleet(const CNavy* pNavy) const
{
    CNavy* first = nullptr;
    if (this->_FleetList.GetSize() >= 1)
        first = (this->_FleetList.GetSize() == 0 ? nullptr : (*this->_FleetList.begin())->GetNavy());
    if (first == pNavy && !this->UsesHomeFleetAsTransport())
        return false;

    int transports = CUnit::GetCount(SubUnitType::TRANSPORT, pNavy->GetSubUnits());
    return transports > 0;
}

bool CAIAdmiral::FleetCheckForTransportDeparture(const CNavy* pFleet)
{
    if (pFleet->GetLocation()->GetTemplate().IsLand()) {
        CFixedPoint avgStrength = pFleet->GetAverageStrength();
        if (avgStrength.GetUnderlyingValue() < kMinAvgStrength)
            return false;
        CFixedPoint avgMorale = pFleet->CUnit::GetAverageMorale(false);
        if (avgMorale.GetUnderlyingValue() < kMinAvgStrength)
            return false;
    }
    return true;
}

bool CAIAdmiral::DesiresMerge(const CNavy* pFleet, const CNavy* pOtherFleet,
                               bool& bFullMerge, int& nNeededHeavy,
                               int& nNeededLight, int& nNeededGalley,
                               int& nNeededTransport) const
{
    const CCountry *country = &this->GetCountry();
    auto mergeTemplate = [&]() -> bool
    {
        int navalMaint = country->GetTechnologyStatus()->GetNavalMaintenance().GetUnderlyingValue();
        CFixedPoint navalForceLimit = country->GetNavalForceLimit(nullptr);
        CFixedPoint seedFP((navalForceLimit.GetUnderlyingValue() / 100 + navalMaint) * 10 + 10000);
        int homeFleetSize = seedFP.GetRounded();

        int fleetTotals[static_cast<int>(SubUnitType::MAX)];
        for (int t = 0; t < static_cast<int>(SubUnitType::MAX); t++) {
            fleetTotals[t] = this->_FleetList.GetTotal(
                static_cast<SubUnitType>(t), CCountryTag::FromRaw(kEmptyCountryTag)).GetUnderlyingValue() / 1000;
        }
        if (!pFleet->IsOnMission()) {
            if (pFleet->IsMoving() || pOtherFleet->IsMoving()) {
                if (pFleet->GetFinalDestination() != pOtherFleet->GetFinalDestination())
                    return false;
            }
        }
        if (pFleet->IsOnTradeNodeMission(nullptr)) {
            if (pOtherFleet->GetMission()->CanMergeWith(pFleet->GetMission())) {
                bFullMerge = true;
                return true;
            }
            return false;
        }

        if ((this->_FleetList.GetSize() == 0 ? nullptr : (*this->_FleetList.begin())->GetNavy()) == pFleet)
            homeFleetSize = this->GetTargetHomeFleetSize();
        bool aIsTransport = this->IsTransportFleet(pFleet);
        int targetSize = static_cast<int>(homeFleetSize * kShipSizeScale);
        if (!aIsTransport)
            targetSize = homeFleetSize;
        int shipFraction = ((this->_FleetList.GetSize() == 0 ? nullptr : (*this->_FleetList.begin())->GetNavy()) != pFleet)
            ? NDefines::NAI::BIGSHIP_FRACTION.GetUnderlyingValue() : 1000;

        int desired[static_cast<int>(SubUnitType::MAX)];
        CFixedPoint lightFP(::Div1000(static_cast<int64_t>((1000 - shipFraction) * (targetSize * 1000))));
        CFixedPoint capitalFP(::Div1000(static_cast<int64_t>(shipFraction * (targetSize * 1000))));
        for (int t = 0; t < static_cast<int>(SubUnitType::MAX); t++) {
            desired[t] = 0;
            switch (t) {
            case static_cast<int>(SubUnitType::HEAVY_SHIP):
            case static_cast<int>(SubUnitType::GALLEY):
                desired[t] = capitalFP.GetRounded();
                break;
            case static_cast<int>(SubUnitType::LIGHT_SHIP):
                desired[t] = lightFP.GetRounded();
                break;
            case static_cast<int>(SubUnitType::TRANSPORT):
                if (aIsTransport)
                    desired[t] = fleetTotals[t];
                break;
            default:
                break;
            }
        }

        int currentA[static_cast<int>(SubUnitType::MAX)];
        int currentB[static_cast<int>(SubUnitType::MAX)];
        for (int t = 0; t < static_cast<int>(SubUnitType::MAX); t++)
            currentA[t] = CUnit::GetCount(static_cast<SubUnitType>(t), pFleet->GetSubUnits());
        for (int t = 0; t < static_cast<int>(SubUnitType::MAX); t++)
            currentB[t] = CUnit::GetCount(static_cast<SubUnitType>(t), pOtherFleet->GetSubUnits());
        int missing[static_cast<int>(SubUnitType::MAX)];
        for (int t = 0; t < static_cast<int>(SubUnitType::MAX); t++) {
            int m = desired[t] - currentA[t];
            missing[t] = (m < 0) ? 0 : m;
        }
        nNeededHeavy     = missing[static_cast<int>(SubUnitType::HEAVY_SHIP)];
        nNeededLight     = missing[static_cast<int>(SubUnitType::LIGHT_SHIP)];
        nNeededGalley    = missing[static_cast<int>(SubUnitType::GALLEY)];
        nNeededTransport = missing[static_cast<int>(SubUnitType::TRANSPORT)];

        for (int t = 0; t < static_cast<int>(SubUnitType::MAX); t++) {
            if (0 < currentB[t] && 0 < missing[t]) {
                bFullMerge = true;
                for (int u = 0; u < static_cast<int>(SubUnitType::MAX); u++) {
                    if (missing[u] < currentB[u]) {
                        bFullMerge = false;
                        return true;
                    }
                }
                return true;
            }
        }
        bFullMerge = false;
        return false;
    };
    if (pFleet == pOtherFleet)
        return false;
    if (pFleet->GetNumSubUnits() == 0 ||
        (pOtherFleet->GetNumSubUnits() == 0 && pOtherFleet->GetLocation() == pFleet->GetLocation()))
        return true;

    if (!(CCountryTag::SameUpperWord(pFleet->GetCountry(), pOtherFleet->GetCountry()) &&
          pOtherFleet->GetAgent() == this &&
          !this->IsExplorerFleet(pFleet) && !this->IsExplorerFleet(pOtherFleet)))
        return false;

    CNavy* firstFleet = (this->_FleetList.GetSize() < 1) ? nullptr : (*this->_FleetList.begin())->GetNavy();

    bool aIsTransport = this->IsTransportFleet(pFleet);
    if (firstFleet == pFleet) {
        if (this->IsTransportFleet(pOtherFleet) && !this->UsesHomeFleetAsTransport())
            return false;
    } else if (aIsTransport) {
        if (firstFleet == pOtherFleet) {
        if (this->UsesHomeFleetAsTransport()) {
            bFullMerge = true;
            return true;
        }
        return false;
        }
    }
    if (pOtherFleet->GetCurrentBlockadeCapacity().GetUnderlyingValue() >= 0) {
        int it = 0;
        int shipEnd = pOtherFleet->GetBlockadedProvinces().GetSize();
        if (it != shipEnd) {
            for (; it != shipEnd; ++it) {
                if (!(pOtherFleet->GetBlockadedProvinces()[it]->GetBlockadeProgress().GetUnderlyingValue() == 1000))
                    break;
            }
            if (it == shipEnd)
                return false;
        }
    }

    const auto canMergeMission = [&]() -> bool
    {
        if (!pFleet->IsMoving() && !pOtherFleet->IsMoving()) {
            bFullMerge = true;
            return true;
        }
        return false;
    };
    if (pFleet->GetAIStraitTarget() == 0) {
        if (pFleet->GetAIBlockadeTarget() == 0) {
            if (pOtherFleet->GetAIBlockadeTarget() == 0 && pOtherFleet->GetAIStraitTarget() == 0) {
                return mergeTemplate();
            }
        } else if (pOtherFleet->GetAIBlockadeTarget() != 0) {
            return canMergeMission();
        }
    } else if (pOtherFleet->GetAIStraitTarget() == pFleet->GetAIStraitTarget()) {
        return canMergeMission();
    }
    return false;
}

void CAIAdmiral::MergeFleets()
{
    auto head = this->GetFleetList()->begin();
    if (head == this->GetFleetList()->end())
        return;

    while(true) {
        CNavy* fleetA = static_cast<CNavy*>(*head);
        auto nextHead = head;
        ++nextHead;

        auto mergeCandidates = [&]() -> void
        {
            CUnitList mergeGroup;

            for (auto cell = head; cell != this->GetFleetList()->end(); ++cell) {
                CNavy* fleetB = static_cast<CNavy*>(*cell);
                int state = 4;

                if (fleetB->GetLocation() == fleetA->GetLocation()) {
                    bool compatible = false;
                    int missingHeavy = 0, missingLight = 0, missingGalley = 0, missingTransport = 0;
                    if (this->DesiresMerge(fleetA, fleetB, compatible, missingHeavy, missingLight,
                                    missingGalley, missingTransport)) {
                        bool viaTransfer = false;
                        if (this->GetFleetList()->begin() == this->GetFleetList()->end()) {
                            viaTransfer = true;
                        } else {
                            int indexA = -1, indexB = -1;
                            int i = 0;
                            for (auto it = this->GetFleetList()->begin(); it != this->GetFleetList()->end(); ++it, i++) {
                                if (static_cast<CNavy*>(*it) == fleetA) { indexA = i; break; }
                            }
                            i = 0;
                            for (auto it = this->GetFleetList()->begin(); it != this->GetFleetList()->end(); ++it, i++) {
                                if (static_cast<CNavy*>(*it) == fleetB) { indexB = i; break; }
                            }
                            if (indexA <= indexB) {
                                viaTransfer = true;
                            } else if (!compatible) {
                                continue;
                            } else {
                                bool bCompatible = false;
                                if (this->DesiresMerge(fleetB, fleetA, bCompatible, missingHeavy,
                                                missingLight, missingGalley, missingTransport)) {
                                } else {
                                    mergeGroup.Append(static_cast<CUnit*>(fleetB));
                                    state = 5;
                                }
                            }
                        }
                        if (viaTransfer) {
                            if (!compatible) {
                                int missing[7] = {0, 0, 0, missingHeavy, missingLight,
                                                missingGalley, missingTransport};
                                for (int t = 0; t < 7; t++) {
                                    int needed = missing[t];
                                    if (needed != 0) {
                                        CArray<CSubUnit*> subs = fleetB->GetSubUnits();
                                        for (int subIndex = 0; subIndex < subs.GetSize(); ++subIndex) {
                                            CSubUnit* sub = subs[subIndex];
                                            if (sub != nullptr && 0 < needed &&
                                                static_cast<SubUnitType>(t) == sub->GetDefinition()->GetType())
                                            {
                                                PostAICommand(new CTransferSubUnitCommand(fleetA->GetID(), sub));
                                                needed--;
                                                missing[t] = needed;
                                            }
                                        }
                                    }
                                }
                                state = 5;
                            } else {
                                mergeGroup.Append(static_cast<CUnit*>(fleetB));
                                state = 5;
                            }
                        }
                    }
                }
                if (state != 4) break;
            }

            if (0 < mergeGroup.GetSize()) {
                this->MergeTheseUnits(*fleetA, &mergeGroup, false);
            }

        };

        mergeCandidates();

        if (nextHead == this->GetFleetList()->end())
            return;
        head = this->GetFleetList()->begin();
    }
}

bool CAIAdmiral::UsesHomeFleetAsTransport() const
{
    CCountry* country = &this->GetCountry();
    int width = country->GetNavalEngagementWidth(nullptr).GetUnderlyingValue();
    if (country->IsAtWar())
        width = width << 1;
    return this->GetTargetHomeFleetSize() < width / 1000;
}

void CAIAdmiral::AssignAndTrimHomeFleet(CCountryFlagCache<char>& AccessCache)
{
    bool latch = this->_bAssignedHomeFleet;

    if (!latch && this->GetFleetList()->GetSize() > 0) {
        CUnit*  chosen         = nullptr;
        int     bestNoT        = 0;
        int     bestWithT      = 0;
        CUnit*  bestWithTFleet = nullptr;
        CUnit*  bestNoTFleet   = nullptr;
        for (CUnit* fleet : *this->GetFleetList()) {
            if (!fleet->IsNavy())
                continue;
            CNavy* navy = fleet->GetNavy();
            if (navy->GetUnitsOnBoard().GetSize() > 0)
                continue;
            if (navy->GetAIStraitTarget() != 0)
                continue;
            int galleyCount = CUnit::GetCount(SubUnitType::GALLEY, fleet->GetSubUnits());
            int heavyCount  = CUnit::GetCount(SubUnitType::HEAVY_SHIP, fleet->GetSubUnits());
            int strength = heavyCount + galleyCount;
            int transports = CUnit::GetCount(SubUnitType::TRANSPORT, navy->GetSubUnits());
            if (transports > 0) {
                if (strength > bestWithT) {
                    bestWithTFleet = fleet;
                    bestWithT = strength;
                }
            } else if (strength > bestNoT) {
                chosen = fleet;
                bestNoTFleet = fleet;
                bestNoT = strength;
            }
        }
        chosen = bestNoTFleet;
        if (chosen == nullptr) {
            chosen = bestWithTFleet;
            if (chosen == nullptr)
                return;
        }

        auto promoteHomeFleet = [&]() -> void
        {
            auto found = this->GetFleetList()->end();
            for (auto it = this->GetFleetList()->begin(); it != this->GetFleetList()->end(); ++it) {
                if (*it == chosen) {
                    found = it;
                    break;
                }
            }
            this->GetFleetList()->Remove(found);
            this->GetFleetList()->Insert(chosen);
        };

        promoteHomeFleet();
        this->_bAssignedHomeFleet = 1;

        if (chosen->IsMoving()) {
            CProvince* destination = chosen->GetFinalDestination();
            CFixedPoint navalDist = destination->GetNavalDistance(chosen->GetLocation()->GetProvinceID());
            if (navalDist.GetUnderlyingValue() >= kHomeFleetStopDistance) {
                if (chosen->IsMovementLocked()) {
                    CNavy* navy = chosen->GetNavy();
                    CProvince* nextTarget = chosen->GetNextTarget();
                    this->MoveFleet(*navy, nextTarget->GetProvinceID(), AccessCache);
                } else {
                    CNavy* navy = chosen->GetNavy();
                    this->StopFleet(*navy, EStopMissionReason::NeededForHomeFleet);
                }
            }
        }
        return;
    }

    if (this->GetFleetList()->GetSize() > 0) {
        CUnit* firstFleet = *this->GetFleetList()->begin();
        CNavy* navy = firstFleet->GetNavy();
        if (navy != nullptr && !this->UsesHomeFleetAsTransport()) {
            if (CUnit::GetCount(SubUnitType::TRANSPORT, navy->GetSubUnits()) > 0) {
                CID cid = CCurrentGameState::AccessInstance()->GenerateCID(EIDCategory::UNIT, false);
                CAIKing* king = &this->GetKing();
                if (king->CreateFleet(cid, navy->GetLocation(), false)) {
                    CArray<CSubUnit*> subs = navy->GetSubUnits();
                    for (int subunitIndex = 0; subunitIndex < subs.GetSize(); ++subunitIndex) {
                        CSubUnit* subunit = subs[subunitIndex];
                        if ((subunit->GetDefinition()->GetType() == SubUnitType::TRANSPORT)) {
                            CAIKing* king2 = &this->GetKing();
                            king2->AssignSubUnitProperly(cid, subunit);
                        }
                    }
                }
            }
        }
    }
}

void CAIAdmiral::NotifyTick(CCountryFlagCache<char>& AccessCache)
{
    if (!this->IsActive())
        return;

    CCountry* country = &this->GetCountry();

    if (country->GetNumOfTotalPorts() > 0) {
        if (country->GetTotalShips().GetUnderlyingValue() > 0 && this->GetFleetList()->GetTotalSize() > 0) {
            this->AssignAndTrimHomeFleet(AccessCache);
            this->UpdateAllowingBlockadeFleets();
            this->CalcBases();
            this->AssignBlockadeFleets(AccessCache);
            this->ClearEmptyFleets(CPdxArray<CUnit*, int>());
            this->MergeFleets();
            this->UpdateFleets(AccessCache);
            int stride = kWarTickStride;
            if (!country->IsAtWar())
                stride = this->GetRandom() % kPeaceTickStride + 1;
            this->Tick(stride);
        } else {
            this->Tick(kNoPortTick);
        }
    } else {
        this->Tick(kNoPortTick);
        if (country->GetNumOfTotalPorts() == 0) {
            for (CUnit* fleet : *this->GetFleetList()) {
                CDisbandCommand* cmd = new CDisbandCommand(fleet, false);
                PostAICommandOnlyIfValid(std::unique_ptr<CCommand>(cmd));
            }
        }
    }
    this->ClearEmptyFleets(CPdxArray<CUnit*, int>());
}

void CAIAdmiral::UpdateFleets(CCountryFlagCache<char>& AccessCache)
{
    if (this->GetFleetList()->GetSize() == 0)
        return;

    CCountry* country = &this->GetCountry();

    for (CUnit* unit : *this->GetFleetList()) {
        if (!unit->IsNavy())
            continue;
        if (!unit->GetLocation()->GetTemplate().IsLand())
            continue;
        if (unit->IsMoving())
            continue;

        for (CUnit* boarded : static_cast<CNavy*>(unit)->GetUnitsOnBoard()) {
            CUnloadArmyCommand* cmd = new CUnloadArmyCommand(boarded);
            PostAICommand(std::unique_ptr<CCommand>(cmd));
        }
    }

    if (country->GetStrategy().GetDefendedHomeStrait() > 0 && this->GetRandom() % kStraitDefenderChance == 1)
        this->AssignStraitDefenders();

    bool anyDetached = false;
    CFixedPointFPML straitFirst{0}, straitSecond{0};
    bool straitCalculated = false;
    CNavy* straitNavy = nullptr;
    for (CUnit* unit : *this->GetFleetList()) {
        CNavy* fleet = static_cast<CNavy*>(unit);

        if (fleet->IsRetreating() || fleet->IsExploring())
            continue;

        if (fleet->GetAttachedTo() == nullptr) {
            if (fleet->InCombat()) {
                if (fleet->GetCombat()->GetCombatType() &&
                    ::CheckShouldNavyRetreat(fleet)) {
                    CProvince* retreat = fleet->GetRetreatProvince(false);
                    if (retreat != nullptr)
                        this->MoveFleet(*fleet, retreat->GetProvinceID(), AccessCache);
                }
                continue;
            }
            if (fleet->IsMoving()) {
                if (!this->IsSafeMove(fleet, fleet->GetNextTarget())) {
                    CFixedPoint score = this->EvaluateProvince(
                        fleet, fleet->GetNextTarget(), CFixedPointFPML{0}, &AccessCache, nullptr);
                    if (score.GetUnderlyingValue() == kNavalEvalHalt) {
                        this->HaltUnit(*fleet, EStopMissionReason::HostilesClose);
                        continue;
                    }
                }
            }

            if (!fleet->IsMoving() &&
                this->ShouldRaidForSlaves(fleet, fleet->GetLocation(), false)) {
                CPerformSlaveRaidCommand* cmd = new CPerformSlaveRaidCommand(fleet);
                PostAICommand(cmd);
                continue;
            }
            if (this->IsExplorerFleet(fleet) &&
                CCurrentGameState::GetInstance()->UsingNewExplorationSystem() &&
                NDefines::NCountry::MIN_SHIPS_TO_EXPLORE < fleet->GetExplorationShipsCount()) {
                this->TrimExplorationFleet(fleet);
                continue;
            }
            if (fleet->GetAIBlockadeTarget() != 0) {
                if (this->AllowingBlockadeFleets()) {
                    if ((this->GetFleetList()->GetSize() == 0 ? nullptr : (*this->GetFleetList()->begin())->GetNavy()) != fleet) {
                    } else {
                        fleet->SetAIBlockadeTarget(0);
                        this->HaltUnit(*fleet, EStopMissionReason::CannotBlockade);
                        continue;
                    }
                } else {
                    fleet->SetAIBlockadeTarget(0);
                    this->HaltUnit(*fleet, EStopMissionReason::CannotBlockade);
                    continue;
                }
            }


            if (fleet->GetAIStraitTarget() != 0) {
                int warId = country->GetStrategy().GetDefendedHomeStrait();
                if (fleet->GetAIStraitTarget() != warId) {
                    fleet->SetAIStraitTarget(0);
                } else {
                    CNavy* first = (this->GetFleetList()->GetSize() < 1) ? nullptr : (*this->GetFleetList()->begin())->GetNavy();
                    if (first == fleet) {
                        fleet->SetAIStraitTarget(0);
                    } else if (!fleet->IsDamaged()) {
                        if (!anyDetached && fleet->GetLocation()->GetProvinceID() == warId) {
                            if (!straitCalculated) {
                                straitNavy = nullptr;
                                this->CalcStraitDefenders(straitFirst, straitSecond,
                                                          straitNavy);
                                straitCalculated = true;
                            }
                            int64_t diff = ::RawFPML(straitSecond) - ::RawFPML(straitFirst);
                            if (diff > 0 || fleet->GetNumTransports() > 0) {
                                int64_t adjust = (diff * kStraitStrengthAdjust) >> 15;
                                CFixedPoint auxOut;
                                CFixedPointFPML strength = fleet->GetNavyStrengthFactor(
                                    CCountryDataBase::AccessInstance()
                                        ->GetCountry(fleet->GetCountry())->GetCachedData().GetModifiers().GetModifier(),
                                    false, true, fleet->GetLocation()->GetTemplate().GetSeaType(),
                                    auxOut, nullptr, SubUnitType::MAX, nullptr);
                                if (0 < adjust)
                                    strength -= ::FPMLFromRaw(adjust);
                                CFixedPointFPML downsizeStrength = strength;
                                if (this->DownsizeFleetToBlockadeCapacityAndStrength(
                                        fleet, fleet->GetLocation(), CFixedPoint{},
                                        downsizeStrength, true)) {
                                    anyDetached = true;
                                    continue;
                                }
                            }
                        }
                    } else {
                        const CPlayerAiPrefs* prefs = &country->GetPlayerAiPrefs();
                        bool detach = false;
                        if (!prefs->IsEnabled()) {
                            if (fleet->GetLocation()->GetProvinceID() == warId)
                                detach = true;
                        } else if (fleet->GetLocation()->GetProvinceID() == warId &&
                                   prefs->IsChangeFleetMissions()) {
                            detach = true;
                        }
                        if (detach && country->CanDetachDamaged(fleet)) {
                            CDetachDamagedCommand* cmd = new CDetachDamagedCommand(fleet);
                            PostAICommand(cmd);
                            anyDetached = true;
                            continue;
                        }
                        fleet->SetAIStraitTarget(0);
                        anyDetached = true;
                    }
                }
            }

            CUnit* attachTarget = this->FindNavyToAttachTo(fleet, fleet->GetLocation());
            if (attachTarget != nullptr && !attachTarget->IsMovementLocked()) {
                if (this->GetRandom() % kAttachChanceMod == 0) {
                    CAttachUnitCommand* cmd = new CAttachUnitCommand(fleet, attachTarget);
                    ::PostAICommand(cmd);
                }
                return;
            }

            const CProvince* nextTarget = ::DetermineNextTarget(
                *CCurrentGameState::GetInstance(), *this, *fleet, *country, AccessCache);
            if (nextTarget != nullptr)
                this->MoveFleet(*fleet, nextTarget->GetProvinceID(), AccessCache);
        } else {
            CUnit* attachTarget = this->FindNavyToAttachTo(fleet, fleet->GetLocation());
            if (attachTarget != nullptr)
                continue;
            CUnit* mission = fleet->GetAttachedTo();
            if (mission->IsMoving() && mission->GetLocation() == fleet->GetLocation())
                continue;
            CDetachUnitCommand* cmd = new CDetachUnitCommand(fleet);
            PostAICommand(cmd);
        }
    }
}

AIMoveResult CAIAdmiral::MoveFleet(CNavy& Fleet, int nTarget, CCountryFlagCache<char>& AccessCache)
{
    if (nTarget == 0)
        return AIMoveResult::_AI_MOVE_RESULT_NOTNEEDED_;

    int currentId = Fleet.GetLocation()->GetProvinceID();
    bool moving = Fleet.IsMoving();
    if (currentId == nTarget) {
        if (moving)
            this->HaltUnit(Fleet, EStopMissionReason::Arrived);
    } else {
        if (moving && Fleet.GetFinalDestination()->GetProvinceID() == nTarget &&
            !Fleet.InCombat())
            return AIMoveResult::_AI_MOVE_RESULT_NOTNEEDED_;

        CProvince* target = &CCurrentGameState::GetInstance()->GetProvince(nTarget);
        if (Fleet.IsLegalToMove(target)) {
            if (Fleet.CanFindPathTo(target, &AccessCache, false)) {
                return CAIAgent::MoveFleet(Fleet, *target, AccessCache, 0);
            }
            return AIMoveResult::_AI_MOVE_RESULT_NOPATH_;
        }
        return AIMoveResult::_AI_MOVE_RESULT_CANTMOVE_;
    }
    return AIMoveResult::_AI_MOVE_RESULT_NOTNEEDED_;
}

CUnit* CAIAdmiral::FindNavyToAttachTo(const CNavy* pNavy, const CProvince* pProv)
{
    CCountry* country = CCountryDataBase::AccessInstance()->GetCountry(pNavy->GetCountry());

    if ((!country->IsAtWar() && !country->IsSubject()) ||
        pNavy->GetAttachedUnits().GetSize() > 0 ||
        NSubjectBehavior::WantsToAvoidAttach(pNavy))
        return nullptr;

    CArray<CWar*> wars;
    country->GetCurrentWars(wars);
    const auto passesLibertyGate = [&]() -> bool
    {
        CCountry* admCountry = &this->GetCountry();
        for (int warIndex = 0; warIndex < wars.GetSize(); ++warIndex) {
                CWar* war = wars[warIndex];
            CCountryTag overlord = admCountry->GetTopOverlord(true);
            if (war != nullptr)
                overlord = war->GetTopOverlordInWarTogether(this->GetCountryTag());
            if (overlord.IsValid()) {
                CFixedPoint limit(NDefines::NCountry::HIGH_LIBERTY_DESIRE);
                if (!admCountry->IsLibertyDesireChainBelowLimit(limit, overlord))
                    return false;
            }
        }
        return true;
    };
    if (!passesLibertyGate())
        return nullptr;

    CUnit* best = nullptr;
    for (CUnit* unit : pProv->GetUnits()) {
        if (unit == pNavy || !unit->IsNavy())
            continue;
        if (CCurrentGameState::GetInstance()->IsControllerAI(unit->GetCountry()) ||
            unit->IsOnMission())
            continue;

        if (best != nullptr) {
            uint32_t candCat = unit->GetCountry().GetCategoryWord();
            uint32_t bestCat = best->GetCountry().GetCategoryWord();
            const CCountry* admCountry = &this->GetCountry();
            uint32_t overlordCat = admCountry->GetOverlord().GetCategoryWord();
            if (bestCat != candCat && overlordCat != candCat &&
                pNavy->GetAttachedTo() != unit)
                continue;
        }

        CCountryTag candTag = unit->GetCountry();
        if (!(candTag.IsValid() &&
              candTag.GetCountryType() != CountryType::_COUNTRY_OBSERVER_ &&
              !candTag.IsRebelPirateNative()
            ))
            continue;

        uint32_t candCat = candTag.GetCategoryWord();

        auto sharesWarOrCountry = [&]() -> bool
        {
            const CCountry* admCountry = &this->GetCountry();
            uint32_t admCat = this->GetCountryTag().GetCategoryWord();
            uint32_t overlordCat = admCountry->GetOverlord().GetCategoryWord();
            if (!country->IsWarFriend(candTag, CCountryTag::FromRaw(kEmptyCountryTag)) &&
                admCat != candCat && overlordCat != candCat)
                return false;

            return true;
        };

        auto passesOverlordThreatGate = [&]() -> bool
        {
            const CCountry* admCountry = &this->GetCountry();
            uint32_t overlordCat = admCountry->GetOverlord().GetCategoryWord();
            if (!country->IsWarFriend(candTag, CCountryTag::FromRaw(kEmptyCountryTag)) &&
                overlordCat == candCat) {
                CCountry* overlordCountry = CCountryDataBase::AccessInstance()->GetCountry(candTag);
                if (overlordCountry->GetRebelThreat(false) < 1)
                    return false;
            }

            return true;
        };

        if (!sharesWarOrCountry())
            continue;
        if (!passesOverlordThreatGate())
            continue;

        if (!unit->CanAttachTo())
            continue;

        auto passesCandidateLibertyGate = [&]() -> bool
        {
            CCountry* fleetCountry = CCountryDataBase::AccessInstance()->GetCountry(pNavy->GetCountry());
            CCountry* admCountry = &this->GetCountry();
            bool libertyOk = true;
            for (int warIndex = 0; warIndex < wars.GetSize(); ++warIndex) {
                CWar* war = wars[warIndex];
                CCountryTag overlord = admCountry->GetTopOverlord(true);
                if (war != nullptr)
                    overlord = war->GetTopOverlordInWarTogether(this->GetCountryTag());
                if (overlord.IsValid()) {
                    CFixedPoint limit(NDefines::NCountry::HIGH_LIBERTY_DESIRE);
                    if (!fleetCountry->IsLibertyDesireChainBelowLimit(limit, overlord)) {
                        libertyOk = false;
                        break;
                    }
                }
            }
            if (!libertyOk)
                return false;

            return true;
        };

        if (!passesCandidateLibertyGate())
            continue;

        bool threatFlag = false;
        if (!country->GetOverlord().IsValid() ||
            country->GetSubjectMilitaryFocus() != ESubjectMilitaryFocus::_FOCUS_SUPPORTIVE_) {
            if (pNavy->GetAttachedTo() == unit) {
                threatFlag = true;
            } else {
                CStrengthFactorCache& cache = CStrengthFactorCacheManager::AccessCache();
                CEU3Date horizon = CCurrentGameState::GetInstance()->GetCurrentDate();
                horizon.SetUniversalDate(horizon.GetUniversalDate() + 0x960);
                const CAIThreatEvaluation& threat = CAIThreatEvaluation::GetInstance();
                int provId = unit->GetRealLocation()->GetProvinceID();
                CNavy* candNavy = unit->GetNavy();
                CFixedPoint threatVal = threat.CalcThreatForNavy(
                    provId, horizon, candNavy, true, cache);
                float ratio = threatVal.GetUnderlyingValue() / 1000.0f;
                threatFlag = kOne < ratio;

                if (CCurrentGameState::GetInstance()->IsControllerAI(unit->GetCountry())) {
                    if (kAttachThreatFloor > ratio || ratio > kFortDivisor)
                        continue;
                } else {
                    if (0.0f >= ratio)
                        continue;
                }
            }
        } else {
            threatFlag = true;
            const CCountry* admCountry = &this->GetCountry();
            uint32_t overlordCat = admCountry->GetOverlord().GetCategoryWord();
            if (candCat != overlordCat)
                continue;
        }

        bool haveWars = true;

        auto checkEnemyWars = [&]() -> void
        {
            CCountry* candCountry = CCountryDataBase::AccessInstance()->GetCountry(candTag);
            for (CCountryTag enemy : country->GetCurrentAtWarWith()) {
                int enemyNumber = enemy.GetIndex();
                const CDiplomacyStatus* status =
                    candCountry->GetDiplomacy()[enemyNumber];
                if (status->GetWar() == nullptr) {
                    haveWars = false;
                    break;
                }
            }
        };
        checkEnemyWars();

        if (!haveWars)
            continue;

        CNavy* candNavy = unit->GetNavy();
        if (CCurrentGameState::GetInstance()->IsControllerAI(unit->GetCountry())) {
            CNavy* firstFleet = (this->GetFleetList()->GetSize() < 1) ? nullptr
                                 : (this->GetFleetList()->GetSize() == 0  ? nullptr
                                 : (*this->GetFleetList()->begin())->GetNavy());
            if (firstFleet != candNavy)
                continue;
        }

        auto selectCandidate = [&]() -> bool
        {
            const CCountry* admCountry = &this->GetCountry();
            uint32_t admCat = this->GetCountryTag().GetCategoryWord();
            if (!threatFlag) {
                if (admCat == candCat)
                    return false;

                if (this->IsExplorerFleet(candNavy))
                    return false;
            }
            bool fleetAInvolved = false;
            bool candInvolved = false;
            for (CCountryTag enemy : country->GetCurrentAtWarWith()) {
                int enemyNumber = enemy.GetIndex();
                const CDiplomacyStatus* status =
                    admCountry->GetDiplomacy()[enemyNumber];
                const CWar* war = status->GetWar();
                if (war == nullptr)
                    return false;
                if (CCountryTag::SameUpperWord(war->GetOriginalAttacker(), pNavy->GetCountry()) ||
                    CCountryTag::SameUpperWord(war->GetOriginalDefender(), pNavy->GetCountry()))
                    fleetAInvolved = true;
                const CArray<CCountryTag>* attackers = war->GetAttackers();
                if (attackers != nullptr && attackers->GetSize() >= 1 &&
                    CCountryTag::SameUpperWord((*attackers)[0], pNavy->GetCountry()))
                    fleetAInvolved = true;
                const CArray<CCountryTag>* defenders = war->GetDefenders();
                if (defenders != nullptr && defenders->GetSize() >= 1 &&
                    CCountryTag::SameUpperWord((*defenders)[0], pNavy->GetCountry()))
                    fleetAInvolved = true;
                if (!CCountryTag::SameUpperWord(war->GetOriginalAttacker(), candTag) &&
                    !CCountryTag::SameUpperWord(war->GetOriginalDefender(), candTag)) {
                    bool notInvolved = true;
                    if (attackers != nullptr && attackers->GetSize() >= 1 &&
                        CCountryTag::SameUpperWord((*attackers)[0], candTag))
                        notInvolved = false;
                    if (notInvolved && defenders != nullptr && defenders->GetSize() >= 1 &&
                        CCountryTag::SameUpperWord((*defenders)[0], candTag))
                        notInvolved = false;
                    if (notInvolved)
                        return false;
                }
                candInvolved = true;
            }

            int candCount = unit->GetNumSubUnits();
            int fleetACount = pNavy->GetNumSubUnits();
            bool sameCountry = (admCat == candCat);
            bool keepBest = false;
            if (!sameCountry) {
                if (fleetAInvolved)
                    keepBest = true;
                else if (!candInvolved && candCount <= fleetACount)
                    keepBest = true;
            }
            if (!keepBest) {
                CUnit* mission = pNavy->GetAttachedTo();
                if (mission == unit || mission == nullptr) {
                    best = unit;
                } else if (mission != best) {
                    best = unit;
                } else {
                    CNavy* bestNavy = best->GetNavy();
                    CNavy* first = (this->GetFleetList()->GetSize() < 1) ? nullptr
                                     : (this->GetFleetList()->GetSize() == 0 ? nullptr
                                     : (*this->GetFleetList()->begin())->GetNavy());
                    if (first != bestNavy &&
                        pNavy->GetAttachedTo() != best) {
                        CNavy* candNavy2 = unit->GetNavy();
                        CNavy* first2 = (this->GetFleetList()->GetSize() < 1) ? nullptr
                                                                  : (this->GetFleetList()->GetSize() == 0 ? nullptr : (*this->GetFleetList()->begin())->GetNavy());
                        if (first2 == candNavy2)
                            best = unit;
                    }
                }
            }

            return true;
        };

        if (!selectCandidate())
            continue;
    }

    return best;
}

void NAssignToArmy::UseFleet(CAIKing* pAiKing, CAIArmy* pArmyAgent, SAvailableFleet& Fleet,
                             CFixedPointFPML& vAssignedStrength, int& nRemainingTransports,
                             const CFixedPointFPML& vMatchStrength, int& nAssignedTransports)
{
    if (Fleet._pFleet->IsOnMission()) {
        CStopUnitMissionCommand* cmd =
            new CStopUnitMissionCommand(Fleet._pFleet, EStopMissionReason::NeededForTransportation,
                                        pArmyAgent->GetID());
        PostAICommand(cmd);
    }
    Fleet._pFleet->SetAIStraitTarget(0);

    vAssignedStrength      += Fleet._Strength;
    nRemainingTransports     -= Fleet._Transports;
    nAssignedTransports       += Fleet._Transports;

    CFixedPointFPML splitStrength{0};
    int   splitTransports = 0;
    CArray<CSubUnit*> subset;
    for (int subIndex = 0; subIndex < Fleet._pFleet->GetSubUnits().GetSize(); ++subIndex) {
        CSubUnit* sub = Fleet._pFleet->GetSubUnits()[subIndex];
        if (vAssignedStrength <= vMatchStrength && nRemainingTransports >= 0)
            break;

        CArray<CSubUnit*> single;
        single.Append(sub);
        CFixedPoint subAuxOut;
        CFixedPointFPML subStrength = Fleet._pFleet->GetNavyStrengthFactor(
            CCountryDataBase::AccessInstance()
                ->GetCountry(Fleet._pFleet->GetCountry())->GetCachedData().GetModifiers().GetModifier(),
            false, true, ESeaType::MAX, subAuxOut,
            nullptr, SubUnitType::MAX, &single);

        if (vMatchStrength < (vAssignedStrength - splitStrength) - subStrength) {
            int transports = (sub->GetDefinition()->GetType() == SubUnitType::TRANSPORT) ? 1 : 0;
            if (nRemainingTransports + (splitTransports + transports) < 1) {
                subset.Append(sub);
                splitStrength      += subStrength;
                splitTransports    += transports;
            }
        }
    }

    if ((::RawFPML(Fleet._Strength) * kFleetSplitKeepNum >> 15) < ::RawFPML(splitStrength) ||
        Fleet._Transports < splitTransports * 3) {
        CID cid = CCurrentGameState::AccessInstance()->GenerateCID(EIDCategory::UNIT, false);
        if (pAiKing->CreateFleet(cid, Fleet._pFleet->GetLocation(), false)) {
            vAssignedStrength -= splitStrength;
            nRemainingTransports    += splitTransports;
            for (int subIndex = 0; subIndex < subset.GetSize(); ++subIndex)
                pAiKing->AssignSubUnitProperly(cid, subset[subIndex]);
        }
    }

    pArmyAgent->AssignFleet(*Fleet._pFleet);
}

std::pair<int, CFixedPointFPML> CAIAdmiral::AssignToArmyAgent(
    CAIArmy* pArmyAgent, int nNeededTransports, int nWantedTransports,
    CFixedPointFPML vNeededStrength, CFixedPointFPML vWantedStrength,
    CCountryFlagCache<char>& AccessCache, const CProvince* pLoadProvince, const CProvince* pOffloadProvince)
{
    CCountry* country = &this->GetCountry();
    std::pair<int, CFixedPointFPML> result{0, CFixedPointFPML{0}};

    if (nNeededTransports < 0)  nNeededTransports  = 0;
    if (nWantedTransports < 0) nWantedTransports = 0;
    if (::RawFPML(vNeededStrength) < 0) vNeededStrength = CFixedPointFPML{0};
    if (::RawFPML(vWantedStrength) < 0) vWantedStrength = CFixedPointFPML{0};

    std::vector<NAssignToArmy::SAvailableFleet> available;

    for (CUnit* unit : *this->GetFleetList()) {
        CNavy* fleet = static_cast<CNavy*>(unit);
        if (fleet->InCombat())
            continue;
        if (fleet->HasTroopsOn())
            continue;
        if (!this->FleetCheckForTransportDeparture(fleet))
            continue;
        if (fleet->GetAIStraitTarget() >= 1 &&
            country->AccessStrategy().CalcIsDefendedHomeStraitUnderAttack())
            continue;

        CSafeAiPathFind pathfinder;

        CPathFind::SFindSettings settings{};

        settings._bDirectIfAdjacent = true;
        settings._bEnableReturnToPreviousNoZocHax = true;
        settings._nNumRegimentsForLoadingOverride  = -1;
        settings._HiredByCountryOverride = CCountryTag::FromRaw(kEmptyCountryTag);
        settings._pAccessCache = &AccessCache;
        settings._bPathLengthMap = 0;
        CList<int> path;
        if (!pathfinder.Find(fleet, fleet->GetLocation(), pLoadProvince, &path, settings, nullptr, nullptr))
            continue;

        CFixedPoint auxOut;
        CFixedPointFPML strength = fleet->GetNavyStrengthFactor(
            CCountryDataBase::AccessInstance()
                ->GetCountry(fleet->GetCountry())->GetCachedData().GetModifiers().GetModifier(),
            false, true, ESeaType::MAX, auxOut,
            nullptr, SubUnitType::MAX, nullptr);
        NAssignToArmy::SAvailableFleet avail;
        avail._pFleet = fleet;
        avail._Distance = path.GetSize();
        avail._Transports = fleet->GetTPC();
        avail._Strength = strength;
        available.push_back(avail);
    }

    for (CUnit* unit : country->GetUnits()) {
        if (!unit->IsNavy())
            continue;
        if (!this->FleetCheckForTransportDeparture(unit->GetNavy()))
            continue;
        if (!unit->IsOnMission())
            continue;
        if (unit->IsExploring())
            continue;

        CSafeAiPathFind pathfinder;

        CPathFind::SFindSettings settings{};

        settings._bDirectIfAdjacent = true;
        settings._bEnableReturnToPreviousNoZocHax = true;
        settings._nNumRegimentsForLoadingOverride  = -1;
        settings._HiredByCountryOverride = CCountryTag::FromRaw(kEmptyCountryTag);
        settings._pAccessCache = &AccessCache;
        settings._bPathLengthMap = 0;
        CList<int> path;
        if (!pathfinder.Find(unit, unit->GetLocation(), pLoadProvince, &path, settings, nullptr, nullptr))
            continue;

        CFixedPoint auxOut;
        CFixedPointFPML strength = unit->GetNavyStrengthFactor(
            CCountryDataBase::AccessInstance()
                ->GetCountry(unit->GetCountry())->GetCachedData().GetModifiers().GetModifier(),
            false, true, ESeaType::MAX, auxOut,
            nullptr, SubUnitType::MAX, nullptr);
        NAssignToArmy::SAvailableFleet avail;
        avail._pFleet = static_cast<CNavy*>(unit);
        avail._Distance = path.GetSize();
        avail._Transports = unit->GetTPC();
        avail._Strength = strength;
        available.push_back(avail);
    }

    auto score = [&](const NAssignToArmy::SAvailableFleet& f) -> int
    {
        if (f._pFleet == nullptr)
            return -1000000;
        int s = -::Div1000(static_cast<int64_t>(NDefines::NAI::INVASION_FLEET_DISTANCE_PENALTY.GetUnderlyingValue()) *
                         f._Distance * 1000);
        if (f._pFleet->IsOnMission())
            s -= NDefines::NAI::INVASION_FLEET_MISSION_PENALTY.GetUnderlyingValue();
        if (!this->IsTransportFleet(f._pFleet)) {
            int64_t d = static_cast<int64_t>(f._pFleet->GetLocation()->GetNavalDistance(pLoadProvince->GetProvinceID()).GetUnderlyingValue())
                 + f._pFleet->GetLocation()->GetNavalDistance(pOffloadProvince->GetProvinceID()).GetUnderlyingValue()
                 + pLoadProvince->GetNavalDistance(pOffloadProvince->GetProvinceID()).GetUnderlyingValue();
            s -= ::Div1000(static_cast<int64_t>(NDefines::NAI::INVASION_HOME_FLEET_PENALTY.GetUnderlyingValue()) * d * 1000);
        }
        if (nWantedTransports > 0) {
            int64_t neededK = static_cast<int64_t>(nWantedTransports) * 1000;
            if (IsValidRange(f._Transports * 1000))
                s += (f._Transports * 1000000) / static_cast<int>(neededK);
            else
                s += static_cast<int>(static_cast<int64_t>(f._Transports) * 1000000 / neededK);
        }
        if (::RawFPML(vWantedStrength) > 0) {
            float ratio = static_cast<float>(
                (::RawFPML(f._Strength) << 15) / ::RawFPML(vWantedStrength)) * kStrengthToFloat;
            float scaled = 1000.0f * ratio + kHalf;
            s += static_cast<int>(ratio >= 0.0f ? scaled : floorf(scaled));
        }
        return s;
    };
    std::stable_sort(available.begin(), available.end(),
                     [&](const NAssignToArmy::SAvailableFleet& a,
                         const NAssignToArmy::SAvailableFleet& b) -> bool {
                         return score(a) > score(b);
                     });

    int   totalTPC      = 0;
    int64_t totalStrength = 0;
    for (const NAssignToArmy::SAvailableFleet& avail : available) {
        if (avail._pFleet == nullptr)
            break;
        totalTPC      += avail._Transports;
        totalStrength += ::RawFPML(avail._Strength);
    }

    if (nNeededTransports <= totalTPC && ::RawFPML(vNeededStrength) <= totalStrength) {
        CFixedPointFPML strengthAcc{0};
        int   tpcRemaining = nWantedTransports;
        for (NAssignToArmy::SAvailableFleet& avail : available) {
            if (avail._pFleet == nullptr)
                break;
            if (tpcRemaining < 1 && vWantedStrength <= strengthAcc)
                break;
            if ((avail._Transports != 0 || strengthAcc < vWantedStrength) &&
                (tpcRemaining > 0 || ::RawFPML(avail._Strength) != 0)) {
                NAssignToArmy::UseFleet(&pArmyAgent->GetKing(), pArmyAgent, avail,
                                        strengthAcc, tpcRemaining, vWantedStrength, result.first);
            }
        }
        result.second = strengthAcc;
    }
    return result;
}
