#pragma once

#include "clausewitz/clausewitzlib/application.h"
#include <cstdint>

class CCommand;
class CSessionInfoObserver;
class CSessionInfoClassObservable;

class CSessionInfoObservable : public CObservable<CSessionInfoObserver, CSessionInfoClassObservable>
{
};

enum class ESessionState : int
{
    DISCONNECTED = 0,
    CONNECTING = 1,
    RECONNECTING = 2,
    RECONNECTING_FAILED = 3,
    CONNECTED = 4,
    WAITING_FOR_GAMESTATE = 5,
    ASYNCHRONOUSLY_CONNECTED = 6,
    REFUSED = 7,
    KICKED = 8,
    IS_BANNED = 9,
    STATE_NAME_TAKEN = 10,
    STATE_NAME_INVALID = 11,
    STATE_NAME_AVAILABLE = 12,
    HOTJOIN_ASKING_FOR_JOIN = 13,
    HOTJOIN_WAITING_FOR_SAVE = 14,
    HOTJOIN_IN_LOBBY = 15,
    HOTJOIN_REFUSED = 16,
    BAD_VERSION = 17,
    PASSWORD_REQUEST = 18,
    BAD_PASSWORD = 19,
    STATE_JOIN_DISABLED = 20,
};

class CSession : public CSessionInfoObservable
{
public:
    bool Post(CCommand *command);                                                                                                 
    bool Update();                                                                                                         
    void SetState(ESessionState state);                                                                                                    
    inline void SetBetweenCommandsCallback(void *context, void (*callback)(void *));                                                      

private:
    uint8_t m_pad0x30[0x818 - 0x30];               
    void *_pBetweenCommandsUserData;                                                        
    void (*_pBetweenCommandsFunc)(void *);                                                
    uint8_t m_pad0x828[0x880 - 0x828];              
};

#include "session.tpp"
