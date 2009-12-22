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

#ifndef _GUIEDITOR_GUI_INSPECTOR_HPP_
#define _GUIEDITOR_GUI_INSPECTOR_HPP_

#include "ObserverPattern.hpp"

#include "wx/wx.h"
#include "wx/propgrid/propgrid.h"
#include "wx/propgrid/manager.h"
#include "wx/propgrid/advprops.h"


namespace GuiEditor
{
    class ChildFrameT;
    class GuiDocumentT;

    class GuiInspectorT : public wxPropertyGridManager, public ObserverT
    {
        public:

        GuiInspectorT(ChildFrameT* Parent, const wxSize& Size);

        // ObserverT implementation.
        void NotifySubjectChanged_GuiPropertyModified(SubjectT* Subject);
        void NotifySubjectDies(SubjectT* dyingSubject);

        void RefreshPropGrid();


        private:

        GuiDocumentT*        m_GuiDocument;
        ChildFrameT*         m_Parent;
        bool                 m_IsRecursiveSelfUpdate;

        void OnPropertyGridChanged(wxPropertyGridEvent& Event);

        DECLARE_EVENT_TABLE()
    };
}

#endif
