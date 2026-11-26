#pragma once

class CTiming
{
public:
    static void CreateInstance();                                                  
    static CTiming *AccessInstance();                                              
    void StartupBegin();                                                         
};
