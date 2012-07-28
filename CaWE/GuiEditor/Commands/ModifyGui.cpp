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

#include "ModifyGui.hpp"

#include "../GuiDocument.hpp"
#include "GuiSys/Window.hpp"


using namespace GuiEditor;


CommandModifyGuiT* CommandModifyGuiT::Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, const wxString& NewValue)
{
    CommandModifyGuiT* Command=new CommandModifyGuiT(GuiDocument, PropertyName);

    Command->m_NewString=NewValue;

    return Command;
}


CommandModifyGuiT* CommandModifyGuiT::Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, int NewValue)
{
    CommandModifyGuiT* Command=new CommandModifyGuiT(GuiDocument, PropertyName);

    Command->m_NewInt=NewValue;

    return Command;
}


CommandModifyGuiT* CommandModifyGuiT::Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, long NewValue)
{
    CommandModifyGuiT* Command=new CommandModifyGuiT(GuiDocument, PropertyName);

    Command->m_NewLong=NewValue;

    return Command;
}


CommandModifyGuiT* CommandModifyGuiT::Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, float NewValue)
{
    CommandModifyGuiT* Command=new CommandModifyGuiT(GuiDocument, PropertyName);

    Command->m_NewFloat=NewValue;

    return Command;
}


CommandModifyGuiT* CommandModifyGuiT::Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, bool NewValue)
{
    CommandModifyGuiT* Command=new CommandModifyGuiT(GuiDocument, PropertyName);

    Command->m_NewBool=NewValue;

    return Command;
}


CommandModifyGuiT* CommandModifyGuiT::Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, const wxColour& NewValue)
{
    CommandModifyGuiT* Command=new CommandModifyGuiT(GuiDocument, PropertyName);

    Command->m_NewColor=NewValue;

    return Command;
}


bool CommandModifyGuiT::Do()
{
    wxASSERT(!m_Done);

    if (m_Done) return false;

    if (m_PropertyName=="Activate")
    {
        m_OldBool=m_GuiDocument->GetGuiProperties().Activate;
        m_GuiDocument->GetGuiProperties().Activate=m_NewBool;
    }
    else if (m_PropertyName=="Interactive")
    {
        m_OldBool=m_GuiDocument->GetGuiProperties().Interactive;
        m_GuiDocument->GetGuiProperties().Interactive=m_NewBool;
    }
    else if (m_PropertyName=="ShowMouse")
    {
        m_OldBool=m_GuiDocument->GetGuiProperties().ShowMouse;
        m_GuiDocument->GetGuiProperties().ShowMouse=m_NewBool;
    }
    else if (m_PropertyName=="DefaultFocus")
    {
        m_OldString=m_GuiDocument->GetGuiProperties().DefaultFocus;
        m_GuiDocument->GetGuiProperties().DefaultFocus=m_NewString;
    }
    else
    {
        return false;
    }

    m_GuiDocument->UpdateAllObservers_GuiPropertyModified();

    m_Done=true;

    return true;
}


void CommandModifyGuiT::Undo()
{
    wxASSERT(m_Done);

    if (!m_Done) return;

    if (m_PropertyName=="Activate")
        m_GuiDocument->GetGuiProperties().Activate=m_OldBool;
    else if (m_PropertyName=="Interactive")
        m_GuiDocument->GetGuiProperties().Interactive=m_OldBool;
    else if (m_PropertyName=="ShowMouse")
        m_GuiDocument->GetGuiProperties().ShowMouse=m_OldBool;
    else if (m_PropertyName=="DefaultFocus")
        m_GuiDocument->GetGuiProperties().DefaultFocus=m_OldString;
    else
        return;

    m_GuiDocument->UpdateAllObservers_GuiPropertyModified();

    m_Done=false;
}


wxString CommandModifyGuiT::GetName() const
{
    return "Modify "+m_PropertyName;
}


CommandModifyGuiT::CommandModifyGuiT(GuiDocumentT* GuiDocument, const wxString& PropertyName)
    : CommandT(),
      m_GuiDocument(GuiDocument),
      m_PropertyName(PropertyName),
      m_NewString(""),
      m_NewInt(0),
      m_NewLong(0),
      m_NewFloat(0.0f),
      m_NewBool(false),
      m_NewColor(),
      m_OldString(""),
      m_OldInt(0),
      m_OldLong(0),
      m_OldFloat(0.0f),
      m_OldBool(false),
      m_OldColor()
{
}
