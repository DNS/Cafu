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


using namespace ModelEditor;


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


void SubjectT::UpdateAllObservers_SelectionChanged(ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->Notify_SelectionChanged(this, Type, OldSel, NewSel);
}


void SubjectT::UpdateAllObservers_Created(ModelElementTypeT Type, const ArrayT<unsigned int>& Indices)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->Notify_Created(this, Type, Indices);
}


void SubjectT::UpdateAllObservers_Deleted(ModelElementTypeT Type, const ArrayT<unsigned int>& Indices)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->Notify_Deleted(this, Type, Indices);
}


void SubjectT::UpdateAllObservers_JointChanged(unsigned int JointNr)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->Notify_JointChanged(this, JointNr);
}


void SubjectT::UpdateAllObservers_MeshChanged(unsigned int MeshNr)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->Notify_MeshChanged(this, MeshNr);
}


void SubjectT::UpdateAllObservers_SkinChanged(unsigned int SkinNr)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->Notify_SkinChanged(this, SkinNr);
}


void SubjectT::UpdateAllObservers_GuiFixtureChanged(unsigned int GuiFixtureNr)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->Notify_GuiFixtureChanged(this, GuiFixtureNr);
}


void SubjectT::UpdateAllObservers_AnimChanged(unsigned int AnimNr)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->Notify_AnimChanged(this, AnimNr);
}


void SubjectT::UpdateAllObservers_ChannelChanged(unsigned int ChannelNr)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->Notify_ChannelChanged(this, ChannelNr);
}


void SubjectT::UpdateAllObservers_SubmodelsChanged()
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->Notify_SubmodelsChanged(this);
}


void SubjectT::UpdateAllObservers_AnimStateChanged()
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->Notify_AnimStateChanged(this);
}


SubjectT::~SubjectT()
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->Notify_SubjectDies(this);
}
