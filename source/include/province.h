#pragma once

#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/pdxhybridarray.h"
#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include "eu3date.h"
#include "province_id.h"
#include "province_modifiers.h"
#include "provincehistory.h"
#include "selectable.h"
#include "unitlist.h"

class CUnit;
class CProvinceTemplate;
class CHistoryEntry;
struct SAdjacency;
class CCombat;
class CConstruction;
class CPort;
class CContinent;
class CRebelFaction;
class CSiegeCombat;
class CRegion;
class CCountryState;
class CColonialRegion;
class CTradeCompanyRegion;
class CTradeNode;
class CRebelType;
class CEU3Idler;
class CTimedModifier;
class CProvinceTriggeredModifier;
class COwnerArea;
class COwnerAreaNoStrait;
class CCulture;
class CReligion;
class CTradeGoods;
class CSuperRegion;
class CProvinceGroup;
class CGreatProjectType;
class CDiplomacySpyAction;
class CCenterOfReformation;
class CCardinal;
class CWriter;
class CReader;
class CSubUnitDefinition;

namespace pdx
{
template <unsigned N, typename T>
class hybrid_bitset
{
public:
    inline bool test(unsigned nIndex) const;                                                               

private:
    CPdxHybridArray<T, N / (sizeof(T) * 8), int> _Storage;                                
    unsigned _Size;                                                                       
};
}                 

class CCachedProvinceData
{
public:
    inline const CProvinceModifiers &GetModifiers() const;                                                   

private:
    uint8_t m_pad0x0[0x8];                       
    CProvinceModifiers _Modifiers;                                            
    uint8_t m_pad0x23a0[0x23c0 - 0x23a0];           
};

class CProvince : public CPersistent, public CSelectable
{
public:
    struct SSetOwnerParameters
    {
        CCountryTag _NewOwner;                                                                     
        CEU3Date _Date;                                                                       
        bool _bAddHistory;                                                                       
        bool _bAddingToSelf;                                                                                 
        bool _bDestroyBuildings;                                                                          
        bool _bSetCountrytag;                                                         
        bool _bClearCountryIfLastProvince;                                                                
        bool _bMigrating;                                                             
        bool _bByForce;                                                             
        uint8_t m_pad0x13[0x14 - 0x13];            

        inline SSetOwnerParameters(CCountryTag newOwner, const CEU3Date &date, bool bAddHistory, bool bAddingToSelf, bool bDestroyBuildings);                                                                                 
    };

    struct SAlliedObjective
    {
        CCountryTag _From;                                              
        CCountryTag _To;                                                      
    };

    ~CProvince() override;                                                                                    
    void WriteMembers(CWriter &Writer) override;                                                                                  
    void ReadMember(CReader &Reader, int Type) override;                                                                         
    void InitPostRead() override;                                                                                         
    SelectableType GetType() const override;                                                                         
    bool IsSelectable() const override;                                                                                    
    void OnSelect(CSelectionList *pList) override;                                                                                   
    void OnDeSelect() override;                                                                                         
    bool PerformAction(CProvince *pTarget, bool status) override;                                                             
    const CString &GetName() const override;                                                                         
    CToolTip GetTooltip(const CProvince *pTarget) const override;                                                           

    int GetProvinceID() const;                                                                                       
    const CList<const SAdjacency *> &GetStraits() const;                                                          
    const CProvinceTemplate &GetTemplate() const;                                                                  
    CCountryTag GetOwner() const;                                                                              
    CCountryTag GetController() const;                                                                               
    const CRegion *GetRegion() const;                                                                           
    int GetFortLevel() const;                                                                                       
    void GetAdjacentRegions(CArray<int> &out) const;                                                                                
    int GetFortLevelAfterMothball() const;                                                                                       
    int GetRegimentsNeededForSiege() const;                                                                                       
    CFixedPoint GetDistance(int nTarget, bool bCheckAdjacency) const;                                               
    CFixedPoint GetNavalDistance(int nTarget) const;                                                                    
    CCountryTag GetAreaController() const;                                                                               
    CFixedPoint GetGarrison() const;                                                                               
    CFixedPoint GetNominalOwnerSupplyLimit() const;                                                                               
    bool HasFort() const;                                                                                     
    bool HasAdjacentEnemy(CCountryTag Country) const;                                                                               
    CRebelFaction *GetRevoltRebelFaction() const;                                                                            
    bool HasMilitaryConstruction() const;                                                                                      
    bool HasConstruction() const;                                                                                      
    bool HasBuildableUnits() const;                                                                                      
    CList<const CSubUnitDefinition *> GetBuildableShips() const;                                                         
    CList<const CSubUnitDefinition *> GetBuildableUnits() const;                                                         
    bool IsOverseas(CCountryTag Tag, bool bCheckOwnerArea) const;                                                              
    bool IsCapital() const;                                                                                     
    CSiegeCombat *GetSiege() const;                                                                            
    CFixedPoint GetSiegeStatus() const;                                                                               
    CEU3Date FindLastOwnerChange(CCountryTag tag) const;                                                                               

    void SetOwner(const SSetOwnerParameters &Params);                                                                     
    void SetCountryState(CCountryState *pCountryState);                                                                
    bool HasPermanentClaim(CCountryTag Tag) const;                                                                     

    CFixedPoint GetDistance(const CProvince *pOther, bool bCheckAdjacency) const;                                                  

    bool HasHostileFortProtectingAgainst(CCountryTag Tag) const;                                                                              

    CFixedPoint GetCurrentTaxIncome(CString *pDesc, bool bMonthly, CFixedPoint vLocalAddition, CFixedPoint vLocalModifierAddition) const;                                                                               

