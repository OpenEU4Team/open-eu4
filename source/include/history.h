#pragma once

#include "clausewitz/pdx_core/persistent.h"
#include <cstdint>

class CHistoryEntry : public CPersistent
{
private:
    uint8_t m_pad0xc[0x30 - 0xc];         
};
