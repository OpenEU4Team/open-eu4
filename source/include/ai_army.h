#pragma once

#include "ai_plan.h"
#include "areas.h"
#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/fixedpoint.h"
#include "clausewitz/pdx_core/list.h"
#include "clausewitz/pdx_core/pdxarray.h"
#include "clausewitz/pdx_core/pdxhybridarray.h"
#include "countrytag.h"
#include "engine.h"
#include "enum_indexed_array.h"
#include "eu3date.h"
#include "eu4misc.h"
#include "pdx_def_val.h"
#include "unitlist.h"
#include <cstdint>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

class CAIRegion;
class CArmy;
class CControllerArea;
class CID;
class CNavy;
class CProvince;
class CString;
class CUnit;
template <typename T>
class CCountryFlagCache;
class CAIAdmiral;
class CAIAgent;
class CAIBuilder;
class CAIExpansionMinister;
class CAIExplorePlan;
class CAIExploreProvincePlan;
class CAIExploreSeaZonePlan;
class CAIFinancialMinister;
class CAIForeignMinister;
class CAIKing;
class CAIMilitaryPlan;
class CAIMinister;
class CAINavalBlockadePlan;
class CAINavalPatrolPlan;
class CAIPeon;
class CAIPlan;
class CAIRecruitMinister;
class CAIReligionMinister;
class CAISendColonistPlan;
class CAISubscriber;
class CAIStrategy;
class CAITransportPlan;
class CEU3AI;
class CProvince;
enum AIMoveResult : int;
enum CAIPlanType : int;

class CStrengthFactorCache;
#define HELPER inline

int GetTargetRegimentCount(CCountryTag Country, const CProvince *pProv);                                                                    

class CReachCostCallback : public CAreaCostCallback
{
public:
    inline CReachCostCallback(const CUnit *pUnit, CCountryFlagCache<char> &AccessCache);                                                                   

    float Cost(const CControllerArea &goal) override;                                                                                          

private:
    const CUnit *const _pUnit;                           
    CCountryFlagCache<char> &_AccessCache;                        
};

struct SFleetInfo
{
    CNavy *_pNavy;                                         
    CFixedPoint _vDistance;                                              
    AIMoveResult _MoveResult;                                    

    bool operator<(const SFleetInfo &Other) const;                                                                                                                                                                          
};

class CAIArmy : public CAIPlan
{
public:
    inline bool IsInvasionInterrupted() const;                                                
    inline bool IsInvadingBySea() const;                                                
    inline int GetConsolidationTarget() const;                                                 

    enum EGetFleetsToLoadZoneResult : int
    {
        CancelInvasion = 0,
        Wait = 1,
        GoGoGo = 2
    };

    ~CAIArmy() override;                                                                                      
    void NotifyTick(CCountryFlagCache<char> &AccessCache) override;                                                                           
    void NotifyArmyMoved(CArmy &Army) override;                                                                                    
    void Cleanup() override;                                                                                         
    void Start() override;                                                                                         
    bool IsArmy() override;                                                                                         
    void NotifyAgentCanceled(CAIAgent &Agent) override;                                                                                    
    inline CUnitList *GetArmyList() override;                                                                             
    HELPER const CUnitList *GetArmyList() const;
    bool AssignArmy(CArmy &Army) override;                                                              
    void DetachArmy(CArmy &Army) override;                                                              
    CUnitList *GetFleetList() override;                                                             
    bool AssignFleet(CNavy &Fleet) override;                                                             
    void DetachFleet(CNavy &Fleet) override;                                                             
    CAIPlanType GetPlanType() override;                                                            
    bool IsMilitaryPlan() override;                                                                   

    const CUnitList *GetFleetList() const;                                                            
    bool IsHunterKiller() const;                                                                        
    void SetHunterKiller(bool b);                                                                       
    bool IsProvinceUnsafeForTransport(int) const;                                                                     
    void SetProvinceUnsafeForTransport(int, int);                                                                      
    void SetInvasionTarget(int nProv, bool bForcedBySea);                                                  
    void SetConsolidationTarget(int nProv);                                                                    

