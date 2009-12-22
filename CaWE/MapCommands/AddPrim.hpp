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

#ifndef _COMMAND_ADD_PRIMITIVE_HPP_
#define _COMMAND_ADD_PRIMITIVE_HPP_

#include "../CommandPattern.hpp"


class MapDocumentT;
class MapElementT;
class MapEntityBaseT;
class MapPrimitiveT;
class CommandSelectT;


/// This class implements a command for adding primitives into the map, as part of their specified parent entities.
/// It is quasi the counterpart to CommandDeleteT.
class CommandAddPrimT : public CommandT
{
    public:

    /// The constructor for creating a command for adding a single primitive into the map.
    /// @param MapDoc    The map document into which the primitive is inserted.
    /// @param AddPrim   The primitive to add. NOTE: AddPrim->GetParent()==NULL is assumed, i.e. AddPrim has no prior parent!
    /// @param Parent    The parent entity that the primitive becomes a part of and which it is added to. Can be the world or a custom entity.
    /// @param Name      The name of this command for the undo history.
    CommandAddPrimT(MapDocumentT& MapDoc, MapPrimitiveT* AddPrim, MapEntityBaseT* Parent, wxString Name="new primitive");

    /// The constructor for creating a command for adding multiple primitives into the map.
    /// Analogous to the constructor for adding a single primitive above.
    CommandAddPrimT(MapDocumentT& MapDoc, const ArrayT<MapPrimitiveT*>& AddPrims, MapEntityBaseT* Parent, wxString Name="new primitives");

    /// The destructor.
    ~CommandAddPrimT();

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    MapDocumentT&          m_MapDoc;
    ArrayT<MapPrimitiveT*> m_AddPrims;
    ArrayT<MapElementT*>   m_AddElems;      ///< The same pointers as in m_AddPrims with the type of the base class.
    MapEntityBaseT*        m_Parent;
    CommandSelectT*        m_CommandSelect; ///< Subcommand for changing the selection.
    wxString               m_Name;
};

#endif
