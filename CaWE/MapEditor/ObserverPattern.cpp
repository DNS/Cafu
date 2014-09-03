/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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
#include "CompMapEntity.hpp"


using namespace MapEditor;


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


void SubjectT::UpdateAllObservers(MapDocOtherDetailT OtherDetail)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged(this, OtherDetail);
}


void SubjectT::UpdateAllObservers_SelectionChanged(const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_Selection(this, OldSelection, NewSelection);
}

void SubjectT::UpdateAllObservers_Created(const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities)
{
    for (unsigned long ObsNr = 0; ObsNr < m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_Created(this, Entities);
}

void SubjectT::UpdateAllObservers_Created(const ArrayT<MapPrimitiveT*>& Primitives)
{
    for (unsigned long ObsNr = 0; ObsNr < m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_Created(this, Primitives);
}

void SubjectT::UpdateAllObservers_Deleted(const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities)
{
    for (unsigned long ObsNr = 0; ObsNr < m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_Deleted(this, Entities);
}

void SubjectT::UpdateAllObservers_Deleted(const ArrayT<MapPrimitiveT*>& Primitives)
{
    for (unsigned long ObsNr = 0; ObsNr < m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_Deleted(this, Primitives);
}


void SubjectT::UpdateAllObservers_Modified(const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_Modified(this, MapElements, Detail);
}


void SubjectT::UpdateAllObservers_Modified(const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_Modified(this, MapElements, Detail, OldBounds);
}


void SubjectT::UpdateAllObservers_EntChanged(const ArrayT< IntrusivePtrT<CompMapEntityT> >& Entities, EntityModDetailE Detail)
{
    for (unsigned long ObsNr = 0; ObsNr < m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->Notify_EntChanged(this, Entities, Detail);
}


void SubjectT::UpdateAllObservers_EntChanged(const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities, EntityModDetailE Detail)
{
    ArrayT< IntrusivePtrT<CompMapEntityT> > MapEnts;

    for (unsigned long EntNr = 0; EntNr < Entities.Size(); EntNr++)
        MapEnts.PushBack(GetMapEnt(Entities[EntNr]));

    UpdateAllObservers_EntChanged(MapEnts, Detail);
}


void SubjectT::UpdateAllObservers_EntChanged(IntrusivePtrT<cf::GameSys::EntityT> Entity, EntityModDetailE Detail)
{
    ArrayT< IntrusivePtrT<CompMapEntityT> > MapEnts;
    MapEnts.PushBack(GetMapEnt(Entity));

    UpdateAllObservers_EntChanged(MapEnts, Detail);
}


void SubjectT::UpdateAllObservers_VarChanged(const cf::TypeSys::VarBaseT& Var)
{
    for (unsigned long ObsNr = 0; ObsNr < m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->Notify_VarChanged(this, Var);
}


void SubjectT::UpdateAllObservers_SubjectDies()
{
    for (unsigned long ObsNr = 0; ObsNr < m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectDies(this);

    m_Observers.Clear();
}


SubjectT::~SubjectT()
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectDies(this);
}
