#pragma once
#include "cached_db_ptr.h"
#include "clausewitz/pdx_core/fixedpoint.h"
#include "engine.h"
#include "unit.h"
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

#include "unit.h"

class CArmy : public CUnit
{
public:
    SelectableType GetType() const override;                                                            
    void OnSelect(CSelectionList *pList) override;                                                                      
    void OnDeSelect() override;                                                                            
    void ReadMember(CReader &Reader, int Type) override;                                                            
    void WriteMembers(CWriter &Writer) override;                                                                     
    void WriteAIData(CWriter &Writer) override;                                                                     
    bool IsParsedCorrectly() const override;                                                                       
    CArmy *GetArmy() override;                                                                         
    CNavy *GetNavy() override;                                                                         
    const CArmy *GetArmy() const override;                                                              
    const CNavy *GetNavy() const override;                                                              
    bool IsArmy() const override;                                                                      
    bool IsCondottieri() const override;                                                                       
    bool IsPendingCondottieri() const override;                                                                       
    CCountryTag PendingCondottieri() const override;                                                                
    CFixedPoint CalcMaintenanceCost() const override;                                                                
    void DailyUpdate() override;                                                                            
    CString GetAttritionDescription() const override;                                                                     

    SManpowerAndSailors ComputeReinforceAndRepair(CFixedPoint, CFixedPoint, std::vector<std::pair<CSubUnit *, SManpowerAndSailors>> *, bool) const override;                                                                                                                                               

    void ReinforceAndRepair(CFixedPoint &vCountryManpower, CFixedPoint &vCountrySailors) override;                                                                            

    CFixedPoint GetMonthlyMenForReinforcement() const override;                                                                        

    CFixedPoint GetAttritionLevel(CString *pDebug, CString *pDesc, const CProvince *pLocation, int nMonthsAtSeaOverride) const override;                                                                                    

    CFixedPoint GetMovementSpeed(const CModifier &Modifier, const CProvince *pWhere, bool LogRandom) const override;                                                                                   

    bool HaveAccess(const CProvince *pTarget, bool bDebug, bool bWalkOnWater, bool bIgnorePreviousZoC, CCountryTag HiredByCountryOverride, CString *pDesc, bool bIgnoreExile, CCountryFlagCache<char> *pAccessCache) const override;                                                                                                                 

    CString GetVisualStrength() const override;                                                            

    bool CanBuildTo(const CSubUnitDefinition *pDef, CToolTip *pToolTip) const override;                                                                                    

    CCountryTag GetAllegiance() const override;                                                        

    bool CanConsolidate(bool bIgnoreZeroStrengthRegiments, CString *pDesc) const override;                                                                  

    CFixedPoint DerivedApplyAttrition(CSubUnit *pSub, int nRandom, const CPdxArray<CWarParticipant *, int> &, bool &) override;                                                                                                            

    int GetNumUnitsQueuedToMerge(SubUnitType UnitType) const override;                                                                               

    int GetNumUnitsQueuedToMerge() const override;                                                                   

    CNavy *GetLoadedOn() const;                                                  
    bool IsLoaded() const;                                                   
    void SetAIDivision(int, int, bool);                                             

    inline int GetTransportTarget();                                                            
    inline void SetTransportTarget(int nTransTarget);                                           
    inline int GetAIDivisionAt() const;                                                      
    inline int GetAIDivisionOf() const;                                                      
    inline bool IsAIDivisionExpectingSiege() const;                                                     
    inline bool IsRelocating() const;                                                     
    inline const CEU3Date &GetRelocationDate() const;                                          
    inline CProvince *GetRelocationLocation() const;                                               

private:
    CNavy *_pLoadedOn;                                                      
    CCountryTag _HiredByCountry;                                                           
    uint8_t m_pad0x220[0x230 - 0x220];                          
    CProvince *_pRelocationProvince;                                                  
    CEU3Date _RelocationDate;                                                     
    CEU3Date _RelocationStartDate;                                                      
    TPdxDefVal<bool, 0, 1> _bIsRelocating;                                      
    uint8_t m_pad0x241[0x242 - 0x241];                          
    TPdxDefVal<bool, 0, 1> _bAIDivisionExpectingSiege;                                       
    uint8_t m_pad0x243[0x248 - 0x243];                          
    TPdxDefVal<int, 0, 1> _nTransTarget;                                       
    uint8_t m_pad0x24c[0x250 - 0x24c];                          
    TPdxDefVal<int, 0, 1> _nAIDivisionAt;                                       
    TPdxDefVal<int, 0, 1> _nAIDivisionOf;                                       
    uint8_t m_pad0x258[0x260 - 0x258];                          
};

#include "army.tpp"
