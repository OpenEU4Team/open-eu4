#pragma once

#include "clausewitz/pdx_core/persistent.h"
#include <cstdint>

class CContextOwner;
class CString;
class CIdler;
class CApplicationObserver;
class CApplicationClassObservable;

template <typename Observer, typename ClassObservable>
class TObservable
{
public:
    virtual inline ~TObservable() {}                                                                                                  
    virtual void Attach(Observer *) = 0;                                                                                                                                    
    virtual void Detach(Observer *) = 0;                                                                                                                                    
    virtual ClassObservable *GetClassObservable() = 0;                                                                                                                                
};

template <typename Observer, typename ClassObservable>
class CObservable : public TObservable<Observer, ClassObservable>
{
public:
    void Attach(Observer *) override;                                                                                                                           
    void Detach(Observer *) override;                                                                                                                           
    ClassObservable *GetClassObservable() override;                                                                                                                       

private:
    uint8_t m_pad0x8[0x30 - 0x8];        
};

class CApplicationObservable : public CObservable<CApplicationObserver, CApplicationClassObservable>
{
};

class CWindowId
{
public:
    HELPER void SetHandle(unsigned hWnd);

private:
    unsigned _hWnd;                                
};

class CApplication : public CPersistent, public CApplicationObservable
{
public:
    ~CApplication() override;                                                                                 
    static int CreateApplicationWindow(const CString &Title);                                                                      
    HELPER void SetHeadlessIdler(CIdler *pIdler);
    inline CIdler *GetIdler() const;                                        

private:
    CIdler *_pIdler;                                                       
    uint8_t m_pad0x48[0x2f8 - 0x48];         
};

#include "application.tpp"
