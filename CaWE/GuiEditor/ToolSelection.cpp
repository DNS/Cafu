/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ToolSelection.hpp"

#include "RenderWindow.hpp"
#include "GuiDocument.hpp"
#include "ChildFrame.hpp"

#include "Commands/Select.hpp"
#include "Commands/Create.hpp"
#include "Commands/Delete.hpp"

#include "../CursorMan.hpp"
#include "../SetCompVar.hpp"

#include "Math3D/Angles.hpp"

#include "wx/artprov.h"


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
      m_ToolState(TS_IDLE),
      m_DragState(NONE),
      m_RotStartAngle(0.0f),
      m_LastMousePos()
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
    const Vector2fT MousePosGUI = RenderWindow->ClientToGui(ME.GetX(), ME.GetY());

    IntrusivePtrT<cf::GuiSys::WindowT> ClickedWindow=m_GuiDocument->GetRootWindow()->Find(MousePosGUI);

    if (ClickedWindow.IsNull())
    {
        if (!ME.ControlDown())
            m_Parent->SubmitCommand(CommandSelectT::Clear(m_GuiDocument));

        return true;
    }

    if (!GuiDocumentT::GetSelComp(ClickedWindow)->IsSelected())
    {
        if (!ME.ControlDown())
            m_Parent->SubmitCommand(CommandSelectT::Clear(m_GuiDocument));

        m_Parent->SubmitCommand(CommandSelectT::Add(m_GuiDocument, ClickedWindow));
        return true;
    }
    else
    {
        if (ME.ControlDown())
        {
            m_Parent->SubmitCommand(CommandSelectT::Remove(m_GuiDocument, ClickedWindow));  // Toggle window selection.
            return true;
        }
    }


    m_DragState = GetHandle(MousePosGUI);

    if (m_DragState != NONE)
    {
        m_ToolState = TS_DRAG_HANDLE;

        // Backup the current transformation of the selected windows.
        const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Selection = m_GuiDocument->GetSelection();

        m_WindowStates.Overwrite();

        for (unsigned long SelNr=0; SelNr<Selection.Size(); SelNr++)
        {
            WindowStateT WindowState;

            WindowState.Position = Selection[SelNr]->GetTransform()->GetPos();
            WindowState.Size     = Selection[SelNr]->GetTransform()->GetSize();
            WindowState.Rotation = Selection[SelNr]->GetTransform()->GetRotAngle();

            m_WindowStates.PushBack(WindowState);
        }

        m_RotStartAngle = GetAngle(MousePosGUI.x, MousePosGUI.y,
            (ClickedWindow->GetTransform()->GetPos().x + ClickedWindow->GetTransform()->GetSize().x)/2.0f,
            (ClickedWindow->GetTransform()->GetPos().y + ClickedWindow->GetTransform()->GetSize().y)/2.0f);

        m_LastMousePos = MousePosGUI;
    }

    return true;
}


bool ToolSelectionT::OnLMouseUp(RenderWindowT* RenderWindow, wxMouseEvent& ME)
{
    if (m_ToolState == TS_DRAG_HANDLE)
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

                SubCommands.PushBack(new CommandSetCompVarT<Vector2fT>(m_GuiDocument->GetAdapter(), *Pos, NewValue));
            }

            if (Size && Size->Get() != m_WindowStates[SelNr].Size)
            {
                const Vector2fT NewValue = Size->Get();
                Size->Set(m_WindowStates[SelNr].Size);

                SubCommands.PushBack(new CommandSetCompVarT<Vector2fT>(m_GuiDocument->GetAdapter(), *Size, NewValue));
            }

            if (RotAngle && RotAngle->Get() != m_WindowStates[SelNr].Rotation)
            {
                const float NewValue = RotAngle->Get();
                RotAngle->Set(m_WindowStates[SelNr].Rotation);

                SubCommands.PushBack(new CommandSetCompVarT<float>(m_GuiDocument->GetAdapter(), *RotAngle, NewValue));
            }
        }

        if (SubCommands.Size() > 0)
        {
            switch (m_DragState)
            {
                case TRANSLATE: m_Parent->SubmitCommand(new CommandMacroT(SubCommands, Sel.Size() == 1 ? "Move window"   : "Move windows"  )); break;
                case ROTATE:    m_Parent->SubmitCommand(new CommandMacroT(SubCommands, Sel.Size() == 1 ? "Rotate window" : "Rotate windows")); break;
                default:        m_Parent->SubmitCommand(new CommandMacroT(SubCommands, Sel.Size() == 1 ? "Scale window"  : "Scale windows" )); break;
            }
        }
    }

    m_ToolState = TS_IDLE;
    m_DragState = NONE;

    m_WindowStates.Overwrite();
    return true;
}


