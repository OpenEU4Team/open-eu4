#pragma once

class CCrudeRandom
{
public:
    CCrudeRandom();                                                                       
    void Set(unsigned nValue, unsigned nFirstValue);                                         

private:
    unsigned _nValue;                                         
    unsigned _nFirstValue;                                               
};
