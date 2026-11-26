#pragma once

#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include "pdx_def_val.h"
#include <cstdint>

class CSubjectType;

class CSubjectType : public CPersistent
{
public:
    ~CSubjectType() override;                                                                            
    void ReadMember(CReader &Reader, int Type) override;                                                                    

    inline bool JoinsOverlordsWars() const;                                                     
    inline bool IsVoluntary() const;                                                     
    inline bool TransferTradePower() const;                                                     
    inline bool HasColonialParent() const;                                                     
    inline bool EatsOverlordsColonies() const;                                                       
    inline bool UsesMilitaryfocus() const;                                                     

private:
    struct SCopyableMembers
    {
        uint8_t m_pad0x0[0x63];                                       
        TPdxDefVal<bool, false, true> _bIsVoluntary;                                                                   
        uint8_t m_pad0x64[0x66 - 0x64];                                
        TPdxDefVal<bool, false, true> _bTransferTradePower;                                                                 
        uint8_t m_pad0x67[0x69 - 0x67];                                
        TPdxDefVal<bool, false, true> _bJoinsOverlordsWars;                                                            
        uint8_t m_pad0x6a[0x8c - 0x6a];                                
        TPdxDefVal<bool, false, true> _bEatsOverlordsColonies;                                                                
        TPdxDefVal<bool, false, true> _bHasColonialParent;                                                         
        uint8_t m_pad0x8e[0x97 - 0x8e];                                
        TPdxDefVal<bool, false, true> _bUsesMilitaryFocus;                                                          
        uint8_t m_pad0x98[0x3e8 - 0x98];                               
    };

    uint8_t m_pad0xc[0xb8 - 0xc];             
    SCopyableMembers _CopyableMembers;                                                
};

#include "subjecttype.tpp"
