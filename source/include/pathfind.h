#pragma once
#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/fixedpoint.h"
#include "clausewitz/pdx_core/list.h"
#include "countrytag.h"
#include "engine.h"
#include <cstdint>
#include <unordered_map>

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

class CPathFind
{
public:
    struct SFindSettings
    {
        bool _bIgnoreBlockades;                                                
        bool _bAvoidStraits;                                                
        bool _bIgnoreAccess;                                                
        bool _bIgnoreForts;                                                
        bool _bBothLandAndSea;                                                  
        bool _bPreferSea;                                                
        bool _bCheapExploration;                                                
        bool _bFastRatherThanCorrect;                                                  
        bool _bDirectIfAdjacent;                                                 
        bool _bEnableReturnToPreviousNoZocHax;                                                     
        int _nNumRegimentsForLoadingOverride;                                                    
        CFixedPoint _vAttritionMultiplier;                                          
        CCountryTag _HiredByCountryOverride;                                             
        CFixedPointFPML _vvExtraOwnStrength;                                         
        float _vCurrentThreat;                                                
        bool _bSteerAwayFromThreat;                                                   
        bool _bRandomLog;                                                 
        CStrengthFactorCache *_pStrengthFactorCache;                                 
        CCountryFlagCache<char> *_pAccessCache;                             
        bool _bPathLengthMap;                                                  
    };

    inline virtual ~CPathFind();                                             

    virtual bool IsValidTarget(const CUnit *pUnit, const CProvince *pTarget, const CProvince *pFrom, const CProvince *pTo, const SAdjacency &Adjacency, const CPathFind::SFindSettings &Settings) const;                                                                                                                       

    static CFixedPoint GetRoughlyEstimatedTravelTime(const CProvince *pFirst, const CProvince *pSecond);                                                                             

    bool Find(const CUnit *pUnit, const CProvince *pFrom, const CProvince *pTarget, CList<int> *pPath, const CPathFind::SFindSettings &Settings, const CProvince **pClosestValid, unsigned char *pProvVisitMapExport) const;                                                                                                 

    std::unordered_map<int, CList<int>, std::hash<int>, std::equal_to<int>, std::allocator<std::pair<int const, CList<int>>>> FindAllPaths(const CUnit *, const CProvince *, const CArray<CProvince const *> &, const CPathFind::SFindSettings &, bool, bool) const;                                                                                                       

protected:
    virtual bool IsValidArgument(const CUnit *pUnit, const CProvince *pOrigin, const CProvince *pTarget, const CPathFind::SFindSettings &Settings) const;                                                                                                        

    virtual bool IsTargetReached(const CProvince *pTarget, const CProvince *pProv) const;                                                                           
};

class CSafePathFind : public CPathFind
{
public:
    inline ~CSafePathFind() override;                                                  

    bool IsValidTarget(const CUnit *pUnit, const CProvince *pTarget, const CProvince *pFrom, const CProvince *pTo, const SAdjacency &Adjacency, const CPathFind::SFindSettings &Settings) const override;                                                                                                                                    

    static bool NavalFleetSafetyCheck(const CUnit *pUnit, const CProvince *pTo, const CPathFind::SFindSettings &Settings, bool bFuture);                                                                                                             

    inline explicit CSafePathFind(bool bAllowServerOnlyThreatCheck = false)                                       
        : _bAllowServerOnlyThreatCheck(bAllowServerOnlyThreatCheck)
    {
    }

private:
    bool _bAllowServerOnlyThreatCheck;                                         
    uint8_t m_pad0x9[0x10 - 0x9];             
};

class CSafeAiPathFind : public CSafePathFind
{
public:
    inline CSafeAiPathFind()                                                    
        : CSafePathFind(true)
    {
    }
};

class COwnedPathFind : public CSafePathFind
{
public:
    inline ~COwnedPathFind() override;                                                   

protected:
    bool IsValidTarget(const CUnit *, const CProvince *, const CProvince *, const CProvince *, const SAdjacency &, const CPathFind::SFindSettings &) const override;                                                                                                                                     
};

#include "pathfind.tpp"
