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

#include "Rocket.hpp"
#include "_ResourceManager.hpp"
#include "EntityCreateParams.hpp"
#include "PhysicsWorld.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/Sound.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "../../GameWorld.hpp"
#include "../../Interpolator.hpp"
#include "Models/Model_cmdl.hpp"
#include "Network/State.hpp"
#include "ParticleEngine/ParticleEngineMS.hpp"
#include "TypeSys.hpp"


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntRocketT::GetType() const
{
    return &TypeInfo;
 // return &EntRocketT::TypeInfo;
}

void* EntRocketT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntRocketT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const cf::TypeSys::TypeInfoT EntRocketT::TypeInfo(GetBaseEntTIM(), "EntRocketT", "BaseEntityT", EntRocketT::CreateInstance, NULL /*MethodsList*/);


EntRocketT::EntRocketT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  BoundingBox3dT(Vector3dT( 100.0,  100.0,  100.0),
                                 Vector3dT(-100.0, -100.0, -100.0)),
                  NUM_EVENT_TYPES),
      m_Model(Params.GameWorld->GetModel("Games/DeathMatch/Models/Weapons/Grenade/Grenade_w.cmdl")),
      m_FireSound(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/Shotgun_dBarrel"))),
      m_Velocity(),
      m_TimeSinceExploded(0.0f)
{
    Register(new InterpolatorT<Vector3dT>(m_Origin));
    m_FireSound->SetPosition(Params.Origin);
}


EntRocketT::~EntRocketT()
{
    // Release sound.
    SoundSystem->DeleteSound(m_FireSound);
}


void EntRocketT::DoSerialize(cf::Network::OutStreamT& Stream) const
{
    Stream << float(m_Velocity.x);
    Stream << float(m_Velocity.y);
    Stream << float(m_Velocity.z);
    Stream << m_TimeSinceExploded;
}


void EntRocketT::DoDeserialize(cf::Network::InStreamT& Stream)
{
    float f = 0.0f;

    Stream >> f; m_Velocity.x = f;
    Stream >> f; m_Velocity.y = f;
    Stream >> f; m_Velocity.z = f;
    Stream >> m_TimeSinceExploded;
}


void EntRocketT::Think(float FrameTime, unsigned long /*ServerFrameNr*/)
{
    if (m_TimeSinceExploded == 0.0f)
    {
        const VectorT WishDist=scale(m_Velocity, double(FrameTime));

        ShapeResultT ShapeResult;
        GameWorld->GetPhysicsWorld().TraceBoundingBox(m_Dimensions, m_Origin, WishDist, ShapeResult);

        m_Origin=m_Origin+scale(WishDist, double(ShapeResult.m_closestHitFraction));

        if (ShapeResult.hasHit())
        {
            m_TimeSinceExploded += FrameTime;

            PostEvent(EVENT_TYPE_EXPLODE);

            // Damage all entities that are close enough.
            // TODO: In order to avoid damaging entities through thin walls, we should also perform a simple "visibility test".
            const ArrayT<unsigned long>& AllEntityIDs=GameWorld->GetAllEntityIDs();

            for (unsigned long EntityIDNr=0; EntityIDNr<AllEntityIDs.Size(); EntityIDNr++)
            {
                IntrusivePtrT<BaseEntityT> OtherEntity=static_pointer_cast<BaseEntityT>(GameWorld->GetGameEntityByID(AllEntityIDs[EntityIDNr]));

                if (OtherEntity    ==NULL) continue;
                if (OtherEntity->ID==  ID) continue;    // We don't damage us ourselves.

                // Note that OtherOrigin=OtherEntity->GetOrigin() is not enough, it must be computed as shown in order to work in all cases:
                // a) With (e.g.) EntHumanPlayerTs, the Dimensions are static and the Origin moves, but
                // b) with EntRigidBodyTs, the Dimensions move while the Origin remains at (0, 0, 0).
                const Vector3dT OtherOrigin=OtherEntity->GetDimensions().GetCenter() + OtherEntity->GetOrigin();
                const Vector3dT Impact     =OtherOrigin-m_Origin;
                const double    Dist       =length(Impact);

                     if (Dist<1000.0) OtherEntity->TakeDamage(this, 100                         , scale(Impact, 1.0/Dist));
                else if (Dist<5000.0) OtherEntity->TakeDamage(this, 100-char((Dist-1000.0)/40.0), scale(Impact, 1.0/Dist));
            }
        }
    }
    else
    {
        // Wait 3 seconds after explosion before this entity is finally removed (such that the explosion event can travel to the clients).
        m_TimeSinceExploded += FrameTime;
        if (m_TimeSinceExploded > 3.0f) GameWorld->RemoveEntity(ID);
    }
}


bool ParticleFunction_RocketExplosionMain(ParticleMST* Particle, float Time)
{
    const float FPS   =15.0f;        // The default value is 20.0.
    const float MaxAge=26.0f/FPS;    // 26 frames at 15 FPS.

    Particle->RenderMat=ResMan.RenderMats[ResMan.PARTICLE_EXPLOSION1_FRAME1+(unsigned long)(Particle->Age*FPS)];

    Particle->Age+=Time;
    if (Particle->Age>=MaxAge) return false;

    return true;
}


bool ParticleFunction_RocketExplosionSmall(ParticleMST* Particle, float Time)
{
    const float MaxAge=0.5f;

    Particle->Age+=Time;
    if (Particle->Age>MaxAge) return false;

    Particle->Origin[0]+=Particle->Velocity[0]*Time;
    Particle->Origin[1]+=Particle->Velocity[1]*Time;
    Particle->Origin[2]+=Particle->Velocity[2]*Time;

 // Particle->Velocity[0]*=0.99;    // TODO: Deceleration should depend on 'Time'...
 // Particle->Velocity[1]*=0.99;
 // Particle->Velocity[2]*=0.90;

    Particle->Color[0]=char(255.0f*(MaxAge-Particle->Age)/MaxAge);
    Particle->Color[1]=char(255.0f*(MaxAge-Particle->Age)/MaxAge*(MaxAge-Particle->Age)/MaxAge);
    return true;
}


void EntRocketT::ProcessEvent(unsigned int /*EventType*/, unsigned int /*NumEvents*/)
{
    // We only receive a single event here ("Detonation!"), thus there is no need to look at 'EventID'.
    // Update souud position.
    m_FireSound->SetPosition(m_Origin);

    // Play the fire sound.
    m_FireSound->Play();

    // Register explosion particles.
    static ParticleMST NewParticle;

    NewParticle.Origin[0]=float(m_Origin.x);
    NewParticle.Origin[1]=float(m_Origin.y);
    NewParticle.Origin[2]=float(m_Origin.z);

    NewParticle.Age=0.0;
    NewParticle.Color[0]=255;
    NewParticle.Color[1]=255;
    NewParticle.Color[2]=255;
    NewParticle.Color[3]=255;

    NewParticle.Radius=2000.0;
    NewParticle.Rotation=char(rand());
    NewParticle.StretchY=1.0;
    NewParticle.RenderMat=ResMan.RenderMats[ResMan.PARTICLE_EXPLOSION1_FRAME1];
    NewParticle.MoveFunction=ParticleFunction_RocketExplosionMain;

    ParticleEngineMS::RegisterNewParticle(NewParticle);

    for (char i=0; i<20; i++)
    {
        NewParticle.Origin[0]=float(m_Origin.x);
        NewParticle.Origin[1]=float(m_Origin.y);
        NewParticle.Origin[2]=float(m_Origin.z);

        NewParticle.Velocity[0]=float(rand()-int(RAND_MAX/2));
        NewParticle.Velocity[1]=float(rand()-int(RAND_MAX/2));
        NewParticle.Velocity[2]=float(rand()-int(RAND_MAX/2));

        NewParticle.Age=0.0;
        NewParticle.Color[0]=255;
        NewParticle.Color[1]=255;
        NewParticle.Color[2]=0;
        NewParticle.Color[3]=0;     // Additive blending is used for this particle.

        NewParticle.Radius=200.0;
        NewParticle.StretchY=1.0;
        NewParticle.RenderMat=ResMan.RenderMats[ResMan.PARTICLE_GENERIC1];
        NewParticle.MoveFunction=ParticleFunction_RocketExplosionSmall;

        ParticleEngineMS::RegisterNewParticle(NewParticle);
    }
}


bool EntRocketT::GetLightSourceInfo(unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const
{
    if (m_TimeSinceExploded == 0.0f)
    {
        DiffuseColor =0x0000FFFF;
        SpecularColor=0x000030FF;
    }
    else
    {
        const float         Dim  =(3.0f - m_TimeSinceExploded)/3.0f;
        const unsigned long Red  =(unsigned long)(0xFF*Dim);
        const unsigned long Green=(unsigned long)(0xFF*Dim*Dim);

        DiffuseColor =(Green << 8)+Red;
        SpecularColor=(Red << 8)+Green;
    }

    Position    =m_Origin;
    Radius      =25000.0;
    CastsShadows=true;

    if (m_Velocity.GetLengthSqr()>1.0) Position-=scale(normalize(m_Velocity, 0.0), 400.0);

    return true;
}


void EntRocketT::Draw(bool /*FirstPersonView*/, float LodDist) const
{
    if (m_TimeSinceExploded > 0.0f) return;

    AnimPoseT* Pose=m_Model->GetSharedPose(m_Model->GetAnimExprPool().GetStandard(0, 0.0f));
    Pose->Draw(-1 /*default skin*/, LodDist);
}
