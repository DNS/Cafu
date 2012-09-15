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

#include "Physics.hpp"
#include "LookupTables.hpp"
#include "../FuncLadder.hpp"
#include "../../../GameWorld.hpp"
#include "ClipSys/ClipWorld.hpp"
#include "ClipSys/TraceResult.hpp"
#include "ClipSys/TraceSolid.hpp"
#include "MaterialSystem/Material.hpp"

using namespace GAME_NAME;


PhysicsHelperT::PhysicsHelperT(Vector3dT& Origin, Vector3dT& Velocity, const BoundingBox3dT& Dimensions,
                               const cf::ClipSys::ClipModelT& ClipModel, const cf::ClipSys::ClipWorldT& ClipWorld)
    : m_Origin(Origin),
      m_Velocity(Velocity),
      m_Dimensions(Dimensions),
      m_ClipModel(ClipModel),
      m_ClipWorld(ClipWorld)
{
}


/// Bestimme unsere PosCat innerhalb der World.
PhysicsHelperT::PosCatT PhysicsHelperT::CategorizePosition() const
{
    // Bestimme die Brushes in der unmittelbaren Nähe unserer BB, insb. diejenigen unter uns. Bloate gemäß unserer Dimensions-BoundingBox.
    cf::ClipSys::TraceResultT Trace(1.0);
    m_ClipWorld.TraceBoundingBox(m_Dimensions, m_Origin, VectorT(0.0, 0.0, -1.0), MaterialT::Clip_Players, &m_ClipModel, Trace);

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
        const VectorT SlideOff=VectorT(0.0, 0.0, -1.0)+scale(Trace.ImpactNormal, Trace.ImpactNormal.z);

        Trace=cf::ClipSys::TraceResultT(1.0);   // Very important - reset the trace results.
        m_ClipWorld.TraceBoundingBox(m_Dimensions, m_Origin, SlideOff, MaterialT::Clip_Players, &m_ClipModel, Trace);

        if (Trace.Fraction==1.0) return InAir;
    }

    return OnSolid;
}


/// Ändert die Velocity gemäß der Reibung.
void PhysicsHelperT::ApplyFriction(double FrameTime, PosCatT PosCat)
{
    const double GroundFriction=4.0;
    const double AirFriction   =0.2;
    const double StopSpeed     =200;
    double       CurrentSpeed  =length(m_Velocity);
    double       Drop          =0;

    // Kontrolliert zum Stillstand kommen
    if (CurrentSpeed<20)
    {
        m_Velocity.x=0;
        m_Velocity.y=0;
     // m_Velocity.z=0;
        return;
    }

    // Siehe Physik-Buch von Dorn-Bader, Seite 45ff.
    switch (PosCat)
    {
        case InAir  : Drop=CurrentSpeed*AirFriction*FrameTime;
                      break;
        case OnSolid: double Control=CurrentSpeed<StopSpeed ? StopSpeed : CurrentSpeed;
                      Drop          =Control*GroundFriction*FrameTime;
                      // To do: if the leading edge of the BB is over a dropoff, increase friction
                      break;
    }

    double NewSpeed=CurrentSpeed-Drop;
    if (NewSpeed<0) NewSpeed=0;
    m_Velocity=scale(m_Velocity, NewSpeed/CurrentSpeed);
}


/// Ändert die Velocity gemäß der Beschleunigung.
void PhysicsHelperT::ApplyAcceleration(double FrameTime, PosCatT PosCat, const VectorT& WishVelocity)
{
    const double AirAcceleration   = 0.7;
    const double GroundAcceleration=10.0;

    double  WishSpeed=length(WishVelocity);
    VectorT WishDir;

    if (WishSpeed>0.001 /*Epsilon*/) WishDir=scale(WishVelocity, 1.0/WishSpeed);

    // Velocity auf WishDir projezieren um festzustellen, wie schnell wir sowieso schon in WishDir laufen
    double CurrentSpeed=dot(m_Velocity, WishDir);
    double Acceleration=0;
    double AddSpeed    =0;

    switch (PosCat)
    {
        case InAir  : // airborn, so little effect on velocity
                      Acceleration=AirAcceleration;
                      AddSpeed=WishSpeed>500.0 ? 500.0-CurrentSpeed : WishSpeed-CurrentSpeed;
                      break;
        case OnSolid: Acceleration=GroundAcceleration;
                      AddSpeed=WishSpeed-CurrentSpeed;
                      // m_Velocity.z=0;
                      break;
    }

    if (AddSpeed<=0) return;

    double AccelSpeed=WishSpeed*FrameTime*Acceleration;
    if (AccelSpeed>AddSpeed) AccelSpeed=AddSpeed;

    m_Velocity=m_Velocity+scale(WishDir, AccelSpeed);
}


