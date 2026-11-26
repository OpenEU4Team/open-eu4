#pragma once

#include "text.h"
#include <sstream>

class CLogStream;

class CLogger
{
public:
    enum ELoggerType : int
    {
        LOGGER_TYPE_DEBUG_WINDOW = 0,
        LOGGER_TYPE_FILE = 1,
        LOGGER_TYPE_FILTER = 2,
        LOGGER_TYPE_NULL = 3,
        LOGGER_TYPE_CONSOLE = 4
    };

    virtual ~CLogger();                                                          
    virtual ELoggerType GetType() = 0;                                                               

    virtual void Log(int nCategory, const CString &File, int nLine, const CString &Message) = 0;                                                                         

    virtual CString GetLogFileName(const CString &Name) const;                                                                   
    virtual CString GetLogFileName(int) const;                                                                          
    virtual unsigned GetLogCount(int) const;                                                                         
    
    static CLogger *AccessInstance();                                                                       

private:
    CString *_pOpenLogCommand;                                                 
    CString *_pOpenLogCommandPostfix;                              
    unsigned _nLogCount;                                               
};

class CLogStream : public std::ostringstream
{
public:
    ~CLogStream() override;                                                                                                            
    CLogStream(CLogger *pLogger, const char *fileName, unsigned nLineNo, int nCategory);                                                 

    [[gnu::abi_tag("cxx11")]] std::ostringstream &operator<<(const char *pVal);                                               
    [[gnu::abi_tag("cxx11")]] std::ostringstream &operator<<(char *pVal);                                                    
    [[gnu::abi_tag("cxx11")]] std::ostringstream &operator<<(const CString &Val);                                                    
    [[gnu::abi_tag("cxx11")]] std::ostringstream &operator<<(int nVal);                                                     
    [[gnu::abi_tag("cxx11")]] std::ostringstream &operator<<(unsigned nVal);                                                
    [[gnu::abi_tag("cxx11")]] std::ostringstream &operator<<(float fVal);                                                   

private:
    CLogger *_pLogger;                                        
    const char *_pFileName;                                
    int _nLineNo;                                     
    int _nCategory;                                        
};
