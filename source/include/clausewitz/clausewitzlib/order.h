#pragma once

#include "clausewitz/pdx_core/persistent.h"
#include "clausewitz/pdx_core/text.h"
#include <cstdint>

class CCommand : public CPersistent
{
public:
    ~CCommand() override;                                                                               
    void WriteMembers(CWriter &Writer) override = 0;                                                                                     
    void ReadMember(CReader &Reader, int Type) override = 0;                                                                            

    virtual bool IsValid() const = 0;                                                                          
    virtual void Execute() const = 0;                                                                          

    virtual unsigned GetSpecificTokenType() const = 0;                                                                                        
    virtual bool MustSynch() const = 0;                                                                                           

    virtual CCommand *Clone(CString SessionName) const = 0;                                                                               

    virtual void PreExecuteProcess();                                                                                
    virtual void PostExecuteProcess();                                                                                
    virtual bool OkDuringHotJoin() const;                                                                           
    virtual bool IgnoreIsAllowedToPostCheck() const;                                                                           
    virtual unsigned GetRecipientAdress() const;                                                                       
    virtual unsigned short GetRecipientPort() const;                                                                 
    virtual void SetRecipientPort(unsigned short nPort);                                                            
    virtual void SetRecipientAdress(unsigned nAdress);                                                                

    inline bool IsTargetedAsynchronous() const;                                                  

protected:
    uint8_t m_pad0xc[0x42 - 0xc];           
    bool _bTargetedAsynchronous;                                                                  
    uint8_t m_pad0x43[0x48 - 0x43];         
};

#include "order.tpp"
