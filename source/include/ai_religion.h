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
class CAISendColonistPlan;
class CAISubscriber;
class CAIStrategy;
class CAITransportPlan;
class CEU3AI;
enum CAIPlanType : int;

class CAIReligionMinister : public CAIPlan
{
public:
    ~CAIReligionMinister() override;                                                                                       

    void NotifyTick(CCountryFlagCache<char> &AccessCache) override;                                                                                        
    
    void Start() override;                                                                                                      
    void Finish() override;                                                                                                      
    CAIPlanType GetPlanType() override;                                                                                                

protected:
    uint8_t m_pad0xaa0[0xab8 - 0xaa0];          
};
