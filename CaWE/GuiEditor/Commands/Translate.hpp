/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _GUIEDITOR_COMMAND_TRANSLATE_HPP_
#define _GUIEDITOR_COMMAND_TRANSLATE_HPP_

#include "../CommandPattern.hpp"

#include "Math3D/Vector3.hpp"


namespace cf { namespace GuiSys { class WindowT; } }


namespace GuiEditor
{
    class GuiDocumentT;

    class CommandTranslateT : public CommandT
    {
        public:

        /// Translates a set of windows.
        /// @param Windows The windows to translate.
        /// @param Position Depending on parameter Done these are the positions where the windows should be moved or the original
        ///                 window positions.
        /// @param Done Determines whether the windows have already been moved or should be moved by the command on first do.
        CommandTranslateT(GuiDocumentT* GuiDocument, const ArrayT<cf::GuiSys::WindowT*>& Windows, const ArrayT<Vector3fT>& Positions, bool Done=false);
        ~CommandTranslateT();

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        GuiDocumentT* m_GuiDocument;

        const ArrayT<cf::GuiSys::WindowT*> m_Windows;

        ArrayT<Vector3fT> m_OldPositions;
        ArrayT<Vector3fT> m_NewPositions;
    };
}

#endif
