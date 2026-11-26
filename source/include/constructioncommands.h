#pragma once

#include "clausewitz/clausewitzlib/id.h"
#include "clausewitz/clausewitzlib/order.h"
#include "countrytag.h"
#include "navy.h"

class CSubUnitDefinition;
class CMercenaryCompany;
enum class ESpecialUnitCategory : int;

class CConstructUnitCommand : public CCommand
{
public:
    ~CConstructUnitCommand() override;                                                                    
    void WriteMembers(CWriter &Writer) override;                                                                              

    void ReadMember(CReader &Reader, int Type) override;                                                                             

    bool IsValid() const override;                                                                                       
    void Execute() const override;                                                                                       
    unsigned GetSpecificTokenType() const override;                                                                                    
    bool MustSynch() const override;                                                                                       
    CCommand *Clone(CString SessionName) const override;                                                                      

    CConstructUnitCommand(CCountryTag who, const CSubUnitDefinition *pSubUnitDefinition, int nProvinceId, ESpecialUnitCategory eCategory);                                                                                                                    
    CConstructUnitCommand(CCountryTag who, const CSubUnitDefinition *pSubUnitDefinition, int nProvinceId, const CMercenaryCompany *pMercenaryCompany);                                                                                                       

private:
    uint8_t m_pad0x48[0x98 - 0x48];         
};

class CCreateUnitCommand : public CCommand
{
public:
    ~CCreateUnitCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                   
    void ReadMember(CReader &Reader, int Type) override;                                                                          
    bool IsValid() const override;                                                                                    
    void Execute() const override;                                                                                    
    unsigned GetSpecificTokenType() const override;                                                                                 
    bool MustSynch() const override;                                                                                    
    CCommand *Clone(CString SessionName) const override;                                                                   

    CCreateUnitCommand();                                            

    CCreateUnitCommand(CCountryTag tag, CID cid, int provinceId, bool createNavy, bool l_var0x05, bool exile, bool canAttach, bool mothball, int previousProvinceId, int previousNoZoCProvinceId, int divisionTarget, bool bWasPrevious, int divisionOrigin, bool bMakeHomeFleet, const CNavy::CMissionSettings *missionSettings, const CMercenaryCompany *merc);                                                                                                                         

private:
    uint8_t m_pad0x48[0x98 - 0x48];         
};
