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

class CNavy;
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

class CAINavalBlockadePlan : public CAIMilitaryPlan
{
public:
    ~CAINavalBlockadePlan() override;                                                         

    void NotifyTick(CCountryFlagCache<char> &) override;                                                                                         

    void NotifyFleetInCombat(CNavy &) override;                                                                                  
    void Cleanup() override;                                                                                   
    void Start() override;                                                                                   
    CUnitList *GetFleetList() override;                                                                              
    bool AssignFleet(CNavy &) override;                                                                                   
    void DetachFleet(CNavy &) override;                                                                                   
    CAIPlanType GetPlanType() override;                                                                             

protected:
    uint8_t m_pad0xaa8[0xad8 - 0xaa8];          
};
