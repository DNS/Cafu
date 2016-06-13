/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_COMMAND_PASTE_HPP_INCLUDED
#define CAFU_GUIEDITOR_COMMAND_PASTE_HPP_INCLUDED

#include "../../CommandPattern.hpp"

#include "Templates/Array.hpp"
#include "Templates/Pointer.hpp"


namespace cf { namespace GuiSys { class WindowT; } }


namespace GuiEditor
{
    class GuiDocumentT;

    class CommandPasteT : public CommandT
    {
        public:

        CommandPasteT(GuiDocumentT* GuiDocument, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows, IntrusivePtrT<cf::GuiSys::WindowT> NewParent);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        GuiDocumentT*                                m_GuiDocument;
        ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > m_Windows;
        IntrusivePtrT<cf::GuiSys::WindowT>           m_NewParent;
    };
}

#endif
