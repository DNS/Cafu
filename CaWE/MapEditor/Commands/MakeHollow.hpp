/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_MAKE_HOLLOW_HPP_INCLUDED
#define CAFU_COMMAND_MAKE_HOLLOW_HPP_INCLUDED

#include "../../CommandPattern.hpp"


class CommandDeleteT;
class CommandSelectT;
class GroupT;
class MapBrushT;
class MapDocumentT;
class MapElementT;


class CommandMakeHollowT : public CommandT
{
    public:

    /// Constructor to hollow the brushes that are among the map elements in the given list.
    CommandMakeHollowT(MapDocumentT& MapDoc, const float WallWidth, const ArrayT<MapElementT*>& Elems);

    ~CommandMakeHollowT();

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&                m_MapDoc;
    ArrayT<MapBrushT*>           m_Brushes;     ///< The brushes that are to be hollowed by this command.
    ArrayT< ArrayT<MapBrushT*> > m_Hollows;     ///< For each brush, this keeps the resulting hollow (Hohlraum) created by this command. Each hollow in turn is defined by a set of "wall" brushes.
    ArrayT<GroupT*>              m_NewGroups;   ///< One new group for the walls of each hollow, when the original brush was in no group before.
    CommandDeleteT*              m_CmdDelete;   ///< Subcommand to delete the m_Brushes.
    CommandSelectT*              m_CmdSelect;   ///< Subcommand to select the (walls of the) new hollows.
};

#endif
