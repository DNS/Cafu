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

#include "ToolSelection.hpp"

#include "RenderWindow.hpp"
#include "GuiDocument.hpp"
#include "ChildFrame.hpp"

#include "Commands/Select.hpp"
#include "Commands/Create.hpp"
#include "Commands/Delete.hpp"
#include "Commands/SetCompVar.hpp"
#include "Windows/EditorWindow.hpp"

#include "../CursorMan.hpp"

#include "Math3D/Angles.hpp"


using namespace GuiEditor;


const float HANDLE_WIDTH=5.0f; // Width in GUI units of the handle used to scale windows.


static float GetAngle(float x1, float y1, float x2, float y2)
{
    const float a=x2-x1;
    const float b=y2-y1;

    return (a==0.0f && b==0.0f) ? 0.0f : cf::math::AnglesfT::RadToDeg(atan2(b, a));
}


ToolSelectionT::ToolSelectionT(GuiDocumentT* GuiDocument, ChildFrameT* Parent)
    : ToolI(),
      m_GuiDocument(GuiDocument),
      m_Parent(Parent),
      m_TransformSelection(false),
      m_TransformationStart(false),
      m_TransState(NONE),
      m_RotStartAngle(0.0f),
      m_LastMousePosX(0.0f),
      m_LastMousePosY(0.0f)
{
}


void ToolSelectionT::Activate()
{
}


void ToolSelectionT::Deactivate()
{
}


void ToolSelectionT::RenderTool(RenderWindowT* RenderWindow)
{
    // TODO Render selection bounding box.
}


bool ToolSelectionT::OnKeyDown(RenderWindowT* RenderWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_DELETE:
            // Delete objects, then clear selection.
            m_Parent->SubmitCommand(new CommandDeleteT(m_GuiDocument, m_GuiDocument->GetSelection()));
            m_Parent->SubmitCommand(CommandSelectT::Clear(m_GuiDocument));
            return true;
    }

    return false;
}


bool ToolSelectionT::OnLMouseDown(RenderWindowT* RenderWindow, wxMouseEvent& ME)
{
    // Translate client mouse position to GUI mouse position.
    Vector3fT MousePosGUI=RenderWindow->ClientToGui(ME.GetX(), ME.GetY());

    IntrusivePtrT<cf::GuiSys::WindowT> ClickedWindow=m_GuiDocument->GetRootWindow()->Find(MousePosGUI.x, MousePosGUI.y);

    if (ClickedWindow.IsNull())
    {
        if (!ME.ControlDown()) m_Parent->SubmitCommand(CommandSelectT::Clear(m_GuiDocument));
        return true;
    }

    if (!GuiDocumentT::GetSibling(ClickedWindow)->IsSelected())
    {
        if (!ME.ControlDown()) m_Parent->SubmitCommand(CommandSelectT::Clear(m_GuiDocument));
        m_Parent->SubmitCommand(CommandSelectT::Add(m_GuiDocument, ClickedWindow));
    }
    else
    {
        if (ME.ControlDown())
        {
            m_Parent->SubmitCommand(CommandSelectT::Remove(m_GuiDocument, ClickedWindow)); // Toggle window selection.
            return true;
        }
    }

    // Backup the current state (in terms of transformation) of the selected windows.
    const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Selection=m_GuiDocument->GetSelection();

    for (unsigned long SelNr=0; SelNr<Selection.Size(); SelNr++)
    {
        WindowStateT WindowState;

        WindowState.Position = Selection[SelNr]->GetTransform()->GetPos();
        WindowState.Size     = Selection[SelNr]->GetTransform()->GetSize();
        WindowState.Rotation = Selection[SelNr]->GetTransform()->GetRotAngle();

        m_WindowStates.PushBack(WindowState);
    }

    // Send a mouse move event to make sure the transform state is set corretly (according to the position we clicked on).
    OnMouseMove(RenderWindow, ME);

    // Start transforming the selection.
    m_TransformationStart=true;

    m_RotStartAngle=GetAngle(MousePosGUI.x, MousePosGUI.y,
        (ClickedWindow->GetTransform()->GetPos().x + ClickedWindow->GetTransform()->GetSize().x)/2.0f,
        (ClickedWindow->GetTransform()->GetPos().y + ClickedWindow->GetTransform()->GetSize().y)/2.0f);
    return true;
}


