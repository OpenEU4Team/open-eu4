#pragma once

#include "history.h"
#include <cstdint>

class CProvinceHistoryEntry : public CHistoryEntry
{
private:
    uint8_t m_pad0x30[0x50 - 0x30];         
};

class CProvinceHistory : public CProvinceHistoryEntry
{
private:
    uint8_t m_pad0x50[0x80 - 0x50];         
};
