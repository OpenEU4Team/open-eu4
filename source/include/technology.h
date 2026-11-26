#pragma once

#include "clausewitz/pdx_core/fixedpoint.h"
#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include <cstdint>

enum TechFlags : int
{
    _TECH_FLAGS_MAY_SUPPORT_REBELS_ = 0,
    _TECH_FLAGS_MAY_PLACE_MERCHANTS_ = 1,
    _TECH_FLAGS_HAS_REDUCED_NAVAL_ATTRITION_ = 2,
    _TECH_FLAGS_ALLOW_CLIENT_STATES_ = 3,
    _TECH_FLAGS_ALLOW_FORCE_MARCH_ = 4,
    _TECH_FLAGS_ALLOW_FABRICATE_CLAIMS_ = 5,
    _TECH_FLAGS_ALLOW_STUDY_TECHNOLOGY_ = 6,
    _TECH_FLAGS_ALLOW_STEAL_MAPS_ = 7,
    _TECH_FLAGS_ALLOW_SABOTAGE_REPUTATION_ = 8,
    _TECH_FLAGS_ALLOW_SLANDER_MERCHANTS_ = 9,
    _TECH_FLAGS_ALLOW_SABOTAGE_RECRUITMENT_ = 10,
    _TECH_FLAGS_ALLOW_SOW_DISCONTENT_ = 11,
    _TECH_FLAGS_ALLOW_AGITATE_FOR_LIBERTY_ = 12,
    _TECH_FLAGS_ALLOW_INFILTRATE_ADMINISTRATION_ = 13,
    _TECH_FLAGS_ALLOW_CORRUPT_OFFICIALS_ = 14,
    _TECH_FLAGS_ALLOW_FLAGSHIPS_ = 15,
    _TECH_FLAGS_ALLOW_DRILLING_ = 16,
    _TECH_FLAGS_NUMOF_ = 17,
};

class CTechnology : public CPersistent
{
public:
    ~CTechnology() override;                                                                            
    void ReadMember(CReader &Reader, int Type) override;                                                                   

    CFixedPoint GetNavalMaintenance() const;                                                         
    CFixedPoint GetSupplyLimit() const;                                                         
    CFixedPoint GetCombatWidth() const;                                                         
    bool Allow(TechFlags which) const;                                                         

private:
    uint8_t m_pad0xc[0x148 - 0xc];        
};
