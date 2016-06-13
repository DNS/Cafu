/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ChildFrame.hpp"
#include "ChildFrameViewWin.hpp"
#include "MapDocument.hpp"
#include "ToolManager.hpp"


ViewWindowT::ViewWindowT(ChildFrameT* ChildFrame)
    : m_ChildFrame(ChildFrame)
{
    m_ChildFrame->GetDoc()->RegisterObserver(this);
    m_ChildFrame->GetToolManager().RegisterObserver(this);
    m_ChildFrame->m_ViewWindows.PushBack(this);
}


ViewWindowT::~ViewWindowT()
{
    if (m_ChildFrame)
    {
        if (m_ChildFrame->GetDoc())
            m_ChildFrame->GetDoc()->UnregisterObserver(this);

        if (&m_ChildFrame->GetToolManager())
            m_ChildFrame->GetToolManager().UnregisterObserver(this);

        // Remove this ViewWindowT from the child frames list - if the child frame has not already died.
        const int Index=m_ChildFrame->m_ViewWindows.Find(this);
        if (Index!=-1) m_ChildFrame->m_ViewWindows.RemoveAtAndKeepOrder(Index);
    }
}


void ViewWindowT::NotifySubjectDies(SubjectT* dyingSubject)
{
    // If the subject (the document) dies, then only because its containing child frame is dying, too.
    // Note that this damn method only exists because we possible live longer than our parent, the m_ChildFrame.
    // See my posting to wx-users from 2005-Oct-13 with subject "wxWindow::Destroy() again..." in this regard,
    // and the comments in ObserverPattern.hpp.
    m_ChildFrame=NULL;
}


wxString ViewWindowT::GetCaption() const
{
    switch (GetViewType())
    {
        case VT_2D_XY:         return "2D Top (X/Y)";
        case VT_2D_XZ:         return "2D Front (X/Z)";
        case VT_2D_YZ:         return "2D Side (Y/Z)";
        case VT_3D_WIREFRAME:  return "3D Wireframe";
        case VT_3D_FLAT:       return "3D Flat";
        case VT_3D_EDIT_MATS:  return "3D Edit Mats";
        case VT_3D_FULL_MATS:  return "3D Full Mats";
        case VT_3D_LM_GRID:    return "3D LM Grid";
        case VT_3D_LM_PREVIEW: return "3D LM Preview";
    }

    return "Map View";
}


ChildFrameT* ViewWindowT::GetChildFrame() const
{
    return m_ChildFrame;
}


MapDocumentT& ViewWindowT::GetMapDoc() const
{
    wxASSERT(m_ChildFrame);     // See the description of NotifyChildFrameDies() for why this should never trigger.

    return *m_ChildFrame->GetDoc();
}


void ViewWindowT::UpdateChildFrameMRU()
{
    wxASSERT(m_ChildFrame);     // See the description of NotifyChildFrameDies() for why this should never trigger.

    const int Index=m_ChildFrame->m_ViewWindows.Find(this);

    wxASSERT(Index!=-1);        // We insert ourselves in the ctor, and remove ourselves in the dtor, so this should never trigger.
    if (Index!=-1) m_ChildFrame->m_ViewWindows.RemoveAtAndKeepOrder(Index);

    m_ChildFrame->m_ViewWindows.InsertAt(0, this);
}
