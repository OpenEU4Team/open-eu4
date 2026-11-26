#pragma once

#include "ai_agent.h"
#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/fixedpoint.h"
#include "clausewitz/pdx_core/hashtable.h"
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

class CArmy;
class CCountry;
class CFocalPoint;
class CID;
class CNavy;
class CProvince;
class CProvinceValue;
class CSubUnit;
class CUnit;
class CWar;
struct SUnitToUnit;
template <typename T>
class CCountryFlagCache;
class CAIAdmiral;
class CAIAgent;
class CAIArmy;
class CAIBuilder;
class CAIExpansionMinister;
class CAIExplorePlan;
class CAIExploreProvincePlan;
class CAIExploreSeaZonePlan;
class CAIFinancialMinister;
class CAIForeignMinister;
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

enum EFocalPointState : signed char;

class CAIKing : public CAIAgent
{
public:
    ~CAIKing() override;                                                                                         
    void Update(CCountryFlagCache<char> &AccessCache) override;                                                                             
    void NotifyTick(CCountryFlagCache<char> &AccessCache) override;                                                                              
    void NotifyArmyMoved(CArmy &Army) override;                                                                                       
    void NotifyFleetMoved(CNavy &Fleet) override;                                                                                      
    void NotifyArmyFinishedCombat(CArmy &Army) override;                                                                                       
    void NotifyFleetFinishedCombat(CNavy &Fleet) override;                                                                                      
    void NotifyPeace(CCountry &Country1, CCountry &Country2) override;                                                                   
    void Cleanup() override;                                                                                            
    void Start() override;                                                                                            
    CAIKing &GetKing() override;                                                                                        
    const CAIKing &GetKing() const override;                                                                             
    bool IsKing() override;                                                                                            
    const char *GetName() override;                                                                                     
    void NotifyAgentFinished(CAIAgent &Agent) override;                                                                                       
    void NotifyAgentCanceled(CAIAgent &Agent) override;                                                                                       
    float GetManPower() override;                                                                                            
    void AddManPower(float v) override;                                                                                      
    void RemoveManPower(float v) override;                                                                                      
    float GetColonists() override;                                                                                            
    void AddColonists(float v) override;                                                                                      
    void RemoveColonists(float v) override;                                                                                      
    float GetDiplomats() override;                                                                                            
    void AddDiplomats(float v) override;                                                                                      
    void RemoveDiplomats(float v) override;                                                                                      
    float GetMerchants() override;                                                                                            
    void AddMerchants(float v) override;                                                                                      
    void RemoveMerchants(float v) override;                                                                                      
    CUnitList *GetArmyList() override;                                                                                       
    bool AssignArmy(CArmy &Army) override;                                                                                        
    void DetachArmy(CArmy &Army) override;                                                                                        
    CUnitList *GetFleetList() override;                                                                                       
    bool AssignFleet(CNavy &Fleet) override;                                                                                       
    void DetachFleet(CNavy &Fleet) override;                                                                                       

    bool CreateFleet(CID id, CProvince *pLocation, bool bMakeHomeFleet);                                                          
    const CFocalPoint *GetAllianceFocalPoint() const;                                                                         
    void AssignSubUnitProperly(CID id, CSubUnit *pSub);                                                                                   
    bool CalculateHomelandThreatened() const;                                                                                       
    bool MayRequestFleets(CAIArmy *pRequester) const;                                                                            

    void CreateArmy(CID id, CProvince *pLocation, CArmy *pAgentArmy, bool bExileStatus, int nPrev, int nPrevNoZoC, int nAIDivisionAt, bool bWasPrevious, int nAIDivisionOf);                                                                     

    int CalcInvasionAreaDivNeed(const CProvince &TargetProv, CCountryFlagCache<char> &AccessCache) const;                                                                                          

    void FindRemoteInvasionTargets(CCountryTag EnemyTag, CPdxHybridArray<CProvinceValue, 32, int> &PrioList, const CWar *pWar) const;                                                                                                                       

    void FindInvasionTargets(CPdxHybridArray<CProvinceValue, 32, int> &PrioList);                                                                                            

    CAIArmy *FindInvasionArmy(const CProvince &Target, const CProvince **pBestTarget, CCountryFlagCache<char> &AccessCache);                                                                                       

    inline const CList<CAIArmy *> &GetArmyAgents() const;                                        
    inline CAIAdmiral *GetAdmiral();                                                            

protected:
    int CalcNominalArmySize() const;                                                              
    void PlanArmyTypes();                                                                  
    float GetShareOfCurrentlyInvadingTroops();                                                                 

    void CountArmiesPlacements(int &nHome, int &nAbroad, int &nCounted, int &nSmallest) const;                                                                

    void CalcDivNeedImpactForUnit(const CUnit *pUnit, CCountryTag Enemy, const CProvince &TargetProv, float &vFriendlyForce, float &vEnemyForce) const;                                                                                              

    CHashTable<CID, SUnitToUnit, ClassicHashKeyTraits<CID, SUnitToUnit>> _UnitAssignments;                              

    CArray<CCountryTag> _EnemyList;                                  
    uint8_t m_pad0xa60[0xa78 - 0xa60];                  
    CAIForeignMinister *_pForeign;                                
    CAIFinancialMinister *_pFinancialMinister;                               
    CAIExpansionMinister *_pExpansionMinister;                               
    uint8_t m_pad0xa90[0xa98 - 0xa90];                  
    CAIRecruitMinister *_pRecruitMinister;                                 
    uint8_t m_pad0xaa0[0xc40 - 0xaa0];                  
    CAIAdmiral *_pAdmiral;                                        
    CList<CAIArmy *> _ArmyAgents;                                     
    uint8_t m_pad0xc60[0xc78 - 0xc60];                  
    bool _IsPendingResource[6];                                               
    uint8_t m_pad0xc7e[0xd50 - 0xc7e];                  
};

class CFocalPoint : public CPersistent
{
public:
    void WriteMembers(CWriter &Writer) override;                                                                            
    void ReadMember(CReader &Reader, int Type) override;                                                                   

    EFocalPointState GetState() const;                                                     
    const CProvince *GetGatheringSpot() const;                                                      

private:
    unsigned char m_pad0xc[0x34];        
};

#include "ai_king.tpp"