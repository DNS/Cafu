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

#ifndef _GUIEDIT_EDITOR_DATA_HPP_
#define _GUIEDIT_EDITOR_DATA_HPP_

#include <string>


namespace cf
{
    namespace GuiSys
    {
        class WindowT;


        /// EditorDataT contains an editor interface to the WindowT class of the GuiSys.
        /// A GUI editor should derive its own data class with editor specific stuff from this one.
        /// EditorDataT is always associated with a WindowT (that MUST be passed to the constructor)
        /// and is deleted upon WindowT destruction.
        class EditorDataT
        {
            public:

            /// Constructor.
            /// @param GuiWindow The GUI window this editor data is created for.
            EditorDataT(WindowT* GuiWindow);

            /// Destructor.
            virtual ~EditorDataT();


            protected:

            WindowT*    m_GuiWindow;  ///< The "real" window from which this editor window was created.
        };
    }
}

#endif
