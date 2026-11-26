#pragma once
#include "clausewitz/clausewitzlib/localize.h"
#include "clausewitz/pdx_core/pdxhybridarray.h"
#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include "engine.h"
#include "modifier.h"

class CCountry;
class CCommand;
class CProvince;
class CSubUnitDefinition;
class CString;
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
template <typename T, typename I>
class CPdxArray;
enum class SubUnitType : int;

bool FlagshipPrerequisitesFulfilled(const CCountry &Country, bool bCheckUnitCost, SubUnitType ShipType, CString *pDesc);                                                                                     

bool CanBuildFlagshipInProvince(const CCountry &Country, const CProvince *pProvince, bool bCheckUnitCost, SubUnitType ShipType, CString *pDesc);                                                                                             

CCommand *BuildFlagshipConstructionCommandForProvince(const CCountry &Country, const CProvince *pProvince, const CSubUnitDefinition *pDef, const CPdxArray<CString, int> &Modifications, const CString &Name);                                                                                                                                         

class CFlagship : public CPersistent
{
public:
    ~CFlagship() override;                                                                           
    void WriteMembers(CWriter &Writer) override;                                                                         
    void ReadMember(CReader &Reader, int Type) override;                                                                
    void InitPostRead() override;                                                                                

    inline CCountryTag GetOriginalOwner() const;                                               
    inline void SetOriginalOwner(CCountryTag Tag);                                             

private:
    uint8_t m_pad0xc[0x30 - 0xc];           
    CCountryTag _OriginalOwner;                               
    uint8_t m_pad0x38[0x110 - 0x38];         
};

class CFlagshipModification : public CPersistent
{
public:
    void ReadMember(CReader &Reader, int Type) override;                                                                             

    inline const CString &GetTag() const;                                                            
    inline const CString GetName() const;                                                             
    inline const CStaticModifier &GetModifier() const;                                                    

private:
    uint8_t m_pad0xc[0x10 - 0xc];             
    const CString _Tag;                                           
    CStaticModifier _Modifier;                                                        
    uint8_t m_pad0x1a0[0x280 - 0x1a0];          
};

struct SAIDesiredFlagship
{
    SubUnitType _Type;                                                                                  
    CPdxHybridArray<const CFlagshipModification *, 3, int> _Modifications;                                 
};

class CFlagshipModificationDatabase
{
public:
    static const CFlagshipModificationDatabase *GetInstance();                                                                  

    SAIDesiredFlagship GetDesiredFlagshipForAI(CCountryTag Tag) const;                                                                                           

private:
    uint64_t m_pad0x0[0x51a0 / 8];        
};

#include "flagship.tpp"
