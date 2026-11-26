#include "enums.h"
#include "ai_sharedeval.h"
#include "ai_army.h"
#include "ai_threatevaluation.h"
#include "army.h"
#include "combat.h"
#include "country.h"
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

float CAISharedEval::CalcAcceptableThreat(const CProvince *pProv, const CArmy *pArmy,
                                          float vArmyStrength, float vFriendlyStrength,
                                          float vEnemyStrength,
                                          int nEnemyForce,
                                          int nSiegeForce,
                                          bool bEnemyArrivesBeforeUs,
                                          bool bFriendsNeedHelp,
                                          bool bOverlordHere,
                                          CCountryTag Enemy)
{
    (void)vEnemyStrength;
    (void)bEnemyArrivesBeforeUs;
    (void)Enemy;

    float vCertainty;
    int siege_needed = pProv->GetRegimentsNeededForSiege();
    if (nSiegeForce < siege_needed)
        vCertainty = (nSiegeForce > 0) ? kThreatCertaintyPartial : kThreatHalf;
    else
        vCertainty = (pProv->GetFortLevel() == 0) ? kThreatCertaintyNoFort : kThreatOne;

    float vAdjust = (vCertainty * vFriendlyStrength + kThreatOne) / (vArmyStrength + kThreatOne) + kThreatOne;

    if (nEnemyForce > 0)
    {
        const float outnumberNeeded = nEnemyForce * kThreatOutnumber;
        const int actual = pArmy->GetNumSubUnits() + pArmy->GetNumAttachedSubunits(true);
        if (std::isgreater(static_cast<float>(actual), outnumberNeeded))
            vAdjust *= kThreatOutnumberMult;
    }

    const auto adjustForSiegeAndDestination = [&]() -> void
    {
        const CSiegeCombat *siege = pArmy->AccessSiege();
        if (siege != nullptr)
        {
            if (pArmy->GetLocation() == pProv && siege->GetCachedSiegeImpact() > 3)
                vAdjust *= kThreatOwnFort;
            if (pArmy->GetLocation() == pProv && !pArmy->IsMoving() && pProv->GetFortLevelAfterMothball() > 0)
            {
                vAdjust = static_cast<float>(static_cast<double>(vAdjust) * kThreatFortDouble);
                return;
            }
        }
        if (pArmy->GetFinalDestination() == pProv)
            vAdjust *= kThreatFinalDest;
    };
    adjustForSiegeAndDestination();

    if (NSubjectBehavior::WantsToFollowFocus(pArmy) &&
        CCountryDataBase::AccessInstance()->GetCountry(pArmy->GetCountry())->GetSubjectMilitaryFocus() == ESubjectMilitaryFocus::_FOCUS_AGGRESSIVE_)
    {
        float focusMult = kThreatFocusWide;
        if (bOverlordHere && bFriendsNeedHelp)
            focusMult = kThreatFocusNarrow;
        vAdjust *= focusMult;
    }

    if (pArmy->CArmy::IsCondottieri())
        vAdjust *= kThreatCondottieri;

    if (NSubjectBehavior::WantsToStayInOwnTerritory(pArmy) &&
        pProv->GetOwner().GetCategoryWord() != pArmy->GetCountry().GetCategoryWord())
        vAdjust *= kThreatHalf;

    float finalMult = kThreatOutnumber;
    if (!bFriendsNeedHelp)
        finalMult = kThreatFinalElse;
    return finalMult * vAdjust;
}
