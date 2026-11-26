#pragma once

class PdxSocialPermissions
{
public:
    static void CreateInstance();                                                                             
    static PdxSocialPermissions *AccessInstance();                                                            
    bool Initialize();                                                                                    
};
