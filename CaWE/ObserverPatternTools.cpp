/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
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
