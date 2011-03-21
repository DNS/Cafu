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

#include "ToolSelection.hpp"

#include "RenderWindow.hpp"
#include "GuiDocument.hpp"
#include "ChildFrame.hpp"

#include "EditorData/Window.hpp"

#include "Commands/Select.hpp"
#include "Commands/Translate.hpp"
#include "Commands/Scale.hpp"
#include "Commands/Create.hpp"
#include "Commands/Delete.hpp"

#include "../CursorMan.hpp"

#include "GuiSys/Window.hpp"
#include "Math3D/Angles.hpp"


using namespace GuiEditor;


class ContextMenuT : public wxMenu
{
    public:

    enum
    {
        ID_MENU_CREATE_WINDOW_BASE=wxID_HIGHEST+1,
        ID_MENU_CREATE_WINDOW_EDIT,
        ID_MENU_CREATE_WINDOW_CHOICE,
        ID_MENU_CREATE_WINDOW_LISTBOX,
        ID_MENU_CREATE_WINDOW_MODEL
    };

    ContextMenuT()
        : wxMenu(),
          ID(-1)
    {
        wxMenu* SubMenuCreate=new wxMenu();
        SubMenuCreate->Append(ID_MENU_CREATE_WINDOW_BASE,    "Window");
        SubMenuCreate->Append(ID_MENU_CREATE_WINDOW_EDIT,    "Text Editor");
        SubMenuCreate->Append(ID_MENU_CREATE_WINDOW_CHOICE,  "Choice Box");
        SubMenuCreate->Append(ID_MENU_CREATE_WINDOW_LISTBOX, "List Box");
        SubMenuCreate->Append(ID_MENU_CREATE_WINDOW_MODEL,   "Model Window");

        // Create context menus.
        this->AppendSubMenu(SubMenuCreate, "Create");
    }

    int GetClickedMenuItem() { return ID; }


    protected:

    void OnMenuClick(wxCommandEvent& CE) { ID=CE.GetId(); }


    private:

    int ID;

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(ContextMenuT, wxMenu)
    EVT_MENU(wxID_ANY, ContextMenuT::OnMenuClick)
END_EVENT_TABLE()


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

    cf::GuiSys::WindowT* ClickedWindow=m_GuiDocument->GetRootWindow()->Find(MousePosGUI.x, MousePosGUI.y);

    if (!ClickedWindow)
    {
        if (!ME.ControlDown()) m_Parent->SubmitCommand(CommandSelectT::Clear(m_GuiDocument));
        return true;
    }

    if (!((EditorDataWindowT*)ClickedWindow->EditorData)->Selected)
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
    const ArrayT<cf::GuiSys::WindowT*>& Selection=m_GuiDocument->GetSelection();

    for (unsigned long SelNr=0; SelNr<Selection.Size(); SelNr++)
    {
        WindowStateT WindowState;

        WindowState.Position=Vector3fT(Selection[SelNr]->Rect[0], Selection[SelNr]->Rect[1], 0.0f);
        WindowState.Size    =Vector3fT(Selection[SelNr]->Rect[2], Selection[SelNr]->Rect[3], 0.0f);
        WindowState.Rotation=Selection[SelNr]->RotAngle;

        m_WindowStates.PushBack(WindowState);
    }

    // Send a mouse move event to make sure the transform state is set corretly (according to the position we clicked on).
    OnMouseMove(RenderWindow, ME);

    // Start transforming the selection.
    m_TransformationStart=true;

    m_RotStartAngle=GetAngle(MousePosGUI.x, MousePosGUI.y, (ClickedWindow->Rect[0]+ClickedWindow->Rect[2])/2.0f, (ClickedWindow->Rect[1]+ClickedWindow->Rect[3])/2.0f);
    return true;
}


