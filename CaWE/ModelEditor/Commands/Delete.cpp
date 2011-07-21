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

#include "Delete.hpp"
#include "Select.hpp"
#include "../ModelDocument.hpp"
#include "MaterialSystem/Renderer.hpp"


using namespace ModelEditor;


namespace
{
    bool IsLess(const unsigned int& i, const unsigned int& j)
    {
        return i<j;
    }


    ArrayT<unsigned int> GetSorted(ArrayT<unsigned int> A)
    {
        A.QuickSort(IsLess);

        // Remove duplicates.
        for (unsigned long i=0; i+1<A.Size(); i++)
        {
            wxASSERT(A[i] <= A[i+1]);

            if (A[i] == A[i+1])
            {
                A.RemoveAtAndKeepOrder(i);
                i--;
            }
        }

        return A;
    }
}


CommandDeleteT::CommandDeleteT(ModelDocumentT* ModelDoc, ModelElementTypeT Type, const ArrayT<unsigned int>& Indices)
    : m_ModelDoc(ModelDoc),
      m_Type(Type),
      m_Indices(GetSorted(Indices)),
      m_Joints(),
      m_Meshes(),
      m_DrawMs(),
      m_Anims(),
      m_Message(),
      m_CommandSelect(CommandSelectT::Remove(m_ModelDoc, m_Type, m_Indices))
{
    for (unsigned long INr=0; INr<m_Indices.Size(); INr++)
    {
        const unsigned int i=m_Indices[INr];

        switch (m_Type)
        {
            case JOINT: m_Joints.PushBack(m_ModelDoc->GetModel()->GetJoints()[i]); break;
            case MESH:  m_Meshes.PushBack(m_ModelDoc->GetModel()->GetMeshes()[i]);
                        m_DrawMs.PushBack(m_ModelDoc->GetModel()->m_Draw_Meshes[i]); break;
            case ANIM:  m_Anims .PushBack(m_ModelDoc->GetModel()->GetAnims ()[i]); break;
        }
    }
}


CommandDeleteT::~CommandDeleteT()
{
    if (m_Done && MatSys::Renderer!=NULL)
    {
        for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
        {
            MatSys::Renderer->FreeMaterial(m_Meshes[MeshNr].RenderMaterial);
            m_Meshes[MeshNr].RenderMaterial=NULL;
        }
    }

    delete m_CommandSelect;
}


bool CommandDeleteT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Fail if there are no elements to delete.
    if (m_Indices.Size()==0) return false;

    if (m_Type==JOINT)
    {
        // Deleting joints is difficult, especially because we have to
        //   - decide what to do with child joints, if any (probably delete them as well),
        //   - decide what to with meshes whose weights refer to the joint or one of its children (probably refuse to delete the joint),
        //   - unload the submodel (if any),
        //   - delete the joint also from the anim joints (this is easy), and restore the deleted anim joints on undo (not so easy).
        // (It might help though to restrict deleting joints to one joint at a time.)
        m_Message="Deleting joints is not supported at this time. Sorry.";
        return false;
    }

    // Deselect any affected elements that are selected.
    m_CommandSelect->Do();

    for (unsigned long INr=0; INr<m_Indices.Size(); INr++)
    {
        const unsigned int i=m_Indices[m_Indices.Size()-INr-1];

        switch (m_Type)
        {
            case JOINT: m_ModelDoc->GetModel()->m_Joints     .RemoveAtAndKeepOrder(i); break;
            case MESH:  m_ModelDoc->GetModel()->m_Meshes     .RemoveAtAndKeepOrder(i);
                        m_ModelDoc->GetModel()->m_Draw_Meshes.RemoveAtAndKeepOrder(i); break;
            case ANIM:  m_ModelDoc->GetModel()->m_Anims      .RemoveAtAndKeepOrder(i); break;
        }
    }

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetModel()->m_Draw_CachedDataAtSequNr=-1234;

    m_ModelDoc->UpdateAllObservers_Deleted(m_Type, m_Indices);

    m_Done=true;
    return true;
}


void CommandDeleteT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    for (unsigned long INr=0; INr<m_Indices.Size(); INr++)
    {
        const unsigned int i=m_Indices[INr];

        switch (m_Type)
        {
            case JOINT: m_ModelDoc->GetModel()->m_Joints     .InsertAt(i, m_Joints[INr]); break;
            case MESH:  m_ModelDoc->GetModel()->m_Meshes     .InsertAt(i, m_Meshes[INr]);
                        m_ModelDoc->GetModel()->m_Draw_Meshes.InsertAt(i, m_DrawMs[INr]); break;
            case ANIM:  m_ModelDoc->GetModel()->m_Anims      .InsertAt(i, m_Anims [INr]); break;
        }
    }

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetModel()->m_Draw_CachedDataAtSequNr=-1234;

    m_ModelDoc->UpdateAllObservers_Created(m_Type, m_Indices);

    // Select the previously selected elements again (unless the command failed on Do(), which can happen e.g. on unchanged selection).
    if (m_CommandSelect->IsDone()) m_CommandSelect->Undo();

    m_Done=false;
}


wxString CommandDeleteT::GetName() const
{
    wxString Name=wxString::Format("Delete %lu ", m_Indices.Size());

    switch (m_Type)
    {
        case JOINT: Name+=(m_Indices.Size()==1) ? "joint"     : "joints";     break;
        case MESH:  Name+=(m_Indices.Size()==1) ? "mesh"      : "meshes";     break;
        case ANIM:  Name+=(m_Indices.Size()==1) ? "animation" : "animations"; break;
    }

    return Name;
}
