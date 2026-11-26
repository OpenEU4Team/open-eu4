#include "army.h"

bool CArmy::IsLoaded() const
{
    return this->GetLoadedOn() != nullptr;
}