bool ToolSelectionT::OnMouseMove(RenderWindowT* RenderWindow, wxMouseEvent& ME)
{
    // Translate client mouse position to GUI mouse position.
    Vector2fT MousePosGUI = RenderWindow->ClientToGui(ME.GetX(), ME.GetY());

    switch (m_ToolState)
    {
        case TS_IDLE:
        {
            RenderWindow->SetCursor(SuggestCursor(GetHandle(MousePosGUI)));
            break;
        }

        case TS_DRAG_HANDLE:
        {
            MousePosGUI = m_Parent->SnapToGrid(MousePosGUI);

            if (MousePosGUI == m_LastMousePos) break;

            const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Selection=m_GuiDocument->GetSelection();

            const Vector2fT Delta = MousePosGUI - m_LastMousePos;

            // Move the selection to the current mouse position.
            for (unsigned long SelNr=0; SelNr<Selection.Size(); SelNr++)
            {
                Vector2fT Pos      = Selection[SelNr]->GetTransform()->GetPos();
                Vector2fT Size     = Selection[SelNr]->GetTransform()->GetSize();
                float     RotAngle = Selection[SelNr]->GetTransform()->GetRotAngle();

                switch (m_DragState)
                {
                    case TRANSLATE:
                        Pos.x += Delta.x; Pos.x = m_Parent->SnapToGrid(Pos.x);
                        Pos.y += Delta.y; Pos.y = m_Parent->SnapToGrid(Pos.y);
                        break;

                    case SCALE_N:
                        Pos.y  += Delta.y; Pos.y  = m_Parent->SnapToGrid(Pos.y);
                        Size.y -= Delta.y; Size.y = m_Parent->SnapToGrid(Size.y);
                        break;

                    case SCALE_NE:
                        Pos.y  += Delta.y; Pos.y  = m_Parent->SnapToGrid(Pos.y);
                        Size.x += Delta.x; Size.x = m_Parent->SnapToGrid(Size.x);
                        Size.y -= Delta.y; Size.y = m_Parent->SnapToGrid(Size.y);
                        break;

                    case SCALE_E:
                        Size.x += Delta.x; Size.x = m_Parent->SnapToGrid(Size.x);
                        break;

                    case SCALE_SE:
                        Size.x += Delta.x; Size.x = m_Parent->SnapToGrid(Size.x);
                        Size.y += Delta.y; Size.y = m_Parent->SnapToGrid(Size.y);
                        break;

                    case SCALE_S:
                        Size.y += Delta.y; Size.y = m_Parent->SnapToGrid(Size.y);
                        break;

                    case SCALE_SW:
                        Pos.x  += Delta.x; Pos.x  = m_Parent->SnapToGrid(Pos.x);
                        Size.x -= Delta.x; Size.x = m_Parent->SnapToGrid(Size.x);
                        Size.y += Delta.y;
                        break;

                    case SCALE_W:
                        Pos.x  += Delta.x; Pos.x  = m_Parent->SnapToGrid(Pos.x);
                        Size.x -= Delta.x; Size.x = m_Parent->SnapToGrid(Size.x);
                        break;

                    case SCALE_NW:
                        Pos.x  += Delta.x; Pos.x  = m_Parent->SnapToGrid(Pos.x);
                        Pos.y  += Delta.y; Pos.y  = m_Parent->SnapToGrid(Pos.y);
                        Size.x -= Delta.x; Size.x = m_Parent->SnapToGrid(Size.x);
                        Size.y -= Delta.y; Size.y = m_Parent->SnapToGrid(Size.y);
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
            break;
        }
    }

    m_LastMousePos = MousePosGUI;
    return true;
}


// This function has been duplicated into other modules, too... can we reconcile them?
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


bool ToolSelectionT::OnRMouseUp(RenderWindowT* RenderWindow, wxMouseEvent& ME)
{
    // Note that GetPopupMenuSelectionFromUser() temporarily disables UI updates for the window,
    // so our menu IDs used below should be doubly clash-free.
    enum
    {
        ID_MENU_CREATE_WINDOW = wxID_HIGHEST + 1,
    };

    // Create a new window and use the top most window under the mouse cursor as parent.
    const Vector2fT                    MousePosGUI = RenderWindow->ClientToGui(ME.GetX(), ME.GetY());
    IntrusivePtrT<cf::GuiSys::WindowT> Parent      = m_GuiDocument->GetRootWindow()->Find(MousePosGUI);

    wxMenu Menu;
    AppendMI(Menu, ID_MENU_CREATE_WINDOW, "Create Window", "window-new", !Parent.IsNull());

    switch (RenderWindow->GetPopupMenuSelectionFromUser(Menu))
    {
        case ID_MENU_CREATE_WINDOW:
            if (!Parent.IsNull())
                m_Parent->SubmitCommand(new CommandCreateT(m_GuiDocument, Parent));
            break;

        default:
            break;
    }

    return true;
}


ToolSelectionT::TrafoHandleT ToolSelectionT::GetHandle(const Vector2fT& GuiPos) const
{
    IntrusivePtrT<cf::GuiSys::WindowT> HitWin = m_GuiDocument->GetRootWindow()->Find(GuiPos);

    if (HitWin.IsNull() || !GuiDocumentT::GetSelComp(HitWin)->IsSelected())
        return NONE;

    const Vector2fT RelPos  = GuiPos - HitWin->GetAbsolutePos();
    const Vector2fT WinSize = HitWin->GetTransform()->GetSize();

    if (RelPos.x < HANDLE_WIDTH)
    {
        if (RelPos.y <             HANDLE_WIDTH) return SCALE_NW;
        if (RelPos.y > WinSize.y - HANDLE_WIDTH) return SCALE_SW;
        return SCALE_W;
    }

    if (RelPos.y < HANDLE_WIDTH)
    {
        if (RelPos.x > WinSize.x - HANDLE_WIDTH) return SCALE_NE;
        return SCALE_N;
    }

    if (RelPos.x > WinSize.x - HANDLE_WIDTH)
    {
        if (RelPos.y > WinSize.y - HANDLE_WIDTH) return SCALE_SE;
        return SCALE_E;
    }

    if (RelPos.y > WinSize.y - HANDLE_WIDTH)
    {
        return SCALE_S;
    }

    // Mouse is deep inside window, so we translate the window.
    return TRANSLATE;
}


wxCursor ToolSelectionT::SuggestCursor(ToolSelectionT::TrafoHandleT TrafoHandle) const
{
    switch (TrafoHandle)
    {
        case NONE:
            break;

        case TRANSLATE:
            return wxCursor(wxCURSOR_SIZING);

        case SCALE_N:
        case SCALE_S:
            return wxCursor(wxCURSOR_SIZENS);

        case SCALE_E:
        case SCALE_W:
            return wxCursor(wxCURSOR_SIZEWE);

        case SCALE_NE:
        case SCALE_SW:
            return wxCursor(wxCURSOR_SIZENESW);

        case SCALE_NW:
        case SCALE_SE:
            return wxCursor(wxCURSOR_SIZENWSE);

        case ROTATE:
            break;
    }

    return *wxSTANDARD_CURSOR;
}
