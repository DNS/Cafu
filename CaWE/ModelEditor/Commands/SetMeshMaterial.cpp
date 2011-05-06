/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#include "SetMeshMaterial.hpp"
#include "../ModelDocument.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


CommandSetMeshMaterialT::CommandSetMeshMaterialT(ModelDocumentT* ModelDoc, unsigned int MeshNr, const wxString& NewName)
    : m_ModelDoc(ModelDoc),
      m_MeshNr(MeshNr),
      m_NewMat(m_ModelDoc->GetModel()->GetMaterialManager().GetMaterial(NewName.ToStdString())),
      m_OldMat(m_ModelDoc->GetModel()->GetMeshes()[m_MeshNr].Material)
{
}


bool CommandSetMeshMaterialT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Cannot assign a material that doesn't exist in the models material manager.
    if (m_NewMat==NULL) return false;

    // If the material didn't really change, don't put this command into the command history.
    if (m_NewMat==m_OldMat) return false;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Meshes[m_MeshNr] as a member,
    // because it's bound to become invalid whenever another command meddles with the array of meshes.
    CafuModelT::MeshT& Mesh=m_ModelDoc->GetModel()->m_Meshes[m_MeshNr];

    Mesh.Material=m_NewMat;

    if (MatSys::Renderer!=NULL)
    {
        MatSys::Renderer->FreeMaterial(Mesh.RenderMaterial);
        Mesh.RenderMaterial=MatSys::Renderer->RegisterMaterial(Mesh.Material);
    }

    // m_ModelDoc->UpdateAllObservers_MeshChanged(m_MeshNr);
    m_Done=true;
    return true;
}


void CommandSetMeshMaterialT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Meshes[m_MeshNr] as a member,
    // because it's bound to become invalid whenever another command meddles with the array of meshes.
    CafuModelT::MeshT& Mesh=m_ModelDoc->GetModel()->m_Meshes[m_MeshNr];

    Mesh.Material=m_OldMat;

    if (MatSys::Renderer!=NULL)
    {
        MatSys::Renderer->FreeMaterial(Mesh.RenderMaterial);
        Mesh.RenderMaterial=MatSys::Renderer->RegisterMaterial(Mesh.Material);
    }

    // m_ModelDoc->UpdateAllObservers_MeshChanged(m_MeshNr);
    m_Done=false;
}


wxString CommandSetMeshMaterialT::GetName() const
{
    return "Assign material";
}
