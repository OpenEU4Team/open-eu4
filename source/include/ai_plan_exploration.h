#pragma once

#include "ai_plan.h"
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
class CNavy;
template <typename T>
class CCountryFlagCache;
class CAIAdmiral;
class CAIAgent;
class CAIArmy;
class CAIBuilder;
class CAIExpansionMinister;
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
enum CAIPlanType : int;

class CAIExplorePlan : public CAIPlan
{
public:
    bool IsExplorer() override;                                                                             
    bool IsExplorePlan() override;                                                                             
    CAIExplorePlan &GetExplorePlan() override;                                                                  

protected:
    uint8_t m_pad0xaa0[0xaa5 - 0xaa0];          
};

class CAIExploreProvincePlan : public CAIExplorePlan
{
public:
    ~CAIExploreProvincePlan() override;                                                                            
    void NotifyTick(CCountryFlagCache<char> &) override;                                                                                           
    void NotifyArmyMoved(CArmy &) override;                                                                                             
    void NotifyArmyStopped(CArmy &) override;                                                                                             
    void Start() override;                                                                                              
    CUnitList *GetArmyList() override;                                                                                         
    bool AssignArmy(CArmy &) override;                                                                                              
    void DetachArmy(CArmy &) override;                                                                                              
    CAIPlanType GetPlanType() override;                                                                                        

protected:
    uint8_t m_pad0xaa5[0xac0 - 0xaa5];          
};

class CAIExploreSeaZonePlan : public CAIExplorePlan
{
public:
    ~CAIExploreSeaZonePlan() override;                                                          

    void NotifyTick(CCountryFlagCache<char> &AccessCache) override;                                                                                          

    void Cleanup() override;                                                                                 
    void Start() override;                                                                                 
    CUnitList *GetFleetList() override;                                                                            
    bool AssignFleet(CNavy &Fleet) override;                                                                            
    void DetachFleet(CNavy &Fleet) override;                                                                            
    CAIPlanType GetPlanType() override;                                                                           

protected:
    uint8_t m_pad0xaa5[0xac0 - 0xaa5];          
};
