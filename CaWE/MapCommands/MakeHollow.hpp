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

#ifndef _COMMAND_MAKE_HOLLOW_HPP_
#define _COMMAND_MAKE_HOLLOW_HPP_

#include "../CommandPattern.hpp"


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
