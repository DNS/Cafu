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

#include "Worldspawn.hpp"
#include "EntityCreateParams.hpp"

#include "SceneGraph/Node.hpp"
#include "TypeSys.hpp"

using namespace GAME_NAME;


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntWorldspawnT::GetType() const
{
    return &TypeInfo;
 // return &EntWorldspawnT::TypeInfo;
}

void* EntWorldspawnT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntWorldspawnT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const cf::TypeSys::TypeInfoT EntWorldspawnT::TypeInfo(GetBaseEntTIM(), "EntWorldspawnT", "BaseEntityT", EntWorldspawnT::CreateInstance, NULL /*MethodsList*/);


EntWorldspawnT::EntWorldspawnT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  Params.RootNode->GetBoundingBox(),
                  0)
{
    // Note that the collision model of the world is already handled
    // as a special-case directly by the client and server code.
    // No need to register or to do anything else with it here.
}
