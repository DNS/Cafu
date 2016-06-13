/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
      m_MeshInfos(),
      m_Skins(),
      m_GuiFixtures(),
      m_Anims(),
      m_Channels(),
      m_Message(),
      m_CommandSelect(CommandSelectT::Remove(m_ModelDoc, m_Type, m_Indices))
{
    for (unsigned long INr=0; INr<m_Indices.Size(); INr++)
    {
        const unsigned int i=m_Indices[INr];

        switch (m_Type)
        {
            case JOINT: m_Joints     .PushBack(m_ModelDoc->GetModel()->GetJoints()     [i]); break;
            case SKIN:  m_Skins      .PushBack(m_ModelDoc->GetModel()->GetSkins()      [i]); break;
            case GFIX:  m_GuiFixtures.PushBack(m_ModelDoc->GetModel()->GetGuiFixtures()[i]); break;
            case ANIM:  m_Anims      .PushBack(m_ModelDoc->GetModel()->GetAnims()      [i]); break;
            case CHAN:  m_Channels   .PushBack(m_ModelDoc->GetModel()->GetChannels()   [i]); break;
            case MESH:
            {
                wxASSERT(m_MeshInfos.Size() == INr);
                m_MeshInfos.PushBackEmpty();
                MeshInfoT& MI=m_MeshInfos[m_MeshInfos.Size()-1];

                MI.Mesh=m_ModelDoc->GetModel()->GetMeshes()[i];

                for (unsigned long SkinNr=0; SkinNr<m_ModelDoc->GetModel()->GetSkins().Size(); SkinNr++)
                {
                    MI.SkinsMaterials      .PushBack(m_ModelDoc->GetModel()->GetSkins()[SkinNr].Materials[i]);
                    MI.SkinsRenderMaterials.PushBack(m_ModelDoc->GetModel()->GetSkins()[SkinNr].RenderMaterials[i]);
                }
                break;
            }
        }
    }
}


