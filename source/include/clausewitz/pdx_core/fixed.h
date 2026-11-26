#pragma once

class fixed
{
public:
    inline fixed()                                
        : m_nVal(0) {} 
    inline explicit fixed(int nValue)                                          
        : m_nVal(nValue) {}
        
    inline int GetUnderlyingValue() const;                                                  
    inline void SetUnderlyingValue(int nValue);                                           

private:
    int m_nVal;                                    
};

#include "fixed.tpp"
