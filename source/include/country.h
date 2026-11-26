#pragma once

#include "ai_strategy.h"
#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/list.h"
#include "clausewitz/pdx_core/pdxarray.h"
#include "clausewitz/pdx_core/persistent.h"
#include "country_cached_data.h"
#include "countrydiplomacy.h"
#include "countrytag.h"
#include "enum_indexed_array.h"
#include "enums.h"
#include "eu3date.h"
#include "land_maintenance.h"
#include "ordered_province_set.h"
#include "technology.h"
#include "unitlist.h"
#include <boost/dynamic_bitset.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

class CCountry;
class CCulture;
class CEventScope;
class CDelayedEvent;
class CProvince;
class CUnit;
class CWar;
class CEvent;
class CLeader;
class CMonarch;
class CHeir;
class CQueen;
class CEnvoy;
class CColonyConstruction;
class CAutomatedDiplomatManager;
class COwnerArea;
class CControllerArea;
class CNativeCountryData;
class CCountryDiplomacy;
class CSubUnit;
class CSubUnitDefinition;
class CFlagship;
class CRebelFaction;
class CMercenaryCompany;
class CEU3AI;
class CPlayerAiPrefs;
class CGovernmentInstance;
class CGovernmentReformStack;
class CSubjectType;
class CString;
class CRival;
struct SLandMaintenance;
template <class T>
class CCountryFlagCache;
enum class SubUnitType : int;
enum class ESpecialUnitCategory : int;
enum EnvoyType : int;

#define HELPER inline

class CTechnologyStatus : public CTechnology
{
public:
    ~CTechnologyStatus() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                  
    void ReadMember(CReader &Reader, int Type) override;                                                                         

    int GetMilitaryLevel();                                                           

private:
    uint8_t m_pad0x148[0x168 - 0x148];          
};

class CCountryVCountryMemory : public CPersistent
{
public:
    ~CCountryVCountryMemory() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                       
    void ReadMember(CReader &Reader, int Type) override;                                                                              

    void AddOccasion(CCountryTag Against);                                                                                          
    void OnTagChange(CCountryTag OldTag, CCountryTag NewTag);                                                                          
    int GetOccasions(CCountryTag tag) const;                                                                                          
    int GetTotalOccasions() const;                                                                                             

private:
    uint8_t m_pad0xc[0x10 - 0xc];                                
    std::unordered_map<CCountryTag, int> _OccasionsCount;                                                   
};

class CCountry : public CPersistent
{
public:
    enum ELandMaintenanceMode : int
    {
        CountEverything = 0,
        IgnoreReinforcementsForPerformance = 1,
    };

    enum ELandMaintenanceCacheMode : int
    {
        CalcFromScratch = 0,
        UseCachedCore = 1,
    };

    ~CCountry() override;                                                                           
    void WriteMembers(CWriter &Writer) override;                                                                        
    void ReadMember(CReader &Reader, int Type) override;                                                               
    void InitPostRead() override;                                                                               

    void VisitFlagship(const std::function<void(const std::unique_ptr<CFlagship> &, const CSubUnit *)> &f) const;                                                                                                                              

    void AddDelayedEvent(CEvent *pEvent, const CEventScope &Scope, int nDays);                                                                       

    CFixedPoint GetRelativeWarThreat(bool bIsNaval, bool bIgnoreHumanBonus) const;                                                       

    bool IsInSameWarAgainst(CCountryTag Friend, CCountryTag Enemy) const;                                                                   

    bool IsInSupplyRange(const CProvince *Province, CProvince **ppBest) const;                                                                 

