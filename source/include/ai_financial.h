#pragma once

#include "ai_minister.h"
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

class CAIFinancialMinister : public CAIMinister
{
public:
    ~CAIFinancialMinister() override;                                                         

    void NotifyTick(CCountryFlagCache<char> &AccessCache) override;                                                                                         

    void Start() override;                                                                                
    void Finish() override;                                                                                
    bool IsTrader() override;                                                                                
    CUnitList *GetFleetList() override;                                                                           
    bool AssignFleet(CNavy &Fleet) override;                                                                           
    void DetachFleet(CNavy &Fleet) override;                                                                           

protected:
    uint8_t m_pad0xa41[0xa88 - 0xa41];          
};