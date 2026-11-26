#pragma once

#include "clausewitz/pdx_core/fixedpoint.h"
#include "clausewitz/pdx_core/persistent.h"
#include "enum_indexed_array.h"
#include <cstdint>

class CGovernmentInstance;
enum class ESpecialUnitCategory : int;

class CGovernmentInstance : public CPersistent
{
public:
    ~CGovernmentInstance() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                    
    void ReadMember(CReader &Reader, int Type) override;                                                                           

    void BuildAISpecialUnitPreferences(CEnumIndexedArray<CFixedPoint, ESpecialUnitCategory, 23> &scores) const;                                                                                                                                        

private:
    uint8_t m_pad0xc[0x330 - 0xc];        
};