    void UpdateEstates(bool bMonthly, bool IsTagSwitching);                                                      
    void ReplaceEnemy(CCountryTag oldTag, CCountryTag newTag);                                                                
    int GetRebelThreat(bool bIncludeSubjects) const;                                                              
    bool IsWarFriend(CCountryTag who, CCountryTag enemy) const;                                                               
    bool IsPreparingForWar() const;                                                                                  
    const CGovernmentInstance *GetGovernmentInstance() const;                                                            
    int GetNumOfRebelControlledProvinces() const;                                                                                   
    int GetNumOfRebelArmies() const;                                                                                   
    ESubjectMilitaryFocus GetSubjectMilitaryFocus() const;                                                                 
    void SetSubjectMilitaryFocus(ESubjectMilitaryFocus l_var0x01);                                                                              
    void GetCurrentWars(CArray<CWar *> &out) const;                                                                              
    CFixedPoint GetInlandSeaRatio() const;                                                                           
    int GetCurrentNumberOfBankLoans() const;                                                                                   
    int GetMaximumBankLoans() const;                                                                                   
    bool HasTradeInterest() const;                                                                                  
    CFixedPoint GetRelativeRebelThreat() const;                                                                           
    bool IsAtWarWithCountry(CCountryTag CountryTag) const;                                                                        
    bool HasDefensivePact(CCountryTag Other) const;                                                                             
    bool IsSmallNation(CCountryTag annexer) const;                                                                           
    CFixedPoint CalcDesperation(bool bIncludeSieges) const;                                                        
    int CalcNominalArmySizeNotForAI() const;                                                                                   
    CFixedPoint GetNavalForceLimit(CString *pDesc) const;                                                                     
    CFixedPoint GetNavalMaintenance(CFixedPoint vSlider) const;                                                                    
    const CSubjectType *GetSubjectType() const;                                                                   
    bool IsPrimitive() const;                                                                                  
    void AddPreviousCountryTag(CCountryTag tag);                                                                                    
    inline CEnvoy *AccessEnvoys(EnvoyType l_var0x0);                                                      
    HELPER CNativeCountryData *AccessNativeDataOrNullptr();
    void SetCountryTag(CCountryTag tag, bool bCancelConstructions);                                                              
    CFixedPoint GetCurrentIncome(bool bForTariffs) const;                                                               
    CFixedPoint GetCurrentExpenses() const;                                                                               
    int GetNumOfCorePorts() const;                                                                                       
    const CEU3AI *GetAIAtHost() const;                                                                             
    COwnerArea *GetHomeOwnerArea() const;                                                                               
    int GetNumOfTotalPorts() const;                                                                                       
    CFixedPoint GetTotalShips() const;                                                                               
    bool CanDetachDamaged(const CUnit *pUnit) const;                                                                           
    const CGovernmentReformStack &GetReformStack() const;                                                             
    int GetNumOfSubUnits(SubUnitType subUnitType) const;                                                                            
    bool AllowedToBuildShips() const;                                                                                      
    CCountryTag GetTopOverlord(bool bIncludeIndependentSubject) const;                                                
    void SetManpower(CFixedPoint vValue);                                                                                     
    void SetSailors(CFixedPoint vValue);                                                                                     
    const CArray<CCountryTag> &GetSubjects() const;                                                                
    CFixedPoint GetCombatWidth() const;                                                                               
    CFixedPoint GetTotalMen() const;                                                                               
    CControllerArea *GetHomeControllerArea() const;                                                                          
    CFixedPoint GetNavalEngagementWidth(CString *pTooltip) const;                                                                      
    CFixedPoint GetLandForceLimit(CString *pDesc) const;                                                                         
    CFixedPoint GetCountryMilitaryTactics(CString *pDesc) const;                                                                         
    const CArray<CCountryTag> &GetEnemies() const;                                                                

    bool IsInImportantWar(bool bOnlyCritical, bool bIgnoreStrengthCheck, const CWar *pParticularWar) const;                                                          

    bool IsFriend(CCountryTag who, bool bIncludeMilAcc, bool bIsNavy, CString *pDesc) const;                                                                

    bool HaveAccess(const CProvince *pTarget, bool bIsNavy, bool bDebug, CString *pDesc, CCountryFlagCache<char> *pAccessCache) const;                                                                                         

    bool DoesSubjectTypeProhibitWarWith(CCountryTag Other, CString *pDesc) const;                                                                                     

    CFixedPoint GetLibertyDesire(CString *pDesc, int nIndent, bool bPositiveIsGood) const;                                                            

    const CSubUnitDefinition *GetSubunitTypePreferred(SubUnitType type) const;                                                                     

    CFixedPoint GetBuildCost(ESpecialUnitCategory eCategory, SubUnitType type, const CProvince *pProvince, unsigned char Flags, CString *pDesc) const;                                                                                                      

    CFixedPoint CalcActualLandMaintenanceCost(const CSubUnitDefinition *const pSubUnitDefinition, ESpecialUnitCategory eCategory, CFixedPoint vUnitStrength, const CMercenaryCompany *pCompany, CString *pDesc) const;                                                                                                                                                     

    CFixedPoint CalcBaseLandMaintenanceCost(SubUnitType eSubUnitType, ESpecialUnitCategory eCategory, bool isMerc, CString *pBuildCostDesc) const;                                                                                                         

    CFixedPoint CalcBaseNavalMaintenanceCost(SubUnitType ESubUnitType, ESpecialUnitCategory eCategory, CString *pBuildCostDesc) const;                                                                                                         

    CFixedPoint CalcActualNavalMaintenanceCost(SubUnitType eSubUnitType, ESpecialUnitCategory eCategory, CFixedPoint vShipCount, CString *pBuildCostDesc) const;                                                                                                                        

    SLandMaintenance GetLandMaintenance(CFixedPoint size, CCountry::ELandMaintenanceMode mode, CCountry::ELandMaintenanceCacheMode cacheMode) const;                                                                                                                         

