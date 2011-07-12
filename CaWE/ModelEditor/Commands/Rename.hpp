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

#ifndef _MODELEDITOR_COMMAND_RENAME_HPP_
#define _MODELEDITOR_COMMAND_RENAME_HPP_

#include "../../CommandPattern.hpp"
#include "../ModelDocument.hpp"


namespace ModelEditor
{
    class ModelDocumentT;

    class CommandRenameT : public CommandT
    {
        public:

        CommandRenameT(ModelDocumentT* ModelDoc, ModelElementTypeT Type, unsigned int ElemNr, const wxString& NewName);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        std::string& GetStringRef() const;
        void UpdateAllObservers() const;

        ModelDocumentT*         m_ModelDoc;
        const ModelElementTypeT m_Type;
        const unsigned int      m_ElemNr;
        const std::string       m_NewName;
        const std::string       m_OldName;
    };
}

#endif