bool ToolSelectionT::OnLMouseUp(RenderWindowT* RenderWindow, wxMouseEvent& ME)
{
    if (m_TransformSelection)
    {
        // Submit command according to transformation state.
        switch (m_TransState)
        {
            case TRANSLATE:
            {
                ArrayT<Vector3fT> Positions;

                for (unsigned long StateNr=0; StateNr<m_WindowStates.Size(); StateNr++)
                    Positions.PushBack(m_WindowStates[StateNr].Position);

                m_Parent->SubmitCommand(new CommandTranslateT(m_GuiDocument, m_GuiDocument->GetSelection(), Positions, true));
                break;
            }

            case SCALE_N:
            case SCALE_NE:
            case SCALE_E:
            case SCALE_SE:
            case SCALE_S:
            case SCALE_SW:
            case SCALE_W:
            case SCALE_NW:
            {
                ArrayT<Vector3fT> Positions;
                ArrayT<Vector3fT> Sizes;

                for (unsigned long StateNr=0; StateNr<m_WindowStates.Size(); StateNr++)
                {
                    Positions.PushBack(m_WindowStates[StateNr].Position);
                    Sizes    .PushBack(m_WindowStates[StateNr].Size);
                }

                m_Parent->SubmitCommand(new CommandScaleT(m_GuiDocument, m_GuiDocument->GetSelection(), Positions, Sizes, true));
                break;
            }

            case ROTATE:
                // TODO
                m_GuiDocument->UpdateAllObservers_Modified(m_GuiDocument->GetSelection(), WMD_TRANSFORMED);
                break;

            default:
                wxASSERT(false);
                break;
        }

        m_TransformSelection=false;
        m_RotStartAngle=0.0f;
        m_TransState=NONE;
    }

    m_WindowStates.Clear();
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

        const ArrayT<cf::GuiSys::WindowT*>& Selection=m_GuiDocument->GetSelection();

        float DeltaX=MousePosGUI.x-m_LastMousePosX;
        float DeltaY=MousePosGUI.y-m_LastMousePosY;

        // Move the selection to the current mouse position.
        for (unsigned long SelNr=0; SelNr<Selection.Size(); SelNr++)
        {
            float* Rect=&Selection[SelNr]->Rect[0];

            switch (m_TransState)
            {
                case TRANSLATE:
                    Rect[0]+=DeltaX; Rect[0]=m_Parent->SnapToGrid(Rect[0]);
                    Rect[1]+=DeltaY; Rect[1]=m_Parent->SnapToGrid(Rect[1]);
                    break;

                case SCALE_N:
                    Rect[1]+=DeltaY; Rect[1]=m_Parent->SnapToGrid(Rect[1]);
                    Rect[3]-=DeltaY; Rect[3]=m_Parent->SnapToGrid(Rect[3]);
                    break;

                case SCALE_NE:
                    Rect[1]+=DeltaY; Rect[1]=m_Parent->SnapToGrid(Rect[1]);
                    Rect[2]+=DeltaX; Rect[2]=m_Parent->SnapToGrid(Rect[2]);
                    Rect[3]-=DeltaY; Rect[3]=m_Parent->SnapToGrid(Rect[3]);
                    break;

                case SCALE_E:
                    Rect[2]+=DeltaX; Rect[2]=m_Parent->SnapToGrid(Rect[2]);
                    break;

                case SCALE_SE:
                    Rect[2]+=DeltaX; Rect[2]=m_Parent->SnapToGrid(Rect[2]);
                    Rect[3]+=DeltaY; Rect[3]=m_Parent->SnapToGrid(Rect[3]);
                    break;

                case SCALE_S:
                    Rect[3]+=DeltaY; Rect[3]=m_Parent->SnapToGrid(Rect[3]);
                    break;

                case SCALE_SW:
                    Rect[0]+=DeltaX; Rect[0]=m_Parent->SnapToGrid(Rect[0]);
                    Rect[2]-=DeltaX; Rect[2]=m_Parent->SnapToGrid(Rect[2]);
                    Rect[3]+=DeltaY;
                    break;

                case SCALE_W:
                    Rect[0]+=DeltaX; Rect[0]=m_Parent->SnapToGrid(Rect[0]);
                    Rect[2]-=DeltaX; Rect[2]=m_Parent->SnapToGrid(Rect[2]);
                    break;

                case SCALE_NW:
                    Rect[0]+=DeltaX; Rect[0]=m_Parent->SnapToGrid(Rect[0]);
                    Rect[1]+=DeltaY; Rect[1]=m_Parent->SnapToGrid(Rect[1]);
                    Rect[2]-=DeltaX; Rect[2]=m_Parent->SnapToGrid(Rect[2]);
                    Rect[3]-=DeltaY; Rect[3]=m_Parent->SnapToGrid(Rect[3]);
                    break;

                case ROTATE:
                {
                    // TODO.
                    float NewAngle=GetAngle(MousePosGUI.x, MousePosGUI.y, (Rect[0]+Rect[2])/2.0f, (Rect[1]+Rect[3])/2.0f);
                    NewAngle-=m_RotStartAngle;
                    Selection[SelNr]->RotAngle=NewAngle;
                    break;
                }

                case NONE:
                    wxASSERT(false);
                    break;
            }

            // Check Rect size.
            if (Rect[2]<10.0f) Rect[2]=10.0f;
            if (Rect[3]<10.0f) Rect[3]=10.0f;
        }

        RenderWindow->Refresh(false);
        RenderWindow->Update();
    }
    else
    {
        cf::GuiSys::WindowT* MouseOverWindow=m_GuiDocument->GetRootWindow()->Find(MousePosGUI.x, MousePosGUI.y);

        // If window under the cursor is selected.
        if (MouseOverWindow && ((EditorDataWindowT*)MouseOverWindow->EditorData)->Selected)
        {
            // Get window rect for window size.
            float* Rect=&MouseOverWindow->Rect[0];

            // Get absolute window position then get relative cursor position to this window.
            float PosX=0.0f;
            float PosY=0.0f;
            MouseOverWindow->GetAbsolutePos(PosX, PosY);
            PosX=MousePosGUI.x-PosX;
            PosY=MousePosGUI.y-PosY;

            if (PosX<HANDLE_WIDTH)
            {
                     if (PosY<        HANDLE_WIDTH) { m_TransState=SCALE_NW; RenderWindow->SetCursor(wxCursor(wxCURSOR_SIZENWSE)); }
                else if (PosY>Rect[3]-HANDLE_WIDTH) { m_TransState=SCALE_SW; RenderWindow->SetCursor(wxCursor(wxCURSOR_SIZENESW)); }
                else                                { m_TransState=SCALE_W;  RenderWindow->SetCursor(wxCursor(wxCURSOR_SIZEWE  )); }
            }
            else if (PosY<HANDLE_WIDTH)
            {
                     if (PosX>Rect[2]-HANDLE_WIDTH) { m_TransState=SCALE_NE; RenderWindow->SetCursor(wxCursor(wxCURSOR_SIZENESW)); }
                else                                { m_TransState=SCALE_N;  RenderWindow->SetCursor(wxCursor(wxCURSOR_SIZENS  )); }
            }
            else if (PosX>Rect[2]-HANDLE_WIDTH)
            {
                     if (PosY>Rect[3]-HANDLE_WIDTH) { m_TransState=SCALE_SE; RenderWindow->SetCursor(wxCursor(wxCURSOR_SIZENWSE)); }
                else                                { m_TransState=SCALE_E;  RenderWindow->SetCursor(wxCursor(wxCURSOR_SIZEWE  )); }
            }
            else if (PosY>Rect[3]-HANDLE_WIDTH)
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
    ContextMenuT ContextMenu;

    RenderWindow->PopupMenu(&ContextMenu);

    // Create a new window and use the top most window under the mouse cursor as parent.
    Vector3fT MousePosGUI      =RenderWindow->ClientToGui(ME.GetX(), ME.GetY());
    cf::GuiSys::WindowT* Parent=m_GuiDocument->GetRootWindow()->Find(MousePosGUI.x, MousePosGUI.y);

    if (!Parent) return false;

    switch (ContextMenu.GetClickedMenuItem())
    {
        case ContextMenuT::ID_MENU_CREATE_WINDOW_BASE:
            m_Parent->SubmitCommand(new CommandCreateT(m_GuiDocument, Parent));
            break;

        case ContextMenuT::ID_MENU_CREATE_WINDOW_EDIT:
            m_Parent->SubmitCommand(new CommandCreateT(m_GuiDocument, Parent, CommandCreateT::WINDOW_TEXTEDITOR));
            break;

        case ContextMenuT::ID_MENU_CREATE_WINDOW_CHOICE:
            m_Parent->SubmitCommand(new CommandCreateT(m_GuiDocument, Parent, CommandCreateT::WINDOW_CHOICE));
            break;

        case ContextMenuT::ID_MENU_CREATE_WINDOW_LISTBOX:
            m_Parent->SubmitCommand(new CommandCreateT(m_GuiDocument, Parent, CommandCreateT::WINDOW_LISTBOX));
            break;

        case ContextMenuT::ID_MENU_CREATE_WINDOW_MODEL:
            m_Parent->SubmitCommand(new CommandCreateT(m_GuiDocument, Parent, CommandCreateT::WINDOW_MODEL));
            break;
    }

    return true;
}
