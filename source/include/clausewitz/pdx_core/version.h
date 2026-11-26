#pragma once

#include "fixedpoint.h"
#include "persistent.h"
#include "text.h"
#include <cstdint>

#include "md5.h"

class CVersion : public CPersistent
{
public:
    explicit CVersion(const CString &name);                                                       
    ~CVersion() override;                                                                           
    void WriteMembers(CWriter &writer) override;                                                                        
    void ReadMember(CReader &reader, int type) override;                                                               
    void InitPostRead() override;                                                                               
    const CString &GetName() const;                                                             

private:
    CFixedPoint _vVersionNumber;                              
    MD5 _Executable;                                             
    MD5 _Data;                                                  
    CString _szName;                                        
    bool _bFinalized;                                                          
    uint8_t m_pad0x109[7];                
    CString _HexDataDigest;                                                
    CString _HexExeDigest;                                                  
    CString _RawDataDigest;                                             
    CString _RawExeDigest;                                               
};
