#pragma once

#include "clausewitz/clausewitzlib/order.h"
#include "clausewitz/clausewitzlib/ref.h"
#include "clausewitz/pdx_core/list.h"

class CUnit;
class CSubUnit;
template <typename T>
class CArray;
enum class EStopMissionReason : unsigned char;

class CAddUnsafeMoveCommand : public CCommand
{
public:
    ~CAddUnsafeMoveCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                      
    void ReadMember(CReader &Reader, int Type) override;                                                                             
    bool IsValid() const override;                                                                                       
    void Execute() const override;                                                                                       
    unsigned GetSpecificTokenType() const override;                                                                                    
    bool MustSynch() const override;                                                                                       
    CCommand *Clone(CString SessionName) const override;                                                                      

    CAddUnsafeMoveCommand(CUnit *unit, int provinceId, int nMonths);                                                       

private:
    uint8_t m_pad0x48[0x60 - 0x48];         
};

class CAttachUnitCommand : public CCommand
{
public:
    ~CAttachUnitCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                   
    void ReadMember(CReader &Reader, int Type) override;                                                                          
    bool IsValid() const override;                                                                                    
    void Execute() const override;                                                                                    
    unsigned GetSpecificTokenType() const override;                                                                                 
    bool MustSynch() const override;                                                                                    
    CCommand *Clone(CString SessionName) const override;                                                                   

    CAttachUnitCommand(const CUnit *pUnit, const CUnit *pTarget);                                                      

private:
    uint8_t m_pad0x48[0x68 - 0x48];         
};

class CDetachDamagedCommand : public CCommand
{
public:
    ~CDetachDamagedCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                      
    void ReadMember(CReader &Reader, int Type) override;                                                                             
    bool IsValid() const override;                                                                                       
    void Execute() const override;                                                                                       
    unsigned GetSpecificTokenType() const override;                                                                                    
    bool MustSynch() const override;                                                                                       
    CCommand *Clone(CString SessionName) const override;                                                                      

    CDetachDamagedCommand(const CUnit *pUnit);                                                      

private:
    uint8_t m_pad0x48[0x58 - 0x48];         
};

class CDetachUnitCommand : public CCommand
{
public:
    ~CDetachUnitCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                   
    void ReadMember(CReader &Reader, int Type) override;                                                                          
    bool IsValid() const override;                                                                                    
    void Execute() const override;                                                                                    
    unsigned GetSpecificTokenType() const override;                                                                                 
    bool MustSynch() const override;                                                                                    
    CCommand *Clone(CString SessionName) const override;                                                                   

    CDetachUnitCommand(const CUnit *pUnit);                                                   

private:
    uint8_t m_pad0x48[0x58 - 0x48];         
};

class CDisbandCommand : public CCommand
{
public:
    ~CDisbandCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                
    void ReadMember(CReader &Reader, int Type) override;                                                                       
    bool IsValid() const override;                                                                                 
    void Execute() const override;                                                                                 
    unsigned GetSpecificTokenType() const override;                                                                              
    bool MustSynch() const override;                                                                                 
    CCommand *Clone(CString SessionName) const override;                                                                

    CDisbandCommand(const CUnit *pUnit, bool bDeleteOnlyIfEmpty);                                                 

private:
    uint8_t m_pad0x48[0x60 - 0x48];         
};

class CDisbandSubUnitCommand : public CCommand
{
public:
    ~CDisbandSubUnitCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                       
    void ReadMember(CReader &Reader, int Type) override;                                                                              
    bool IsValid() const override;                                                                                        
    void Execute() const override;                                                                                        
    unsigned GetSpecificTokenType() const override;                                                                                     
    bool MustSynch() const override;                                                                                        
    CCommand *Clone(CString SessionName) const override;                                                                       

    CDisbandSubUnitCommand();                                                                      
    explicit CDisbandSubUnitCommand(CSubUnit *sub);                                                         

private:
    uint8_t m_pad0x48[0x58 - 0x48];         
};

class CForceMarchCommand : public CCommand
{
public:
    ~CForceMarchCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                   
    void ReadMember(CReader &Reader, int Type) override;                                                                          
    bool IsValid() const override;                                                                                    
    void Execute() const override;                                                                                    
    unsigned GetSpecificTokenType() const override;                                                                                 
    bool MustSynch() const override;                                                                                    
    CCommand *Clone(CString SessionName) const override;                                                                   

    CForceMarchCommand(const CUnit *pUnit);                                                   

private:
    uint8_t m_pad0x48[0x58 - 0x48];         
};

