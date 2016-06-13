/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "UpdateTriangle.hpp"
#include "../ModelDocument.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


CommandUpdateTriangleT::CommandUpdateTriangleT(ModelDocumentT* ModelDoc, unsigned int MeshNr, unsigned int TriNr, bool SkipDraw)
    : m_ModelDoc(ModelDoc),
      m_MeshNr(MeshNr),
      m_TriNr(TriNr),
      m_NewSkipDraw(SkipDraw),
      m_OldSkipDraw(m_ModelDoc->GetModel()->GetMeshes()[m_MeshNr].Triangles[m_TriNr].SkipDraw)
{
}


bool CommandUpdateTriangleT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If the triangle didn't really change, don't put this command into the command history.
    if (m_NewSkipDraw==m_OldSkipDraw) return false;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Meshes[m_MeshNr].Triangles[m_TriNr].SkipDraw,
    // because it's bound to become invalid whenever another command meddles with the arrays.
    m_ModelDoc->GetModel()->m_Meshes[m_MeshNr].Triangles[m_TriNr].SkipDraw=m_NewSkipDraw;

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetAnimState().Pose.SetNeedsRecache();

    m_ModelDoc->UpdateAllObservers_MeshChanged(m_MeshNr);
    m_Done=true;
    return true;
}


void CommandUpdateTriangleT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Meshes[m_MeshNr].Triangles[m_TriNr].SkipDraw,
    // because it's bound to become invalid whenever another command meddles with the arrays.
    m_ModelDoc->GetModel()->m_Meshes[m_MeshNr].Triangles[m_TriNr].SkipDraw=m_OldSkipDraw;

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetAnimState().Pose.SetNeedsRecache();

    m_ModelDoc->UpdateAllObservers_MeshChanged(m_MeshNr);
    m_Done=false;
}


wxString CommandUpdateTriangleT::GetName() const
{
    return m_NewSkipDraw ? "Hide Triangle" : "Show Triangle";
}
