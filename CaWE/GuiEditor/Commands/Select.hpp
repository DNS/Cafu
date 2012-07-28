/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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
