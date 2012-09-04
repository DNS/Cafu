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

#include "../../BaseEntity.hpp"
#include "../../GameWorld.hpp"
#include "../../Interpolator.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "EntityCreateParams.hpp"
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


namespace
{
    ConVarT interpolateNPCs("interpolateNPCs", true, ConVarT::FLAG_MAIN_EXE, "Toggles whether the origin of NPCs is interpolated for rendering.");
}


// Note that we cannot simply replace this method with a global TypeInfoManT instance,
// because it is called during global static initialization time. The TIM instance being
// embedded in the function guarantees that it is properly initialized before first use.
cf::TypeSys::TypeInfoManT& GetBaseEntTIM()
{
    static cf::TypeSys::TypeInfoManT TIM;

    return TIM;
}


// Entity State
// ************

EntityStateT::EntityStateT(const VectorT& Velocity_,
                           char StateOfExistance_, char Flags_, char ModelIndex_, char ModelSequNr_, float ModelFrameNr_,
                           char Health_, char Armor_, unsigned long HaveItems_, unsigned long HaveWeapons_,
                           char ActiveWeaponSlot_, char ActiveWeaponSequNr_, float ActiveWeaponFrameNr_)
    : Velocity(Velocity_),
      StateOfExistance(StateOfExistance_),
      Flags(Flags_),
   // PlayerName[]
      ModelIndex(ModelIndex_),
      ModelSequNr(ModelSequNr_),
      ModelFrameNr(ModelFrameNr_),

      Health(Health_),
      Armor(Armor_),
      HaveItems(HaveItems_),
      HaveWeapons(HaveWeapons_),
      ActiveWeaponSlot(ActiveWeaponSlot_),
      ActiveWeaponSequNr(ActiveWeaponSequNr_),
      ActiveWeaponFrameNr(ActiveWeaponFrameNr_)
   // HaveAmmo[]
   // HaveAmmoInWeapons[]
{
    PlayerName[0]=0;

    for (int Nr=0; Nr<16; Nr++) HaveAmmo         [Nr]=0;
    for (int Nr=0; Nr<32; Nr++) HaveAmmoInWeapons[Nr]=0;
}


// Base Entity
// ***********

// Unfortunately, the BaseEntityT::GetProp("name", "") method can not yet replace this function,
// due to initialization order in the BaseEntityT ctor below.
// If we had a proper PropDictT class though, we could call Params.Properties.GetProp("name", "").
static std::string ObtainEntityName(const std::map<std::string, std::string>& Properties)
{
    std::map<std::string, std::string>::const_iterator NamePair=Properties.find("name");

    if (NamePair!=Properties.end()) return NamePair->second;

    return "";
}


BaseEntityT::BaseEntityT(const EntityCreateParamsT& Params, const BoundingBox3dT& Dimensions, const unsigned int NUM_EVENT_TYPES)
    : ID(Params.ID),
      Name(ObtainEntityName(Params.Properties)),
      Properties(Params.Properties),
      WorldFileIndex(Params.WorldFileIndex),
      ParentID(0xFFFFFFFF),
      GameWorld(Params.GameWorld),
      CollisionModel(Params.CollisionModel),
      ClipModel(GameWorld->GetClipWorld()),  // Creates a clip model in the given clip world with a NULL collision model.

      m_Origin(Params.Origin),
      m_Dimensions(Dimensions),
      m_Heading(0),
      m_Pitch(0),
      m_Bank(0),
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

    // Evaluate the common 'Properties'.
    std::map<std::string, std::string>::const_iterator It=Properties.find("angles");

    if (It!=Properties.end())
    {
        double d;
        std::istringstream iss(It->second);

        iss >> d; iss >> d;
        m_Heading=(unsigned short)(d*8192.0/45.0);
    }

    if (CollisionModel==NULL)
    {
        // Okay, the mapper did not provide any map primitives (e.g. brushes, bezier patches, etc.) for this entity,
        // meaning that for this entity, there is no "precompiled" collision model that comes with the world file.
        // Now see if the "collisionModel" property has been set, specifying an external file or a parametric description
        // (cube, cylinder, etc.) as the collision model.
        It=Properties.find("collisionModel");

        if (It!=Properties.end() && It->second!="")
        {
            if (It->second=="cube")     // consider:  if (It->second.BeginsWith("cube ")) ...  [params follow in same string].
            {
                // TODO...
                ;
            }
            else if (It->second=="cylinder")
            {
                // TODO...
                ;
            }
            else
            {
                CollisionModel=cf::ClipSys::CollModelMan->GetCM("Games/DeathMatch/"+It->second);
            }
        }
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
    Stream << float(m_Origin.x);
    Stream << float(m_Origin.y);
    Stream << float(m_Origin.z);
    Stream << float(m_Dimensions.Min.z);
    Stream << float(m_Dimensions.Max.z);
    Stream << m_Heading;
    Stream << m_Pitch;
    Stream << m_Bank;

    for (unsigned int i = 0; i < m_EventsCount.Size(); i++)
        Stream << m_EventsCount[i];

    // Let the derived classes add their own data.
    DoSerialize(Stream);
}


