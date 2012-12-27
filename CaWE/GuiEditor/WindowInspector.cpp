/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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
#include "VarVisitors.hpp"
#include "Commands/AddComponent.hpp"
#include "Commands/DeleteComponent.hpp"
#include "Commands/ModifyWindow.hpp"
#include "Windows/EditorWindow.hpp"
#include "GuiSys/CompBase.hpp"

#include "wx/artprov.h"
#include "wx/notifmsg.h"


using namespace GuiEditor;


BEGIN_EVENT_TABLE(WindowInspectorT, wxPropertyGridManager)
    EVT_PG_CHANGING(wxID_ANY, WindowInspectorT::OnPropertyGridChanging)
    EVT_PG_CHANGED(wxID_ANY, WindowInspectorT::OnPropertyGridChanged)
    EVT_PG_RIGHT_CLICK(wxID_ANY, WindowInspectorT::OnPropertyGridRightClick)
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


void WindowInspectorT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& OldSelection, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& NewSelection)
{
    RefreshPropGrid();
}


void WindowInspectorT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows)
{
    if (m_SelectedWindow.IsNull()) return;

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


void WindowInspectorT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows, WindowModDetailE Detail)
{
    if (m_IsRecursiveSelfNotify) return;
    if (m_SelectedWindow.IsNull()) return;
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


void WindowInspectorT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows, WindowModDetailE Detail, const wxString& PropertyName)
{
    if (m_IsRecursiveSelfNotify) return;
    if (m_SelectedWindow.IsNull()) return;
    if (Windows.Find(m_SelectedWindow)==-1) return;

    wxPGProperty* Property=GetProperty(PropertyName);
    wxASSERT(Property);

    GuiDocumentT::GetSibling(m_SelectedWindow)->UpdateProperty(Property);
    RefreshGrid();
}


void WindowInspectorT::Notify_WinChanged(SubjectT* Subject, const EditorWindowT* Win, const wxString& PropName)
{
    if (m_IsRecursiveSelfNotify) return;
    if (m_SelectedWindow.IsNull()) return;
    if (Win->GetDual()!=m_SelectedWindow) return;

    wxPGProperty* Property=GetProperty(PropName);
    wxASSERT(Property);

    GuiDocumentT::GetSibling(m_SelectedWindow)->UpdateProperty(Property);
    RefreshGrid();
}


