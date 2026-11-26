#pragma once
#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include <cstdint>

class CRegion;

class CMapArea : public CPersistent
{
public:
    ~CMapArea() override;                                                           
    void Read(CReader &Reader) override;                                                       

    CRegion *AccessRegion() const;                                                                     
    void ChangeRegion(CRegion *pNewRegion);                                                                   
    const CArray<int> &GetProvinces() const;                                                           
    int GetNumberOfOwnedProvinces(CCountryTag Tag) const;                                                                       
    bool IsCoastal() const;                                                                        

private:
    uint8_t m_pad0xc[0x18 - 0xc];          
    CRegion *_pRegion;                                              
    uint8_t m_pad0x20[0x80 - 0x20];         
    CArray<int> _Provinces;                                             
};
