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

#include "Eagle.hpp"
#include "EntityCreateParams.hpp"
#include "Libs/LookupTables.hpp"
#include "../../GameWorld.hpp"

#include "ClipSys/ClipWorld.hpp"
#include "ClipSys/TraceResult.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Models/Model_cmdl.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/Sound.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "TypeSys.hpp"


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntEagleT::GetType() const
{
    return &TypeInfo;
 // return &EntEagleT::TypeInfo;
}

void* EntEagleT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntEagleT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const cf::TypeSys::TypeInfoT EntEagleT::TypeInfo(GetBaseEntTIM(), "EntEagleT", "BaseEntityT", EntEagleT::CreateInstance, NULL /*MethodsList*/);


EntEagleT::EntEagleT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  EntityStateT(Params.Origin,
                               VectorT(),
                               BoundingBox3T<double>(VectorT( 100.0,  100.0,  100.0),
                                                     VectorT(-100.0, -100.0, -100.0)),
                               0,       // Heading (Ergibt sich aus den "angles" in den PropertyPairs, die vom BaseEntity ausgewertet werden!)
                               0,
                               0,
                               0,
                               0,
                               0,       // ModelIndex
                               0,       // ModelSequNr
                               0.0,     // ModelFrameNr
                               40,      // Health
                               0,       // Armor
                               0,       // HaveItems
                               0,       // HaveWeapons
                               0,       // ActiveWeaponSlot
                               0,       // ActiveWeaponSequNr
                               0.0)),   // ActiveWeaponFrameNr
      m_Model(Params.GameWorld->GetModel("Games/DeathMatch/Models/LifeForms/Eagle.mdl")),
      FlightState(CruiseFlight),
      OldOrigin(),
      LoopCenter(),
      FigureDistance(0.0),
      FigureLeft(0.0),
      TimeUntilNextCry(15.0),
      EagleCry(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Ambient/Jungle")))
{
}


EntEagleT::~EntEagleT()
{
    // Release sound.
    SoundSystem->DeleteSound(EagleCry);
}


void EntEagleT::Think(float FrameTime, unsigned long /*ServerFrameNr*/)
{
    const float Pi              =3.14159265359f;
    const float ManeuverDistance=7000.0;            // Wieviel Platz voraus wir gerne für unser Wendemanöver hätten

    float FlightDistance   =4000.0f*FrameTime;      // Wie weit wir gerne fliegen würden
    char  MaxManeuverRepeat=2;

    while (MaxManeuverRepeat--)
    {
        switch (FlightState)
        {
            case CruiseFlight:
            {
                // State.Heading+=1820.0*FrameTime;    // Kreise mit ca. 10 Grad pro Sekunde

                cf::ClipSys::TraceResultT Result(1.0);
                const Vector3dT           FlightDir=Vector3dT(LookupTables::Angle16ToSin[State.Heading], LookupTables::Angle16ToCos[State.Heading], 0.0);

                GameWorld->GetClipWorld().TraceRay(State.Origin, FlightDir*999999.9, MaterialT::Clip_Players, NULL, Result);

                const float TerrainDistance=float(999999.9*Result.Fraction);

                if (TerrainDistance>ManeuverDistance+FlightDistance)
                {
                    // Continue cruise flight
                    State.Origin.x+=LookupTables::Angle16ToSin[State.Heading]*FlightDistance;
                    State.Origin.y+=LookupTables::Angle16ToCos[State.Heading]*FlightDistance;

                    FigureLeft=1.0;     // Muß einen Wert größer 0 zuweisen, damit die while-Schleife verlassen wird!
                    break;
                }

                // Uh! Terrain ahead! Initiate emergency turn maneuver!
                // 80% der verbleibenden Strecke für das Gesamtmanöver nutzen, und davon jeweils 1/3 für die einzelnen Figuren.
                // State.Origin  =VectorT(5000.0, 45000.0, -4000.0);    // Activate this for debugging in JrBase1
                FigureDistance=TerrainDistance*0.8f/3.0f;
                OldOrigin     =State.Origin;
                LoopCenter    =State.Origin+VectorT(2.0*FigureDistance*LookupTables::Angle16ToSin[State.Heading],
                                                    2.0*FigureDistance*LookupTables::Angle16ToCos[State.Heading],
                                                       -FigureDistance);

                // Zustandswechsel nach ControlledCruise vorbereiten
                FigureLeft =2.0f*FigureDistance;
                FlightState=ControlledCruise;
                // Intentional fall-through!
            }

            case ControlledCruise:
                FigureLeft-=FlightDistance;

                State.Origin.x+=LookupTables::Angle16ToSin[State.Heading]*FlightDistance;
                State.Origin.y+=LookupTables::Angle16ToCos[State.Heading]*FlightDistance;

                if (FigureLeft>0.0) break;

                // Zustandswechsel nach HalfLoopAndRoll vorbereiten
                FigureLeft+=FlightDistance+Pi*FigureDistance;
                FlightState=HalfLoopAndRoll;
                // Intentional fall-through!

            case HalfLoopAndRoll:
            {
                FigureLeft-=FlightDistance;

                const unsigned short DegreesLeft=(unsigned short)(FigureLeft/FigureDistance/Pi*32768.0);
                const float          HorLoopPos =LookupTables::Angle16ToSin[DegreesLeft]*FigureDistance;    // Nur eine Abkürzung

                State.Origin.x=LoopCenter.x+LookupTables::Angle16ToSin[State.Heading]*HorLoopPos;
                State.Origin.y=LoopCenter.y+LookupTables::Angle16ToCos[State.Heading]*HorLoopPos;
                State.Origin.z=LoopCenter.z-                                          LookupTables::Angle16ToCos[DegreesLeft]*FigureDistance;   // VerLoopPos
                State.Pitch   =32768-DegreesLeft;
                State.Bank    =State.Pitch;

                if (FigureLeft>0.0) break;

                // Zustandswechsel nach ClimpBackToCruiseAlt vorbereiten
                State.Heading+=32768;
                State.Pitch   =0;
                State.Bank    =0;

                // Die nächste Figur ist der "Cosinus-Aufschwung" auf die alte Flughöhe.
                // Auf geradem Wege würde eine Diagonale abgeflogen, deren Länge 2.0*1.414213562373*FigureDistance beträgt.
                // Der Cosinus-Bogen ist aber offensichtich etwas länger, schätze ihn mit 2.0*1.5*FigureDistance ab.
                FigureLeft+=FlightDistance+3.0f*FigureDistance;
                FlightState=ClimpBackToCruiseAlt;
                // Intentional fall-through!
            }

            case ClimpBackToCruiseAlt:
            {
                FigureLeft-=FlightDistance;

                const float          GroundDistLeft=FigureLeft/1.5f;                                            // Wieviel "über Grund" noch abzufliegen ist
                const unsigned short DegreesLeft   =(unsigned short)(GroundDistLeft/FigureDistance*16384.0f);   // Wieviel "Degrees" dies entspricht

                State.Origin.x=OldOrigin.x-LookupTables::Angle16ToSin[State.Heading]*GroundDistLeft;    // Beachte: Wir sind jetzt auf Umkehrkurs!
                State.Origin.y=OldOrigin.y-LookupTables::Angle16ToCos[State.Heading]*GroundDistLeft;
                State.Origin.z=OldOrigin.z+FigureDistance*(LookupTables::Angle16ToCos[DegreesLeft]-1.0f);

                if (FigureLeft>0.0) break;

                // Zustandswechsel nach CruiseFlight vorbereiten
                State.Origin  = OldOrigin;
                // State.Heading+= rand() >> 4;
                FlightDistance=-FigureLeft;
                FlightState   = CruiseFlight;
                // Intentional fall-through!
            }
        }

        if (FigureLeft>0.0) break;
    }
}


void EntEagleT::Draw(bool /*FirstPersonView*/, float LodDist) const
{
    Vector3fT   LgtPos(MatSys::Renderer->GetCurrentLightSourcePosition());
    Vector3fT   EyePos(MatSys::Renderer->GetCurrentEyePosition());

    const float DegPitch=float(State.Pitch)/8192.0f*45.0f;
    const float DegBank =float(State.Bank )/8192.0f*45.0f;

    LgtPos=LgtPos.GetRotY(-DegPitch);
    EyePos=EyePos.GetRotY(-DegPitch);
    MatSys::Renderer->RotateY(MatSys::RendererI::MODEL_TO_WORLD, DegPitch);

    LgtPos=LgtPos.GetRotX(-DegBank);
    EyePos=EyePos.GetRotX(-DegBank);
    MatSys::Renderer->RotateX(MatSys::RendererI::MODEL_TO_WORLD, DegBank);

    MatSys::Renderer->SetCurrentLightSourcePosition(LgtPos.x, LgtPos.y, LgtPos.z);
    MatSys::Renderer->SetCurrentEyePosition(EyePos.x, EyePos.y, EyePos.z);

    AnimPoseT* Pose=m_Model->GetSharedPose(m_Model->GetAnimExprPool().GetStandard(State.ModelSequNr, State.ModelFrameNr));
    Pose->Draw(-1 /*default skin*/, LodDist);
}


void EntEagleT::PostDraw(float FrameTime, bool /*FirstPersonView*/)
{
    IntrusivePtrT<AnimExprStandardT> StdAE=m_Model->GetAnimExprPool().GetStandard(State.ModelSequNr, State.ModelFrameNr);
    StdAE->AdvanceTime(FrameTime, true);
    State.ModelFrameNr=StdAE->GetFrameNr();

    // Update sound position and velocity.
    EagleCry->SetPosition(State.Origin);
    EagleCry->SetVelocity(State.Velocity);

    TimeUntilNextCry-=FrameTime;
    if (TimeUntilNextCry<0.0)
    {
        EagleCry->Play();
        TimeUntilNextCry=20.0;
    }
}
