#pragma once

#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include <cstdint>

class CPlayerAiPrefs;

class CPlayerAiPrefs : public CPersistent
{
public:
    void WriteMembers(CWriter &Writer) override;                                                                               
    void ReadMember(CReader &Reader, int Type) override;                                                                      

    bool IsEnabled() const;                                                             
    bool IsChangeFleetMissions() const;                                                              
    bool IsDisbandUnits() const;                                                              

private:
    uint8_t m_pad0xc[0x28 - 0xc];        
};
