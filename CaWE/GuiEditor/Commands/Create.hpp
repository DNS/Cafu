/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_COMMAND_CREATE_HPP_INCLUDED
#define CAFU_GUIEDITOR_COMMAND_CREATE_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Templates/Pointer.hpp"


namespace cf { namespace GuiSys { class WindowT; } }


namespace GuiEditor
{
    class GuiDocumentT;

    class CommandCreateT : public CommandT
    {
        public:

        CommandCreateT(GuiDocumentT* GuiDocument, IntrusivePtrT<cf::GuiSys::WindowT> Parent);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        GuiDocumentT* m_GuiDocument;

        IntrusivePtrT<cf::GuiSys::WindowT>                 m_Parent;
        IntrusivePtrT<cf::GuiSys::WindowT>                 m_NewWindow;
        const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > m_OldSelection;
    };
}

#endif
