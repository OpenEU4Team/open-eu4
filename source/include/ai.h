#pragma once

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
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

class CCommand;
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
class CAITransportPlan;

class CEU3AI
{
public:
    CEU3AI(CCountryTag tag);                                          
    CAIKing *GetKing() const;                                      

protected:
    uint8_t m_pad0x0[0x28 - 0x0];           
    CAIKing *_pKing;                                                   
    uint8_t m_pad0x30[0xa58 - 0x30];         
};

void PostAICommand(CCommand *pCommand);                                                            
void PostAICommand(std::unique_ptr<CCommand> pCommand);                                                                                  

bool PostAICommandOnlyIfValid(std::unique_ptr<CCommand> pCommand);                                                                                             
