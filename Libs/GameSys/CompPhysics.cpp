/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompPhysics.hpp"
#include "AllComponents.hpp"
#include "Entity.hpp"
#include "World.hpp"

#include "ClipSys/ClipModel.hpp"
#include "ClipSys/CollisionModel_base.hpp"
#include "PhysicsWorld.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


const char* ComponentPhysicsT::DocClass =
    "This component includes the body of this entity in the dynamic simulation of physics.\n"
    "\n"
    "Without this component, the entity is either *static* (it doesn't move at all), *kinematic*\n"
    "(it is moved by script or program code), or it doesn't participate in physics computations\n"
    "at all.\n"
    "\n"
    "With this component, the entity's body is subject to gravity, impulses, and generally to\n"
    "the dynamic simulation of physics effects in the game world.";


const cf::TypeSys::VarsDocT ComponentPhysicsT::DocVars[] =
{
    { "Mass", "The mass of the entity's body, in kilograms [kg]." },
    { NULL, NULL }
};


ComponentPhysicsT::ComponentPhysicsT()
    : ComponentBaseT(),
      m_Mass("Mass", 0.0f),
      m_CollisionShape(NULL),
      m_RigidBody(NULL)
{
    GetMemberVars().Add(&m_Mass);
}


ComponentPhysicsT::ComponentPhysicsT(const ComponentPhysicsT& Comp)
    : ComponentBaseT(Comp),
      m_Mass(Comp.m_Mass),
      m_CollisionShape(NULL),
      m_RigidBody(NULL)
{
    GetMemberVars().Add(&m_Mass);
}


ComponentPhysicsT::~ComponentPhysicsT()
{
    if (m_RigidBody)
    {
        assert(GetEntity()->GetWorld().GetPhysicsWorld());
        GetEntity()->GetWorld().GetPhysicsWorld()->RemoveRigidBody(m_RigidBody);
    }

    delete m_RigidBody;
    m_RigidBody = NULL;

    delete m_CollisionShape;
    m_CollisionShape = NULL;
}


ComponentPhysicsT* ComponentPhysicsT::Clone() const
{
    return new ComponentPhysicsT(*this);
}


namespace
{
    void InsertCollisionBB(BoundingBox3fT& BB, IntrusivePtrT<ComponentBaseT> Comp)
    {
        if (Comp == NULL) return;
        if (!Comp->GetClipModel()) return;
        if (!Comp->GetClipModel()->GetCollisionModel()) return;

        assert(Comp->GetClipModel()->GetCollisionModel()->GetBoundingBox().IsInited());

        BB += Comp->GetClipModel()->GetCollisionModel()->GetBoundingBox().AsBoxOfFloat();
    }
}


void ComponentPhysicsT::UpdateDependencies(EntityT* Entity)
{
    if (GetEntity() != Entity)
    {
        if (m_RigidBody)
        {
            assert(GetEntity()->GetWorld().GetPhysicsWorld());
            GetEntity()->GetWorld().GetPhysicsWorld()->RemoveRigidBody(m_RigidBody);
        }

        delete m_RigidBody;
        m_RigidBody = NULL;

        delete m_CollisionShape;
        m_CollisionShape = NULL;
    }

    ComponentBaseT::UpdateDependencies(Entity);

    if (!m_CollisionShape || !m_RigidBody)
    {
        assert(!m_CollisionShape && !m_RigidBody);    // It's all or nothing.

        if (m_Mass.Get() <= 0.0f) return;             // TODO: Should we instead create a kinematic or static object here??
        if (!GetEntity()) return;
        if (!GetEntity()->GetWorld().GetPhysicsWorld()) return;
        if (GetEntity()->GetApp().IsNull()) return;   // Skip this if the "app" component has not been assigned yet!
        if (GetEntity()->GetID() == 0) return;        // The world itself should really not have dynamic physics.

        BoundingBox3fT BB;
        const ArrayT< IntrusivePtrT<ComponentBaseT> >& Components = GetEntity()->GetComponents();

        InsertCollisionBB(BB, GetEntity()->GetApp());

        for (unsigned int CompNr = 0; CompNr < Components.Size(); CompNr++)
            InsertCollisionBB(BB, Components[CompNr]);

        if (!BB.IsInited()) return;

        const Vector3fT HalfExtents = BB.Max - BB.GetCenter();

        // No matter if on client or server side: we always initialize the m_CollisionShape on both.
        // It seems that Bullet doesn't really support moving *concave* shapes:
        //   - No inertia is computed for them (see implementations of btConcaveShape::calculateLocalInertia()).
        //   - For collision detection, only the gimpact algorithm is available, see section "Collision Matrix" in Bullet_User_Manual.pdf.
        // The division by 1000.0 is because our physics world unit is meters.
        m_CollisionShape = new btBoxShape(conv(UnitsToPhys(HalfExtents)));

        btVector3 Inertia;
        m_CollisionShape->calculateLocalInertia(m_Mass.Get(), Inertia);

        m_RigidBody = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(
            m_Mass.Get(), this /*btMotionState for this body*/, m_CollisionShape, Inertia));

        m_RigidBody->setUserPointer(this);      // Associate the m_RigidBody and this component to each other.

        // TODO: Client-side rigid bodies should probably be "kinematic", not "dynamic", because the client doesn't actively simulate our bodies.
        //       From the pdf user manual: "every simulation frame, dynamics world will get new world transform using btMotionState::getWorldTransform"
        GetEntity()->GetWorld().GetPhysicsWorld()->AddRigidBody(m_RigidBody);

        // m_RigidBody->setGravity(btVector3(0, 0, 0));    // for debugging; reset gravity via map script commands
    }
}


