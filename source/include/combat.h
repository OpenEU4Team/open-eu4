#pragma once
#include "clausewitz/clausewitzlib/id.h"
#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/fixedpoint.h"
#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include "enums.h"
#include "unitlist.h"
#include <cstdint>
#include <vector>

#define HELPER inline

class CUnit;
class CProvince;
class CCombatant;
class CEU3Idler;
class CToolTip;
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

class CCombat : public CReferenceObject
{
public:
    void WriteMembers(CWriter &Writer) override;                                                                       
    void ReadMember(CReader &Reader, int Type) override;                                                              
    void InitPostRead() override;                                                                              
    bool Create() override;                                                                             

    virtual void CreateCombatants();                                                                               
    virtual int GetToken() = 0;                                                                                         
    virtual CombatType GetCombatType() const = 0;                                                                              
    virtual void ArrangeBattleField();                                                                               
    virtual bool CanLeaveCombat() const;                                                                          
    virtual void OnStart();                                                                              
    virtual void OnEnd();                                                                              
    virtual bool DailyUpdate(CEU3Idler *pGraphicsOwner);                                                                
    virtual void SetLocation(CProvince *pProvince);                                                                     
    virtual bool TryToJoin(CUnit *pUnit);                                                                        

    inline const CCombatant &GetAttacker() const;                                        
    inline const CCombatant &GetDefender() const;                                      

protected:
    uint8_t m_pad0x15[0x3];          
    CCombatant *_pAttacker;                               
    CCombatant *_pDefender;                               
    uint8_t m_pad0x28[0x18];         
};

class CCombatantCountry : public CPersistent
{
public:
    void WriteMembers(CWriter &Writer) override;                                                                                  
    void ReadMember(CReader &Reader, int Type) override;                                                                         

    HELPER CCountryTag GetCountryTag() const;

protected:
    CCountryTag _CountryTag;                          
    CFixedPoint _StartingMorale;                           
};

class CCombatantCountryArray : public CArray<CCombatantCountry>
{
public:
    bool Contains(CCountryTag Tag) const;                                                                         
    CList<CCountryTag> GetCountryList() const;                                                               
};

class CCombatant : public CPersistent
{
public:
    ~CCombatant() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                           
    void ReadMember(CReader &Reader, int Type) override;                                                                  

    virtual const CNavalCombatant *GetNavalCombatant() const = 0;                                                                              
    virtual const CLandCombatant *GetLandCombatant() const = 0;                                                                               
    virtual void ArrangeBattleField(CCombatant *) = 0;                                                                                                     

    virtual bool IsNavy();                                                                                  
    virtual void AddUnit(CUnit *pUnit, bool bAddCount);                                                             
    virtual void RemoveUnit(CUnit *pUnit, bool bSilent);                                                                
    virtual void OnDelete(CSubUnit *) = 0;                                                                                          
    virtual void DailySetup();                                                                                   

    virtual void DailyUpdate(CCombatant *, CombatPhase, const CLeader *, const CLeader *) = 0;                                                                                                             

    virtual void PreDailyUpdate(CCombatant *pOpponent);                                                                            
    virtual bool IsValidOpponent(const CUnit *pUnit) const;                                                                               
    virtual CFixedPoint CalcAverageMorale() const;                                                                                   
    virtual bool HasMoraleCollapsed() const;                                                                                          
    virtual bool HasAnyTroopsAbleToFight() const;                                                                                          
    virtual void CheckLeaderSkillGain(CFixedPoint vTraditionGained);                                                                               
    virtual int CalcCheckSum(int nCheckSum) const;                                                                              

    inline const CUnitList &GetUnits() const;                                                                      
    inline const CCombatantCountryArray &GetParticipatingCountries() const;                                                         

protected:
    virtual void AddParticipatingCountry(CCountryTag Tag);                                                                                      
    virtual void AddParticipatingCountries(CUnit *pUnit);                                                                                   
    virtual CFixedPoint GetMoraleFromCountry(CCountryTag) = 0;                                                                                            

    uint8_t m_pad0xc[0xd8 - 0xc];                          
    CUnitList _Units;                                               
    CCombatantCountryArray _ParticipatingCountries;                                   
    uint8_t m_pad0x108[0x160 - 0x108];                       
};

class CLandCombat : public CCombat
{
public:
    ~CLandCombat() override;                                                                  
    void CreateCombatants() override;                                                                         
    int GetToken() override;                                                                         
    CombatType GetCombatType() const override;                                                              
    void ArrangeBattleField() override;                                                                         
    void OnStart() override;                                                                        
    void OnEnd() override;                                                                        
};

class CNavalCombat : public CCombat
{
public:
    ~CNavalCombat() override;                                                                  
    void CreateCombatants() override;                                                                          
    int GetToken() override;                                                                          
    CombatType GetCombatType() const override;                                                               
    void OnStart() override;                                                                         
};

class CSiegeCombat : public CCombat
{
public:
    ~CSiegeCombat() override;                                                                             
    void WriteMembers(CWriter &Writer) override;                                                                              
    void ReadMember(CReader &Reader, int Type) override;                                                                     
    void CreateCombatants() override;                                                                                     
    int GetToken() override;                                                                                     
    CombatType GetCombatType() const override;                                                                          
    bool CanLeaveCombat() const override;                                                                                
    void OnStart() override;                                                                                    
    bool DailyUpdate(CEU3Idler *pGraphicsOwner) override;                                                                      
    void SetLocation(CProvince *pProvince) override;                                                                           
    bool TryToJoin(CUnit *pUnit) override;                                                                              

    int GetMaxSiegeMemory() const;                                                        

    inline int GetCachedSiegeImpact() const;                                                    

private:
    uint8_t m_pad0x40[0x54 - 0x40];         
    int _nSiegeImpact;                                     
    uint8_t m_pad0x58[0x70 - 0x58];         
};

#include "combat.tpp"
