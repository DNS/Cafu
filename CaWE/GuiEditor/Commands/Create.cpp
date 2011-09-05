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

#include "Create.hpp"

#include "../GuiDocument.hpp"

#include "../EditorData/Window.hpp"

#include "GuiSys/Window.hpp"
#include "GuiSys/WindowChoice.hpp"
#include "GuiSys/WindowEdit.hpp"
#include "GuiSys/WindowListBox.hpp"
#include "GuiSys/WindowModel.hpp"
#include "GuiSys/WindowCreateParams.hpp"

#include "Math3D/Vector3.hpp"


using namespace GuiEditor;


CommandCreateT::CommandCreateT(GuiDocumentT* GuiDocument, cf::GuiSys::WindowT* Parent, WindowTypeE Type)
    : m_GuiDocument(GuiDocument),
      m_Parent(Parent),
      m_Type(Type),
      m_NewWindow(NULL),
      m_OldSelection(m_GuiDocument->GetSelection())
{
}


CommandCreateT::~CommandCreateT()
{
    if (!m_Done)
        delete m_NewWindow;
}


bool CommandCreateT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    if (!m_NewWindow)
    {
        cf::GuiSys::WindowCreateParamsT CreateParams(*m_GuiDocument->GetGui());

        // Create window and editor data.
        switch (m_Type)
        {
            case WINDOW_BASIC:
                m_NewWindow=new cf::GuiSys::WindowT(CreateParams);
                break;

            case WINDOW_TEXTEDITOR:
                m_NewWindow=new cf::GuiSys::EditWindowT(CreateParams);
                break;

            case WINDOW_CHOICE:
                m_NewWindow=new cf::GuiSys::ChoiceT(CreateParams);
                break;

            case WINDOW_LISTBOX:
                m_NewWindow=new cf::GuiSys::ListBoxT(CreateParams);
                break;

            case WINDOW_MODEL:
                m_NewWindow=new cf::GuiSys::ModelWindowT(CreateParams);
                break;
        }

        // Set windows parent (this has to be done BEFORE creating the editor data below, so the window name can be
        // made unique when creating the editor data).
        m_NewWindow->Parent=m_Parent;

        new EditorDataWindowT(m_NewWindow, m_GuiDocument);

        // Set a window default size and center it on its parent.
        // If the size is larger than parentsize/2, set it to parentsize/2.
        Vector3fT Size(100.0f, 50.0f, 0.0f);

        if (Size.x>m_Parent->Rect[2]/2.0f) Size.x=m_Parent->Rect[2]/2.0f;
        if (Size.y>m_Parent->Rect[3]/2.0f) Size.y=m_Parent->Rect[3]/2.0f;

        Vector3fT Position((m_Parent->Rect[2]-Size.x)/2.0f, (m_Parent->Rect[3]-Size.y)/2.0f, 0.0f);

        m_NewWindow->Rect[0]=Position.x;
        m_NewWindow->Rect[1]=Position.y;
        m_NewWindow->Rect[2]=Size.x;
        m_NewWindow->Rect[3]=Size.y;
    }

    m_Parent->Children.PushBack(m_NewWindow);

    ArrayT<cf::GuiSys::WindowT*> NewSelection;
    NewSelection.PushBack(m_NewWindow);

    m_GuiDocument->SetSelection(NewSelection);

    m_GuiDocument->UpdateAllObservers_Created(m_NewWindow);
    m_GuiDocument->UpdateAllObservers_SelectionChanged(m_OldSelection, NewSelection);

    m_Done=true;
    return true;
}


void CommandCreateT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    m_Parent->Children.DeleteBack();

    ArrayT<cf::GuiSys::WindowT*> NewSelection;
    NewSelection.PushBack(m_NewWindow);

    m_GuiDocument->SetSelection(m_OldSelection);

    m_GuiDocument->UpdateAllObservers_Deleted(m_NewWindow);
    m_GuiDocument->UpdateAllObservers_SelectionChanged(NewSelection, m_OldSelection);

    m_Done=false;
}


wxString CommandCreateT::GetName() const
{
    return "Create new window";
}
