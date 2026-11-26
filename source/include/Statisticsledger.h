#pragma once

#include "clausewitz/pdx_core/persistent.h"

class CStatisticsLedger : public CPersistent
{
public:
    static void LogBudgetingHeader();                                                             
};
