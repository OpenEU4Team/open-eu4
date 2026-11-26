#pragma once

#include "engine.h"
#include <cstdint>

struct CPixel;
class CProvince;

enum EAdjacencyType : int
{
    _ADJACENCY_NORMAL_ = 0,
    _ADJACENCY_STRAIT_ = 1,
    _ADJACENCY_RIVER_ = 2,
    _ADJACENCY_IMPASSABLE_ = 3,
    _ADJACENCY_CANAL_ = 4,
};

struct SAdjacency
{
protected:
    int _nProvince;                                                     
    int _nFrom;                                                     
    int _nThrough;                                                                   
    uint8_t m_pad0x0c[0x1c - 0x0c];         
    EAdjacencyType _Type;                                            
    uint8_t m_pad0x20[0x24 - 0x20];         

public:
    HELPER int GetProvinceId() const;
    HELPER EAdjacencyType GetType() const;

    SAdjacency(EAdjacencyType Type, int nProvince, int nThrough, int nFrom, const CPixel &ConnectionPoint);                                                               

    const CProvince &GetProvince() const;                                                
    const CProvince &GetFrom() const;                                               
    const CProvince &GetThrough() const;                                                
    CProvince &AccessProvince() const;                                                      
    CProvince &AccessThrough() const;                                                      
};

#include "adjacency.tpp"
