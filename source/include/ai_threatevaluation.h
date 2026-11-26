#pragma once

#include <cstdint>
#include <set>
#include <unordered_map>
#include <utility>

#include "clausewitz/clausewitzlib/id.h"
#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/fixedpoint.h"
#include "countrytag.h"
#include "eu3date.h"
#include "pathfind.h"

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
class CCountry;
class CUnit;
class CArmy;
class CNavy;
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

namespace NAiThreat
{
struct CIDHash
{
    inline size_t operator()(CID id) const;                                                
};

struct SThreatEntry
{
    int16_t _date;                                      
    uint8_t _nSteps;                                       
    uint8_t _nArmyVersion;                               
    CID _UnitID;                                                    
    bool _bCurrentlyBlocked;                                              
    uint8_t m_pad0xd[0x10 - 0xd];         

    CUnit *GetUnit() const;                                                       

    inline bool operator<(const SThreatEntry &other) const;                                                    
};

struct SUnitEvaluatorForCountry
{
    inline SUnitEvaluatorForCountry(CCountryTag Tag, bool bIncludeNeutral, CStrengthFactorCache &Cache)                                                             
        : _Tag(Tag), _bIncludeNeutral(bIncludeNeutral), _Cache(Cache)
    {
    }

    void EvaluateUnit(const CUnit *pUnit, std::pair<float, float> &Strength, float fMultiplier);                                                                                            

private:
    CCountryTag _Tag;                                                         
    bool _bIncludeNeutral;                                            
    uint8_t m_pad0x9[0x10 - 0x9];        
    CStrengthFactorCache &_Cache;                                  
};

struct SUnitEvaluatorForUnit
{
    inline SUnitEvaluatorForUnit(const CUnit *pOwnUnit, bool bIncludeNeutral, CStrengthFactorCache &Cache, float fOwnStrength)                                                                         
        : _pOwnUnit(pOwnUnit), _bIncludeNeutral(bIncludeNeutral), _Cache(Cache), _fOwnStrength(fOwnStrength)
    {
    }

    bool ShouldAssumeLessDistance(const CUnit *pUnit) const;                                                                                         

    inline CFixedPoint CalcScore(const std::pair<float, float> &Strength);                                                             

    void EvaluateUnit(const CUnit *pUnit, std::pair<float, float> &Strength, float fMultiplier);                                                                                         

private:
    const CUnit *_pOwnUnit;                                                
    bool _bIncludeNeutral;                                            
    uint8_t m_pad0x9[0x10 - 0x9];        
    CStrengthFactorCache &_Cache;                                  
    float _fOwnStrength;                                                   
};

CFixedPoint GetNativeAndRebelThreat(int nProvinceID, CStrengthFactorCache &Cache, const CUnit *pUnit);                                                                                        

bool CanTrustCountry(const CCountry &Me, const CCountry &Them);                                                              

template <typename Evaluator>
CFixedPoint CalcThreat(const CArray<SThreatEntry> &Threats, int maxDays, const std::unordered_map<CID, std::pair<uint8_t, CEU3Date>, CIDHash> &ArmyVersions, Evaluator Eval);                                                                                                                                                                                                             

}                       

class CAIThreatEvaluation
{
public:
    static const CAIThreatEvaluation &GetInstance();                                                        
    static CAIThreatEvaluation &AccessInstance();                                                              
    void UpdateArmy(const CArmy *pArmy);                                                                          
    void UpdateNavy(const CNavy *pNavy);                                                                          

    CFixedPoint CalcArmyThreatForCountry(int nProvinceID, const CEU3Date &maxDate, CCountryTag Tag, bool bIncludeNeutral, CStrengthFactorCache &Cache) const;                                                                                                                      

    CFixedPoint CalcNavyThreatForCountry(int nProvinceID, const CEU3Date &maxDate, CCountryTag Tag, bool bIncludeNeutral, CStrengthFactorCache &Cache) const;                                                                                                                      

    bool ArmyHasNearbyThreat(int nProvinceID, CCountryTag Tag, int nDaysAway, bool bIncludeNeutral) const;                                                                                

    bool NavyHasNearbyThreat(int nProvinceID, CCountryTag Tag, int nDaysAway, bool bIncludeNeutral) const;                                                                                

    CFixedPoint CalcThreatForArmy(int nProvinceID, const CEU3Date &maxDate, const CArmy *pArmy, bool bIncludeNeutral, CStrengthFactorCache &Cache) const;                                                                                                          

    CFixedPoint CalcThreatForArmy(int nProvinceID, const CEU3Date &maxDate, const CArmy *pArmy, bool bIncludeNeutral, CStrengthFactorCache &Cache, CFixedPoint eff) const;                                                                                                                       

    CFixedPoint CalcThreatForNavy(int nProvinceID, const CEU3Date &maxDate, const CNavy *pNavy, bool bIncludeNeutral, CStrengthFactorCache &Cache) const;                                                                                                          

    CFixedPoint CalcThreatForNavy(int nProvinceID, const CEU3Date &maxDate, const CNavy *pNavy, bool bIncludeNeutral, CStrengthFactorCache &Cache, CFixedPoint eff) const;                                                                                                                       

    template <typename Evaluator>
    CFixedPoint CalcThreatForUnit(const CArray<CArray<NAiThreat::SThreatEntry>> &ThreatArray, int nProvinceID, const CEU3Date &maxDate, const CUnit *pUnit, bool bIncludeNeutral, CStrengthFactorCache &Cache, Evaluator Eval) const;                                                                                                                                                                                                 

private:
    bool HasNearbyThreat(const CArray<CArray<NAiThreat::SThreatEntry>> &ThreatArray, int nProvinceID, CCountryTag Tag, int nDaysAway, bool bIncludeNeutral) const;                                                                                                                      

    CArray<CArray<NAiThreat::SThreatEntry>> _ArmyThreatArray;                                             
    CArray<CArray<NAiThreat::SThreatEntry>> _NavyThreatArray;                                             

    std::unordered_map<CID, std::pair<uint8_t, CEU3Date>, NAiThreat::CIDHash> _ArmyVersions;                                                    

    CEU3Date _CompareDate;                                
    bool _bIsInitialized;                                                       
};

class CAISiegeMap
{
public:
    static CAISiegeMap &AccessInstance();                                                      
    static const CAISiegeMap &GetInstance();                                                
    void Move(const CArmy *pArmy, int nTo);                                                 

    int GetFriendlyRegiments(const CArmy *pArmy, int nProv, bool bExcludeAttachedToo) const;                                                                   

private:
    alignas(8) unsigned char m_pad0x0[0x68];        
};

class CNeighborFinder : private CPathFind
{
public:
    using CPathFind::IsValidTarget;

    struct SResultType
    {
        int _nProvince;                                 
        short _nDays;                          
        bool _bHasAccess;                             
    };
};

#include "ai_threatevaluation.tpp"