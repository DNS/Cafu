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

#include "Eagle.hpp"
#include "EntityCreateParams.hpp"
#include "Interpolator.hpp"
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

using namespace GAME_NAME;


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
                  BoundingBox3dT(Vector3dT( 4.0,  4.0,  4.0),
                                 Vector3dT(-4.0, -4.0, -4.0)),
                  0),
      m_Model(Params.GameWorld->GetModel("Games/DeathMatch/Models/LifeForms/Eagle/Eagle.cmdl")),
      FlightState(CruiseFlight),
      OldOrigin(),
      LoopCenter(),
      FigureDistance(0.0),
      FigureLeft(0.0),
      m_ModelSequNr(0),
      m_ModelFrameNr(0.0f),
      m_TimeUntilNextCry(15.0),
      m_EagleCry(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Ambient/Jungle")))
{
    Register(new InterpolatorT<Vector3dT>(m_Origin));
}


EntEagleT::~EntEagleT()
{
    // Release sound.
    SoundSystem->DeleteSound(m_EagleCry);
}


float EntEagleT::fnCruiseFlight(float FlightLeft)
{
    // m_Heading+=1820.0*FrameTime;    // Kreise mit ca. 10 Grad pro Sekunde

    cf::ClipSys::TraceResultT Result(1.0);
    const Vector3dT           FlightDir = Vector3dT(LookupTables::Angle16ToSin[m_Heading], LookupTables::Angle16ToCos[m_Heading], 0.0);

    GameWorld->GetClipWorld().TraceRay(m_Origin, FlightDir * 1000.0, MaterialT::Clip_Players, NULL, Result);

    const float TerrainDistance  = float(1000.0*Result.Fraction);
    const float ManeuverDistance = 280.0;            // Wieviel Platz voraus wir gerne für unser Wendemanöver hätten

    if (TerrainDistance > ManeuverDistance + FlightLeft)
    {
        // Continue cruise flight
        m_Origin.x += LookupTables::Angle16ToSin[m_Heading] * FlightLeft;
        m_Origin.y += LookupTables::Angle16ToCos[m_Heading] * FlightLeft;

        return 0.0;
    }

    // Uh! Terrain ahead! Initiate emergency turn maneuver!
    // 80% der verbleibenden Strecke für das Gesamtmanöver nutzen, und davon jeweils 1/3 für die einzelnen Figuren.
    FigureDistance = TerrainDistance * 0.8f / 3.0f;
    OldOrigin      = m_Origin;
    LoopCenter     = m_Origin+VectorT(2.0 * FigureDistance * LookupTables::Angle16ToSin[m_Heading],
                                      2.0 * FigureDistance * LookupTables::Angle16ToCos[m_Heading],
                                           -FigureDistance);

    // Zustandswechsel nach ControlledCruise vorbereiten
    FlightState = ControlledCruise;
    FigureLeft  = 2.0f * FigureDistance;

    return FlightLeft;
}


float EntEagleT::fnControlledCruise(float FlightLeft)
{
    if (FlightLeft > FigureLeft)
    {
        const float Pi = 3.14159265359f;

        FlightState = HalfLoopAndRoll;
        FigureLeft = Pi * FigureDistance;

        return FlightLeft - FigureLeft;
    }

    FigureLeft -= FlightLeft;

    m_Origin.x += LookupTables::Angle16ToSin[m_Heading] * FlightLeft;
    m_Origin.y += LookupTables::Angle16ToCos[m_Heading] * FlightLeft;

    return 0.0f;
}


float EntEagleT::fnHalfLoopAndRoll(float FlightLeft)
{
    const float Pi = 3.14159265359f;

    if (FlightLeft > FigureLeft)
    {
        // Zustandswechsel nach ClimbBackToCruiseAlt vorbereiten
        m_Heading += 32768;
        m_Pitch    = 0;
        m_Bank     = 0;

        // Die nächste Figur ist der "Cosinus-Aufschwung" auf die alte Flughöhe.
        // Auf geradem Wege würde eine Diagonale abgeflogen, deren Länge 2.0*1.414213562373*FigureDistance beträgt.
        // Der Cosinus-Bogen ist aber offensichtich etwas länger, schätze ihn mit 2.0*1.5*FigureDistance ab.
        FlightState = ClimbBackToCruiseAlt;
        FigureLeft = 3.0f * FigureDistance;

        return FlightLeft - FigureLeft;
    }

    FigureLeft -= FlightLeft;

    const unsigned short DegreesLeft=(unsigned short)(FigureLeft/FigureDistance/Pi*32768.0);
    const float          HorLoopPos =LookupTables::Angle16ToSin[DegreesLeft]*FigureDistance;    // Nur eine Abkürzung

    m_Origin.x = LoopCenter.x + LookupTables::Angle16ToSin[m_Heading] * HorLoopPos;
    m_Origin.y = LoopCenter.y + LookupTables::Angle16ToCos[m_Heading] * HorLoopPos;
    m_Origin.z = LoopCenter.z - LookupTables::Angle16ToCos[DegreesLeft] * FigureDistance;   // VerLoopPos
    m_Pitch    = 32768 - DegreesLeft;
    m_Bank     = m_Pitch;

    return 0.0f;
}


float EntEagleT::fnClimbBackToCruiseAlt(float FlightLeft)
{
    if (FlightLeft > FigureLeft)
    {
        // Zustandswechsel nach CruiseFlight vorbereiten
        m_Origin  = OldOrigin;

        FlightState = CruiseFlight;
        FigureLeft = 0.0;   // Not relevant in CruiseFlight.

        return FlightLeft - FigureLeft;
    }

    FigureLeft -= FlightLeft;

    const float          GroundDistLeft = FigureLeft/1.5f;                                              // Wieviel "über Grund" noch abzufliegen ist
    const unsigned short DegreesLeft    = (unsigned short)(GroundDistLeft / FigureDistance * 16384.0f); // Wieviel "Degrees" dies entspricht

    m_Origin.x = OldOrigin.x - LookupTables::Angle16ToSin[m_Heading]*GroundDistLeft;    // Beachte: Wir sind jetzt auf Umkehrkurs!
    m_Origin.y = OldOrigin.y - LookupTables::Angle16ToCos[m_Heading]*GroundDistLeft;
    m_Origin.z = OldOrigin.z + FigureDistance * (LookupTables::Angle16ToCos[DegreesLeft] - 1.0f);

    return 0.0f;
}


void EntEagleT::Think(float FrameTime, unsigned long /*ServerFrameNr*/)
{
    float FlightLeft = 160.0f * FrameTime;      // Wie weit wir gerne fliegen würden

    while (FlightLeft > 0.0f)
    {
        switch (FlightState)
        {
            case CruiseFlight:
                FlightLeft = fnCruiseFlight(FlightLeft);
                break;

            case ControlledCruise:
                FlightLeft = fnControlledCruise(FlightLeft);
                break;

            case HalfLoopAndRoll:
                FlightLeft = fnHalfLoopAndRoll(FlightLeft);
                break;

            case ClimbBackToCruiseAlt:
                FlightLeft = fnClimbBackToCruiseAlt(FlightLeft);
                break;
        }
    }
}


void EntEagleT::Draw(bool /*FirstPersonView*/, float LodDist) const
{
    Vector3fT   LgtPos(MatSys::Renderer->GetCurrentLightSourcePosition());
    Vector3fT   EyePos(MatSys::Renderer->GetCurrentEyePosition());

    const float DegPitch=float(m_Pitch)/8192.0f*45.0f;
    const float DegBank =float(m_Bank )/8192.0f*45.0f;

    LgtPos=LgtPos.GetRotY(-DegPitch);
    EyePos=EyePos.GetRotY(-DegPitch);
    MatSys::Renderer->RotateY(MatSys::RendererI::MODEL_TO_WORLD, DegPitch);

    LgtPos=LgtPos.GetRotX(-DegBank);
    EyePos=EyePos.GetRotX(-DegBank);
    MatSys::Renderer->RotateX(MatSys::RendererI::MODEL_TO_WORLD, DegBank);

    MatSys::Renderer->SetCurrentLightSourcePosition(LgtPos.x, LgtPos.y, LgtPos.z);
    MatSys::Renderer->SetCurrentEyePosition(EyePos.x, EyePos.y, EyePos.z);

    AnimPoseT* Pose=m_Model->GetSharedPose(m_Model->GetAnimExprPool().GetStandard(m_ModelSequNr, m_ModelFrameNr));
    Pose->Draw(-1 /*default skin*/, LodDist);
}


void EntEagleT::PostDraw(float FrameTime, bool /*FirstPersonView*/)
{
    IntrusivePtrT<AnimExprStandardT> StdAE=m_Model->GetAnimExprPool().GetStandard(m_ModelSequNr, m_ModelFrameNr);

    StdAE->SetForceLoop(true);
    StdAE->AdvanceTime(FrameTime);

    m_ModelFrameNr=StdAE->GetFrameNr();

    // Update sound position and velocity.
    m_EagleCry->SetPosition(m_Origin);
    // m_EagleCry->SetVelocity(State.Velocity);

    m_TimeUntilNextCry-=FrameTime;

    if (m_TimeUntilNextCry<0.0)
    {
        m_EagleCry->Play();
        m_TimeUntilNextCry=20.0;
    }
}
