#pragma once

#include "monarchinwaiting.h"
#include <cstdint>

class CHeir;
class CToolTip;

class CHeir : public CMonarchInWaiting
{
public:
    void WriteMembers(CWriter &Writer) override;                                                                                
    void ReadMember(CReader &Reader, int Type) override;                                                                       
    bool IsHeir() const override;                                                                                 
    CFixedPoint GetClaimStrength() const override;                                                                           
    void GetTooltip(CToolTip &Tip, CCountryTag For) const override;                                                                          

    virtual void MonthlyUpdate();                                                      

private:
    uint8_t m_pad0x140[0x148 - 0x140];          
};
