#pragma once
#include "clausewitz/pdx_core/pdxarray.h"
#include "countrytag.h"
#include "engine.h"
#include "province_list.h"
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

class CAreaCostCallback
{
public:
    virtual ~CAreaCostCallback();                                                      

    virtual float Cost(const CControllerArea &) = 0;                                                                                                   
};

struct CNeighborArea
{
    CControllerArea &_Area;                                 
    int _nRefCount;                                             

    inline CNeighborArea(CControllerArea &Area, int nRefCount)                                       
        : _Area(Area), _nRefCount(nRefCount)
    {
    }
};

class CNeighborAreaList : public CList<CNeighborArea>
{
public:
    Iterator Find(const CControllerArea &Area) const;                                                                 
};

class CProvinceList;
class CNeighborAreaList;

class CControllerArea
{
public:
    virtual ~CControllerArea();                                                              
    virtual CCountryTag GetOwner() const;                                                            

    static bool FindBestPathV2(const CControllerArea &start, const CControllerArea &goal, CAreaCostCallback *callback, CList<const CControllerArea *> *path);                                                                                              

    bool CalcIsWarZone(CCountryTag who, bool bIncludeRebels) const;                                                                    
    bool IsAdjacent(const CControllerArea &Area) const;                                                                   

    inline const CProvinceList &GetProvinceList() const;                                                      
    inline int GetSize() const;                                                                       
    inline int GetDivs() const;                                                                       
    inline const CPdxArray<CProvince *> &GetPorts() const;                                             
    inline const CNeighborAreaList &GetNeighborList() const;                                                  

protected:
    virtual void BuildNeighborList();                                                                                 
    virtual void BuildArea(CProvince &Prov);                                                                           
    virtual void AddProvince(CProvince &Prov);                                                                            
    virtual bool RemoveProvince(CProvince &Prov);                                                                            

    virtual void JoinWithControllerArea(CControllerArea &Area);                                                                            
    virtual void TrySplit(CProvince &Prov, CCountryTag Owner);                                                                                   

    CProvinceList _ProvList;                                             
    CNeighborAreaList _NeighborList;                                 
    CPdxArray<CProvince *> _Ports;                                             
    uint8_t m_pad0x50[0x58 - 0x50];          
    int _nDivs;                                                                    
};

class COwnerArea : public CControllerArea
{
public:
    ~COwnerArea() override;                                                              
    CCountryTag GetOwner() const override;                                                       

protected:
    void BuildNeighborList() override;                                                                                         
    void BuildArea(CProvince &Prov) override;                                                                                   
    void TrySplit(CProvince &Prov, CCountryTag Owner) override;                                                                             
};

#include "areas.tpp"