    CString GenerateSubUnitName(const CSubUnitDefinition *pDef, const CProvince *pHome, ESpecialUnitCategory category, CRebelFaction *pFaction, int *pStartAndEndIndex, bool bIsSynced);                                                                                                                              

    int GetNumOfSubUnits(ESpecialUnitCategory category) const;                                                                       

    bool IsLibertyDesireChainBelowLimit(CFixedPoint LibertyLimit, CCountryTag TopOverlordToCheck) const;                                                                                         

    bool CanMergeUnits(CList<CUnit *> &list, CString *pDesc, bool bIgnoreMoveLockBlock) const;                                                                        

    inline const CList<CLeader *> &GetActiveLeaders() const;                                              
    inline CMonarch *GetCurrentMonarch() const;                                                          
    inline CQueen *GetCurrentQueen() const;                                                               
    inline const CList<int> &GetCoreProvinces() const;                                                    
    inline const CList<int> &GetClaimProvinces() const;                                                    
    inline CList<CRival> &AccessRivals();                                                           
    inline CCountryTag GetCoalitionTarget() const;                                                          

    CHeir *GetCurrentHeir() const;                                                                                            
    const CList<CColonyConstruction *> &GetCurrentColonyConstructions() const;                                                               

    inline CAIStrategy &AccessStrategy();                                                                                
    inline const CLeader *GetBestReassignableArmyLeader() const;                                                                        
    inline const CAIStrategy &GetStrategy() const;                                                                      
    inline const CUnitList &GetUnits() const;                                                                        
    inline const COrderedProvinceSet &GetOwnedProvinces() const;                                                              
    inline const CCountryDiplomacy &GetDiplomacy() const;                                                              
    inline CCountryTag GetOverlord() const;                                                                             
    inline const CCountryCachedData &GetCachedData() const;                                                               
    inline CProvince *GetCapitalLocation() const;                                                                            
    inline CFixedPoint GetManpower() const;                                                                           
    inline CFixedPoint GetMaxManpower() const;                                                                           
    inline CTechnologyStatus *GetTechnologyStatus() const;                                                                    
    inline const CPlayerAiPrefs &GetPlayerAiPrefs() const;                                                                   
    inline CFixedPoint GetWarExhaustion() const;                                                                             
    inline CFixedPoint GetCachedDevelopmentRaw() const;                                                                           
    inline const CArray<CRebelFaction *> &GetRebelFactions() const;                                                          
    inline bool HasUnconditionalSurrender() const;                                                                                    
    inline bool HasCriticalHomeAreaStrait() const;                                                                                  
    inline bool IsAtWar() const;                                                                                    
    inline bool IsSubject() const;                                                                                  
    inline const CList<int> &GetControlledProvinces() const;                                                                       
    inline CFixedPoint GetSailors() const;                                                                           
    inline CCountryTag GetCountryTag() const;                                                                           
    inline COrderedProvinceSet &GetOwnedProvinces();                                                                          
    inline CFixedPoint GetTreasury() const;                                                                             
    inline const CList<CCountryTag> &GetCurrentAtWarWith() const;                                                             
    inline const CList<CCountryTag> &GetCurrentWarAllies() const;                                                             
    inline CList<std::unique_ptr<CControllerArea>> &AccessControllerAreaList();                                                    
    inline const CCulture *GetPrimaryCulture() const;                                                                       
    inline const CList<std::unique_ptr<COwnerArea>> &GetOwnerAreaList() const;                                             

    int CalcCurrentAmountOfRegiments() const;                                                                                               
    CFixedPoint CalcManpowerReinforcementsNeeded(bool bIncludeSubjects) const;                                                                  
    bool IsAcceptedCultureByAllRules(const CCulture *pCulture) const;                                                                                

private:
    uint8_t m_pad0xc[0x1c - 0xc];                                    
    CCountryTag _Tag;                                                                        
    uint8_t m_pad0x24[0x78 - 0x24];                                   
    CArray<CCountryTag> _PreviousCountryTags;                                                          
    uint8_t m_pad0x90[0x108 - 0x90];                                  
    CList<CColonyConstruction *> _CurrentColonyConstructions;                                      
    uint8_t m_pad0x120[0x3c0 - 0x120];                                 
    CAIStrategy _AIStrategy;                                                                  
    uint8_t m_pad0x740[0xb90 - 0x740];                                 
    CFixedPoint _vTreasury;                                                           
    uint8_t m_pad0xb94[0xcf0 - 0xb94];                                 
    CFixedPoint _vWarExhaustion;                                                            
    uint8_t m_pad0xcf4[0xcf8 - 0xcf4];                                 
    CUnitList _Units;                                                                       
    uint8_t m_pad0xd10[0xd28 - 0xd10];                                 

