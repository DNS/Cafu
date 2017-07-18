/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompPlayerPhysics.hpp"
#include "AllComponents.hpp"
#include "CompCollisionModel.hpp"
#include "Entity.hpp"
#include "World.hpp"

#include "ClipSys/ClipModel.hpp"
#include "ClipSys/ClipWorld.hpp"
#include "ClipSys/TraceResult.hpp"
#include "MaterialSystem/Material.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


const char* ComponentPlayerPhysicsT::DocClass =
    "This component implements human player physics for its entity.\n"
    "It updates the entity's origin according to the laws of simple physics\n"
    "that are appropriate for player movement.\n"
    "The component does not act on its own in a server's Think() step, but is\n"
    "only a helper to other C++ or script code that must drive it explicitly.";


const cf::TypeSys::VarsDocT ComponentPlayerPhysicsT::DocVars[] =
{
    { "Velocity",    "The current velocity of the entity." },
    { "Dimensions",  "The bounding box of the entity (relative to the origin)." },
    { "StepHeight",  "The maximum height that the entity can climb in one step." },
    { NULL, NULL }
};


ComponentPlayerPhysicsT::ComponentPlayerPhysicsT()
    : ComponentBaseT(),
      m_Velocity("Velocity", Vector3dT(0, 0, 0)),
      m_Dimensions("Dimensions", BoundingBox3dT(Vector3dT(-8, -8, -8), Vector3dT(8, 8, 8))),
      m_StepHeight("StepHeight", 0.0),
      m_ClipWorld(NULL),
      m_IgnoreClipModel(NULL),
      m_Origin(),
      m_Vel(),
      m_DimSolid(m_Dimensions.Get())
{
    GetMemberVars().Add(&m_Velocity);
    GetMemberVars().Add(&m_Dimensions);
    GetMemberVars().Add(&m_StepHeight);
}


ComponentPlayerPhysicsT::ComponentPlayerPhysicsT(const ComponentPlayerPhysicsT& Comp)
    : ComponentBaseT(Comp),
      m_Velocity(Comp.m_Velocity),
      m_Dimensions(Comp.m_Dimensions),
      m_StepHeight(Comp.m_StepHeight),
      m_ClipWorld(NULL),
      m_IgnoreClipModel(NULL),
      m_Origin(),
      m_Vel(),
      m_DimSolid(m_Dimensions.Get())
{
    GetMemberVars().Add(&m_Velocity);
    GetMemberVars().Add(&m_Dimensions);
    GetMemberVars().Add(&m_StepHeight);
}


void ComponentPlayerPhysicsT::MoveHuman(float FrameTime, const Vector3fT& WishVelocity, const Vector3fT& WishVelLadder, bool WishJump)
{
    if (!GetEntity()) return;
    if (!m_ClipWorld) return;

    IntrusivePtrT<ComponentCollisionModelT> CompCollMdl = dynamic_pointer_cast<ComponentCollisionModelT>(GetEntity()->GetComponent("CollisionModel"));

    if (CompCollMdl != NULL)
        m_IgnoreClipModel = CompCollMdl->GetClipModel();

    m_Origin   = GetEntity()->GetTransform()->GetOriginWS().AsVectorOfDouble();
    m_Vel      = m_Velocity.Get();
    m_DimSolid = cf::ClipSys::TraceBoxT(m_Dimensions.Get());

    MoveHuman(FrameTime, WishVelocity.AsVectorOfDouble(), WishVelLadder.AsVectorOfDouble(), WishJump);

    m_IgnoreClipModel = NULL;
    GetEntity()->GetTransform()->SetOriginWS(m_Origin.AsVectorOfFloat());
    m_Velocity.Set(m_Vel);
}


ComponentPlayerPhysicsT* ComponentPlayerPhysicsT::Clone() const
{
    return new ComponentPlayerPhysicsT(*this);
}


void ComponentPlayerPhysicsT::UpdateDependencies(EntityT* Entity)
{
    m_ClipWorld = NULL;

    ComponentBaseT::UpdateDependencies(Entity);

    if (Entity)
    {
        m_ClipWorld = Entity->GetWorld().GetClipWorld();
    }
}


