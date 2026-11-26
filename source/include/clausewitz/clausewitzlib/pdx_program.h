#pragma once
#include "appinfo.h"
#include "systemsettings.h"
class CPdxProgram
{
public:
    virtual ~CPdxProgram();                                                                               
    static void CreateInstance(CSystemSettings &Settings);                                                                    
    static CPdxProgram &AccessInstance();                                                                    
    void Initialize();                                                                                   
    CAppInfo &AccessAppInfo();                                                                              
};
