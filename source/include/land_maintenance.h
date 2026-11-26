#pragma once

#include "clausewitz/pdx_core/fixedpoint.h"

struct SLandMaintenanceComponent
{
    CFixedPoint _ArmyCost;                                               
    CFixedPoint _OldMercsCost;                                            
    CFixedPoint _MercenaryCompaniesCost;                                      
    CFixedPoint _CondottieriCost;                                       
    CFixedPoint _DrillCost;                                          
};

struct SLandMaintenance
{
    SLandMaintenanceComponent _BaseCost;                                               
    SLandMaintenanceComponent _ReinforcementCost;                                   
    SLandMaintenanceComponent _ExtraCost;                                                    
};

SLandMaintenanceComponent ApplyMaintenanceSlider(const SLandMaintenanceComponent &Component, CFixedPoint vSlider);                                                                                     
