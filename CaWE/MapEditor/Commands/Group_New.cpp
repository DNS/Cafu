/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Group_New.hpp"
#include "../Group.hpp"
#include "../MapDocument.hpp"


CommandNewGroupT::CommandNewGroupT(MapDocumentT& MapDoc, const wxString& Name)
    : m_MapDoc(MapDoc),
      m_Group(NULL)
{
    // Important detail: The new group is created *visible*, in order to minimize
    // potential side-effects on any map elements that are added to it later.
    m_Group = new GroupT(MapDoc, Name);

    wxASSERT(m_Group->IsVisible);
}


CommandNewGroupT::~CommandNewGroupT()
{
    if (!m_Done)
    {
        delete m_Group;
    }
}


bool CommandNewGroupT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Add m_Group to the groups array of the m_MapDoc
    // and notify all observers that our groups inventory changed.
    m_MapDoc.GetGroups().PushBack(m_Group);
    m_MapDoc.UpdateAllObservers_GroupsChanged();

    m_Done=true;
    return true;
}


void CommandNewGroupT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Remove m_Group from the groups array of the m_MapDoc again
    // and notify all observers that our groups inventory changed.
    wxASSERT(m_MapDoc.GetGroups()[m_MapDoc.GetGroups().Size()-1]==m_Group);
    m_MapDoc.GetGroups().DeleteBack();
    m_MapDoc.UpdateAllObservers_GroupsChanged();

    m_Done=false;
}


wxString CommandNewGroupT::GetName() const
{
    return "Add new group";
}