void ComponentPhysicsT::getWorldTransform(btTransform& worldTrans) const
{
    assert(GetEntity());

    // Return the initial transformation of our rigid body to the physics world.
    worldTrans.setIdentity();
    worldTrans.setOrigin(conv(UnitsToPhys(GetEntity()->GetTransform()->GetOriginWS())));
    worldTrans.setRotation(conv(GetEntity()->GetTransform()->GetQuatWS()));
}


void ComponentPhysicsT::setWorldTransform(const btTransform& worldTrans)
{
    assert(m_RigidBody);
    assert(GetEntity());

    if (!m_RigidBody->isActive())
    {
        // See my post at http://www.bulletphysics.com/Bullet/phpBB3/viewtopic.php?t=2256 for details...
        return;
    }

    // Update the transformation of our entity according to the physics world results.
    GetEntity()->GetTransform()->SetOriginWS(PhysToUnits(conv(worldTrans.getOrigin())));
    GetEntity()->GetTransform()->SetQuatWS(conv(worldTrans.getRotation()));
}


static const cf::TypeSys::MethsDocT META_ApplyImpulse =
{
    "ApplyImpulse",
    "This method applies an impulse at the center of the entity's body.\n"
    "The impulse is applied at the center of the body, so that it changes the body's\n"
    "linear velocity, but not its torque.",
    "", "(Vector3T Impulse)"
};

static const cf::TypeSys::MethsDocT META_ApplyImpulse_overload =
{
    "ApplyImpulse",
    "This method applies an off-center impulse to the entity's body.\n"
    "The impulse is applied at the center of the body, offset by `rel_pos`,\n"
    "changing the linear velocity and the body's torque appropriately.",
    "", "(Vector3T Impulse, Vector3T rel_pos)"
};

int ComponentPhysicsT::ApplyImpulse(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentPhysicsT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentPhysicsT> >(1);

    if (!Comp->m_RigidBody)
        luaL_error(LuaState, "The component does not have a rigid body allocated yet.");

    luaL_argcheck(LuaState, lua_istable(LuaState, 2), 2, "Expected a Vector3T (a table).");

    btVector3 Impulse;

    lua_rawgeti(LuaState, 2, 1); Impulse.setX(float(lua_tonumber(LuaState, -1))); lua_pop(LuaState, 1);
    lua_rawgeti(LuaState, 2, 2); Impulse.setY(float(lua_tonumber(LuaState, -1))); lua_pop(LuaState, 1);
    lua_rawgeti(LuaState, 2, 3); Impulse.setZ(float(lua_tonumber(LuaState, -1))); lua_pop(LuaState, 1);

    if (lua_gettop(LuaState) == 2)
    {
        Comp->m_RigidBody->applyCentralImpulse(Impulse);
        Comp->m_RigidBody->activate();
        return 0;
    }

    luaL_argcheck(LuaState, lua_istable(LuaState, 3), 3, "Expected a Vector3T (a table).");

    btVector3 rel_pos;

    lua_rawgeti(LuaState, 3, 1); rel_pos.setX(float(UnitsToPhys(lua_tonumber(LuaState, -1)))); lua_pop(LuaState, 1);
    lua_rawgeti(LuaState, 3, 2); rel_pos.setY(float(UnitsToPhys(lua_tonumber(LuaState, -1)))); lua_pop(LuaState, 1);
    lua_rawgeti(LuaState, 3, 3); rel_pos.setZ(float(UnitsToPhys(lua_tonumber(LuaState, -1)))); lua_pop(LuaState, 1);

    // Console->Print("Applying impulse " + convertToString(conv(Impulse)) +
    //     " with rel_pos " + convertToString(conv(rel_pos)) + "\n");

    Comp->m_RigidBody->applyImpulse(Impulse, rel_pos);
    Comp->m_RigidBody->activate();
    return 0;
}


static const cf::TypeSys::MethsDocT META_SetGravity =
{
    "SetGravity",
    "This method sets the gravity vector for this object, in m/s^2.\n"
    "The default gravity vector is `(0, 0, -9.81)`.",
    "", "(number gx, number gy, number gz)"
};

int ComponentPhysicsT::SetGravity(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentPhysicsT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentPhysicsT> >(1);

    btVector3 Gravity;

    Gravity.setX(float(luaL_checknumber(LuaState, 2)));
    Gravity.setY(float(luaL_checknumber(LuaState, 3)));
    Gravity.setZ(float(luaL_checknumber(LuaState, 4)));

    if (Comp->m_RigidBody)
    {
        Comp->m_RigidBody->setGravity(Gravity);
        Comp->m_RigidBody->activate();
    }

    return 0;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentPhysicsT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "physics component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentPhysicsT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentPhysicsT();
}

const luaL_Reg ComponentPhysicsT::MethodsList[] =
{
    { "ApplyImpulse", ApplyImpulse },
    { "SetGravity", SetGravity },
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentPhysicsT::DocMethods[] =
{
    META_ApplyImpulse,
    META_ApplyImpulse_overload,
    META_SetGravity,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentPhysicsT::TypeInfo(GetComponentTIM(), "GameSys::ComponentPhysicsT", "GameSys::ComponentBaseT", ComponentPhysicsT::CreateInstance, MethodsList, DocClass, DocMethods, NULL, DocVars);
