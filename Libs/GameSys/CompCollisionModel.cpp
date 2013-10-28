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

#include "CompCollisionModel.hpp"
#include "AllComponents.hpp"
#include "Entity.hpp"
#include "World.hpp"

#include "ClipSys/ClipModel.hpp"
#include "ClipSys/CollisionModelMan.hpp"
#include "ClipSys/CollisionModel_static.hpp"    // Only needed for ScaleDown254()
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
    { "Name", "The file name of the collision model." },
    { NULL, NULL }
};


ComponentCollisionModelT::ComponentCollisionModelT()
    : ComponentBaseT(),
      m_CollMdlName("Name", "", FlagsIsFileName),
      m_PrevName(""),
      m_CollisionModel(NULL),
      m_ClipModel(NULL),
      m_ClipPrevOrigin(),
      m_ClipPrevQuat()
{
    GetMemberVars().Add(&m_CollMdlName);
}


ComponentCollisionModelT::ComponentCollisionModelT(const ComponentCollisionModelT& Comp)
    : ComponentBaseT(Comp),
      m_CollMdlName(Comp.m_CollMdlName),
      m_PrevName(""),
      m_CollisionModel(NULL),
      m_ClipModel(NULL),
      m_ClipPrevOrigin(),
      m_ClipPrevQuat()
{
    GetMemberVars().Add(&m_CollMdlName);
}


ComponentCollisionModelT::~ComponentCollisionModelT()
{
    CleanUp();
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


void ComponentCollisionModelT::DoServerFrame(float t)
{
    // TODO:
    // This should actually be in some PostThink() method, so that we can be sure that
    // all behaviour and physics scripts (that possibly alter the origin and orientation)
    // have already been run when we update the clip model.
    UpdateClipModel();
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
    const Vector3fT              o = GetEntity()->GetTransform()->GetOrigin();
    const cf::math::QuaternionfT q = GetEntity()->GetTransform()->GetQuat();

    if (IsNewClipModel || o != m_ClipPrevOrigin || q != m_ClipPrevQuat)
    {
        m_ClipModel->SetOrigin(o.AsVectorOfDouble());
        m_ClipModel->SetOrientation(cf::math::Matrix3x3dT(cf::math::QuaterniondT(q.x, q.y, q.z, q.w)));
        m_ClipModel->Register();

        m_ClipPrevOrigin = o;
        m_ClipPrevQuat   = q;
    }
}


void ComponentCollisionModelT::DoDeserialize(cf::Network::InStreamT& Stream, bool IsIniting)
{
    // Deserialization may have updated our origin or orientation,
    // so we have to update the clip model.
    UpdateClipModel();
}


static const cf::TypeSys::MethsDocT META_SetBoundingBox =
{
    "SetBoundingBox",
    "Sets the given bounding-box as the collision model.\n"
    "Instead of loading a collision model from a file, a script can call this method\n"
    "to set a bounding-box with the given dimensions as the collision model.",
    "", "(number min_x, number min_y, number min_z, number max_x, number max_y, number max_z)"
};

int ComponentCollisionModelT::SetBoundingBox(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentCollisionModelT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentCollisionModelT> >(1);

    if (!Comp->GetEntity())
        luaL_error(LuaState, "The component must be added to an entity before this function can be called.");

    // Have to take the detour by 25.4, because the CollisionModelStaticT ctor uses epsilon values
    // (MapT::RoundEpsilon and MapT::MinVertexDist) that are still coined to the millimeters worlds...
    const double CA3DE_SCALE = 25.4;

    const BoundingBox3dT BB(
        Vector3dT(luaL_checknumber(LuaState, 2), luaL_checknumber(LuaState, 3), luaL_checknumber(LuaState, 4)) * CA3DE_SCALE,
        Vector3dT(luaL_checknumber(LuaState, 5), luaL_checknumber(LuaState, 6), luaL_checknumber(LuaState, 7)) * CA3DE_SCALE);

    MaterialT* Mat = MaterialManager->GetMaterial(luaL_checkstring(LuaState, 8));

    Comp->CleanUp();
    Comp->m_CollisionModel = Comp->GetEntity()->GetWorld().GetCollModelMan().GetCM(BB, Mat);

    // These lines can be removed, as soon as the 25.4 scaling above is removed. See the related commit of 2013-10-28 for details.
    cf::ClipSys::CollisionModelStaticT* CMS = dynamic_cast<cf::ClipSys::CollisionModelStaticT*>(const_cast<cf::ClipSys::CollisionModelT*>(Comp->m_CollisionModel));
    assert(CMS);
    CMS->ScaleDown254();

    Comp->m_CollMdlName.Set("bounding-box");
    Comp->m_PrevName = Comp->m_CollMdlName.Get();

    Comp->UpdateClipModel();
    return 0;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__toString",
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
    { "SetBoundingBox", ComponentCollisionModelT::SetBoundingBox },
    { "__tostring",     ComponentCollisionModelT::toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentCollisionModelT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentCollisionModelT::TypeInfo(GetComponentTIM(), "ComponentCollisionModelT", "ComponentBaseT", ComponentCollisionModelT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
