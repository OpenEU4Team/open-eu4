#pragma once
#include "clausewitz/clausewitzlib/order.h"
#include "clausewitz/pdx_core/fixedpoint.h"
#include <cstdint>
#include <memory>

class CSpendingList
{
public:
    void AddItem(std::unique_ptr<CCommand> pCommand, CFixedPoint vCost, CFixedPoint vDesire);                                                                                                            
    
    CFixedPoint DoSpending(CFixedPoint vBudget);                                                             

private:
    uint64_t m_pad0x0[0x28 / sizeof(uint64_t)];        
};
