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

#ifndef CAFU_MAPEDITOR_DIALOG_ENTITY_INSPECTOR_HPP_INCLUDED
#define CAFU_MAPEDITOR_DIALOG_ENTITY_INSPECTOR_HPP_INCLUDED

#include "ObserverPattern.hpp"

#include "wx/wx.h"
#include "wx/propgrid/manager.h"


namespace cf { namespace GameSys { class ComponentBaseT; } }
namespace cf { namespace GameSys { class EntityT; } }
namespace cf { namespace TypeSys { class VarBaseT; } }
class ChildFrameT;
class MapDocumentT;


namespace MapEditor
{
    class EntityInspectorDialogT : public wxPropertyGridManager, public ObserverT
    {
        public:

        EntityInspectorDialogT(wxWindow* Parent, ChildFrameT* ChildFrame, const wxSize& Size);
        ~EntityInspectorDialogT();

        // ObserverT implementation.
        void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection) override;
        void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities) override;
        void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail) override;
        void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds) override;
        void Notify_EntChanged(SubjectT* Subject, const ArrayT< IntrusivePtrT<MapEditor::CompMapEntityT> >& Entities, EntityModDetailE Detail) override;
        void Notify_VarChanged(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var) override;
        void NotifySubjectDies(SubjectT* dyingSubject) override;


        private:

        void RefreshPropGrid();
        void AppendComponent(IntrusivePtrT<cf::GameSys::ComponentBaseT> Comp);

        void OnPropertyGridChanging(wxPropertyGridEvent& Event);
        void OnPropertyGridChanged(wxPropertyGridEvent& Event);
        void OnPropertyGridRightClick(wxPropertyGridEvent& Event);

        MapDocumentT*                       m_MapDocument;
        ChildFrameT*                        m_ChildFrame;
        IntrusivePtrT<cf::GameSys::EntityT> m_SelectedEntity;
        bool                                m_IsRecursiveSelfNotify;

        DECLARE_EVENT_TABLE()
    };
}

#endif
