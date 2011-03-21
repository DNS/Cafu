/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

/***************************/
/*** Hand Grenade (Code) ***/
/***************************/

#include "HandGrenade.hpp"
#include "_ResourceManager.hpp"
#include "EntityCreateParams.hpp"
#include "TypeSys.hpp"
#include "Libs/Physics.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/Sound.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "../../GameWorld.hpp"
#include "Models/Model_proxy.hpp"
#include "ParticleEngine/ParticleEngineMS.hpp"


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntHandGrenadeT::GetType() const
{
    return &TypeInfo;
 // return &EntHandGrenadeT::TypeInfo;
}

void* EntHandGrenadeT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntHandGrenadeT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const cf::TypeSys::TypeInfoT EntHandGrenadeT::TypeInfo(GetBaseEntTIM(), "EntHandGrenadeT", "BaseEntityT", EntHandGrenadeT::CreateInstance, NULL /*MethodsList*/);


EntHandGrenadeT::EntHandGrenadeT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  EntityStateT(Params.Origin,
                               VectorT(),
                               BoundingBox3T<double>(VectorT( 60.0,  60.0, 120.0),
                                                     VectorT(-60.0, -60.0,   0.0)),
                               0,       // Heading
                               0,       // Pitch
                               0,       // Bank
                               0,       // StateOfExistance
                               0,       // Flags
                               0,       // ModelIndex
                               0,       // ModelSequNr
                               0.0,     // ModelFrameNr
                               0,       // Health
                               0,       // Armor
                               0,       // HaveItems
                               0,       // HaveWeapons
                               0,       // ActiveWeaponSlot
                               0,       // ActiveWeaponSequNr
                               0.0)),   // ActiveWeaponFrameNr
      FireSound(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/Shotgun_dBarrel")))
{
    FireSound->SetPosition(Params.Origin);
}


EntHandGrenadeT::~EntHandGrenadeT()
{
    // Release sound.
    SoundSystem->DeleteSound(FireSound);
}


void EntHandGrenadeT::Think(float FrameTime, unsigned long /*ServerFrameNr*/)
{
    const float OldTimer=State.ActiveWeaponFrameNr;

    // Let the detonation timer tick.
    State.ActiveWeaponFrameNr+=FrameTime;


    if (State.ActiveWeaponFrameNr<3.0)
    {
        bool OldWishJump=false;

        Physics::MoveHuman(State, ClipModel, FrameTime, VectorT() /*WishVelocity*/, VectorT() /*WishVelLadder*/, false /*WishJump*/, OldWishJump, 0.0, GameWorld->GetClipWorld());
    }
    else if (State.ActiveWeaponFrameNr<6.0)     // (3.0<=State.ActiveWeaponFrameNr<6.0)
    {
        if (OldTimer<3.0)
        {
            // Explode!
            State.Events=1;     // "Flip" bit 1.

            // Damage all entities that are close enough.
            // TODO: In order to avoid damaging entities through thin walls, we should also perform a simple "visibility test".
            const ArrayT<unsigned long>& AllEntityIDs=GameWorld->GetAllEntityIDs();

            for (unsigned long EntityIDNr=0; EntityIDNr<AllEntityIDs.Size(); EntityIDNr++)
            {
                BaseEntityT* OtherEntity=GameWorld->GetBaseEntityByID(AllEntityIDs[EntityIDNr]);

                if (OtherEntity    ==NULL) continue;
                if (OtherEntity->ID==  ID) continue;    // We don't damage us ourselves.

                // Note that OtherOrigin=OtherEntity->State.Origin is not enough, it must be computed as shown in order to work in all cases:
                // a) With (e.g.) EntHumanPlayerTs, the Dimensions are static and the Origin moves, but
                // b) with EntRigidBodyTs, the Dimensions move while the Origin remains at (0, 0, 0).
                const Vector3dT OtherOrigin=OtherEntity->State.Dimensions.GetCenter() + OtherEntity->State.Origin;
                const Vector3dT Impact     =OtherOrigin-State.Origin;
                const double    Dist       =length(Impact);

                     if (Dist<1000.0) OtherEntity->TakeDamage(this, 100                         , scale(Impact, 1.0/Dist));
                else if (Dist<5000.0) OtherEntity->TakeDamage(this, 100-char((Dist-1000.0)/40.0), scale(Impact, 1.0/Dist));
            }
        }
    }
    else
    {
        // Remove this entity (it exploded about 3 seconds ago).
        GameWorld->RemoveEntity(ID);
    }
}


bool ParticleFunction_HandGrenadeExplosionMain(ParticleMST* Particle, float Time)
{
    const float FPS   =18.0f;        // The default value is 20.0.
    const float MaxAge=27.0f/FPS;    // 27 frames at 18 FPS.

    Particle->RenderMat=ResMan.RenderMats[ResMan.PARTICLE_EXPLOSION2_FRAME1+(unsigned long)(Particle->Age*FPS)];

    Particle->Age+=Time;
    if (Particle->Age>=MaxAge) return false;

    return true;
}


bool ParticleFunction_HandGrenadeExplosionSmall(ParticleMST* Particle, float Time)
{
    const float MaxAge=0.7f;

    Particle->Age+=Time;
    if (Particle->Age>MaxAge) return false;

    Particle->Origin[0]+=Particle->Velocity[0]*Time;
    Particle->Origin[1]+=Particle->Velocity[1]*Time;
    Particle->Origin[2]+=Particle->Velocity[2]*Time;

    Particle->Velocity[0]*=0.98f;   // TODO: Deceleration should depend on 'Time'...
    Particle->Velocity[1]*=0.98f;
    Particle->Velocity[2]-=2.0f*9810.0f*Time;     // double gravity...

    Particle->Color[0]=char(255.0f*(MaxAge-Particle->Age)/MaxAge);
    Particle->Color[1]=char(255.0f*(MaxAge-Particle->Age)/MaxAge*(MaxAge-Particle->Age)/MaxAge);
    return true;
}


void EntHandGrenadeT::ProcessEvent(char /*EventID*/)
{
    // We only receive a single event here ("Detonation!"), thus there is no need to look at 'EventID'.
    // Update sound position.
    FireSound->SetPosition(State.Origin);

    // Play the fire sound.
    FireSound->Play();

    // Register explosion particles.
    static ParticleMST NewParticle;

    NewParticle.Origin[0]=float(State.Origin.x);
    NewParticle.Origin[1]=float(State.Origin.y);
    NewParticle.Origin[2]=float(State.Origin.z+200.0);

    NewParticle.Age=0.0;
    NewParticle.Color[0]=255;
    NewParticle.Color[1]=255;
    NewParticle.Color[2]=255;
    NewParticle.Color[3]=0;

    NewParticle.Radius=1500.0;
    NewParticle.Rotation=char(rand());
    NewParticle.StretchY=1.0;
    NewParticle.RenderMat=ResMan.RenderMats[ResMan.PARTICLE_EXPLOSION2_FRAME1];
    NewParticle.MoveFunction=ParticleFunction_HandGrenadeExplosionMain;

    ParticleEngineMS::RegisterNewParticle(NewParticle);

    for (char i=0; i<20; i++)
    {
        NewParticle.Origin[0]=float(State.Origin.x);
        NewParticle.Origin[1]=float(State.Origin.y);
        NewParticle.Origin[2]=float(State.Origin.z);

        NewParticle.Velocity[0]=float(rand()-int(RAND_MAX/2));
        NewParticle.Velocity[1]=float(rand()-int(RAND_MAX/2));
        NewParticle.Velocity[2]=float(rand());

        NewParticle.Age=0.0;
        NewParticle.Color[0]=255;
        NewParticle.Color[1]=255;
        NewParticle.Color[2]=0;
        NewParticle.Color[3]=0;

        NewParticle.Radius=300.0;
        NewParticle.StretchY=1.0;
        NewParticle.RenderMat=ResMan.RenderMats[ResMan.PARTICLE_GENERIC1];
        NewParticle.MoveFunction=ParticleFunction_HandGrenadeExplosionSmall;

        ParticleEngineMS::RegisterNewParticle(NewParticle);
    }
}


bool EntHandGrenadeT::GetLightSourceInfo(unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const
{
    const float Duration=0.5f;

    if (State.ActiveWeaponFrameNr< 3.0f         ) return false;
    if (State.ActiveWeaponFrameNr>=3.0f+Duration) return false;

    const float Amount=1.0f-(State.ActiveWeaponFrameNr-3.0f)/Duration;

    const unsigned long Red  =(unsigned long)(0xFF*Amount);
    const unsigned long Green=(unsigned long)(0xFF*Amount*Amount);
    const unsigned long Blue =(unsigned long)(0xFF*Amount*Amount*Amount);

    DiffuseColor =(Blue << 16)+(Green << 8)+Red;
    SpecularColor=DiffuseColor;
    Position     =State.Origin;
    Radius       =10000.0;
    CastsShadows =true;

    return true;
}


void EntHandGrenadeT::Draw(bool /*FirstPersonView*/, float LodDist) const
{
    if (State.ActiveWeaponFrameNr>=3.0) return;

    static ModelProxyT HandGrenadeModel("Games/DeathMatch/Models/Weapons/Grenade_w.mdl");

    HandGrenadeModel.Draw(State.ModelSequNr, State.ModelFrameNr, LodDist);
}
