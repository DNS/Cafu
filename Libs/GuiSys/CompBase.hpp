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

#ifndef CAFU_GUISYS_COMPONENT_BASE_HPP_INCLUDED
#define CAFU_GUISYS_COMPONENT_BASE_HPP_INCLUDED

//#include "Templates/Array.hpp"
#include "Templates/Pointer.hpp"


namespace cf
{
    namespace GuiSys
    {
        class WindowT;

        /// This is the base class for the components that a window is composed/aggregated of.
        class ComponentBaseT
        {
            public:

            ComponentBaseT(WindowT* Window);

            /// Returns the parent window that contains this component.
            IntrusivePtrT<WindowT> GetWindow() const { return m_Window; }

            /// This method is called after all components of the parent window have been loaded.
            /// The component can use the opportunity to search the window for "sibling" components
            /// that it depends on, and optionally store direct pointers to them.
            virtual void ResolveDependencies() { }


            private:

            WindowT* m_Window;    ///< The parent window that contains this component.
        };
    }
}

#endif
