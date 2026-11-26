#pragma once

#include "enums.h"

#include "ai_subscriber.h"
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

class CArmy;
class CCountry;
class CNavy;
class CPathFind;
class CProvince;
class CUnit;
template <typename T>
class CCountryFlagCache;
class CAIAdmiral;
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
enum AIMoveResult : int;

struct SUnitToUnit
{
    CID _UnitID;                               
    CID _ToUnitID;                                

    SUnitToUnit(const CID &UnitID, const CID &ToUnitID);                                             
    inline const CID &GetKey() const;                                                        
};

class CAIAgent : public CAISubscriber
{
public:
    inline bool IsFinished();                                                                        
    inline bool IsCanceled();                                                                        
    ~CAIAgent() override;                                                                                  
    bool WantTicks() override;                                                                                     
    void Update(CCountryFlagCache<char> &AccessCache) override;                                                                       

    virtual bool CreateWithOwner(CAIAgent &Owner);                                                                       
    virtual void Cleanup();                                                                                   
    virtual void Kill();                                                                                   
    virtual void Start();                                                                                   
    virtual void Finish();                                                                                   
    virtual void Cancel();                                                                                   
    virtual CUnit &TrySplitUnit(CUnit &Unit);                                                                             
    virtual CAIKing &GetKing() = 0;                                                                                   
    virtual const CAIKing &GetKing() const = 0;                                                                        
    virtual bool IsExplorer();                                                                                    
    virtual bool IsKing();                                                                                   
    virtual bool IsPlan();                                                                                   
    virtual CAIPlan &GetPlan();                                                                               
    virtual const char *GetName();                                                                            
    virtual CAIMinister &GetMinister();                                                                            
    virtual bool IsMinister();                                                                                    
    virtual bool IsArmy();                                                                                   
    virtual bool IsAdmiral();                                                                                   
    virtual bool IsTrader();                                                                                   
    virtual void NotifyAgentFinished(CAIAgent &Agent);                                                                       
    virtual void NotifyAgentCanceled(CAIAgent &Agent);                                                                       
    virtual void NotifyDiplomatSent(int nProvince);                                                                       
    virtual void NotifyDiplomatNotSent(int nProvince);                                                                       
    virtual void NotifyColonistSent(int nProvince);                                                                       
    virtual void NotifyColonistNotSent(int nProvince);                                                                       
    virtual void NotifyTraderSent(int nProvince);                                                                       
    virtual void NotifyTraderNotSent(int nProvince);                                                                       
    virtual void NotifyMerchantSent(int nProvince);                                                                       
    virtual void NotifyMerchantNotSent(int nProvince);                                                                       
    virtual void NotifyManufacturyStarted(int nProvince);                                                                       
    virtual void NotifyManufacturyCancelled(int nProvince);                                                                       
    virtual void NotifyPlanValueChanged(CAIPlan &Plan);                                                                               
    virtual float GetManPower();                                                                                   
    virtual void AddManPower(float v);                                                                             
    virtual void RemoveManPower(float v);                                                                             
    virtual float GetColonists();                                                                                   
    virtual void AddColonists(float v);                                                                             
    virtual void RemoveColonists(float v);                                                                             
    virtual float GetDiplomats();                                                                                   
    virtual void AddDiplomats(float v);                                                                             
    virtual void RemoveDiplomats(float v);                                                                             
    virtual float GetMerchants();                                                                                   
    virtual void AddMerchants(float v);                                                                             
    virtual void RemoveMerchants(float v);                                                                             
    virtual void RejectUnit(CUnit &Unit);                                                                               
    virtual void ClearRejectionList();                                                                                    
    virtual bool HasRejectedUnit(CUnit &Unit);                                                                               
    virtual void RemoveRejection(CUnit &Unit);                                                                               
    virtual void AddRejection(CUnit &Unit);                                                                               
    virtual bool AcceptArmyOverflow();                                                                                    
    virtual CUnitList *GetArmyList();                                                                              
    virtual bool AssignArmy(CArmy &Army);                                                                               
    virtual void DetachArmy(CArmy &Army);                                                                               
    virtual bool AcceptFleets();                                                                                    
    virtual CUnitList *GetFleetList();                                                                              
    virtual bool AssignFleet(CNavy &Fleet);                                                                              
    virtual void DetachFleet(CNavy &Fleet);                                                                              

    virtual float GetApproxNavalPower(const CUnitList *list, bool bConsiderMorale, bool bEnemyOnly, bool bUsOnly);                                                                              

    AIMoveResult MoveFleet(CNavy &Fleet, CProvince &Prov, CCountryFlagCache<char> &AccessCache, int nRemoveLastMoves);                                                                                  

    AIMoveResult MoveUnitThroughOwned(CUnit &, CProvince &, CCountryFlagCache<char> &);                                                                                             

    AIMoveResult MoveUnit(CUnit &Unit, CProvince &Prov, CCountryFlagCache<char> &AccessCache, int nRemoveLastMoves);                                                                                 

    AIMoveResult MoveUnit(CUnit &Unit, CProvince &Prov, CList<int> &, CPathFind &, CCountryFlagCache<char> &AccessCache, int nRemoveLastMoves);                                                                                                      

    bool AcceptAndMove(CArmy &, CProvince &, int);                                                                              
    bool AcceptAndMove(CNavy &, CProvince &, int);                                                                              
    bool AcceptAndMove(CUnit &, CProvince &, CList<int> &, int);                                                                          
    bool AcceptAndMove(CUnit &, CProvince &, int);                                                                              
    void StopFleet(CNavy &, EStopMissionReason);                                                                                   
    void StopUnit(CUnit &Unit, EStopMissionReason Reason);                                                                        
    bool MergeArmy(CArmy &);                                                                                   
    bool MergeFleet(CNavy &);                                                                                    
    bool MergeUnit(CUnit &Unit);                                                                               
    bool MergeUnit(CUnit &Unit, CUnitList *pList);                                                                        
    bool MergeTheseUnits(CUnit &, CUnitList *, bool);                                                                             
    int GetRandom() const;                                                                                
    bool IsActive();                                                                                    
    void AcceptUnit(CUnit &Unit);                                                                                

    inline CCountry &GetCountry() const;                                             
    inline CCountryTag GetCountryTag() const;                                         
    inline CList<CAIAgent *> &GetAgentList();                                        

protected:
    CCountryTag _Country;                                            
    unsigned char _IsActive;                                               
    unsigned char _IsCanceled;                                                 
    unsigned char _IsFinished;                                                
    uint8_t m_pad0x27[0x28 - 0x27];          
    CAIAgent *_pOwner;                                          
    CList<CAIAgent *> _AgentList;                                     
    CUnitList _RejectUnitList;                                             
    uint8_t m_pad0x60[0xa38 - 0x60];         
};

#include "ai_agent.tpp"
