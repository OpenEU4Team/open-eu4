#pragma once

#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include <cstdint>

class CEnvoy;

class CEnvoy : public CPersistent
{
public:
    ~CEnvoy() override;                                                                           
    void WriteMembers(CWriter &Writer) override;                                                                      
    void ReadMember(CReader &Reader, int Type) override;                                                             

    void SetTag(CCountryTag tag);                                                

private:
    uint8_t m_pad0xc[0x38 - 0xc];        
};
