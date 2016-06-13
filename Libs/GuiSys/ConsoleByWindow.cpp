/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ConsoleByWindow.hpp"
#include "CompText.hpp"
#include "Window.hpp"


using namespace cf::GuiSys;


ConsoleByWindowT::ConsoleByWindowT(IntrusivePtrT<WindowT> Win)
    : m_Win(Win),
      m_TextComp(NULL)
{
    if (!Win.IsNull())
        m_TextComp = dynamic_pointer_cast<ComponentTextT>(Win->GetComponent("Text"));
}


void ConsoleByWindowT::Print(const std::string& s)
{
    if (m_TextComp.IsNull()) return;

    m_TextComp->AppendText(s);
}


void ConsoleByWindowT::DevPrint(const std::string& s)
{
    if (m_TextComp.IsNull()) return;

    m_TextComp->AppendText("[Dev] " + s);
}


void ConsoleByWindowT::Warning(const std::string& s)
{
    if (m_TextComp.IsNull()) return;

    m_TextComp->AppendText("Warning: " + s);
}


void ConsoleByWindowT::DevWarning(const std::string& s)
{
    if (m_TextComp.IsNull()) return;

    m_TextComp->AppendText("[Dev] Warning: " + s);
}
