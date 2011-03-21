/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _COMMAND_APPLY_MATERIAL_HPP_
#define _COMMAND_APPLY_MATERIAL_HPP_

#include "../CommandPattern.hpp"


class MapDocumentT;
class EditorMaterialI;


class CommandApplyMaterialT : public CommandT
{
    public:

    /// Constructor to apply the given material to an array of objects.
    CommandApplyMaterialT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& Objects, EditorMaterialI* Material);

    /// Destructor.
    ~CommandApplyMaterialT();

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    MapDocumentT&              m_MapDoc;
    const ArrayT<MapElementT*> m_Objects;
    ArrayT<MapElementT*>       m_OldStates;
    EditorMaterialI*           m_Material;
};

#endif