/// Bestimme unsere PosCat innerhalb der World.
ComponentPlayerPhysicsT::PosCatT ComponentPlayerPhysicsT::CategorizePosition() const
{
    // Bestimme die Brushes in der unmittelbaren Nähe unserer BB, insb. diejenigen unter uns. Bloate gemäß unserer Dimensions-BoundingBox.
    cf::ClipSys::TraceResultT Trace(1.0);
    m_ClipWorld->TraceConvexSolid(m_DimSolid, m_Origin, Vector3dT(0.0, 0.0, -0.1), MaterialT::Clip_Players, m_IgnoreClipModel, Trace);

    // OLD CODE:
    // return (Trace.Fraction==1.0 || Trace.ImpactNormal.z<0.7) ? InAir : OnSolid;

    // Are we airborne or standing on a slope that is steeper than 45 degrees?
    if (Trace.Fraction==1.0) return InAir;

    if (Trace.ImpactNormal.z<0.7)
    {
        // We're touching a ramp that is steeper than 45 degrees.
        // Now figure out if we are on ground or in air.

        // Project (0, 0, -1) onto the hit plane.
        // SlideOff is computed by the same expression as in FlyMove(), except that OriginalVelocity is known to be (0, 0, -1).
        const Vector3dT SlideOff = Vector3dT(0.0, 0.0, -0.1) + scale(Trace.ImpactNormal, Trace.ImpactNormal.z);

        Trace=cf::ClipSys::TraceResultT(1.0);   // Very important - reset the trace results.
        m_ClipWorld->TraceConvexSolid(m_DimSolid, m_Origin, SlideOff, MaterialT::Clip_Players, m_IgnoreClipModel, Trace);

        if (Trace.Fraction==1.0) return InAir;
    }

    return OnSolid;
}


/// Ändert die Velocity gemäß der Reibung.
void ComponentPlayerPhysicsT::ApplyFriction(double FrameTime, PosCatT PosCat)
{
    const double GroundFriction = 4.0;
    const double AirFriction    = 0.2;
    const double StopSpeed      = 8.0;
    double       CurrentSpeed   = length(m_Vel);
    double       Drop           = 0;

    // Kontrolliert zum Stillstand kommen
    if (CurrentSpeed < 1.0)
    {
        m_Vel.x = 0;
        m_Vel.y = 0;
     // m_Vel.z = 0;
        return;
    }

    // Siehe Physik-Buch von Dorn-Bader, Seite 45ff.
    switch (PosCat)
    {
        case InAir:
            Drop = CurrentSpeed*AirFriction*FrameTime;
            break;

        case OnSolid:
            double Control = CurrentSpeed<StopSpeed ? StopSpeed : CurrentSpeed;
            Drop           = Control*GroundFriction*FrameTime;

            // To do: if the leading edge of the BB is over a dropoff, increase friction
            break;
    }

    double NewSpeed = CurrentSpeed - Drop;
    if (NewSpeed < 0) NewSpeed = 0;
    m_Vel = scale(m_Vel, NewSpeed/CurrentSpeed);
}


/// Ändert die Velocity gemäß der Beschleunigung.
void ComponentPlayerPhysicsT::ApplyAcceleration(double FrameTime, PosCatT PosCat, const Vector3dT& WishVelocity)
{
    const double AirAcceleration   = 0.7;
    const double GroundAcceleration=10.0;

    double    WishSpeed=length(WishVelocity);
    Vector3dT WishDir;

    if (WishSpeed>0.001 /*Epsilon*/) WishDir=scale(WishVelocity, 1.0/WishSpeed);

    // Velocity auf WishDir projezieren um festzustellen, wie schnell wir sowieso schon in WishDir laufen
    double CurrentSpeed = dot(m_Vel, WishDir);
    double Acceleration = 0;
    double AddSpeed     = 0;

    switch (PosCat)
    {
        case InAir  : // airborn, so little effect on velocity
                      Acceleration=AirAcceleration;
                      AddSpeed=WishSpeed>20.0 ? 20.0-CurrentSpeed : WishSpeed-CurrentSpeed;
                      break;
        case OnSolid: Acceleration=GroundAcceleration;
                      AddSpeed=WishSpeed-CurrentSpeed;
                      // m_Vel.z=0;
                      break;
    }

    if (AddSpeed<=0) return;

    double AccelSpeed=WishSpeed*FrameTime*Acceleration;
    if (AccelSpeed>AddSpeed) AccelSpeed=AddSpeed;

    m_Vel = m_Vel + scale(WishDir, AccelSpeed);
}


