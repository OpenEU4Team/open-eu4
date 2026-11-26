#pragma once

class CProvince;
class CSelectionList;
class CMultiStopMissionAndPerformAction;
class CToolTip;
class CString;
enum SelectableType : int;

class CSelectable
{
public:
    virtual ~CSelectable();                                                                    
    virtual SelectableType GetType() const = 0;                                                         
    virtual bool IsSelectable() const = 0;                                                                    

    virtual CToolTip GetTooltip(const CProvince *pTarget) const = 0;                                                                        
    virtual void OnSelect(CSelectionList *pList) = 0;                                                                                        
    virtual void OnDeSelect() = 0;                                                                                              
    virtual bool PerformAction(CProvince *pTarget, bool status) = 0;                                                                          

    virtual bool CanPerformActionImmediately(CMultiStopMissionAndPerformAction &ActionsToPerform, CProvince *pTarget, bool status);                                                                                                                          

    virtual const CString &GetName() const = 0;                                                         

    inline bool IsSelected() const;                                                  
    inline void SetSelected(bool bSelected);                                          

protected:
    bool _IsSelected;                                  
};

#include "selectable.tpp"
