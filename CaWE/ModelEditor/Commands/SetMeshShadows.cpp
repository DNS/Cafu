/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "SetMeshShadows.hpp"
#include "../ModelDocument.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


CommandSetMeshShadowsT::CommandSetMeshShadowsT(ModelDocumentT* ModelDoc, unsigned int MeshNr, bool NewCastShadows)
    : m_ModelDoc(ModelDoc),
      m_MeshNr(MeshNr),
      m_NewCastShadows(NewCastShadows),
      m_OldCastShadows(m_ModelDoc->GetModel()->GetMeshes()[m_MeshNr].CastShadows)
{
}


bool CommandSetMeshShadowsT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If the CastShadows didn't really change, don't put this command into the command history.
    if (m_NewCastShadows==m_OldCastShadows) return false;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Meshes[m_MeshNr] as a member,
    // because it's bound to become invalid whenever another command meddles with the array of meshes.
    CafuModelT::MeshT& Mesh=m_ModelDoc->GetModel()->m_Meshes[m_MeshNr];

    Mesh.CastShadows=m_NewCastShadows;

    m_ModelDoc->UpdateAllObservers_MeshChanged(m_MeshNr);
    m_Done=true;
    return true;
}


void CommandSetMeshShadowsT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Meshes[m_MeshNr] as a member,
    // because it's bound to become invalid whenever another command meddles with the array of meshes.
    CafuModelT::MeshT& Mesh=m_ModelDoc->GetModel()->m_Meshes[m_MeshNr];

    Mesh.CastShadows=m_OldCastShadows;

    m_ModelDoc->UpdateAllObservers_MeshChanged(m_MeshNr);
    m_Done=false;
}


wxString CommandSetMeshShadowsT::GetName() const
{
    return "Set mesh shadows";
}
