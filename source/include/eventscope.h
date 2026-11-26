#pragma once

#include "clausewitz/clausewitzlib/id.h"
#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/cruderandom.h"
#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include <cstdint>
#include <memory>

class CString;

class CSavedEventTarget : public CPersistent
{
public:
    void WriteMembers(CWriter &Writer) override;                                                                                  
    void ReadMember(CReader &Reader, int Type) override;                                                                         

    CSavedEventTarget(const short flags, int provinceId);                                             
    CSavedEventTarget(const short flags, CCountryTag tag);                                                        
    void SetProvince(const int nProvince);                                                                      
    void SetCountryTag(CCountryTag Tag);                                                                                      
    void SetName(const CString &name);                                                                              
    void SetUnitId(CID id);                                                                                     

private:
    int32_t _nProvince;                                             
    CCountryTag _Country;                                           
    CID _UnitId;                                                
    int16_t _Flag;                               
};

struct CEventTargetContainer
{
private:
    CArray<CSavedEventTarget> _SavedEventTargets;                                        
    CArray<CSavedEventTarget> _SavedEventTargetTooltips;                                   
};

class CEventScope : public CPersistent
{
public:
    inline ~CEventScope() override;                                                                     
    void WriteMembers(CWriter &Writer) override;                                                                            
    void ReadMember(CReader &Reader, int Type) override;                                                                   

    inline CEventScope(const CEventScope &Other);                                        
    CEventScope(CEventScope &&Other);                                                   

private:
    CEventScope *_pRoot;                                                                                     
    CEventScope *_pFrom;                                                                      
    CEventScope *_pPrev;                                                                   
    int32_t _nProvince;                                                                      
    CCountryTag _Country;                                                                
    bool _bIsValid;                                                                            
    bool _bIsUndefined;                                                                 
    bool _bConsoleTriggered;                                                               
    bool _bIgnoreGameState;                                                                  
    int32_t _nSeed;                                                                     
    CCrudeRandom _Random;                                                               
    CCountryTag _OverrideTag;                                                                  
    CID _RebelFaction;                                                                           
    CID _UnitId;                                                                        
    CID _EnemyUnitId;                                                                         
    CArray<int> _TargetProvinces;                                                       
    CEventTargetContainer _TargetContainer;                                                 
    std::unique_ptr<CEventScope> _pInternalRoot;                                         
    std::unique_ptr<CEventScope> _pInternalFrom;                                           
    std::unique_ptr<CEventScope> _pInternalPrev;                                             
    const CString *_pContext;                                                                   
};

#include "eventscope.tpp"
