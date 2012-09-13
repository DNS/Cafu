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

#include "FaceHugger.hpp"
#include "EntityCreateParams.hpp"
#include "Interpolator.hpp"
#include "_ResourceManager.hpp"
#include "../../GameWorld.hpp"
#include "TypeSys.hpp"
#include "Models/Model_cmdl.hpp"
#include "Network/State.hpp"
#include "ParticleEngine/ParticleEngineMS.hpp"


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntFaceHuggerT::GetType() const
{
    return &TypeInfo;
 // return &EntFaceHuggerT::TypeInfo;
}

void* EntFaceHuggerT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntFaceHuggerT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const cf::TypeSys::TypeInfoT EntFaceHuggerT::TypeInfo(GetBaseEntTIM(), "EntFaceHuggerT", "BaseEntityT", EntFaceHuggerT::CreateInstance, NULL /*MethodsList*/);


EntFaceHuggerT::EntFaceHuggerT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  BoundingBox3dT(Vector3dT( 100.0,  100.0,  100.0),
                                 Vector3dT(-100.0, -100.0,    0.0)),
                  0),
      m_Velocity(),
      m_Physics(m_Origin, m_Velocity, m_Dimensions, ClipModel, GameWorld->GetClipWorld()),
      m_Model(Params.GameWorld->GetModel("Games/DeathMatch/Models/LifeForms/FaceHugger/FaceHugger.cmdl")),
      m_ModelSequNr(0),
      m_ModelFrameNr(0.0f)
{
    Register(new InterpolatorT<Vector3dT>(m_Origin));
}


void EntFaceHuggerT::DoSerialize(cf::Network::OutStreamT& Stream) const
{
    Stream << float(m_Velocity.x);
    Stream << float(m_Velocity.y);
    Stream << float(m_Velocity.z);
}


void EntFaceHuggerT::DoDeserialize(cf::Network::InStreamT& Stream)
{
    float f = 0.0f;

    Stream >> f; m_Velocity.x = f;
    Stream >> f; m_Velocity.y = f;
    Stream >> f; m_Velocity.z = f;
}


void EntFaceHuggerT::Think(float FrameTime, unsigned long /*ServerFrameNr*/)
{
    bool OldWishJump=false;

    m_Physics.MoveHuman(FrameTime, m_Heading, VectorT() /*WishVelocity*/, VectorT() /*WishVelLadder*/, false /*WishJump*/, OldWishJump, 0.0);
}


bool TestParticleMoveFunction(ParticleMST* Particle, float Time)
{
    const float MaxAge=3.0f;

    Particle->Age+=Time;
    if (Particle->Age>MaxAge) return false;

    if (Particle->Age<1.0f)
    {
        Particle->Color[2]=char(255.0f*(1.0f-Particle->Age));
    }
    else if (Particle->Age<2.0f)
    {
        Particle->Color[1]=char(255.0f*(2.0f-Particle->Age));
    }
    else if (Particle->Age<3.0f)
    {
        Particle->Color[0]=char(255.0f*(3.0f-Particle->Age));
    }

    Particle->Origin[2]+=1000.0f*Time;
    return true;
}


void EntFaceHuggerT::Draw(bool /*FirstPersonView*/, float LodDist) const
{
    AnimPoseT* Pose=m_Model->GetSharedPose(m_Model->GetAnimExprPool().GetStandard(m_ModelSequNr, m_ModelFrameNr));
    Pose->Draw(-1 /*default skin*/, LodDist);
}


void EntFaceHuggerT::PostDraw(float FrameTime, bool /*FirstPersonView*/)
{
    IntrusivePtrT<AnimExprStandardT> StdAE=m_Model->GetAnimExprPool().GetStandard(m_ModelSequNr, m_ModelFrameNr);

    StdAE->SetForceLoop(true);
    StdAE->AdvanceTime(FrameTime);

    const bool SequenceWrap=StdAE->GetFrameNr() < m_ModelFrameNr;
    m_ModelFrameNr=StdAE->GetFrameNr();

    if (SequenceWrap)
    {
        // Register a new particle.
        static ParticleMST TestParticle;

        TestParticle.Origin[0]=float(m_Origin.x);
        TestParticle.Origin[1]=float(m_Origin.y);
        TestParticle.Origin[2]=float(m_Origin.z+200.0);

        TestParticle.Velocity[0]=0;
        TestParticle.Velocity[1]=0;
        TestParticle.Velocity[2]=0;

        TestParticle.Age=0.0;

        TestParticle.Color[0]=255;
        TestParticle.Color[1]=255;
        TestParticle.Color[2]=255;
        TestParticle.Color[3]=255;

        TestParticle.Radius      =300.0;
        TestParticle.StretchY    =1.0;
        TestParticle.RenderMat   =ResMan.RenderMats[ResMan.PARTICLE_GENERIC1];
        TestParticle.MoveFunction=TestParticleMoveFunction;

        ParticleEngineMS::RegisterNewParticle(TestParticle);
    }
}
