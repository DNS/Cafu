/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
