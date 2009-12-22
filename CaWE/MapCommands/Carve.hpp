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

#ifndef _COMMAND_CARVE_HPP_
#define _COMMAND_CARVE_HPP_

#include "../CommandPattern.hpp"


class CommandDeleteT;
class GroupT;
class MapDocumentT;
class MapBrushT;
class MapElementT;
class MapEntityBaseT;


class CommandCarveT : public CommandT
{
    public:

    /// Constructor to carve an array of objects from the world.
    CommandCarveT(MapDocumentT& MapDoc, const ArrayT<const MapBrushT*>& Carvers);

    ~CommandCarveT();

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&                  m_MapDoc;
    const ArrayT<const MapBrushT*> m_Carvers;
    CommandDeleteT*                m_DeleteCommand;     ///< Subcommand to delete the original brushes that are carved.
    ArrayT<GroupT*>                m_NewCarveGroups;    ///< One new group for the carve pieces of each original brush, when the original brush was in no group before and carved into at least two pieces.

    ArrayT<MapElementT*>           m_OriginalBrushes;   ///< The affected brushes before they were carved.
    ArrayT<MapEntityBaseT*>        m_Parents;           ///< Parent entities of the original brushes.
    ArrayT< ArrayT<MapBrushT*> >   m_CarvedBrushes;     ///< For each original brush, the brushes resulting from the carve operation.
};

#endif
