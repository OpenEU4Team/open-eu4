#pragma once

#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include <cstdint>

class CNativeCountryData;

class CNativeCountryData : public CPersistent
{
public:
    ~CNativeCountryData() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                   
    void ReadMember(CReader &Reader, int Type) override;                                                                          

    void ChangeTag(CCountryTag Tag);                                                                

private:
    uint8_t m_pad0xc[0x58 - 0xc];        
};
