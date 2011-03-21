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

#include "ConsoleComposite.hpp"


using namespace cf;


CompositeConsoleT::CompositeConsoleT()
{
}


bool CompositeConsoleT::Attach(ConsoleI* c)
{
    if (m_Consoles.Find(c)>=0) return false;

    m_Consoles.PushBack(c);
    return true;
}


bool CompositeConsoleT::Detach(ConsoleI* c)
{
    for (unsigned long ConNr=0; ConNr<m_Consoles.Size(); ConNr++)
        if (m_Consoles[ConNr]==c)
        {
            m_Consoles.RemoveAtAndKeepOrder(ConNr);
            return true;
        }

    return false;
}


void CompositeConsoleT::Print(const std::string& s)
{
    for (unsigned long ConNr=0; ConNr<m_Consoles.Size(); ConNr++)
        m_Consoles[ConNr]->Print(s);
}


void CompositeConsoleT::DevPrint(const std::string& s)
{
    for (unsigned long ConNr=0; ConNr<m_Consoles.Size(); ConNr++)
        m_Consoles[ConNr]->DevPrint(s);
}


void CompositeConsoleT::Warning(const std::string& s)
{
    for (unsigned long ConNr=0; ConNr<m_Consoles.Size(); ConNr++)
        m_Consoles[ConNr]->Warning(s);
}


void CompositeConsoleT::DevWarning(const std::string& s)
{
    for (unsigned long ConNr=0; ConNr<m_Consoles.Size(); ConNr++)
        m_Consoles[ConNr]->DevWarning(s);
}
