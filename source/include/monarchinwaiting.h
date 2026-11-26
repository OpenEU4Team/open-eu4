#pragma once

#include "monarch.h"
#include <cstdint>

class CMonarch;
class CToolTip;
class CString;

class CMonarchInWaiting : public CMonarch
{
public:
    void WriteMembers(CWriter &Writer) override;                                                                                  
    void ReadMember(CReader &Reader, int Type) override;                                                                         

    int GetNationDesignerCost(CString *pDesc) const override;                                                                                    

    int GetMaxNumOfPersonalities() const override;                                                                               

    virtual CFixedPoint GetClaimStrength() const = 0;                                                                   

    virtual void GetTooltip(CToolTip &Tip, CCountryTag For) const;                                                                                       

    virtual bool IsForeign() const;                                                               

private:
    uint8_t m_pad0x118[0x140 - 0x118];          
};
