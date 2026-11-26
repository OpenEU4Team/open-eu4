#pragma once

#include "date.h"

class CGregorianDate : public CDate
{
public:
    void AddMonths(int nMonths);                                                
    int GetTotalDays() const;                                                        
};