/// Changes the velocity according to gravity.
void PhysicsHelperT::ApplyGravity(double FrameTime, PosCatT PosCat)
{
    if (PosCat==InAir) m_Velocity.z-=9810.0*FrameTime;
}


/// This is the basic solid shape movement function that slides the m_Dimensions bounding-box along multiple planes, according to velocity and time left.
void PhysicsHelperT::FlyMove(double TimeLeft)
{
    const VectorT   OriginalVelocity=m_Velocity;
    ArrayT<VectorT> PlaneNormals;

    for (char BumpCount=0; BumpCount<3; BumpCount++)
    {
        const VectorT             DistLeft=scale(m_Velocity, TimeLeft);
        cf::ClipSys::TraceResultT Trace(1.0);

        m_ClipWorld.TraceBoundingBox(m_Dimensions, m_Origin, DistLeft, MaterialT::Clip_Players, &m_ClipModel, Trace);

        if (Trace.StartSolid)               // MassChunk is trapped in another solid
        {
            // Print_32(LFB32,10,55,0x00FFFF88,false,"Trace.StartSolid");
            m_Velocity=VectorT(0, 0, 0);
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
            m_Velocity=OriginalVelocity-scale(PlaneNormals[Nr], dot(OriginalVelocity, PlaneNormals[Nr]));

            // if (fabs(m_Velocity.x)<0.1) m_Velocity.x=0;
            // if (fabs(m_Velocity.y)<0.1) m_Velocity.y=0;
            // if (fabs(m_Velocity.z)<0.1) m_Velocity.z=0;

            // Testen, ob die Velocity nun auch von allen getroffenen Planes wegzeigt
            unsigned long Nr_;

            for (Nr_=0; Nr_<PlaneNormals.Size(); Nr_++)
            {
                if (Nr_==Nr) continue;
                if (dot(m_Velocity, PlaneNormals[Nr_])<0) break;
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
                m_Velocity=VectorT(0, 0, 0);
                break;
            }

            // Gehe entlang des Knicks, der Falte
            VectorT Dir=cross(PlaneNormals[0], PlaneNormals[1]);
            m_Velocity=scale(Dir, dot(Dir, m_Velocity));
        }

        // If Velocity is against the OriginalVelocity, stop dead to avoid tiny occilations in sloping corners
        // (es scheint nicht, als das dies jemals passieren könnte und wir jemals zum if-Code kommen?!?).
        if (dot(m_Velocity, OriginalVelocity)<=0)
        {
            m_Velocity=VectorT(0, 0, 0);
            break;
        }
    }
}


