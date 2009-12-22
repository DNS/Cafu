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

#include "MapHelperBB.hpp"
#include "MapEntity.hpp"
#include "Options.hpp"
#include "Renderer3D.hpp"
#include "TypeSys.hpp"


/*** Begin of TypeSys related definitions for this class. ***/

void* MapHelperBoundingBoxT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapHelperBoundingBoxT::TypeInfo(GetMapElemTIM(), "MapHelperBoundingBoxT", "MapHelperT", MapHelperBoundingBoxT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapHelperBoundingBoxT::MapHelperBoundingBoxT(const MapEntityT* ParentEntity, const BoundingBox3fT& BB, bool Wireframe)
    : MapHelperT(),
      m_ParentEntity(ParentEntity),
      m_BB(BB),
      m_Wireframe(Wireframe)
{
}


MapHelperBoundingBoxT::MapHelperBoundingBoxT(const MapHelperBoundingBoxT& Box)
    : MapHelperT(Box),
      m_ParentEntity(Box.m_ParentEntity),
      m_BB(Box.m_BB),
      m_Wireframe(Box.m_Wireframe)
{
}


MapHelperBoundingBoxT* MapHelperBoundingBoxT::Clone() const
{
    return new MapHelperBoundingBoxT(*this);
}


void MapHelperBoundingBoxT::Assign(const MapElementT* Elem)
{
    if (Elem==this) return;

    MapHelperT::Assign(Elem);

    const MapHelperBoundingBoxT* Box=dynamic_cast<const MapHelperBoundingBoxT*>(Elem);
    wxASSERT(Box!=NULL);
    if (Box==NULL) return;

    m_ParentEntity=Box->m_ParentEntity;
    m_BB          =Box->m_BB;
    m_Wireframe   =Box->m_Wireframe;
}


BoundingBox3fT MapHelperBoundingBoxT::GetBB() const
{
    const Vector3fT Origin=m_ParentEntity->GetOrigin();

    return BoundingBox3fT(Origin+m_BB.Min, Origin+m_BB.Max);
}


void MapHelperBoundingBoxT::Render2D(Renderer2DT& Renderer) const
{
    // Render nothing in 2D, as the parent entity already renders its bounding box,
    // center cross, orientation (angles), etc. by itself.
}


void MapHelperBoundingBoxT::Render3D(Renderer3DT& Renderer) const
{
    Renderer.RenderBox(GetBB(),
        IsSelected() ? Options.colors.Selection : m_ParentEntity->GetColor(Options.view2d.UseGroupColors), !m_Wireframe /* Solid? */);
}
