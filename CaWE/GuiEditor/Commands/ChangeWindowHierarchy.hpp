/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_COMMAND_CHANGE_WINDOW_HIERARCHY_HPP_INCLUDED
#define CAFU_GUIEDITOR_COMMAND_CHANGE_WINDOW_HIERARCHY_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Templates/Pointer.hpp"


namespace cf { namespace GuiSys { class WindowT; } }


namespace GuiEditor
{
    class GuiDocumentT;

    class CommandChangeWindowHierarchyT : public CommandT
    {
        public:

        CommandChangeWindowHierarchyT(GuiDocumentT* GuiDocument, IntrusivePtrT<cf::GuiSys::WindowT> Window, IntrusivePtrT<cf::GuiSys::WindowT> NewParent, unsigned long NewPosition);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        GuiDocumentT*                      m_GuiDocument;
        IntrusivePtrT<cf::GuiSys::WindowT> m_Window;
        IntrusivePtrT<cf::GuiSys::WindowT> m_NewParent;
        unsigned long                      m_NewPosition;
        IntrusivePtrT<cf::GuiSys::WindowT> m_OldParent;
        unsigned long                      m_OldPosition;
        const std::string                  m_OldName;
    };
}

#endif
