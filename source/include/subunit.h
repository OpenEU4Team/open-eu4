#pragma once
#include "clausewitz/clausewitzlib/id.h"
#include "clausewitz/pdx_core/fixedpoint.h"
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

class CSubUnitDefinition : public CPersistent
{
public:
    void WriteMembers(CWriter &Writer) override;                                                                                   
    void ReadMember(CReader &Reader, int Type) override;                                                                          
    void InitPostRead() override;                                                                                          

    virtual const CString GetName() const;                                                                     
    virtual const CString &GetTag() const;                                                                    
    virtual const CString GetTypeName() const;                                                                      
    virtual const CString GetDescription() const;                                                                      
    virtual const CString GetStatsDesc() const;                                                                      
    virtual bool IsValid() const;                                                                              

    int GetOffensiveMorale() const;                                                                                            
    int GetDefensiveMorale() const;                                                                                            
    int GetOffensiveFire() const;                                                                                            
    int GetDefensiveFire() const;                                                                                            
    int GetOffensiveShock() const;                                                                                            
    int GetDefensiveShock() const;                                                                                            
    CFixedPoint GetSailSpeed() const;                                                                                    
    static CString GetTypeName(SubUnitType type);                                                                                  
    CFixedPoint GetPowerImpact(CCountryTag) const;                                                                                     
    CFixedPoint GetPowerImpact(const CModifier &Modifier) const;                                                                      

    CFixedPoint GetPowerImpact(const CModifier &Modifier, const CModifier &SecondModifier) const;                                                                         

    bool IsRegiment() const;                                                       

    inline SubUnitType GetType() const;                                                              
    inline const CFixedPoint &GetDefaultMenCost() const;                                                       

protected:
    SubUnitType _Type;                            
    uint8_t m_pad0x10[0x50 - 0x10];         
    CFixedPoint _vDefaultMenCost;                                                   
    uint8_t m_pad0x54[0xf0 - 0x54];         
};

class CSubUnit : public CReferenceObject
{
public:
    ~CSubUnit() override;                                                                           
    void WriteMembers(CWriter &Writer) override;                                                                        
    void ReadMember(CReader &Reader, int Type) override;                                                               
    void InitPostRead() override;                                                                               

    virtual const CShip *GetShip() const = 0;                                                                        
    virtual const CRegiment *GetRegiment() const = 0;                                                                     
    virtual bool IsRegiment() const;                                                                               
    virtual bool IsShip() const;                                                                              
    virtual CFixedPoint GetDefaultMorale() const = 0;                                                                          
    virtual CString GetVisualStrength() const = 0;                                                                              
    virtual CFixedPoint GetSizeModifier() const;                                                                        
    virtual void GetTooltip(CToolTip &) = 0;                                                                                     
    virtual CFixedPoint GetMoraleModifier() const;                                                                        
    virtual CFixedPoint GetTradePower(CString *pDesc) const;                                                                  

    void LoseStrength(CFixedPoint v);                                                                 
    void SetMorale(CFixedPoint);                                                                  
    ESpecialUnitCategory GetCategory() const;                                             

    static CFixedPoint GetManpowerOrSailorsUsagePercentage(const CCountry &Country, ESpecialUnitCategory Category);                                                                                                    

    static bool UseMercenaryReinforceCost(ESpecialUnitCategory);                                                                               

    CFixedPoint CalcStrengthFactor(const CCountry &TechCountry, const CProvince *pProvinceOrNullptr, bool bLowMoralePenalty, const CSubUnitDefinition *pReplacementDefinition) const;                                                                                                 

    CFixedPoint CalcStrengthFactor(const CCountry &TechCountry, const CProvince *pProvinceOrNullptr, bool bLowMoralePenalty, const CSubUnitDefinition *pReplacementDefinition, CEnumIndexedArray<std::pair<CSubUnitDefinition const *, CFixedPoint>, SubUnitType, 3> &, CEnumIndexedArray<std::pair<CSubUnitDefinition const *, CFixedPoint>, ESpecialUnitCategory, 23> &) const;                                                                                                                                                                                                     

    inline const CSubUnitDefinition *GetDefinition() const;                                         
    inline CFixedPoint GetStrength() const;                                                       
    inline CFixedPoint GetMorale() const;                                                       

protected:
    uint8_t m_pad0x15[0x18 - 0x15];                 
    const CSubUnitDefinition *_pDefinition;                      
    CFixedPoint _vStrength;                                    
    CFixedPoint _vMorale;                                    
    uint8_t m_pad0x28[0x88 - 0x28];                 
};

class CShip : public CSubUnit
{
public:
    void WriteMembers(CWriter &Writer) override;                                                                          
    void ReadMember(CReader &Reader, int Type) override;                                                                 
    const CShip *GetShip() const override;                                                                   
    const CRegiment *GetRegiment() const override;                                                                
    bool IsShip() const override;                                                                           
    CFixedPoint GetDefaultMorale() const override;                                                                     
    CString GetVisualStrength() const override;                                                                         
    void GetTooltip(CToolTip &Tip) override;                                                                             
    CFixedPoint GetMoraleModifier() const override;                                                                     
    CFixedPoint GetTradePower(CString *pDesc) const override;                                                               

private:
    uint8_t m_pad0x88[0x90 - 0x88];         
};

#include "subunit.tpp"