bool ToolSelectionT::OnLMouseUp(RenderWindowT* RenderWindow, wxMouseEvent& ME)
{
    if (m_TransformSelection)
    {
        const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Sel = m_GuiDocument->GetSelection();
        ArrayT<CommandT*> SubCommands;

        wxASSERT(Sel.Size() == m_WindowStates.Size());

        for (unsigned int SelNr = 0; SelNr < Sel.Size(); SelNr++)
        {
            cf::TypeSys::VarT<Vector2fT>* Pos      = dynamic_cast<cf::TypeSys::VarT<Vector2fT>*>(Sel[SelNr]->GetTransform()->GetMemberVars().Find("Pos"));
            cf::TypeSys::VarT<Vector2fT>* Size     = dynamic_cast<cf::TypeSys::VarT<Vector2fT>*>(Sel[SelNr]->GetTransform()->GetMemberVars().Find("Size"));
            cf::TypeSys::VarT<float>*     RotAngle = dynamic_cast<cf::TypeSys::VarT<float>*>    (Sel[SelNr]->GetTransform()->GetMemberVars().Find("Rotation"));

            // As an alternative to restoring the old value and having the command (re-)set the new one,
            // we could use the CommandSetCompVarT ctor that takes the variable at the new value directly.
            // However, this ctor requires the old value to be passed in cf::Network::StateT form...
            if (Pos && Pos->Get() != m_WindowStates[SelNr].Position)
            {
                const Vector2fT NewValue = Pos->Get();
                Pos->Set(m_WindowStates[SelNr].Position);

                SubCommands.PushBack(new CommandSetCompVarT<Vector2fT>(m_GuiDocument, *Pos, NewValue));
            }

            if (Size && Size->Get() != m_WindowStates[SelNr].Size)
            {
                const Vector2fT NewValue = Size->Get();
                Size->Set(m_WindowStates[SelNr].Size);

                SubCommands.PushBack(new CommandSetCompVarT<Vector2fT>(m_GuiDocument, *Size, NewValue));
            }

            if (RotAngle && RotAngle->Get() != m_WindowStates[SelNr].Rotation)
            {
                const float NewValue = RotAngle->Get();
                RotAngle->Set(m_WindowStates[SelNr].Rotation);

                SubCommands.PushBack(new CommandSetCompVarT<float>(m_GuiDocument, *RotAngle, NewValue));
            }
        }

        if (SubCommands.Size() > 0)
        {
            switch (m_TransState)
            {
                case TRANSLATE: m_Parent->SubmitCommand(new CommandMacroT(SubCommands, Sel.Size() == 1 ? "Move window"   : "Move windows"  )); break;
                case ROTATE:    m_Parent->SubmitCommand(new CommandMacroT(SubCommands, Sel.Size() == 1 ? "Rotate window" : "Rotate windows")); break;
                default:        m_Parent->SubmitCommand(new CommandMacroT(SubCommands, Sel.Size() == 1 ? "Scale window"  : "Scale windows" )); break;
            }
        }

        m_TransformSelection=false;
        m_RotStartAngle=0.0f;
        m_TransState=NONE;
    }

    m_WindowStates.Overwrite();
    m_TransformationStart=false;

    return true;
}


