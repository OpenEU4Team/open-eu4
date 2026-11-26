#pragma once

class CRandom
{
public:
    static bool IsForbidden();                                                        
    static void SetForBidden(bool bForbid);                                            

    inline static bool IsLoggingHotJoin();                                           

private:
    CRandom();                                

    static bool _bLoggingHotJoin;                                               
};

class CForbidRandomInScope
{
public:
    inline ~CForbidRandomInScope();                                              

    inline explicit CForbidRandomInScope(bool bForbid);                                                               

private:
    bool _bStoredForbid;                                          
};

#include "random.tpp"