    const CAIRegion *GetAssignedRegion() const;                                                              
    CArmy *GetMainArmy() const;                                                                        
    bool IsNearHotSpot() const;                                                                          
    bool MayInvade() const;                                                                         
    int TakeFromList(CUnitList *pArmyList, int nNeeded);                                                          
    int RecalcInvasionTarget(CArmy *pOurArmy);                                                                       
    void ReclaimArmiesUnderCreation();                                                                               
    bool CanInvade(int) const;                                                                      
    const CProvince *GetFocalProvince(bool &bGather) const;                                                  

    bool CanInvadeBySea(CArmy *pArmy, CProvince *pTarget, int &nNeededTPC, int &nWantedTPC, CCountryFlagCache<char> &AccessCache, bool bIgnoreStrengthChecks);                                                                                            

    bool CanGoByLand(CArmy *pArmy, CProvince *pTarget, CList<int> &Path, CCountryFlagCache<char> &AccessCache, int nRegsOverride);                                                                                              

    void SplitOff(CArmy *pArmy, int nWantedSize, int nAIDivisionAt, int nAIDivisionOf);                                                

    CProvince *CalcInvasionTarget(CControllerArea *pArea, const CArmy *pOurArmy);                                                                           

    void AssignToRegion(const CAIRegion *pRegion, CCountryFlagCache<char> *pAccessCache);                                                                                

    bool CanReach(const CArmy *pArmy, const CProvince *pTarget, CCountryFlagCache<char> &AccessCache);                                                                                 

    bool IsForExploration(EStopMissionReason *pReason) const;                                                                     
    bool IsCondottieri() const;                                                                            

    static void DetermineEnemy(CCountryTag &, const CArmy *, const CProvince *);                                                                               

    float EvaluateProvince(const CArmy *pArmy, float vAverageMorale, const CProvince *const pProv, float &vTotalBalance, bool &bFriendsNeedHelp, bool &bFriendsEnteringCombat, bool &bFriendToAttachTo, bool bAllowAttach, float vDeprecated, float vProvThreat, float vCurrentThreat, bool &bIndirectlyThreatened, float vAcceptableBalanceMult, bool *pShouldStayPut, bool *pRunningAway, CString *pDesc, CCountryFlagCache<char> *pAccessCache);                                                                                                                           

    float GetThreat(const CArmy *pArmy, const CProvince *pProv, bool bLowMorale, bool bIncludeNeutral);                                                             

    inline int GetBaseProvince() const;                                               
    inline int GetInvasionTarget() const;                                                 
    inline CEU3Date GetInvasionDate() const;                                          

protected:
    void DecayUnsafety();                                                                                     
    void UnassignFleets();                                                                                     
    int MainLoop(CCountryFlagCache<char> &AccessCache);                                                                       
    void ProcessMainArmy(CCountryFlagCache<char> &AccessCache);                                                                       
    void ProcessInvasion(CCountryFlagCache<char> &AccessCache);                                                                       
    bool ShouldGather() const;                                                                                
    int CalcGatherProvince() const;                                                                                 
    void CheckShouldEndSplinterInvasion();                                                                                     
    void HaltUnit(CUnit &Unit, EStopMissionReason Reason);                                                                        
    bool WantsDivision(const CArmy *pMainArmy, int &nTargetProv);                                                      
    void AbortDivision(CArmy *);                                                                                    
    bool CheckDivisionSafety(const CArmy *) const;                                                                          

    void ProcessSplinterInvasion(CArmy *pArmy, CCountryFlagCache<char> &AccessCache);                                                                                    

    bool ProcessSplinter(CArmy *pArmy, CArmy *pMainArmy, bool bShouldGather, CCountryFlagCache<char> &AccessCache);                                                                                

    CAIArmy::EGetFleetsToLoadZoneResult GetFleetsToLoadZoneAndDetachUnnecessary(CCountryFlagCache<char> &AccessCache, const CProvince *pLoadProv, const CProvince *pInvasionTarget, int nWantedTransports, int nNeededTransports, CFixedPointFPML vWantedStrength, CFixedPointFPML vNeededStrength);                                                                                                                                                  

