#pragma once

#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include "eu3date.h"
#include <cstdint>

class CRival;

class CRival : public CPersistent
{
public:
    void WriteMembers(CWriter &Writer) override;                                                                      
    void ReadMember(CReader &Reader, int Type) override;                                                             

    CRival();                                                             
    CRival(CCountryTag tag, CEU3Date date);                                                    

    inline CCountryTag GetCountry() const;                                        
    inline void SetCountry(CCountryTag tag);                                    

protected:
    CCountryTag _Country;                            
    CEU3Date _Date;                                            
};

#include "rival.tpp"