class CMainArmyCommand : public CCommand
{
public:
    ~CMainArmyCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                 
    void ReadMember(CReader &Reader, int Type) override;                                                                        
    bool IsValid() const override;                                                                                  
    void Execute() const override;                                                                                  
    unsigned GetSpecificTokenType() const override;                                                                               
    bool MustSynch() const override;                                                                                  
    CCommand *Clone(CString SessionName) const override;                                                                 

    CMainArmyCommand(CUnit *unit, bool isMain, bool hunterKiller, bool isInvasionMain);                                                   

private:
    uint8_t m_pad0x48[0x60 - 0x48];         
};

class CMergeCommand : public CCommand
{
public:
    ~CMergeCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                              
    void ReadMember(CReader &Reader, int Type) override;                                                                     
    bool IsValid() const override;                                                                               
    void Execute() const override;                                                                               
    unsigned GetSpecificTokenType() const override;                                                                            
    bool MustSynch() const override;                                                                               
    CCommand *Clone(CString SessionName) const override;                                                              

    CMergeCommand(const CList<CUnit *> &list);                                                       

protected:
    CList<CRef<CUnit>> _UnitList;                              
};

class CMoveCommand : public CCommand
{
public:
    ~CMoveCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                             
    void ReadMember(CReader &Reader, int Type) override;                                                                    
    bool IsValid() const override;                                                                              
    void Execute() const override;                                                                              
    unsigned GetSpecificTokenType() const override;                                                                           
    bool MustSynch() const override;                                                                              
    CCommand *Clone(CString SessionName) const override;                                                             

    CMoveCommand();                                      

    CMoveCommand(CUnit *pUnit, int nProvince, bool bClearPrevious, bool bSafeMove, bool bCancelAutoTransport, bool bClearAllButNext);                                                 

    CMoveCommand(CUnit *pUnit, const CArray<int> &path, bool bClearPrevious, bool bSafeMove, bool bClearAllButNext);                                                           

private:
    static void AddDestination(CUnit *pUnit, const int nDestination, const bool bClearPrevious, bool bSafeMove, int nPosition);                                                              

    uint8_t m_pad0x48[0x98 - 0x48];         
};

class CPerformSlaveRaidCommand : public CCommand
{
public:
    ~CPerformSlaveRaidCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                         
    void ReadMember(CReader &Reader, int Type) override;                                                                                
    bool IsValid() const override;                                                                                          
    void Execute() const override;                                                                                          
    unsigned GetSpecificTokenType() const override;                                                                                       
    bool MustSynch() const override;                                                                                          
    CCommand *Clone(CString SessionName) const override;                                                                         

    CPerformSlaveRaidCommand(const CUnit *unit);                                                         

private:
    uint8_t m_pad0x48[0x58 - 0x48];         
};

class CStopUnitMissionCommand : public CCommand
{
public:
    ~CStopUnitMissionCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                        
    void ReadMember(CReader &Reader, int Type) override;                                                                               
    bool IsValid() const override;                                                                                         
    void Execute() const override;                                                                                         
    unsigned GetSpecificTokenType() const override;                                                                                      
    bool MustSynch() const override;                                                                                         
    CCommand *Clone(CString SessionName) const override;                                                                        

    CStopUnitMissionCommand(const CUnit *unit, EStopMissionReason reason, CID cid);                                                                                

private:
    uint8_t m_pad0x48[0x68 - 0x48];         
};

class CTransferSubUnitCommand : public CCommand
{
public:
    ~CTransferSubUnitCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                        
    void ReadMember(CReader &Reader, int Type) override;                                                                               
    bool IsValid() const override;                                                                                         
    void Execute() const override;                                                                                         
    unsigned GetSpecificTokenType() const override;                                                                                      
    bool MustSynch() const override;                                                                                         
    CCommand *Clone(CString SessionName) const override;                                                                        

    CTransferSubUnitCommand(CID unit, CSubUnit *pSub);                                                              

private:
    uint8_t m_pad0x48[0x60 - 0x48];         
};

class CUnloadArmyCommand : public CCommand
{
public:
    ~CUnloadArmyCommand() override;                                                                            
    void WriteMembers(CWriter &Writer) override;                                                                                   
    void ReadMember(CReader &Reader, int Type) override;                                                                          
    bool IsValid() const override;                                                                                    
    void Execute() const override;                                                                                    
    unsigned GetSpecificTokenType() const override;                                                                                 
    bool MustSynch() const override;                                                                                    
    CCommand *Clone(CString SessionName) const override;                                                                   

    CUnloadArmyCommand(CUnit *unit);                                                  

private:
    uint8_t m_pad0x48[0x58 - 0x48];         
};