    void ProcessSplinters(CCountryFlagCache<char> &AccessCache);                                                                      

    bool CheckRetreat(const CArmy *pArmy, bool &bIncomingEnemies, CCountryFlagCache<char> &AccessCache);                                                                            

    CUnit *FindArmyToAttachTo(const CArmy *pArmy, const CProvince *pProv);                                                                     

    CProvince *ShouldReturnToBaseWithBoats(CCountryFlagCache<char> &AccessCache);                                                                                 

    bool IsSafeForSiegeForce(CProvince *pProvince, CArmy *pArmy, CFixedPointFPML vSiegeForce, CFixedPoint vLeaderAndTerrain, CCountryFlagCache<char> &AccessCache, bool bIgnoreFriendly, bool bIgnoreRebels) const;                                                                                                                                            

    bool TryMergeAtLocation(const CProvince &Prov, CUnitList *pList, bool bDoValidityCheck);                                                                         

    AIMoveResult MoveArmy(CUnit &Army, int nTarget, CCountryFlagCache<char> &AccessCache, int nWhen, bool bCheckSafety);                                                                       

    AIMoveResult MoveFleet(CNavy &Fleet, int nTarget, bool bCancelInvasionIfFailing, CCountryFlagCache<char> &AccessCache);                                                                       

    bool WantsToTakeProvinceEasily(const CArmy *, const CProvince &);                                                                            

    bool MakeDivision(CArmy *pMainArmy, int nTargetProv, CCountryFlagCache<char> &AccessCache);                                                                          

    void ProcessDivision(CArmy *pArmy, std::map<std::pair<int, int>, CList<CUnit *>, std::less<std::pair<int, int>>, std::allocator<std::pair<std::pair<int, int> const, CList<CUnit *>>>> &ToMerge, CCountryFlagCache<char> &AccessCache);                                                                                                                                           

    void MergeDivisions(const std::map<std::pair<int, int>, CList<CUnit *>, std::less<std::pair<int, int>>, std::allocator<std::pair<std::pair<int, int> const, CList<CUnit *>>>> &);                                                                                                             

    CUnitList _ArmyList;                                                
    CUnitList _FleetList;                                                
    CList<CID> _OurArmies;                                               
    int _nBaseProv;                                                     
    uint8_t m_pad0xaec[0xaf0 - 0xaec];                     
    const CAIRegion *_pAssignedRegion;                                        
    int _nInvasionTarget;                                                     
    TPdxDefVal<bool, 1, 1> _bInvasionInterrupted;                                  
    uint8_t m_pad0xafd[0xb00 - 0xafd];                     
    int _nConsolidationTarget;                                                     
    uint8_t m_pad0xb04[0xb08 - 0xb04];                     
    bool _bInvadingBySea;                                                     
    bool _bIsIdleForInvasion;                                                      
    uint8_t m_pad0xb0a[0xb10 - 0xb0a];                     
    CEU3Date _InvasionDate;                                                 
    bool _bHunterKiller;                                                    
    uint8_t m_pad0xb15[0xb16 - 0xb15];                     
    bool _bNeedsHelp;                                                    
    bool _bWillWaitForHelp;                                                      
    int _nHelpTicks;                                                     
    int _nSplitOff;                                                     
    uint8_t m_pad0xb20[0xb28 - 0xb20];                     
    CList<int> _CannotInvadeList;                                                
    CList<int> _BadLoadZoneList;                                                 
    bool _bIsExploration;                                                    
    EStopMissionReason _StopExplorationReason;                                        
    uint8_t m_pad0xb5a[0xb68 - 0xb5a];                     

    std::unordered_map<int, CEU3Date, TTrivialHasher<int>, std::equal_to<int>, std::allocator<std::pair<int const, CEU3Date>>> _UnsafeForTransportMemory;                                         

    uint8_t m_pad0xba0[0xbe0 - 0xba0];          
};

namespace NSubjectBehavior
{
bool WantsToFollowFocus(const CArmy *pArmy);                                                                          
bool WantsToStayInOwnTerritory(const CArmy *pArmy);                                                                          
}                              

#include "ai_army.tpp"
