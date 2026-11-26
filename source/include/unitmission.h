#pragma once
#include "clausewitz/pdx_core/persistent.h"
#include "engine.h"
#include <cstdint>

class CToolTip;
class CWar;
class CCountryTag;
enum WAR_GOAL_TYPES : int;
template <class T, class I>
class CPdxArray;
class CFixedPoint;
class CEU3Date;
class CAIAgent;
class CArmy;
class CNavy;
class CUnit;
class CSubUnit;
class CSubUnitDefinition;
class CShip;
class CRegiment;
class CCombat;
class CCombatant;
class CLandCombatant;
class CNavalCombatant;
class CSiegeCombat;
class CCountry;
class CProvince;
class CLeader;
class CModifier;
class CStaticModifier;
class CEU3Idler;
class CMercenaryCompany;
struct CMercenaryComposition;
class CUnitMission;
class CAutomaticFleetTransport;
class CWarParticipant;
class CTradeNode;
class CWarGoal;
class CWarGoalType;
class CRegion;
class CContinent;
class CRebelFaction;
class CCasusBelliType;
class CBattleResult;
class CCulture;
class CReligion;
class CControllerArea;
class CStrengthFactorCache;
class CFlagshipModification;
struct SAdjacency;
struct SManpowerAndSailors;
struct SAIDesiredFlagship;
struct SPeaceTreatyTag;
struct CPixel;
template <class T>
class CCountryFlagCache;
template <class T, class E, unsigned long N>
class CEnumIndexedArray;
enum class SubUnitType : int;
enum class ESpecialUnitCategory : int;
enum ModifierType : int;
enum class ESeaType : char;
enum class EStopMissionReason : unsigned char;
enum EConstructionType : int;
enum EUnitMission : int;
enum CombatPhase : int;

class CUnitMission : public CPersistent
{
public:
    enum EOutlinerFrame : int
    {
        OUTLINER_NONE = 0,
        OUTLINER_MOVING = 1,
        OUTLINER_PATROL = 2,
        OUTLINER_PROTECT_TRADE = 3,
        OUTLINER_HUNT_REBELS = 4,
        OUTLINER_FORCED_MARCH = 5,
        OUTLINER_PRIVATEER = 6,
        OUTLINER_HUNT_PIRATES = 7,
        OUTLINER_EXPLORE = 8,
        OUTLINER_EVADE = 9,
        OUTLINER_HUNT = 10,
        OUTLINER_BLOCKADE = 11,
        OUTLINER_INTERCEPT = 12,
        OUTLINER_HIDING = 13,
        OUTLINER_REPAIR = 14,
        OUTLINER_IN_SIEGE = 15,
        OUTLINER_DRILL_ARMY = 16,
        OUTLINER_CARPET_SIEGE = 17,
        OUTLINER_MISSION_NUM_OF = 18,
    };

    void WriteMembers(CWriter &) override = 0;                                                                                  
    void ReadMember(CReader &, int) override = 0;                                                                              

    virtual int GetButtonIconFrame(const CUnit *pUnit) const;                                                                           

    virtual CUnitMission::EOutlinerFrame GetOutlinerIconFrame(const CUnit *pUnit) const;                                                                             

    virtual EUnitMission GetType() const;                                                        
    virtual int GetToken() const;                                                                 
    virtual bool IsNaval() const;                                                                
    virtual void OnStart(CUnit *pUnit);                                                               
    virtual bool CanCancel() const;                                                                

    virtual EStopMissionReason ShouldCancel(const CUnit *pUnit) const;                                                                     

    virtual EStopMissionReason OnDailyUpdate(CUnit *pUnit);                                                                            
    virtual EStopMissionReason OnMonthlyUpdate(CUnit *pUnit) const;                                                                       
    virtual void OnProvinceArrival(CUnit *pUnit);                                                                                          

    virtual void GetShortDesc(const CUnit *pUnit, CString &Desc) const;                                                                              

    virtual void GetTooltip(const CUnit *pUnit, CToolTip &TipOut) const;                                                                             

    virtual void GetMapTooltip(const CUnit *pUnit, CString &Desc) const;                                                                               

    virtual bool CanMergeWith(const CUnitMission *pOtherMission) const;                                                                 

private:
    uint8_t m_pad0xc[0x10 - 0xc];        
};
