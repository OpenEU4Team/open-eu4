#pragma once

#include "clausewitz/pdx_core/text.h"

struct SClausewitzCommandLineOptions
{
public:
    SClausewitzCommandLineOptions();                                                       

private:
    CString _UpdateLoc;                                                          
    CString _LocalizationImportLocalized;                                        
    bool _bLocalizationExportUnlocalized;                                       
    bool _bLocalizationImportLocalized;                                       
    bool _bLocalizationTagLocalized;                                       
    bool _bLocalizationPurgeLocalized;                                       
    bool _bLocalizationVerifyParams;                                               
};
