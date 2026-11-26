#pragma once

#include "clausewitz/clausewitzlib/application.h"
#include <memory>

class CEU3Graphics;
class CSession;

class CEU4Application : public CApplication
{
public:
    CEU4Application(CContextOwner &Contexts, const CWindowId &Window);                                                                    
    ~CEU4Application() override;                                                                                          
    void InitInMainThread(bool bStandalone, double &Current);                                                                    
    void InitEU4(double Current);                                                                                    
    void InitMap();                                                                                                  
    HELPER CEU3Graphics *GetGraphics() const;
    inline CSession *GetSession() const;                                             

private:
    CEU3Graphics *_pGraphics;                                                 
    uint8_t m_pad0x300[0x18];                     
    std::unique_ptr<CSession> _pSession;                                    
    uint8_t m_pad0x320[0x328];                    
};

#include "eu4application.tpp"
