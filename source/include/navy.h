#pragma once
#include "cached_db_ptr.h"
#include "clausewitz/pdx_core/fixedpoint.h"
#include "clausewitz/pdx_core/persistent.h"
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

class CNavy : public CUnit
{
public:
    class CMissionSettings : public CPersistent
    {
    public:
        void WriteMembers(CWriter &Writer) override;                                                                                       
        void ReadMember(CReader &Reader, int Type) override;                                                                              

        bool IsDefault() const;                                                         

    private:
        uint8_t m_pad0xc[0x18 - 0xc];        
    };

    enum ERepairThreshold : int
    {
        REPAIR_ANY_DAMAGE = 0,
        REPAIR_LOW_DAMAGE = 1,
        REPAIR_DEFAULT = 2,
        REPAIR_HIGH_DAMAGE = 3,
        REPAIR_NEVER = 4,
    };

    enum EAggressiveness : int
    {
        AGGRESSIVENESS_CAUTIOUS = 0,
        AGGRESSIVENESS_BOLD = 1,
        AGGRESSIVENESS_OBSOLETE = 2,
    };

    ~CNavy() override;                                                                           
    SelectableType GetType() const override;                                                            
    bool IsSelectable() const override;                                                                       
    void OnSelect(CSelectionList *pList) override;                                                                      
    void OnDeSelect() override;                                                                            
    void ReadMember(CReader &Reader, int Type) override;                                                            
    void WriteMembers(CWriter &Writer) override;                                                                     
    bool IsParsedCorrectly() const override;                                                                       
    CArmy *GetArmy() override;                                                                         
    CNavy *GetNavy() override;                                                                         
    const CArmy *GetArmy() const override;                                                              
    const CNavy *GetNavy() const override;                                                              
    bool IsNavy() const override;                                                                      

    void SetLocation(CProvince *pLocation, bool bSetForAttached) override;                                                               

    bool HasLowMorale() const override;                                                                          
    CFixedPoint CalcMaintenanceCost() const override;                                                                   
    void SetGraphicsOwner(CEU3Idler *pOwner) override;                                                                        
    CPixel GetPositionFor(CProvince *pTarget) override;                                                                     
    void DailyUpdate() override;                                                                               
    CFixedPoint MonthlyUpdate(bool bChangeMorale) override;                                                       
    CString GetAttritionDescription() const override;                                                                        

    SManpowerAndSailors ComputeReinforceAndRepair(CFixedPoint, CFixedPoint, std::vector<std::pair<CSubUnit *, SManpowerAndSailors>> *, bool) const override;                                                                                                                                               

    void ReinforceAndRepair(CFixedPoint &vCountryManpower, CFixedPoint &vCountrySailors) override;                                                                            

    CFixedPoint GetMonthlyMenForReinforcement() const override;                                                                        

    CFixedPoint GetAttritionLevel(CString *pDebug, CString *pDesc, const CProvince *pLocation, int nMonthsAtSeaOverride) const override;                                                                                    

    bool CanExploreNonAdjacentTI() const override;                                                                  

    CFixedPoint GetMovementSpeed(const CModifier &Modifier, const CProvince *pWhere, bool LogRandom) const override;                                                                                   

    bool IsMovementLocked() const override;                                                           

    bool HaveAccess(const CProvince *pTarget, bool bDebug, bool bWalkOnWater, bool bIgnorePreviousZoC, CCountryTag HiredByCountryOverride, CString *pDesc, bool bIgnoreExile, CCountryFlagCache<char> *pAccessCache) const override;                                                                                                                 

    CString GetVisualStrength() const override;                                                            

    bool CanBuildTo(const CSubUnitDefinition *pDef, CToolTip *pToolTip) const override;                                                                                    

    void ProcessMissionDaily() override;                                                             

    bool CanConsolidate(bool bIgnoreZeroStrengthRegiments, CString *pDesc) const override;                                                                  

    CFixedPoint DerivedApplyAttrition(CSubUnit *pSub, int nRandom, const CPdxArray<CWarParticipant *, int> &, bool &) override;                                                                                                            

    bool IsLegalToMoveNavy() const override;                                                            

    int GetNumUnitsQueuedToMerge(SubUnitType UnitType) const override;                                                                               

    int GetNumUnitsQueuedToMerge() const override;                                                                   

    bool CanLoad(const CUnit *pArmy, int nRegsOverride);                                            
    bool CanUnLoad(CProvince *pTarget) const;                                                            
    bool HasTroopsOn() const;                                                                     
    void RemoveOverload(bool log);                                                                  
    bool IsMothballing() const;                                                                     
    void SetMothballing(bool);                                                                      
    bool IsDamaged() const;                                                                    
    int GetAtSea() const;                                                                     
    void SetAtSea(int nSea);                                                                 
    void UseSailorsMonthly();                                                                          
    void ReturnSailors(CFixedPoint);                                                                           
    const CUnitList &GetUnitsOnBoard() const;                                                         
    CUnitList &GetUnitsOnBoard();                                                                    

    bool IsNavyAtDangerousSea(const CProvince *pLocation) const;                                                                  
    bool IsOnTradeNodeMission(const CTradeNode *pInTradeNode) const;                                                                

    SManpowerAndSailors ComputeReinforceAndRepair(CFixedPoint, CFixedPoint, CFixedPoint, std::vector<std::pair<CSubUnit *, SManpowerAndSailors>> *, bool) const;                                                                                                                                      

    inline int GetAIStraitTarget() const;                                                                     
    inline void SetAIStraitTarget(int nTarget);                                                               
    inline int GetAIBlockadeTarget() const;                                                                     
    inline void SetAIBlockadeTarget(int nTarget);                                                               
    inline const CArray<CProvince *> &GetBlockadedProvinces() const;                                              
    inline CFixedPoint GetCurrentBlockadeCapacity() const;                                                             

private:
    uint8_t m_pad0x210[0x228 - 0x210];               
    CArray<CProvince *> _BlockedProvinces;                                              
    uint8_t m_pad0x240[0x248 - 0x240];               
    CFixedPoint _vCurrentBlockadeCapacity;                                       
    uint8_t m_pad0x24c[0x270 - 0x24c];               
    TPdxDefVal<int, 0, 1> _nAIStraitTarget;                            
    int _nAIBlockadeTarget;                                              
    uint8_t m_pad0x278[0x280 - 0x278];               
};

#include "navy.tpp"