/// Changes the velocity according to gravity.
void ComponentPlayerPhysicsT::ApplyGravity(double FrameTime, PosCatT PosCat)
{
    if (PosCat == InAir) m_Vel.z -= 386.22 * FrameTime;  // 9.81 m/s^2
}


/// This is the basic solid shape movement function that slides the m_Dimensions bounding-box along multiple planes, according to velocity and time left.
void ComponentPlayerPhysicsT::FlyMove(double TimeLeft)
{
    const Vector3dT   OriginalVelocity = m_Vel;
    ArrayT<Vector3dT> PlaneNormals;

    for (char BumpCount=0; BumpCount<3; BumpCount++)
    {
        const Vector3dT           DistLeft = scale(m_Vel, TimeLeft);
        cf::ClipSys::TraceResultT Trace(1.0);

        m_ClipWorld->TraceConvexSolid(m_DimSolid, m_Origin, DistLeft, MaterialT::Clip_Players, m_IgnoreClipModel, Trace);

        if (Trace.StartSolid)               // MassChunk is trapped in another solid
        {
            // Print_32(LFB32,10,55,0x00FFFF88,false,"Trace.StartSolid");
            m_Vel = Vector3dT(0, 0, 0);
            return;
        }

        // Print_32(LFB32,10,55,0x00FFFF88,false,"Trace.Fraction %5.4f",Trace.Fraction);
        m_Origin=m_Origin+scale(DistLeft, Trace.Fraction);
        if (Trace.Fraction==1.0) break;     // Moved the entire distance left

        // Hit a plane, otherwise
        TimeLeft=(1.0-Trace.Fraction)*TimeLeft;
        PlaneNormals.PushBack(Trace.ImpactNormal);

        // Modify OriginalVelocity so it parallels all of the clip planes
        unsigned long Nr;

        for (Nr=0; Nr<PlaneNormals.Size(); Nr++)
        {
            // OriginalVelocity auf jede getroffene Plane projezieren
            m_Vel = OriginalVelocity - scale(PlaneNormals[Nr], dot(OriginalVelocity, PlaneNormals[Nr]));

            // if (fabs(m_Vel.x)<0.1) m_Vel.x=0;
            // if (fabs(m_Vel.y)<0.1) m_Vel.y=0;
            // if (fabs(m_Vel.z)<0.1) m_Vel.z=0;

            // Testen, ob die Velocity nun auch von allen getroffenen Planes wegzeigt
            unsigned long Nr_;

            for (Nr_=0; Nr_<PlaneNormals.Size(); Nr_++)
            {
                if (Nr_==Nr) continue;
                if (dot(m_Vel, PlaneNormals[Nr_])<0) break;
            }
            if (Nr_==PlaneNormals.Size()) break;
        }

        if (Nr<PlaneNormals.Size())
        {
            // Ausweg gefunden, gehe entlang der Plane
        }
        else
        {
            // Keinen Ausweg gefunden
            if (PlaneNormals.Size()!=2)
            {
                m_Vel = Vector3dT(0, 0, 0);
                break;
            }

            // Gehe entlang des Knicks, der Falte
            const Vector3dT Dir = cross(PlaneNormals[0], PlaneNormals[1]);

            m_Vel = scale(Dir, dot(Dir, m_Vel));
        }

        // If Velocity is against the OriginalVelocity, stop dead to avoid tiny occilations in sloping corners
        // (es scheint nicht, als das dies jemals passieren könnte und wir jemals zum if-Code kommen?!?).
        if (dot(m_Vel, OriginalVelocity)<=0)
        {
            m_Vel = Vector3dT(0, 0, 0);
            break;
        }
    }
}


