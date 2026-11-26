#pragma once

#include "clausewitz/clausewitzlib/id.h"
#include "countrytag.h"
#include <cstdint>

class CUnit;
class CLeader;
class CToolTip;

class CLeader : public CReferenceObject
{
public:
    ~CLeader() override;                                                                           
    void WriteMembers(CWriter &Writer) override;                                                                       
    void ReadMember(CReader &Reader, int Type) override;                                                              

    virtual bool IsValid() const;                                                         
    virtual CToolTip GetTooltip() const;                                                      

    CUnit *GetUnit() const;                                        
    bool CanExplore() const;                                           
    int GetFire() const;                                           
    int GetShock() const;                                           
    int GetManuever() const;                                            

    inline CCountryTag GetCountry() const;                                       
    inline void SetCountry(CCountryTag tag);                                     

private:
    uint8_t m_pad0x15[0x68 - 0x15];         
    CCountryTag _CountryTag;                                     
    uint8_t m_pad0x70[0xa0 - 0x70];         
};

#include "leader.tpp"
