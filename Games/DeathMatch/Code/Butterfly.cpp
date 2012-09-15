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

#include "Butterfly.hpp"
#include "TypeSys.hpp"
#include "EntityCreateParams.hpp"
#include "Interpolator.hpp"
#include "Libs/LookupTables.hpp"
#include "../../GameWorld.hpp"
#include "Models/Model_cmdl.hpp"

using namespace GAME_NAME;


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntButterflyT::GetType() const
{
    return &TypeInfo;
 // return &EntButterflyT::TypeInfo;
}

void* EntButterflyT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntButterflyT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const cf::TypeSys::TypeInfoT EntButterflyT::TypeInfo(GetBaseEntTIM(), "EntButterflyT", "BaseEntityT", EntButterflyT::CreateInstance, NULL /*MethodsList*/);


EntButterflyT::EntButterflyT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  BoundingBox3dT(Vector3dT( 100.0,  100.0,  100.0),
                                 Vector3dT(-100.0, -100.0, -100.0)),
                  0),
      m_Model(Params.GameWorld->GetModel("Games/DeathMatch/Models/LifeForms/Butterfly/Butterfly.cmdl")),
      m_ArcCenter(Params.Origin),
      m_ArcRadius(500.0f),
      m_ModelSequNr(0),
      m_ArcPos(0),
      m_ModelFrameNr(0.0f)
{
    Register(new ExtrapolatorT<Vector3dT>(m_Origin));
    Think(0.0f, 0);
}


void EntButterflyT::Think(float FrameTime, unsigned long /*ServerFrameNr*/)
{
    const float DegPerSecond = 7300.0f;     // Entspricht 0.7 RadPerSecond oder 40 GradPerSecond

    m_ArcPos += (unsigned short)(DegPerSecond*FrameTime);   // "wraps" automatically.

    // Info: Die Bogenlänge zwischen 'ArcPos' und 'ArcPos+1' bei 'ArcRadius==500.0' beträgt 0.048,
    // die "Auflösung" ist also mehr als ausreichend!
    m_Origin.x = m_ArcCenter.x + LookupTables::Angle16ToSin[m_ArcPos]*m_ArcRadius;
    m_Origin.y = m_ArcCenter.y + LookupTables::Angle16ToCos[m_ArcPos]*m_ArcRadius;
    m_Origin.z = m_ArcCenter.z + LookupTables::Angle16ToSin[(unsigned short)(m_ArcPos*2)]*m_ArcRadius*0.2f;

    m_Heading = m_ArcPos + 16384;
}


void EntButterflyT::Draw(bool /*FirstPersonView*/, float LodDist) const
{
    AnimPoseT* Pose=m_Model->GetSharedPose(m_Model->GetAnimExprPool().GetStandard(m_ModelSequNr, m_ModelFrameNr));
    int        SkinNr=-1;   // -1 is the default skin.

    if (m_Model->GetSkins().Size() > 0)
        SkinNr = ID % m_Model->GetSkins().Size();

    Pose->Draw(SkinNr, LodDist);
}


void EntButterflyT::PostDraw(float FrameTime, bool /*FirstPersonView*/)
{
    // Implicit simple "mini-prediction".
    IntrusivePtrT<AnimExprStandardT> AnimExpr = m_Model->GetAnimExprPool().GetStandard(m_ModelSequNr, m_ModelFrameNr);

    AnimExpr->SetForceLoop(true);
    AnimExpr->AdvanceTime(FrameTime);

    m_ModelFrameNr = AnimExpr->GetFrameNr();
}
