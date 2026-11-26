#pragma once
#include "clausewitz/clausewitzlib/ref.h"
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

class CWarGoal : public CPersistent
{
public:
    ~CWarGoal() override;                                                                           
    void WriteMembers(CWriter &Writer) override;                                                                        
    void ReadMember(CReader &Reader, int Type) override;                                                               

    virtual CString GetName() const;                                                                   
    virtual CString GetDesc() const;                                                                   
    virtual CString GetWarName() const;                                                                    
    virtual const CWarGoalType *GetType() const;                                                       
    virtual void SetType(const CWarGoalType *pType);                                                                 
    virtual int GetWarGoalToken() const;                                                                        
    virtual bool Validate() const;                                                                      
    virtual bool IsValid() const;                                                                      

    virtual void CalculateWarScore(CCountryTag Attacker, CCountryTag Defender, CFixedPoint &vAttackerWarScore, CFixedPoint &vDefenderWarScore, CString *pOptionalDesc) const;                                                                                                       

    virtual void CalculateController(bool &bAttackerControls, bool &bDefenderControls, CString *pOptionalDesc) const;                                                                             

    virtual void CalculateBattleScoreBonus(const CBattleResult *pResult, CFixedPoint &vAttacker, CFixedPoint &vDefender, CString *pOptionalDesc) const;                                                                                                                

    virtual CFixedPoint GetTickingWarScoreFactor() const;                                                                     

    virtual CFixedPoint GetProgress(CCountryTag Tag, CString *pOptionalDesc) const;                                                                             

    virtual CWarGoal *CreateFallbackWarGoal() const;                                                                           
    virtual void SetProvince(int nProvince);                                                                        
    virtual void SetCountryTag(const CCountryTag CountryTag);                                                                     
    virtual void SetRegion(const CRegion *pRegion);                                                                       
    virtual void SetContinent(const CContinent *pContinent);                                                                      
    virtual int GetProvince() const;                                                                                 
    virtual CCountryTag GetCountryTag() const;                                                                         
    virtual const CRegion *GetRegion() const;                                                                     
    virtual const CContinent *GetContinent() const;                                                                    

    virtual void SetRebelFaction(CRef<CRebelFaction> pRebelFaction);                                                                                 

    virtual CRef<CRebelFaction> AccessRebelFaction() const;                                                                
    virtual void SetCasusBelli(const CCasusBelliType *pCB);                                                                            
    virtual const CCasusBelliType *GetCasusBelli() const;                                                             
    virtual void SetWarGoalSubject(CCountryTag PartnerTag);                                                                          
    virtual CCountryTag GetWarGoalSubject() const;                                                                        
    virtual void InitNullWarGoal();                                                                                    
    static int GetWarGoalToken(WAR_GOAL_TYPES Type);                                                                      

private:
    uint8_t m_pad0xc[0x60 - 0xc];        
};
