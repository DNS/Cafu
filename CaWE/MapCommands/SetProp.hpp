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

#ifndef _COMMAND_SET_PROP_HPP_
#define _COMMAND_SET_PROP_HPP_

#include "../CommandPattern.hpp"


class MapDocumentT;
class MapEntityBaseT;


class CommandSetPropertyT : public CommandT
{
    public:

    /// Constructor to set the property of an entity (the property is created if not existent).
    CommandSetPropertyT(MapDocumentT& MapDoc, MapEntityBaseT* Ent, const wxString& Key, const wxString& NewValue);

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&   m_MapDoc;
    MapEntityBaseT* m_Ent;
    const bool      m_CreatedProp;  ///< Determines if a property has been created to set the NewValue. Created properties need to be deleted on Undo() and not just reset to OldValue.
    const wxString  m_Key;
    const wxString  m_OldValue;     ///< Previous value of the changed entity key.
    const wxString  m_NewValue;
};

#endif
