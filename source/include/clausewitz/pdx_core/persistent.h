#pragma once

#include <cstdint>

class CReader;
class CWriter;
class CString;

class CPersistent
{
public:
    virtual ~CPersistent();                                                                            
    virtual void Write(CWriter &Writer);                                                                           
    virtual void WriteMembers(CWriter &Writer);                                                                            
    virtual void Read(CReader &Reader);                                                                           
    virtual void ReadMember(CReader &Reader, int Type);                                                                   
    virtual void InitPostRead();                                                                                   

    virtual void InitPostRead(const CString &FileName, int nFirstLine, int nLastLine);                                                                       

    virtual void OnError(const CReader &Reader) const;                                                                
    
    CPersistent();                                                                         
    
    inline int GetToken() const;                                                               
    inline void SetToken(int nToken);                                                        

protected:
    explicit CPersistent(int nToken);                                     

private:
    int _TypeToken;                                 
};

#include "persistent.tpp"
