#pragma once
#include "clausewitz/pdx_core/persistent.h"
#include "clausewitz/pdx_core/text.h"
class CSystemSettings : public CPersistent
{
public:
    void LoadSettings(CString FileName, bool bReload);                                                             
    void SaveSettings();                                                                                   
};
