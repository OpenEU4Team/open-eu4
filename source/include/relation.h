#pragma once

#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include "eu3date.h"
#include <cstdint>

class CWar;

class CDiplomacyStatus
{
public:
    ~CDiplomacyStatus();                                                              
    const CWar *GetWar() const;                                                             
    bool IsFightingWarTogether() const;                                                                     
    bool HasWar() const;                                                                    
    bool HasRoyalMarriage() const;                                                                     
    CEU3Date GetLastExpireTruceDate() const;                                                                 
    bool HasAlliance() const;                                                                     
    bool HasDependency() const;                                                                     

    inline bool HasTruce() const;                                              

protected:
    uint8_t m_pad0x0[0x45];                
    unsigned char _bTruce : 1;                                             
    uint8_t m_pad0x46[0x48 - 0x46];         
};

#include "relation.tpp"
