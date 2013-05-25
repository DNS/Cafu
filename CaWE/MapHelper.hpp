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

#ifndef CAFU_MAP_HELPER_HPP_INCLUDED
#define CAFU_MAP_HELPER_HPP_INCLUDED

#include "Math3D/BoundingBox.hpp"


class MapEntRepresT;
class Renderer2DT;
class Renderer3DT;


class MapHelperT
{
    public:

    /// The default constructor.
    MapHelperT(MapEntRepresT& Repres);

    /// The copy constructor for copying a helper.
    /// @param Helper   The helper to copy-construct this helper from.
    MapHelperT(const MapHelperT& Helper);

    /// The virtual destructor, so that derived classes can be deleted via a MapHelperT pointer.
    virtual ~MapHelperT() { }

    /// Returns the spatial bounding-box of this map element.
    virtual BoundingBox3fT GetBB() const=0;

    virtual void Render2D(Renderer2DT& Renderer) const { }
    virtual void Render3D(Renderer3DT& Renderer) const { }


    protected:

    MapEntRepresT& m_Repres;    ///< The entity representation instance that we are a helper for.
};

#endif
