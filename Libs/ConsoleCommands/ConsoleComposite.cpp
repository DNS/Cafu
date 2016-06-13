/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
