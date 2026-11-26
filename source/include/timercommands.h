#pragma once

#include "checksums.h"
#include "clausewitz/clausewitzlib/order.h"
#include "eu3date.h"

class CDailyTickCommand : public CCommand
{
public:
    ~CDailyTickCommand() override;                                                           
    void Execute() const override;                                                                  
    void PostExecuteProcess() override;                                                                        
    HELPER const CArray<CBasicChecksum> &GetChecksums() const;

private:
    CEU3Date _Date;                                                      
    CArray<CBasicChecksum> _Checksums;                                            
};

#include "timercommands.tpp"
