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

#ifndef _MODELEDITOR_OBSERVER_PATTERN_HPP_
#define _MODELEDITOR_OBSERVER_PATTERN_HPP_

/// \file
/// This file provides the classes for the Observer pattern as described in the book by the GoF.
/// Note that implementations of ObserverT normally maintain a pointer to the subject(s) that they observe,
/// e.g. in order to be able to redraw themselves also outside of and independent from the NotifySubjectChanged() method.
/// This however in turn creates problems when the life of the observer begins before or ends after the life of the observed subject(s).
/// In fact, it seems that the observers have to maintain lists of valid, observed subjects as the subjects maintain lists of observers.
/// Fortunately, these possibly tough problems can (and apparently must) be addressed by the concrete implementation classes of
/// observers and subjects, not by the base classes that the Observer pattern describes and provided herein.

#include "Templates/Array.hpp"
// #include "wx/string.h"


namespace ModelEditor
{
    class SubjectT;


    class ObserverT
    {
        public:

        // /// This method is called whenever the window selection of a GUI subject changed.
        // /// @param Subject The GUI document in which the selection has been changed.
        // /// @param OldSelection Array of the previously selected windows.
        // /// @param NewSelection Array of the new selected windows.
        // virtual void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<cf::GuiSys::WindowT*>& OldSelection, const ArrayT<cf::GuiSys::WindowT*>& NewSelection) { }

        /// Notifies the observer that a joint has changed.
        /// @param Subject   The model document with the model in which the joint has changed.
        /// @param JointNr   The number of the joint that has changed.
        virtual void Notify_JointChanged(SubjectT* Subject, unsigned int JointNr) { }

        /// This method is called whenever a subject is about the be destroyed (and become unavailable).
        /// \param dyingSubject   The subject that is being destroyed.
        virtual void Notify_SubjectDies(SubjectT* dyingSubject)=0;

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

     // virtual void UpdateAllObservers_SelectionChanged(const ArrayT<cf::GuiSys::WindowT*>& OldSelection, const ArrayT<cf::GuiSys::WindowT*>& NewSelection);
        virtual void UpdateAllObservers_JointChanged(unsigned int JointNr);

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
