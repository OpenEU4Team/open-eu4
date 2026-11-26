#pragma once

#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/pdxarray.h"
#include "clausewitz/pdx_core/persistent.h"
#include "clausewitz/pdx_core/text.h"

struct StoreContextSTORE;
struct UGCContextUGC;

class CDLCDescriptor : public CPersistent
{
public:
    HELPER const CString &GetFileName() const;
    HELPER bool IsDisabledNotLoggedIn() const;

private:
    uint8_t m_pad0xc[0x90 - 0xc];           
    CString _FileName;                                                         
    uint8_t m_pad0xb0[0x32b - 0xb0];         
    bool _bDisabledNotLoggedIn;                                             
};

class CDLCManager
{
public:
    struct SInitSettings
    {
    public:
        inline SInitSettings() = default;                                                              
        HELPER void SetFolders(const CString &dlc, const CString &builtin, const CString &mod);
        HELPER CPdxArray<CString> &AccessBuiltInDLCNames();

    private:
        StoreContextSTORE *_StoreContext = nullptr;                                
        UGCContextUGC *_UGCContext = nullptr;                                         
        CString _DLCFolder;                                                                  
        CString _BuiltInDLCFolder;                                                       
        CString _ModFolder;                                                           
        CPdxArray<CString> _BuiltInDLCNames;                                               
    };

    static CDLCManager *CreateInstance(bool bSkipOwnerTests);                                                   
    void Initialize(const SInitSettings &Settings);                                                                             
    
    enum class EVerificationResult : int
    {
        DLC_OK,
        DLC_BACKEND_OFFLINE,
        DLC_NOT_OWNED,
        FAILED_CHECKSUM,
        FREE_BUT_ALREADY_OWNED,
        FREE_WRONG_APP_NAME,
        GENERAL_ERROR
    };
    EVerificationResult VerifyAllDLCOwnership(bool bForce);                                                                     
    CArray<CString> GetEnabledDLCNames() const;                                                                               
    const CDLCDescriptor *GetDLCDescriptor(const CString &Name) const;                                                               
    void AddToDisabledList(const CString &Name);                                                                                     
    inline const CPdxArray<CString> &GetEnabledMods() const;                                                       

private:
    uint8_t m_pad0x0[0x88];                 
    CPdxArray<CString> _EnabledMods;                                      
};

#include "dlc.tpp"