    CEnumIndexedArray<const CSubUnitDefinition *, SubUnitType, 7> _SubUnitSelection;                                           

    uint8_t m_pad0xd60[0xd98 - 0xd60];                                        
    CFixedPoint _vManpower;                                                                       
    CFixedPoint _vMaxManpower;                                                                     
    CFixedPoint _vSailors;                                                                      
    uint8_t m_pad0xda4[0xdfc - 0xda4];                                        
    CFixedPoint _vCachedDevelopmentRaw;                                                               
    uint8_t m_pad0xe00[0xe48 - 0xe00];                                        
    CArray<CRebelFaction *> _RebelFactions;                                                        
    uint8_t m_pad0xe60[0x11f8 - 0xe60];                                       
    const CCulture *_pPrimaryCulture;                                                                          
    uint8_t m_pad0x1200[0x1270 - 0x1200];                                      
    CMonarch *_pCurrentMonarch;                                                                       
    uint8_t m_pad0x1278[0x1288 - 0x1278];                                      
    CHeir *_pCurrentHeir;                                                                               
    uint8_t m_pad0x1290[0x12a0 - 0x1290];                                      
    CQueen *_pCurrentQueen;                                                                        
    uint8_t m_pad0x12a8[0x1320 - 0x12a8];                                      
    COrderedProvinceSet _OwnedProvinces;                                                             
    CList<int> _ControlledProvinces;                                                                      
    uint8_t m_pad0x1350[0x1370 - 0x1350];                                      
    CList<int> _CoreProvinces;                                                                      
    CList<int> _ClaimProvinces;                                                                        
    uint8_t m_pad0x13a0[0x13c8 - 0x13a0];                                      
    CList<std::unique_ptr<CControllerArea>> _ControllerAreaList;                                      
    CList<std::unique_ptr<COwnerArea>> _OwnerAreaList;                                           
    uint8_t m_pad0x13f8[0x1458 - 0x13f8];                                      
    CList<CLeader *> _Leaders;                                                                        
    CArray<CEnvoy *> _Envoys;                                                               
    std::unique_ptr<CAutomatedDiplomatManager> _pAutomaticDiplomats;                                     
    uint8_t m_pad0x1490[0x1498 - 0x1490];                                      
    const CLeader *_pBestReassignableArmyLeader;                                                                
    CProvince *_pCapitalLocation;                                                                     
    uint8_t m_pad0x14a8[0x14e8 - 0x14a8];                                      
    CCountryDiplomacy _Diplomacy;                                                                                       
    uint8_t m_pad0x1868[0x18d8 - 0x1868];                                      
    CCountryTag _OverLord;                                                                    
    CCountryTag _CoalitionTarget;                                                                     
    uint8_t m_pad0x18e8[0x1af8 - 0x18e8];                                      
    boost::dynamic_bitset<unsigned long> _IsFriend;                                                   
    uint8_t m_pad0x1b18[0x1b70 - 0x1b18];                                      
    CList<CCountryTag> _CurrentAtWarWith;                                                            
    uint8_t m_pad0x1b88[0x1ba8 - 0x1b88];                                      
    CList<CCountryTag> _CurrentWarAllies;                                                               
    uint8_t m_pad0x1bc0[0x2208 - 0x1bc0];                                      
    CArray<CDelayedEvent *> _DelayedEvents;                                                        
    CArray<CCountryTag> _Enemies;                                                                    
    uint8_t m_pad0x2238[0x2250 - 0x2238];                                      
    CList<CRival> _Rivals;                                                                  
    uint8_t m_pad0x2268[0x2448 - 0x2268];                                      
    CTechnologyStatus *_pTechnology;                                                                    
    uint8_t m_pad0x2450[0x2460 - 0x2450];                                      
    std::unique_ptr<CPlayerAiPrefs> _PlayerAiPrefs;                                                        
    uint8_t m_pad0x2468[0x283c - 0x2468];                                      
    ESubjectMilitaryFocus _eSubjectMilitaryFocus;                                                           
    uint8_t m_pad0x2840[0x285b - 0x2840];                                      
    bool _bHasUnconditionalSurrender;                                                                            
    uint8_t m_pad0x285c[0x2865 - 0x285c];                                      
    bool _bHasHomeAreaStrait;                                                                                       
    uint8_t m_pad0x2866[0x2873 - 0x2866];                                      
    bool _AtWar;                                                                                               
    uint8_t m_pad0x2874[0x2898 - 0x2874];                                      
    CCountryCachedData _CachedData;                                                                        
    uint8_t m_pad0x95a0[0x96c8 - 0x95a0];                                      
    std::unique_ptr<CNativeCountryData> _pNativeData;                                                       
    uint8_t m_pad0x96d0[0x97b8 - 0x96d0];                                      
};

#include "country.tpp"
