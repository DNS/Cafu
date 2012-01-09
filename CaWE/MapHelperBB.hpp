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

#ifndef _MAP_HELPER_BOUNDINGBOX_HPP_
#define _MAP_HELPER_BOUNDINGBOX_HPP_

#include "MapHelper.hpp"
#include "Math3D/BoundingBox.hpp"


class MapEntityT;
class Renderer2DT;
class Renderer3DT;
namespace cf { namespace TypeSys { class TypeInfoT; } }
namespace cf { namespace TypeSys { class TypeInfoManT; } }
namespace cf { namespace TypeSys { class CreateParamsT; } }


class MapHelperBoundingBoxT : public MapHelperT
{
    public:

    /// The constructor.
    MapHelperBoundingBoxT(const MapEntityT* ParentEntity, const BoundingBox3fT& BB, bool Wireframe=false);

    /// The copy constructor for copying a box.
    /// @param Box   The box to copy-construct this box from.
    MapHelperBoundingBoxT(const MapHelperBoundingBoxT& Box);


    // Implementations and overrides for base class methods.
    MapHelperBoundingBoxT* Clone() const;
    void                   Assign(const MapElementT* Elem);


    BoundingBox3fT GetBB() const;

    void Render2D(Renderer2DT& Renderer) const;
    void Render3D(Renderer3DT& Renderer) const;

    wxString GetDescription() const { return "Bounding box"; }

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    BoundingBox3fT m_BB;        ///< The dimensions of this box, relative to m_Origin.
    bool           m_Wireframe; ///< Whether this box is rendered as wire-frame in the 3D views, or solid.
};

#endif
