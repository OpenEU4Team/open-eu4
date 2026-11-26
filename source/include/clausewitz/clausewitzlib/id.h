#pragma once

#include "clausewitz/pdx_core/persistent.h"
#include <set>

class CID
{
public:
    CString ToString() const;                                    

    inline bool operator==(const CID &Id) const;                                   
    inline unsigned GetType() const;                                          
    inline unsigned GetID() const;                                            
    inline unsigned GetHashValue() const;                                          

protected:
    unsigned _Type;                               
    unsigned _ID;                                                
};

class CReferenceObject : public CPersistent
{
public:
    ~CReferenceObject() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                 
    void ReadMember(CReader &Reader, int Type) override;                                                                        

    virtual bool Create();                                                                                           
    virtual bool Create(const CID &Id, bool bFailIfExists);                                                                

    CReferenceObject();                                                                               
    static void TotalInit(const std::set<int> &Types, int nMachine, int nGame);                                                                             
    explicit CReferenceObject(const CID &Id);                                                              
    static CReferenceObject *GetObjectFromID(const CID &Id);                                                              
    bool operator==(const CReferenceObject &Other) const;                                                 
    bool operator!=(const CReferenceObject &Other) const;                                                 

    inline const CID &GetID() const;                                           
    inline bool IsCreated() const;                                               

protected:
    CID _ID;                                          
    bool _bIsCreated;                                          
};

#include "id.tpp"
