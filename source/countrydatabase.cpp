#include "countrytag.h"

CCountry *CCountryDataBase::GetCountry(CCountryTag tag)
{
    return this->GetDBCountryList()[tag.GetIndex()];
}
