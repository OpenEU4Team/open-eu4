#pragma once

namespace pdx
{
template <class Iterator>
class range
{
public:
    inline Iterator begin() const;                                                  
    inline Iterator end() const;                                                

private:
    Iterator _first;                            
    Iterator _last;                                   
};
}                 

#include "range.tpp"
