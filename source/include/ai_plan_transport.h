#pragma once

#include "ai_plan_military.h"
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
class CProvince;
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
class CEU3AI;
enum CAIPlanType : int;

class CAITransportPlan : public CAIMilitaryPlan
{
public:
    inline CList<int> &GetPath();                                           

    ~CAITransportPlan() override;                                                     

    void NotifyTick(CCountryFlagCache<char> &) override;                                                                                     

    void NotifyArmyMoved(CArmy &) override;                                                                             
    void NotifyFleetMoved(CNavy &) override;                                                                             
    void NotifyArmyStopped(CArmy &) override;                                                                             
    void NotifyFleetStopped(CNavy &) override;                                                                             
    void Finish() override;                                                                              
    void Cancel() override;                                                                              
    CUnitList *GetArmyList() override;                                                                         
    bool AssignArmy(CArmy &) override;                                                                              
    void DetachArmy(CArmy &) override;                                                                              
    CUnitList *GetFleetList() override;                                                                         
    bool AssignFleet(CNavy &) override;                                                                              
    void DetachFleet(CNavy &) override;                                                                              
    CAIPlanType GetPlanType() override;                                                                        

protected:
    void TryLoad();                                                             
    void TryUnload();                                                             
    CUnitList _ArmyList;                               
    CUnitList _FleetList;                               
    CList<int> *_pPath;                           
    CProvince *_pCoastalSeaZone;                              
    bool _IsLoading;                                    
};

#include "ai_plan_transport.tpp"
