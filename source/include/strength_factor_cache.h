#pragma once
#include "clausewitz/pdx_core/fixedpoint.h"
#include <cstdint>

class CUnit;
class CProvince;
class CRebelFaction;

class CStrengthFactorCache
{
public:
    virtual ~CStrengthFactorCache();                                                         

    virtual CFixedPointFPML GetStrengthFactor(const CUnit *pUnit, int &nLeaderPips);                                                                                   

    virtual CFixedPointFPML GetRebelAndNativeThreatNumerator(const CProvince *pProv);                                                                                                    

    virtual CFixedPoint GetPotentialRebelStrength(const CRebelFaction *pRebel);                                                                                                  
private:
    uint8_t m_pad0x8[0xa8];        
};

class CStrengthFactorCacheManager
{
public:
    static CStrengthFactorCache &AccessCache();                                                                
};
