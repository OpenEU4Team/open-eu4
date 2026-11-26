#pragma once

inline void CSession::SetBetweenCommandsCallback(void *context, void (*callback)(void *))
{
    this->_pBetweenCommandsUserData = context;
    this->_pBetweenCommandsFunc = callback;
}
