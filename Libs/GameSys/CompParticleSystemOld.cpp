/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
}


ComponentParticleSystemOldT::ComponentParticleSystemOldT(const ComponentParticleSystemOldT& Comp)
    : ComponentBaseT(Comp),
      m_Type(Comp.m_Type)
{
    GetMemberVars().Add(&m_Type);
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
    if (m_RenderMats.Size() > 0) return;    // Already inited?

    char ParticleName[256];

    if (m_Type.Get() == "Rocket_Expl_main")
    {
        for (unsigned int FrameNr = 0; FrameNr < 26; FrameNr++)
        {
            sprintf(ParticleName, "Sprites/expl1/expl_%02u", FrameNr+1);
            m_RenderMats.PushBack(MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial(ParticleName)));
        }
    }
    else if (m_Type.Get() == "ARGrenade_Expl_main")
    {
        for (unsigned int FrameNr = 0; FrameNr < 55; FrameNr++)
        {
            sprintf(ParticleName, "Sprites/expl4/expl_%02u", FrameNr+1);
            m_RenderMats.PushBack(MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial(ParticleName)));
        }
    }
    else if (m_Type.Get() == "Z")
    {
        for (unsigned int FrameNr = 0; FrameNr < 32; FrameNr++)
        {
            sprintf(ParticleName, "Sprites/smoke1/smoke_%02u", FrameNr+1);
            m_RenderMats.PushBack(MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial(ParticleName)));
        }
    }
    else if (m_Type.Get() == "HandGrenade_Expl_main")
    {
        for (unsigned int FrameNr = 0; FrameNr < 27; FrameNr++)
        {
            sprintf(ParticleName, "Sprites/expl3/expl_%02u", FrameNr+1);
            m_RenderMats.PushBack(MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial(ParticleName)));
        }
    }
    else  // FaceHugger, ARGrenade_Expl_sparkle, HandGrenade_Expl_sparkle, Rocket_Expl_sparkle, ...
    {
        m_RenderMats.PushBack(MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("Sprites/Generic1")));
    }
}


namespace
{
    bool ParticleFunction_ARGrenadeExplMain(ParticleMST* Particle, float Time)
    {
        const float FPS    = 20.0f;       // The default value is 20.0.
        const float MaxAge = 55.0f/FPS;   // 55 frames at 20 FPS.

        const unsigned long MatNr = (unsigned long)(Particle->Age * FPS);
        assert(MatNr < Particle->AllRMs->Size());
        Particle->RenderMat = (*Particle->AllRMs)[MatNr];

        Particle->Age += Time;
        if (Particle->Age >= MaxAge) return false;

        return true;
    }


    bool ParticleFunction_ARGrenadeExplSparkle(ParticleMST* Particle, float Time)
    {
        const float MaxAge = 3.0f;

        Particle->Age += Time;
        if (Particle->Age > MaxAge) return false;

        Particle->Velocity[0] -= Particle->Velocity[0]*Time;  // Physically, this line is (mostly) nonsense.
        Particle->Velocity[1] -= Particle->Velocity[1]*Time;  // Physically, this line is (mostly) nonsense.
        Particle->Velocity[2] -= 392.4f*Time;                 // v = a*t    9810.0 / 25.0 == 392.4

        Particle->Origin[0] += Particle->Velocity[0]*Time;    // s=v*t
        Particle->Origin[1] += Particle->Velocity[1]*Time;
        Particle->Origin[2] += Particle->Velocity[2]*Time;

        if (Particle->Origin[2] < Particle->AuxData[0])
        {
            // Particle hit the ground.
            Particle->Origin[2] = Particle->AuxData[0];
            Particle->Velocity[2] = -Particle->Velocity[2]*0.5f;
        }

        Particle->Color[0] = char(255.0f*(MaxAge - Particle->Age)/MaxAge);
        Particle->Color[1] = char(255.0f*(MaxAge - Particle->Age)/MaxAge*(MaxAge - Particle->Age)/MaxAge);
        return true;
    }


