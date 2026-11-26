#pragma once

#include "clausewitz/pdx_core/fixedpoint.h"
#include "clausewitz/pdx_core/text.h"
extern "C" bool ReloadPdxLocalize();                                  

class CPdxLocalizeKeyValuePair
{
public:
    CPdxLocalizeKeyValuePair();                                                  

private:
    const char *_pKey;                                                      
    CString _Key;                                                        
    int _Type;                                               
    uint8_t m_pad0x2c[0x30 - 0x2c];         
    CString _StringValue;                                    
    CFixedPoint _FixedPoint;                                           
    uint8_t m_pad0x54[0x58 - 0x54];         
    CFixedPointFPML _FixedPoint64;                                            
    union
    {
        const char *_CharPtr;                                   
        float _Float;                                            
        double _Double;                                            
        int _Int;                                                
        unsigned _UInt;                                            
        void *_CustomData;                               
    };
};

CString PdxLocalizeAndReplace(const char *pStr, const CPdxLocalizeKeyValuePair *pKeyValues, const int nKeyValueCount);                                                                          
