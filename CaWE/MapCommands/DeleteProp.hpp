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

#ifndef _COMMAND_DELETEPROP_HPP_
#define _COMMAND_DELETEPROP_HPP_

#include "../CommandPattern.hpp"
#include "../EntProperty.hpp"


class MapDocumentT;
class MapEntityBaseT;


class CommandDeletePropertyT : public CommandT
{
    public:

    /// Constructor to delete a property from an entity by key name.
    CommandDeletePropertyT(MapDocumentT& MapDoc, MapEntityBaseT* Entity, const wxString& Key);

    /// Constructor to delete a property from an entity by key index.
    CommandDeletePropertyT(MapDocumentT& MapDoc, MapEntityBaseT* Entity, int Index);

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&      m_MapDoc;
    MapEntityBaseT*    m_Entity;
    const int          m_Index;
    const EntPropertyT m_PropBackup;
};

#endif
