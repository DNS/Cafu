/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ObserverPattern.hpp"
#include "GuiSys/Window.hpp"


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


void SubjectT::UpdateAllObservers_SelectionChanged(const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& OldSelection, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& NewSelection)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_Selection(this, OldSelection, NewSelection);
}


void SubjectT::UpdateAllObservers_Created(const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_Created(this, Windows);
}


void SubjectT::UpdateAllObservers_Created(IntrusivePtrT<cf::GuiSys::WindowT> Window)
{
    ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > Windows;
    Windows.PushBack(Window);

    UpdateAllObservers_Created(Windows);
}


void SubjectT::UpdateAllObservers_Deleted(const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_Deleted(this, Windows);
}


void SubjectT::UpdateAllObservers_Deleted(IntrusivePtrT<cf::GuiSys::WindowT> Window)
{
    ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > Windows;
    Windows.PushBack(Window);

    UpdateAllObservers_Deleted(Windows);
}


void SubjectT::UpdateAllObservers_GuiPropertyModified()
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_GuiPropertyModified(this);
}


void SubjectT::UpdateAllObservers_Modified(const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows, WindowModDetailE Detail)
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectChanged_Modified(this, Windows, Detail);
}


void SubjectT::UpdateAllObservers_Modified(IntrusivePtrT<cf::GuiSys::WindowT> Window, WindowModDetailE Detail)
{
    ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > Windows;
    Windows.PushBack(Window);

    UpdateAllObservers_Modified(Windows, Detail);
}


void SubjectT::UpdateAllObservers_Modified(const cf::TypeSys::VarBaseT& Var)
{
    for (unsigned long ObsNr = 0; ObsNr < m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->Notify_Changed(this, Var);
}


SubjectT::~SubjectT()
{
    for (unsigned long ObsNr=0; ObsNr<m_Observers.Size(); ObsNr++)
        m_Observers[ObsNr]->NotifySubjectDies(this);
}
