#pragma once

#include <string>

class CString
{
public:
    CString();                                                            
    CString(const char *s);                                                 
    CString(const char *s, int len);                                         
    CString(const CString &other);                                           
    CString(CString &&Other);                                               
    inline ~CString();                                                    
    void operator=(const CString &other);                                    
    CString &operator=(CString &&Other);                                    
    void SetSize(int nSize, char cFill);                                         
    void AddNewlineIfNeeded();                                                              
    CString &operator+=(CString &&Other);                                   
    inline const char *GetCharPtr() const;                                     

private:
    std::string _str;                               
};

#include "text.tpp"