/// MassChunk is on ground, with no upwards velocity
void PhysicsHelperT::GroundMove(double FrameTime, double StepHeight)
{
    // m_Velocity.z=0;        // Ganz sicher gehen, daß wir an der Höhe wirklich nichts ändern
    if (!m_Velocity.x && !m_Velocity.y && !m_Velocity.z) return;

    // Zunächst versuchen, das Ziel direkt zu erreichen
    const VectorT             DistLeft=scale(m_Velocity, FrameTime);
    cf::ClipSys::TraceResultT Trace(1.0);

    m_ClipWorld.TraceBoundingBox(m_Dimensions, m_Origin, DistLeft, MaterialT::Clip_Players, &m_ClipModel, Trace);

    if (Trace.Fraction==1.0)
    {
        m_Origin=m_Origin+DistLeft;
        return;
    }

    // Versuche auf dem Boden sowie StepHeight Units höher vorwärts zu gleiten,
    // und nimm die Bewegung, die am weitesten führt.
    VectorT OriginalPos=m_Origin;
    VectorT OriginalVel=m_Velocity;

    FlyMove(FrameTime);     // Correctly slide along walls, just calling m_ClipWorld.TraceBoundingBox() is not enough.

    VectorT GroundPos=m_Origin;
    VectorT GroundVel=m_Velocity;

    // Restore original position and velocity.
    m_Origin  =OriginalPos;
    m_Velocity=OriginalVel;

    // One step up.
    const VectorT StepUp  =VectorT(0, 0,  StepHeight);
    const VectorT StepDown=VectorT(0, 0, -StepHeight);

    Trace=cf::ClipSys::TraceResultT(1.0);   // Very important - reset the trace results.
    m_ClipWorld.TraceBoundingBox(m_Dimensions, m_Origin, StepUp, MaterialT::Clip_Players, &m_ClipModel, Trace);
    m_Origin=m_Origin+scale(StepUp, Trace.Fraction);

    // Then forward.
    FlyMove(FrameTime);

    // One step down again.
    Trace=cf::ClipSys::TraceResultT(1.0);   // Very important - reset the trace results.
    m_ClipWorld.TraceBoundingBox(m_Dimensions, m_Origin, StepDown, MaterialT::Clip_Players, &m_ClipModel, Trace);

    // Don't climb up somewhere where it is pointless.
    if (Trace.Fraction<1.0 && Trace.ImpactNormal.z<0.7)
    {
        m_Origin  =GroundPos;
        m_Velocity=GroundVel;
        return;
    }

    m_Origin=m_Origin+scale(StepDown, Trace.Fraction);

    // VectorT StepPos=m_Origin;
    // VectorT StepVel=m_Velocity;

    double GroundDist=length(GroundPos   -OriginalPos);
    double StepDist  =length(m_Origin-OriginalPos);

    if (GroundDist>StepDist)
    {
        m_Origin  =GroundPos;
        m_Velocity=GroundVel;
    }
    // else m_Velocity.z=GroundVel.z;     // ????????

    // StepHeight doesn't only help with walking steps up, it also helps
    // with walking down (steps or light declines/slopes, not steeper than steps).

    // if at a dead stop, retry the move with nudges to get around lips
}


