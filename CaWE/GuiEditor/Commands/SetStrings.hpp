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

#ifndef _GUIEDITOR_SET_STRINGS_HPP_
#define _GUIEDITOR_SET_STRINGS_HPP_

#include "../../CommandPattern.hpp"


namespace GuiEditor
{
    class GuiDocumentT;
    class EditorWindowT;

    class CommandSetStringsT : public CommandT
    {
        public:

        CommandSetStringsT(GuiDocumentT* GuiDoc, const EditorWindowT* Win, const wxString& PropertyName,
            ArrayT<std::string>& Strings, const ArrayT<std::string>& NewStrings);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        GuiDocumentT*             m_GuiDoc;
        const EditorWindowT*      m_Win;
        const wxString            m_PropertyName;
        ArrayT<std::string>&      m_Strings;
        const ArrayT<std::string> m_NewStrings;
        const ArrayT<std::string> m_OldStrings;
    };
}

#endif
