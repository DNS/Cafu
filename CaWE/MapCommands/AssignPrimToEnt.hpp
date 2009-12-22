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

#ifndef _COMMAND_ASSIGN_PRIM_TO_ENTITY_HPP_
#define _COMMAND_ASSIGN_PRIM_TO_ENTITY_HPP_

#include "../CommandPattern.hpp"


class CommandSelectT;
class CommandDeleteT;
class MapDocumentT;
class MapEntityBaseT;
class MapPrimitiveT;


/// A command to (re-)assign primitives to entities.
class CommandAssignPrimToEntT : public CommandT
{
    public:

    /// Constructor to (re-)assign a list of map primitives to a given entity (the world or any custom entity).
    /// @param Prims    The primitives that are assigned to the given entity.
    /// @param Entity   The entity which all primitives are assigned to.
    CommandAssignPrimToEntT(MapDocumentT& MapDoc, const ArrayT<MapPrimitiveT*>& Prims, MapEntityBaseT* Entity);

    ~CommandAssignPrimToEntT();

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&                m_MapDoc;
    const ArrayT<MapPrimitiveT*> m_Prims;
    ArrayT<MapEntityBaseT*>      m_PrevParents;
    MapEntityBaseT*              m_Entity;
    CommandSelectT*              m_CommandSelect;   ///< Subcommand used by this command to change the selection.
    CommandDeleteT*              m_CommandDelete;   ///< Subcommand to remove empty entities resulting from this command.
};

#endif
