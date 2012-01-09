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

#include "DialogInsp-EntityTree.hpp"
#include "EntityClass.hpp"
#include "EntityClassVar.hpp"
#include "GameConfig.hpp"
#include "CommandHistory.hpp"
#include "MapDocument.hpp"
#include "MapEntityBase.hpp"
#include "ChildFrame.hpp"

#include "MapCommands/Select.hpp"

#include "wx/wx.h"
#include "wx/confbase.h"
#include "wx/statline.h"
#include "wx/notebook.h"


BEGIN_EVENT_TABLE(InspDlgEntityTreeT, wxPanel)
    EVT_CHECKBOX(InspDlgEntityTreeT::ID_CHECKBOX_ShowSolidEntities,  InspDlgEntityTreeT::OnCheckbox_ShowSolidEntities)
    EVT_CHECKBOX(InspDlgEntityTreeT::ID_CHECKBOX_ShowPointEntities,  InspDlgEntityTreeT::OnCheckbox_ShowPointEntities)
    EVT_CHECKBOX(InspDlgEntityTreeT::ID_CHECKBOX_ShowHiddenEntities, InspDlgEntityTreeT::OnCheckbox_ShowHiddenEntities)
    EVT_CHECKBOX(InspDlgEntityTreeT::ID_CHECKBOX_FilterByProp,       InspDlgEntityTreeT::OnCheckbox_FilterByProp)
    EVT_CHECKBOX(InspDlgEntityTreeT::ID_CHECKBOX_FilterByPropExact,  InspDlgEntityTreeT::OnCheckbox_FilterByPropExact)
    EVT_CHECKBOX(InspDlgEntityTreeT::ID_CHECKBOX_FilterByClass,      InspDlgEntityTreeT::OnCheckbox_FilterByClass)
    EVT_TEXT    (InspDlgEntityTreeT::ID_TEXT_FilterKey,              InspDlgEntityTreeT::OnText_FilterKeyChanged)
    EVT_TEXT    (InspDlgEntityTreeT::ID_TEXT_FilterValue,            InspDlgEntityTreeT::OnText_FilterValueChanged)
    EVT_COMBOBOX(InspDlgEntityTreeT::ID_COMBOBOX_FilterClass,        InspDlgEntityTreeT::OnComboBox_FilterClass_SelectionChanged)
    EVT_TEXT    (InspDlgEntityTreeT::ID_COMBOBOX_FilterClass,        InspDlgEntityTreeT::OnComboBox_FilterClass_TextChanged)
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

    wxBoxSizer *item7 = new wxBoxSizer( wxHORIZONTAL );

    ShowSolidEntitiesCheckBox = new wxCheckBox( parent, ID_CHECKBOX_ShowSolidEntities, wxT("Solid Entities"), wxDefaultPosition, wxDefaultSize, 0 );
    item7->Add( ShowSolidEntitiesCheckBox, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    ShowPointEntitiesCheckBox = new wxCheckBox( parent, ID_CHECKBOX_ShowPointEntities, wxT("Point Entities"), wxDefaultPosition, wxDefaultSize, 0 );
    item7->Add( ShowPointEntitiesCheckBox, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    item5->Add( item7, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer *item10 = new wxBoxSizer( wxHORIZONTAL );

    ShowHiddenEntitiesCheckBox = new wxCheckBox( parent, ID_CHECKBOX_ShowHiddenEntities, wxT("Include hidden Entities"), wxDefaultPosition, wxDefaultSize, 0 );
    item10->Add( ShowHiddenEntitiesCheckBox, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    item5->Add( item10, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxStaticLine *item12 = new wxStaticLine( parent, -1, wxDefaultPosition, wxSize(20,-1), wxLI_HORIZONTAL );
    item5->Add( item12, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxBoxSizer *item13 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer *item14 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer *item15 = new wxBoxSizer( wxHORIZONTAL );

    FilterByPropCheckBox = new wxCheckBox( parent, ID_CHECKBOX_FilterByProp, wxT("By Key/Value"), wxDefaultPosition, wxDefaultSize, 0 );
    item15->Add( FilterByPropCheckBox, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxTOP, 5 );

    item15->Add( 20, 10, 1, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxTOP, 5 );

    FilterByPropExactCheckBox = new wxCheckBox( parent, ID_CHECKBOX_FilterByPropExact, wxT("Exact"), wxDefaultPosition, wxDefaultSize, 0 );
    item15->Add( FilterByPropExactCheckBox, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxTOP, 5 );

    item14->Add( item15, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer *item18 = new wxBoxSizer( wxHORIZONTAL );

    // Warning: Setting a non-empty default text here does cause a text update event,
    // and thus an early call to the event handler that in turn will cause a crash, as we are still in mid-constructor!
    FilterKeyText = new wxTextCtrl( parent, ID_TEXT_FilterKey, wxT(""), wxDefaultPosition, wxSize(80,-1), 0 );
    item18->Add( FilterKeyText, 1, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    wxStaticText *item20 = new wxStaticText( parent, -1, wxT("="), wxDefaultPosition, wxDefaultSize, 0 );
    item18->Add( item20, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    // Warning: Setting a non-empty default text here does cause a text update event,
    // and thus an early call to the event handler that in turn will cause a crash, as we are still in mid-constructor!
    FilterValueText = new wxTextCtrl( parent, ID_TEXT_FilterValue, wxT(""), wxDefaultPosition, wxSize(80,-1), 0 );
    item18->Add( FilterValueText, 1, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    item14->Add( item18, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer *item22 = new wxBoxSizer( wxHORIZONTAL );

    FilterByClassCheckBox = new wxCheckBox( parent, ID_CHECKBOX_FilterByClass, wxT("By Entity Class"), wxDefaultPosition, wxDefaultSize, 0 );
    item22->Add( FilterByClassCheckBox, 1, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxTOP, 5 );

    item14->Add( item22, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer *item24 = new wxBoxSizer( wxHORIZONTAL );

    FilterClassComboBox = new wxComboBox( parent, ID_COMBOBOX_FilterClass, "", wxDefaultPosition, wxSize(100,-1), 0, NULL, wxCB_DROPDOWN | wxCB_SORT);
    item24->Add( FilterClassComboBox, 1, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    item14->Add( item24, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    item13->Add( item14, 1, wxALIGN_CENTER_VERTICAL, 5 );

    item5->Add( item13, 0, wxALIGN_CENTER_VERTICAL, 5 );

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
      ShowSolidEntitiesCheckBox(NULL),
      ShowPointEntitiesCheckBox(NULL),
      ShowHiddenEntitiesCheckBox(NULL),
      FilterByPropCheckBox(NULL),
      FilterByPropExactCheckBox(NULL),
      FilterByClassCheckBox(NULL),
      FilterKeyText(NULL),
      FilterValueText(NULL),
      FilterClassComboBox(NULL)
{
    EntityReportInit(this, true, true);

    const ArrayT<const EntityClassT*>& Classes=MapDoc->GetGameConfig()->GetEntityClasses();

    for (unsigned long ClassNr=0; ClassNr<Classes.Size(); ClassNr++)
        if (Classes[ClassNr]->GetName()!="worldspawn")
            FilterClassComboBox->Append(Classes[ClassNr]->GetName());

    ShowSolidEntitiesCheckBox ->SetValue(wxConfigBase::Get()->Read("Entity Report Dialog/Show Brush Entities" , 1l)!=0);
    ShowPointEntitiesCheckBox ->SetValue(wxConfigBase::Get()->Read("Entity Report Dialog/Show Point Entities" , 1l)!=0);
    ShowHiddenEntitiesCheckBox->SetValue(wxConfigBase::Get()->Read("Entity Report Dialog/Show Hidden Entities", 1l)!=0);
    FilterByPropCheckBox      ->SetValue(wxConfigBase::Get()->Read("Entity Report Dialog/Filter By Prop"      , 0l)!=0);
    FilterByPropExactCheckBox ->SetValue(wxConfigBase::Get()->Read("Entity Report Dialog/Filter By Prop Exact", 0l)!=0);
    FilterByClassCheckBox     ->SetValue(wxConfigBase::Get()->Read("Entity Report Dialog/Filter By Class"     , 0l)!=0);
    FilterKeyText             ->SetValue(wxConfigBase::Get()->Read("Entity Report Dialog/Filter Key"          , ""));
    FilterValueText           ->SetValue(wxConfigBase::Get()->Read("Entity Report Dialog/Filter Value"        , ""));
    FilterClassComboBox       ->SetValue(wxConfigBase::Get()->Read("Entity Report Dialog/Filter Class"        , ""));

    // These call UpdateEntityListBox() automatically.
    wxCommandEvent CE1; OnCheckbox_FilterByProp(CE1);
    wxCommandEvent CE2; OnCheckbox_FilterByClass(CE2);

    MapDoc->RegisterObserver(this);
}


InspDlgEntityTreeT::~InspDlgEntityTreeT()
{
    wxConfigBase::Get()->Write("Entity Report Dialog/Show Brush Entities",  ShowSolidEntitiesCheckBox ->GetValue());
    wxConfigBase::Get()->Write("Entity Report Dialog/Show Point Entities",  ShowPointEntitiesCheckBox ->GetValue());
    wxConfigBase::Get()->Write("Entity Report Dialog/Show Hidden Entities", ShowHiddenEntitiesCheckBox->GetValue());
    wxConfigBase::Get()->Write("Entity Report Dialog/Filter By Prop",       FilterByPropCheckBox      ->GetValue());
    wxConfigBase::Get()->Write("Entity Report Dialog/Filter By Prop Exact", FilterByPropExactCheckBox ->GetValue());
    wxConfigBase::Get()->Write("Entity Report Dialog/Filter By Class",      FilterByClassCheckBox     ->GetValue());
    wxConfigBase::Get()->Write("Entity Report Dialog/Filter Key",           FilterKeyText             ->GetValue());
    wxConfigBase::Get()->Write("Entity Report Dialog/Filter Value",         FilterValueText           ->GetValue());
    wxConfigBase::Get()->Write("Entity Report Dialog/Filter Class",         FilterClassComboBox       ->GetValue());

    if (MapDoc) MapDoc->UnregisterObserver(this);
}


void InspDlgEntityTreeT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection)
{
    if (IsRecursiveSelfNotify) return;

    UpdateEntityListBox();
}


void InspDlgEntityTreeT::NotifySubjectChanged_Created(const ArrayT<MapElementT*>& MapElements)
{
    UpdateEntityListBox();
}


void InspDlgEntityTreeT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements)
{
    UpdateEntityListBox();
}


void InspDlgEntityTreeT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const wxString& Key)
{
    UpdateEntityListBox();
}


void InspDlgEntityTreeT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail)
{
    if (Detail!=MEMD_ENTITY_CLASS_CHANGED) return;

    if (IsRecursiveSelfNotify) return;

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
        MapEntityBaseT* NewEntity=MapDoc->GetEntities()[ChildNr];

        if (!ShowSolidEntitiesCheckBox ->IsChecked() &&  NewEntity->GetClass()->IsSolidClass()) continue;
        if (!ShowPointEntitiesCheckBox ->IsChecked() && !NewEntity->GetClass()->IsSolidClass()) continue;
        if (!ShowHiddenEntitiesCheckBox->IsChecked() && !NewEntity->IsVisible()) continue;

        if (FilterByPropCheckBox->IsChecked())
        {
            unsigned long PropNr;

            for (PropNr=0; PropNr<NewEntity->GetProperties().Size(); PropNr++)
            {
                // First see if the key matches (case-independent, but exact otherwise).
                if (wxStricmp(FilterKeyText->GetValue(), NewEntity->GetProperties()[PropNr].Key)) continue;

                // Now see if the value matches (always case-independent, rest depends on the "exact" checkbox).
                if (FilterByPropExactCheckBox->IsChecked())
                {
                    if (wxStricmp(FilterValueText->GetValue(), NewEntity->GetProperties()[PropNr].Value)) continue;
                }
                else
                {
                    if (NewEntity->GetProperties()[PropNr].Value.Upper().find(FilterValueText->GetValue().Upper())==wxString::npos) continue;
                }

                // Managing to get here means that this entity is a match.
                break;
            }

            if (PropNr>=NewEntity->GetProperties().Size()) continue;
        }

        if (FilterByClassCheckBox->IsChecked())
            if (NewEntity->GetClass()->GetName().Upper().find(FilterClassComboBox->GetValue().Upper())==wxString::npos) continue;

        wxString NewEntityDescription=NewEntity->GetClass()->GetName();

        if (NewEntity->FindProperty("name")!=NULL) NewEntityDescription+=" ("+NewEntity->FindProperty("name")->Value+")";

        const int NewID=EntityListBox->Append(NewEntityDescription, NewEntity);

        // Select the selected entities also in the ListBox!
        if (NewEntity->IsSelected()) EntityListBox->Select(NewID);
    }
}


void InspDlgEntityTreeT::OnCheckbox_ShowSolidEntities(wxCommandEvent& Event)
{
    UpdateEntityListBox();
}


void InspDlgEntityTreeT::OnCheckbox_ShowPointEntities(wxCommandEvent& Event)
{
    UpdateEntityListBox();
}


void InspDlgEntityTreeT::OnCheckbox_ShowHiddenEntities(wxCommandEvent& Event)
{
    UpdateEntityListBox();
}


void InspDlgEntityTreeT::OnCheckbox_FilterByProp(wxCommandEvent& Event)
{
    FilterByPropExactCheckBox->Enable(FilterByPropCheckBox->IsChecked());
    FilterKeyText            ->Enable(FilterByPropCheckBox->IsChecked());
    FilterValueText          ->Enable(FilterByPropCheckBox->IsChecked());

    UpdateEntityListBox();
}


void InspDlgEntityTreeT::OnCheckbox_FilterByPropExact(wxCommandEvent& Event)
{
    UpdateEntityListBox();
}


void InspDlgEntityTreeT::OnCheckbox_FilterByClass(wxCommandEvent& Event)
{
    FilterClassComboBox->Enable(FilterByClassCheckBox->IsChecked());

    UpdateEntityListBox();
}


void InspDlgEntityTreeT::OnText_FilterKeyChanged(wxCommandEvent& Event)
{
    UpdateEntityListBox();
}


void InspDlgEntityTreeT::OnText_FilterValueChanged(wxCommandEvent& Event)
{
    UpdateEntityListBox();
}


void InspDlgEntityTreeT::OnComboBox_FilterClass_SelectionChanged(wxCommandEvent& Event)
{
    UpdateEntityListBox();
}


void InspDlgEntityTreeT::OnComboBox_FilterClass_TextChanged(wxCommandEvent& Event)
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
        EntitySelection.PushBack((MapEntityBaseT*)EntityListBox->GetClientData(SelEntries[SelNr]));

    MapDoc->GetHistory().SubmitCommand(CommandSelectT::Set(MapDoc, EntitySelection));

    IsRecursiveSelfNotify=false;
}
