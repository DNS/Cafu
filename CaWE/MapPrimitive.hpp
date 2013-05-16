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

#ifndef CAFU_MAP_PRIMITIVE_HPP_INCLUDED
#define CAFU_MAP_PRIMITIVE_HPP_INCLUDED

#include "MapElement.hpp"


/// A common base class for all the basic, atomic map elements in the world and the entities.
class MapPrimitiveT : public MapElementT
{
    public:

    /// The default constructor.
    MapPrimitiveT(const wxColour& Color);

    /// The copy constructor for copying a primitive.
    /// @param Prim   The primitive to copy-construct this primitive from.
    MapPrimitiveT(const MapPrimitiveT& Prim);

    /// Explicitly declare the override for MapElementT::Clone().
    /// Without this declaration, a statement like
    ///     MapPrimitiveT* NewPrim = Prim->Clone();
    /// would call the Clone() method of MapElementT, return a `MapElementT*`,
    /// and then fail, because a `MapElementT*` cannot be assigned to `NewPrim`.
    MapPrimitiveT* Clone() const=0;


    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;
};

#endif
