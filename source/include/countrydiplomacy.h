#pragma once
#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/pdxarray.h"
#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include "relation.h"
#include <utility>

class CWar;

namespace NDiplomacy
{
template <typename R>
class CRelationsContainerArray
{
public:
    bool HasEntry(CCountryTag Tag) const;                                                                                         

private:
    CPdxArray<std::pair<int, R *>, int> _Entries;                                            
};
}                        

class CCountryDiplomacy : public CPersistent
{
public:
    ~CCountryDiplomacy() override;                                                      

    inline bool HasWar(CCountryTag Tag) const;                                             

    inline const CDiplomacyStatus *operator[](int nIndex) const;                                               
    inline CDiplomacyStatus *operator[](int nIndex);                                                           

private:
    uint8_t m_pad0xc[0x18 - 0xc];                            
    CArray<CDiplomacyStatus> _Statuses;                                                                
    uint8_t m_pad0x30[0x48 - 0x30];                           
    NDiplomacy::CRelationsContainerArray<CWar> _Wars;                                       
    uint8_t m_pad0x60[0x380 - 0x60];                          
};

#include "countrydiplomacy.tpp"
