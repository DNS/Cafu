/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "PlantNode.hpp"
#include "_aux.hpp"

#include "Plants/PlantDescription.hpp"
#include "Plants/PlantDescrMan.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "TextParser/TextParser.hpp"
#include "Math3D/Matrix.hpp"


cf::SceneGraph::PlantNodeT::PlantNodeT()
    : m_Tree(),
      m_RandomSeed(0),
      m_Position(),
      m_DescrFileName(""),
      m_Bounds(m_Tree.GetTreeBounds().AsBoxOfDouble())
{
}


cf::SceneGraph::PlantNodeT::PlantNodeT(const PlantDescriptionT* PlantDescription, unsigned long RandomSeed, const Vector3dT& Position, const Vector3fT& Angles)
    : m_Tree(PlantDescription, RandomSeed),
      m_RandomSeed(RandomSeed),
      m_Position(Position),
      m_Angles(Angles),
      m_DescrFileName(PlantDescription->FileName),
      m_Bounds(m_Tree.GetTreeBounds().AsBoxOfDouble())
{
}


cf::SceneGraph::PlantNodeT* cf::SceneGraph::PlantNodeT::CreateFromFile_cw(std::istream& InFile, aux::PoolT& Pool, LightMapManT& /*LMM*/, SHLMapManT& /*SMM*/, PlantDescrManT& PDM)
{
    PlantNodeT* PlantNode=new PlantNodeT();

    PlantNode->m_DescrFileName=Pool.ReadString(InFile);
    PlantNode->m_RandomSeed   =aux::ReadUInt32(InFile);
    PlantNode->m_Position     =aux::ReadVector3d(InFile);
    PlantNode->m_Angles       =Pool.ReadVector3f(InFile);

    PlantNode->m_Tree=TreeT(PDM.GetPlantDescription(PlantNode->m_DescrFileName), PlantNode->m_RandomSeed);

    PlantNode->m_Bounds=PlantNode->m_Tree.GetTreeBounds().AsBoxOfDouble();

    return PlantNode;
}


cf::SceneGraph::PlantNodeT::~PlantNodeT()
{
}


void cf::SceneGraph::PlantNodeT::WriteTo(std::ostream& OutFile, aux::PoolT& Pool) const
{
    aux::Write(OutFile, "Plant");

    Pool.Write(OutFile, m_DescrFileName);
    aux::Write(OutFile, aux::cnc_ui32(m_RandomSeed));
    aux::Write(OutFile, m_Position);
    Pool.Write(OutFile, m_Angles);
}


const BoundingBox3T<double>& cf::SceneGraph::PlantNodeT::GetBoundingBox() const
{
    return m_Bounds;
}


void cf::SceneGraph::PlantNodeT::DrawAmbientContrib(const Vector3dT& ViewerPos) const
{
    // Nothing to do here. We could draw the root and all its branches here however.
}


void cf::SceneGraph::PlantNodeT::DrawTranslucentContrib(const Vector3dT& ViewerPos) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT);

    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);

        MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, float(m_Position.x), float(m_Position.y), float(m_Position.z));
        MatSys::Renderer->RotateZ(MatSys::RendererI::MODEL_TO_WORLD, m_Angles.z);
        MatSys::Renderer->RotateY(MatSys::RendererI::MODEL_TO_WORLD, m_Angles.y);
        MatSys::Renderer->RotateX(MatSys::RendererI::MODEL_TO_WORLD, m_Angles.x);
        MatSys::Renderer->Scale(MatSys::RendererI::MODEL_TO_WORLD, 1000.0f); // Scale tree units (meters) to Cafu units (mm).

        m_Tree.Draw();

    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
}
