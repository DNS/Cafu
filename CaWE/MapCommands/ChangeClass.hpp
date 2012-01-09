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

#ifndef CAFU_COMMAND_CHANGE_CLASS_HPP_INCLUDED
#define CAFU_COMMAND_CHANGE_CLASS_HPP_INCLUDED

#include "../CommandPattern.hpp"


class MapDocumentT;
class MapEntityT;
class EntityClassT;
class EntPropertyT;


class CommandChangeClassT : public CommandT
{
    public:

    /// Constructor to change the class of an entity.
    /// @param MapDoc     The map document that the entity lives in.
    /// @param Entity     Cannot be NULL.
    /// @param NewClass   The new class for the entity.
    CommandChangeClassT(MapDocumentT& MapDoc, MapEntityT* Entity, const EntityClassT* NewClass);

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapEntityT*                m_Entity;
    const ArrayT<EntPropertyT> m_PrevProps; ///< Changing the class of an entity can update (alter) the properties as well, so we need to remember previous properties for restoring them on Undo().
    MapDocumentT&              m_MapDoc;
    const EntityClassT*        m_NewClass;
    const EntityClassT*        m_PrevClass;
};

#endif
