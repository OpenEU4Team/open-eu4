#pragma once
#include "clausewitz/pdx_core/persistent.h"
#include <cstdint>
#define HELPER inline

class CProvinceValue : public CPersistent
{
public:
    void WriteMembers(CWriter &Writer) override;                                                                               
    void ReadMember(CReader &Reader, int Type) override;                                                                      

    HELPER int32_t GetScore() const;
    HELPER void SetScore(int32_t value);
    HELPER int32_t GetId() const;
    HELPER void SetId(int32_t value);

protected:
    int32_t id;                            
    int32_t val;                              
};

class CProvinceValueDescComp
{
public:
    inline bool operator()(const CProvinceValue &a, const CProvinceValue &b) const;                                                    
    inline bool operator()(const CProvinceValue *a, const CProvinceValue *b) const;                                                    
};

#include "values.tpp"
