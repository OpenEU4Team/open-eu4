#pragma once

#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include <cstdint>

class CDiplomacy : public CPersistent
{
public:
    ~CDiplomacy() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                           
    void ReadMember(CReader &Reader, int Type) override;                                                                  

private:
    uint8_t m_pad0xc[0x60 - 0xc];        
};