void BaseEntityT::Deserialize(cf::Network::InStreamT& Stream, bool IsIniting)
{
    float f=0.0f;

    Stream >> f; m_Origin.x=f;
    Stream >> f; m_Origin.y=f;
    Stream >> f; m_Origin.z=f;
    Stream >> f; m_Dimensions.Min.z=f;
    Stream >> f; m_Dimensions.Max.z=f;
    Stream >> m_Heading;
    Stream >> m_Pitch;
    Stream >> m_Bank;

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


float BaseEntityT::GetProp(const std::string& Key, float Default) const
{
    std::map<std::string, std::string>::const_iterator KeyValue=Properties.find(Key);

    if (KeyValue==Properties.end()) return Default;

    float Value=Default;
    std::istringstream iss(KeyValue->second);

    iss >> Value;

    return Value;
}


double BaseEntityT::GetProp(const std::string& Key, double Default) const
{
    std::map<std::string, std::string>::const_iterator KeyValue=Properties.find(Key);

    if (KeyValue==Properties.end()) return Default;

    double Value=Default;
    std::istringstream iss(KeyValue->second);

    iss >> Value;

    return Value;
}


int BaseEntityT::GetProp(const std::string& Key, int Default) const
{
    std::map<std::string, std::string>::const_iterator KeyValue=Properties.find(Key);

    if (KeyValue==Properties.end()) return Default;

    int Value=Default;
    std::istringstream iss(KeyValue->second);

    iss >> Value;
    return Value;
}


std::string BaseEntityT::GetProp(const std::string& Key, std::string Default) const
{
    std::map<std::string, std::string>::const_iterator KeyValue=Properties.find(Key);

    if (KeyValue==Properties.end()) return Default;

    return KeyValue->second;
}


Vector3fT BaseEntityT::GetProp(const std::string& Key, Vector3fT Default) const
{
    std::map<std::string, std::string>::const_iterator KeyValue=Properties.find(Key);

    if (KeyValue==Properties.end()) return Default;

    Vector3fT Value=Default;
    std::istringstream iss(KeyValue->second);

    iss >> Value.x >> Value.y >> Value.z;

    return Value;
}


Vector3dT BaseEntityT::GetProp(const std::string& Key, Vector3dT Default) const
{
    std::map<std::string, std::string>::const_iterator KeyValue=Properties.find(Key);

    if (KeyValue==Properties.end()) return Default;

    Vector3dT Value=Default;
    std::istringstream iss(KeyValue->second);

    iss >> Value.x >> Value.y >> Value.z;

    return Value;
}


void BaseEntityT::ProcessConfigString(const void* /*ConfigData*/, const char* /*ConfigString*/)
{
}


void BaseEntityT::NotifyTouchedBy(BaseEntityT* /*Entity*/)
{
}


void BaseEntityT::OnTrigger(BaseEntityT* /*Activator*/)
{
}


void BaseEntityT::OnPush(ArrayT<BaseEntityT*>& Pushers, const Vector3dT& PushVector)
{
    // For now, let's just implement this method in the most trivial way.
    m_Origin+=PushVector;

    ClipModel.SetOrigin(m_Origin);
    ClipModel.Register();  // Re-register ourselves with the clip world.
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

    lua_pushstring(LuaState, Ent->Name.c_str());
    return 1;
}


int BaseEntityT::GetOrigin(lua_State* LuaState)
{
    cf::ScriptBinderT Binder(LuaState);
    IntrusivePtrT<BaseEntityT> Ent=Binder.GetCheckedObjectParam< IntrusivePtrT<BaseEntityT> >(1);

    lua_pushnumber(LuaState, Ent->m_Origin.x);
    lua_pushnumber(LuaState, Ent->m_Origin.y);
    lua_pushnumber(LuaState, Ent->m_Origin.z);

    return 3;
}


int BaseEntityT::SetOrigin(lua_State* LuaState)
{
    cf::ScriptBinderT Binder(LuaState);
    IntrusivePtrT<BaseEntityT> Ent=Binder.GetCheckedObjectParam< IntrusivePtrT<BaseEntityT> >(1);

    const double Ox=luaL_checknumber(LuaState, 2);
    const double Oy=luaL_checknumber(LuaState, 3);
    const double Oz=luaL_checknumber(LuaState, 4);

    Ent->m_Origin=Vector3dT(Ox, Oy, Oz);

    return 0;
}


void* BaseEntityT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    Console->Warning("Cannot instantiate abstract class!\n");
    assert(false);
    return NULL;
}


static const luaL_reg MethodsList[]=
{
    { "GetName",    BaseEntityT::GetName   },
    { "GetOrigin",  BaseEntityT::GetOrigin },
    { "SetOrigin",  BaseEntityT::SetOrigin },
 // { "__tostring", toString },
    { NULL, NULL }
};


const cf::TypeSys::TypeInfoT BaseEntityT::TypeInfo(GetBaseEntTIM(), "BaseEntityT", NULL /*No base class.*/, BaseEntityT::CreateInstance, MethodsList);


unsigned long BaseEntityT::GetTypeNr() const
{
    return GetType()->TypeNr;
}
