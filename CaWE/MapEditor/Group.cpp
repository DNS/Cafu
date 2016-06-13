/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Group.hpp"
#include "MapDocument.hpp"
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


GroupT::GroupT(const MapDocumentT& MapDoc, const wxString& Name_)
    : Name(Name_),
      Color(wxColour(80+(rand() % 176), 80+(rand() % 176), 80+(rand() % 176))),
      IsVisible(true),
      CanSelect(true),
      SelectAsGroup(false),
      m_MapDoc(MapDoc)
{
}


GroupT::GroupT(const MapDocumentT& MapDoc, TextParserT& TP)
    : m_MapDoc(MapDoc)
{
    TP.AssertAndSkipToken("GroupDef");

    /*const int GroupNr = */ TP.GetNextTokenAsInt();
    // wxASSERT(GroupNr == m_MapDoc.GetGroups().Size());

    Name          = TP.GetNextToken();
    Color         = wxColour(TP.GetNextToken());
    IsVisible     = TP.GetNextTokenAsInt() != 0;
    CanSelect     = TP.GetNextTokenAsInt() != 0;
    SelectAsGroup = TP.GetNextTokenAsInt() != 0;
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


ArrayT<MapElementT*> GroupT::GetMembers() const
{
    ArrayT<MapElementT*> Members;
    ArrayT<MapElementT*> Elems;

    m_MapDoc.GetAllElems(Elems);

    for (unsigned int ElemNr = 0; ElemNr < Elems.Size(); ElemNr++)
        if (Elems[ElemNr]->GetGroup() == this) Members.PushBack(Elems[ElemNr]);

    return Members;
}


bool GroupT::HasMembers() const
{
    ArrayT<MapElementT*> Elems;

    m_MapDoc.GetAllElems(Elems);

    for (unsigned int ElemNr = 0; ElemNr < Elems.Size(); ElemNr++)
        if (Elems[ElemNr]->GetGroup() == this) return true;

    return false;
}
