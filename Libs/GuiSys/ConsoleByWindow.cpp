/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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
