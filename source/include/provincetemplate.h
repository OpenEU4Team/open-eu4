#pragma once
#include "adjacency.h"
#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/persistent.h"
#include "enums.h"
#include <cstdint>

class CMapArea;
class CProvince;

class CProvinceTemplate : public CPersistent
{
public:
    ~CProvinceTemplate() override;                                                             
    void Write(CWriter &Writer) override;                                                                  

    ESeaType GetSeaType() const;                                                                    
    bool IsIsland(bool bExcludeStraits) const;                                                   

    inline bool IsLand() const;                                                                       
    inline const CArray<SAdjacency> &GetNeighbours() const;                                                  
    inline int GetSeaZoneForPort() const;                                                                        
    inline int GetNavalIndex() const;                                                                        
    inline CMapArea *GetMapArea() const;                                                                  

protected:
    uint8_t m_pad0xc[0x34];                   
    CMapArea *_pMapArea;                                                              
    uint8_t m_pad0x48[0x108];                  
    int _nProvince;                                                                       
    uint8_t m_pad0x154[0x14];                   
    CArray<SAdjacency> _Neighbours;                                                
    uint8_t m_pad0x180[0x20];                   
    int _nSeaZone;                                                                            
    uint8_t m_pad0x1a4[0xc];                    
    int _nNavalIndex;                                                      
    uint8_t m_pad0x1b4[0x74];                   
    bool _bNeedsUpdate : 1;                                                   
    bool _bTI : 1;                                                           
    bool _bFOW : 1;                                                                    
    bool _bIsLand : 1;                                                 
    bool _bIsLake : 1;                                                   
    bool _bHasTradeWind : 1;                                                   
    bool _bIsCoastal : 1;                                                  
    bool _bIsCoastalSea : 1;                                           
    uint8_t m_pad0x229[0x230 - 0x229];          
};

#include "provincetemplate.tpp"
