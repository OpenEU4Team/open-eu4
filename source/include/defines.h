#pragma once

class CFixedPoint;

namespace NDefines
{
namespace NEconomy
{
extern CFixedPoint LAND_MAINTENANCE_FACTOR;                                                                        
extern CFixedPoint MAX_LD_FOR_CONSTRUCT_IN_SUBJECT;                                                                        
}                      

namespace NAI
{
extern CFixedPoint MAX_CAV_PERCENTAGE;                                                                            
extern CFixedPoint MIN_CAV_PERCENTAGE;                                                                            
extern CFixedPoint ARTILLERY_FRACTION;                                                                            
extern int FORCE_COMPOSITION_CHANGE_TECH_LEVEL;                                                                                    
extern int ONLY_INFANTRY_MERCS;                                                                                    
extern CFixedPoint MIN_FORCE_LIMIT_SHARE_REGION_ASSIGN;                                                                            
extern int MAX_ARMIES_NEW_REGION_ASSIGN_ALGORITHM;                                                                                    
extern int MAX_TASKS_NEW_REGION_ASSIGN_ALGORITHM;                                                                                    
extern CFixedPoint REGION_PLANNING_HOMELAND_PRIORIZATION;                                                                            
extern CFixedPoint ACCEPTABLE_BALANCE_MULT_FRIEND_IN_COMBAT;                                                                            
extern CFixedPoint ACCEPTABLE_BALANCE_MULT_OFFENSIVE;                                                                            
extern CFixedPoint ACCEPTABLE_BALANCE_THREAT_WEIGHT;                                                                            
extern CFixedPoint TRANSPORT_FRACTION;                                                                            
extern CFixedPoint INVASION_FLEET_DISTANCE_PENALTY;                                                                            
extern CFixedPoint INVASION_FLEET_MISSION_PENALTY;                                                                            
extern CFixedPoint INVASION_HOME_FLEET_PENALTY;                                                                            
extern CFixedPoint BIGSHIP_FRACTION;                                                                            
extern CFixedPoint INVADING_MAX_AWAY_RATIO;                                                                            
extern CFixedPoint INVADING_BRAVERY;                                                                            
extern CFixedPoint IMPORANT_PROVINCE_THRESHOLD;                                                                            
}                 

namespace NCountry
{
extern CFixedPoint GALLEY_INLAND_SEA_COAST_RATIO;                                                                      
extern CFixedPoint HIGH_LIBERTY_DESIRE;                                                                      
extern int MIN_SHIPS_TO_EXPLORE;                                                                              
}                      

namespace NMilitary
{
extern CFixedPoint INSUFFICIENT_SUPPORT;                                                                      
extern CFixedPoint MOTHBALLING_MINIMUM_STRENGTH;                                                                      
extern CFixedPoint MOTHBALLING_MONTHLY_DECREASE;                                                                      
}                       
}                      
