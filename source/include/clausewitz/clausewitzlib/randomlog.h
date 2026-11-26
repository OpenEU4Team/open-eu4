#pragma once

#include <functional>

class CRandomLog
{
public:
    static void CallstackPadding(const std::function<void()> &Function);                                                                                        
    static bool IsEnabled();                                                                                                          
    static void Log(const char *pSourceFile, unsigned nLineNumber, const char *pStr, ...);                                             
};