    bool ParticleFunction_HandGrenadeExplMain(ParticleMST* Particle, float Time)
    {
        const float FPS    = 18.0f;     // The default value is 20.0.
        const float MaxAge = 27.0f/FPS; // 27 frames at 18 FPS.

        const unsigned long MatNr = (unsigned long)(Particle->Age * FPS);
        assert(MatNr < Particle->AllRMs->Size());
        Particle->RenderMat = (*Particle->AllRMs)[MatNr];

        Particle->Age += Time;
        if (Particle->Age >= MaxAge) return false;

        return true;
    }


    bool ParticleFunction_HandGrenadeExplSparkle(ParticleMST* Particle, float Time)
    {
        const float MaxAge = 0.7f;

        Particle->Age += Time;
        if (Particle->Age > MaxAge) return false;

        Particle->Origin[0] += Particle->Velocity[0]*Time;    // s=v*t
        Particle->Origin[1] += Particle->Velocity[1]*Time;
        Particle->Origin[2] += Particle->Velocity[2]*Time;

        Particle->Velocity[0]*=0.98f;   // TODO: Deceleration should depend on 'Time'...
        Particle->Velocity[1]*=0.98f;
        Particle->Velocity[2]-=2.0f*392.4f*Time;     // double gravity...

        Particle->Color[0] = char(255.0f*(MaxAge - Particle->Age)/MaxAge);
        Particle->Color[1] = char(255.0f*(MaxAge - Particle->Age)/MaxAge*(MaxAge - Particle->Age)/MaxAge);
        return true;
    }


    bool ParticleFunction_RocketExplMain(ParticleMST* Particle, float Time)
    {
        const float FPS    = 15.0f;     // The default value is 20.0.
        const float MaxAge = 26.0f/FPS; // 26 frames at 15 FPS.

        const unsigned long MatNr = (unsigned long)(Particle->Age * FPS);
        assert(MatNr < Particle->AllRMs->Size());
        Particle->RenderMat = (*Particle->AllRMs)[MatNr];

        Particle->Age += Time;
        if (Particle->Age >= MaxAge) return false;

        return true;
    }


    bool ParticleFunction_RocketExplSparkle(ParticleMST* Particle, float Time)
    {
        const float MaxAge = 0.5f;

        Particle->Age += Time;
        if (Particle->Age > MaxAge) return false;

        Particle->Origin[0] += Particle->Velocity[0]*Time;    // s=v*t
        Particle->Origin[1] += Particle->Velocity[1]*Time;
        Particle->Origin[2] += Particle->Velocity[2]*Time;

     // Particle->Velocity[0] *= 0.99;  // TODO: Deceleration should depend on 'Time'...
     // Particle->Velocity[1] *= 0.99;
     // Particle->Velocity[2] *= 0.90;

        Particle->Color[0] = char(255.0f*(MaxAge - Particle->Age)/MaxAge);
        Particle->Color[1] = char(255.0f*(MaxAge - Particle->Age)/MaxAge*(MaxAge - Particle->Age)/MaxAge);
        return true;
    }


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

    Comp->InitRenderMats();

    const Vector3fT& Origin = Comp->GetEntity()->GetTransform()->GetOriginWS();

    // Register a new particle.
    static ParticleMST P;

    P.Age       = 0.0;
    P.Rotation  = 0;
    P.StretchY  = 1.0;
    P.AllRMs    = &Comp->m_RenderMats;
    P.RenderMat = Comp->m_RenderMats[0];

