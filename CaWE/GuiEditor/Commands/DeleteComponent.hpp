/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_COMMAND_DELETE_COMPONENT_HPP_INCLUDED
#define CAFU_GUIEDITOR_COMMAND_DELETE_COMPONENT_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Templates/Pointer.hpp"


namespace cf { namespace GuiSys { class ComponentBaseT; } }
namespace cf { namespace GuiSys { class WindowT; } }


namespace GuiEditor
{
    class GuiDocumentT;

    class CommandDeleteComponentT : public CommandT
    {
        public:

        CommandDeleteComponentT(GuiDocumentT* GuiDocument, IntrusivePtrT<cf::GuiSys::WindowT> Window, unsigned long Index);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        GuiDocumentT*                             m_GuiDocument;
        IntrusivePtrT<cf::GuiSys::WindowT>        m_Window;
        IntrusivePtrT<cf::GuiSys::ComponentBaseT> m_Component;
        const unsigned long                       m_Index;
    };
}

#endif
