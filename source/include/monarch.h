#pragma once

#include "clausewitz/clausewitzlib/id.h"
#include "clausewitz/pdx_core/fixedpoint.h"
#include "clausewitz/pdx_core/text.h"
#include "countrytag.h"
#include <cstdint>

class CMonarch;
class CSimpleRandom;
class CString;

class CMonarch : public CReferenceObject
{
public:
    ~CMonarch() override;                                                                           
    void WriteMembers(CWriter &Writer) override;                                                                        
    void ReadMember(CReader &Reader, int Type) override;                                                               
    void InitPostRead() override;                                                                               

    virtual void SetName(CString Name);                                                         
    virtual bool IsValid() const;                                                         
    virtual bool IsQueenRegent() const;                                                          
    virtual bool IsHeir() const;                                                         
    virtual bool IsQueen() const;                                                         

    virtual int GetNationDesignerCost(CString *pDesc) const;                                                                          

    virtual int GetMaxNumOfPersonalities() const;                                                                                  
    virtual CCountryTag GetOriginCountry() const;                                                                          
    virtual void InitReligionAndCulture(CSimpleRandom *pRand);                                                                                 

    inline void SetCountry(CCountryTag tag);                                      

protected:
    uint8_t m_pad0x15[0x16 - 0x15];          
    CCountryTag _CountryTag;                                       
    uint8_t m_pad0x1e[0x118 - 0x1e];         
};

#include "monarch.tpp"
