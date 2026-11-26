#pragma once

#include "id.h"

class CUnit;

template <typename T>
class CRef
{
public:
    inline CID GetId() const;                                    

private:
    mutable T *_pPointer;                                     
    CID _id;                                                         
};

#include "ref.tpp"
