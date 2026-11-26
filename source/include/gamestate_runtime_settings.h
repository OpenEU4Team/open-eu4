#pragma once

#include <cstdint>

struct SGamestateRuntimeSettings
{
    bool _bAI;                                                       
    bool _bArmyAI;                                                        
    uint8_t m_pad0x2[0x4 - 0x2];         
    int _nAIInvasionTarget;                                           
    uint8_t m_pad0x8[0x48 - 0x8];        
};
