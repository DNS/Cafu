/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_COMMAND_DELETE_HPP_INCLUDED
#define CAFU_GUIEDITOR_COMMAND_DELETE_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Templates/Pointer.hpp"


namespace cf { namespace GuiSys { class WindowT; } }


namespace GuiEditor
{
    class CommandSelectT;
    class GuiDocumentT;


    class CommandDeleteT : public CommandT
    {
        public:

        CommandDeleteT(GuiDocumentT* GuiDocument, IntrusivePtrT<cf::GuiSys::WindowT> Window);
        CommandDeleteT(GuiDocumentT* GuiDocument, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows);
        ~CommandDeleteT();

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        GuiDocumentT* m_GuiDocument;

        ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > m_Windows;
        ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > m_Parents;
        ArrayT<unsigned long>                        m_Indices;
        CommandSelectT*                              m_CommandSelect;   ///< The command that unselects all windows before they are deleted.
    };
}

#endif
