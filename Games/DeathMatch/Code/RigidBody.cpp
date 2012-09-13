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

#include "RigidBody.hpp"
#include "EntityCreateParams.hpp"
#include "Interpolator.hpp"
#include "../../GameWorld.hpp"

#include "PhysicsWorld.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"
#include "Network/State.hpp"
#include "SceneGraph/Node.hpp"
#include "UniScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntRigidBodyT::GetType() const
{
    return &TypeInfo;
 // return &EntRigidBodyT::TypeInfo;
}

void* EntRigidBodyT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntRigidBodyT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const luaL_Reg EntRigidBodyT::MethodsList[]=
{
    { "ApplyImpulse", EntRigidBodyT::ApplyImpulse },
    { "SetGravity", EntRigidBodyT::SetGravity },
 // { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT EntRigidBodyT::TypeInfo(GetBaseEntTIM(), "EntRigidBodyT", "BaseEntityT", CreateInstance, MethodsList);


EntRigidBodyT::EntRigidBodyT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  Params.RootNode->GetBoundingBox(),
                  0),
      m_RootNode(Params.RootNode),
      m_CollisionShape(NULL),
      m_RigidBody(NULL),
      m_OrigOffset(m_Dimensions.GetCenter()-m_Origin),
      m_HalfExtents((m_Dimensions.Max-m_Dimensions.Min)/2.0 - Vector3dT(100.0, 100.0, 100.0)),  // FIXME !!! Where in the world does the extra 100 padding in Params.RootNode come from???
      m_Rotation(0, 0, 0, 1)
{
    Register(new InterpolatorT<Vector3dT>(m_Origin));
    ClipModel.Register();

    /*
     * Note that the ClipModel and the m_RootNode are defined in *world space*.
     * That means that at this point, m_Origin is *not* at the center of the rigid body, but at (0, 0, 0),
     * and that the m_Dimensions correspond to the bounding-box of the body (e.g. a crate) in world space.
     *
     * It helps to think of the m_OrigOffset vector from the origin to the body center as part of the model
     * that is fixed to the model at its tip:
     *
     *                                   +---------+   body
     *                  m_OrigOffset     |         |
     *           0-----------------------+--->X    |
     *                                   |         |
     *                                   +---------+
     *
     * As in the code in this class we want a more natural setup where the origin is at the center of the crate
     * and its dimensions are relative to it (in "model space"), we make the appropriate adjustments here.
     * This setup also happens to be used in the Bullet physics library.
     *
     * These adjustments must be accounted for / be undone when we deal with the ClipModel or the m_RootNode:
     * Observe how much the root of the m_OrigOffset vector changes even for tiny rotations around the body center
     * (the vector tip). The root of the rotated m_OrigOffset vector is the proper origin for the ClipModel and
     * the m_RootNode. This is also why this origin can *not* be interpolated across frames on the client, whereas
     * our origin that is adjusted to the center of the body can.
     */
    m_Origin = m_Dimensions.GetCenter();

    // Update the Dimensions box of the entity so that the server code properly determines whether we're in the clients PVS or not.
    // TODO / FIXME: The server computes our world-space bounding-box by offsetting our m_Dimensions by m_Origin.
    //      Thus, we update the m_Dimensions accordingly below, but actually the server code should be changed!
    //      For example, BaseEntityTs should just have a (virtual) method GetWorldBB() - the server should never query our m_Dimensions
    //      member directly. Maybe the dimensions should not even be a member of BaseEntityT??
    //      Also see old   svn diff -c 831   for how this affects other code!
    m_Dimensions.Min = -m_HalfExtents*1.732;    // The 1.732 is ca. sqrt(3), for the otherwise not accounted for possible rotation of the box.
    m_Dimensions.Max =  m_HalfExtents*1.732;


    // Consider the properties specific to this entity.
    float Mass=0.0f;

    for (std::map<std::string, std::string>::const_iterator It=Properties.begin(); It!=Properties.end(); ++It)
    {
        const std::string& Key  =It->first;
        const std::string& Value=It->second;

        if (Key=="Mass")
        {
            Mass=float(atof(Value.c_str()));
        }
    }


    // No matter if on client or server side: we always initialize the m_CollisionShape on both.
    // It seems that Bullet doesn't really support moving *concave* shapes:
    //   - No inertia is computed for them (see implementations of btConcaveShape::calculateLocalInertia()).
    //   - For collision detection, only the gimpact algorithm is available, see section "Collision Matrix" in Bullet_User_Manual.pdf.
    // The division by 1000.0 is because our physics world unit is meters.
    m_CollisionShape=new btBoxShape(conv(m_HalfExtents/1000.0));

    btVector3 Inertia;
    m_CollisionShape->calculateLocalInertia(Mass, Inertia);

    m_RigidBody=new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(Mass, this /*btMotionState for this body*/, m_CollisionShape, Inertia));

    m_RigidBody->setUserPointer(this);  // This entity is associated to the m_RigidBody.

    // TODO: Client-side rigid bodies should probably be "kinematic", not "dynamic", because the client doesn't actively simulate our bodies.
    //       From the pdf user manual: "every simulation frame, dynamics world will get new world transform using btMotionState::getWorldTransform"
    GameWorld->GetPhysicsWorld().AddRigidBody(m_RigidBody);
    // m_RigidBody->setGravity(btVector3(0, 0, 0));    // for debugging; reset gravity via map script commands
}


