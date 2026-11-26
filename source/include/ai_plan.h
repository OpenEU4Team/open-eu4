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

class CAIRequest;
class CUnit;
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
class CAIRecruitMinister;
class CAIReligionMinister;
class CAISendColonistPlan;
class CAISubscriber;
class CAIStrategy;
class CAITransportPlan;
class CEU3AI;
enum CAIPlanType : int;

class CAIPlan : public CAIPeon
{
public:
    bool CreateWithOwner(CAIAgent &Owner) override;                                                                        
    void Kill() override;                                                                             
    void Start() override;                                                                             
    CUnit &TrySplitUnit(CUnit &Unit) override;                                                                       
    bool IsPlan() override;                                                                             
    CAIPlan &GetPlan() override;                                                                         
    const char *GetName() override;                                                                      
    void NotifyAgentCanceled(CAIAgent &Agent) override;                                                                        
    void RejectUnit(CUnit &Unit) override;                                                                         
    void ClearRejectionList() override;                                                                              
    bool HasRejectedUnit(CUnit &Unit) override;                                                                         
    void RemoveRejection(CUnit &Unit) override;                                                                         
    void AddRejection(CUnit &Unit) override;                                                                         
    virtual CAIPlanType GetPlanType() = 0;                                                                             
    virtual bool IsMilitaryPlan();                                                                               
    virtual CAIMilitaryPlan &GetMilitaryPlan();                                                                   
    virtual bool IsExplorePlan();                                                                               
    virtual CAIExplorePlan &GetExplorePlan();                                                                    

protected:
    uint8_t m_pad0xa41[0xa58 - 0xa41];          
    CAIRequest *_pRequest[6];                                                       
    uint8_t m_pad0xa88[0xaa0 - 0xa88];          
};
