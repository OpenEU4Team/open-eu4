#pragma once
#include "ai_agent.h"
#include "unitlist.h"

inline CCountry &CAIAgent::GetCountry() const
{
    return *CCountryDataBase::AccessInstance()->GetCountry(this->GetCountryTag());
}
inline CCountryTag CAIAgent::GetCountryTag() const
{
    return this->_Country;
}
inline CList<CAIAgent *> &CAIAgent::GetAgentList()
{
    return this->_AgentList;
}
inline const CID &SUnitToUnit::GetKey() const
{
    return this->_UnitID;
}

inline CUnit &CAIAgent::TrySplitUnit(CUnit &Unit)
{
    return Unit;
}

inline bool CAIAgent::IsCanceled()
{
    return this->_IsCanceled;
}

inline bool CAIAgent::IsFinished()
{
    return this->_IsFinished;
}
