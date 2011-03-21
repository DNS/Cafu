/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#include "ObserverPattern.hpp"


using namespace GuiEditor;


ObserverT::ObserverT()
{
}


ObserverT::~ObserverT()
{
}


SubjectT::SubjectT()
{
}


void SubjectT::RegisterObserver(ObserverT* Obs)
{
    // Check if the observer has already been registered (don't register twice).
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        if (m_Observers[ObsNr]==Obs) return;

    // Obs was not registered earlier, register it now.
    m_Observers.PushBack(Obs);
}


void SubjectT::UnregisterObserver(ObserverT* Obs)
{
    const int ObsNr=m_Observers.Find(Obs);

    if (ObsNr!=-1) m_Observers.RemoveAt(ObsNr);
}


void SubjectT::UpdateAllObservers_SelectionChanged(const ArrayT<cf::GuiSys::WindowT*>& OldSelection, const ArrayT<cf::GuiSys::WindowT*>& NewSelection)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_Selection(this, OldSelection, NewSelection);
}


void SubjectT::UpdateAllObservers_Created(const ArrayT<cf::GuiSys::WindowT*>& Windows)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_Created(this, Windows);
}


void SubjectT::UpdateAllObservers_Created(cf::GuiSys::WindowT* Window)
{
    ArrayT<cf::GuiSys::WindowT*> Windows;
    Windows.PushBack(Window);

    UpdateAllObservers_Created(Windows);
}


void SubjectT::UpdateAllObservers_Deleted(const ArrayT<cf::GuiSys::WindowT*>& Windows)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_Deleted(this, Windows);
}


void SubjectT::UpdateAllObservers_Deleted(cf::GuiSys::WindowT* Window)
{
    ArrayT<cf::GuiSys::WindowT*> Windows;
    Windows.PushBack(Window);

    UpdateAllObservers_Deleted(Windows);
}


void SubjectT::UpdateAllObservers_GuiPropertyModified()
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_GuiPropertyModified(this);
}


void SubjectT::UpdateAllObservers_Modified(const ArrayT<cf::GuiSys::WindowT*>& Windows, WindowModDetailE Detail)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_Modified(this, Windows, Detail);
}


void SubjectT::UpdateAllObservers_Modified(cf::GuiSys::WindowT* Window, WindowModDetailE Detail)
{
    ArrayT<cf::GuiSys::WindowT*> Windows;
    Windows.PushBack(Window);

    UpdateAllObservers_Modified(Windows, Detail);
}


void SubjectT::UpdateAllObservers_Modified(const ArrayT<cf::GuiSys::WindowT*>& Windows, WindowModDetailE Detail, const wxString& PropertyName)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_Modified(this, Windows, Detail, PropertyName);
}


void SubjectT::UpdateAllObservers_Modified(cf::GuiSys::WindowT* Window, WindowModDetailE Detail, const wxString& PropertyName)
{
    ArrayT<cf::GuiSys::WindowT*> Windows;
    Windows.PushBack(Window);

    UpdateAllObservers_Modified(Windows, Detail, PropertyName);
}


SubjectT::~SubjectT()
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectDies(this);
}