void PhysicsHelperT::MoveHuman(float FrameTime, unsigned short Heading, const VectorT& WishVelocity,
                               const VectorT& WishVelLadder, bool WishJump, bool& OldWishJump, double StepHeight)
{
    // 1. Die Positions-Kategorie des MassChunks bestimmen:
    //    Wir können uns in der Luft befinden (im Flug/freien Fall oder schwebend im Wasser),
    //    oder auf einem Entity stehen (dazu gehören die Map, alle Objekte wie Türen usw., Monster und andere Spieler).
    //    Unabhängig davon kann in beiden Fällen das Wasser verschieden hoch stehen.
    PosCatT PosCat=CategorizePosition();


    // 2. Determine if we are on a ladder.
    // ViewDir should probably be perpendicular to the gravity vector...
    const Vector3dT ViewDir=Vector3dT(LookupTables::Angle16ToSin[Heading], LookupTables::Angle16ToCos[Heading], 0.0);

    // Note that contrary to m_ClipWorld.TraceBoundingBox(), m_ClipWorld.GetContacts() doesn't consider the world,
    // which is very helpful in some situations.
    cf::ClipSys::ContactsResultT Contacts;
    cf::ClipSys::TraceResultT    LadderResult(1.0);

    m_ClipWorld.GetContacts(m_Dimensions, m_Origin, ViewDir*150.0, MaterialT::Clip_Players, &m_ClipModel, Contacts);

    for (unsigned long ContactNr=0; LadderResult.Fraction==1.0 && ContactNr<Contacts.NrOfRepContacts; ContactNr++)
    {
        const cf::ClipSys::ClipModelT* CM=Contacts.ClipModels[ContactNr];

        if (CM->GetUserData()!=NULL && static_cast<BaseEntityT*>(CM->GetUserData())->GetType()==&EntFuncLadderT::TypeInfo)
            LadderResult=Contacts.TraceResults[ContactNr];
    }

    const bool OnLadder=(LadderResult.Fraction<1.0);

    if (OnLadder)
    {
        if (WishJump)
        {
            if (!OldWishJump)
            {
                OldWishJump=true;
                // TODO: Move 'm_Origin' along 'ImpactNormal' until we do not touch this brush any longer.
                m_Velocity=scale(LadderResult.ImpactNormal, 6200.0);    // 6200 is just a guessed value.
            }
        }
        else
        {
            // Decompose the 'WishVelLadder' into the ladder plane.
            double  Normal =dot(WishVelLadder, LadderResult.ImpactNormal);
            VectorT Into   =scale(LadderResult.ImpactNormal, Normal);           // This is the velocity orthogonal into the plane of the ladder.
            VectorT Lateral=WishVelLadder-Into;                                 // This is the remaining (lateral) velocity in the plane of the ladder.
            VectorT Perpend=cross(VectorT(0, 0, 1), LadderResult.ImpactNormal); // A perpendicular (orthogonal) vector.
            double  LengthP=length(Perpend);

            // If reasonably possible, normalize 'Perpend'.
            Perpend=(LengthP>0.0001) ? scale(Perpend, 1.0/LengthP) : VectorT();

            // This turns the velocity into the face of the ladder ('Into') into velocity
            // that is roughly vertically perpendicular to the face of the latter.
            // Note: It IS possible to face up and move down or face down and move up, because the velocity
            // is a sum of the directional velocity and the converted velocity through the face of the ladder.
            m_Velocity=Lateral-scale(cross(LadderResult.ImpactNormal, Perpend), Normal);

            if (PosCat==OnSolid && Normal>0) m_Velocity=m_Velocity+scale(LadderResult.ImpactNormal, 4200.0);
            OldWishJump=false;
        }
    }


    if (!OnLadder)
    {
        // 3. Neubestimmen der Velocity - Physics
        //    Die neue Velocity des MassChunks ergibt sich aus (fast) allen Variablen,
        //    die uns innerhalb der Klasse und dieser Funktion zur Verfügung stehen.

        // 3.1. Deal with WishJump
        //      Consider all eight possibilities and the actions to be taken:
        //
        //      PosCat   WJ OWJ   Action
        //      ---------------------------------------------------------
        //      InAir     0  0    nothing (or OWJ=false;)
        //      InAir     0  1    OWJ=false;
        //      InAir     1  0    OWJ=true;
        //      InAir     1  1    nothing
        //      OnGround  0  0    nothing (or OWJ=false;)
        //      OnGround  0  1    OWJ=false;
        //      OnGround  1  0    OWJ=true; (and jump: PosCat=InAir; m_Velocity.z+=...;)
        //      OnGround  1  1    nothing
        if (WishJump)
        {
            if (!OldWishJump)
            {
                OldWishJump=true;

                if (PosCat==OnSolid)
                {
                    // Jump
                    PosCat=InAir;
                    m_Velocity.z+=5624.0;   // v=sqrt(2*a*s), mit s = 44" = 44*0.026m = 1.144m für normalen Jump, und s = 62" = 1.612m für crouch-Jump.
                }
            }
        }
        else OldWishJump=false;

        // 3.2. Apply Physics
        ApplyFriction    (FrameTime, PosCat);
        ApplyAcceleration(FrameTime, PosCat, WishVelocity);
        ApplyGravity     (FrameTime, PosCat);
    }


    // 4. Umsetzen der Bewegung in der World, so gut es geht.
    //    Bestimme dazu alle Brushes, die unsere Bewegung evtl. behindern könnten, gebloated gemäß unserer Dimensions-BoundingBox.
    switch (PosCat)
    {
        case InAir:   FlyMove   (FrameTime);             break;
        case OnSolid: GroundMove(FrameTime, StepHeight); break;
    }
}
