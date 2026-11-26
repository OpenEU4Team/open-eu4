#pragma once

#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/fixedpoint.h"
#include "clausewitz/pdx_core/list.h"
#include "clausewitz/pdx_core/pdxarray.h"
#include "clausewitz/pdx_core/pdxhybridarray.h"
#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include "engine.h"
#include "enum_indexed_array.h"
#include "eu3date.h"
#include "pdx_def_val.h"
#include "unitlist.h"
#include <cstdint>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

class CReader;
class CWriter;
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
class CAIRecruitMinister;
class CAIReligionMinister;
class CAISendColonistPlan;
class CAISubscriber;
class CAITransportPlan;
class CEU3AI;
enum AIBudgetType : int;

class CAIStrategy : public CPersistent
{
public:
    struct SArmyOrNavySizeObjective
    {
        int _nSize;                                               
        CFixedPoint _vImportance;                                               
    };

    ~CAIStrategy() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                            
    void ReadMember(CReader &Reader, int Type) override;                                                                   

    void SetTag(CCountryTag);                                                                                 
    bool IsNavyUseful() const;                                                                            
    int GetWantedSubUnits(SubUnitType Type) const;                                                                         
    inline CCountryTag GetCountryTag() const;                                                       
    bool IsPreparingWar() const;                                                                            
    CCountryTag GetWarTarget() const;                                                                     
    int GetConquerValue(int nProv) const;                                                                    
    int CalcDesiredAmountOfRegiments() const;                                                                             
    CFixedPoint GetArmyMoney() const;                                                                     
    CFixedPoint GetNavyMoney() const;                                                                     
    void SetNeedsShips(ESpecialUnitCategory NeedsShips);                                                                       
    bool CalcIsDefendedHomeStraitUnderAttack();                                                                                 
    bool IsRich(CFixedPoint vExtraMargin) const;                                                               

    void AddArmySizeObjective(CAIStrategy::SArmyOrNavySizeObjective);                                                                                      

    void AddNavySizeObjective(CAIStrategy::SArmyOrNavySizeObjective);                                                                                      

    void CalcDesiredAmountOfShipsFromCurrent(int &nHeavies, int &nGalleys, int &nLight, int &nTransports) const;                                                                                   

    void SetNeedsRegiments(ESpecialUnitCategory NeedsRegiments);                                                                           

    void GetPotentialNavalCompetition(CArray<CCountryTag> &, bool) const;                                                                                         

    inline bool IsBudgetInitialized() const;                                                                
    inline CFixedPoint GetBudget(AIBudgetType type) const;                                        
    inline ESpecialUnitCategory GetNeedsShips() const;                                                
    inline ESpecialUnitCategory GetNeedsRegiments() const;                                                
    inline int GetDefendedHomeStrait() const;                                                                 
    inline bool IsFixed() const;                                                                
    inline bool IsInitialized() const;                                                                   

    inline const CPdxArray<CAIStrategy::SArmyOrNavySizeObjective, int> &GetArmySizeObjectives() const;                                                    

private:
    uint8_t m_pad0xc[0x18 - 0xc];                                                  
    CCountryTag _CountryTag;                                                                       
    uint8_t m_pad0x20[0x24 - 0x20];                                                 
    bool _bInitialized;                                                                            
    uint8_t m_pad0x25[0x26 - 0x25];                                                 
    bool _bInitializedBudget;                                                                             
    uint8_t m_pad0x27[0x2c - 0x27];                                                 
    CFixedPoint _vArmyMoney;                                                                      
    CFixedPoint _vNavyMoney;                                                                      
    uint8_t m_pad0x34[0x50 - 0x34];                                                 
    CEnumIndexedArray<CFixedPoint, AIBudgetType, 10> _Budget;                                                                         
    uint8_t m_pad0x78[0x79 - 0x78];                                                 
    bool _bFixed;                                                                            
    uint8_t m_pad0x7a[0x1d8 - 0x7a];                                                
    CPdxArray<CAIStrategy::SArmyOrNavySizeObjective, int> _ArmySizeTargets;                               
    CPdxArray<CAIStrategy::SArmyOrNavySizeObjective, int> _NavySizeTargets;                               
    uint8_t m_pad0x208[0x20c - 0x208];                                               
    CCountryTag _WarTarget;                                                                        
    uint8_t m_pad0x214[0x310 - 0x214];                                               
    CEnumIndexedArray<int, SubUnitType, 7> _SubUnits;                                             
    ESpecialUnitCategory _NeedsShips;                                                               
    ESpecialUnitCategory _NeedsRegiments;                                                               
    uint8_t m_pad0x334[0x36c - 0x334];                                               
    TPdxDefVal<int, 0, 1> _nDefendedHomeStrait;                                                              
    uint8_t m_pad0x370[0x372 - 0x370];                                               
    TPdxDefVal<bool, 0, 1> _bIsNavyUseful;                                                             
    uint8_t m_pad0x373[0x380 - 0x373];                                               
};

#include "ai_strategy.tpp"
