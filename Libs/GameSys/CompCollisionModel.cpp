/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompCollisionModel.hpp"
#include "AllComponents.hpp"
#include "CompScript.hpp"
#include "Entity.hpp"
#include "World.hpp"

#include "ClipSys/ClipModel.hpp"
#include "ClipSys/ClipWorld.hpp"
#include "ClipSys/CollisionModelMan.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


namespace
{
    const char* FlagsIsFileName[] = { "IsGenericFileName", NULL };
}


const char* ComponentCollisionModelT::DocClass =
    "This component adds a collision model to its entity.";


const cf::TypeSys::VarsDocT ComponentCollisionModelT::DocVars[] =
{
    { "Name",         "The file name of the collision model." },
    { "IgnoreOrient", "If true, the orientation of the entity does not affect the orientation of the collision model.\n"
                      "This is used with players, monsters and other NPCs whose world-space collision model must not\n"
                      "change when they rotate (in order to not get them accidentally stuck in nearby walls)." },
    { NULL, NULL }
};


ComponentCollisionModelT::ComponentCollisionModelT()
    : ComponentBaseT(),
      m_CollMdlName("Name", "", FlagsIsFileName),
      m_PrevName(""),
      m_CollisionModel(NULL),
      m_IgnoreOrient("IgnoreOrient", false),
      m_ClipModel(NULL),
      m_ClipPrevOrigin(),
      m_ClipPrevQuat()
{
    GetMemberVars().Add(&m_CollMdlName);
    GetMemberVars().Add(&m_IgnoreOrient);
}


ComponentCollisionModelT::ComponentCollisionModelT(const ComponentCollisionModelT& Comp)
    : ComponentBaseT(Comp),
      m_CollMdlName(Comp.m_CollMdlName),
      m_PrevName(""),
      m_CollisionModel(NULL),
      m_IgnoreOrient(Comp.m_IgnoreOrient),
      m_ClipModel(NULL),
      m_ClipPrevOrigin(),
      m_ClipPrevQuat()
{
    GetMemberVars().Add(&m_CollMdlName);
    GetMemberVars().Add(&m_IgnoreOrient);
}


ComponentCollisionModelT::~ComponentCollisionModelT()
{
    CleanUp();
}


void ComponentCollisionModelT::SetBoundingBox(const BoundingBox3dT& BB, const char* MatName)
{
    if (!GetEntity()) return;

    MaterialT* Mat = MaterialManager->GetMaterial(MatName);

    CleanUp();
    m_CollisionModel = GetEntity()->GetWorld().GetCollModelMan().GetCM(BB, Mat);

    m_CollMdlName.Set("bounding-box");
    m_PrevName = m_CollMdlName.Get();

    UpdateClipModel();
}


ComponentCollisionModelT* ComponentCollisionModelT::Clone() const
{
    return new ComponentCollisionModelT(*this);
}


void ComponentCollisionModelT::UpdateDependencies(EntityT* Entity)
{
    if (GetEntity() != Entity)
        CleanUp();

    ComponentBaseT::UpdateDependencies(Entity);

    UpdateClipModel();
}


void ComponentCollisionModelT::DoDeserialize(cf::Network::InStreamT& Stream, bool IsIniting)
{
    // Deserialization may have updated our origin or orientation,
    // so we have to update the clip model.
    UpdateClipModel();
}


void ComponentCollisionModelT::DoServerFrame(float t)
{
    // TODO:
    // This should actually be in some PostThink() method, so that we can be sure that
    // all behaviour and physics scripts (that possibly alter the origin and orientation)
    // have already been run when we update the clip model.
    // (Same is true for the clip model in the CompGameEntityT class.)
    UpdateClipModel();


    // See if we walked into the trigger volume of any entity.
    //
    // For the user (mapper), it looks as if trigger entities call script methods whenever something walks into their
    // (trigger) brushes, but in truth, the roles are reversed, and it is this code that checks if the entity walked
    // itself into a trigger volume, and then calls the `OnTrigger()` script callback. Trigger entities are therefore
    // really quite passive, but thinking of them the other way round (triggers are actively checking their volumes
    // and activate the calls) is probably more suggestive to users (script and map designers).
    //
    // Note that the mapper is free to put trigger brushes into *any* entity -- the functionality is only dependent on
    // the presence of a trigger volume in the clip world (contributed directly via brushwork in the map, or via a
    // ComponentCollisionModelT with appropriate materials), and an associated ComponentScriptT to implement the
    // OnTrigger() callback.
    //
    // Note that this code could easily be elsewhere, e.g. in a different component or even in its own component.
    // It is here only because it too "uses" the clip world, and because we can conveniently obtain the required
    // `AbsBB` from the readily available `m_ClipModel`.
    if (m_ClipModel)
    {
        const BoundingBox3dT AbsBB(m_ClipModel->GetAbsoluteBB());
        const double         Radius     = (AbsBB.Max.x - AbsBB.Min.x) / 2.0;
        const double         HalfHeight = (AbsBB.Max.z - AbsBB.Min.z) / 2.0;
        const Vector3dT      Test1      = AbsBB.GetCenter() + Vector3dT(0, 0, HalfHeight - Radius);
        const Vector3dT      Test2      = AbsBB.GetCenter() - Vector3dT(0, 0, HalfHeight - Radius);

        ArrayT<cf::ClipSys::ClipModelT*> ClipModels;
        GetEntity()->GetWorld().GetClipWorld()->GetClipModelsFromBB(ClipModels, MaterialT::Clip_Trigger, AbsBB);

        for (unsigned long ClipModelNr = 0; ClipModelNr < ClipModels.Size(); ClipModelNr++)
        {
            if (ClipModels[ClipModelNr]->GetContents(Test1, Radius, MaterialT::Clip_Trigger) == 0 &&
                ClipModels[ClipModelNr]->GetContents(Test2, Radius, MaterialT::Clip_Trigger) == 0) continue;

            // TODO: if (another clip model already triggered Owner's entity) continue;
            //       *or* pass ClipModels[ClipModelNr] as another parameter to ScriptComp:OnTrigger().

            ComponentBaseT* Owner = ClipModels[ClipModelNr]->GetOwner();
            if (Owner == NULL) continue;
            if (Owner == this) continue;

            EntityT* Ent = Owner->GetEntity();
            if (Ent == NULL) continue;

            IntrusivePtrT<ComponentScriptT> ScriptComp = dynamic_pointer_cast<ComponentScriptT>(Ent->GetComponent("Script"));
            if (ScriptComp == NULL) continue;

            UniScriptStateT& ScriptState = Ent->GetWorld().GetScriptState();
            lua_State*       LuaState    = ScriptState.GetLuaState();
            ScriptBinderT    Binder(LuaState);

            Binder.Push(IntrusivePtrT<EntityT>(GetEntity()));   // Don't pass the raw `EntityT*` pointer!

            ScriptComp->CallLuaMethod("OnTrigger", 1);
        }
    }
}


