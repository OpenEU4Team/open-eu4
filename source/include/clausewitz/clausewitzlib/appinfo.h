#pragma once
class CAppInfo
{
public:
    enum class AppType : int
    {
        APP_BETA,
        APP_RELEASE,
        APP_UNKNOWN
    };
    void SetAppIds(unsigned Steam, unsigned GOG, unsigned Rail);                                           
    AppType CheckAppType() const;                                                                             
};
