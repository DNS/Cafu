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

#include "Add.hpp"
#include "../ModelDocument.hpp"


using namespace ModelEditor;


CommandAddT::CommandAddT(ModelDocumentT* ModelDoc, const CafuModelT::SkinT& Skin)
    : m_ModelDoc(ModelDoc),
      m_Type(SKIN),
      m_Skins()
{
    m_Skins.PushBack(Skin);
}


CommandAddT::CommandAddT(ModelDocumentT* ModelDoc, const ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures)
    : m_ModelDoc(ModelDoc),
      m_Type(GFIX),
      m_GuiFixtures(GuiFixtures)
{
}


bool CommandAddT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    ArrayT<unsigned int> Indices;

    switch (m_Type)
    {
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

        default:
            break;
    }

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetModel()->m_Draw_CachedDataAtSequNr=-1234;

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

        default:
            break;
    }

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetModel()->m_Draw_CachedDataAtSequNr=-1234;

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
        case ANIM:  break;

        case GFIX:
        {
            const unsigned long Num=m_GuiFixtures.Size();

            Name=(Num==1) ? wxString("Add GUI fixture") : wxString::Format("Add %lu GUI fixtures", Num);
            break;
        }
    }

    return Name;
}
