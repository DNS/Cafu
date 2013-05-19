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

#ifndef CAFU_COMMAND_NEW_ENTITY_HPP_INCLUDED
#define CAFU_COMMAND_NEW_ENTITY_HPP_INCLUDED

#include "../CommandPattern.hpp"


class CommandSelectT;
class MapDocumentT;
class MapEntityBaseT;


/// This commands inserts a new entity into the map.
class CommandNewEntityT : public CommandT
{
    public:

    /// The constructor.
    /// @param MapDoc   Map document into which the entity is inserted.
    /// @param Entity   The entity to insert.
    CommandNewEntityT(MapDocumentT& MapDoc, MapEntityBaseT* Entity);

    /// The constructor.
    /// @param MapDoc     Map document into which the entities are inserted.
    /// @param Entities   The entities to insert.
    CommandNewEntityT(MapDocumentT& MapDoc, const ArrayT<MapEntityBaseT*>& Entities);

    /// The destructor.
    ~CommandNewEntityT();

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    MapDocumentT&           m_MapDoc;
    ArrayT<MapEntityBaseT*> m_Entities;
    CommandSelectT*         m_CommandSelect;    ///< Subcommand for changing the selection.
};

#endif
