#pragma once

HELPER CEU3Graphics *CEU4Application::GetGraphics() const
{
    return this->_pGraphics;
}

inline CSession *CEU4Application::GetSession() const
{
    return this->_pSession.get();
}
