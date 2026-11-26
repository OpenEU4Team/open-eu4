#pragma once
#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
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

class CWar : public CPersistent
{
public:
    ~CWar() override;                                                                           
    void WriteMembers(CWriter &Writer) override;                                                                    
    void ReadMember(CReader &Reader, int Type) override;                                                           

    bool IsActive() const;                                                               
    const CArray<CCountryTag> *GetAttackers() const;                                          
    const CArray<CCountryTag> *GetDefenders() const;                                          
    CWarGoal *GetWarGoal() const;                                                           
    bool IsWarLeader(CCountryTag tag) const;                                                             

    CCountryTag GetTopOverlordInWarTogether(CCountryTag MyTag) const;                                                                     

    const CWarGoal *GetAppliedWarGoal(CCountryTag For, CCountryTag Versus) const;                                                              

    inline CCountryTag GetOriginalAttacker() const;                                             
    inline CCountryTag GetOriginalDefender() const;                                             

private:
    uint8_t m_pad0xc[0x140 - 0xc];            
    CCountryTag _OriginalAttacker;                                  
    CCountryTag _OriginalDefender;                                  
    uint8_t m_pad0x150[0x248 - 0x150];          
};

#include "war.tpp"