EntRigidBodyT::~EntRigidBodyT()
{
    GameWorld->GetPhysicsWorld().RemoveRigidBody(m_RigidBody);

    delete m_RigidBody;
    delete m_CollisionShape;
}


void EntRigidBodyT::DoSerialize(cf::Network::OutStreamT& Stream) const
{
    Stream << m_Rotation.x();
    Stream << m_Rotation.y();
    Stream << m_Rotation.z();
    Stream << m_Rotation.w();
}


void EntRigidBodyT::DoDeserialize(cf::Network::InStreamT& Stream)
{
    float f = 0.0f;

    Stream >> f; m_Rotation.setX(f);
    Stream >> f; m_Rotation.setY(f);
    Stream >> f; m_Rotation.setZ(f);
    Stream >> f; m_Rotation.setW(f);


    // Client-side: Properly update the clip model of the "old" ClipSys at the new position and orientation.
    const btMatrix3x3            Basis(m_Rotation);
    cf::math::Matrix3x3T<double> Orient;

    for (unsigned long i=0; i<3; i++)
        for (unsigned long j=0; j<3; j++)
            Orient[i][j]=Basis[i][j];

    ClipModel.SetOrigin(m_Origin - Orient*m_OrigOffset);
    ClipModel.SetOrientation(Orient);
    ClipModel.Register();
}


void EntRigidBodyT::TakeDamage(BaseEntityT* Entity, char Amount, const VectorT& ImpactDir)
{
    const float     Scale=2.0f;                                         // Just an experimentally determined number that seems to work well.
    const Vector3fT Imp  =ImpactDir.AsVectorOfFloat()*(Scale*Amount);   // Imp has no meaningful unit...
    const btVector3 Impulse(Imp.x, Imp.y, Imp.z);

    // See http://www.bulletphysics.com/Bullet/phpBB3/viewtopic.php?f=9&t=3079 for more details, especially for
    // why we can compute rel_pos correctly as Ob-Oc, instead of having to compute the exact location of the impact!
    const Vector3dT& Ob     =Entity->GetOrigin();   // Assumes that the damage was caused / originated at Entity->GetOrigin(). Should this be a parameter to TakeDamage()?
    const Vector3dT  Oc     =m_Origin;
    const Vector3fT  rel_pos=(Ob-Oc).AsVectorOfFloat()/1000.0f;

    m_RigidBody->applyImpulse(Impulse, btVector3(rel_pos.x, rel_pos.y, rel_pos.z));
    m_RigidBody->activate();
}


