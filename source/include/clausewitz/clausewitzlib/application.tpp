#pragma once

HELPER void CWindowId::SetHandle(unsigned hWnd)
{
    this->_hWnd = hWnd;
}

HELPER void CApplication::SetHeadlessIdler(CIdler *pIdler)
{
    this->_pIdler = pIdler;
}

inline CIdler *CApplication::GetIdler() const
{
    return this->_pIdler;
}
