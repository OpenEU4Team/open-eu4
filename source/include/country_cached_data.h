#pragma once

#include "clausewitz/pdx_core/fixedpoint.h"
#include <cstdint>

class CCountryModifiers;

class CCountryCachedData
{
public:
    ~CCountryCachedData();                                                                    
    const CCountryModifiers &GetModifiers() const;                                                         
    CFixedPointFPML GetArmyPower() const;                                                                  

    inline CFixedPoint GetWarThreat() const;                                                  

private:
    uint8_t m_pad0x0[0x6a98 - 0x0];              
    CFixedPointFPML _vArmyPower;                                            
    uint8_t m_pad0x6aa0[0x6c8c - 0x6aa0];           
    CFixedPoint _vWarThreat;                                                           
    uint8_t m_pad0x6c90[0x6d08 - 0x6c90];           
};

#include "country_cached_data.tpp"
