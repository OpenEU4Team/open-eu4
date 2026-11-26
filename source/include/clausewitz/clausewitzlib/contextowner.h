#pragma once

#include "boost/core/noncopyable.hpp"
#include "clausewitz/pdx_core/pdxarray.h"
#include "clausewitz/pdx_core/text.h"

struct AudioContextAUDIO;
struct AnimationContextANIM;
struct MatchmakingContextMATCHMAKING;
struct ParticleContextPARTICLE;
struct LightContextLIGHT;
struct EntityContextENTITY;
struct AchievementsContextACHIEVEMENTS;
struct CloudStorageContextCLOUDSTORAGE;
struct StoreContextSTORE;
struct UGCContextUGC;
struct BrowserContextBROWSER;
struct ContextPOPS_HANDLE;
class CPdxSocial;
class CPdxGameControllerGroup;

class CContextOwner : private boost::noncopyable_::noncopyable
{
public:
    CContextOwner(const CString &application, const CString &version);                                                   
    ~CContextOwner();                                                                                        

private:
    struct SLegalDocumentData
    {
    private:
        bool _bShouldShowPrivacyPolicy;                                              
        CString _PrivacyURL;                                              
        CString _PrivacyTitle;                                             
        CString _TOSURL;                                                    
        CString _TOSTitle;                                                   
        CString _LegalNoticeURL;                                        
        CString _LegalNoticeTitle;                                       
        CString _LegalNotices;                                      
    };

    AudioContextAUDIO *_AudioContext;                                                  
    AnimationContextANIM *_AnimationContext;                                               
    MatchmakingContextMATCHMAKING *_MatchmakingContext;                                      
    ParticleContextPARTICLE *_ParticleContext;                                            
    LightContextLIGHT *_LightContext;                                                  
    EntityContextENTITY *_EntityContext;                                                
    AchievementsContextACHIEVEMENTS *_AchievementsContext;                                    
    CloudStorageContextCLOUDSTORAGE *_CloudStorageContext;                                     
    StoreContextSTORE *_StoreContext;                                                  
    UGCContextUGC *_UGCContext;                                                               
    BrowserContextBROWSER *_BrowserContext;                                              
    CPdxSocial *_pSocialContext;                                                        
    ContextPOPS_HANDLE *_POContext;                                                     
    CPdxArray<CString, int> _DocumentsToShow;                                                   
    CPdxArray<CString, int> _ShownDocuments;                                                     
    SLegalDocumentData _LegalDocumentData;                                           
    bool _bTelemetryInitialized;                                                                       
    CPdxGameControllerGroup *_pGameControllerGroup;                                        
};
