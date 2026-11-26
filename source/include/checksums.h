#pragma once

#include "clausewitz/pdx_core/array.h"
#include "clausewitz/pdx_core/persistent.h"

class CGameState;

class CBasicChecksum : public CPersistent
{
public:
    inline operator int() const;                                              

private:
    int _nSum;                                   
};

template <typename T>
void CalcChecksums(const CGameState *state, CArray<T> &checksums);                                                                               

#include "checksums.tpp"
