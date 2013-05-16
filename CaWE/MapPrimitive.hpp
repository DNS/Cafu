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


/// This class adds no functionality of its own, but only exists for proper type separation.
/// Especially MapBaseEntityT keeps a MapEntRepresT and an array of MapPrimitiveT%s, and the two
/// "sets" must not overlap (we don't want MapEntRepresT instances among the "regular" primitives,
/// and no regular primitive should ever be in place of the m_Repres member).
/// In many other regards, all derived classes are considered equivalent and treated the same;
/// then we use arrays of MapElementT%s.
/// The clear distinction between MapElementT%s and MapPrimitiveT%s (the former can also contain
/// MapEntRepresT%s, the latter cannot) is also a great help in documentation and communication.
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
    /// would call the Clone() method of MapElementT, return a `MapElementT*`, and
    /// thus fail to compile, because a `MapElementT*` cannot be assigned to `NewPrim`.
    MapPrimitiveT* Clone() const=0;


    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;
};

#endif
