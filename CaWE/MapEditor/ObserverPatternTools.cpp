/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ObserverPatternTools.hpp"


ToolsSubjectT::ToolsSubjectT()
{
}


void ToolsSubjectT::RegisterObserver(ToolsObserverT* Obs)
{
    // Check if the observer has already been registered (don't register twice).
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        if (m_Observers[ObsNr]==Obs) return;

    // Obs was not registered earlier, register it now.
    m_Observers.PushBack(Obs);
}


void ToolsSubjectT::UnregisterObserver(ToolsObserverT* Obs)
{
    const int ObsNr=m_Observers.Find(Obs);

    if (ObsNr!=-1) m_Observers.RemoveAt(ObsNr);
}


void ToolsSubjectT::UpdateAllObservers(ToolT* Tool, ToolsUpdatePriorityT Priority)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged(this, Tool, Priority);
}


ToolsSubjectT::~ToolsSubjectT()
{
    // for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
    //     m_Observers[ObsNr]->NotifySubjectDies(this);
}
