#pragma once
#include <cstdint>

class MD5
{
private:
    uint32_t state[4];                                   
    uint32_t count[2];                                      
    unsigned char buffer[64];                             
    unsigned char digest[16];                               
    unsigned char finalized;                                           
};