    CFixedPoint GetProductionIncome(CString *pDesc, bool bMonthly) const;                                                               

    CFixedPoint GetDevelopment(bool bCheckCustom, bool bIncludeTribalDevelopment, CString *pDesc) const;                                                           

    CFixedPoint GetSupplyLimit(CCountryTag who, CString *pDesc, bool bNominal) const;                                                                       

    CFixedPoint GetWarValue(bool bForOccupation, CString *pDesc, int nIndent) const;                                                        

    void Discover(CCountryTag Tag, const CEU3Date &Date, bool bAddHistory, bool bAddMessage, bool bAddPrestige, bool bRemoveFoW);                                                                    

    void SetController(CCountryTag tag, const CEU3Date &date, bool add_history, const char *pDebugCaller);                                                                          

    void AddAsCore(CCountryTag Tag, const CEU3Date &Date, bool bSilent);                                                                  

    bool AddAsClaim(CCountryTag tag, bool add_history, bool bPermanent, bool LogError);                                                           

    CEU3Date FindLastControllerChange(const CHistoryEntry *pHistory, CCountryTag tag) const;                                                                                        

    CFixedPoint GetUnrest() const;                                           

    inline const CProvinceHistory &GetHistory() const;                                       

    inline const CProvinceModifier &GetModifier() const;                                          

    inline CProvince(const CProvince &);                                              
    CProvince(CProvince &&);                                                        
    inline CProvince(int);                                                            
    CProvince(SProvinceId);                                                                   
    inline CProvince(int, CProvinceTemplate *);                                       
    CProvince(SProvinceId, CProvinceTemplate *);                                                                  
    HELPER bool IsTerraIncognitaActive() const;
    HELPER bool IsDiscoveredBy(CCountryTag tag) const;

    inline CControllerArea *GetControllerArea() const;                                                            
    inline COwnerArea *GetOwnerArea() const;                                                                 
    inline const CUnitList &GetUnits() const;                                                          
    inline CFixedPoint GetBlockadeProgress() const;                                                               
    inline CPort *GetPort() const;                                                                    
    inline CCombat *GetCombat() const;                                                                    
    inline const CCulture *GetCulture() const;                                                             
    inline CContinent *GetContinent() const;                                                                 
    inline CColonialRegion *GetColonialRegion() const;                                                            
    inline const CList<CConstruction *> &GetConstructions() const;                                                
    const CArray<CCountryTag> &GetCores() const;                                                            
    inline const CCountryState *GetCountryState() const;                                                         

private:
    SProvinceId _Id;                                                                           
    CProvinceTemplate &_Template;                                                            
    CProvinceHistory _History;                                                       
    uint8_t m_pad0xa8[0x190 - 0xa8];                        
    CList<const SAdjacency *> _Straits;                                                                       
    uint8_t m_pad0x1a8[0x2c0 - 0x1a8];                       
    CCountryState *_pCountryState;                                                                        
    uint8_t m_pad0x2c8[0x318 - 0x2c8];                       
    CControllerArea *_pControllerArea;                                                                         
    COwnerArea *_pOwnerArea;                                                                              
    uint8_t m_pad0x328[0x330 - 0x328];                       
    CUnitList _Units;                                                                             
    uint8_t m_pad0x348[0x360 - 0x348];                       
    CCombat *_pCombat;                                                                                         
    CSiegeCombat *_pSiege;                                                                                 
    uint8_t m_pad0x370[0x420 - 0x370];                       
    const CCulture *_pCulture;                                                        
    uint8_t m_pad0x428[0x448 - 0x428];                       
    CCountryTag _Owner;                                                                           
    CCountryTag _Controller;                                                                                  
    uint8_t m_pad0x458[0x470 - 0x458];                       
    CArray<CCountryTag> _Core;                                                                                 
    uint8_t m_pad0x488[0x4b8 - 0x488];                       
    CList<CConstruction *> _Constructions;                                                                 
    uint8_t m_pad0x4d0[0x530 - 0x4d0];                       
    CPort *_pPort;                                                                         
    uint8_t m_pad0x538[0x540 - 0x538];                       
    CContinent *_pContinent;                                                                      
    uint8_t m_pad0x548[0x56c - 0x548];                       
    CFixedPoint _vBlockadeProgress;                                                                    
    uint8_t m_pad0x570[0x5b8 - 0x570];                       
    pdx::hybrid_bitset<4096, unsigned> _Discovered;                                                        
    uint8_t m_pad0x7d8[0x880 - 0x7d8];                       
    CColonialRegion *_pColonialRegion;                                                                     
    uint8_t m_pad0x888[0x938 - 0x888];                       
    bool _bConverterGenerated : true;                                                                    
    bool _bFogOfWar : true;                                                                      
    bool _HasActiveTradeCompany : true;                                                      
    bool _bRNWName : true;                                                                                   
    bool _isCity : true;                                                               
    bool _bEmpireStatus : true;                                                                 
    bool _bIsBlockaded : true;                                                                
    bool _bUpdateBlockade : true;                                                                 
    bool _IsCenterOfReformation : true;                                                          
    bool _bCancelBuildingPending : true;                                                         
    bool _bHasInlandPort : true;                                                               
    bool _bHreLiberated : true;                                                                    
    bool _bTIActive : true;                                                                   
    bool _bIsFowActive : true;                                                                     
    bool _bUserChangedName : true;                                                             
    bool _bPresentInSaveGame : true;                                                                    
    bool _HasRevolution : true;                                                               
    uint8_t m_pad0x93b[0x960 - 0x93b];                       
    CCachedProvinceData _CachedData;                                                        
};

#include "province.tpp"
