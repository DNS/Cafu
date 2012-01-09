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

#include "ModelNode.hpp"
#include "_aux.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Models/Model_cmdl.hpp"
#include "Models/ModelManager.hpp"


cf::SceneGraph::ModelNodeT::ModelNodeT()
    : m_Model(NULL),
      m_Label(""),
      m_Origin(),
      m_Angles(),
      m_Scale(0.0f),
      m_SeqNumber(0),
      m_FrameOffset(0.0f),
      m_FrameTimeScale(1.0f),
      m_FrameNumber(0.0f),
      m_Animate(false),
      m_Timer()
{
}


cf::SceneGraph::ModelNodeT::ModelNodeT(const CafuModelT* Model, const std::string& Label, const Vector3fT& Origin, const Vector3fT& Angles, float Scale, int SeqNumber, float FrameOffset, float FrameTimeScale, bool Animate)
    : m_Model(Model),
      m_Label(Label),
      m_Origin(Origin),
      m_Angles(Angles),
      m_Scale(Scale),
      m_SeqNumber(SeqNumber),
      m_FrameOffset(FrameOffset),
      m_FrameTimeScale(FrameTimeScale),
      m_FrameNumber(m_FrameOffset),
      m_Animate(Animate),
      m_Timer()
{
}


cf::SceneGraph::ModelNodeT* cf::SceneGraph::ModelNodeT::CreateFromFile_cw(std::istream& InFile, aux::PoolT& Pool, ModelManagerT& ModelMan)
{
    ModelNodeT* ModelNode=new ModelNodeT();

    ModelNode->m_Model    =ModelMan.GetModel(Pool.ReadString(InFile));
    ModelNode->m_Label    =aux::ReadString(InFile);
    ModelNode->m_Origin   =aux::ReadVector3f(InFile);

    ModelNode->m_Angles   =Pool.ReadVector3f(InFile);

    ModelNode->m_Scale    =aux::ReadFloat(InFile);
    ModelNode->m_SeqNumber=aux::ReadInt32(InFile);

    ModelNode->m_FrameOffset   =aux::ReadFloat(InFile);
    ModelNode->m_FrameTimeScale=aux::ReadFloat(InFile);

    ModelNode->m_FrameNumber=ModelNode->m_FrameOffset;

    InFile.read((char*)&ModelNode->m_Animate, sizeof(ModelNode->m_Animate));

    return ModelNode;
}


cf::SceneGraph::ModelNodeT::~ModelNodeT()
{
}


void cf::SceneGraph::ModelNodeT::WriteTo(std::ostream& OutFile, aux::PoolT& Pool) const
{
    aux::Write(OutFile, "Model");

    Pool.Write(OutFile, m_Model->GetFileName());
    aux::Write(OutFile, m_Label);
    aux::Write(OutFile, m_Origin);

    Pool.Write(OutFile, m_Angles);

    aux::Write(OutFile, m_Scale);
    aux::Write(OutFile, (int32_t)m_SeqNumber);

    aux::Write(OutFile, m_FrameOffset);
    aux::Write(OutFile, m_FrameTimeScale);

    OutFile.write((char*)&m_Animate, sizeof(m_Animate));
}


const BoundingBox3T<double>& cf::SceneGraph::ModelNodeT::GetBoundingBox() const
{
    static BoundingBox3dT BB;

    BB=m_Model->GetSharedPose(m_SeqNumber, m_FrameNumber)->GetBB().AsBoxOfDouble();

    return BB;
}


void cf::SceneGraph::ModelNodeT::DrawAmbientContrib(const Vector3dT& ViewerPos) const
{
    // Calculate model distance from viewer position.
    float      Distance=length(m_Origin-ViewerPos.AsVectorOfFloat());
    AnimPoseT* Pose    =m_Model->GetSharedPose(m_SeqNumber, m_FrameNumber);

    if (m_Animate)
    {
        Pose->Advance(float(m_Timer.GetSecondsSinceLastCall())*m_FrameTimeScale);

        m_FrameNumber=Pose->GetFrameNr();
    }

    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);

        MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, m_Origin.x, m_Origin.y, m_Origin.z);
        MatSys::Renderer->RotateZ(MatSys::RendererI::MODEL_TO_WORLD, m_Angles.z);
        MatSys::Renderer->RotateY(MatSys::RendererI::MODEL_TO_WORLD, m_Angles.y);
        MatSys::Renderer->RotateX(MatSys::RendererI::MODEL_TO_WORLD, m_Angles.x);
        MatSys::Renderer->Scale(MatSys::RendererI::MODEL_TO_WORLD, m_Scale);

        Pose->Draw(-1 /*default skin*/, Distance);

    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
}
