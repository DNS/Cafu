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

#include "Group.hpp"
#include "MapDocument.hpp"
#include "MapEntityBase.hpp"
#include "MapPrimitive.hpp"
#include "TextParser/TextParser.hpp"


/**
 * Groups can become abandoned and thus empty whenever
 *     - map elements are regrouped/reassigned into another group
 *     - map elements are deleted, either explicitly (delete selection) or implicitly
 *       (CommandDeleteT occurs as a subcommand). Implicit deletes occur with these commands:
 *           - Carve (delete brushes that are completely carved  away).
 *           - Clip  (delete brushes that are completely clipped away).
 *           - AssignPrimToEntity (parent entity becomes empty).
 *
 * Therefore, we implement the purging of groups as an explicit command following the Delete, Carve,
 * Clip and MakeHollow commands whereever they are instantiated and submitted.
 * We use explicit commands for deleting the groups (instead of putting everything into a macro command)
 * so that the user has the option to undo the purge (separately from the deletion) if he wishes.
 */


GroupT::GroupT(const wxString& Name_)
    : Name(Name_),
      Color(wxColour(80+(rand() % 176), 80+(rand() % 176), 80+(rand() % 176))),
      IsVisible(true),
      CanSelect(true),
      SelectAsGroup(false)
{
}


/*static*/ GroupT GroupT::Create_cmap(TextParserT& TP)
{
    GroupT NewGroup("new group");

    TP.AssertAndSkipToken("GroupDef");
    /*const int GroupNr=*/TP.GetNextTokenAsInt();

    // wxASSERT(GroupNr==GetDoc().GetGroups().Size());

    NewGroup.Name         =TP.GetNextToken();
    NewGroup.Color        =wxColour(TP.GetNextToken());
    NewGroup.IsVisible    =TP.GetNextTokenAsInt()!=0;
    NewGroup.CanSelect    =TP.GetNextTokenAsInt()!=0;
    NewGroup.SelectAsGroup=TP.GetNextTokenAsInt()!=0;

    return NewGroup;
}


void GroupT::Save_cmap(std::ostream& OutFile, unsigned long GroupNr) const
{
    wxString SaveName=Name;

    SaveName.Replace("\"", "'");

    OutFile << "GroupDef " << GroupNr << " \""
            << SaveName << "\" \""
            << Color.GetAsString() << "\" "
            << int(IsVisible) << " "
            << int(CanSelect) << " "
            << int(SelectAsGroup) << "\n";
}


ArrayT<MapElementT*> GroupT::GetMembers(const MapDocumentT& MapDoc) const
{
    ArrayT<MapElementT*> Members;

    for (unsigned long EntNr=0; EntNr<MapDoc.GetEntities().Size(); EntNr++)
    {
        MapEntityBaseT*               Ent       =MapDoc.GetEntities()[EntNr];
        const ArrayT<MapPrimitiveT*>& Primitives=Ent->GetPrimitives();

        if (Ent->GetGroup()==this) Members.PushBack(Ent);

        for (unsigned long PrimNr=0; PrimNr<Primitives.Size(); PrimNr++)
            if (Primitives[PrimNr]->GetGroup()==this) Members.PushBack(Primitives[PrimNr]);
    }

    return Members;
}


bool GroupT::HasMembers(const MapDocumentT& MapDoc) const
{
    for (unsigned long EntNr=0; EntNr<MapDoc.GetEntities().Size(); EntNr++)
    {
        MapEntityBaseT*               Ent       =MapDoc.GetEntities()[EntNr];
        const ArrayT<MapPrimitiveT*>& Primitives=Ent->GetPrimitives();

        if (Ent->GetGroup()==this) return true;

        for (unsigned long PrimNr=0; PrimNr<Primitives.Size(); PrimNr++)
            if (Primitives[PrimNr]->GetGroup()==this) return true;
    }

    return false;
}