/// MassChunk is on ground, with no upwards velocity
void ComponentPlayerPhysicsT::GroundMove(double FrameTime)
{
    // m_Vel.z=0;        // Ganz sicher gehen, daß wir an der Höhe wirklich nichts ändern
    if (!m_Vel.x && !m_Vel.y && !m_Vel.z) return;

    // Zunächst versuchen, das Ziel direkt zu erreichen
    const Vector3dT           DistLeft = scale(m_Vel, FrameTime);
    cf::ClipSys::TraceResultT Trace(1.0);

    m_ClipWorld->TraceConvexSolid(m_DimSolid, m_Origin, DistLeft, MaterialT::Clip_Players, m_IgnoreClipModel, Trace);

    if (Trace.Fraction==1.0)
    {
        m_Origin=m_Origin+DistLeft;
        return;
    }

    // Versuche auf dem Boden sowie StepHeight Units höher vorwärts zu gleiten,
    // und nimm die Bewegung, die am weitesten führt.
    Vector3dT OriginalPos = m_Origin;
    Vector3dT OriginalVel = m_Vel;

    FlyMove(FrameTime);     // Correctly slide along walls, just calling m_ClipWorld->TraceConvexSolid() is not enough.

    Vector3dT GroundPos = m_Origin;
    Vector3dT GroundVel = m_Vel;

    // Restore original position and velocity.
    m_Origin = OriginalPos;
    m_Vel    = OriginalVel;

    // One step up.
    const Vector3dT StepUp   = Vector3dT(0, 0,  m_StepHeight.Get());
    const Vector3dT StepDown = Vector3dT(0, 0, -m_StepHeight.Get());

    Trace = cf::ClipSys::TraceResultT(1.0);   // Very important - reset the trace results.
    m_ClipWorld->TraceConvexSolid(m_DimSolid, m_Origin, StepUp, MaterialT::Clip_Players, m_IgnoreClipModel, Trace);
    m_Origin = m_Origin + scale(StepUp, Trace.Fraction);

    // Then forward.
    FlyMove(FrameTime);

    // One step down again.
    Trace = cf::ClipSys::TraceResultT(1.0);   // Very important - reset the trace results.
    m_ClipWorld->TraceConvexSolid(m_DimSolid, m_Origin, StepDown, MaterialT::Clip_Players, m_IgnoreClipModel, Trace);

    // Don't climb up somewhere where it is pointless.
    if (Trace.Fraction < 1.0 && Trace.ImpactNormal.z < 0.7)
    {
        m_Origin = GroundPos;
        m_Vel    = GroundVel;
        return;
    }

    m_Origin = m_Origin + scale(StepDown, Trace.Fraction);

    // Vector3dT StepPos = m_Origin;
    // Vector3dT StepVel = m_Vel;

    double GroundDist=length(GroundPos   -OriginalPos);
    double StepDist  =length(m_Origin-OriginalPos);

    if (GroundDist>StepDist)
    {
        m_Origin = GroundPos;
        m_Vel    = GroundVel;
    }
    // else m_Vel.z=GroundVel.z;     // ????????

    // StepHeight doesn't only help with walking steps up, it also helps
    // with walking down (steps or light declines/slopes, not steeper than steps).

    // if at a dead stop, retry the move with nudges to get around lips
}


