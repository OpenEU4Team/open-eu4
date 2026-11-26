#pragma once

#include "clausewitz/pdx_core/fixedpoint.h"
#include "clausewitz/pdx_core/persistent.h"
#include "countrytag.h"
#include <cstdint>

class CString;
enum ModifierType : int;

class CModifier : public CPersistent
{
public:
    ~CModifier() override;                                                                           
    void WriteMembers(CWriter &Writer) override;                                                                         
    void ReadMember(CReader &Reader, int type) override;                                                                

    virtual const CString &GetName() const;                                                    
    virtual void Clear();                                                                   

    virtual CString GetFullDescription(const int nMaxLines, int nLevel, bool bAllOnOneLine, CCountryTag Perspective, bool bShowLimitedVisibilityAsGrey) const;                                                                                

    virtual CString GetFullDescriptionWithMultiplier(const int nMaxLines, int nLevel, bool bAllOnOneLine, CCountryTag Perspective, bool bShowLimitedVisibilityAsGrey, CFixedPoint vMultiplier) const;                                                                                                           

    virtual bool IsProvinceModifier() const;                                                                   
    virtual bool IsReligionSubModifier() const;                                                                   
    virtual bool IsImperialModifier() const;                                                                   
    virtual CFixedPoint GetChecksum() const;                                                            

    CFixedPoint GetValue(ModifierType type) const;                                                                  
    bool GetBoolean(ModifierType type) const;                                                                          
    CFixedPoint GetRawValue(ModifierType modifierType) const;                                                           

private:
    uint8_t m_pad0xc[0xb8 - 0xc];        
};

class CProvince;

class CProvinceModifier : public CModifier
{
protected:
    const CProvince *_pProvince;                                               
};

class CGraphicalModifier : public CModifier
{
public:
    ~CGraphicalModifier() override;                                                                            
    void ReadMember(CReader &Reader, int type) override;                                                                          
    void Clear() override;                                                                                         
    bool IsReligionSubModifier() const override;                                                                                     

    virtual void FindPictureTags();                                                                      

protected:
    uint8_t m_pad0xb8[0x148 - 0xb8];         
};

class CStaticModifier : public CGraphicalModifier
{
public:
    ~CStaticModifier() override;                                                                            
    void ReadMember(CReader &Reader, int Type) override;                                                                       
    const CString &GetName() const override;                                                                       

    CString GetFullDescription(const int nMaxLines, int nLevel, bool bAllOnOneLine, CCountryTag Perspective, bool bShowLimitedVisibilityAsGrey) const override;                                                                                       

    CString GetFullDescriptionWithMultiplier(const int nMaxLines, int nLevel, bool bAllOnOneLine, CCountryTag Perspective, bool bShowLimitedVisibilityAsGrey, CFixedPoint vMultiplier) const override;                                                                                                                  

    bool IsImperialModifier() const override;                                                                       

    virtual CString GetHistoricalExplanationText(CCountryTag CountryTag, int nProvince) const;                                                                                              

    virtual bool IsValid() const;                                                           

protected:
    uint8_t m_pad0x148[0x170 - 0x148];          
};
