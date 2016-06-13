/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Rename.hpp"
#include "../ModelDocument.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


CommandRenameT::CommandRenameT(ModelDocumentT* ModelDoc, ModelElementTypeT Type, unsigned int ElemNr, const wxString& NewName)
    : m_ModelDoc(ModelDoc),
      m_Type(Type),
      m_ElemNr(ElemNr),
      m_NewName(std::string(NewName)),
      m_OldName(GetStringRef())
{
}


bool CommandRenameT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If the name didn't really change, don't put this command into the command history.
    if (m_NewName==m_OldName) return false;

    // Cannot store the result of GetStringRef(), because it's bound to become
    // invalid whenever another command meddles with the array of joints/meshes/anims.
    GetStringRef()=m_NewName;

    UpdateAllObservers();
    m_Done=true;
    return true;
}


void CommandRenameT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Cannot store the result of GetStringRef(), because it's bound to become
    // invalid whenever another command meddles with the array of joints/meshes/anims.
    GetStringRef()=m_OldName;

    UpdateAllObservers();
    m_Done=false;
}


wxString CommandRenameT::GetName() const
{
    switch (m_Type)
    {
        case JOINT: return "Rename joint";
        case MESH:  return "Rename mesh";
        case SKIN:  return "Rename skin";
        case GFIX:  return "Rename GUI fixture";
        case ANIM:  return "Rename anim";
        case CHAN:  return "Rename channel";
    }

    wxASSERT(false);
    return "???";
}


std::string& CommandRenameT::GetStringRef() const
{
    switch (m_Type)
    {
        case JOINT: return m_ModelDoc->GetModel()->m_Joints     [m_ElemNr].Name;
        case MESH:  return m_ModelDoc->GetModel()->m_Meshes     [m_ElemNr].Name;
        case SKIN:  return m_ModelDoc->GetModel()->m_Skins      [m_ElemNr].Name;
        case GFIX:  return m_ModelDoc->GetModel()->m_GuiFixtures[m_ElemNr].Name;
        case ANIM:  return m_ModelDoc->GetModel()->m_Anims      [m_ElemNr].Name;
        case CHAN:  return m_ModelDoc->GetModel()->m_Channels   [m_ElemNr].Name;
    }

    wxASSERT(false);
    static std::string BadType("???");
    return BadType;
}


void CommandRenameT::UpdateAllObservers() const
{
    switch (m_Type)
    {
        case JOINT: m_ModelDoc->UpdateAllObservers_JointChanged     (m_ElemNr); break;
        case MESH:  m_ModelDoc->UpdateAllObservers_MeshChanged      (m_ElemNr); break;
        case SKIN:  m_ModelDoc->UpdateAllObservers_SkinChanged      (m_ElemNr); break;
        case GFIX:  m_ModelDoc->UpdateAllObservers_GuiFixtureChanged(m_ElemNr); break;
        case ANIM:  m_ModelDoc->UpdateAllObservers_AnimChanged      (m_ElemNr); break;
        case CHAN:  m_ModelDoc->UpdateAllObservers_ChannelChanged   (m_ElemNr); break;
    }
}
