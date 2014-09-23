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

#include "DialogInspector.hpp"
#include "DialogInsp-PrimitiveProps.hpp"
#include "DialogEntityInspector.hpp"
#include "MapDocument.hpp"
#include "MapEntRepres.hpp"

#include "wx/notebook.h"


using namespace MapEditor;


InspectorDialogT::InspectorDialogT(wxWindow* Parent, MapDocumentT* MapDoc)
    : wxPanel(Parent, -1),
      m_MapDoc(MapDoc),
      Notebook(NULL),
      m_EntityInspectorDialog(NULL)
{
    wxSizer* mainSizer=new wxBoxSizer(wxVERTICAL);

    Notebook                = new wxNotebook(this, -1, wxDefaultPosition, wxSize(350, 450));
    m_EntityInspectorDialog = new EntityInspectorDialogT(Notebook, MapDoc->GetChildFrame(), wxSize(300, 200));
    PrimitiveProps          = new InspDlgPrimitivePropsT(Notebook, MapDoc);

    Notebook->AddPage(m_EntityInspectorDialog, "Entity");
    Notebook->AddPage(PrimitiveProps,          "Primitive");

    mainSizer->Add(Notebook, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 2);

    this->SetSizer(mainSizer);
    mainSizer->SetSizeHints(this);

    m_MapDoc->RegisterObserver(this);
}


InspectorDialogT::~InspectorDialogT()
{
    if (m_MapDoc) m_MapDoc->UnregisterObserver(this);
}


void InspectorDialogT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection)
{
    // If the new selection is empty, don't change the notebook page.
    if (NewSelection.Size() == 0) return;

    bool HaveEntities   = false;
    bool HavePrimitives = false;

    for (unsigned long SelNr = 0; SelNr < NewSelection.Size(); SelNr++)
    {
        if (NewSelection[SelNr]->GetType() == &MapEntRepresT::TypeInfo)
        {
            HaveEntities = true;
        }
        else
        {
            HavePrimitives = true;
        }
    }

    // If the old selection was empty and the new selection has entities, then show the
    // Entity Inspector (irrespective of whether the new selection has primitives as well).
    if (OldSelection.Size() == 0 && HaveEntities)
    {
        Notebook->ChangeSelection(0);
        return;
    }

    // If the new selection has both entities and primitives, don't change the notebook page.
    if (HaveEntities && HavePrimitives) return;

    // Show the notebook page that reflects the type of the selected elements.
    Notebook->ChangeSelection(HaveEntities ? 0 : 1);
}


void InspectorDialogT::NotifySubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject == m_MapDoc);

    m_MapDoc = NULL;
}
