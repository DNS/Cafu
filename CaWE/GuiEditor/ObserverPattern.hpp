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

#ifndef _GUIEDITOR_OBSERVER_PATTERN_HPP_
#define _GUIEDITOR_OBSERVER_PATTERN_HPP_

/// \file This file provides the classes for the Observer pattern as described in the book by the GoF.
/// Note that implementations of ObserverT normally maintain a pointer to the subject(s) that they observe,
/// e.g. in order to be able to redraw themselves also outside of and independent from the NotifySubjectChanged() method.
/// This however in turn creates problems when the life of the observer begins before or ends after the life of the observed subject(s).
/// In fact, it seems that the observers have to maintain lists of valid, observed subjects as the subjects maintain lists of observers.
/// Fortunately, these possibly tough problems can (and apparently must) be addressed by the concrete implementation classes of
/// observers and subjects, not by the base classes that the Observer pattern describes and provided herein.

#include "Templates/Array.hpp"

#include "wx/string.h"


namespace cf { namespace GuiSys { class WindowT; } }


namespace GuiEditor
{
    class SubjectT;
    class UpdateBoxT;

    enum WindowModDetailE
    {
        WMD_GENERIC,          ///< Generic change of windows (useful if the subject doesn't know what exactly has been changed).
        WMD_PROPERTY_CHANGED, ///< A windows property has been changed.
        WMD_TRANSFORMED,      ///< A window has been transformed.
        WMD_HOR_TEXT_ALIGN,   ///< The horizontal text alignment of a window has been changed.
        WMD_HIERARCHY         ///< The position of a window in the window hierarchy has been changed.
    };

    class ObserverT
    {
        public:

        /// This method is called whenever the window selection of a GUI subject changed.
        /// @param Subject The GUI document in which the selection has been changed.
        /// @param OldSelection Array of the previously selected windows.
        /// @param NewSelection Array of the new selected windows.
        virtual void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<cf::GuiSys::WindowT*>& OldSelection, const ArrayT<cf::GuiSys::WindowT*>& NewSelection) { }

        /// Notifies the observer that one or more windows have been created.
        /// @param Subject The GUI document in which the windows have been created.
        /// @param Windows List of created windows.
        virtual void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT<cf::GuiSys::WindowT*>& Windows) { }

        /// Notifies the observer that one or more windows have been deleted.
        /// @param Subject The GUI document in which the windows have been deleted.
        /// @param Windows List of deleted windows.
        virtual void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<cf::GuiSys::WindowT*>& Windows) { }

        /// Notifies the observer that a general  GUI property has been modified.
        /// @param Subject The GUI document whose GUI property has been modified.
        virtual void NotifySubjectChanged_GuiPropertyModified(SubjectT* Subject) { }

        /// @name Modification notification method and overloads.
        /// These methods notify the observer that one or more GUI windows have been modified.
        /// The first 3 parameters are common to all of these methods since they are the basic information needed to communicate
        /// an object modification.
        /// @param Subject The GUI document in which the elements have been modified.
        /// @param Windows List of modified windows.
        /// @param Detail Information about what has been modified. Note that not all WindowModDetailE elements can be used with all
        ///        overloads of this method since some methods use only a part of them (see detailed documentation below).
        //@{

        /// @param Detail Can be WMD_GENERIC, WMD_TRANSFORMED, WMD_HOR_TEXT_ALIGN or WMD_HIERARCHY.
        virtual void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<cf::GuiSys::WindowT*>& Windows, WindowModDetailE Detail) { }

        /// @param Can only be WMD_PROPERTY_CHANGED.
        virtual void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<cf::GuiSys::WindowT*>& Windows, WindowModDetailE Detail, const wxString& PropertyName) { }
        //@}

        /// This method is called whenever a subject is about the be destroyed (and become unavailable).
        /// \param dyingSubject   The subject that is being destroyed.
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
        virtual void RegisterObserver(ObserverT* Obs);

        /// Unregisters the observer Obs from further notification on updates of this class.
        /// \param Obs   The observer that is to be unregistered.
        virtual void UnregisterObserver(ObserverT* Obs);

        virtual void UpdateAllObservers_SelectionChanged(const ArrayT<cf::GuiSys::WindowT*>& OldSelection, const ArrayT<cf::GuiSys::WindowT*>& NewSelection);
        virtual void UpdateAllObservers_Created(const ArrayT<cf::GuiSys::WindowT*>& Windows);
        virtual void UpdateAllObservers_Created(cf::GuiSys::WindowT* Window);
        virtual void UpdateAllObservers_Deleted(const ArrayT<cf::GuiSys::WindowT*>& Windows);
        virtual void UpdateAllObservers_Deleted(cf::GuiSys::WindowT* Window);
        virtual void UpdateAllObservers_GuiPropertyModified();
        virtual void UpdateAllObservers_Modified(const ArrayT<cf::GuiSys::WindowT*>& Windows, WindowModDetailE Detail);
        virtual void UpdateAllObservers_Modified(cf::GuiSys::WindowT* Window, WindowModDetailE Detail);
        virtual void UpdateAllObservers_Modified(const ArrayT<cf::GuiSys::WindowT*>& Windows, WindowModDetailE Detail, const wxString& PropertyName);
        virtual void UpdateAllObservers_Modified(cf::GuiSys::WindowT* Window, WindowModDetailE Detail, const wxString& PropertyName);

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