void ComponentPlayerPhysicsT::MoveHuman(float FrameTime, const Vector3dT& WishVelocity, const Vector3dT& WishVelLadder, bool WishJump)
{
    // 1. Die Positions-Kategorie des MassChunks bestimmen:
    //    Wir können uns in der Luft befinden (im Flug/freien Fall oder schwebend im Wasser),
    //    oder auf einem Entity stehen (dazu gehören die Map, alle Objekte wie Türen usw., Monster und andere Spieler).
    //    Unabhängig davon kann in beiden Fällen das Wasser verschieden hoch stehen.
    PosCatT PosCat=CategorizePosition();


    // 2. Determine if we are on a ladder.
    // BodyDir does not take the player's actual view dir into account (from its camera entity),
    // but only its heading (from the orientation of its body).
    const Vector3dT BodyDir = cf::math::Matrix3x3fT(GetEntity()->GetTransform()->GetQuatWS()).GetAxis(0).AsVectorOfDouble();
    cf::ClipSys::TraceResultT LadderResult(1.0);

    m_ClipWorld->TraceConvexSolid(m_DimSolid, m_Origin, BodyDir * 6.0, MaterialT::Clip_Players, m_IgnoreClipModel, LadderResult);

    const bool OnLadder = LadderResult.Fraction < 1.0 && LadderResult.Material && ((LadderResult.Material->ClipFlags & MaterialT::SP_Ladder) != 0);

    if (OnLadder)
    {
        if (WishJump)
        {
            // TODO: Move 'm_Origin' along 'ImpactNormal' until we do not touch this brush any longer.
            m_Vel = scale(LadderResult.ImpactNormal, 248.0);    // 248 is just a guessed value.
        }
        else
        {
            // Decompose the 'WishVelLadder' into the ladder plane.
            double    Normal  = dot(WishVelLadder, LadderResult.ImpactNormal);
            Vector3dT Into    = scale(LadderResult.ImpactNormal, Normal);           // This is the velocity orthogonal into the plane of the ladder.
            Vector3dT Lateral = WishVelLadder-Into;                                 // This is the remaining (lateral) velocity in the plane of the ladder.
            Vector3dT Perpend = cross(Vector3dT(0, 0, 1), LadderResult.ImpactNormal); // A perpendicular (orthogonal) vector.
            double    LengthP = length(Perpend);

            // If reasonably possible, normalize 'Perpend'.
            Perpend = (LengthP > 0.0001) ? scale(Perpend, 1.0/LengthP) : Vector3dT();

            // This turns the velocity into the face of the ladder ('Into') into velocity
            // that is roughly vertically perpendicular to the face of the latter.
            // Note: It IS possible to face up and move down or face down and move up, because the velocity
            // is a sum of the directional velocity and the converted velocity through the face of the ladder.
            m_Vel = Lateral - scale(cross(LadderResult.ImpactNormal, Perpend), Normal);

            if (PosCat == OnSolid && Normal > 0) m_Vel = m_Vel + scale(LadderResult.ImpactNormal, 168.0);
        }
    }
    else
    {
        // We're not on a ladder.
        // 3.1. Deal with WishJump
        if (WishJump && PosCat == OnSolid)
        {
            PosCat = InAir;
            m_Vel.z += 219.0;  // v=sqrt(2*a*s), mit a = 9.81m/s^2 und mit s = 44" = 44*0.026m = 1.144m für normalen Jump, und s = 62" = 1.612m für crouch-Jump.
        }

        // 3.2. Apply Physics
        ApplyFriction    (FrameTime, PosCat);
        ApplyAcceleration(FrameTime, PosCat, WishVelocity);
        ApplyGravity     (FrameTime, PosCat);
    }


    // 4. Umsetzen der Bewegung in der World, so gut es geht.
    //    Bestimme dazu alle Brushes, die unsere Bewegung evtl. behindern könnten, gebloated gemäß unserer Dimensions-BoundingBox.
    switch (PosCat)
    {
        case InAir:   FlyMove   (FrameTime); break;
        case OnSolid: GroundMove(FrameTime); break;
    }
}


static const cf::TypeSys::MethsDocT META_MoveHuman =
{
    "MoveHuman",
    "This is the main method of this component: It advances the entity's origin\n"
    "according to the laws of simple physics and the given state and parameters.\n"
    "Other C++ or script code of the entity typically calls this method on each\n"
    "clock-tick (frame) of the server.\n"
    "@param FrameTime       The time across which the entity is to be advanced.\n"
    "@param WishVelocity    The desired velocity of the entity as per user input.\n"
    "@param WishVelLadder   The desired velocity on a ladder as per user input.\n"
    "@param WishJump        Does the user want the entity to jump?",
    "", "(number FrameTime, Vector3T WishVelocity, Vector3T WishVelLadder, bool WishJump)"
};

int ComponentPlayerPhysicsT::MoveHuman(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentPlayerPhysicsT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentPlayerPhysicsT> >(1);

    const float     FrameTime = float(luaL_checknumber(LuaState, 2));
    const Vector3dT WishVelocity (luaL_checknumber(LuaState, 3), luaL_checknumber(LuaState, 4), luaL_checknumber(LuaState, 5));
    const Vector3dT WishVelLadder(luaL_checknumber(LuaState, 6), luaL_checknumber(LuaState, 7), luaL_checknumber(LuaState, 8));
    const bool      WishJump = lua_isnumber(LuaState, 9) ? (lua_tointeger(LuaState, 9) != 0) : (lua_toboolean(LuaState, 9) != 0);

    Comp->MoveHuman(FrameTime, WishVelocity.AsVectorOfFloat(), WishVelLadder.AsVectorOfFloat(), WishJump);
    return 0;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentPlayerPhysicsT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "human payer physics component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentPlayerPhysicsT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentPlayerPhysicsT();
}

const luaL_Reg ComponentPlayerPhysicsT::MethodsList[] =
{
    { "MoveHuman", MoveHuman },
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentPlayerPhysicsT::DocMethods[] =
{
    META_MoveHuman,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentPlayerPhysicsT::TypeInfo(GetComponentTIM(), "GameSys::ComponentPlayerPhysicsT", "GameSys::ComponentBaseT", ComponentPlayerPhysicsT::CreateInstance, MethodsList, DocClass, DocMethods, NULL, DocVars);
