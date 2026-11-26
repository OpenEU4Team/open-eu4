#pragma once
#include "clausewitz/clausewitzlib/id.h"
#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/fixedpoint.h"
#include "clausewitz/pdx_core/list.h"
#include "countrytag.h"
#include "engine.h"
#include "eu3date.h"
#include "selectable.h"
#include "unitlist.h"
#include <cstdint>
#include <vector>

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
class CMapArea;
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

struct SManpowerAndSailors
{
    CFixedPoint _vManpower;                            
    CFixedPoint _vSailors;                            
};

template <typename T>
class TUnsafeMove
{
public:
    HELPER T GetId() const;
    HELPER const CEU3Date &GetDate() const;
    HELPER CFixedPoint GetDistance() const;

protected:
    T _id;                                          
    CEU3Date _Date;                             
    CFixedPoint _vDist;                             
};

class CUnit : protected CArray<CSubUnit *>, public CSelectable, public CReferenceObject
{
public:
    enum EMoraleType : int
    {
        Simulation = 0,
        Display = 1,
    };

    ~CUnit() override;                                                                                     
    bool IsSelectable() const override;                                                                                 
    CToolTip GetTooltip(const CProvince *pTarget) const override;                                                                
    void OnSelect(CSelectionList *pList) override;                                                                                
    void OnDeSelect() override;                                                                                      
    bool PerformAction(CProvince *pTarget, bool bStatus) override;                                                                 

    bool CanPerformActionImmediately(CMultiStopMissionAndPerformAction &, CProvince *, bool) override;                                                                                                                   

    const CString &GetName() const override;                                                

    virtual CString GetTruncatedName(int nLength) const;                                                          

    void ReadMember(CReader &Reader, int Type) override;                                                            
    void WriteMembers(CWriter &Writer) override;                                                                     

    virtual void WriteAIData(CWriter &Writer);                                                                 
    virtual bool IsParsedCorrectly() const;                                                                   
    virtual CArmy *GetArmy() = 0;                                                                          
    virtual CNavy *GetNavy() = 0;                                                                          
    virtual const CArmy *GetArmy() const = 0;                                                               
    virtual const CNavy *GetNavy() const = 0;                                                               
    virtual bool IsArmy() const;                                                                  
    virtual bool IsNavy() const;                                                                  
    virtual bool IsCondottieri() const;                                                                   
    virtual bool IsPendingCondottieri() const;                                                                   
    virtual CCountryTag PendingCondottieri() const;                                                            

    virtual void SetLocation(CProvince *pLocation, bool bSetForAttached);                                                               

    virtual bool HasLowMorale() const;                                                                          
    virtual CFixedPoint CalcMaintenanceCost() const;                                                                   
    virtual void SetGraphicsOwner(CEU3Idler *pOwner);                                                                        
    virtual CPixel GetPositionFor(CProvince *pTarget);                                                                     
    virtual void DailyUpdate();                                                                               
    virtual CFixedPoint MonthlyUpdate(bool bChangeMorale);                                                       
    virtual CString GetAttritionDescription() const = 0;                                                                             

    virtual SManpowerAndSailors ComputeReinforceAndRepair(CFixedPoint, CFixedPoint, std::vector<std::pair<CSubUnit *, SManpowerAndSailors>> *, bool) const = 0;                                                                                                                                                        

    virtual void ReinforceAndRepair(CFixedPoint &, CFixedPoint &) = 0;                                                                                     

    virtual CFixedPoint GetMonthlyMenForReinforcement() const = 0;                                                                                 

    virtual CFixedPoint GetAttritionLevel(CString *, CString *, const CProvince *, int) const = 0;                                                                                             

    virtual bool CanExploreNonAdjacentTI() const;                                                                  

    virtual CFixedPoint GetMovementSpeed(const CModifier &, const CProvince *, bool) const = 0;                                                                                            

    virtual bool IsMovementLocked() const;                                                           

    virtual bool HaveAccess(const CProvince *pTarget, bool bDebug, bool bWalkOnWater, bool bIgnorePreviousZoC, CCountryTag HiredByCountryOverride, CString *pDesc, bool bIgnoreExile, CCountryFlagCache<char> *pAccessCache) const;                                                                                                                 

    virtual CString GetVisualStrength() const = 0;                                                                     

    virtual bool CanBuildTo(const CSubUnitDefinition *, CToolTip *) const = 0;                                                                                             

    virtual CCountryTag GetAllegiance() const;                                                                     
    virtual void ProcessMissionDaily();                                                                                 
    virtual bool CanConsolidate(bool, CString *) const = 0;                                                                           

