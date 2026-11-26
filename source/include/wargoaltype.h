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

class CWarGoalPeace : public CPersistent
{
public:
    void ReadMember(CReader &Reader, int Type) override;                                                                     

    void NotifyScriptedTreatiesLoaded();                                                                   

    bool AllowsPeaceOption(const SPeaceTreatyTag &Option, const CCasusBelliType *pCBType) const;                                                                                              

private:
    uint8_t m_pad0xc[0x238 - 0xc];        
};

class CWarGoalType : public CPersistent
{
public:
    void ReadMember(CReader &Reader, int Type) override;                                                                    

    virtual bool IsValid() const;                                                        

    void NotifyScriptedTreatiesLoaded();                                                                  

    const CWarGoalPeace &GetPeace(const CWar *pWar, CCountryTag Who, const bool bIsDemanding) const;                                                                  

    inline const CWarGoalPeace &GetPeace(bool bAttacker) const;                                           

private:
    uint8_t m_pad0xc[0x38 - 0xc];             
    CWarGoalPeace _AttackerPeace;                                           
    CWarGoalPeace _DefenderPeace;                                            
    uint8_t m_pad0x4a8[0x4d0 - 0x4a8];          
};

#include "wargoaltype.tpp"
