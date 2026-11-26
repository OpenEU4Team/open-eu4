#pragma once

#include "clausewitz/clausewitzlib/id.h"
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

class CAlliance;
class CArmy;
class CBattleResult;
class CCombat;
class CCountry;
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
class CAIStrategy;
class CAITransportPlan;
class CEU3AI;
enum CAINotificationType : int;
enum LeaderType : int;

class CAISubscriber : public CReferenceObject
{
public:
    ~CAISubscriber() override;                                                  

    virtual bool WantTicks();                                                                                
    virtual void Update(CCountryFlagCache<char> &);                                                                             

    virtual void NotifyTick(CCountryFlagCache<char> &AccessCache);                                                                                  

    virtual void NotifyArmyMoved(CArmy &Army);                                                                           
    virtual void NotifyFleetMoved(CNavy &Fleet);                                                                          
    virtual void NotifyArmyStopped(CArmy &Army);                                                                           
    virtual void NotifyFleetStopped(CNavy &Fleet);                                                                          
    virtual void NotifyArmyCreated(CArmy &Army);                                                                           
    virtual void NotifyFleetCreated(CNavy &Fleet);                                                                          
    virtual void NotifyArmyDeleted(CArmy &Army);                                                                           
    virtual void NotifyFleetDeleted(CNavy &Fleet);                                                                          

    virtual void NotifyArmyConstructionCancelled(CArmy &Army);                                                                                       

    virtual void NotifyFleetConstructionCancelled(CNavy &Fleet);                                                                                        

    virtual void NotifyArmyInCombat(CArmy &Army);                                                                                  
    virtual void NotifyFleetInCombat(CNavy &Fleet);                                                                                 
    virtual void NotifyArmyFinishedCombat(CArmy &Army);                                                                                  
    virtual void NotifyFleetFinishedCombat(CNavy &Fleet);                                                                                 

    virtual void NotifyBattleResult(CCountryTag Country, CCombat &Combat, CBattleResult &result);                                                                                                         

    virtual void NotifyHostileMovement(CCountry &Against, CCountry &Country, CProvince &Province, float vIntensity);                                                                                               

    virtual void NotifyRevoltOccurred(CProvince &Prov);                                                                                             
    virtual void NotifySiegeOccurred(CProvince &Prov);                                                                                             
    virtual void NotifyBlockade(CProvince &Prov, bool IsBlockaded);                                                                            

    virtual void NotifyProvinceDiscovered(CCountry &Country, CProvince &Province);                                                                                               

    virtual void NotifyCBRecieved(CCountry &Country1, CCountry &Country2);                                                                                 
    virtual void NotifyTradeRefusal(CCountry &Country1, CCountry &Country2);                                                                                 
    virtual void NotifyPeace(CCountry &Country1, CCountry &Country2);                                                                                 
    virtual void NotifyReligionChanged(CCountry &Country1);                                                                                                  

    virtual void NotifyJoinAlliance(CCountry &Country1, CAlliance &Alliance);                                                                                         

    virtual void NotifyLeftAlliance(CCountry &Country1, CAlliance &Alliance);                                                                                         

    virtual void NotifyRoyalMarriage(CCountry &Country1, CCountry &Country2);                                                                                  

    virtual void NotifyRoyalMarriageCancelled(CCountry &Country1, CCountry &Country2);                                                                                           

    virtual void NotifyVassalization(CCountry &Country1, CCountry &Country2);                                                                                  

    virtual void NotifyVassalizationCancelled(CCountry &Country1, CCountry &Country2);                                                                                           

    virtual void NotifyColonialAction(CCountry &Country1, CProvince &Province);                                                                                           

    virtual void NotifyMerchantPlacement(CCountry &Country1, CProvince &Province);                                                                                              

    virtual void NotifyLeaderArrived(CCountry &Country, LeaderType t);                                                                                           

    static bool TotalShutdown();                                                    
    void Tick(int);                                                       

protected:
    void Subscribe(CAINotificationType, bool);                                                                    
    void Unsubscribe(CAINotificationType);                                                                          

    uint8_t m_pad0x15[0x18 - 0x15];         
    int _nTick;                                                                      
};