void ComponentCollisionModelT::CleanUp()
{
    if (!GetEntity())
    {
        assert(m_CollisionModel == NULL);
        assert(m_ClipModel == NULL);
        assert(m_PrevName == "");
        return;
    }

    delete m_ClipModel;
    m_ClipModel = NULL;

    GetEntity()->GetWorld().GetCollModelMan().FreeCM(m_CollisionModel);
    m_CollisionModel = NULL;

    // If m_CollMdlName.Get() != "", make sure that another attempt is made
    // (e.g. when GetEntity() becomes non-NULL in the future) to establish m_CollisionModel again.
    m_PrevName = "";
}


void ComponentCollisionModelT::UpdateClipModel()
{
    if (!GetEntity()) return;
    if (!GetEntity()->GetWorld().GetClipWorld()) return;

    if (m_CollMdlName.Get() != m_PrevName)
    {
        CleanUp();

        // `GetEntity() != NULL` is checked above already.
        if (/*GetEntity() &&*/ m_CollMdlName.Get() != "")
            m_CollisionModel = GetEntity()->GetWorld().GetCollModelMan().GetCM(m_CollMdlName.Get());

        m_PrevName = m_CollMdlName.Get();
    }

    if (!m_CollisionModel) return;

    const bool IsNewClipModel = (m_ClipModel == NULL);

    if (!m_ClipModel)
    {
        m_ClipModel = new cf::ClipSys::ClipModelT(*GetEntity()->GetWorld().GetClipWorld());

        m_ClipModel->SetCollisionModel(m_CollisionModel);
        m_ClipModel->SetOwner(this);
    }

    // Has the origin or orientation changed since we last registered clip model? If so, re-register!
    const Vector3fT              o = GetEntity()->GetTransform()->GetOriginWS();
    const cf::math::QuaternionfT q = m_IgnoreOrient.Get() ? cf::math::QuaternionfT() : GetEntity()->GetTransform()->GetQuatWS();

    if (IsNewClipModel || o != m_ClipPrevOrigin || q != m_ClipPrevQuat)
    {
        m_ClipModel->SetOrigin(o.AsVectorOfDouble());
        m_ClipModel->SetOrientation(m_IgnoreOrient.Get() ? cf::math::Matrix3x3dT() : cf::math::Matrix3x3dT(cf::math::QuaterniondT(q.x, q.y, q.z, q.w)));
        m_ClipModel->Register();

        m_ClipPrevOrigin = o;
        m_ClipPrevQuat   = q;
    }
}


static const cf::TypeSys::MethsDocT META_SetBoundingBox =
{
    "SetBoundingBox",
    "Sets the given bounding-box as the collision model.\n"
    "Instead of loading a collision model from a file, a script can call this method\n"
    "to set a bounding-box with the given dimensions as the collision model.",
    "", "(number min_x, number min_y, number min_z, number max_x, number max_y, number max_z, string MatName)"
};

int ComponentCollisionModelT::SetBoundingBox(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentCollisionModelT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentCollisionModelT> >(1);

    if (!Comp->GetEntity())
        luaL_error(LuaState, "The component must be added to an entity before this function can be called.");

    const BoundingBox3dT BB(
        Vector3dT(luaL_checknumber(LuaState, 2), luaL_checknumber(LuaState, 3), luaL_checknumber(LuaState, 4)),
        Vector3dT(luaL_checknumber(LuaState, 5), luaL_checknumber(LuaState, 6), luaL_checknumber(LuaState, 7)));

    Comp->SetBoundingBox(BB, luaL_checkstring(LuaState, 8));
    return 0;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentCollisionModelT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "collision model component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentCollisionModelT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentCollisionModelT();
}

const luaL_Reg ComponentCollisionModelT::MethodsList[] =
{
    { "SetBoundingBox", SetBoundingBox },
    { "__tostring",     toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentCollisionModelT::DocMethods[] =
{
    META_SetBoundingBox,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentCollisionModelT::TypeInfo(GetComponentTIM(), "GameSys::ComponentCollisionModelT", "GameSys::ComponentBaseT", ComponentCollisionModelT::CreateInstance, MethodsList, DocClass, DocMethods, NULL, DocVars);
