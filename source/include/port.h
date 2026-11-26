#pragma once
#include "engine.h"
#include "selectable.h"
#include "unitlist.h"
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

class CPort : public CUnitList, public CSelectable
{
public:
    ~CPort() override;                                                                                    
    SelectableType GetType() const override;                                                                     
    bool IsSelectable() const override;                                                                                
    CToolTip GetTooltip(const CProvince *pTarget) const override;                                                               
    void OnSelect(CSelectionList *pList) override;                                                                               
    void OnDeSelect() override;                                                                                     
    bool PerformAction(CProvince *pTarget, bool status) override;                                                                 
    const CString &GetName() const override;                                                                     

    int GetSeaZone();                                        

protected:
    int _nSeaZone;                                     
    uint8_t m_pad0x2c[0x30 - 0x2c];         
};