void EntRigidBodyT::Think(float FrameTime, unsigned long ServerFrameNr)
{
    // Rigid bodies don't think by themselves:
    // the physics world does all the thinking for them.
}


void EntRigidBodyT::Draw(bool FirstPersonView, float LodDist) const
{
    // BIG PROBLEM:
    // The EyePos as obtained from MatSys::Renderer->GetCurrentEyePosition() currently oscillates up and down,
    // as added by the code in ClientWorld.cpp for eye-candy specular-effect...

/*    Vector3dT EyePos(MatSys::Renderer->GetCurrentEyePosition());
    Vector3dT LightPos(MatSys::Renderer->GetCurrentLightSourcePosition());
    float     LightRadius=MatSys::Renderer->GetCurrentLightSourceRadius();

    // UNDO things the EngineEntityT::Draw() code did with .mdl models in mind...
 // MatSys::Renderer->...();
    MatSys::Renderer->Scale(MatSys::RendererI::MODEL_TO_WORLD, 1.0f/25.4f);
    MatSys::Renderer->RotateZ(MatSys::RendererI::MODEL_TO_WORLD, -90.0f);

    // UNDO things the EngineEntityT::Draw() code did with .mdl models in mind...
 // EyePos=EyePos-Entity->m_Origin;         // Convert into unrotated model space.
    EyePos=scale(EyePos, 25.4);
    EyePos=EyePos.GetRotZ(90.0);

    // UNDO things the EngineEntityT::Draw() code did with .mdl models in mind...
 // LightPos=LightPos-Entity->m_Origin;         // Convert into unrotated model space.
    LightPos=scale(LightPos, 25.4);
    LightPos=LightPos.GetRotZ(90.0);

    // UNDO things the EngineEntityT::Draw() code did with .mdl models in mind...
    LightRadius*=25.4f;

    // UNDO things the EngineEntityT::Draw() code did with .mdl models in mind...
    // Set the modified (now in model space) lighting parameters.
    MatSys::Renderer->SetCurrentLightSourcePosition(float(LightPos.x), float(LightPos.y), float(LightPos.z));
    MatSys::Renderer->SetCurrentLightSourceRadius(LightRadius);
    MatSys::Renderer->SetCurrentEyePosition(float(EyePos.x), float(EyePos.y), float(EyePos.z)); */


    // RESET THE MODEL_TO_WORLD MATRIX.
    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);

    // Copy the Basis into the upper 3x3 submatrix of M2W.
    const btMatrix3x3 Basis(m_Rotation);
    MatrixT           M2W;

    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            M2W[i][j]=Basis[i][j];

    const Vector3dT DrawOrig = m_Origin - M2W.Mul0(m_OrigOffset);

    M2W[0][3]=float(DrawOrig.x);
    M2W[1][3]=float(DrawOrig.y);
    M2W[2][3]=float(DrawOrig.z);

    MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, M2W);


    // RESET THE LIGHTING INFORMATION.
    MatSys::Renderer->PopLightingParameters();
    MatSys::Renderer->PushLightingParameters();

    // Get the currently set lighting parameters.
    Vector3fT LightPos(MatSys::Renderer->GetCurrentLightSourcePosition());
    float     LightRadius=MatSys::Renderer->GetCurrentLightSourceRadius();
    Vector3fT EyePos(MatSys::Renderer->GetCurrentEyePosition());

    // Transform from world to model space.
    LightPos=M2W.InvXForm(LightPos);
    EyePos=M2W.InvXForm(EyePos);

    // Set the modified (now in model space) lighting parameters.
    MatSys::Renderer->SetCurrentLightSourcePosition(LightPos.x, LightPos.y, LightPos.z);
    MatSys::Renderer->SetCurrentLightSourceRadius(LightRadius);
    MatSys::Renderer->SetCurrentEyePosition(EyePos.x, EyePos.y, EyePos.z);

    const Vector3dT LightPosD=LightPos.AsVectorOfDouble();
    const Vector3dT EyePosD  =EyePos.AsVectorOfDouble();


    switch (MatSys::Renderer->GetCurrentRenderAction())
    {
        case MatSys::RendererI::AMBIENT:
            m_RootNode->DrawAmbientContrib(EyePosD);
            m_RootNode->DrawTranslucentContrib(EyePosD);
            break;

        case MatSys::RendererI::LIGHTING:
            m_RootNode->DrawLightSourceContrib(EyePosD, LightPosD);
            break;

        case MatSys::RendererI::STENCILSHADOW:
            m_RootNode->DrawStencilShadowVolumes(LightPosD, LightRadius);
            break;
    }
}