    virtual CFixedPoint DerivedApplyAttrition(CSubUnit *, int, const CPdxArray<CWarParticipant *, int> &, bool &) = 0;                                                                                                                     

    virtual bool IsLegalToMoveNavy() const;                                                                                 
    virtual int GetNumUnitsQueuedToMerge(SubUnitType) const = 0;                                                                                        
    virtual int GetNumUnitsQueuedToMerge() const = 0;                                                                                       

    bool CanBeSplitAndMerged(bool bIgnoreMoveLockBlock, CString *pDesc) const;                                                           

    CFixedPointFPML GetNavyStrengthFactor(const CModifier &Modifier, bool bAttached, bool bIgnoreMorale, ESeaType eSeaType, CFixedPoint &vStats, const CSubUnit *pExclude, const SubUnitType eExcludeType, const CArray<CSubUnit *> *pOverrideShips) const;                                                                                                                               

    CFixedPointFPML GetArmyStrengthFactor(const CProvince *pCombatProvinceOrNullptr, bool bLowMoralePenalty, bool bAttached, int &nLeaderPips) const;                                                                   

    CFixedPointFPML GetAttachedArmyStrengthFactor(const CProvince *, bool, bool, int &) const;                                                                           

    CFixedPoint GetAnyModifierValue(ModifierType Type, bool bAllegiance, CString *pDesc) const;                                                                         

    bool WillClearExile(const CProvince *pProv, CCountryFlagCache<char> *pAccessCache) const;                                                                               

    CFixedPoint GetTravelTime(const CProvince *pTarget, const CProvince *pFrom) const;                                                          

    CFixedPoint CalcTargetCost(const CProvince *pTarget, const CProvince *pFrom, bool LogRandom) const;                                                            

    CEU3Date GetEstimatedArrival(const CProvince *pTarget, bool bIgnoreAllowedArrivalDate) const;                                                              

    void AddMovement(const CList<int> &path, bool clear, EStopMissionReason Reason);                                                                        

    bool CanFindPathTo(const CProvince *pTarget, CCountryFlagCache<char> *pAccessCache, bool bRequireSafe) const;                                                                               

    CFixedPointFPML CalcArmyStrengthFactor(const CProvince *pCombatProvinceOrNullptr, bool bLowMoralePenalty, bool bAttached, int &nLeaderPips) const;                                                                    

    CFixedPoint GetMaximumMorale(CUnit::EMoraleType Type, CString *pDesc) const;                                                                        

    bool PerformAction(CProvince *pTarget, bool bStatus, bool bAllowAutoTransport);                                                       

    int GetCount(SubUnitType, ESpecialUnitCategory) const;                                                                        

    static int GetCount(const CSubUnitDefinition *, const CArray<CSubUnit *> &);                                                                               

    static int GetCount(SubUnitType, const CArray<CSubUnit *> &);                                                                      

    CFixedPoint CalcArmyStrengthFactor(ESpecialUnitCategory) const;                                                                          

    CFixedPoint GetMaximumStrength() const;                                                                          
    void MonthlyDecayDrill();                                                                                      
    void ProcessMissionMonthly();                                                                                      
    void SetAgent(CAIAgent *pAgent);                                                                              
    int GetNumSubUnits() const;                                                                                  
    int GetNumAttachedSubunits(bool bSameLocation) const;                                                                
    CProvince *GetRealLocation() const;                                                                           
    CProvince *GetFinalLocation() const;                                                                           
    CProvince *GetPrevious() const;                                                                           
    CProvince *GetPreviousNoZoC() const;                                                                           
    const CList<int> &GetPath() const;                                                                   
    CProvince *GetNextTarget() const;                                                                           
    CProvince *GetFinalDestination() const;                                                                           
    void SetCountry(CCountryTag tag);                                                                                   
    const CLeader *GetLeader() const;                                                                      
    bool HasLeader() const;                                                                                
    bool InCombat() const;                                                                                
    bool InSiege() const;                                                                                
    bool CanMove() const;                                                                                
    bool IsRetreating() const;                                                                                 
    bool IsRetreatingShattered() const;                                                                                 
    CProvince *GetRetreatProvince(bool use_random) const;                                                            
    bool HasExileStatus() const;                                                                                 
    void AdjustMorale();                                                                                      
    CFixedPoint GetAverageMorale(bool bRetActualValue) const;                                                      
    CFixedPoint GetAverageStrength() const;                                                                          
    CCountryTag GetTechCountry() const;                                                                          
    CFixedPoint GetTotalStrength() const;                                                                          
    CFixedPoint GetAttachedTotalStrength(bool bSameLocation) const;                                                        
    CFixedPoint GetTotalAIStrength() const;                                                                          
    CFixedPoint ApplyAttrition();                                                                               
    int GetWeight() const;                                                                                 
    int GetTPC() const;                                                                                 
    bool IsMoving() const;                                                                                
    bool IsLegalToMove(const CProvince *pWhere) const;                                                                     
    void OnCombatStart(bool attacker);                                                                         
    int GetNumTransports() const;                                                                                  
    bool IsExploring() const;                                                                                 
    int GetExplorationShipsCount() const;                                                                                  
    bool IsForceMarch() const;                                                                                 
    const CAutomaticFleetTransport *GetAutoTransportMission() const;                                                      
    CRebelFaction *GetRebelFaction() const;                                                                       
    bool CanAttachTo() const;                                                                                 
    bool ShouldGoHomeAtWar() const;                                                                                 
    bool IsUnsafeMove(int nProv) const;                                                                        
    void UpdateUnsafeMoves();                                                                                      
    bool IsOnMission() const;                                                                                 
    CFixedPoint GetMaximumMorale(CString *) const;                                                                         
    const CArray<CSubUnit *> &GetSubUnits() const;                                                            
    CArray<CSubUnit *> &GetSubUnits();                                                                       

