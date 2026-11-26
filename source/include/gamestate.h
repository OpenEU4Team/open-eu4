#pragma once

#include "clausewitz/clausewitzlib/id.h"
#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/pdx/range.h"
#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include "enums.h"
#include "eu3date.h"
#include "gamestate_runtime_settings.h"
#include "province_id.h"
#include <cstdint>

class CProvince;
class CEU3Idler;
enum class EIDCategory : unsigned
{
    COMBAT = 0,
    MONARCH = 1,
    LEADER = 2,
    ADVISOR = 3,
    REBEL = 4,
    DIPLOMACY = 5,
    CARDINAL = 6,
    MERCENARY_COMPANY = 7,
    LOAN = 8,
    UNIT = 9,
    SUBUNIT = 10,
    UNIT_TEMPLATE = 11,
    SIZE = 12,
};

class CGameState : public CPersistent
{
public:
    ~CGameState() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                           
    void ReadMember(CReader &Reader, int Type) override;                                                                  
    void InitPostRead() override;                                                                                  

    virtual int GetMaxNumberOfAllCountries() const;                                                                          
    virtual int GetMaxNumberOfIncomeBoosters();                                                                               

    void RecalculateCachedData();                                                                     
    void SetFederationForMembers();                                                                     
    void StartAI(bool bStart);                                                         
    void SetGraphicsOwner(CEU3Idler *pOwner);                                                              
    CEU3Idler *GetGraphicsOwner() const;                                                          
    void Tick(float vDelta);                                                        
    inline int GetGamespeed() const;                                                   

    CID GenerateCID(EIDCategory category, bool generateNew);                                                             
    bool IsControllerAI(CCountryTag Tag) const;                                                                             
    ControllerType GetControllerType(CCountryTag Tag) const;                                                                   
    void SetController(CCountryTag Tag, ControllerType Type);                                                                             

    inline const CArray<CProvince *> &GetProvinces() const;                                          
    inline const CEU3Date &GetCurrentDate() const;                                                     
    HELPER CProvince &GetProvince(SProvinceId l_var0x01) const;
    inline CProvince &GetProvince(int nProvince) const;                                         

protected:
    uint8_t m_pad0xc[0x1c60 - 0xc];                
    CArray<CProvince *> _Provinces;                                           
    pdx::range<CProvince *> _ProvincesView;                                                  
    uint8_t m_pad0x1c88[0x1d18 - 0x1c88];             
    CArray<ControllerType> _Controllers;                                             
    uint8_t m_pad0x1d30[0x1da0 - 0x1d30];             
    CEU3Date _CurrentDate;                                                    
    uint8_t m_pad0x1da4[0x1dd8 - 0x1da4];             
    int _nGameSpeed;                                                                  
    uint8_t m_pad0x1ddc[0x7420 - 0x1ddc];             
};

class CCurrentGameState : public CGameState
{
public:
    ~CCurrentGameState() override;                                                      

    static inline CCurrentGameState *AccessInstance();                                                   
    static const CCurrentGameState *GetInstance();                                                          
    bool UsingNewExplorationSystem() const;                                                                                

    bool Load(const CString &FileName, bool bReadHeader, bool bCloud);                                                          
    void ApplySavegameFixes();                                                                                                      
    void SetGameStarted(bool bStarted);                                                                                         
    inline bool HasGameStarted() const;                                                                                     
    inline const SGamestateRuntimeSettings &GetRuntimeSettings() const;                                                         

protected:
    static CCurrentGameState *_pInstance;                                                          
    uint8_t m_pad0x7420[0x742c - 0x7420];                 
    bool _bHasGameStarted;                                                               
    uint8_t m_pad0x742d[0x7478 - 0x742d];                 
    SGamestateRuntimeSettings _RuntimeSettings;                                               
};

#include "gamestate.tpp"
