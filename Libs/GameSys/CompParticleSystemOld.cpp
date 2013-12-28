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

#include "CompParticleSystemOld.hpp"
#include "AllComponents.hpp"
#include "Entity.hpp"

#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "ParticleEngine/ParticleEngineMS.hpp"
#include "UniScriptState.hpp"

using namespace cf::GameSys;


const char* ComponentParticleSystemOldT::DocClass =
    "This component adds a particle system to its entity.\n"
    "The particle system is obsolete though: This is just a quick and dirty port\n"
    "of the particle system in the old game system to the new component system.\n"
    "Both its interface and its implementation need a thorough overhaul.";


const cf::TypeSys::VarsDocT ComponentParticleSystemOldT::DocVars[] =
{
    { "Type", "The type of the particles emitted by this system." },
    { NULL, NULL }
};


ComponentParticleSystemOldT::ComponentParticleSystemOldT()
    : ComponentBaseT(),
      m_Type("Type", "")
{
    GetMemberVars().Add(&m_Type);

    InitRenderMats();
}


ComponentParticleSystemOldT::ComponentParticleSystemOldT(const ComponentParticleSystemOldT& Comp)
    : ComponentBaseT(Comp),
      m_Type(Comp.m_Type)
{
    GetMemberVars().Add(&m_Type);

    InitRenderMats();
}


ComponentParticleSystemOldT::~ComponentParticleSystemOldT()
{
    for (unsigned long RMNr = 0; RMNr < m_RenderMats.Size(); RMNr++)
        MatSys::Renderer->FreeMaterial(m_RenderMats[RMNr]);

    m_RenderMats.Clear();
}


ComponentParticleSystemOldT* ComponentParticleSystemOldT::Clone() const
{
    return new ComponentParticleSystemOldT(*this);
}


void ComponentParticleSystemOldT::InitRenderMats()
{
    {
        m_RenderMats.PushBack(MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("Sprites/Generic1")));
    }
}


namespace
{
    bool ParticleMove_FaceHugger(ParticleMST* Particle, float Time)
    {
        const float MaxAge = 3.0f;

        Particle->Age += Time;
        if (Particle->Age > MaxAge) return false;

        if (Particle->Age < 1.0f)
        {
            Particle->Color[2] = char(255.0f * (1.0f - Particle->Age));
        }
        else if (Particle->Age < 2.0f)
        {
            Particle->Color[1] = char(255.0f * (2.0f - Particle->Age));
        }
        else if (Particle->Age < 3.0f)
        {
            Particle->Color[0] = char(255.0f * (3.0f - Particle->Age));
        }

        Particle->Origin[2] += 40.0f * Time;
        return true;
    }
}


static const cf::TypeSys::MethsDocT META_EmitParticle =
{
    "EmitParticle",
    "Emits a new particle in this particle system.",
    "", "()"
};

int ComponentParticleSystemOldT::EmitParticle(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentParticleSystemOldT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentParticleSystemOldT> >(1);

    if (!Comp->GetEntity())
        luaL_error(LuaState, "The component must be added to an entity before this function can be called.");

    const Vector3fT& Origin = Comp->GetEntity()->GetTransform()->GetOrigin();

    // Register a new particle.
    static ParticleMST TestParticle;

    {
        TestParticle.Origin[0] = Origin.x;
        TestParticle.Origin[1] = Origin.y;
        TestParticle.Origin[2] = Origin.z + 8.0f;

        TestParticle.Velocity[0] = 0;
        TestParticle.Velocity[1] = 0;
        TestParticle.Velocity[2] = 0;

        TestParticle.Age = 0.0;

        TestParticle.Color[0] = 255;
        TestParticle.Color[1] = 255;
        TestParticle.Color[2] = 255;
        TestParticle.Color[3] = 255;

        TestParticle.Radius       = 12.0;
        TestParticle.StretchY     = 1.0;
        TestParticle.RenderMat    = Comp->m_RenderMats[0];
        TestParticle.MoveFunction = ParticleMove_FaceHugger;
    }

    ParticleEngineMS::RegisterNewParticle(TestParticle);

    return 0;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentParticleSystemOldT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "particle system (old) component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentParticleSystemOldT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentParticleSystemOldT();
}

const luaL_Reg ComponentParticleSystemOldT::MethodsList[] =
{
    { "EmitParticle", EmitParticle },
    { "__tostring",   toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentParticleSystemOldT::DocMethods[] =
{
    META_EmitParticle,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentParticleSystemOldT::TypeInfo(GetComponentTIM(), "GameSys::ComponentParticleSystemOldT", "GameSys::ComponentBaseT", ComponentParticleSystemOldT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
