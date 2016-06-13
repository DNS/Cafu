/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Add.hpp"
#include "../ModelDocument.hpp"


using namespace ModelEditor;


CommandAddT::CommandAddT(ModelDocumentT* ModelDoc, const CafuModelT::SkinT& Skin)
    : m_ModelDoc(ModelDoc),
      m_Type(SKIN),
      m_Skins()
{
    wxASSERT(Skin.Materials.Size()       == m_ModelDoc->GetModel()->GetMeshes().Size());
    wxASSERT(Skin.RenderMaterials.Size() == m_ModelDoc->GetModel()->GetMeshes().Size());

    m_Skins.PushBack(Skin);
}


CommandAddT::CommandAddT(ModelDocumentT* ModelDoc, const ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures)
    : m_ModelDoc(ModelDoc),
      m_Type(GFIX),
      m_GuiFixtures(GuiFixtures)
{
}


CommandAddT::CommandAddT(ModelDocumentT* ModelDoc, const ArrayT<CafuModelT::AnimT>& Anims)
    : m_ModelDoc(ModelDoc),
      m_Type(ANIM),
      m_Anims(Anims)
{
}


CommandAddT::CommandAddT(ModelDocumentT* ModelDoc, const CafuModelT::ChannelT& Channel)
    : m_ModelDoc(ModelDoc),
      m_Type(CHAN),
      m_Channels()
{
    m_Channels.PushBack(Channel);
}


bool CommandAddT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    ArrayT<unsigned int> Indices;

    switch (m_Type)
    {
        case JOINT:
            // If we supported adding joints, note that we had to update all channels as well.
            break;

        case MESH:
            // If we supported adding meshes, note that we had to add a
            // new "NULL" material (and render material) to each skin as well.
            break;

        case SKIN:
            for (unsigned long SkinNr=0; SkinNr<m_Skins.Size(); SkinNr++)
            {
                Indices.PushBack(m_ModelDoc->GetModel()->m_Skins.Size());
                m_ModelDoc->GetModel()->m_Skins.PushBack(m_Skins[SkinNr]);
            }
            break;

        case GFIX:
            for (unsigned long GFixNr=0; GFixNr<m_GuiFixtures.Size(); GFixNr++)
            {
                Indices.PushBack(m_ModelDoc->GetModel()->m_GuiFixtures.Size());
                m_ModelDoc->GetModel()->m_GuiFixtures.PushBack(m_GuiFixtures[GFixNr]);
            }
            break;

        case ANIM:
            for (unsigned long AnimNr=0; AnimNr<m_Anims.Size(); AnimNr++)
            {
                Indices.PushBack(m_ModelDoc->GetModel()->m_Anims.Size());
                m_ModelDoc->GetModel()->m_Anims.PushBack(m_Anims[AnimNr]);
            }
            break;

        case CHAN:
            for (unsigned long ChannelNr=0; ChannelNr<m_Channels.Size(); ChannelNr++)
            {
                Indices.PushBack(m_ModelDoc->GetModel()->m_Channels.Size());
                m_ModelDoc->GetModel()->m_Channels.PushBack(m_Channels[ChannelNr]);
            }
            break;
    }

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetAnimState().Pose.SetNeedsRecache();

    m_ModelDoc->UpdateAllObservers_Created(m_Type, Indices);

    m_Done=true;
    return true;
}


void CommandAddT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    ArrayT<unsigned int> Indices;

    switch (m_Type)
    {
        case JOINT:
            // For undoing the addition of joints, note that we had to update all channels as well.
            break;

        case MESH:
            // For undoing the addition of a mesh, note that we had to remove
            // the related material (and render material) in each skin as well.
            break;

        case SKIN:
            for (unsigned long SkinNr=0; SkinNr<m_Skins.Size(); SkinNr++)
            {
                m_ModelDoc->GetModel()->m_Skins.DeleteBack();
                Indices.InsertAt(0, m_ModelDoc->GetModel()->m_Skins.Size());
            }
            break;

        case GFIX:
            for (unsigned long GFixNr=0; GFixNr<m_GuiFixtures.Size(); GFixNr++)
            {
                m_ModelDoc->GetModel()->m_GuiFixtures.DeleteBack();
                Indices.InsertAt(0, m_ModelDoc->GetModel()->m_GuiFixtures.Size());
            }
            break;

        case ANIM:
            for (unsigned long AnimNr=0; AnimNr<m_Anims.Size(); AnimNr++)
            {
                m_ModelDoc->GetModel()->m_Anims.DeleteBack();
                Indices.InsertAt(0, m_ModelDoc->GetModel()->m_Anims.Size());
            }
            break;

        case CHAN:
            for (unsigned long ChannelNr=0; ChannelNr<m_Channels.Size(); ChannelNr++)
            {
                m_ModelDoc->GetModel()->m_Channels.DeleteBack();
                Indices.InsertAt(0, m_ModelDoc->GetModel()->m_Channels.Size());
            }
            break;
    }

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetAnimState().Pose.SetNeedsRecache();

    m_ModelDoc->UpdateAllObservers_Deleted(m_Type, Indices);

    m_Done=false;
}


wxString CommandAddT::GetName() const
{
    wxString Name;

    switch (m_Type)
    {
        case JOINT: break;
        case MESH:  break;

        case SKIN:
        {
            Name=(m_Skins.Size()==1) ? wxString("Add skin") : wxString::Format("Add %lu skins", m_Skins.Size());
            break;
        }

        case GFIX:
        {
            const unsigned long Num=m_GuiFixtures.Size();

            Name=(Num==1) ? wxString("Add GUI fixture") : wxString::Format("Add %lu GUI fixtures", Num);
            break;
        }

        case ANIM:
        {
            Name=(m_Anims.Size()==1) ? wxString("Add anim sequence") : wxString::Format("Add %lu anim sequences", m_Anims.Size());
            break;
        }

        case CHAN:
        {
            Name=(m_Channels.Size()==1) ? wxString("Add channel") : wxString::Format("Add %lu channels", m_Channels.Size());
            break;
        }
    }

    return Name;
}
