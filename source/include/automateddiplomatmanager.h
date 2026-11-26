#pragma once

#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include <cstdint>

class CEnvoy;
class CAutomatedDiplomatManager;

class CAutomatedDiplomatManager : public CPersistent
{
public:
    ~CAutomatedDiplomatManager() override;                                                                                  
    void WriteMembers(CWriter &Writer) override;                                                                                                
    void ReadMember(CReader &Reader, int Type) override;                                                                                       
    CAutomatedDiplomatManager(CCountryTag tag, CEnvoy &envoy);                                                                       

    inline void SetCountryTag(CCountryTag tag);                                                          

private:
    CCountryTag _ParentCountry;                                                   
    uint8_t m_pad0x14[0x140 - 0x14];         
};

#include "automateddiplomatmanager.tpp"
