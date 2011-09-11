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

#include "WindowInspector.hpp"
#include "ChildFrame.hpp"
#include "GuiDocument.hpp"
#include "Commands/ModifyWindow.hpp"
#include "Windows/EditorWindow.hpp"


using namespace GuiEditor;


BEGIN_EVENT_TABLE(WindowInspectorT, wxPropertyGridManager)
    EVT_PG_CHANGED(wxID_ANY, WindowInspectorT::OnPropertyGridChanged)
END_EVENT_TABLE()


WindowInspectorT::WindowInspectorT(ChildFrameT* Parent, const wxSize& Size)
    : wxPropertyGridManager(Parent, wxID_ANY, wxDefaultPosition, Size, wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER), // | wxPG_DESCRIPTION
      m_GuiDocument(Parent->GetGuiDoc()),
      m_Parent(Parent),
      m_SelectedWindow(NULL),
      m_IsRecursiveSelfNotify(false)
{
    SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS | wxPG_EX_MODE_BUTTONS);
    AddPage("Window Properties");
}


void WindowInspectorT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<cf::GuiSys::WindowT*>& OldSelection, const ArrayT<cf::GuiSys::WindowT*>& NewSelection)
{
    RefreshPropGrid();
}


void WindowInspectorT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<cf::GuiSys::WindowT*>& Windows)
{
    if (!m_SelectedWindow) return;

    // Check if our currently selected window is one of the deleted windows.
    for (unsigned long WindowNr=0; WindowNr<Windows.Size(); WindowNr++)
    {
        if (m_SelectedWindow==Windows[WindowNr])
        {
            m_SelectedWindow=NULL;
            ClearPage(0);
            RefreshGrid();
            break;
        }
    }
}


void WindowInspectorT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<cf::GuiSys::WindowT*>& Windows, WindowModDetailE Detail)
{
    if (m_IsRecursiveSelfNotify) return;
    if (!m_SelectedWindow) return;
    if (Windows.Find(m_SelectedWindow)==-1) return;

    switch (Detail)
    {
        case WMD_HIERARCHY:
        {
            // The window hierarchy doesn't affect the window inspector.
            break;
        }

        case WMD_GENERIC:           // Intentional fall-through.
        case WMD_PROPERTY_CHANGED:
        {
            RefreshPropGrid();
            break;
        }

        case WMD_TRANSFORMED:
        {
            // Update all transformation related properties (position, size, rotation).
            wxPGProperty* Property=GetProperty("Position.X");
            Property->SetValue(wxVariant(m_SelectedWindow->Rect[0]));

            Property=GetProperty("Position.Y");
            Property->SetValue(wxVariant(m_SelectedWindow->Rect[1]));

            Property=GetProperty("Size.Width");
            Property->SetValue(wxVariant(m_SelectedWindow->Rect[2]));

            Property=GetProperty("Size.Height");
            Property->SetValue(wxVariant(m_SelectedWindow->Rect[3]));

            Property=GetProperty("Rotation");
            Property->SetValue(wxVariant(m_SelectedWindow->RotAngle));

            RefreshGrid();
            break;
        }

        case WMD_HOR_TEXT_ALIGN:
        {
            wxPGProperty* Property=GetProperty("HorizontalAlign");
            Property->SetValueFromInt(int(m_SelectedWindow->TextAlignHor));

            RefreshGrid();
            break;
        }
    }
}


void WindowInspectorT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<cf::GuiSys::WindowT*>& Windows, WindowModDetailE Detail, const wxString& PropertyName)
{
    if (m_IsRecursiveSelfNotify) return;
    if (!m_SelectedWindow) return;
    if (Windows.Find(m_SelectedWindow)==-1) return;

    wxPGProperty* Property=GetProperty(PropertyName);
    wxASSERT(Property);

    GuiDocumentT::GetSibling(m_SelectedWindow)->UpdateProperty(Property);
    RefreshGrid();
}


void WindowInspectorT::Notify_WinChanged(SubjectT* Subject, const EditorWindowT* Win, const wxString& PropName)
{
    if (m_IsRecursiveSelfNotify) return;
    if (!m_SelectedWindow) return;
    if (Win->GetDual()!=m_SelectedWindow) return;

    wxPGProperty* Property=GetProperty(PropName);
    wxASSERT(Property);

    GuiDocumentT::GetSibling(m_SelectedWindow)->UpdateProperty(Property);
    RefreshGrid();
}


void WindowInspectorT::NotifySubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject==m_GuiDocument);

    m_GuiDocument=NULL;

    ClearPage(0);
}


void WindowInspectorT::RefreshPropGrid()
{
    if (m_GuiDocument==NULL) return;

    ClearPage(0);

    const ArrayT<cf::GuiSys::WindowT*>& Selection=m_GuiDocument->GetSelection();

    if (Selection.Size()==1)
    {
        m_SelectedWindow=Selection[0];

        GuiDocumentT::GetSibling(m_SelectedWindow)->FillInPG(this);
    }
    else
    {
        wxString InfoMessage="Nothing selected";

        if (Selection.Size()>1) InfoMessage="Multiple selection";

        // Multiple selection and no selection are handled by showing an info message in the property grid.
        wxPGProperty* Info=Append(new wxStringProperty("Info", wxPG_LABEL, InfoMessage));
        DisableProperty(Info);

        m_SelectedWindow=NULL;
    }

    RefreshGrid();
}


void WindowInspectorT::OnPropertyGridChanged(wxPropertyGridEvent& Event)
{
    if (m_SelectedWindow==NULL) return;

    // Changing a property by pressing ENTER doesn't change the selection. In consequence the property refresh below does not result in
    // any change since selected properties are not updated (because the user could be in the process of editing a value).
    // Since the user is definitely finished editing this property we can safely clear the selection.
    ClearSelection();

    m_IsRecursiveSelfNotify=true;
    GuiDocumentT::GetSibling(m_SelectedWindow)->HandlePGChange(Event, m_Parent);
    m_IsRecursiveSelfNotify=false;
}
