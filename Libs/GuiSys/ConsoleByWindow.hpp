/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUISYS_CONSOLE_BY_WINDOW_HPP_INCLUDED
#define CAFU_GUISYS_CONSOLE_BY_WINDOW_HPP_INCLUDED

#include "ConsoleCommands/Console.hpp"
#include "Templates/Pointer.hpp"


namespace cf
{
    namespace GuiSys
    {
        class ComponentTextT;
        class WindowT;


        /// This class implements the cf::ConsoleI interface by means of a cf::GuiSys::WindowT, thus providing us with a GuiSys-based console.
        /// It quasi acts as a kind of "mediator" between the cf::ConsoleI interface and the WindowT instance.
        /// Note that the target window must live longer than instances of this class!
        class ConsoleByWindowT : public cf::ConsoleI
        {
            public:

            /// Constructor for a console that is implemented by means of the given WindowT object.
            ConsoleByWindowT(IntrusivePtrT<WindowT> Win);

            // Implementation of the cf::ConsoleI interface.
            void Print(const std::string& s);
            void DevPrint(const std::string& s);
            void Warning(const std::string& s);
            void DevWarning(const std::string& s);


            private:

            IntrusivePtrT<WindowT>        m_Win;        ///< The "target" window that is supposed to receive the console output.
            IntrusivePtrT<ComponentTextT> m_TextComp;   ///< The "Text" component of m_Win.
        };
    }
}

#endif
