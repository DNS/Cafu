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

#ifndef _MAP_PRIMITIVE_HPP_
#define _MAP_PRIMITIVE_HPP_

#include "MapElement.hpp"


class MapEntityBaseT;


/// A common base class for all the basic, atomic map elements in the world and the entities.
class MapPrimitiveT : public MapElementT
{
    public:

    /// The default constructor.
    MapPrimitiveT(const wxColour& Color);

    /// The copy constructor for copying a primitive.
    /// @param Prim   The primitive to copy-construct this primitive from.
    MapPrimitiveT(const MapPrimitiveT& Prim);


    // Implementations and overrides for base class methods.
    MapPrimitiveT* Clone() const=0;
    void           Assign(const MapElementT* Elem);
    wxColour       GetColor(bool ConsiderGroup=true) const;


    /// Assigns the parent entity for this primitive.
    void SetParent(MapEntityBaseT* Ent) { m_Parent=Ent; }

    /// Returns the parent entity of this primitive.
    /// The returned pointer can be NULL when the primitive is not anchored in the world
    /// (this can happen when the primitive is kept in the clipboard, in a command, etc.).
    MapEntityBaseT* GetParent() const { return m_Parent; }


    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    protected:

    MapEntityBaseT* m_Parent;
};

#endif