bool ToolSelectionT::OnMouseMove(RenderWindowT* RenderWindow, wxMouseEvent& ME)
{
    if (m_TransformationStart)
    {
        m_TransformSelection=true;
        m_TransformationStart=false;
    }

    // Translate client mouse position to GUI mouse position.
    Vector3fT MousePosGUI=RenderWindow->ClientToGui(ME.GetX(), ME.GetY());

    if (m_TransformSelection)
    {
        MousePosGUI=m_Parent->SnapToGrid(MousePosGUI);

        if (MousePosGUI.x==m_LastMousePosX && MousePosGUI.y==m_LastMousePosY) return true;

        const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Selection=m_GuiDocument->GetSelection();

        float DeltaX=MousePosGUI.x-m_LastMousePosX;
        float DeltaY=MousePosGUI.y-m_LastMousePosY;

        // Move the selection to the current mouse position.
        for (unsigned long SelNr=0; SelNr<Selection.Size(); SelNr++)
        {
            Vector2fT Pos      = Selection[SelNr]->GetTransform()->GetPos();
            Vector2fT Size     = Selection[SelNr]->GetTransform()->GetSize();
            float     RotAngle = Selection[SelNr]->GetTransform()->GetRotAngle();

            switch (m_TransState)
            {
                case TRANSLATE:
                    Pos.x += DeltaX; Pos.x = m_Parent->SnapToGrid(Pos.x);
                    Pos.y += DeltaY; Pos.y = m_Parent->SnapToGrid(Pos.y);
                    break;

                case SCALE_N:
                    Pos.y  += DeltaY; Pos.y  = m_Parent->SnapToGrid(Pos.y);
                    Size.y -= DeltaY; Size.y = m_Parent->SnapToGrid(Size.y);
                    break;

                case SCALE_NE:
                    Pos.y  += DeltaY; Pos.y  = m_Parent->SnapToGrid(Pos.y);
                    Size.x += DeltaX; Size.x = m_Parent->SnapToGrid(Size.x);
                    Size.y -= DeltaY; Size.y = m_Parent->SnapToGrid(Size.y);
                    break;

                case SCALE_E:
                    Size.x += DeltaX; Size.x = m_Parent->SnapToGrid(Size.x);
                    break;

                case SCALE_SE:
                    Size.x += DeltaX; Size.x = m_Parent->SnapToGrid(Size.x);
                    Size.y += DeltaY; Size.y = m_Parent->SnapToGrid(Size.y);
                    break;

                case SCALE_S:
                    Size.y += DeltaY; Size.y = m_Parent->SnapToGrid(Size.y);
                    break;

                case SCALE_SW:
                    Pos.x  += DeltaX; Pos.x  = m_Parent->SnapToGrid(Pos.x);
                    Size.x -= DeltaX; Size.x = m_Parent->SnapToGrid(Size.x);
                    Size.y += DeltaY;
                    break;

                case SCALE_W:
                    Pos.x  += DeltaX; Pos.x  = m_Parent->SnapToGrid(Pos.x);
                    Size.x -= DeltaX; Size.x = m_Parent->SnapToGrid(Size.x);
                    break;

                case SCALE_NW:
                    Pos.x  += DeltaX; Pos.x  = m_Parent->SnapToGrid(Pos.x);
                    Pos.y  += DeltaY; Pos.y  = m_Parent->SnapToGrid(Pos.y);
                    Size.x -= DeltaX; Size.x = m_Parent->SnapToGrid(Size.x);
                    Size.y -= DeltaY; Size.y = m_Parent->SnapToGrid(Size.y);
                    break;

                case ROTATE:
                {
                    RotAngle = GetAngle(MousePosGUI.x, MousePosGUI.y, (Pos.x + Size.x)/2.0f, (Pos.y + Size.y)/2.0f) - m_RotStartAngle;
                    break;
                }

                case NONE:
                    wxASSERT(false);
                    break;
            }

            // Enforce minimum size.
            if (Size.x < 10.0f) Size.x = 10.0f;
            if (Size.y < 10.0f) Size.y = 10.0f;

            Selection[SelNr]->GetTransform()->SetPos(Pos);
            Selection[SelNr]->GetTransform()->SetSize(Size);
            Selection[SelNr]->GetTransform()->SetRotAngle(RotAngle);
        }

        RenderWindow->Refresh(false);
        RenderWindow->Update();
    }
    else
    {
        IntrusivePtrT<cf::GuiSys::WindowT> MouseOverWindow=m_GuiDocument->GetRootWindow()->Find(MousePosGUI.x, MousePosGUI.y);

        // If window under the cursor is selected.
        if (!MouseOverWindow.IsNull() && GuiDocumentT::GetSibling(MouseOverWindow)->IsSelected())
        {
            const Vector2fT WinSize = MouseOverWindow->GetTransform()->GetSize();

            // Get absolute window position then get relative cursor position to this window.
            float PosX=0.0f;
            float PosY=0.0f;

            MouseOverWindow->GetAbsolutePos(PosX, PosY);

            PosX=MousePosGUI.x-PosX;
            PosY=MousePosGUI.y-PosY;

            if (PosX < HANDLE_WIDTH)
            {
                     if (PosY <           HANDLE_WIDTH) { m_TransState=SCALE_NW; RenderWindow->SetCursor(wxCursor(wxCURSOR_SIZENWSE)); }
                else if (PosY > WinSize.y-HANDLE_WIDTH) { m_TransState=SCALE_SW; RenderWindow->SetCursor(wxCursor(wxCURSOR_SIZENESW)); }
                else                                    { m_TransState=SCALE_W;  RenderWindow->SetCursor(wxCursor(wxCURSOR_SIZEWE  )); }
            }
            else if (PosY < HANDLE_WIDTH)
            {
                     if (PosX > WinSize.x-HANDLE_WIDTH) { m_TransState=SCALE_NE; RenderWindow->SetCursor(wxCursor(wxCURSOR_SIZENESW)); }
                else                                    { m_TransState=SCALE_N;  RenderWindow->SetCursor(wxCursor(wxCURSOR_SIZENS  )); }
            }
            else if (PosX > WinSize.x-HANDLE_WIDTH)
            {
                     if (PosY > WinSize.y-HANDLE_WIDTH) { m_TransState=SCALE_SE; RenderWindow->SetCursor(wxCursor(wxCURSOR_SIZENWSE)); }
                else                                    { m_TransState=SCALE_E;  RenderWindow->SetCursor(wxCursor(wxCURSOR_SIZEWE  )); }
            }
            else if (PosY > WinSize.y-HANDLE_WIDTH)
            {
                // Only this one left.
                m_TransState=SCALE_S;
                RenderWindow->SetCursor(wxCursor(wxCURSOR_SIZENS));
            }
            else
            {
                // Mouse is deep inside window, so we translate the window.
                m_TransState=TRANSLATE;
                RenderWindow->SetCursor(wxCursor(wxCURSOR_SIZING));
            }
        }
        else
        {
            RenderWindow->SetCursor(*wxSTANDARD_CURSOR);
            m_TransState=NONE;
        }
    }

    m_LastMousePosX=MousePosGUI.x;
    m_LastMousePosY=MousePosGUI.y;

    return true;
}


