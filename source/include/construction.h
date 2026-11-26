#pragma once
#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include "engine.h"
#include "selectable.h"
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

class CConstruction : public CPersistent, public CSelectable
{
public:
    ~CConstruction() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                              
    void ReadMember(CReader &Reader, int Type) override;                                                                     
    void InitPostRead() override;                                                                                     

    virtual bool OnCreation(bool) = 0;                                                                             
    virtual bool OnFinish() = 0;                                                                                

    virtual bool IsParsedCorrectly(const CProvince *pLocation) const;                                                                               

    virtual void Cancel() = 0;                                                                                       
    virtual CString GetCancelingDesc() const;                                                                   

    SelectableType GetType() const override;                                                                              
    bool IsSelectable() const override;                                                                                         
    void OnSelect(CSelectionList *pList) override;                                                                                        
    void OnDeSelect() override;                                                                                              
    bool PerformAction(CProvince *pTarget, bool status) override;                                                                          
    const CString &GetName() const override;                                                                              

    virtual void OnTagChange(CCountryTag OldTag, CCountryTag NewTag);                                                                            

    virtual EConstructionType GetConstructionType() const = 0;                                                                                       

    virtual bool Stall();                                                      

    virtual bool ShouldReloadForCountry(CCountryTag Country);                                                                                    

    CToolTip GetTooltip(const CProvince *pTarget) const override;                                                                        

    virtual void SetCountry(CCountryTag tag);                                                                                
    virtual void Update(const CEU3Date &Date);                                                                        
    virtual bool DoEnd(const CEU3Date &today) const;                                                                  
    virtual CString GetActivityDescription() const;                                                                           
    virtual const CProvince *GetFromProvince() const;                                                                  
    virtual const CProvince *GetToProvince() const;                                                                   
    virtual CFixedPoint GetProgress() const;                                                                        
    virtual bool IsTraveling() const;                                                                               

    CCountryTag GetCountry() const;                                                  

private:
    uint8_t m_pad0x19[0x80 - 0x19];         
};

class CMilitaryConstruction : public CConstruction
{
public:
    ~CMilitaryConstruction() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                      
    void ReadMember(CReader &Reader, int Type) override;                                                                             
    void InitPostRead() override;                                                                                             
    bool OnCreation(bool bIsLoading) override;                                                                              
    bool OnFinish() override;                                                                                            

    bool IsParsedCorrectly(const CProvince *pLocation) const override;                                                                                       

    void Cancel() override;                                                                                               
    CString GetCancelingDesc() const override;                                                                                        
    EConstructionType GetConstructionType() const override;                                                                              

    CToolTip GetTooltip(const CProvince *pTarget) const override;                                                                                

    const CSubUnitDefinition *GetSubUnit() const;                                                          

private:
    uint8_t m_pad0x80[0xc8 - 0x80];         
};

class CColonyConstruction : public CConstruction
{
public:
    ~CColonyConstruction() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                    
    void ReadMember(CReader &Reader, int Type) override;                                                                           
    bool OnCreation(bool bIsLoading) override;                                                                            
    bool OnFinish() override;                                                                                          
    void Cancel() override;                                                                                          

    EConstructionType GetConstructionType() const override;                                                                            

    CToolTip GetTooltip(const CProvince *pTarget) const override;                                                                              

    void Update(const CEU3Date &Date) override;                                                                              
    bool DoEnd(const CEU3Date &today) const override;                                                                        
    CString GetActivityDescription() const override;                                                                                 
    const CProvince *GetFromProvince() const override;                                                                        
    const CProvince *GetToProvince() const override;                                                                         
    bool IsTraveling() const override;                                                                                     

    virtual CFixedPoint GetColonistChance(CString *pDesc) const;                                                                                   

    virtual const CCulture *GetColonistCulture() const;                                                                              
    virtual const CReligion *GetColonistReligion() const;                                                                             
    virtual bool CanCancel(CString *Tooltip) const;                                                                                

private:
    uint8_t m_pad0x80[0x88 - 0x80];         
};
