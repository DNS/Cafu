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

/*********************/
/*** Corspe (Code) ***/
/*********************/

#include "Corpse.hpp"
#include "EntityCreateParams.hpp"
#include "HumanPlayer.hpp"
#include "TypeSys.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Models/Model_proxy.hpp"


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntCorpseT::GetType() const
{
    return &TypeInfo;
 // return &EntCorpseT::TypeInfo;
}

void* EntCorpseT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntCorpseT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const cf::TypeSys::TypeInfoT EntCorpseT::TypeInfo(GetBaseEntTIM(), "EntCorpseT", "BaseEntityT", EntCorpseT::CreateInstance, NULL /*MethodsList*/);


EntCorpseT::EntCorpseT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  // Bad. Should either have a default ctor for 'EntityStateT', or even better get it passed as const reference.
                  EntityStateT(VectorT(),
                               VectorT(),
                               BoundingBox3T<double>(Vector3dT()),
                               0,       // Heading
                               0,
                               0,
                               0,       // StateOfExistance
                               0,
                               0,       // ModelIndex
                               0,       // ModelSequNr
                               0.0,     // ModelFrameNr
                               0,       // Health
                               0,       // Armor
                               0,       // HaveItems
                               0,       // HaveWeapons
                               0,       // ActiveWeaponSlot
                               0,       // ActiveWeaponSequNr
                               0.0))    // ActiveWeaponFrameNr
{
}


void EntCorpseT::Think(float /*FrameTime*/, unsigned long /*ServerFrameNr*/)
{
    // TODO: Disappear when some condition is met (timeout, not in anyones PVS, alpha fade-out, too many corpses, ...)
    // TODO: Decompose to gibs when hit by a rocket.
}


void EntCorpseT::Draw(bool /*FirstPersonView*/, float LodDist) const
{
    MatSys::Renderer->GetCurrentLightSourcePosition()[2]+=32.0f;
    MatSys::Renderer->GetCurrentEyePosition        ()[2]+=32.0f;
    MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, 0.0f, 0.0f, -32.0f);

    EntHumanPlayerT::GetModelFromPlayerModelIndex(State.ModelIndex).Draw(State.ModelSequNr, State.ModelFrameNr, LodDist/*, (State.HaveWeapons & (1 << State.ActiveWeaponSlot)) ? &ModelManager::GetModelByIndex(GetModelIndexPlayerByWeaponSlot[State.ActiveWeaponSlot]) : NULL*/);
}