bool ToolSelectionT::OnRMouseUp(RenderWindowT* RenderWindow, wxMouseEvent& ME)
{
    // Note that GetPopupMenuSelectionFromUser() temporarily disables UI updates for the window,
    // so our menu IDs used below should be doubly clash-free.
    enum
    {
        ID_MENU_CREATE_WINDOW_BASE=wxID_HIGHEST+1,
        ID_MENU_CREATE_WINDOW_EDIT,
        ID_MENU_CREATE_WINDOW_CHOICE,
        ID_MENU_CREATE_WINDOW_LISTBOX,
        ID_MENU_CREATE_WINDOW_MODEL
    };

    // Create a new window and use the top most window under the mouse cursor as parent.
    Vector3fT MousePosGUI=RenderWindow->ClientToGui(ME.GetX(), ME.GetY());
    IntrusivePtrT<cf::GuiSys::WindowT> Parent=m_GuiDocument->GetRootWindow()->Find(MousePosGUI.x, MousePosGUI.y);

    if (Parent.IsNull()) return false;

    wxMenu Menu;
    wxMenu* SubMenuCreate=new wxMenu();
    SubMenuCreate->Append(ID_MENU_CREATE_WINDOW_BASE,    "Window");
    // SubMenuCreate->Append(ID_MENU_CREATE_WINDOW_EDIT,    "Text Editor");
    // SubMenuCreate->Append(ID_MENU_CREATE_WINDOW_CHOICE,  "Choice Box");
    // SubMenuCreate->Append(ID_MENU_CREATE_WINDOW_LISTBOX, "List Box");
    // SubMenuCreate->Append(ID_MENU_CREATE_WINDOW_MODEL,   "Model Window");
    Menu.AppendSubMenu(SubMenuCreate, "Create");

    switch (RenderWindow->GetPopupMenuSelectionFromUser(Menu))
    {
        case ID_MENU_CREATE_WINDOW_BASE:
            m_Parent->SubmitCommand(new CommandCreateT(m_GuiDocument, Parent));
            break;

        default:
            break;
    }

    return true;
}
