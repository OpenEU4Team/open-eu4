#pragma once

#include "clausewitz/clausewitzlib/order.h"
#include "countrytag.h"

enum class ESpecialUnitCategory : int;

class CSetNeedsRegimentsCommand : public CCommand
{
public:
    ~CSetNeedsRegimentsCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                          
    void ReadMember(CReader &Reader, int Type) override;                                                                                 
    bool IsValid() const override;                                                                                           
    void Execute() const override;                                                                                           
    unsigned GetSpecificTokenType() const override;                                                                                        
    bool MustSynch() const override;                                                                                           
    CCommand *Clone(CString SessionName) const override;                                                                          

    CSetNeedsRegimentsCommand(CCountryTag Country, ESpecialUnitCategory category, bool bNeedsMoney);                                                                                      

private:
    uint8_t m_pad0x48[0x58 - 0x48];         
};

class CSetNeedsShipsCommand : public CCommand
{
public:
    ~CSetNeedsShipsCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                      
    void ReadMember(CReader &Reader, int Type) override;                                                                             
    bool IsValid() const override;                                                                                       
    void Execute() const override;                                                                                       
    unsigned GetSpecificTokenType() const override;                                                                                    
    bool MustSynch() const override;                                                                                       
    CCommand *Clone(CString SessionName) const override;                                                                      

    CSetNeedsShipsCommand(CCountryTag Country, ESpecialUnitCategory category, bool needsShips);                                                                                  

private:
    uint8_t m_pad0x48[0x58 - 0x48];         
};