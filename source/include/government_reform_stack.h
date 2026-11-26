#pragma once

#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include <cstdint>

class CGovernmentReformStack;

class CGovernmentReformStack : public CPersistent
{
public:
    ~CGovernmentReformStack() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                       
    void ReadMember(CReader &Reader, int Type) override;                                                                              

    bool IsNomad() const;                                                       

private:
    uint8_t m_pad0xc[0x2f8 - 0xc];        
};