void WindowInspectorT::Notify_Changed(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var)
{
    if (m_IsRecursiveSelfNotify) return;
    if (m_SelectedWindow.IsNull()) return;
    if (!GetPage(0)) return;

    // Find the property that is related to Var.
    for (wxPropertyGridIterator it = GetPage(0)->GetIterator(); !it.AtEnd(); it++)
    {
        wxPGProperty* Prop = *it;

        if (Prop->GetClientData() == &Var)
        {
            VarVisitorUpdatePropT UpdateProp(*Prop);

            Var.accept(UpdateProp);
            RefreshGrid();
            break;
        }
    }
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

    const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Selection=m_GuiDocument->GetSelection();

    if (Selection.Size()==1)
    {
        m_SelectedWindow=Selection[0];

        GuiDocumentT::GetSibling(m_SelectedWindow)->FillInPG(this);

        VarVisitorAddPropT AddProp(*this, m_GuiDocument);

        for (unsigned long CompNr = 0; CompNr < m_SelectedWindow->GetComponents().Size(); CompNr++)
        {
            IntrusivePtrT<cf::GuiSys::ComponentBaseT> Comp       = m_SelectedWindow->GetComponents()[CompNr];
            const ArrayT<cf::TypeSys::VarBaseT*>&     MemberVars = Comp->GetMemberVars().GetArray();
            const wxString                            UniqueName = wxString::Format("%p", Comp.get());
            wxPGProperty*                             CatProp    = new wxPropertyCategory(Comp->GetName(), UniqueName);

            // With "category" properties we set the component pointer as client data,
            // with other properties we set (in the AddProp visitor) the variable as client data.
            CatProp->SetClientData(Comp.get());
            Append(CatProp);

            for (unsigned long VarNr = 0; VarNr < MemberVars.Size(); VarNr++)
                MemberVars[VarNr]->accept(AddProp);
        }
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


/*
 * Normally, if we make a change to e.g. a "double" property and press RETURN, we see events as expected:
 *
 *     a1) OnPropertyGridChanging: double value "RotAngle": 45
 *     a2) OnPropertyGridChanged:  double value "RotAngle": 45
 *
 * Unfortunately, when the property is "<composed>", the sequence of events (with wxWidgets 2.9.2) is not so clear:
 * If "Pos" is a "<composed>" property with "x", "y" and "z" as sub-properties, changing "y" yields:
 *
 *     b1) OnPropertyGridChanging: double value "y": 120
 *     b2) OnPropertyGridChanged:  double value "y": 120
 *     b3) OnPropertyGridChanged:  string value "Pos": 0; 120; 200
 *
 * Note that there is no "changing" event for Pos!
 * If instead we directly change the "z" sub-property in the top-level "Pos" string:
 *
 *     c1) OnPropertyGridChanging: string value "Pos": 0; 120; 240
 *     c2) OnPropertyGridChanged:  string value "Pos": 0; 120; 240
 *
 * Note that there is no event at all related to the changed "z" value!
 */
void WindowInspectorT::OnPropertyGridChanging(wxPropertyGridEvent& Event)
{
    // Changing a property by pressing ENTER doesn't change the selection. In consequence the property refresh below does not result in
    // any change since selected properties are not updated (because the user could be in the process of editing a value).
    // Since the user is definitely finished editing this property we can safely clear the selection.
    // ClearSelection();
    wxASSERT(Event.CanVeto());  // EVT_PG_CHANGING events can be vetoed (as opposed to EVT_PG_CHANGED events).
    wxLogDebug("%s: %s value \"%s\": %s", __FUNCTION__, Event.GetValue().GetType(), Event.GetProperty()->GetLabel(), Event.GetValue().MakeString());

    m_IsRecursiveSelfNotify=true;

    const wxPGProperty*    Prop = Event.GetProperty();
    cf::TypeSys::VarBaseT* Var  = Prop && !Prop->IsCategory() ? static_cast<cf::TypeSys::VarBaseT*>(Prop->GetClientData()) : NULL;

    if (Var)
    {
        // Handle cases a1) and c1).
        VarVisitorHandlePropChangingEventT PropChange(Event, m_Parent);

        Var->accept(PropChange);
        if (!PropChange.Ok()) Event.Veto();
    }
    else
    {
        // Handle case b1), if applicable.
        Prop = Prop->GetParent();
        Var  = Prop && !Prop->IsCategory() ? static_cast<cf::TypeSys::VarBaseT*>(Prop->GetClientData()) : NULL;

        if (Var)
        {
            VarVisitorHandleSubChangingEventT PropChange(Event, m_Parent);

            Var->accept(PropChange);
            if (!PropChange.Ok()) Event.Veto();
        }
    }

    if (!Event.WasVetoed() && Var->GetExtraMessage() != "")
    {
        // A wxInfoBar is a possible alternative to wxNotificationMessage.
        wxNotificationMessage Notify(wxString("Setting \"") + Var->GetName() + "\"", Var->GetExtraMessage(), m_Parent);

        Notify.Show();
    }

    m_IsRecursiveSelfNotify=false;
}


void WindowInspectorT::OnPropertyGridChanged(wxPropertyGridEvent& Event)
{
    if (m_SelectedWindow==NULL) return;

    // Changing a property by pressing ENTER doesn't change the selection. In consequence the property refresh below does not result in
    // any change since selected properties are not updated (because the user could be in the process of editing a value).
    // Since the user is definitely finished editing this property we can safely clear the selection.
    // ClearSelection();
    wxLogDebug("%s: %s value \"%s\": %s", __FUNCTION__, Event.GetValue().GetType(), Event.GetProperty()->GetLabel(), Event.GetValue().MakeString());

    m_IsRecursiveSelfNotify=true;
    GuiDocumentT::GetSibling(m_SelectedWindow)->HandlePGChange(Event, m_Parent);
    m_IsRecursiveSelfNotify=false;
}


static wxMenuItem* AppendMI(wxMenu& Menu, int MenuID, const wxString& Label, const wxArtID& ArtID, bool Active=true, const wxString& Help="")
{
    wxMenuItem* MI = new wxMenuItem(&Menu, MenuID, Label, Help);

    // Under wxMSW (2.9.2), the bitmap must be set before the menu item is added to the menu.
    if (ArtID != "")
        MI->SetBitmap(wxArtProvider::GetBitmap(ArtID, wxART_MENU));

    // Under wxGTK (2.9.2), the menu item must be added to the menu before we can call Enable().
    Menu.Append(MI);

    MI->Enable(Active);

    return MI;
}


void WindowInspectorT::OnPropertyGridRightClick(wxPropertyGridEvent& Event)
{
    // Find the component that this right click corresponds to.
    if (m_SelectedWindow == NULL) return;

    IntrusivePtrT<cf::GuiSys::ComponentBaseT> Comp = NULL;

    for (wxPGProperty* Prop = Event.GetProperty(); Prop; Prop = Prop->GetParent())
        if (Prop->IsCategory())
        {
            Comp = static_cast<cf::GuiSys::ComponentBaseT*>(Prop->GetClientData());
            break;
        }

    if (Comp == NULL) return;

    const unsigned int Index = m_SelectedWindow->GetComponents().Find(Comp);

    if (Index >= m_SelectedWindow->GetComponents().Size()) return;


    // Note that GetPopupMenuSelectionFromUser() temporarily disables UI updates for the window,
    // so our menu IDs used below should be doubly clash-free.
    enum
    {
        ID_MENU_MOVE_COMPONENT_UP=wxID_HIGHEST+1+100,
        ID_MENU_MOVE_COMPONENT_DOWN,
        ID_MENU_COPY_COMPONENT,
        ID_MENU_PASTE_COMPONENT,
        ID_MENU_REMOVE_COMPONENT
    };

    wxMenu Menu;

    AppendMI(Menu, ID_MENU_MOVE_COMPONENT_UP,   "Move Component Up",   "list-selection-up",   Index > 0);
    AppendMI(Menu, ID_MENU_MOVE_COMPONENT_DOWN, "Move Component Down", "list-selection-down", Index+1 < m_SelectedWindow->GetComponents().Size());
    Menu.AppendSeparator();
    AppendMI(Menu, ID_MENU_COPY_COMPONENT,  "Copy Component",  wxART_COPY,  false);
    AppendMI(Menu, ID_MENU_PASTE_COMPONENT, "Paste Component", wxART_PASTE, false);
    Menu.AppendSeparator();
    AppendMI(Menu, ID_MENU_REMOVE_COMPONENT, "Remove Component", wxART_DELETE);

    switch (GetPopupMenuSelectionFromUser(Menu))
    {
        case ID_MENU_MOVE_COMPONENT_UP:
        {
            if (Index == 0) break;

            ArrayT<CommandT*> Commands;

            Commands.PushBack(new CommandDeleteComponentT(m_GuiDocument, m_SelectedWindow, Index));
            Commands.PushBack(new CommandAddComponentT(m_GuiDocument, m_SelectedWindow, Comp, Index-1));

            m_Parent->SubmitCommand(new CommandMacroT(Commands, "Move component up"));
            break;
        }

        case ID_MENU_MOVE_COMPONENT_DOWN:
        {
            if (Index+1 >= m_SelectedWindow->GetComponents().Size()) break;

            ArrayT<CommandT*> Commands;

            Commands.PushBack(new CommandDeleteComponentT(m_GuiDocument, m_SelectedWindow, Index));
            Commands.PushBack(new CommandAddComponentT(m_GuiDocument, m_SelectedWindow, Comp, Index+1));

            m_Parent->SubmitCommand(new CommandMacroT(Commands, "Move component down"));
            break;
        }

        case ID_MENU_COPY_COMPONENT:
            break;

        case ID_MENU_PASTE_COMPONENT:
            break;

        case ID_MENU_REMOVE_COMPONENT:
        {
            m_Parent->SubmitCommand(new CommandDeleteComponentT(m_GuiDocument, m_SelectedWindow, Index));
            break;
        }
    }
}
