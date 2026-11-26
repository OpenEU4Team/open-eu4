#pragma once

class CString;

class CDate
{
public:
    inline int GetUniversalDate() const;                                               
    inline void SetUniversalDate(int nDate);                                         

protected:
    int _nUniversalDate;                                           
};

#include "date.tpp"
