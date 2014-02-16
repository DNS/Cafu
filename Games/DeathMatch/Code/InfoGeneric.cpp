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

#include "InfoGeneric.hpp"
#include "EntityCreateParams.hpp"

#include "GameSys/CompLightPoint.hpp"
#include "TypeSys.hpp"

using namespace GAME_NAME;


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntInfoGenericT::GetType() const
{
    return &TypeInfo;
 // return &EntInfoGenericT::TypeInfo;
}

void* EntInfoGenericT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntInfoGenericT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const cf::TypeSys::TypeInfoT EntInfoGenericT::TypeInfo(GetBaseEntTIM(), "EntInfoGenericT", "BaseEntityT", EntInfoGenericT::CreateInstance, NULL /*MethodsList*/);


EntInfoGenericT::EntInfoGenericT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  BoundingBox3dT(Vector3dT( 4.0,  4.0,  4.0),
                                 Vector3dT(-4.0, -4.0, -4.0)),
                  0)
{
    // Whatever clip model the entity may have brought (in the cmap file), register it with the clip world
    // (needed so that e.g. trigger brushes will work).
    ClipModel.SetOrigin(m_Origin);
    ClipModel.Register();
}


bool EntInfoGenericT::GetLightSourceInfo(unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const
{
    IntrusivePtrT<cf::GameSys::ComponentPointLightT> L = dynamic_pointer_cast<cf::GameSys::ComponentPointLightT>(m_Entity->GetComponent("PointLight"));
    if (L == NULL) return false;
    if (!L->IsOn()) return false;

    const Vector3fT Col = L->GetColor() * 255.0f;

    DiffuseColor  = (unsigned long)(Col.x) + ((unsigned long)(Col.y) << 8) + ((unsigned long)(Col.z) << 16);
    SpecularColor = DiffuseColor;
    Position      = m_Entity->GetTransform()->GetOriginWS().AsVectorOfDouble();
    Radius        = L->GetRadius();
    CastsShadows  = L->CastsShadows();

    return true;
}