void EntRigidBodyT::getWorldTransform(btTransform& worldTrans) const
{
    // Return the initial transformation of our rigid body to the physics world.
    // The division by 1000.0 is because our physics world unit is meters.
    worldTrans.setIdentity();
    worldTrans.setOrigin(conv(m_Origin/1000.0));
}


void EntRigidBodyT::setWorldTransform(const btTransform& worldTrans)
{
    if (!m_RigidBody->isActive()) return;   // See my post at http://www.bulletphysics.com/Bullet/phpBB3/viewtopic.php?t=2256 for details...

    // Update the transformation of our graphics object according to the physics world results.
    m_Origin = convd(worldTrans.getOrigin())*1000.0;

    // We're actually only interested in the basis vectors, but quaternions have many advantages for representng
    // spatial rotations (see http://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation for details), and they
    // are great for sync'ing the orientation over the network as well (only four simple numbers rather than nine).
    //
    // Why not use Euler angles instead? They're more intuitive, but if B and C are btMatrix3x3 and B is
    // a valid basis, then
    //     B.getEuler(y, p, r);
    //     C.setEulerYPR(y, p, r);
    // yields in C a basis different from B! That is, getEuler() and setEulerYPR() unexpectedly cannot be used
    // together, as is also pointed out in http://www.bulletphysics.com/Bullet/phpBB3/viewtopic.php?f=9&t=1961
    m_Rotation = worldTrans.getRotation();


    // Properly update the clip model of the "old" ClipSys at the new position and orientation.
    cf::math::Matrix3x3T<double> Orient;

    for (unsigned long i=0; i<3; i++)
        for (unsigned long j=0; j<3; j++)
            Orient[i][j]=worldTrans.getBasis()[i][j];

    ClipModel.SetOrigin(m_Origin - Orient*m_OrigOffset);
    ClipModel.SetOrientation(Orient);
    ClipModel.Register();
}


int EntRigidBodyT::ApplyImpulse(lua_State* LuaState)
{
    cf::ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntRigidBodyT> Ent=Binder.GetCheckedObjectParam< IntrusivePtrT<EntRigidBodyT> >(1);
    btVector3         Impulse;

    Impulse.setX(float(luaL_checknumber(LuaState, 2)));
    Impulse.setY(float(luaL_checknumber(LuaState, 3)));
    Impulse.setZ(float(luaL_checknumber(LuaState, 4)));

    Ent->m_RigidBody->applyCentralImpulse(Impulse);
    Ent->m_RigidBody->activate();
    return 0;
}


int EntRigidBodyT::SetGravity(lua_State* LuaState)
{
    cf::ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntRigidBodyT> Ent=Binder.GetCheckedObjectParam< IntrusivePtrT<EntRigidBodyT> >(1);
    btVector3         Gravity;

    Gravity.setX(float(luaL_checknumber(LuaState, 2)));
    Gravity.setY(float(luaL_checknumber(LuaState, 3)));
    Gravity.setZ(float(luaL_checknumber(LuaState, 4)));

    Ent->m_RigidBody->setGravity(Gravity);
    Ent->m_RigidBody->activate();
    return 0;
}
