/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "UpdateUVCoords.hpp"
#include "../ModelDocument.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


CommandUpdateUVCoordsT::CommandUpdateUVCoordsT(ModelDocumentT* ModelDoc, unsigned int MeshNr, const AnimPoseT& Pose, const Vector3fT& u, const Vector3fT& v)
    : m_ModelDoc(ModelDoc),
      m_MeshNr(MeshNr),
      m_NewUVs(GetNewUVs(Pose, u, v)),
      m_OldUVs(GetOldUVs())
{
}


CommandUpdateUVCoordsT::CommandUpdateUVCoordsT(ModelDocumentT* ModelDoc, unsigned int MeshNr, const ArrayT<CoordT>& NewUVs)
    : m_ModelDoc(ModelDoc),
      m_MeshNr(MeshNr),
      m_NewUVs(NewUVs),
      m_OldUVs(GetOldUVs())
{
}


bool CommandUpdateUVCoordsT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If the UV-coordinates didn't really change, don't put this command into the command history.
    if (m_NewUVs==m_OldUVs) return false;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Meshes[m_MeshNr].Vertices,
    // because it's bound to become invalid whenever another command meddles with the arrays.
    ArrayT<CafuModelT::MeshT::VertexT>& Vertices=m_ModelDoc->GetModel()->m_Meshes[m_MeshNr].Vertices;

    for (unsigned long VertexNr=0; VertexNr<Vertices.Size(); VertexNr++)
    {
        Vertices[VertexNr].u=m_NewUVs[VertexNr].u;
        Vertices[VertexNr].v=m_NewUVs[VertexNr].v;
    }

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetAnimState().Pose.SetNeedsRecache();

    m_ModelDoc->UpdateAllObservers_MeshChanged(m_MeshNr);
    m_Done=true;
    return true;
}


void CommandUpdateUVCoordsT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Meshes[m_MeshNr].Vertices,
    // because it's bound to become invalid whenever another command meddles with the arrays.
    ArrayT<CafuModelT::MeshT::VertexT>& Vertices=m_ModelDoc->GetModel()->m_Meshes[m_MeshNr].Vertices;

    for (unsigned long VertexNr=0; VertexNr<Vertices.Size(); VertexNr++)
    {
        Vertices[VertexNr].u=m_OldUVs[VertexNr].u;
        Vertices[VertexNr].v=m_OldUVs[VertexNr].v;
    }

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetAnimState().Pose.SetNeedsRecache();

    m_ModelDoc->UpdateAllObservers_MeshChanged(m_MeshNr);
    m_Done=false;
}


wxString CommandUpdateUVCoordsT::GetName() const
{
    return "Update UV-coords";
}


ArrayT<CommandUpdateUVCoordsT::CoordT> CommandUpdateUVCoordsT::GetNewUVs(const AnimPoseT& Pose, const Vector3fT& u, const Vector3fT& v) const
{
    const ArrayT<AnimPoseT::MeshInfoT::VertexT>& Vertices=Pose.GetMeshInfos()[m_MeshNr].Vertices;

    ArrayT<CoordT> UVs;
    UVs.PushBackEmptyExact(Vertices.Size());

    const float uu=dot(u, u);
    const float vv=dot(v, v);

    for (unsigned long VertexNr=0; VertexNr<Vertices.Size(); VertexNr++)
    {
        UVs[VertexNr].u=dot(Vertices[VertexNr].Pos, u) / uu;
        UVs[VertexNr].v=dot(Vertices[VertexNr].Pos, v) / vv;
    }

    return UVs;
}


ArrayT<CommandUpdateUVCoordsT::CoordT> CommandUpdateUVCoordsT::GetOldUVs() const
{
    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Meshes[m_MeshNr].Vertices,
    // because it's bound to become invalid whenever another command meddles with the arrays.
    const ArrayT<CafuModelT::MeshT::VertexT>& Vertices=m_ModelDoc->GetModel()->m_Meshes[m_MeshNr].Vertices;

    ArrayT<CoordT> UVs;
    UVs.PushBackEmptyExact(Vertices.Size());

    for (unsigned long VertexNr=0; VertexNr<Vertices.Size(); VertexNr++)
    {
        UVs[VertexNr].u=Vertices[VertexNr].u;
        UVs[VertexNr].v=Vertices[VertexNr].v;
    }

    return UVs;
}
