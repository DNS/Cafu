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

#include "../../GameWorld.hpp"
#include "BaseEntity.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "EntityCreateParams.hpp"
#include "Interpolator.hpp"
#include "TypeSys.hpp"
#include "ClipSys/ClipModel.hpp"
#include "ClipSys/CollisionModelMan.hpp"
#include "ConsoleCommands/Console.hpp"
#include "Network/State.hpp"
#include "UniScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace GAME_NAME;


namespace
{
    ConVarT interpolateNPCs("interpolateNPCs", true, ConVarT::FLAG_MAIN_EXE, "Toggles whether the origin of NPCs is interpolated for rendering.");
}


// Note that we cannot simply replace this method with a global TypeInfoManT instance,
// because it is called during global static initialization time. The TIM instance being
// embedded in the function guarantees that it is properly initialized before first use.
cf::TypeSys::TypeInfoManT& GAME_NAME::GetBaseEntTIM()
{
    static cf::TypeSys::TypeInfoManT TIM;

    return TIM;
}


// Base Entity
// ***********

BaseEntityT::BaseEntityT(const EntityCreateParamsT& Params, const BoundingBox3dT& Dimensions, const unsigned int NUM_EVENT_TYPES)
    : ID(Params.ID),
      Properties(Params.Properties),
      ParentID(0xFFFFFFFF),
      m_Entity(Params.Entity),
      GameWorld(Params.GameWorld),
      CollisionModel(NULL),
      ClipModel(GameWorld->GetClipWorld()),  // Creates a clip model in the given clip world with a NULL collision model.

      m_Dimensions(Dimensions),
      m_EventsCount(),
      m_EventsRef(),
      m_Interpolators()
{
    m_EventsCount.PushBackEmptyExact(NUM_EVENT_TYPES);
    m_EventsRef  .PushBackEmptyExact(NUM_EVENT_TYPES);

    for (unsigned int i = 0; i < NUM_EVENT_TYPES; i++)
    {
        m_EventsCount[i] = 0;
        m_EventsRef  [i] = 0;
    }

    ClipModel.SetCollisionModel(CollisionModel);
    ClipModel.SetUserData(this);    // As user data of the clip model, set to pointer back to us, the owner of the clip model (the clip model is the member of "this" entity).
}


BaseEntityT::~BaseEntityT()
{
    for (unsigned int i = 0; i < m_Interpolators.Size(); i++)
        delete m_Interpolators[i];

    ClipModel.SetCollisionModel(NULL);
    ClipModel.SetUserData(NULL);
    cf::ClipSys::CollModelMan->FreeCM(CollisionModel);
}


void BaseEntityT::Register(ApproxBaseT* Interp)
{
    m_Interpolators.PushBack(Interp);
}


void BaseEntityT::Serialize(cf::Network::OutStreamT& Stream) const
{
    Stream << float(m_Dimensions.Min.z);
    Stream << float(m_Dimensions.Max.z);

    for (unsigned int i = 0; i < m_EventsCount.Size(); i++)
        Stream << m_EventsCount[i];

    // Let the derived classes add their own data.
    DoSerialize(Stream);
}


void BaseEntityT::Deserialize(cf::Network::InStreamT& Stream, bool IsIniting)
{
    float f=0.0f;

    Stream >> f; m_Dimensions.Min.z=f;
    Stream >> f; m_Dimensions.Max.z=f;

    for (unsigned int i = 0; i < m_EventsCount.Size(); i++)
        Stream >> m_EventsCount[i];

    // Let the derived classes get their own data.
    DoDeserialize(Stream);

    // Process events.
    // Note that events, as implemented here, are fully predictable:
    // they work well even in the presence of client prediction.
    for (unsigned int i = 0; i < m_EventsCount.Size(); i++)
    {
        // Don't process the events if we got here as part of the
        // construction / first-time initialization of the entity.
        if (!IsIniting && m_EventsCount[i] > m_EventsRef[i])
        {
            ProcessEvent(i, m_EventsCount[i] - m_EventsRef[i]);
        }

        m_EventsRef[i] = m_EventsCount[i];
    }

    // Deserialization has brought new reference values for interpolated values.
    for (unsigned int i = 0; i < m_Interpolators.Size(); i++)
    {
        if (IsIniting || !interpolateNPCs.GetValueBool())
        {
            m_Interpolators[i]->ReInit();
        }
        else
        {
            m_Interpolators[i]->NotifyOverwriteUpdate();
        }
    }
}


void BaseEntityT::NotifyTouchedBy(BaseEntityT* /*Entity*/)
{
}


void BaseEntityT::OnTrigger(BaseEntityT* /*Activator*/)
{
}


void BaseEntityT::TakeDamage(BaseEntityT* /*Entity*/, char /*Amount*/, const VectorT& /*ImpactDir*/)
{
}


void BaseEntityT::Think(float /*FrameTime*/, unsigned long /*ServerFrameNr*/)
{
}


void BaseEntityT::ProcessEvent(unsigned int /*EventType*/, unsigned int /*NumEvents*/)
{
}


bool BaseEntityT::GetLightSourceInfo(unsigned long& /*DiffuseColor*/, unsigned long& /*SpecularColor*/, VectorT& /*Position*/, float& /*Radius*/, bool& /*CastsShadows*/) const
{
    return false;
}


void BaseEntityT::Draw(bool /*FirstPersonView*/, float /*LodDist*/) const
{
}


void BaseEntityT::Interpolate(float FrameTime)
{
    if (interpolateNPCs.GetValueBool())
        for (unsigned int i = 0; i < m_Interpolators.Size(); i++)
            m_Interpolators[i]->Interpolate(FrameTime);
}


void BaseEntityT::PostDraw(float /*FrameTime*/, bool /*FirstPersonView*/)
{
}


const cf::TypeSys::TypeInfoT* BaseEntityT::GetType() const
{
    return &TypeInfo;
 // return &BaseEntityT::TypeInfo;
}


// *********** Scripting related code starts here ****************

int BaseEntityT::GetName(lua_State* LuaState)
{
    cf::ScriptBinderT Binder(LuaState);
    IntrusivePtrT<BaseEntityT> Ent=Binder.GetCheckedObjectParam< IntrusivePtrT<BaseEntityT> >(1);

    lua_pushstring(LuaState, Ent->m_Entity->GetBasics()->GetEntityName().c_str());
    return 1;
}


void* BaseEntityT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    Console->Warning("Cannot instantiate abstract class!\n");
    assert(false);
    return NULL;
}


static const luaL_Reg MethodsList[]=
{
    { "GetName",    BaseEntityT::GetName   },
 // { "__tostring", toString },
    { NULL, NULL }
};


const cf::TypeSys::TypeInfoT BaseEntityT::TypeInfo(GetBaseEntTIM(), "BaseEntityT", NULL /*No base class.*/, BaseEntityT::CreateInstance, MethodsList);
