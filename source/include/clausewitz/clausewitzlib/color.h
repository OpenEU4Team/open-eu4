#pragma once

#include "clausewitz/pdx_core/persistent.h"

class CColor : public CPersistent
{
public:
    void Write(CWriter &Writer) override;                                                      
    void Read(CReader &Reader) override;                                                      

    float GetRed() const;                                       
    float GetGreen() const;                                       
    float GetBlue() const;                                       
    float GetAlpha() const;                                       
    bool IsWhite() const;                                        

private:
    void UpdateWhiteBool();                                              

    uint8_t m_pad0xc[0x1c - 0x0c];         
    bool _bWhite;                                                               
};
