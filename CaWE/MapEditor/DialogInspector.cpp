/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
