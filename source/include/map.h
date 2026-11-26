#pragma once
#include "clausewitz/pdx_core/fixedpoint.h"
#include "clausewitz/pdx_core/persistent.h"
#include <cstdint>

class CProvince;
class CEU3Idler;

class CMap : public CPersistent
{
public:
    ~CMap() override;                                                                           
    void ReadMember(CReader &Reader, int Type) override;                                                           

    virtual void InitPostReset();                                                              
    virtual int GetMaxNumberOfProvinces();                                                               

    CProvince &GetProvince(unsigned long nProvince);                                                    
    const CProvince &GetProvince(unsigned long nProvince) const;                                         
    static CFixedPoint GetNavalDistanceInfinity();                                                                   
    void SetGraphicsOwner(CEU3Idler *pOwner);                                                       

    static CMap *_pMap;                                
private:
    uint8_t m_pad0xc[0x1afc];        
};
