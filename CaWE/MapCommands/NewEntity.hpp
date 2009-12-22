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

#ifndef _COMMAND_NEW_ENTITY_HPP_
#define _COMMAND_NEW_ENTITY_HPP_

#include "../CommandPattern.hpp"
#include "Math3D/Plane3.hpp"


class CommandSelectT;
class EntityClassT;
class MapDocumentT;
class MapEntityT;


class CommandNewEntityT : public CommandT
{
    public:

    /// Constructor to create a new entity.
    /// @param MapDoc        Map document in which the entity is created.
    /// @param ClassName     Class of the point entity that is created.
    /// @param Position      Position at which the entity is created.
    /// @param AdjustPlane   The (optional) plane the new entities origin is adjusted to.
    CommandNewEntityT(MapDocumentT& MapDoc, const EntityClassT* EntityClass, const Vector3fT& Position, const Plane3fT* AdjustPlane=NULL);

    /// The destructor.
    ~CommandNewEntityT();

    /// Returns the new entity created by this command.
    MapEntityT* GetEntity() const { return m_NewEntity; }

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    MapDocumentT&   m_MapDoc;
    MapEntityT*     m_NewEntity;
    CommandSelectT* m_CommandSelect;    ///< Subcommand for changing the selection.
};

#endif
