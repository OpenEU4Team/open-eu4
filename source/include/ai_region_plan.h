#pragma once
#include "ai_coalition.h"
#include "areas.h"
#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/pdxarray.h"
#include "countrytag.h"
#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

class CProvince;
class CCountry;
class CAIArmy;
class CAIKing;

class CAIRegion
{
public:
    inline int32_t GetIndex() const;                                                                       
    inline const CPdxArray<int> &GetProvinces() const;                                                       
    inline const CPdxArray<const CAIRegion *> &GetNeighbors() const;                                           

private:
    int32_t _nIndex;                                                    
    uint8_t m_pad0x4[0x24];                         
    CPdxArray<int> _Provinces;                                                     
    CPdxArray<const CAIRegion *> _Neighbors;                                   
};

class CAIRegionMap
{
public:
    static int GetNumberOfRegions();                                                                                  
    static const CAIRegion *GetRegion(const CProvince *pProv);                                                         
};

template <typename T>
class CCountryFlagCache
{
public:
    bool GetStatus(T &eStatusOut, CCountryTag ForCountryTag, CCountryTag OnCountryTag) const;                                                                        
    void SetFlag(CCountryTag ForCountryTag, CCountryTag OnCountryTag, T eValue);                                                                                 

private:
    uint64_t m_pad0x0[4];         
};

namespace NAIPlan
{
class CRegionDistanceCacher
{
public:
    struct LocalHasher
    {
        inline size_t operator()(const std::pair<const CAIArmy *, const CAIRegion *> &input) const;                                                                         
    };

    explicit CRegionDistanceCacher(CCountryFlagCache<char> *pAccessCache)                                                                             
        : _pAccessCache(pAccessCache)
    {
    }

    int operator()(const std::pair<const CAIArmy *, const CAIRegion *> &input);                                                                                       

    void CalcAndCache(const CAIArmy *pArmy, const CArray<int> &Regions);                                                                                        

private:
    std::unordered_map<std::pair<const CAIArmy *, const CAIRegion *>, int, LocalHasher> _CachedResult;                                                       
    CCountryFlagCache<char> *_pAccessCache;                                                                                               
};

class CApproximateReachCostCallback : public CAreaCostCallback
{
public:
    inline explicit CApproximateReachCostCallback(const CCountry &Country)                                                                
        : _Country(Country)
    {
    }

    float Cost(const CControllerArea &goal) override;                                                                                               

private:
    const CCountry &_Country;                                                     
};

struct SRegionInfo
{
    const CAIRegion *_pRegion;                                                   
    std::array<float, 7> _AssignedStrength;                                              
    std::array<int, 7> _AssignedSiegeForce;                                                     
    float _vAlliedStrength;                                               
    float _vMyStrength;                                            
    float _vEnemyStrength;                                               
    float _vValueToGain;                                                           
    float _vValueToLose;                                                         
    int32_t _nAlliedSiegeForce;                                                  
    int32_t _nSiegeNeed;                                                          
    int32_t _nMinSiegeNeed;                                                          
    bool _bWeHavePresence;                                                             
    bool _bHomeRegion;                                                             
    bool _bHomeAdjacent;                                                             
    bool _bHomeThreatened;                                                       

    void InitRegion(const CAIRegion *pRegion, CCountryTag MyTag);                                                                               

    float GetProvinceValue(const CProvince &Prov, CCountryTag MyTag) const;                                                                                      
};

struct SAIRegionTask;

struct SRegionAssignment
{
    float _vScore;                                                                                               
    std::unordered_map<CAIArmy *, const CAIRegion *> _Assignments;                                            

    void CalcScore(const CArray<SRegionInfo> &regions);                                                                                      

private:
    friend struct SAIRegionTask;
    float CalcSinglePartialScore(const SRegionInfo &ri, int ring) const;                                                                                             
};

struct SAIRegionTask : public NAICoalition::CCoalitionFormationGeneratorSequenced<CAIArmy, double>::CCoalitionTask
{
    CRegionDistanceCacher *_pDistanceToRegionCache;                                     
    SRegionInfo *_pRegionInfo;                                                     

    double EvaluateCoalition(const std::vector<CAIArmy *> &ArmiesToAssign) const override;                                                                                                        

    virtual ~SAIRegionTask();                                                          
};

SRegionAssignment GenerateBestArmyToRegionAssignment(const CArray<CAIArmy *> &ArmiesToAssign, const CArray<SAIRegionTask> &TasksToAssign);                                                                                                               

void PlanArmyRegions(CAIKing *pKing, CCountryFlagCache<char> *pAccessCache);                                                                              

template <typename UnitListT>
void AddArmyStrengthsToRegions(const UnitListT &UnitList, CArray<NAIPlan::SRegionInfo> &Regions, bool bEnemy);                                                                                                     

}                     

#include "ai_region_plan.tpp"
