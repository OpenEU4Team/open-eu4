#pragma once

#include "eventscope.h"
#include <cstdint>

class CEvent;

class CDelayedEvent : public CPersistent
{
public:
    ~CDelayedEvent() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                              
    void ReadMember(CReader &Reader, int Type) override;                                                                     

    CDelayedEvent(CEvent *event, const CEventScope &scope, int delay);                                                              

    inline int GetRemainingDays() const;                                                        
    inline const CEventScope &GetScope() const;                                         
    inline CEvent *GetEvent() const;                                                      

private:
    uint8_t m_pad0xc[0x10 - 0xc];          
    CEvent *_pEvent;                                           
    CEventScope _Scope;                                       
    int _nDays;                                                   
    uint8_t m_pad0xec[0xf0 - 0xec];         
};

#include "event.tpp"
