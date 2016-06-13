/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "SetMeshTSMethod.hpp"
#include "../ModelDocument.hpp"


using namespace ModelEditor;


CommandSetMeshTSMethodT::CommandSetMeshTSMethodT(ModelDocumentT* ModelDoc, unsigned int MeshNr, CafuModelT::MeshT::TangentSpaceMethodT NewTSMethod)
    : m_ModelDoc(ModelDoc),
      m_MeshNr(MeshNr),
      m_NewTSMethod(NewTSMethod),
      m_OldTSMethod(m_ModelDoc->GetModel()->GetMeshes()[m_MeshNr].TSMethod)
{
}


bool CommandSetMeshTSMethodT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If the TSMethod didn't really change, don't put this command into the command history.
    if (m_NewTSMethod==m_OldTSMethod) return false;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Meshes[m_MeshNr] as a member,
    // because it's bound to become invalid whenever another command meddles with the array of meshes.
    CafuModelT::MeshT& Mesh=m_ModelDoc->GetModel()->m_Meshes[m_MeshNr];

    Mesh.TSMethod=m_NewTSMethod;

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetAnimState().Pose.SetNeedsRecache();

    m_ModelDoc->UpdateAllObservers_MeshChanged(m_MeshNr);
    m_Done=true;
    return true;
}


void CommandSetMeshTSMethodT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Meshes[m_MeshNr] as a member,
    // because it's bound to become invalid whenever another command meddles with the array of meshes.
    CafuModelT::MeshT& Mesh=m_ModelDoc->GetModel()->m_Meshes[m_MeshNr];

    Mesh.TSMethod=m_OldTSMethod;

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetAnimState().Pose.SetNeedsRecache();

    m_ModelDoc->UpdateAllObservers_MeshChanged(m_MeshNr);
    m_Done=false;
}


wxString CommandSetMeshTSMethodT::GetName() const
{
    return "Set tangent-space method";
}
