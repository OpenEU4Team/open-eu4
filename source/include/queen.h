#pragma once

#include "monarchinwaiting.h"
#include <cstdint>

class CQueen;
class CToolTip;

class CQueen : public CMonarchInWaiting
{
public:
    void WriteMembers(CWriter &Writer) override;                                                                                 
    void ReadMember(CReader &Reader, int Type) override;                                                                        
    bool IsQueenRegent() const override;                                                                                   
    bool IsQueen() const override;                                                                                  
    CCountryTag GetOriginCountry() const override;                                                                            
    CFixedPoint GetClaimStrength() const override;                                                                            
    void GetTooltip(CToolTip &Tip, CCountryTag For) const override;                                                                           
    bool IsForeign() const override;                                                                                  

private:
    uint8_t m_pad0x140[0x148 - 0x140];          
};