CommandDeleteT::~CommandDeleteT()
{
    if (m_Done && MatSys::Renderer!=NULL)
    {
        for (unsigned long MeshInfoNr=0; MeshInfoNr<m_MeshInfos.Size(); MeshInfoNr++)
        {
            MeshInfoT& MI=m_MeshInfos[MeshInfoNr];

            MatSys::Renderer->FreeMaterial(MI.Mesh.RenderMaterial);
            MI.Mesh.RenderMaterial=NULL;

            for (unsigned long SkinNr=0; SkinNr<MI.SkinsRenderMaterials.Size(); SkinNr++)
            {
                MatSys::Renderer->FreeMaterial(MI.SkinsRenderMaterials[SkinNr]);
                MI.SkinsRenderMaterials[SkinNr]=NULL;
            }
        }

        for (unsigned long SkinNr=0; SkinNr<m_Skins.Size(); SkinNr++)
        {
            for (unsigned long MatNr=0; MatNr<m_Skins[SkinNr].RenderMaterials.Size(); MatNr++)
            {
                MatSys::Renderer->FreeMaterial(m_Skins[SkinNr].RenderMaterials[MatNr]);
                m_Skins[SkinNr].RenderMaterials[MatNr]=NULL;
            }
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
        //   - delete the joint also from the anim channels,
        //   - delete the joint also from the anim joints (this is easy), and restore the deleted anim joints on undo (not so easy).
        // (It might help though to restrict deleting joints to one joint at a time.)
        m_Message="Deleting joints is not supported at this time. Sorry.";
        return false;
    }

    if (m_Type==MESH)
    {
        if (m_Indices.Size() >= m_ModelDoc->GetModel()->GetMeshes().Size())
        {
            m_Message="Cannot delete all meshes -- at least one mesh must be left.";
            return false;
        }

        for (unsigned long GFixNr=0; GFixNr<m_ModelDoc->GetModel()->GetGuiFixtures().Size(); GFixNr++)
            for (unsigned int PointNr=0; PointNr<3; PointNr++)
                if (m_Indices.Find(m_ModelDoc->GetModel()->GetGuiFixtures()[GFixNr].Points[PointNr].MeshNr)>=0)
                {
                    m_Message="There are still GUI fixtures referring to the selected mesh(es). Delete the GUI fixtures first, then delete the meshes.";
                    return false;
                }
    }

    // Deselect any affected elements that are selected.
    m_CommandSelect->Do();

    for (unsigned long INr=0; INr<m_Indices.Size(); INr++)
    {
        const unsigned int i=m_Indices[m_Indices.Size()-INr-1];

        switch (m_Type)
        {
            case JOINT: m_ModelDoc->GetModel()->m_Joints     .RemoveAtAndKeepOrder(i); break;
            case SKIN:  m_ModelDoc->GetModel()->m_Skins      .RemoveAtAndKeepOrder(i); break;
            case GFIX:  m_ModelDoc->GetModel()->m_GuiFixtures.RemoveAtAndKeepOrder(i); break;
            case ANIM:  m_ModelDoc->GetModel()->m_Anims      .RemoveAtAndKeepOrder(i); break;
            case CHAN:  m_ModelDoc->GetModel()->m_Channels   .RemoveAtAndKeepOrder(i); break;
            case MESH:
            {
                m_ModelDoc->GetModel()->m_Meshes.RemoveAtAndKeepOrder(i);

                for (unsigned long SkinNr=0; SkinNr<m_ModelDoc->GetModel()->GetSkins().Size(); SkinNr++)
                {
                    m_ModelDoc->GetModel()->m_Skins[SkinNr].Materials      .RemoveAtAndKeepOrder(i);
                    m_ModelDoc->GetModel()->m_Skins[SkinNr].RenderMaterials.RemoveAtAndKeepOrder(i);
                }
                break;
            }
        }
    }

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetAnimState().Pose.SetNeedsRecache();

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
            case JOINT: m_ModelDoc->GetModel()->m_Joints     .InsertAt(i, m_Joints     [INr]); break;
            case SKIN:  m_ModelDoc->GetModel()->m_Skins      .InsertAt(i, m_Skins      [INr]); break;
            case GFIX:  m_ModelDoc->GetModel()->m_GuiFixtures.InsertAt(i, m_GuiFixtures[INr]); break;
            case ANIM:  m_ModelDoc->GetModel()->m_Anims      .InsertAt(i, m_Anims      [INr]); break;
            case CHAN:  m_ModelDoc->GetModel()->m_Channels   .InsertAt(i, m_Channels   [INr]); break;
            case MESH:
            {
                const MeshInfoT& MI=m_MeshInfos[INr];

                m_ModelDoc->GetModel()->m_Meshes.InsertAt(i, MI.Mesh);

                for (unsigned long SkinNr=0; SkinNr<m_ModelDoc->GetModel()->GetSkins().Size(); SkinNr++)
                {
                    m_ModelDoc->GetModel()->m_Skins[SkinNr].Materials      .InsertAt(i, MI.SkinsMaterials[SkinNr]);
                    m_ModelDoc->GetModel()->m_Skins[SkinNr].RenderMaterials.InsertAt(i, MI.SkinsRenderMaterials[SkinNr]);
                }
                break;
            }
        }
    }

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetAnimState().Pose.SetNeedsRecache();

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
        case JOINT: Name+=(m_Indices.Size()==1) ? "joint"       : "joints";       break;
        case MESH:  Name+=(m_Indices.Size()==1) ? "mesh"        : "meshes";       break;
        case SKIN:  Name+=(m_Indices.Size()==1) ? "skin"        : "skins";        break;
        case GFIX:  Name+=(m_Indices.Size()==1) ? "GUI fixture" : "GUI fixtures"; break;
        case ANIM:  Name+=(m_Indices.Size()==1) ? "animation"   : "animations";   break;
        case CHAN:  Name+=(m_Indices.Size()==1) ? "channel"     : "channels";     break;
    }

    return Name;
}
