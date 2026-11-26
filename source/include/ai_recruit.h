#pragma once

#include "ai_plan.h"
#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/fixedpoint.h"
#include "clausewitz/pdx_core/list.h"
#include "clausewitz/pdx_core/pdxarray.h"
#include "clausewitz/pdx_core/pdxhybridarray.h"
#include "countrytag.h"
#include "engine.h"
#include "enum_indexed_array.h"
#include "eu3date.h"
#include "unitlist.h"
#include <cstdint>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

class CCountry;
class CProvince;
class CMercenaryCompany;
class CSpendingList;
class CSubUnit;
class CSubUnitDefinition;
template <typename T>
class CCountryFlagCache;
class CAIAdmiral;
class CAIAgent;
class CAIArmy;
class CAIBuilder;
class CAIExpansionMinister;
class CAIExplorePlan;
class CAIExploreProvincePlan;
class CAIExploreSeaZonePlan;
class CAIFinancialMinister;
class CAIForeignMinister;
class CAIKing;
class CAIMilitaryPlan;
class CAIMinister;
class CAINavalBlockadePlan;
class CAINavalPatrolPlan;
class CAIPeon;
class CAIPlan;
class CAIReligionMinister;
class CAISendColonistPlan;
class CAISubscriber;
class CAIStrategy;
class CAITransportPlan;
class CEU3AI;
enum CAIPlanType : int;

class CAIRecruitMinister : public CAIPlan
{
public:
    struct SCategoryScoreMultiplier : public CEnumIndexedArray<CFixedPoint, ESpecialUnitCategory, 23>
    {
    };

    struct STypeInfo
    {
        const CSubUnitDefinition *pDefinition;                                    
        int nCurrent;                                                          
        int nWanted;                                                           
        CFixedPoint vCost;                                                     
    };

    ~CAIRecruitMinister() override;                                                       

    void NotifyTick(CCountryFlagCache<char> &AccessCache) override;                                                                                       

    void Start() override;                                                                         
    void Finish() override;                                                                         
    CAIPlanType GetPlanType() override;                                                                   

    CAIRecruitMinister(CAIKing &);                                                    
    void RecruitLeaders();                                                                  

    ESpecialUnitCategory MaintainTroopLevels(CSpendingList &Proxy, bool &bNeedsMoney);                                                                                

    ESpecialUnitCategory MaintainFleet(CSpendingList &Proxy, bool &bNeedsMoney);                                                                          

    bool WouldDisbandLand(int nAdditionalRegulars, int nAdditionalMercs, bool bOnlyCheckedType) const;                                                               

    static CArray<ESpecialUnitCategory> GetPreferredLandUnitCategories(const CCountry &Country, const CAIRecruitMinister::SCategoryScoreMultiplier &ScoreMultipliers);                                                                                                                    

    static CArray<ESpecialUnitCategory> GetPreferredSeaUnitCategories(const CCountry &Country, const CAIRecruitMinister::SCategoryScoreMultiplier &ScoreMultipliers);                                                                                                                   

    static void GetDesiredLandRegimentTypeFractions(const CCountry &Country, CFixedPoint nForceLimit, CFixedPoint vTotalArmyBudget, CFixedPoint &vInfantry, CFixedPoint &vCavalry, CFixedPoint &vArtillery);                                                                                                                   

private:
    bool DisbandLandUnit();                                                                
    bool DisbandMercenaryUnit();                                                                
    bool UpgradeOutdatedShips();                                                                

    CSubUnit *FindShipToDisband(SubUnitType eType, bool bOutdatedOnly) const;                                                                           

    void CalculatePotentialMercenaryCompanies(const CProvince *pProvince, CPdxArray<const CMercenaryCompany *, int> &Companies) const;                                                                                                                               

    CEnumIndexedArray<CAIRecruitMinister::STypeInfo, SubUnitType, 7> GetShipTypeInfo() const;                                                            

    void IncludeShipsBeingBuilt(CEnumIndexedArray<CAIRecruitMinister::STypeInfo, SubUnitType, 7> &, const CPdxArray<int, int> &);                                                                                                                                      

    bool HasOutdatedShips(const CEnumIndexedArray<CAIRecruitMinister::STypeInfo, SubUnitType, 7> &) const;                                                                                                                  

    static CFixedPoint GetDesiredCavToInfRatio(const CCountry &Country);                                                                             

    int _nCount;                   
};

struct SLandDisbandEval
{
    bool bOnlyCheckedType;                                                                    
    int nCurrentNumOfRegiments;                                           
    int nAdditionalRegulars;                                               
    int nAdditionalMercs;                                                    
    CFixedPoint vRatio;                                       
    const CCountry &Country;                                                  

    bool WillReallySoonFightRebels(int nMonths, bool bAbortOnNoncontrolled) const;                                                                                   
};

struct SDesiredTypeResult
{
    SubUnitType _Type;                                                        
    bool _bNeedsMoneyBeforeBuildingNext;                                                 
};
