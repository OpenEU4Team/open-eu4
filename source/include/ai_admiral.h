#pragma once

#include "ai_peon.h"
#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/fixedpoint.h"
#include "clausewitz/pdx_core/list.h"
#include "clausewitz/pdx_core/pdxarray.h"
#include "clausewitz/pdx_core/pdxhybridarray.h"
#include "countrytag.h"
#include "engine.h"
#include "enum_indexed_array.h"
#include "eu3date.h"
#include "unitlist.h"
#include <cstdint>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

class CNavy;
class CProvince;
class CString;
class CUnit;
class CAIAgent;
class CAIArmy;
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
class CNavy;
class CProvince;
class CGameState;
class CAIAdmiral;
class CCountry;

template <typename T>
class CCountryFlagCache;
template <typename T>
class CCountryFlagCache;

enum AIMoveResult : int;

bool CheckShouldNavyRetreat(const CNavy *pNavy);                                                   

class CAIAdmiral : public CAIPeon
{
public:
    ~CAIAdmiral() override;                                               

    void NotifyTick(CCountryFlagCache<char> &AccessCache) override;                                                                               

    void Cleanup() override;                                                                      
    void Start() override;                                                                      
    const char *GetName() override;                                                               
    bool IsAdmiral() override;                                                                      
    CUnitList *GetFleetList() override;                                                                 
    bool AssignFleet(CNavy &Fleet) override;                                                                 
    void DetachFleet(CNavy &Fleet) override;                                                                 

    std::pair<int, CFixedPointFPML> AssignToArmyAgent(CAIArmy *pArmyAgent, int nNeededTransports, int nWantedTransports, CFixedPointFPML vNeededStrength, CFixedPointFPML vWantedStrength, CCountryFlagCache<char> &AccessCache, const CProvince *pLoadProvince, const CProvince *pOffloadProvince);                                                                                                                                         

    CFixedPoint EvaluateProvince(CNavy *pNavy, const CProvince *pProv, CFixedPointFPML vvAdjustmentToStrength, CCountryFlagCache<char> *pAccessCache, CString *pTooltip) const;                                                                                                                                        

    CPdxArray<CUnit *, int> ClearEmptyFleets(const CPdxArray<CUnit *, int> &ExcludeUnits);                                                                         

    bool IsTransportFleet(const CNavy *pNavy) const;                                                            
    bool IsExplorerFleet(const CNavy *pNavy) const;                                                            
    bool UsesHomeFleetAsTransport() const;                                                                       

    static bool FleetCheckForTransportDeparture(const CNavy *pFleet);                                                                          

protected:
    void UpdateFleets(CCountryFlagCache<char> &AccessCache);                                                                                
    bool IsSafeMove(const CNavy *pNavy, const CProvince *pProv) const;                                                                  

    void HaltUnit(CUnit &Unit, EStopMissionReason Reason);                                                                     
    int GetTargetHomeFleetSize() const;                                                                              
    void CalcBases();                                                                                 
    void TrimExplorationFleet(CNavy *pNavy);                                                                            
    bool AllowingBlockadeFleets() const;                                                                             
    void UpdateAllowingBlockadeFleets();                                                                                  
    void MergeFleets();                                                                                  
    void AssignStraitDefenders();                                                                                  
    bool WantsToAvoidBlockades() const;                                                                             

    bool DesiresMerge(const CNavy *pFleet, const CNavy *pOtherFleet, bool &bFullMerge, int &nNeededHeavy, int &nNeededLight, int &nNeededGalley, int &nNeededTransport) const;                                                                        

    CUnit *FindNavyToAttachTo(const CNavy *pNavy, const CProvince *pProv);                                                                         

    void AssignBlockadeFleets(CCountryFlagCache<char> &AccessCache);                                                                              

    void AssignAndTrimHomeFleet(CCountryFlagCache<char> &AccessCache);                                                                                

    void CalcStraitDefenders(CFixedPointFPML &vAvailableStrength, CFixedPointFPML &vAssignedStrength, CNavy *&pBest) const;                                                                                                    

    bool DownsizeFleetToBlockadeCapacityAndStrength(CNavy *pFleet, const CProvince *pTarget, CFixedPoint vCapacityWithoutThisSubunit, CFixedPointFPML vvNavalStrength, bool bForceDetachTransports);                                                                                                                                               

    bool ShouldRaidForSlaves(const CNavy *pFleet, const CProvince *pLocation, bool bIgnoreCurrentLocation) const;                                                                            

    AIMoveResult MoveFleet(CNavy &Fleet, int nTarget, CCountryFlagCache<char> &AccessCache);                                                                          

    uint8_t m_pad0xa41[0xa48 - 0xa41];          
    CUnitList _FleetList;                                    
    bool _bAllowingBlockadeFleets;                                         
    uint8_t m_pad0xa61[0xa78 - 0xa61];          
    bool _bAssignedHomeFleet;                                         
    uint8_t m_pad0xa79[0xa98 - 0xa79];          
};

namespace NAssignToArmy
{
struct SAvailableFleet
{
    CNavy *_pFleet;                                
    int32_t _Distance;                                              
    int32_t _Transports;                                        
    CFixedPointFPML _Strength;                              
};

void UseFleet(CAIKing *pAiKing, CAIArmy *pArmyAgent, SAvailableFleet &Fleet, CFixedPointFPML &vAssignedStrength, int &nRemainingTransports, const CFixedPointFPML &vMatchStrength, int &nAssignedTransports);                                                                                                                                 

}                           
