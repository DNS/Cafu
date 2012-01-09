/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef _MODELEDITOR_JOINT_INSPECTOR_HPP_
#define _MODELEDITOR_JOINT_INSPECTOR_HPP_

#include "ObserverPattern.hpp"
#include "wx/wx.h"
#include "wx/propgrid/manager.h"


namespace ModelEditor
{
    class ChildFrameT;
    class ModelDocumentT;

    class JointInspectorT : public wxPropertyGridManager, public ObserverT
    {
        public:

        JointInspectorT(ChildFrameT* Parent, const wxSize& Size);
        ~JointInspectorT();

        // ObserverT implementation.
        void Notify_SelectionChanged(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel);
        void Notify_JointChanged(SubjectT* Subject, unsigned int JointNr);
        void Notify_SubjectDies(SubjectT* dyingSubject);


        private:

        void RefreshPropGrid();
        void OnPropertyGridChanging(wxPropertyGridEvent& Event);

        DECLARE_EVENT_TABLE()

        ModelDocumentT* m_ModelDoc;
        ChildFrameT*    m_Parent;
        bool            m_IsRecursiveSelfNotify;
    };
}

#endif
