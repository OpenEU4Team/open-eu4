#pragma once
#include "engine.h"
#include "eventscope.h"

inline CEventScope::CEventScope(const CEventScope &Other)
    : CPersistent(PERSISTENT_CLASS_ID), _pRoot(nullptr), _pFrom(nullptr), _pPrev(nullptr), _nProvince(Other._nProvince), _Country(Other._Country), _bIsValid(Other._bIsValid), _bIsUndefined(Other._bIsUndefined), _bConsoleTriggered(Other._bConsoleTriggered), _bIgnoreGameState(Other._bIgnoreGameState), _nSeed(Other._nSeed), _Random(), _OverrideTag(Other._OverrideTag), _RebelFaction(Other._RebelFaction), _UnitId(Other._UnitId), _EnemyUnitId(Other._EnemyUnitId), _TargetProvinces(Other._TargetProvinces), _TargetContainer(Other._TargetContainer), _pInternalRoot(), _pInternalFrom(), _pInternalPrev(), _pContext(nullptr)
{
    this->_Random.Set(static_cast<unsigned>(this->_nSeed), static_cast<unsigned>(this->_nSeed));

    const auto cloneScope = [](CEventScope *pSelf, CEventScope *&pPublic,
                               std::unique_ptr<CEventScope> &pOwner, CEventScope *pSource,
                               const CEventScope *pOther) -> void
    {
        if (pSource == pOther)
        {
            pPublic = pSelf;
            pOwner.reset();
        }
        else
        {
            pOwner.reset(new CEventScope(*pSource));
            pPublic = pOwner.get();
        }
    };
    cloneScope(this, this->_pRoot, this->_pInternalRoot, Other._pRoot, &Other);
    cloneScope(this, this->_pFrom, this->_pInternalFrom, Other._pFrom, &Other);
    cloneScope(this, this->_pPrev, this->_pInternalPrev, Other._pPrev, &Other);
}

inline CEventScope::~CEventScope()
{
}
