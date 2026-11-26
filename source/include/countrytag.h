#pragma once

#include "clausewitz/pdx_core/pdxarray.h"
#include <cstdint>
#include <functional>

class CCountry;

#define HELPER inline

enum CountryType : unsigned char
{
    _COUNTRY_NORMAL_ = 0,
    _COUNTRY_COLONY_ = 1,
    _COUNTRY_CLIENT_ = 2,
    _COUNTRY_ESTATE_ = 3,
    _COUNTRY_FEDERATION_ = 4,
    _COUNTRY_TRADING_CITY_ = 5,
    _COUNTRY_CUSTOM_ = 6,
    _COUNTRY_OBSERVER_ = 7,
};

class CCountryTag
{
public:
    bool IsDynamic() const;                                                  
    bool IsRebels() const;                                                  
    const char *GetTag() const;                                           

    inline CCountryTag()                                            
        : _Tag{'\0', '\0', '\0', '\0'},
          _nIndex(0),
          _Type(0),
          _bIsValid(0)
    {
    }

    inline int GetIndex() const;                                                     
    inline bool IsPirates() const;                                                  
    inline bool IsNatives() const;                                                  
    inline bool IsRebelPirateNative() const;                                                    
    inline bool IsValid() const;                                                    
    inline bool CalcIsValid() const;                                                  

    static HELPER CCountryTag FromRaw(uint64_t value);
    HELPER uint64_t GetRaw() const;
    HELPER int GetType() const;
    HELPER CountryType GetCountryType() const;
    HELPER uint32_t GetCategoryWord() const;
    static HELPER bool SameUpperWord(CCountryTag a, CCountryTag b);

protected:
    static int _nRebelIndex;                                                  
    static int _nPirateIndex;                                                  
    static int _nNativesIndex;                                                  

    char _Tag[4];                                                                     
    int16_t _nIndex;                                          
    uint8_t _Type;                              
    bool _bIsValid;                                                          
};

bool operator==(CCountryTag lhs, CCountryTag rhs);                                    

template <>
struct std::hash<CCountryTag>
{
    inline size_t operator()(CCountryTag tag) const;                                                    
};

class CCountryDataBase
{
public:
    CCountryDataBase();                                                             
    CCountry *GetCountry(CCountryTag tag);                                                                

    inline static const CCountryDataBase *GetInstance();                                                 
    inline int GetMaxNumberOfCountries() const;                                                                         
    inline static CCountryDataBase *AccessInstance();                                                       
    inline CPdxArray<CCountry *, int> &GetDBCountryList();                                                       

protected:
    static CCountryDataBase *_pInstance;                                                       
    uint8_t m_pad0x0[0xf0];                         
    CPdxArray<CCountry *, int> _CountryPtrs;                                
    uint8_t m_pad0x108[0x7e0 - 0x108];                
};

#include "countrytag.tpp"
