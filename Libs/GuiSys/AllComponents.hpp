/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#ifndef CAFU_GUISYS_ALL_COMPONENTS_HPP_INCLUDED
#define CAFU_GUISYS_ALL_COMPONENTS_HPP_INCLUDED

#include "TypeSys.hpp"


namespace cf
{
    namespace GuiSys
    {
        /// All classes in the ComponentBaseT hierarchy must register their TypeInfoT
        /// members with this TypeInfoManT instance.
        cf::TypeSys::TypeInfoManT& GetComponentTIM();

        /// Returns whether the given component type is fundamental to its parent window.
        /// Fundamental components are explicit C++ members of class WindowT. Users cannot delete them in the GUI
        /// Editor or via script, and cannot add new ones in the GUI Editor. (It is possible to add additional
        /// ones via script, but not useful.)
        /// Note that most components are not fundamental, but "custom": Users can freely add or delete them in
        /// the GUI Editor or via scripts.
        bool IsFundamental(const cf::TypeSys::TypeInfoT* CompType);
    }
}

#endif
