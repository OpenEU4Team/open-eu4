#pragma once
#include "clausewitz/pdx_core/pdxarray.h"
#include "countrytag.h"
#include <cstdint>

class CProvince;
class CArmy;
class CNavy;
class CUnit;
class CStrengthFactorCache;
class CString;

namespace CAISharedEval
{

struct SServerOnlyThreatData
{
    bool _bEnabled;                                                       
    float _vCurrentThreat;                                           
    const float *_pPrecalculatedThreat;                                          
};

void DetermineEnemy(CCountryTag &enemy, const CArmy *pArmy, CCountryTag MyTag, const CProvince *pProv);                                                                                         

int FindEnemyETA(const CArmy *pArmy, const CProvince *pProv, int nMinEnemies);                                                                       

int GetOurTerrainAdvantage(const CArmy *pArmy, const CProvince *pProv, bool bFriendBeingAttacked, bool bFriendInCombat, bool bEnemyArrivesBeforeUs, CCountryTag enemy);                                                                                                

bool ShouldAvoidStrongNatives(const CProvince *pTarget, const CArmy *pArmy);                                                                                  

float CalcAcceptableThreat(const CProvince *pProv, const CArmy *pArmy, float vArmyStrength, float vFriendlyStrength, float vEnemyStrength, int nEnemyForce, int nSiegeForce, bool bEnemyArrivesBeforeUs, bool bFriendsNeedHelp, bool bOverlordHere, CCountryTag Enemy);                                                                                                   

template <typename T>
bool IsBalanceAcceptable(const CProvince *pProv, const CArmy *pArmy, CCountryTag enemy, bool bWantsCombat, bool bEnemyArrivesBeforeUs, T vFriendlyStrength, T vExtraFriendlyStrength, T vExtraExtraFriendlyStrength, int nFriendlyForce, int nExtraFriendlyForce, int nExtraExtraFriendlyForce, T vEnemyStrength, int nEnemyForce, int nOurBestLeaderPips, int nTheirBestLeaderPips, int nOurTerrainAdvantage, T &vTotalBalance, CStrengthFactorCache &StrengthFactorCache, T vAcceptableBalanceBase, int nRebelForce, T vProvThreat, CString *pDesc);                                                                                                                                                             

template <typename T>
bool IsSafeMove(const CArmy *pArmy, const CProvince *pTarget, CStrengthFactorCache &StrengthFactorCache, const SServerOnlyThreatData &ServerOnlyThreatData, T vAcceptableBalanceBase);                                                                                                                              

template <typename T>
void EvaluateUnitsInProvince(const CProvince *pProv, const CArmy *pArmy, CCountryTag enemy, CPdxArray<CUnit *, int> &Counted, CPdxArray<CUnit *, int> &CountedFriendlyUnits, bool &bEnemyArrivesBeforeUs, bool &bFriendInCombat, bool &bFriendBeingAttacked, bool &bMySplinterHere, bool &bOverlordHere, int &nMyForce, int &nSiegeForce, int &nSiegeForceNotMe, int &nFriendlyForce, T &vFriendlyStrength, int &nOurBestLeaderPips, int &nTheirBestLeaderPips, int &nRebelForce, int &nEnemyForce, T &vRebelStrength, T &vEnemyStrength, int nEnemyETA, CStrengthFactorCache &StrengthFactorCache);                                                                                                                                                                                                

template <typename T>
void EvaluateUnitsMovingIntoProvince(const CProvince *pProv, const CArmy *pArmy, CCountryTag enemy, CPdxArray<CUnit *, int> &Counted, CPdxArray<CUnit *, int> &CountedFriendlyUnits, bool bIsEvaluate, bool &bEnemyArrivesBeforeUs, int &nOurBestLeaderPips, int &nTheirBestLeaderPips, int &, int &nEnemyForce, T &vEnemyStrength, int nMinEnemies, int &nExtraFriendlyForce, int &nExtraExtraFriendlyForce, T &vExtraFriendlyStrength, T &vExtraExtraFriendlyStrength, int &nExtraEnemyForce, bool &bBorderPrio, bool &bEnemyBorder, bool &bEnemyFortBorder, bool &bBorder, bool &bNextToFriendly, bool &bNextToEnemyForce, bool &bNextToRebels, int &nNextToSiegeForce, bool &bFriendInCombat, bool &bFriendBeingAttacked, int nFriendlyForce, CStrengthFactorCache &StrengthFactorCache);                                                                                                                                                                                                                         

}                           

namespace NSubjectBehavior
{
bool WantsToAvoidAttach(const CNavy *pNavy);                                                                   
}

#include "ai_sharedeval.tpp"
