/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_COMMAND_SELECT_HPP_INCLUDED
#define CAFU_GUIEDITOR_COMMAND_SELECT_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Templates/Pointer.hpp"


namespace cf { namespace GuiSys { class WindowT; } }


namespace GuiEditor
{
    class GuiDocumentT;

    class CommandSelectT : public CommandT
    {
        public:

        // Named constructors for easier command creation.
        static CommandSelectT* Clear (GuiDocumentT* GuiDocument);
        static CommandSelectT* Add   (GuiDocumentT* GuiDocument, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows);
        static CommandSelectT* Add   (GuiDocumentT* GuiDocument, IntrusivePtrT<cf::GuiSys::WindowT> Window);
        static CommandSelectT* Remove(GuiDocumentT* GuiDocument, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows);
        static CommandSelectT* Remove(GuiDocumentT* GuiDocument, IntrusivePtrT<cf::GuiSys::WindowT> Window);
        static CommandSelectT* Set   (GuiDocumentT* GuiDocument, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows);

        ~CommandSelectT();

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        // Only named constructors may create this command.
        CommandSelectT(GuiDocumentT* GuiDocument, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& OldSelection, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& NewSelection);

        GuiDocumentT* m_GuiDocument;

        const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > m_OldSelection;
        const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > m_NewSelection;
    };
}

#endif
