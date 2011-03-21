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

#include "DialogMapInfo.hpp"
#include "EntityClass.hpp"
#include "MapEntity.hpp"
#include "MapFace.hpp"
#include "MapBrush.hpp"
#include "MapDocument.hpp"
#include "Templates/Array.hpp"
#include "EditorMaterial.hpp"


// TODO:
//   - number of faces
//   - info about used textures
//   - integrate with main properties ("inspector") dialog

MapInfoDialogT::MapInfoDialogT(MapDocumentT& MapDoc)
    : wxDialog(NULL, -1, wxString("World Info"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    std::map<const cf::TypeSys::TypeInfoT*, unsigned long> Stats;

    for (unsigned long EntNr=0; EntNr<MapDoc.GetEntities().Size(); EntNr++)
    {
        const MapEntityBaseT* Ent=MapDoc.GetEntities()[EntNr];

        Stats[Ent->GetType()]++;

        for (unsigned long PrimNr=0; PrimNr<Ent->GetPrimitives().Size(); PrimNr++)
        {
            const MapPrimitiveT* Prim=Ent->GetPrimitives()[PrimNr];

            Stats[Prim->GetType()]++;
        }
    }


    // Assemble the dialog.
    // This code was created by wxDesigner, and brought here by copy'n'paste.
    // Then I've modified it to better meet my needs:
    // a) The parent of the controls is "this".
    // b) The ID of the controls can be -1, we don't need any specific values for this dialog.
    // c) Added own code to set the contents of the controls.

    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer *item1 = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer *item2 = new wxBoxSizer( wxVERTICAL );

    for (std::map<const cf::TypeSys::TypeInfoT*, unsigned long>::const_iterator It=Stats.begin(); It!=Stats.end(); ++It)
    {
        wxStaticText* LeftCell = new wxStaticText(this, -1, It->first->ClassName, wxDefaultPosition, wxDefaultSize, 0 );
        item2->Add( LeftCell, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
    }

    item1->Add( item2, 0, wxALIGN_CENTER|wxALL, 5 );

    wxBoxSizer *item9 = new wxBoxSizer( wxVERTICAL );

    for (std::map<const cf::TypeSys::TypeInfoT*, unsigned long>::const_iterator It=Stats.begin(); It!=Stats.end(); ++It)
    {
        wxStaticText* RightCell = new wxStaticText(this, -1, wxString::Format("%lu", It->second), wxDefaultPosition, wxDefaultSize, 0 );
        item9->Add( RightCell, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
    }

    item1->Add( item9, 0, wxALIGN_CENTER|wxALL, 5 );

    item0->Add( item1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxButton *item18 = new wxButton(this, wxID_CANCEL, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    item18->SetDefault();
    item0->Add( item18, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5 );


    this->SetSizer(item0);
    item0->SetSizeHints(this);
}
