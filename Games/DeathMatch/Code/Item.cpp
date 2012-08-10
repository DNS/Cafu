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

#include "Item.hpp"
#include "EntityCreateParams.hpp"
#include "PhysicsWorld.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/Sound.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "../../GameWorld.hpp"
#include "ConsoleCommands/Console.hpp"
#include "Models/Model_cmdl.hpp"
#include "Network/State.hpp"
#include "TypeSys.hpp"


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntItemT::GetType() const
{
    return &TypeInfo;
 // return &EntItemT::TypeInfo;
}

void* EntItemT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    Console->Warning("Cannot instantiate abstract class!\n");
    assert(false);
    return NULL;
}

const cf::TypeSys::TypeInfoT EntItemT::TypeInfo(GetBaseEntTIM(), "EntItemT", "BaseEntityT", EntItemT::CreateInstance, NULL /*MethodsList*/);


EntItemT::EntItemT(const EntityCreateParamsT& Params, const std::string& ModelName)
    : BaseEntityT(Params,
                  BoundingBox3dT(Vector3dT( 200.0,  200.0,  400.0),
                                 Vector3dT(-200.0, -200.0, -100.0)),
                  NUM_EVENT_TYPES),
      m_TimeLeftNotActive(0.0f),
      m_ItemModel(Params.GameWorld->GetModel(ModelName)),
      m_PickUp(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Item/PickUp"))),
      m_Respawn(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Item/Respawn")))
{
    // Drop items on ground. It doesn't look good when they hover in the air.
    // TODO: Do this only on the server side, it doesn't make sense for clients (where m_Origin==(0, 0, 0) here).
    RayResultT RayResult(NULL /*No object to ignore*/);
    GameWorld->GetPhysicsWorld().TraceRay((m_Origin+Vector3dT(0, 0, 200.0))/1000.0, VectorT(0.0, 0.0, -999999.0/1000.0), RayResult);

    if (RayResult.hasHit())
        m_Origin=convd(RayResult.m_hitPointWorld)*1000.0;
}


EntItemT::~EntItemT()
{
    // Release sounds.
    SoundSystem->DeleteSound(m_PickUp);
    SoundSystem->DeleteSound(m_Respawn);
}


void EntItemT::DoSerialize(cf::Network::OutStreamT& Stream) const
{
    Stream << m_TimeLeftNotActive;
}


void EntItemT::DoDeserialize(cf::Network::InStreamT& Stream)
{
    Stream >> m_TimeLeftNotActive;
}


void EntItemT::Think(float FrameTime, unsigned long /*ServerFrameNr*/)
{
    if (!IsActive())
    {
        m_TimeLeftNotActive-=FrameTime;

        if (IsActive())
        {
            PostEvent(EVENT_TYPE_RESPAWN);
        }
    }
}


void EntItemT::ProcessEvent(unsigned int EventType, unsigned int /*NumEvents*/)
{
    switch (EventType)
    {
        case EVENT_TYPE_PICKED_UP:
            m_PickUp->Play();
            break;

        case EVENT_TYPE_RESPAWN:
            m_Respawn->Play();
            break;
    }
}


void EntItemT::Draw(bool /*FirstPersonView*/, float LodDist) const
{
    if (!IsActive()) return;

    AnimPoseT* Pose=m_ItemModel->GetSharedPose(m_ItemModel->GetAnimExprPool().GetStandard(0, 0.0f));
    Pose->Draw(-1 /*default skin*/, LodDist);

    // RotAngle  +=  234.0*FrameTime; if (RotAngle>360.0) RotAngle-=360.0;
    // SwingAngle+=54321.0*FrameTime; // Wraps automatically!
}


void EntItemT::PostDraw(float FrameTime, bool FirstPersonView)
{
    // Set sound position to entity origin.
    m_PickUp ->SetPosition(m_Origin);
    m_Respawn->SetPosition(m_Origin);
}