    if (Comp->m_Type.Get() == "ARGrenade_Expl_main")
    {
        P.Origin[0] = Origin.x;
        P.Origin[1] = Origin.y;
        P.Origin[2] = Origin.z + 80.0f - 20.0f;

        P.Color[0] = 255;
        P.Color[1] = 255;
        P.Color[2] = 255;
        P.Color[3] = 255;

        P.Radius = 40.0;
        P.StretchY  = 2.0;
        P.MoveFunction = ParticleFunction_ARGrenadeExplMain;
    }
    else if (Comp->m_Type.Get() == "ARGrenade_Expl_sparkle")
    {
        P.Origin[0] = Origin.x;
        P.Origin[1] = Origin.y;
        P.Origin[2] = Origin.z;

        P.Velocity[0] = (rand()-int(RAND_MAX/2))/16.0f / 25.0f;
        P.Velocity[1] = (rand()-int(RAND_MAX/2))/16.0f / 25.0f;
        P.Velocity[2] = rand()/4.0f / 25.0f;

        P.Color[0] = 255;
        P.Color[1] = 255;
        P.Color[2] = 0;
        P.Color[3] = 0;

        P.Radius = 4.0;
        P.MoveFunction = ParticleFunction_ARGrenadeExplSparkle;
        P.AuxData[0] = Origin.z;
    }
    else if (Comp->m_Type.Get() == "HandGrenade_Expl_main")
    {
        P.Origin[0] = Origin.x;
        P.Origin[1] = Origin.y;
        P.Origin[2] = Origin.z + 8.0f;

        P.Color[0] = 255;
        P.Color[1] = 255;
        P.Color[2] = 255;
        P.Color[3] = 0;

        P.Radius = 60.0;
        P.Rotation = char(rand());
        P.MoveFunction = ParticleFunction_HandGrenadeExplMain;
    }
    else if (Comp->m_Type.Get() == "HandGrenade_Expl_sparkle")
    {
        P.Origin[0] = Origin.x;
        P.Origin[1] = Origin.y;
        P.Origin[2] = Origin.z;

        P.Velocity[0] = float(rand()-int(RAND_MAX/2)) / 25.0f;
        P.Velocity[1] = float(rand()-int(RAND_MAX/2)) / 25.0f;
        P.Velocity[2] = float(rand()) / 25.0f;

        P.Color[0] = 255;
        P.Color[1] = 255;
        P.Color[2] = 0;
        P.Color[3] = 0;

        P.Radius = 12.0;
        P.MoveFunction = ParticleFunction_HandGrenadeExplSparkle;
    }
    else if (Comp->m_Type.Get() == "Rocket_Expl_main")
    {
        P.Origin[0] = Origin.x;
        P.Origin[1] = Origin.y;
        P.Origin[2] = Origin.z;

        P.Color[0] = 255;
        P.Color[1] = 255;
        P.Color[2] = 255;
        P.Color[3] = 255;

        P.Radius = 80.0;
        P.Rotation = char(rand());
        P.MoveFunction = ParticleFunction_RocketExplMain;
    }
    else if (Comp->m_Type.Get() == "Rocket_Expl_sparkle")
    {
        P.Origin[0] = Origin.x;
        P.Origin[1] = Origin.y;
        P.Origin[2] = Origin.z;

        P.Velocity[0] = float(rand()-int(RAND_MAX/2)) / 25.0f;
        P.Velocity[1] = float(rand()-int(RAND_MAX/2)) / 25.0f;
        P.Velocity[2] = float(rand()-int(RAND_MAX/2)) / 25.0f;

        P.Color[0] = 255;
        P.Color[1] = 255;
        P.Color[2] = 0;
        P.Color[3] = 0;     // Additive blending is used for this particle.

        P.Radius = 8.0;
        P.MoveFunction = ParticleFunction_RocketExplSparkle;
    }
    else  // "FaceHugger"
    {
        P.Origin[0] = Origin.x;
        P.Origin[1] = Origin.y;
        P.Origin[2] = Origin.z + 8.0f;

        P.Velocity[0] = 0;
        P.Velocity[1] = 0;
        P.Velocity[2] = 0;

        P.Color[0] = 255;
        P.Color[1] = 255;
        P.Color[2] = 255;
        P.Color[3] = 255;

        P.Radius       = 12.0;
        P.MoveFunction = ParticleMove_FaceHugger;
    }

    ParticleEngineMS::RegisterNewParticle(P);
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

const cf::TypeSys::TypeInfoT ComponentParticleSystemOldT::TypeInfo(GetComponentTIM(), "GameSys::ComponentParticleSystemOldT", "GameSys::ComponentBaseT", ComponentParticleSystemOldT::CreateInstance, MethodsList, DocClass, DocMethods, NULL, DocVars);
