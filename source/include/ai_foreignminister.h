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
#include "pdx_def_val.h"
#include "unitlist.h"
#include <cstdint>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

class CAlliance;
class CBattleResult;
class CCombat;
class CCountry;
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

class CAIForeignMinister : public CAIMinister
{
public:
    struct SGreatPowerTarget
    {
        CArray<CCountryTag> _Tags;                                    
        float _vAllianceStrength;                                       
        float _vEnemyStrength;                                          
        TPdxDefVal<int, 0, 1> _nHumans;                                       

        SGreatPowerTarget();                                                                    
        float GetBalance() const;                                                                          
    };

    ~CAIForeignMinister() override;                                                       

    void NotifyTick(CCountryFlagCache<char> &AccessCache) override;                                                                                       

    void NotifyBattleResult(CCountryTag country, CCombat &Combat, CBattleResult &Result) override;                                                                                                              

    void NotifyHostileMovement(CCountry &Against, CCountry &Country, CProvince &Province, float vIntensity) override;                                                                                                    

    void NotifyRevoltOccurred(CProvince &Prov) override;                                                                                      

    void NotifyCBRecieved(CCountry &Country1, CCountry &Country2) override;                                                                                    

    void NotifyPeace(CCountry &Country1, CCountry &Country2) override;                                                                               

    void NotifyJoinAlliance(CCountry &Country, CAlliance &Alliance) override;                                                                                              

    void NotifyLeftAlliance(CCountry &Country1, CAlliance &Alliance) override;                                                                                              

    void Cleanup() override;                                                                                        
    void Start() override;                                                                                        
    void NotifyDiplomatSent(int Country) override;                                                                              
    void NotifyDiplomatNotSent(int nCountry) override;                                                                             

protected:
    uint8_t m_pad0xa41[0xb48 - 0xa41];          
};
