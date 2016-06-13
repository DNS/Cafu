/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_OBSERVER_PATTERN_HPP_INCLUDED
#define CAFU_GUIEDITOR_OBSERVER_PATTERN_HPP_INCLUDED

/// \file
/// This file provides the classes for the Observer pattern as described in the book by the GoF.
/// Note that implementations of ObserverT normally maintain a pointer to the subject(s) that they observe,
/// e.g. in order to be able to redraw themselves also outside of and independent from the NotifySubjectChanged() method.
/// This however in turn creates problems when the life of the observer begins before or ends after the life of the observed subject(s).
/// In fact, it seems that the observers have to maintain lists of valid, observed subjects as the subjects maintain lists of observers.
/// Fortunately, these possibly tough problems can (and apparently must) be addressed by the concrete implementation classes of
/// observers and subjects, not by the base classes that the Observer pattern describes and provided herein.

#include "Templates/Array.hpp"
#include "Templates/Pointer.hpp"


namespace cf { namespace GuiSys { class WindowT; } }
namespace cf { namespace TypeSys { class VarBaseT; } }


namespace GuiEditor
{
    class SubjectT;

    enum WindowModDetailE
    {
        WMD_GENERIC,    ///< Generic change of windows (useful if the subject doesn't know what exactly has been changed).
        WMD_HIERARCHY   ///< The position of a window in the window hierarchy has been changed.
    };


    class ObserverT
    {
        public:

        /// This method is called whenever the window selection of a GUI subject changed.
        /// @param Subject The GUI document in which the selection has been changed.
        /// @param OldSelection Array of the previously selected windows.
        /// @param NewSelection Array of the new selected windows.
        virtual void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& OldSelection, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& NewSelection) { }

        /// Notifies the observer that one or more windows have been created.
        /// @param Subject The GUI document in which the windows have been created.
        /// @param Windows List of created windows.
        virtual void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows) { }

        /// Notifies the observer that one or more windows have been deleted.
        /// @param Subject The GUI document in which the windows have been deleted.
        /// @param Windows List of deleted windows.
        virtual void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows) { }

        /// Notifies the observer that a general  GUI property has been modified.
        /// @param Subject The GUI document whose GUI property has been modified.
        virtual void NotifySubjectChanged_GuiPropertyModified(SubjectT* Subject) { }

        /// @name Modification notification method and overloads.
        /// These methods notify the observer that one or more GUI windows have been modified.
        /// The first 3 parameters are common to all of these methods since they are the basic information needed to communicate
        /// an object modification.
        //@{

        /// @param Subject   The GUI document in which the elements have been modified.
        /// @param Windows   List of modified windows.
        /// @param Detail    Information about what has been modified:
        ///                  Can be WMD_GENERIC or WMD_HIERARCHY.
        virtual void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows, WindowModDetailE Detail) { }
        //@}

        /// Notifies the observer that a variable has changed.
        /// @param Subject   The GUI document in which a variable has changed.
        /// @param Var       The variable whose value has changed.
        virtual void Notify_Changed(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var) { }

        /// This method is called whenever a subject is about the be destroyed (and become unavailable).
        /// @param dyingSubject   The subject that is being destroyed.
        virtual void NotifySubjectDies(SubjectT* dyingSubject)=0;

        /// The virtual destructor.
        virtual ~ObserverT();


        protected:

        /// The constructor. It is protected so that only derived classes can create instances of this class.
        ObserverT();
    };


    class SubjectT
    {
        public:

        /// Registers the observer Obs for notification on updates of this class.
        /// \param Obs   The observer that is to be registered.
        void RegisterObserver(ObserverT* Obs);

        /// Unregisters the observer Obs from further notification on updates of this class.
        /// \param Obs   The observer that is to be unregistered.
        void UnregisterObserver(ObserverT* Obs);

        void UpdateAllObservers_SelectionChanged(const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& OldSelection, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& NewSelection);
        void UpdateAllObservers_Created(const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows);
        void UpdateAllObservers_Created(IntrusivePtrT<cf::GuiSys::WindowT> Window);
        void UpdateAllObservers_Deleted(const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows);
        void UpdateAllObservers_Deleted(IntrusivePtrT<cf::GuiSys::WindowT> Window);
        void UpdateAllObservers_GuiPropertyModified();
        void UpdateAllObservers_Modified(const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows, WindowModDetailE Detail);
        void UpdateAllObservers_Modified(IntrusivePtrT<cf::GuiSys::WindowT> Window, WindowModDetailE Detail);
        void UpdateAllObservers_Modified(const cf::TypeSys::VarBaseT& Var);

        /// The virtual destructor.
        virtual ~SubjectT();


        protected:

        /// The constructor. It is protected so that only derived classes can create instances of this class.
        SubjectT();


        private:

        /// The list of the observers that have registered for notification on updates of this class.
        ArrayT<ObserverT*> m_Observers;
    };
}

#endif
