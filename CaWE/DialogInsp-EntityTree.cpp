/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#include "DialogInsp-EntityTree.hpp"
#include "CompMapEntity.hpp"
#include "EntityClass.hpp"
#include "EntityClassVar.hpp"
#include "GameConfig.hpp"
#include "CommandHistory.hpp"
#include "MapDocument.hpp"
#include "MapEntRepres.hpp"
#include "ChildFrame.hpp"

#include "MapCommands/Select.hpp"

#include "wx/wx.h"
#include "wx/confbase.h"
#include "wx/statline.h"
#include "wx/notebook.h"


using namespace MapEditor;


BEGIN_EVENT_TABLE(InspDlgEntityTreeT, wxPanel)
    EVT_CHECKBOX(InspDlgEntityTreeT::ID_CHECKBOX_ShowHiddenEntities, InspDlgEntityTreeT::OnCheckbox_ShowHiddenEntities)
    EVT_LISTBOX (InspDlgEntityTreeT::ID_LISTBOX_Entities,            InspDlgEntityTreeT::OnListBox_SelectionChanged)
END_EVENT_TABLE()


wxSizer* InspDlgEntityTreeT::EntityReportInit( wxWindow *parent, bool call_fit, bool set_sizer )
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    EntityListBox = new wxListBox( parent, ID_LISTBOX_Entities, wxDefaultPosition, wxSize(80,400), 0, NULL, wxLB_EXTENDED | wxLB_HSCROLL | wxLB_SORT );
    item0->Add( EntityListBox, 1, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer *item2 = new wxBoxSizer( wxHORIZONTAL );

    wxButton *item3 = new wxButton( parent, ChildFrameT::ID_MENU_VIEW_CENTER_2D_VIEWS, wxT("Go to"), wxDefaultPosition, wxDefaultSize, 0 );
    item2->Add( item3, 0, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item4 = new wxButton( parent, ChildFrameT::ID_MENU_EDIT_DELETE, wxT("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
    item2->Add( item4, 0, wxALIGN_CENTER|wxALL, 5 );

    item0->Add( item2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxStaticBox *item6 = new wxStaticBox( parent, -1, wxT("Filter") );
    wxStaticBoxSizer *item5 = new wxStaticBoxSizer( item6, wxVERTICAL );

    wxBoxSizer *item10 = new wxBoxSizer( wxHORIZONTAL );

    ShowHiddenEntitiesCheckBox = new wxCheckBox( parent, ID_CHECKBOX_ShowHiddenEntities, wxT("Include hidden Entities"), wxDefaultPosition, wxDefaultSize, 0 );
    item10->Add( ShowHiddenEntitiesCheckBox, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    item5->Add( item10, 0, wxALIGN_CENTER_VERTICAL, 5 );

    item0->Add( item5, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    if (set_sizer)
    {
        parent->SetSizer( item0 );
        if (call_fit)
            item0->SetSizeHints( parent );
    }

    return item0;
}


InspDlgEntityTreeT::InspDlgEntityTreeT(wxNotebook* Parent_, MapDocumentT* MapDoc_)
    : wxPanel(Parent_, -1),
      MapDoc(MapDoc_),
      IsRecursiveSelfNotify(false),
      EntityListBox(NULL),
      ShowHiddenEntitiesCheckBox(NULL)
{
    EntityReportInit(this, true, true);

    ShowHiddenEntitiesCheckBox->SetValue(wxConfigBase::Get()->Read("Entity Report Dialog/Show Hidden Entities", 1l)!=0);

    MapDoc->RegisterObserver(this);
}


InspDlgEntityTreeT::~InspDlgEntityTreeT()
{
    wxConfigBase::Get()->Write("Entity Report Dialog/Show Hidden Entities", ShowHiddenEntitiesCheckBox->GetValue());

    if (MapDoc) MapDoc->UnregisterObserver(this);
}


void InspDlgEntityTreeT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection)
{
    if (IsRecursiveSelfNotify) return;

    UpdateEntityListBox();
}


void InspDlgEntityTreeT::NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities)
{
    UpdateEntityListBox();
}


void InspDlgEntityTreeT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities)
{
    UpdateEntityListBox();
}


void InspDlgEntityTreeT::Notify_EntChanged(SubjectT* Subject, const ArrayT< IntrusivePtrT<CompMapEntityT> >& Entities, EntityModDetailE Detail)
{
    UpdateEntityListBox();
}


void InspDlgEntityTreeT::NotifySubjectDies(SubjectT* dyingSubject)
{
    assert(dyingSubject==MapDoc);
    MapDoc=NULL;
}


void InspDlgEntityTreeT::UpdateEntityListBox()
{
    EntityListBox->Clear();

    for (unsigned long ChildNr=0/*with world*/; ChildNr<MapDoc->GetEntities().Size(); ChildNr++)
    {
        IntrusivePtrT<CompMapEntityT> NewEntity = MapDoc->GetEntities()[ChildNr];

        if (!ShowHiddenEntitiesCheckBox->IsChecked() && !NewEntity->GetRepres()->IsVisible()) continue;

        wxString NewEntityDescription = NewEntity->GetEntity()->GetBasics()->GetEntityName();

        const int NewID = EntityListBox->Append(NewEntityDescription, NewEntity.get());

        // Select the selected entities also in the ListBox!
        if (NewEntity->GetRepres()->IsSelected()) EntityListBox->Select(NewID);
    }
}


void InspDlgEntityTreeT::OnCheckbox_ShowHiddenEntities(wxCommandEvent& Event)
{
    UpdateEntityListBox();
}


void InspDlgEntityTreeT::OnListBox_SelectionChanged(wxCommandEvent& Event)
{
    IsRecursiveSelfNotify=true;

    // Get selected entries and number of selections from listbox.
    wxArrayInt SelEntries;
    int        NrOfSelections=EntityListBox->GetSelections(SelEntries);

    ArrayT<MapElementT*> EntitySelection;

    for (int SelNr=0; SelNr<NrOfSelections; SelNr++)
        EntitySelection.PushBack(((CompMapEntityT*)EntityListBox->GetClientData(SelEntries[SelNr]))->GetRepres());

    MapDoc->GetHistory().SubmitCommand(CommandSelectT::Set(MapDoc, EntitySelection));

    IsRecursiveSelfNotify=false;
}