    inline const CCombat *GetCombat() const;                                                      
    inline CCombat *AccessCombat() const;                                                            
    inline CSiegeCombat *AccessSiege() const;                                                       
    inline void SetGoHomeAtWar(bool bGoHomeAtWar);                                                     
    inline CUnit *GetAttachedTo() const;                                                                
    inline const CUnitMission *GetMission() const;                                                    
    inline CUnitMission *AccessMission() const;                                                       
    inline CCountryTag GetCountry() const;                                                         
    inline CAIAgent *GetAgent() const;                                                             
    inline CFixedPoint GetCachedAttritionLevel() const;                                                           
    inline bool GetWasPrevious() const;                                                                
    inline bool IsInvading() const;                                                                
    inline bool IsMainArmy() const;                                                                  
    inline bool IsHunterKiller() const;                                                                  
    inline CProvince *GetLocation() const;                                                            
    inline int GetCount(const CSubUnitDefinition *pDef) const;                                   
    inline int GetCount(SubUnitType type) const;                                                   
    inline const CMercenaryCompany *GetMercenaryCompany() const;                                            
    inline const CUnitList &GetAttachedUnits() const;                                                      
    inline CUnitList &GetAttachedUnits();                                                                

protected:
    uint8_t m_pad0x3d[0x40 - 0x3d];                              
    CMercenaryCompany *_pMercenaryCompany;                                           
    CUnit *_pAttachedTo;                                                       
    CUnitList _AttachedUnits;                                                     
    uint8_t m_pad0x68[0xb8 - 0x68];                              
    CCombat *_pCombat;                                                    
    CSiegeCombat *_pSiege;                                               
    CCountryTag _Country;                                                  
    CLeader *_pLeader;                                                         
    uint8_t m_pad0xd8[0xe0 - 0xd8];                              
    CProvince *_pLocation;                                                  
    uint8_t m_pad0xe8[0xf8 - 0xe8];                              
    CList<int> _CurrentPath;                                                                    
    uint8_t m_pad0x110[0x130 - 0x110];                            
    CFixedPoint _vAttritionLevel;                                                   
    uint8_t m_pad0x134[0x160 - 0x134];                            
    CAIAgent *_pAgent;                                                    
    uint8_t m_pad0x168[0x1a8 - 0x168];                            
    CArray<TUnsafeMove<int>> _UnsafeProvs;                                                   
    CArray<TUnsafeMove<const CMapArea *>> _UnsafeAreas;                                  
    CArray<TUnsafeMove<const CRegion *>> _UnsafeRegions;                                   
    CUnitMission *_pMission;                                                
    uint8_t m_pad0x1f8[0x1fb - 0x1f8];                            
    bool _isRetreat;                                                                   
    bool _isRetreatShattered;                                                                    
    uint8_t m_pad0x1fd[0x1fe - 0x1fd];                            
    bool _bWasPrevious;                                                                             
    bool _bExiled;                                                                 
    bool _bHunterKiller;                                                                    
    bool _bMainArmy;                                                                      
    uint8_t m_pad0x202[0x204 - 0x202];                            
    bool _bIsInvading;                                                               
    uint8_t m_pad0x205[0x207 - 0x205];                            
    bool _bGoHomeAtWar;                                                                                
    uint8_t m_pad0x208[0x210 - 0x208];                            
};

#include "unit.tpp"
