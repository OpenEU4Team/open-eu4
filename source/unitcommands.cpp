#include "unitcommands.h"
#include "clausewitz/clausewitzlib/id.h"
#include "clausewitz/clausewitzlib/order.h"
#include "clausewitz/clausewitzlib/random.h"
#include "clausewitz/clausewitzlib/randomlog.h"
#include "country.h"
#include "countrytag.h"
#include "engine.h"
#include "enums.h"
#include "gamestate.h"
#include "navy.h"
#include "pathfind.h"
#include "province.h"
#include "unit.h"
#include "unitlist.h"

#include <memory>
#include <utility>

bool CMergeCommand::IsValid() const
{
    bool hasArmy = false;
    bool hasOther = false;
    CList<CUnit *> resolved;
    CUnit *lastUnit = nullptr;

    for (CRef<CUnit> &unitRef : this->_UnitList)
    {
        CReferenceObject *cidObj = CReferenceObject::GetObjectFromID(unitRef.GetId());
        if (cidObj == nullptr)
        {
            return false;
        }
        lastUnit = static_cast<CUnit *>(cidObj);
        resolved.Append(std::move(lastUnit));

        if (lastUnit->IsArmy())
            hasArmy = true;
        else
            hasOther = true;
    }

    if (lastUnit == nullptr)
        return false;
    if (hasArmy && hasOther)
    {
        return false;
    }

    CCountry *country = CCountryDataBase::AccessInstance()->GetCountry(lastUnit->GetCountry());
    bool canMerge = country->CanMergeUnits(resolved, nullptr, false);

    return canMerge;
}

void CMoveCommand::AddDestination(CUnit *pUnit, int nDestination,
                                  bool bClearPrevious, bool bSafeMove,
                                  int nPosition)
{
    (void)nPosition;

    if (CRandomLog::IsEnabled() && CRandom::IsLoggingHotJoin())
    {
        CForbidRandomInScope forbid(true);
        CRandomLog::Log("/mnt/gsg/eu4/eu4/eu4/source/unitcommands.cpp", kDestinationLogSourceLine,
                        "AddDestination, %i, %s", nDestination, bClearPrevious ? "yes" : "no");
    }

    CList<int> path;
    CPathFind pathFindBase;
    CSafePathFind pathFindSafe;

    CPathFind::SFindSettings settings{};
    settings._bDirectIfAdjacent = true;
    settings._HiredByCountryOverride = CCountryTag::FromRaw(kEmptyCountryTag);
    settings._nNumRegimentsForLoadingOverride = -1;

    CProvince *from;
    CProvince *to;
    bool found;
    if (bClearPrevious || pUnit->GetFinalDestination() == nullptr)
    {
        from = pUnit->GetLocation();
        to = &CCurrentGameState::GetInstance()->GetProvince(nDestination);
        settings._bEnableReturnToPreviousNoZocHax = false;
        if (bSafeMove)
            found = pathFindSafe.Find(pUnit, from, to, &path, settings, nullptr, nullptr);
        else
            found = pathFindBase.Find(pUnit, from, to, &path, settings, nullptr, nullptr);
    }
    else
    {
        from = pUnit->GetFinalDestination();
        to = &CCurrentGameState::GetInstance()->GetProvince(nDestination);
        settings._bEnableReturnToPreviousNoZocHax = true;
        if (bSafeMove)
            found = pathFindSafe.Find(pUnit, from, to, &path, settings, nullptr, nullptr);
        else
            found = pathFindBase.Find(pUnit, from, to, &path, settings, nullptr, nullptr);
    }

    if (found)
    {
        if (CRandomLog::IsEnabled())
        {
            CForbidRandomInScope forbid(true);
            CRandomLog::Log("/mnt/gsg/eu4/eu4/eu4/source/unitcommands.cpp", kMovementLogSourceLine,
                            "AddMovement");
        }
        if (pUnit->IsNavy())
            pUnit->GetNavy()->SetGoHomeAtWar(false);
        pUnit->AddMovement(path, bClearPrevious, EStopMissionReason::OverriddenByNewCommand);
    }
}
