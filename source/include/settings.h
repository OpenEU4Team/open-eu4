#pragma once
#include "clausewitz/clausewitzlib/systemsettings.h"
class CSettings : public CSystemSettings
{
public:
    static CSettings *AccessInstance();                                                
    static void CreateInstance();                                                      
};
